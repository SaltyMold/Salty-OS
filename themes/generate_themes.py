#!/usr/bin/env python3
"""Generate a Theme Area binary from one or more themes, given by name.

    python3 generate_themes.py epsilon
    python3 generate_themes.py epsilon kawaii omega -o output/theme_area.bin
    python3 generate_themes.py                      # every theme of themes/themes/
    python3 generate_themes.py --list               # available theme names

Layout of the themes folder (a theme is a JSON file + a folder of images, both
named after the theme):

    themes
    ├── generate_themes.py
    ├── themes_manager.py
    └── themes
        ├── epsilon
        │   ├── calculation_icon.png        (see FIXED_ICON_FILES)
        │   ├── ...
        │   └── wallpaper.png               (optional)
        └── epsilon.json

The JSON is the very same format as the default theme that generates the
palette of the OS (see themes_manager.py): the name, a few variables, and the
colors, with the core colors first and then one group of colors per app
(Calculation, Python, Graph, Elements...). A theme only lists the colors it
changes, every other color comes from themes/themes/epsilon.json (an "@Name"
default follows the color the theme gives to Name):

    {
      "name": "Ocean",                              (<= 15 characters)
      "variables": {"defaultCircleLevel": 2, "isAppNameBackgroundHover": 1},
      "colors": {
        "YellowDark": "1f6feb",                     core color
        "Python": {"Keyword": "d6336c"},            PythonKeyword, Python app only
        "Graph": {"Curve": "e8590c"}                GraphCurve, Graph app only
      }
    }

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
      uint8_t  defaultCircleLevel;
      uint8_t  isAppNameBackgroundHover;
      uint8_t  reserved[3];
      uint32_t wallpaperOffset;
      uint32_t wallpaperSize;
      uint32_t paletteOffset;
      uint32_t paletteSize;
      uint32_t iconOffsets[12];
      uint32_t iconSizes[12];
    };                                              // 136 bytes, no padding

Binary layout produced by this script:

    [ThemeAreaHeader]            12 bytes, at ThemeAreaStart + 0
    [ThemeEntry] * themeCount    136 bytes each, immediately after the header
    [asset data]                 wallpapers / palettes / icons, back to back
    [zero padding]                up to THEME_AREA_SIZE (1.5 MiB)

The palette of a theme is self-describing: it carries the NAME of each color
(core colors, then the colors of each app, as in themes/themes/epsilon.json), so
the firmware needs nothing from the build to apply it. Changing a theme, or
the default theme, only needs a new Theme Area, never a rebuild. A color the
firmware has no name for is ignored, a color the palette does not carry keeps
the default value compiled in the firmware.

    uint32 magic ("PAL2"), uint32 count, then count records of
    { char name[32] (NUL-terminated), uint32 color (0x00RRGGBB) }
    little-endian, back to back, without alignment.

All offset fields inside a ThemeEntry (wallpaperOffset, paletteOffset,
iconOffsets[i]) are ABSOLUTE byte offsets from the start of the Theme Area
(i.e. from ThemeAreaStart), so firmware code can dereference them directly as
`ThemeAreaStart + entry.wallpaperOffset`. An offset of 0 means "absent",
which is always safe because byte 0 of the area is the header's magic field
and can never be a real asset.

The generated .bin can be flashed directly in the Theme Area region.
A human-readable `<output>.txt` report is written alongside it.
"""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

import lz4.block

import themes_manager as tm

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
VERSION = 5
THEME_COUNT_MAX = 256
THEME_ICON_COUNT = 12
THEME_NAME_LENGTH = 16
THEME_AREA_SIZE = int(1.5 * 1024 * 1024)

# Palette blob, see ThemeManager::paletteColorCount in apps/theme_manager.h
PALETTE_MAGIC = 0x324C4150  # "PAL2"
PALETTE_NAME_SIZE = 32


# struct ThemeAreaHeader { uint32_t magic, version, themeCount; }
HEADER_FORMAT = "<III"
HEADER_SIZE = struct.calcsize(HEADER_FORMAT)  # 12

# struct ThemeEntry {
#   char name[16]; uint8_t x8 (isWallpaper, isPalette, isIcons,
#   defaultCircleLevel, isAppNameBackgroundHover, reserved[3]);
#   uint32_t x4; uint32_t[12]; uint32_t[12];
# }
ENTRY_FORMAT = f"<{THEME_NAME_LENGTH}sBBBBBBBBIIII{THEME_ICON_COUNT}I{THEME_ICON_COUNT}I"
ENTRY_SIZE = struct.calcsize(ENTRY_FORMAT)  # 132

