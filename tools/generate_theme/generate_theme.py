#!/usr/bin/env python3
"""Generate a Theme Area binary from one or more theme folders.

The binary layout matches the ThemeAreaHeader/ThemeEntry structures declared
in: shared/ion/src/device/include/n0120/config/board.h

    struct ThemeAreaHeader {
      uint32_t magic;
      uint32_t version;
      uint32_t themeCount;
    };                                              // 12 bytes, no padding

    struct ThemeEntry {
      char     name[16];
      uint8_t  isWallpaper;
      uint8_t  isPalette;
      uint8_t  isIcons;
      uint8_t  reserved;
      uint32_t wallpaperOffset;
      uint32_t wallpaperSize;
      uint32_t paletteOffset;
      uint32_t paletteSize;
      uint32_t iconOffsets[12];
      uint32_t iconSizes[12];
    };                                              // 132 bytes, no padding

Binary layout produced by this script:

    [ThemeAreaHeader]            12 bytes, at ThemeAreaStart + 0
    [ThemeEntry] * themeCount    132 bytes each, immediately after the header
    [asset data]                 wallpapers / palettes / icons, back to back
    [zero padding]                up to THEME_AREA_SIZE (256 KiB)

All offset fields inside a ThemeEntry (wallpaperOffset, paletteOffset,
iconOffsets[i]) are ABSOLUTE byte offsets from the start of the Theme Area
(i.e. from ThemeAreaStart), so firmware code can dereference them directly as
`ThemeAreaStart + entry.wallpaperOffset`. An offset of 0 means "absent",
which is always safe because byte 0 of the area is the header's magic field
and can never be a real asset.

Usage:
    python generate_theme.py input output.bin
    python generate_theme.py input/1 output.bin

Each theme folder may contain:
    - name.txt
    - palette.txt        (up to 64 lines: "<name> 0x<RRGGBB>")
    - wallpaper.png       (optional)
    - <fixed icon names>.png (optional, up to 12, see FIXED_ICON_FILES)

The generated .bin can be flashed directly in the Theme Area region.
A human-readable `<output>.txt` report is written alongside it.
"""

from __future__ import annotations

import argparse
import re
import struct
from pathlib import Path

import lz4.block

try:
    import png
except ImportError:  # pragma: no cover - fallback for minimal environments
    png = None

try:
    from PIL import Image
except ImportError:  # pragma: no cover - fallback for minimal environments
    Image = None

# --------------------------------------------------------------------------
# Constants mirrored from board.h - keep these in sync manually, there is no
# way to parse the C++ header automatically here.
# --------------------------------------------------------------------------
MAGIC = 0x87654321
VERSION = 1
THEME_COUNT_MAX = 4
THEME_ICON_COUNT = 12
THEME_NAME_LENGTH = 16
THEME_COLOR_COUNT = 64
THEME_AREA_SIZE = 256 * 1024

# struct ThemeAreaHeader { uint32_t magic, version, themeCount; }
HEADER_FORMAT = "<III"
HEADER_SIZE = struct.calcsize(HEADER_FORMAT)  # 12

# struct ThemeEntry { char name[16]; uint8_t x4; uint32_t x4; uint32_t[12]; uint32_t[12]; }
ENTRY_FORMAT = f"<{THEME_NAME_LENGTH}sBBBBIIII{THEME_ICON_COUNT}I{THEME_ICON_COUNT}I"
ENTRY_SIZE = struct.calcsize(ENTRY_FORMAT)  # 132

WALLPAPER_FILE = "wallpaper.png"
FIXED_ICON_FILES = [
    "calculation_icon.png",
    "code_icon.png",
    "distributions_icon.png",
    "elements_icon.png",
    "finance_icon.png",
    "graph_icon.png",
    "inference_icon.png",
    "regression_icon.png",
    "sequence_icon.png",
    "settings_icon.png",
    "solver_icon.png",
    "stat_icon.png",
]
assert len(FIXED_ICON_FILES) == THEME_ICON_COUNT


# --------------------------------------------------------------------------
# Small data holders used to build the readable report
# --------------------------------------------------------------------------
class AssetInfo:
    """Bookkeeping for one asset (wallpaper or icon) used only for reporting."""

    def __init__(self, label: str, offset: int, size: int,
                 raw_size: int | None = None, dims: tuple[int, int] | None = None):
        self.label = label
        self.offset = offset
        self.size = size
        self.raw_size = raw_size
        self.dims = dims


