#pragma once

/* Default colors of the OS (the "Default" theme, and the value of a color that
 * a flashed theme does not provide). Keep them in sync with the default theme
 * themes/themes/epsilon.json: `python3 themes/themes_manager.py --check` tells
 * when they differ. */

#include <kandinsky/color.h>

namespace Escher {

class Palette {
 public:

  // Core
  constexpr static KDColor YellowDark = KDColor::RGB24(0xffb734);
  constexpr static KDColor YellowLightBattery = KDColor::RGB24(0xffcc7b);
  constexpr static KDColor YellowLight = KDColor::RGB24(0xffebc7);
  constexpr static KDColor PurpleBright = KDColor::RGB24(0x656975);
  constexpr static KDColor PurpleDark = KDColor::RGB24(0x414147);
  constexpr static KDColor GrayWhite = KDColor::RGB24(0xf5f5f5);
  constexpr static KDColor GrayBright = KDColor::RGB24(0xececec);
  constexpr static KDColor GrayMiddle = KDColor::RGB24(0xd9d9d9);
  constexpr static KDColor GrayDarkMiddle = KDColor::RGB24(0xb8bbc5);
  constexpr static KDColor GrayDark = KDColor::RGB24(0xa7a7a7);
  constexpr static KDColor GrayVeryDark = KDColor::RGB24(0x8c8c8c);
  constexpr static KDColor GrayDarkest = KDColor::RGB24(0x333333);
  constexpr static KDColor Select = KDColor::RGB24(0xd4d7e0);
  constexpr static KDColor SelectDark = KDColor::RGB24(0xb0b8d8);
  constexpr static KDColor WallScreen = KDColor::RGB24(0xf7f9fa);
  constexpr static KDColor WallScreenDark = KDColor::RGB24(0xe0e6ed);
  constexpr static KDColor PopUpTitleBackground = KDColor::RGB24(0x696475);
  constexpr static KDColor LowBattery = KDColor::RGB24(0xf30211);
  constexpr static KDColor Red = KDColor::RGB24(0xff000c);
  constexpr static KDColor RedLight = KDColor::RGB24(0xffcccc);
  constexpr static KDColor Magenta = KDColor::RGB24(0xff0588);
  constexpr static KDColor MagentaLight = KDColor::RGB24(0xffd9eb);
  constexpr static KDColor Turquoise = KDColor::RGB24(0x60c1ec);
  constexpr static KDColor TurquoiseLight = KDColor::RGB24(0xd0edff);
  constexpr static KDColor Pink = KDColor::RGB24(0xff99aa);
  constexpr static KDColor PinkLight = KDColor::RGB24(0xffe3e7);
  constexpr static KDColor Blue = KDColor::RGB24(0x5075f2);
  constexpr static KDColor BlueLight = KDColor::RGB24(0xdce3fd);
  constexpr static KDColor Orange = KDColor::RGB24(0xfe871f);
  constexpr static KDColor OrangeLight = KDColor::RGB24(0xffc89a);
  constexpr static KDColor Green = KDColor::RGB24(0x50c102);
  constexpr static KDColor GreenLight = KDColor::RGB24(0xdcf3cc);
  constexpr static KDColor Brown = KDColor::RGB24(0x8d7350);
  constexpr static KDColor Purple = KDColor::RGB24(0x6e2d79);
  constexpr static KDColor BlueishGray = KDColor::RGB24(0x919ea4);
  constexpr static KDColor Cyan = KDColor::RGB24(0x00ffff);
  constexpr static KDColor Violet = KDColor::RGB24(0x8d37e9);
  constexpr static KDColor VioletLight = KDColor::RGB24(0xd7b3ff);
  constexpr static KDColor Mint = KDColor::RGB24(0x00c6b1);
  constexpr static KDColor MintLight = KDColor::RGB24(0xb1fef7);
  constexpr static KDColor TextColor = KDColor::RGB24(0xffffff);
  constexpr static KDColor TextColorHover = KDColor::RGB24(0x000000);
  constexpr static KDColor WallpaperColor = KDColor::RGB24(0xffffff);
  constexpr static KDColor White = KDColor::RGB24(0xffffff);
  constexpr static KDColor Black = KDColor::RGB24(0x000000);
  constexpr static KDColor Background = KDColor::RGB24(0xffffff);
  constexpr static KDColor InputBackground = KDColor::RGB24(0xffffff);
  constexpr static KDColor PrimaryText = KDColor::RGB24(0x000000);
  constexpr static KDColor Accent = KDColor::RGB24(0xffb734);

  // Calculation
  constexpr static KDColor CalculationBorderEven = KDColor::RGB24(0xffffff);
  constexpr static KDColor CalculationBorderOdd = KDColor::RGB24(0xf7f9fa);
  constexpr static KDColor CalculationBorderSelected = KDColor::RGB24(0xd4d7e0);