WALLPAPER_FILE = "wallpaper.png"

# Known theme variables: name -> (default, min, max). generate_themes.py
# rejects any name in the "variables" of a theme that isn't listed here, and any
# value out of range, so a typo fails loudly at generation time instead of
# being silently ignored on-device.
VARIABLE_DEFS: dict[str, dict[str, int]] = {
    "defaultCircleLevel": {"default": 0, "min": 0, "max": 7},
    "isAppNameBackgroundHover": {"default": 0, "min": 0, "max": 1},
}
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
    def __init__(self, name: str, json_path: Path, assets_dir: Path):
        self.name = name
        self.json_path = json_path
        self.assets_dir = assets_dir
        self.entry_offset = 0
        self.color_count = 0
        # group name ("" = core) -> (colors the theme sets, colors of the group)
        self.group_counts: dict[str, tuple[int, int]] = {}
        self.palette_asset: AssetInfo | None = None
        self.wallpaper_asset: AssetInfo | None = None
        self.icon_assets: list[AssetInfo] = []
        self.missing_icons: list[str] = []
        self.variables: dict[str, int] = {}
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
def encode_palette(names: list[str], colors: list[int]) -> bytes:
    # Header, then one { name, color } record per color. The color is a uint32
    # (0x00RRGGBB, little-endian). Records are not aligned: the firmware reads
    # them byte by byte.
    assert len(names) == len(colors)
    blob = struct.pack("<II", PALETTE_MAGIC, len(colors))
    for name, color in zip(names, colors):
        raw = name.encode("ascii")
        if len(raw) >= PALETTE_NAME_SIZE:
            raise ValueError(f"color name {name!r} is too long ({PALETTE_NAME_SIZE - 1} max)")
        blob += raw.ljust(PALETTE_NAME_SIZE, b"\x00") + struct.pack("<I", color)
    return blob


def palette_group_counts(theme: tm.Theme, default: tm.Theme) -> dict[str, tuple[int, int]]:
    """For each group of the default theme ("" = core colors): how many of its
    colors this theme sets itself, and how many it has in total."""
    provided = set(theme.color_map())
    counts: dict[str, list[int]] = {}
    for entry in default.colors:
        pair = counts.setdefault(entry.group, [0, 0])
        pair[1] += 1
        if entry.name in provided:
            pair[0] += 1
    return {group: (own, total) for group, (own, total) in counts.items()}


# --------------------------------------------------------------------------
# Variables
# --------------------------------------------------------------------------
def parse_variables(raw: dict, where: str) -> dict[str, int]:
    """Checks the "variables" of a theme against VARIABLE_DEFS. Missing names
    fall back to their default; unknown names or out-of-range values raise, so a
    typo in a theme fails at generation time instead of silently doing nothing
    on-device."""
    values = {name: spec["default"] for name, spec in VARIABLE_DEFS.items()}
    if not isinstance(raw, dict):
        raise ValueError(f"{where}: \"variables\" must be an object")

    for name, raw_value in raw.items():
        if name.startswith("_"):
            continue
        if name not in VARIABLE_DEFS:
            raise ValueError(
                f"Unknown variable {name!r} in {where} "
                f"(known: {', '.join(VARIABLE_DEFS)})"
            )
        if isinstance(raw_value, bool) or not isinstance(raw_value, int):
            raise ValueError(f"{name} must be an integer in {where}")
        spec = VARIABLE_DEFS[name]
        if not (spec["min"] <= raw_value <= spec["max"]):
            raise ValueError(
                f"{name}={raw_value} out of range "
                f"[{spec['min']}, {spec['max']}] in {where}"
            )
        values[name] = raw_value

    return values


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
def sanitize_name(raw: str, report: ThemeReport) -> str:
    name = raw.strip().splitlines()[0].strip() if raw.strip() else ""
    if not name:
        raise ValueError(f"{report.json_path}: the theme name is empty")
    if len(name) > THEME_NAME_LENGTH - 1:
        report.warnings.append(
            f"Name {name!r} is longer than {THEME_NAME_LENGTH - 1} characters, "
            f"it is cut on the device."
        )
        name = name[: THEME_NAME_LENGTH - 1]
    return name


