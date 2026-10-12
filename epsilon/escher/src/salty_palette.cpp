#include <escher/theme/light_theme.h>
#include <escher/theme/salty_palette.h>

namespace Escher {

bool SaltyPalette::s_active = false;
ThemeContainer SaltyPalette::s_theme;

namespace {

PaletteValues s_values;

// Theme file slot order -> PaletteValues member (nullptr: slot unused by the
// light theme, kept so that theme files stay compatible).
constexpr KDColor PaletteValues::*k_slots[] = {
    &PaletteValues::YellowDark,
    &PaletteValues::YellowLightBattery,
    &PaletteValues::YellowLight,
    &PaletteValues::PurpleBright,
    &PaletteValues::PurpleDark,
    &PaletteValues::GrayWhite,
    &PaletteValues::GrayBright,
    &PaletteValues::GrayMiddle,
    &PaletteValues::GrayDarkMiddle,
    &PaletteValues::GrayDark,
    &PaletteValues::GrayVeryDark,
    &PaletteValues::GrayDarkest,
    &PaletteValues::Select,
    &PaletteValues::SelectDark,
    &PaletteValues::WallScreen,
    &PaletteValues::WallScreenDark,
    &PaletteValues::BoxTitleBackground,  // PopUpTitleBackground
    &PaletteValues::LowBattery,
    &PaletteValues::Red,
    &PaletteValues::RedLight,
    &PaletteValues::Magenta,
    &PaletteValues::MagentaLight,
    &PaletteValues::Turquoise,
    &PaletteValues::TurquoiseLight,
    &PaletteValues::Pink,
    &PaletteValues::PinkLight,
    &PaletteValues::Blue,
    &PaletteValues::BlueLight,
    &PaletteValues::Orange,
    &PaletteValues::OrangeLight,
    &PaletteValues::Green,
    &PaletteValues::GreenLight,
    nullptr,  // Brown
    nullptr,  // Purple
    &PaletteValues::PYTHON_BlueishGray,  // BlueishGray
    nullptr,  // Cyan
    &PaletteValues::Violet,
    &PaletteValues::VioletLight,
    &PaletteValues::Mint,
    &PaletteValues::MintLight,
    &PaletteValues::TextColor,
    &PaletteValues::TextColorHover,
    &PaletteValues::WallpaperColor,
};
static_assert(sizeof(k_slots) / sizeof(k_slots[0]) ==
              SaltyPalette::numberOfSlots());

}  // namespace

void SaltyPalette::Apply(const KDColor* colors, size_t count) {
  s_values = k_defaultPaletteValues;
  size_t n = count < numberOfSlots() ? count : numberOfSlots();
  for (size_t i = 0; i < n; i++) {
    if (k_slots[i] != nullptr) {
      s_values.*k_slots[i] = colors[i];
    }
  }
  s_theme = MakeLightTheme(s_values);
  s_active = true;
}

void SaltyPalette::Reset() { s_active = false; }

KDColor SaltyPalette::WallpaperColor() {
  return s_active ? s_values.WallpaperColor : k_defaultPaletteValues.WallpaperColor;
}
KDColor SaltyPalette::TextColor() {
  return s_active ? s_values.TextColor : k_defaultPaletteValues.TextColor;
}
KDColor SaltyPalette::TextColorHover() {
  return s_active ? s_values.TextColorHover
                  : k_defaultPaletteValues.TextColorHover;
}

}  // namespace Escher
