#pragma once

#include <kandinsky/color.h>
#include <stdint.h>

#include "../../shared/ion/src/device/include/n0120/config/board.h"

class ThemeManager {
 public:
  static void init();
  static bool isValid() { return s_valid; }
  static int themeCount() {
    return s_valid ? (int)s_header->themeCount : 0;
  }
  static const Ion::Device::Board::Config::ThemeEntry* entry(int index);

  // --- Theme selection -----------------------------------------------
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

 private:
  static const Ion::Device::Board::Config::ThemeEntry* currentEntry();

  static const Ion::Device::Board::Config::ThemeAreaHeader* s_header;
  static bool s_valid;
  static int s_selectedTheme;
};