def choose_wallpaper(assets_dir: Path) -> Path | None:
    wallpaper_path = assets_dir / WALLPAPER_FILE
    return wallpaper_path if wallpaper_path.exists() else None


def choose_icons(assets_dir: Path) -> tuple[list[Path], list[str]]:
    found: list[Path] = []
    missing: list[str] = []
    for file_name in FIXED_ICON_FILES:
        candidate = assets_dir / file_name
        if candidate.exists():
            found.append(candidate)
        else:
            missing.append(file_name)
    return found, missing


def build_theme(theme_name: str, default: tm.Theme, asset_cursor: int,
                output_bytes: bytearray) -> tuple[bytes, ThemeReport, int]:
    """Appends this theme's asset bytes to `output_bytes` (already positioned
    at `asset_cursor` inside the final Theme Area) and returns the packed
    ThemeEntry bytes, a ThemeReport for the summary, and the new cursor."""

    json_path = tm.theme_json_path(theme_name)
    assets_dir = tm.theme_assets_dir(theme_name)
    theme = tm.load_theme(json_path)

    report = ThemeReport(theme.name, json_path, assets_dir)
    name = sanitize_name(theme.name, report)
    report.name = name
    name_bytes = name.encode("ascii", errors="ignore")[: THEME_NAME_LENGTH - 1]

    if not assets_dir.is_dir():
        report.warnings.append(
            f"No folder {assets_dir.relative_to(tm.ROOT)}: no wallpaper, no icons."
        )

    # --- Wallpaper -------------------------------------------------------
    wallpaper_path = choose_wallpaper(assets_dir)
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
        report.wallpaper_asset = AssetInfo(WALLPAPER_FILE, wallpaper_offset,
                                            wallpaper_size, raw_size, dims)
    else:
        report.warnings.append(f"No {WALLPAPER_FILE} found - theme will have no wallpaper.")

    # --- Palette: core colors + one group of colors per app ----------------
    colors = tm.slot_values(theme, default)
    palette_data = encode_palette([e.name for e in default.colors], colors)
    palette_offset = asset_cursor
    palette_size = len(palette_data)
    output_bytes.extend(palette_data)
    asset_cursor += palette_size
    is_palette = 1
    report.color_count = len(colors)
    report.group_counts = palette_group_counts(theme, default)
    report.palette_asset = AssetInfo(json_path.name, palette_offset, palette_size)

    # --- Icons -------------------------------------------------------------
    icon_paths, missing_icons = choose_icons(assets_dir)
    report.missing_icons = missing_icons
    icon_offsets = [0] * THEME_ICON_COUNT
    icon_sizes = [0] * THEME_ICON_COUNT
    icon_dims_seen: set[tuple[int, int]] = set()
    is_icons = 1 if icon_paths else 0

    # icon_paths follows the fixed FIXED_ICON_FILES order (choose_icons keeps it),
    # so index in FIXED_ICON_FILES == index in ThemeEntry.iconOffsets/iconSizes.
    fixed_index_by_path = {assets_dir / n: i for i, n in enumerate(FIXED_ICON_FILES)}
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

    # --- Variables -----------------------------------------------------
    variables = parse_variables(theme.variables, str(json_path))
    report.variables = variables

    entry = struct.pack(
        ENTRY_FORMAT,
        name_bytes.ljust(THEME_NAME_LENGTH, b"\x00"),
        is_wallpaper, is_palette, is_icons,
        variables["defaultCircleLevel"], variables["isAppNameBackgroundHover"],
        0, 0, 0,  # reserved[3]
        wallpaper_offset, wallpaper_size,
        palette_offset, palette_size,
        *icon_offsets,
        *icon_sizes,
    )
    assert len(entry) == ENTRY_SIZE

    return entry, report, asset_cursor