class ThemeReport:
    def __init__(self, name: str, dir_path: Path):
        self.name = name
        self.dir_path = dir_path
        self.entry_offset = 0
        self.color_count = 0
        self.palette_asset: AssetInfo | None = None
        self.wallpaper_asset: AssetInfo | None = None
        self.icon_assets: list[AssetInfo] = []
        self.missing_icons: list[str] = []
        self.warnings: list[str] = []

    @property
    def total_size(self) -> int:
        total = 0
        if self.palette_asset:
            total += self.palette_asset.size
        if self.wallpaper_asset:
            total += self.wallpaper_asset.size
        total += sum(a.size for a in self.icon_assets)
        return total


# --------------------------------------------------------------------------
# Palette
# --------------------------------------------------------------------------
def parse_palette_file(path: Path) -> list[int]:
    if not path.exists():
        raise FileNotFoundError(f"Missing palette file: {path}")

    values: list[int] = []
    for line in path.read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        match = re.match(r"^\s*(\S+)\s+0x([0-9A-Fa-f]+)\s*$", line)
        if not match:
            raise ValueError(f"Invalid palette line: {line!r} in {path}")
        color = int(match.group(2), 16)
        if color < 0 or color > 0xFFFFFF:
            raise ValueError(f"Palette value out of range: {line!r}")
        values.append(color)

    if not values:
        raise ValueError(f"No palette values found in {path}")
    if len(values) > THEME_COLOR_COUNT:
        raise ValueError(
            f"Too many palette entries in {path}: {len(values)} > {THEME_COLOR_COUNT}"
        )

    return values


def encode_palette(colors: list[int]) -> bytes:
    # One uint32 per color (0x00RRGGBB, little-endian). This wastes one byte
    # per entry compared to tightly-packed RGB888 (3 bytes), but keeps every
    # entry 4-byte aligned for cheap direct indexing on the Cortex-M4.
    return b"".join(struct.pack("<I", color) for color in colors)


# --------------------------------------------------------------------------
# Images (wallpaper / icons) -> RGB565, LZ4 block-compressed
# --------------------------------------------------------------------------
def rgba888_to_rgb565(red: int, green: int, blue: int, alpha: int) -> int:
    del alpha
    return ((red >> 3) << 11) | ((green >> 2) << 5) | (blue >> 3)


def read_png_rows(png_path: Path):
    if png is not None:
        reader = png.Reader(filename=str(png_path))
        width, height, rows, info = reader.asRGBA8()
        return width, height, list(rows)
    if Image is None:
        raise RuntimeError("Neither pypng nor Pillow is installed")
    image = Image.open(png_path).convert("RGBA")
    width, height = image.size
    rows = []
    for y in range(height):
        row = []
        for x in range(width):
            r, g, b, a = image.getpixel((x, y))
            row.extend((r, g, b, a))
        rows.append(row)
    return width, height, rows


def png_to_rgb565(png_path: Path) -> tuple[int, int, bytes]:
    width, height, rows = read_png_rows(png_path)
    rgb565_data = bytearray()
    for row in rows:
        for i in range(0, len(row), 4):
            r, g, b, a = row[i], row[i + 1], row[i + 2], row[i + 3]
            rgb565_data.extend(rgba888_to_rgb565(r, g, b, a).to_bytes(2, "little"))
    return width, height, bytes(rgb565_data)


def lz4_compress(data: bytes) -> bytes:
    return lz4.block.compress(data, compression=12, mode="high_compression", store_size=False)


def png_to_lz4_wallpaper_blob(png_path: Path, chunk_height: int = 16) -> tuple[bytes, int, tuple[int, int]]:
    """Chunked LZ4 wallpaper blob, so firmware can decompress row-chunks into
    a small buffer instead of needing a full-screen RGB565 buffer at once.

    Blob layout: uint32 width, height, chunk_rows, chunk_count;
                 uint32 offsets[chunk_count]; uint32 sizes[chunk_count];
                 <compressed chunks back to back>
    """
    width, height, rgb565_data = png_to_rgb565(png_path)
    raw_size = len(rgb565_data)

    row_size = width * 2
    chunk_rows = chunk_height if chunk_height > 0 else height
    chunk_count = (height + chunk_rows - 1) // chunk_rows

    compressed_chunks = []
    offsets = []
    sizes = []
    offset = 0
    for chunk_index in range(chunk_count):
        chunk_start = chunk_index * chunk_rows
        chunk_end = min(height, chunk_start + chunk_rows)
        chunk_data = bytes(rgb565_data[chunk_start * row_size: chunk_end * row_size])
        compressed = lz4_compress(chunk_data)
        compressed_chunks.append(compressed)
        offsets.append(offset)
        sizes.append(len(compressed))
        offset += len(compressed)

    blob = struct.pack("<IIII", width, height, chunk_rows, chunk_count)
    blob += struct.pack(f"<{chunk_count}I", *offsets)
    blob += struct.pack(f"<{chunk_count}I", *sizes)
    blob += b"".join(compressed_chunks)
    return bytes(blob), raw_size, (width, height)


