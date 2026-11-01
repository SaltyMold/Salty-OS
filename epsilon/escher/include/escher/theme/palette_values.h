#pragma once

#include <escher/palette.h>
#include <kandinsky/color.h>
#include <stddef.h>

namespace Escher {

/* Every color a Salty-OS theme can change, by name. This is the only list to
 * edit by hand to add one (plus its default value in palette.h and in
 * themes/themes/epsilon.json, see `python3 themes/themes_manager.py --check`).
 *
 * A flashed theme describes its palette with these very names (see
 * generate_themes.py), the firmware looks each of them up when the theme is
 * selected: changing, adding or removing colors in a theme, or reordering
 * them, never needs a rebuild. The core colors come first, then one group of
 * colors per app (group + key in the JSON = name here). The light theme is built
 * from these values (see MakeLightTheme) and SaltyPalette fills them at runtime
 * from the theme chosen by the user. */
#define SALTY_PALETTE_SLOTS(X) \
  /* Core colors */ \
  X(YellowDark) \
  X(YellowLightBattery) \
  X(YellowLight) \
  X(PurpleBright) \
  X(PurpleDark) \
  X(GrayWhite) \
  X(GrayBright) \
  X(GrayMiddle) \
  X(GrayDarkMiddle) \
  X(GrayDark) \
  X(GrayVeryDark) \
  X(GrayDarkest) \
  X(Select) \
  X(SelectDark) \
  X(WallScreen) \
  X(WallScreenDark) \
  X(PopUpTitleBackground) \
  X(LowBattery) \
  X(Red) \
  X(RedLight) \
  X(Magenta) \
  X(MagentaLight) \
  X(Turquoise) \
  X(TurquoiseLight) \
  X(Pink) \
  X(PinkLight) \
  X(Blue) \
  X(BlueLight) \
  X(Orange) \
  X(OrangeLight) \
  X(Green) \
  X(GreenLight) \
  X(Brown) \
  X(Purple) \
  X(BlueishGray) \
  X(Cyan) \
  X(Violet) \
  X(VioletLight) \
  X(Mint) \
  X(MintLight) \
  X(TextColor) \
  X(TextColorHover) \
  X(WallpaperColor) \
  X(White) \
  X(Black) \
  X(Background) \
  X(InputBackground) \
  X(PrimaryText) \
  X(Accent) \
  /* Calculation */ \
  X(CalculationBorderEven) \
  X(CalculationBorderOdd) \
  X(CalculationBorderSelected) \
  /* Python */ \
  X(PythonText) \
  X(PythonKeyword) \
  X(PythonGutterBackground) \
  X(PythonComment) \
  X(PythonNumber) \
  X(PythonOperator) \
  X(PythonString) \
  X(PythonAutocomplete) \
  /* Graph */ \
  X(GraphCurve) \
  X(GraphCurveHatchedArea) \
  X(GraphIntersectionArea) \
  X(GraphTangent) \
  X(GraphIndicator) \
  X(GraphIndicatorSecondary) \
  X(GraphPolarGridLabels) \
  X(GraphPolarGridLight) \
  X(GraphSpecialLabels) \
  X(GraphInterestPoint) \
  X(GraphAxisMain) \
  X(GraphAxisLight) \
  X(GraphAxisMedium) \
  X(GraphAxisSecondary) \
  X(GraphMagnifyIndicator) \
  /* Elements */ \
  X(ElementsConnector) \
  X(ElementsBanner) \
  X(ElementsBannerBorder)

struct PaletteValues {
#define X(name) KDColor name;
  SALTY_PALETTE_SLOTS(X)
#undef X
};

// What the OS uses when no flashed theme is selected, or for a color a theme does not provide
constexpr PaletteValues k_defaultPaletteValues = {
#define X(name) .name = Palette::name,
    SALTY_PALETTE_SLOTS(X)
#undef X
};

constexpr size_t k_numberOfPaletteSlots = 0
#define X(name) +1
    SALTY_PALETTE_SLOTS(X)
#undef X
    ;

}  // namespace Escher