  // Python
  constexpr static KDColor PythonText = KDColor::RGB24(0x919ea4);
  constexpr static KDColor PythonKeyword = KDColor::RGB24(0xff000c);
  constexpr static KDColor PythonGutterBackground = KDColor::RGB24(0xe4e6e7);
  constexpr static KDColor PythonComment = KDColor::RGB24(0x999988);
  constexpr static KDColor PythonNumber = KDColor::RGB24(0x009999);
  constexpr static KDColor PythonOperator = KDColor::RGB24(0xd73a49);
  constexpr static KDColor PythonString = KDColor::RGB24(0x032f62);
  constexpr static KDColor PythonAutocomplete = KDColor::RGB24(0xc6c6c6);

  // Graph
  constexpr static KDColor GraphCurve = KDColor::RGB24(0xffb734);
  constexpr static KDColor GraphCurveHatchedArea = KDColor::RGB24(0x656975);
  constexpr static KDColor GraphIntersectionArea = KDColor::RGB24(0x000000);
  constexpr static KDColor GraphTangent = KDColor::RGB24(0x8c8c8c);
  constexpr static KDColor GraphIndicator = KDColor::RGB24(0xff000c);
  constexpr static KDColor GraphIndicatorSecondary = KDColor::RGB24(0x000000);
  constexpr static KDColor GraphPolarGridLabels = KDColor::RGB24(0xd9d9d9);
  constexpr static KDColor GraphPolarGridLight = KDColor::RGB24(0xececec);
  constexpr static KDColor GraphSpecialLabels = KDColor::RGB24(0xff000c);
  constexpr static KDColor GraphInterestPoint = KDColor::RGB24(0x333333);
  constexpr static KDColor GraphAxisMain = KDColor::RGB24(0x000000);
  constexpr static KDColor GraphAxisLight = KDColor::RGB24(0xf5f5f5);
  constexpr static KDColor GraphAxisMedium = KDColor::RGB24(0xd9d9d9);
  constexpr static KDColor GraphAxisSecondary = KDColor::RGB24(0xa7a7a7);
  constexpr static KDColor GraphMagnifyIndicator = KDColor::RGB24(0x000000);

  // Elements
  constexpr static KDColor ElementsConnector = KDColor::RGB24(0xd9d9d9);
  constexpr static KDColor ElementsBanner = KDColor::RGB24(0xf5f5f5);
  constexpr static KDColor ElementsBannerBorder = KDColor::RGB24(0xd9d9d9);

  // Fixed colors (not part of the runtime themes)
  // Misc
  constexpr static KDColor DARK_VioletDeep = KDColor::RGB24(0x733fff);
  constexpr static KDColor DARK_VioletMiddle = KDColor::RGB24(0x9296f6);
  constexpr static KDColor DARK_VioletPale = KDColor::RGB24(0xc3caf4);
  constexpr static KDColor DARK_AxisMain = KDColor::RGB24(0xdedade);
  constexpr static KDColor DARK_AxisSecondary = KDColor::RGB24(0x636363);
  constexpr static KDColor DARK_Red = KDColor::RGB24(0xff6b70);
  constexpr static KDColor DARK_Blue = KDColor::RGB24(0x7b96f9);
  constexpr static KDColor DARK_Green = KDColor::RGB24(0x4ab300);
  constexpr static KDColor DARK_Yellow = KDColor::RGB24(0xffb531);
  constexpr static KDColor DARK_Magenta = KDColor::RGB24(0xff6bba);
  constexpr static KDColor DARK_Turquoise = KDColor::RGB24(0xa6daf2);
  constexpr static KDColor DARK_Pink = KDColor::RGB24(0xffd6dc);
  constexpr static KDColor DARK_Orange = KDColor::RGB24(0xffa352);
  constexpr static KDColor PYTHON_Red = KDColor::RGB24(0xff000c);
  constexpr static KDColor PYTHON_KhakiGray = KDColor::RGB24(0x999988);
  constexpr static KDColor PYTHON_DarkCyan = KDColor::RGB24(0x009999);
  constexpr static KDColor PYTHON_RaspberryRed = KDColor::RGB24(0xd73a49);
  constexpr static KDColor PYTHON_DarkBlue = KDColor::RGB24(0x032f62);
  constexpr static KDColor PYTHON_GrayMiddle = KDColor::RGB24(0xc6c6c6);
  constexpr static KDColor PYTHON_GrayLight = KDColor::RGB24(0xe4e6e7);
  constexpr static KDColor PYTHON_BlueishGray = KDColor::RGB24(0x919ea4);
};

}  // namespace Escher
