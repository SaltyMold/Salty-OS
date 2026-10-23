#pragma once

#include <apps/i18n.h>
#include <apps/salty_info.h>
#include <escher/theme/salty_palette.h>
#include <string.h>

#include "../theme_manager.h"

namespace SaltySettings {

using Ion::Device::Board::Config::ThemeEntry;
using Ion::Device::Board::Config::ThemeIconCount;
using Ion::Device::Board::Config::ThemeNameLength;

/* The names of the palette slots and of the icons come from the themes folder
 * of the repository (see generate_info.sh). If one of these checks fails, the
 * format of the themes changed: rebuild after updating themes/ and board.h. */
static_assert(SaltyInfo::k_numberOfPaletteSlotNames ==
                  Escher::SaltyPalette::numberOfSlots(),
              "themes/epsilon/palette.txt and SaltyPalette do not match");
static_assert(SaltyInfo::k_numberOfIconNames == ThemeIconCount,
              "FIXED_ICON_FILES of generate_theme.py and board.h do not match");

// Theme 0 is the virtual default theme, theme t > 0 is flashed entry t - 1
constexpr static int k_maxThemeNameSize = ThemeNameLength + 1;

inline void ThemeName(int theme, char* out /* k_maxThemeNameSize */) {
  const ThemeEntry* e = theme == 0 ? nullptr : ThemeManager::entry(theme - 1);
  if (e == nullptr) {
    strlcpy(out, I18n::translate(I18n::Message::SaltySettingsDefaultTheme),
            k_maxThemeNameSize);
    return;
  }
  memcpy(out, e->name, ThemeNameLength);
  out[ThemeNameLength] = 0;
}

/* The assets of the Theme Area are stored back to back, without any alignment
 * (see generate_theme.py): the palette of a theme that has a wallpaper starts
 * right after the wallpaper blob, at any address. Dereferencing a uint32_t* on
 * such an address is a hard fault on the calculator, so everything inside the
 * assets is read byte by byte. */
inline uint32_t ReadUnaligned32(uintptr_t address) {
  const volatile uint8_t* p = reinterpret_cast<const volatile uint8_t*>(address);
  return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
         (static_cast<uint32_t>(p[2]) << 16) |
         (static_cast<uint32_t>(p[3]) << 24);
}

struct WallpaperHeader {
  uint32_t width;
  uint32_t height;
  uint32_t chunkRows;
  uint32_t chunkCount;
};

// False if the theme has no wallpaper
inline bool ReadWallpaperHeader(const ThemeEntry* e, WallpaperHeader* out) {
  if (e == nullptr || !e->isWallpaper || e->wallpaperOffset == 0) {
    return false;
  }
  const uintptr_t base = ThemeManager::areaBase() +
                        e->wallpaperOffset;
  out->width = ReadUnaligned32(base);
  out->height = ReadUnaligned32(base + 4);
  out->chunkRows = ReadUnaligned32(base + 8);
  out->chunkCount = ReadUnaligned32(base + 12);
  return true;
}

inline uint32_t ThemeDataSize(const ThemeEntry* e) {
  uint32_t total = e->wallpaperSize + e->paletteSize;
  for (uint32_t i = 0; i < ThemeIconCount; i++) {
    total += e->iconSizes[i];
  }
  return total;
}

inline uint32_t NumberOfIcons(const ThemeEntry* e) {
  uint32_t count = 0;
  if (e->isIcons) {
    for (uint32_t i = 0; i < ThemeIconCount; i++) {
      if (e->iconOffsets[i] != 0) {
        count++;
      }
    }
  }
  return count;
}

inline bool HasPalette(const ThemeEntry* e) {
  return e->isPalette && e->paletteOffset != 0;
}

}  // namespace SaltySettings