# --------------------------------------------------------------------------
# Top-level generation
# --------------------------------------------------------------------------
def generate_theme_area(theme_names: list[str], output_path: Path) -> None:
    default = tm.load_default()
    if len(theme_names) > THEME_COUNT_MAX:
        raise ValueError(f"Too many themes: {len(theme_names)} > {THEME_COUNT_MAX}")

    table_size = len(theme_names) * ENTRY_SIZE
    asset_region_start = HEADER_SIZE + table_size
    asset_cursor = asset_region_start

    payload = bytearray()
    entries: list[bytes] = []
    reports: list[ThemeReport] = []

    for index, theme_name in enumerate(theme_names):
        entry, report, asset_cursor = build_theme(theme_name, default, asset_cursor, payload)
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

    struct.pack_into(HEADER_FORMAT, final, 0, MAGIC, VERSION, len(theme_names))

    for index, entry in enumerate(entries):
        start = HEADER_SIZE + index * ENTRY_SIZE
        final[start:start + ENTRY_SIZE] = entry

    final[asset_region_start:asset_region_start + len(payload)] = payload

    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_bytes(final)

    summary_path = _summary_path_for(output_path)
    summary_path.write_text(
        render_summary(reports, len(default.colors), asset_region_start,
                       len(payload), total_used),
        encoding="utf-8",
    )

    print(f"Wrote {len(final)} bytes to {output_path}")
    print(f"Wrote report to {summary_path}")
    print(f"Themes: {', '.join(r.name for r in reports)}")
    print(f"Themes: {len(theme_names)}  |  used {total_used}/{THEME_AREA_SIZE} bytes "
          f"({100 * total_used / THEME_AREA_SIZE:.1f}%)")
    warning_count = sum(len(r.warnings) for r in reports)
    if warning_count:
        print(f"{warning_count} warning(s), see {summary_path}")


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


def render_summary(reports: list[ThemeReport], slot_count: int,
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
    lines.append(f"theme_count        {len(reports)} / {THEME_COUNT_MAX}")
    lines.append(f"header_size        {HEADER_SIZE} bytes  (sizeof(ThemeAreaHeader))")
    lines.append(f"entry_size         {ENTRY_SIZE} bytes  (sizeof(ThemeEntry))")
    lines.append(f"table_size         {len(reports) * ENTRY_SIZE} bytes "
                 f"({len(reports)} x {ENTRY_SIZE})")
    lines.append(f"palette_slots      {slot_count}  (themes/themes/{tm.DEFAULT_THEME_NAME}.json)")
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
                     f"(source: {report.json_path.relative_to(tm.ROOT)})")
        lines.append(f"  entry offset       0x{report.entry_offset:06X} "
                     f"(+{report.entry_offset - HEADER_SIZE} in table)")
        lines.append(f"  total asset size   {human_size(report.total_size)}")
        lines.append("")

        # Palette
        pal = report.palette_asset
        lines.append(f"  Palette: {report.color_count}/{slot_count} colors")
        if pal:
            lines.append(f"    offset 0x{pal.offset:06X}   size {human_size(pal.size)}")
        lines.append("    colors set by the theme itself (the others come from the default theme):")
        for group, (own, total) in report.group_counts.items():
            lines.append(f"      {group or 'Core':<14} {own:>3} / {total}")
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

        # Variables
        lines.append("  Variables:")
        for name, value in report.variables.items():
            lines.append(f"    {name:<24} {value}")
        lines.append("")

        if report.warnings:
            lines.append("  Warnings:")
            for w in report.warnings:
                lines.append(f"    ! {w}")
            lines.append("")

    rule()
    lines.append("Flash with:  python3 ../device/dfu.py -D <output.bin> -s 0x90270000")
    rule()

    return "\n".join(lines) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Generate a Theme Area binary from themes given by name "
                    "(themes/themes/<name>.json and themes/themes/<name>/)."
    )
    parser.add_argument(
        "themes",
        nargs="*",
        metavar="theme",
        help="Names of the themes to put in the Theme Area, in this order "
             "(example: epsilon kawaii). Every theme of themes/themes/ when "
             "omitted.",
    )
    parser.add_argument(
        "-o",
        "--output",
        default=Path("output/theme_area.bin"),
        type=Path,
        help="Path of the output binary (default: output/theme_area.bin)",
    )
    parser.add_argument(
        "-l",
        "--list",
        action="store_true",
        help="List the available theme names and exit",
    )
    args = parser.parse_args()

    if args.list:
        print("\n".join(tm.available_themes()))
        return 0

    names: list[str] = []
    for raw in args.themes or tm.available_themes():
        name = raw[:-5] if raw.endswith(".json") else raw
        if name not in names:
            names.append(name)

    if not names:
        print(f"ERROR: no theme found in {tm.THEMES_JSON_DIR}", file=sys.stderr)
        return 2

    try:
        generate_theme_area(names, args.output)
    except (tm.ThemeError, FileNotFoundError, ValueError) as error:
        print(f"ERROR: {error}", file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    sys.exit(main())


# python3 ../device/dfu.py -D output/theme_area.bin -s 0x90270000
