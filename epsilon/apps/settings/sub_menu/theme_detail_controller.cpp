#include "theme_detail_controller.h"

#include <string.h>

namespace Settings {

using namespace Ion::Device::Board::Config;

void ThemeDetailController::setTheme(int theme) {
  m_theme = theme;
  ThemeName(theme, m_title);
}

void ThemeDetailController::describe(RowPicker* p) const {
  using M = I18n::Message;

  const bool isDefault = m_theme == 0;
  const ThemeEntry* e = isDefault ? nullptr : ThemeManager::entry(m_theme - 1);
  const bool active = ThemeManager::selectedTheme() == m_theme;

  p->section(M::ThemesSectionGeneral);
  p->row(M::ThemesName).str(m_title);
  p->row(M::ThemesIndex).dec(m_theme);
  p->row(M::ThemesActive)
      .message(active ? M::ThemesYes : M::ThemesNo);

  if (e == nullptr) {
    // The default theme is not stored in the Theme Area
    p->row(M::ThemesBuiltinLook)
        .message(M::ThemesBuiltinLookHint);
    return;
  }

  p->row(M::ThemesCircleLevel).dec(e->defaultCircleLevel);
  p->row(M::ThemesNameHover)
      .message(e->isAppNameBackgroundHover ? M::ThemesYes
                                           : M::ThemesNo);
  p->row(M::ThemesTotalSize).size(ThemeDataSize(e));

  p->section(M::ThemesSectionWallpaper);
  WallpaperHeader info;
  if (ReadWallpaperHeader(e, &info)) {
    p->row(M::ThemesDimensions)
        .dec(info.width)
        .str(" x ")
        .dec(info.height)
        .str(" px");
    p->row(M::ThemesChunks)
        .dec(info.chunkCount)
        .str(" x ")
        .dec(info.chunkRows)
        .str(" px");
    p->row(M::ThemesCompressed).size(e->wallpaperSize);
    p->row(M::ThemesRawSize)
        .size(info.width * info.height * sizeof(KDColor));
    p->row(M::ThemesOffset).str("0x").hex(e->wallpaperOffset);
  } else {
    p->row(M::ThemesSectionWallpaper).message(M::ThemesNone);
  }

  p->section(M::ThemesSectionPalette);
  if (HasPalette(e)) {
    // The palette lists its colors by name: shown as the theme describes them
    const uintptr_t records = ThemeManager::areaBase() + e->paletteOffset +
                              ThemeManager::k_paletteHeaderSize;
    const uint32_t colorCount = ThemeManager::paletteColorCount(e);
    const uint32_t slots = Escher::SaltyPalette::numberOfSlots();
    p->row(M::ThemesColors).dec(colorCount).str(" / ").dec(slots);
    p->row(M::ThemesOffset).str("0x").hex(e->paletteOffset);
    for (uint32_t i = 0; i < colorCount; i++) {
      const uintptr_t record = records + i * ThemeManager::k_paletteRecordSize;
      // Names are NUL-terminated in the Theme Area (see generate_themes.py)
      const char* name = reinterpret_cast<const char*>(record);
      const uint32_t color =
          ReadUnaligned32(record + ThemeManager::k_paletteNameSize) & 0xFFFFFF;
      p->colorRow(name, color).str("#").hex(color, 6);
    }
  } else {
    p->row(M::ThemesSectionPalette).message(M::ThemesNone);
  }

  p->section(M::ThemesSectionIcons);
  p->row(M::ThemesPresent)
      .dec(NumberOfIcons(e))
      .str(" / ")
      .dec(ThemeIconCount);
  for (uint32_t i = 0; i < ThemeIconCount; i++) {
    const bool present = e->isIcons && e->iconOffsets[i] != 0;
    AboutText value = p->row(SaltyInfo::k_iconNames[i]);
    if (present) {
      value.size(e->iconSizes[i]);
    } else {
      value.message(M::ThemesMissing);
    }
  }
}

}  // namespace Settings