def png_to_lz4_image_blob(png_path: Path) -> tuple[bytes, int, tuple[int, int]]:
    width, height, rgb565_data = png_to_rgb565(png_path)
    compressed = lz4_compress(rgb565_data)
    return compressed, len(rgb565_data), (width, height)


# --------------------------------------------------------------------------
# Theme discovery / assembly
# --------------------------------------------------------------------------
def sanitize_name(raw: str) -> str:
    name = raw.strip().splitlines()[0].strip()
    if not name:
        raise ValueError("Theme name is empty")
    name = name[: THEME_NAME_LENGTH - 1]
    return name


def choose_wallpaper(theme_dir: Path) -> Path | None:
    wallpaper_path = theme_dir / WALLPAPER_FILE
    return wallpaper_path if wallpaper_path.exists() else None


def choose_icons(theme_dir: Path) -> tuple[list[Path], list[str]]:
    found: list[Path] = []
    missing: list[str] = []
    for file_name in FIXED_ICON_FILES:
        candidate = theme_dir / file_name
        if candidate.exists():
            found.append(candidate)
        else:
            missing.append(file_name)
    return found, missing


def build_theme(theme_dir: Path, asset_cursor: int,
                 output_bytes: bytearray) -> tuple[bytes, ThemeReport, int]:
    """Appends this theme's asset bytes to `output_bytes` (already positioned
    at `asset_cursor` inside the final Theme Area) and returns the packed
    ThemeEntry bytes, a ThemeReport for the summary, and the new cursor."""

    name = sanitize_name((theme_dir / "name.txt").read_text(encoding="utf-8"))
    name_bytes = name.encode("ascii", errors="ignore")[: THEME_NAME_LENGTH - 1]

    report = ThemeReport(name, theme_dir)

    # --- Wallpaper -------------------------------------------------------
    wallpaper_path = choose_wallpaper(theme_dir)
    wallpaper_offset = 0
    wallpaper_size = 0
    is_wallpaper = 0
    if wallpaper_path is not None:
        blob, raw_size, dims = png_to_lz4_wallpaper_blob(wallpaper_path)
        wallpaper_offset = asset_cursor
        wallpaper_size = len(blob)
        output_bytes.extend(blob)
        asset_cursor += wallpaper_size
        is_wallpaper = 1
        report.wallpaper_asset = AssetInfo("wallpaper.png", wallpaper_offset,
                                            wallpaper_size, raw_size, dims)
    else:
        report.warnings.append("No wallpaper.png found - theme will have no wallpaper.")

    # --- Palette -----------------------------------------------------------
    colors = parse_palette_file(theme_dir / "palette.txt")
    palette_data = encode_palette(colors)
    palette_offset = asset_cursor
    palette_size = len(palette_data)
    output_bytes.extend(palette_data)
    asset_cursor += palette_size
    is_palette = 1
    report.color_count = len(colors)
    report.palette_asset = AssetInfo("palette.txt", palette_offset, palette_size)
    if len(colors) < THEME_COLOR_COUNT:
        report.warnings.append(
            f"Palette has only {len(colors)}/{THEME_COLOR_COUNT} colors defined."
        )

    # --- Icons -------------------------------------------------------------
    icon_paths, missing_icons = choose_icons(theme_dir)
    report.missing_icons = missing_icons
    icon_offsets = [0] * THEME_ICON_COUNT
    icon_sizes = [0] * THEME_ICON_COUNT
    icon_dims_seen: set[tuple[int, int]] = set()
    is_icons = 1 if icon_paths else 0

    # icon_paths follows the fixed FIXED_ICON_FILES order (choose_icons keeps it),
    # so index in FIXED_ICON_FILES == index in ThemeEntry.iconOffsets/iconSizes.
    fixed_index_by_path = {theme_dir / n: i for i, n in enumerate(FIXED_ICON_FILES)}
    for icon_path in icon_paths:
        index = fixed_index_by_path[icon_path]
        blob, raw_size, dims = png_to_lz4_image_blob(icon_path)
        icon_dims_seen.add(dims)
        icon_offsets[index] = asset_cursor
        icon_sizes[index] = len(blob)
        output_bytes.extend(blob)
        asset_cursor += len(blob)
        report.icon_assets.append(
            AssetInfo(icon_path.name, icon_offsets[index], icon_sizes[index], raw_size, dims)
        )

    if len(icon_dims_seen) > 1:
        report.warnings.append(
            f"Icons have inconsistent dimensions: {sorted(icon_dims_seen)} "
            "(firmware assumes a single fixed icon size)."
        )
    if missing_icons:
        report.warnings.append(
            f"{len(missing_icons)}/{THEME_ICON_COUNT} icons missing: "
            + ", ".join(missing_icons)
        )

    entry = struct.pack(
        ENTRY_FORMAT,
        name_bytes.ljust(THEME_NAME_LENGTH, b"\x00"),
        is_wallpaper, is_palette, is_icons, 0,
        wallpaper_offset, wallpaper_size,
        palette_offset, palette_size,
        *icon_offsets,
        *icon_sizes,
    )
    assert len(entry) == ENTRY_SIZE

    return entry, report, asset_cursor


