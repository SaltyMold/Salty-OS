#pragma once

#include <apps/i18n.h>
#include <kandinsky/color.h>
#include <stdint.h>

#include "../../shared/ion/src/device/include/n0120/config/board.h"
#include "apps_window.h"

class ThemeManager {
 public:
  static void init();
  static bool isValid() { return s_valid; }

  // themeCount() includes the virtual "Default" entry at index 0, so it is
  // (number of flashed themes) + 1 whenever the Theme Area is valid.
  static int themeCount() {
    return s_valid ? (int)s_header->themeCount + 1 : 0;
  }

  // Raw access to a FLASHED theme entry, 0-based within the Theme Area
  // (i.e. NOT offset by the virtual "Default" entry). Returns nullptr if
  // index is out of range. Most callers should go through
  // selectTheme()/currentEntry()-backed accessors (hasWallpaper(),
  // hasPalette(), ...) instead.
  static const Ion::Device::Board::Config::ThemeEntry* entry(int index);

  // --- Theme selection -----------------------------------------------
  // Index 0 is a virtual "Default" theme that is not backed by any
  // flashed ThemeEntry: selecting it makes every accessor below behave
  // exactly as if the Theme Area were invalid/empty (no wallpaper, no
  // palette, no icons), which restores Escher's builtin palette/icons.
  // Indices [1, themeCount()) map to flashed entries at index-1.
  static int selectedTheme() { return s_selectedTheme; }
  // No-op (keeps previous selection) if index is out of range.
  static void selectTheme(int index);

  // --- Wallpaper of the CURRENTLY SELECTED theme ----------------------
  // All of these read the currently selected theme's entry; they return
  // false/0/nullptr if the Theme Area is invalid or the theme has no
  // wallpaper, so callers can always fall back gracefully (e.g. an
  // unflashed Theme Area, or a theme built without a wallpaper.png).
  static bool hasWallpaper();
  static int wallpaperWidth();
  static int wallpaperHeight();

  // Returns a pointer to a decompressed buffer holding the wallpaper chunk
  // that contains pixel row `globalY` (0 <= globalY < wallpaperHeight()).
  // *localY is set to the row's index within that returned buffer.
  // Returns nullptr if there is no wallpaper or globalY is out of range.
  //
  // The buffer is a single shared cache reused by every caller (AppCell,
  // Controller, ...) and is only valid until the next call to this
  // function - copy out what you need (memcpy) before calling again.
  static const KDColor* wallpaperChunkContaining(int globalY, int* localY);

  // --- Icons of the CURRENTLY SELECTED theme --------------------------
  // Maps a builtin app's name to its theme icon slot (0..ThemeIconCount-1),
  // matching the FIXED_ICON_FILES order in generate_theme.py. Returns -1 if
  // this app has no themed-icon slot (e.g. unknown/new app).
  static int iconIndexForApp(I18n::Message appName);

  // True if the current theme actually provides an icon for that slot
  // (the slot can exist in the mapping above but be missing from the
  // flashed binary, like "stat_icon.png" in the sample report).
  static bool hasIcon(int iconIndex);

  // Decompresses (and caches) the icon at `iconIndex` into a shared static
  // buffer sized iconWidth * iconHeight and returns a pointer to it, or
  // nullptr if unavailable or too large. Same single-shared-buffer caching
  // contract as wallpaperChunkContaining: copy out what you need before
  // calling again for a different icon/theme.
  static const KDColor* iconPixels(int iconIndex, int iconWidth, int iconHeight);

  // --- Palette of the CURRENTLY SELECTED theme ------------------------
  // True if the current theme actually provides a palette (a Theme Area
  // can have isPalette == 0, e.g. a theme with only a wallpaper).
  static bool hasPalette();

  // Reads the currently selected theme's palette blob (raw 0x00RRGGBB
  // uint32s, see encode_palette() in generate_theme.py) and pushes it
  // into Escher::Palette::ApplyPalette(). If the current theme has no
  // palette (including the virtual "Default" theme), resets Escher's
  // colors to their compiled-in defaults instead of leaving a previously
  // applied theme's colors in place.
  static void applyPalette();

  static void refreshTheme(AppsWindow* window);

 private:
  static const Ion::Device::Board::Config::ThemeEntry* currentEntry();

  static const Ion::Device::Board::Config::ThemeAreaHeader* s_header;
  static bool s_valid;
  static int s_selectedTheme;
};