#!/usr/bin/env python3
"""Salty-OS theme manager: loads the JSON themes, shared with generate_themes.py.

Every color of the OS has a name, and a theme is a JSON file of colors by name.
`themes/themes/epsilon.json` is the default theme: it lists every color the OS
knows, with its default value, and gives every other theme its defaults.

Nothing is generated from the JSON for the firmware: the Theme Area written by
generate_themes.py carries the name of every color, so changing a theme (or the
default theme) only needs a new Theme Area, never a rebuild. The one thing to
keep in sync by hand is the list of colors the firmware knows (SALTY_PALETTE_SLOTS
in escher/theme/palette_values.h) and their compiled-in default values (escher/palette.h):
`--check` verifies that they match epsilon.json.

Usage:
    python3 themes/themes_manager.py --check    fail if the firmware and epsilon.json disagree
    python3 themes/themes_manager.py --list     print every color of the default theme
    python3 themes/themes_manager.py --names    color names only

Theme file format (themes/themes/<name>.json):

    {
      "name": "My theme",                       (<= 15 characters on the device)
      "variables": {"defaultCircleLevel": 2},   (optional)
      "colors": {
        "MainColor": "ff0000",                  flat entry: color name -> RRGGBB
        "Python": {                             group: Group + Key -> PythonKeyword
          "Keyword": "00ff00",
          "String": "@MainColor"                "@Name": same color as Name
        }
      }
    }

A theme only lists the colors it changes: every other color takes the value of
the default theme (an "@Name" default follows the theme's own Name color).
Keys starting with "_" are comments and are ignored.

Per-app palettes: a group is the palette of one app (or of one family of
screens, like Graph). Group + key is the color name ("Calculation" + "BorderOdd"
= CalculationBorderOdd). The default theme gives every app color an "@Name"
default pointing to a core color, so a theme that only changes the core colors
still recolors every app, and an app color is only written in a theme when this
theme wants that app to look different.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path

THEMES_DIR = Path(__file__).resolve().parent
ROOT = THEMES_DIR.parent
THEMES_JSON_DIR = THEMES_DIR / "themes"  # <name>.json + <name>/ (images)
DEFAULT_THEME_NAME = "epsilon"
DEFAULT_THEME = THEMES_JSON_DIR / f"{DEFAULT_THEME_NAME}.json"

HEX_RE = re.compile(r"^[0-9a-fA-F]{6}$")
NAME_RE = re.compile(r"^[A-Za-z][A-Za-z0-9_]*$")
# Names are stored in a 32 bytes field of the Theme Area, NUL included
MAX_NAME_LENGTH = 31


class ThemeError(ValueError):
    pass


# --------------------------------------------------------------------------
# Loading
# --------------------------------------------------------------------------
class Entry:
    def __init__(self, name: str, value: str, group: str):
        self.name = name
        self.value = value  # "rrggbb" (lowercase) or "@Other"
        self.group = group  # top level JSON key ("" for a flat entry)

    @property
    def is_alias(self) -> bool:
        return self.value.startswith("@")


def flatten(section: dict, where: str) -> list[Entry]:
    """Upsilon-like flattening: {"Group": {"Key": v}} -> "GroupKey"."""
    entries: list[Entry] = []
    for key, value in section.items():
        if key.startswith("_"):
            continue
        if isinstance(value, dict):
            for sub_key, sub_value in value.items():
                if sub_key.startswith("_"):
                    continue
                if not isinstance(sub_value, str):
                    raise ThemeError(f"{where}: {key}.{sub_key} must be a string")
                entries.append(Entry(key + sub_key, sub_value, key))
        elif isinstance(value, str):
            entries.append(Entry(key, value, ""))
        else:
            raise ThemeError(f"{where}: {key} must be a string or an object")

    for entry in entries:
        if not NAME_RE.match(entry.name):
            raise ThemeError(f"{where}: invalid color name {entry.name!r}")
        if entry.value.startswith("@"):
            if not NAME_RE.match(entry.value[1:]):
                raise ThemeError(f"{where}: invalid alias {entry.name} = {entry.value!r}")
        else:
            value = entry.value[1:] if entry.value.startswith("#") else entry.value
            if value.lower().startswith("0x"):
                value = value[2:]
            if not HEX_RE.match(value):
                raise ThemeError(f"{where}: {entry.name} = {entry.value!r} is not RRGGBB")
            entry.value = value.lower()
    names = [e.name for e in entries]
    dup = {n for n in names if names.count(n) > 1}
    if dup:
        raise ThemeError(f"{where}: duplicated color name(s): {', '.join(sorted(dup))}")
    return entries


class Theme:
    def __init__(self, path: Path):
        self.path = path
        try:
            data = json.loads(path.read_text(encoding="utf-8"))
        except json.JSONDecodeError as e:
            raise ThemeError(f"{path}: invalid JSON ({e})") from e
        self.name: str = data.get("name", path.stem)
        self.variables: dict[str, int] = data.get("variables", {})
        self.colors = flatten(data.get("colors", {}), f"{path} colors")

    def color_map(self) -> dict[str, Entry]:
        return {e.name: e for e in self.colors}


def load_theme(path: Path) -> Theme:
    return Theme(path)


def available_themes() -> list[str]:
    """Names of the themes of themes/themes/ (the default theme first)."""
    names = sorted(p.stem for p in THEMES_JSON_DIR.glob("*.json"))
    if DEFAULT_THEME_NAME in names:
        names.remove(DEFAULT_THEME_NAME)
        names.insert(0, DEFAULT_THEME_NAME)
    return names


def theme_json_path(name: str) -> Path:
    """themes/themes/<name>.json, from the name given on a command line."""
    name = name[:-5] if name.endswith(".json") else name
    path = THEMES_JSON_DIR / f"{name}.json"
    if not path.is_file():
        raise ThemeError(
            f"unknown theme {name!r}: {path.relative_to(ROOT)} does not exist "
            f"(available: {', '.join(available_themes()) or 'none'})"
        )
    return path


def theme_assets_dir(name: str) -> Path:
    """themes/themes/<name>/: the icons and the wallpaper of a theme."""
    return THEMES_JSON_DIR / (name[:-5] if name.endswith(".json") else name)


def load_default() -> Theme:
    theme = load_theme(DEFAULT_THEME)
    seen: set[str] = set()
    for entry in theme.colors:
        if entry.is_alias:
            target = entry.value[1:]
            if target not in seen:
                raise ThemeError(
                    f"{DEFAULT_THEME}: {entry.name} = @{target}: the target must be "
                    f"a slot declared BEFORE it"
                )
        seen.add(entry.name)
    too_long = [e.name for e in theme.colors if len(e.name) > MAX_NAME_LENGTH]
    if too_long:
        raise ThemeError(
            f"{DEFAULT_THEME}: color name(s) longer than {MAX_NAME_LENGTH} characters: "
            f"{', '.join(too_long)}"
        )
    return theme


def resolve_default(theme: Theme) -> dict[str, int]:
    """name -> 0xRRGGBB for every slot of the default theme."""
    values: dict[str, int] = {}
    for entry in theme.colors:
        values[entry.name] = (
            values[entry.value[1:]] if entry.is_alias else int(entry.value, 16)
        )
    return values


def slot_values(theme: Theme, default: Theme) -> list[int]:
    """The palette of the Theme Area for `theme`: one 0xRRGGBB per color of the
    default theme, in the default theme's order. Colors the theme does not list
    come from the default theme (aliases follow the theme's own colors)."""
    known = {e.name for e in default.colors}
    provided = theme.color_map()
    unknown = [n for n in provided if n not in known]
    if unknown:
        raise ThemeError(
            f"{theme.path}: unknown color(s): {', '.join(unknown)} "
            f"(see `python3 themes/themes_manager.py --list`)"
        )
    final: dict[str, int] = {}
    out: list[int] = []
    for entry in default.colors:
        source = provided.get(entry.name, entry)
        if source.is_alias:
            target = source.value[1:]
            if target not in final:
                raise ThemeError(
                    f"{theme.path}: {entry.name} = @{target} must refer to a slot "
                    f"declared before it in the default theme"
                )
            final[entry.name] = final[target]
        else:
            final[entry.name] = int(source.value, 16)
        out.append(final[entry.name])
    return out


# --------------------------------------------------------------------------
# Consistency with the firmware (the only thing that is not read from the Theme Area)
# --------------------------------------------------------------------------
ESCHER = ROOT / "epsilon/escher/include/escher"
PALETTE_H = ESCHER / "palette.h"
PALETTE_VALUES_H = ESCHER / "theme/palette_values.h"


def firmware_names() -> list[str]:
    """Colors the firmware knows by name: SALTY_PALETTE_SLOTS."""
    text = PALETTE_VALUES_H.read_text(encoding="utf-8")
    block = text[text.index("#define SALTY_PALETTE_SLOTS"):text.index("struct PaletteValues")]
    return re.findall(r"\bX\((\w+)\)", block)


def firmware_defaults() -> dict[str, int]:
    """Compiled-in default value of every color of escher/palette.h."""
    text = PALETTE_H.read_text(encoding="utf-8")
    return {
        name: int(value, 16)
        for name, value in re.findall(
            r"constexpr static KDColor (\w+) = KDColor::RGB24\(0x([0-9a-fA-F]{6})\)", text
        )
    }


def check(theme: Theme) -> list[str]:
    problems: list[str] = []
    json_names = [e.name for e in theme.colors]
    fw_names = firmware_names()
    for name in json_names:
        if name not in fw_names:
            problems.append(f"{name}: in epsilon.json but not in SALTY_PALETTE_SLOTS "
                            f"(the firmware ignores it): add it to {PALETTE_VALUES_H.relative_to(ROOT)}")
    for name in fw_names:
        if name not in json_names:
            problems.append(f"{name}: in SALTY_PALETTE_SLOTS but not in epsilon.json: "
                            f"add it to {DEFAULT_THEME.relative_to(ROOT)}")
    defaults = firmware_defaults()
    resolved = resolve_default(theme)
    for name, value in resolved.items():
        if name not in defaults:
            problems.append(f"{name}: no default value in {PALETTE_H.relative_to(ROOT)}")
        elif defaults[name] != value:
            problems.append(f"{name}: epsilon.json says {value:06x}, "
                            f"{PALETTE_H.relative_to(ROOT)} says {defaults[name]:06x}")
    return problems


# --------------------------------------------------------------------------
def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description="Inspect the Salty-OS default theme")
    parser.add_argument("--check", action="store_true",
                        help="fail if the colors known by the firmware and epsilon.json disagree")
    parser.add_argument("--list", action="store_true", help="list every color of the default theme")
    parser.add_argument("--names", action="store_true", help="print the color names, one per line")
    args = parser.parse_args(argv)

    try:
        theme = load_default()
    except ThemeError as e:
        print(f"ERROR: {e}", file=sys.stderr)
        return 2

    if args.names:
        print("\n".join(e.name for e in theme.colors))
        return 0

    if args.list:
        values = resolve_default(theme)
        for i, entry in enumerate(theme.colors):
            alias = f"  (= {entry.value})" if entry.is_alias else ""
            print(f"{i:3d} {entry.name:32s} {values[entry.name]:06x}{alias}")
        return 0

    problems = check(theme)
    for problem in problems:
        print(f"MISMATCH: {problem}", file=sys.stderr)
    if problems:
        return 1
    print(f"{len(theme.colors)} colors: the firmware and epsilon.json agree")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
