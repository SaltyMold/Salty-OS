#!/usr/bin/env python3
"""Convert a PNG image to a C header containing a raw RGB565 uint16_t array."""

import argparse
import re
from pathlib import Path

from PIL import Image

DEFAULT_SIZE = (320, 240)


def parse_size(value: str) -> tuple[int, int]:
    """Parse a WIDTHxHEIGHT string like '320x240'."""
    match = re.fullmatch(r"(\d+)x(\d+)", value.strip(), flags=re.IGNORECASE)
    if not match:
        raise ValueError("Format attendu: WIDTHxHEIGHT, par exemple 320x240")
    width, height = map(int, match.groups())
    if width <= 0 or height <= 0:
        raise ValueError("La taille doit être positive")
    return width, height


def rgb_to_rgb565(r: int, g: int, b: int) -> int:
    """Convert 8-bit RGB to 16-bit RGB565."""
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)


def composite_over_white(r: int, g: int, b: int, a: int) -> tuple[int, int, int]:
    """Blend RGBA over a white background; transparent pixels stay visible."""
    if a >= 255:
        return r, g, b
    alpha = a / 255.0
    r = int(r * alpha + 255 * (1.0 - alpha))
    g = int(g * alpha + 255 * (1.0 - alpha))
    b = int(b * alpha + 255 * (1.0 - alpha))
    return r, g, b


def load_rgb565_pixels(path: Path, target_size: tuple[int, int] = DEFAULT_SIZE):
    img = Image.open(path).convert("RGBA")
    if img.size != target_size:
        img = img.resize(target_size, Image.Resampling.LANCZOS)
    width, height = img.size
    pixels = []

    for y in range(height):
        for x in range(width):
            r, g, b, a = img.getpixel((x, y))
            if a == 0:
                pixels.append(0)
                continue
            r, g, b = composite_over_white(r, g, b, a)
            pixels.append(rgb_to_rgb565(r, g, b))

    return width, height, pixels


def sanitize_identifier(name: str) -> str:
    name = Path(name).stem
    name = re.sub(r"[^0-9A-Za-z_]", "_", name)
    if not name:
        name = "image"
    if name[0].isdigit():
        name = "img_" + name
    return name


def generate_header(output_path: Path, input_path: Path, symbol_name: str, target_size: tuple[int, int] = DEFAULT_SIZE):
    width, height, pixels = load_rgb565_pixels(input_path, target_size)
    guard = sanitize_identifier(output_path.name).upper() + "_H"
    symbol = sanitize_identifier(symbol_name)

    lines = [
        "#ifndef " + guard,
        "#define " + guard,
        "",
        "#include <stdint.h>",
        "",
        f"static const uint16_t {symbol}_width = {width};",
        f"static const uint16_t {symbol}_height = {height};",
        f"static const uint16_t {symbol}_pixels[{len(pixels)}] = {{",
    ]

    for i in range(0, len(pixels), 12):
        chunk = pixels[i : i + 12]
        values = ", ".join(f"0x{value:04X}" for value in chunk)
        lines.append(f"    {values},")

    lines.append("};")
    lines.append("")
    lines.append("#endif")

    output_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def parse_args():
    parser = argparse.ArgumentParser(
        description="Convert a PNG file to a C header with a raw uint16_t RGB565 pixel array."
    )
    parser.add_argument("input", type=Path, help="Path to the input PNG file")
    parser.add_argument("output", type=Path, help="Path to the generated .h file")
    parser.add_argument(
        "--size",
        default="320x240",
        help="Output resolution in WIDTHxHEIGHT, default: 320x240",
    )
    parser.add_argument(
        "--symbol",
        default=None,
        help="Optional C symbol prefix for the generated array and dimensions",
    )
    return parser.parse_args()


def main():
    args = parse_args()
    try:
        target_size = parse_size(args.size)
    except ValueError as exc:
        raise SystemExit(str(exc))

    symbol = args.symbol or sanitize_identifier(args.input)
    generate_header(args.output, args.input, symbol, target_size)
    print(f"Generated {args.output} from {args.input}")
    print(f"Array: {symbol}_pixels, width: {target_size[0]}, height: {target_size[1]}")


if __name__ == "__main__":
    main()