def discover_theme_dirs(root: Path) -> list[Path]:
    if (root / "name.txt").exists() or (root / "palette.txt").exists():
        return [root]
    dirs = [p for p in sorted(root.iterdir()) if p.is_dir()]
    if not dirs:
        raise FileNotFoundError(f"No theme directories found in {root}")
    return dirs


# --------------------------------------------------------------------------
# Top-level generation
# --------------------------------------------------------------------------
def generate_theme_area(input_dir: Path, output_path: Path) -> None:
    theme_dirs = discover_theme_dirs(input_dir)
    if len(theme_dirs) > THEME_COUNT_MAX:
        raise ValueError(f"Too many themes: {len(theme_dirs)} > {THEME_COUNT_MAX}")

    table_size = len(theme_dirs) * ENTRY_SIZE
    asset_region_start = HEADER_SIZE + table_size
    asset_cursor = asset_region_start

    payload = bytearray()
    entries: list[bytes] = []
    reports: list[ThemeReport] = []

    for index, theme_dir in enumerate(theme_dirs):
        entry, report, asset_cursor = build_theme(theme_dir, asset_cursor, payload)
        report.entry_offset = HEADER_SIZE + index * ENTRY_SIZE
        entries.append(entry)
        reports.append(report)

    total_used = asset_region_start + len(payload)
    if total_used > THEME_AREA_SIZE:
        raise ValueError(
            f"Theme Area overflow: {total_used} bytes needed > "
            f"{THEME_AREA_SIZE} bytes available "
            f"(over by {total_used - THEME_AREA_SIZE} bytes)"
        )

    final = bytearray(THEME_AREA_SIZE)  # zero-filled

    struct.pack_into(HEADER_FORMAT, final, 0, MAGIC, VERSION, len(theme_dirs))

    for index, entry in enumerate(entries):
        start = HEADER_SIZE + index * ENTRY_SIZE
        final[start:start + ENTRY_SIZE] = entry

    final[asset_region_start:asset_region_start + len(payload)] = payload

    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_bytes(final)

    summary_path = _summary_path_for(output_path)
    summary_path.write_text(
        render_summary(theme_dirs, reports, asset_region_start, len(payload), total_used),
        encoding="utf-8",
    )

    print(f"Wrote {len(final)} bytes to {output_path}")
    print(f"Wrote report to {summary_path}")
    print(f"Themes: {len(theme_dirs)}  |  used {total_used}/{THEME_AREA_SIZE} bytes "
          f"({100 * total_used / THEME_AREA_SIZE:.1f}%)")


def _summary_path_for(output_path: Path) -> Path:
    if output_path.suffix:
        return output_path.with_suffix(output_path.suffix + ".txt")
    return output_path.parent / f"{output_path.name}.txt"


# --------------------------------------------------------------------------
# Human-readable report
# --------------------------------------------------------------------------
def human_size(n: int) -> str:
    if n < 1024:
        return f"{n} B"
    return f"{n} B ({n / 1024:.2f} KiB)"


