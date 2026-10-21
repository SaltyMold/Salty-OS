#include "theme_detail_controller.h"

#include <string.h>

namespace SaltySettings {

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

  p->section(M::SaltySettingsSectionGeneral);
  p->row(M::SaltySettingsName).str(m_title);
  p->row(M::SaltySettingsIndex).dec(m_theme);
  p->row(M::SaltySettingsActive)
      .message(active ? M::SaltySettingsYes : M::SaltySettingsNo);

  if (e == nullptr) {
    // The default theme is not stored in the Theme Area
    p->row(M::SaltySettingsBuiltinLook)
        .message(M::SaltySettingsBuiltinLookHint);
    return;
  }

  p->row(M::SaltySettingsCircleLevel).dec(e->defaultCircleLevel);
  p->row(M::SaltySettingsNameHover)
      .message(e->isAppNameBackgroundHover ? M::SaltySettingsYes
                                           : M::SaltySettingsNo);
  p->row(M::SaltySettingsTotalSize).size(ThemeDataSize(e));

  p->section(M::SaltySettingsSectionWallpaper);
  WallpaperHeader info;
  if (ReadWallpaperHeader(e, &info)) {
    p->row(M::SaltySettingsDimensions)
        .dec(info.width)
        .str(" x ")
        .dec(info.height)
        .str(" px");
    p->row(M::SaltySettingsChunks)
        .dec(info.chunkCount)
        .str(" x ")
        .dec(info.chunkRows)
        .str(" px");
    p->row(M::SaltySettingsCompressed).size(e->wallpaperSize);
    p->row(M::SaltySettingsRawSize)
        .size(info.width * info.height * sizeof(KDColor));
    p->row(M::SaltySettingsOffset).str("0x").hex(e->wallpaperOffset);
  } else {
    p->row(M::SaltySettingsSectionWallpaper).message(M::SaltySettingsNone);
  }

  p->section(M::SaltySettingsSectionPalette);
  if (HasPalette(e)) {
    const uintptr_t paletteAddress = ThemeManager::areaBase() + e->paletteOffset;
    const uint32_t colorCount = e->paletteSize / sizeof(uint32_t);
    const uint32_t slots = Escher::SaltyPalette::numberOfSlots();
    p->row(M::SaltySettingsColors).dec(colorCount).str(" / ").dec(slots);
    p->row(M::SaltySettingsOffset).str("0x").hex(e->paletteOffset);
    const uint32_t shown = colorCount < slots ? colorCount : slots;
    for (uint32_t i = 0; i < shown; i++) {
      const uint32_t color =
          ReadUnaligned32(paletteAddress + i * sizeof(uint32_t)) & 0xFFFFFF;
      p->colorRow(SaltyInfo::k_paletteSlotNames[i], color)
          .str("#")
          .hex(color, 6);
    }
  } else {
    p->row(M::SaltySettingsSectionPalette).message(M::SaltySettingsNone);
  }

  p->section(M::SaltySettingsSectionIcons);
  p->row(M::SaltySettingsPresent)
      .dec(NumberOfIcons(e))
      .str(" / ")
      .dec(ThemeIconCount);
  for (uint32_t i = 0; i < ThemeIconCount; i++) {
    const bool present = e->isIcons && e->iconOffsets[i] != 0;
    Line value = p->row(SaltyInfo::k_iconNames[i]);
    if (present) {
      value.size(e->iconSizes[i]);
    } else {
      value.message(M::SaltySettingsMissing);
    }
  }
}

}  // namespace SaltySettings
