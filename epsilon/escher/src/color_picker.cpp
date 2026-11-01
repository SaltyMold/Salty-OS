#include <escher/layout_preferences.h>
#include <escher/palette.h>
#include <escher/theme/color_picker.h>
#include <escher/theme/dark_theme.h>
#include <escher/theme/light_theme.h>
#include <escher/theme/salty_palette.h>
#include <omg/unreachable.h>

namespace Escher {

KDColor ColorPicker::ResolveCustomColor(KDColor color) {
  switch (GetCurrentTheme()) {
    case Theme::Light:
      return color;
    case Theme::Dark:
      return SaltyPalette::IsActive() ? SaltyPalette::Values().Background
                                      : Palette::Black;
    default:
      OMG_UNREACHABLE
  }
}

KDColor ColorPicker::ResolveCustomColorForHighlight(KDColor color) {
  switch (GetCurrentTheme()) {
    case Theme::Light:
      return color;
    case Theme::Dark:
      return SaltyPalette::IsActive() ? SaltyPalette::Values().Accent
                                      : Palette::DARK_VioletDeep;
    default:
      OMG_UNREACHABLE
  }
}

KDColor ColorPicker::ResolveCustomColorForText(KDColor color) {
  switch (GetCurrentTheme()) {
    case Theme::Light:
      return color;
    case Theme::Dark:
      return SaltyPalette::IsActive() ? SaltyPalette::Values().PrimaryText
                                      : Palette::DARK_VioletPale;
    default:
      OMG_UNREACHABLE
  }
}

ColorPicker::BlendLevel ColorPicker::GetBlendLevel() {
  switch (GetCurrentTheme()) {
    case Theme::Light:
      return BlendLevel::High;
    case Theme::Dark:
      return BlendLevel::Low;
    default:
      OMG_UNREACHABLE
  }
}

ColorPicker::SelectionThickness ColorPicker::GetSelectionThickness() {
  switch (GetCurrentTheme()) {
    case Theme::Light:
      return SelectionThickness::Simple;
    case Theme::Dark:
      return SelectionThickness::Double;
    default:
      OMG_UNREACHABLE
  }
}

bool ColorPicker::HasTableGrid() {
  switch (GetCurrentTheme()) {
    case Theme::Light:
      return false;
    case Theme::Dark:
      return true;
    default:
      OMG_UNREACHABLE
  }
}

Theme ColorPicker::GetCurrentTheme() {
  // A Salty-OS theme overrides the user preference, light or dark depending on
  // its background
  if (SaltyPalette::IsActive()) {
    return SaltyPalette::IsDark() ? Theme::Dark : Theme::Light;
  }
  return SharedPreferences->theme();
}

KDColor ColorPicker::Get(Theme theme, ThemeComponent component) {
  // The colors of a Salty-OS theme are all in its palette, light or dark
  if (SaltyPalette::IsActive()) {
    return SaltyPalette::Theme().get(component);
  }
  switch (theme) {
    case Theme::Light:
      return k_lightTheme.get(component);
    case Theme::Dark:
      return k_darkTheme.get(component);
    default:
      OMG_UNREACHABLE
  }
}

}  // namespace Escher