def render_summary(theme_dirs: list[Path], reports: list[ThemeReport],
                    asset_region_start: int, payload_size: int, total_used: int) -> str:
    bar_width = 40
    lines: list[str] = []

    def rule(char: str = "=") -> None:
        lines.append(char * 78)

    rule()
    lines.append("THEME AREA - GENERATION REPORT")
    rule()
    lines.append(f"magic              0x{MAGIC:08X}")
    lines.append(f"version            {VERSION}")
    lines.append(f"theme_count        {len(theme_dirs)} / {THEME_COUNT_MAX}")
    lines.append(f"header_size        {HEADER_SIZE} bytes  (sizeof(ThemeAreaHeader))")
    lines.append(f"entry_size         {ENTRY_SIZE} bytes  (sizeof(ThemeEntry))")
    lines.append(f"table_size         {len(theme_dirs) * ENTRY_SIZE} bytes "
                 f"({len(theme_dirs)} x {ENTRY_SIZE})")
    lines.append(f"asset_region_start 0x{asset_region_start:06X}  "
                 f"(offset {asset_region_start} from ThemeAreaStart)")
    lines.append(f"payload_size       {human_size(payload_size)}")
    lines.append("")

    used_pct = total_used / THEME_AREA_SIZE
    filled = int(bar_width * used_pct)
    bar = "#" * filled + "-" * (bar_width - filled)
    lines.append(f"capacity           [{bar}] {used_pct * 100:5.1f}%")
    lines.append(f"                   {total_used} / {THEME_AREA_SIZE} bytes used, "
                 f"{THEME_AREA_SIZE - total_used} bytes free")
    lines.append("")

    for index, report in enumerate(reports):
        rule("-")
        lines.append(f"THEME {index}: \"{report.name}\"  "
                     f"(source: {report.dir_path})")
        lines.append(f"  entry offset       0x{report.entry_offset:06X} "
                     f"(+{report.entry_offset - HEADER_SIZE} in table)")
        lines.append(f"  total asset size   {human_size(report.total_size)}")
        lines.append("")

        # Palette
        pal = report.palette_asset
        lines.append(f"  Palette: {report.color_count}/{THEME_COLOR_COUNT} colors")
        if pal:
            lines.append(f"    offset 0x{pal.offset:06X}   size {human_size(pal.size)}")
        lines.append("")

        # Wallpaper
        lines.append("  Wallpaper:")
        wp = report.wallpaper_asset
        if wp:
            ratio = (wp.raw_size / wp.size) if wp.size else 0
            dims = f"{wp.dims[0]}x{wp.dims[1]}" if wp.dims else "?"
            lines.append(f"    {wp.label}  ({dims})")
            lines.append(f"    offset 0x{wp.offset:06X}   "
                         f"compressed {human_size(wp.size)}   "
                         f"raw {human_size(wp.raw_size)}   "
                         f"ratio {ratio:.2f}x")
        else:
            lines.append("    (none)")
        lines.append("")

        # Icons
        lines.append(f"  Icons: {len(report.icon_assets)}/{THEME_ICON_COUNT} present")
        if report.icon_assets:
            name_width = max(len(a.label) for a in report.icon_assets)
            for a in report.icon_assets:
                ratio = (a.raw_size / a.size) if a.size else 0
                dims = f"{a.dims[0]}x{a.dims[1]}" if a.dims else "?"
                lines.append(
                    f"    {a.label:<{name_width}}  offset 0x{a.offset:06X}  "
                    f"{human_size(a.size):>18}  raw {human_size(a.raw_size):>16}  "
                    f"({dims}, {ratio:.2f}x)"
                )
        if report.missing_icons:
            lines.append(f"    missing: {', '.join(report.missing_icons)}")
        lines.append("")

        if report.warnings:
            lines.append("  Warnings:")
            for w in report.warnings:
                lines.append(f"    ! {w}")
            lines.append("")

    rule()
    lines.append(f"Flash with:  python3 ../device/dfu.py -D <output.bin> -s 0x907B0000")
    rule()

    return "\n".join(lines) + "\n"


def main() -> None:
    parser = argparse.ArgumentParser(description="Generate a Theme Area binary from input folders.")
    parser.add_argument("input_dir", nargs="?", default=Path("input"), type=Path,
                        help="Theme directory or folder containing theme subdirs")
    parser.add_argument("output", nargs="?", default=Path("output/theme_area.bin"), type=Path,
                        help="Path of the output binary")
    args = parser.parse_args()

    generate_theme_area(args.input_dir, args.output)


if __name__ == "__main__":
    main()


# python3 ../device/dfu.py -D output/theme_area.bin -s 0x907B0000