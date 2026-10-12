#pragma once

#include <escher/palette.h>
#include <kandinsky/color.h>

namespace Escher {

/* Every Palette color read by the light theme, gathered in one struct so that a
 * light-like theme can be rebuilt at runtime from user-provided colors (see
 * SaltyPalette). The extra WallpaperColor / TextColor / TextColorHover entries
 * are Salty-OS specific (home screen wallpaper and app names). */
struct PaletteValues {
  KDColor WallScreen;
  KDColor WallScreenDark;
  KDColor White;
  KDColor Black;
  KDColor BoxTitleBackground;
  KDColor GrayWhite;
  KDColor GrayMiddle;
  KDColor Select;
  KDColor SelectDark;
  KDColor YellowDark;
  KDColor PurpleBright;
  KDColor GrayDarkMiddle;
  KDColor GrayDark;
  KDColor GrayVeryDark;
  KDColor GrayBright;
  KDColor PurpleDark;
  KDColor Red;
  KDColor GrayDarkest;
  KDColor YellowLightBattery;
  KDColor LowBattery;
  KDColor Blue;
  KDColor Green;
  KDColor Magenta;
  KDColor Turquoise;
  KDColor Pink;
  KDColor Orange;
  KDColor Violet;
  KDColor Mint;
  KDColor RedLight;
  KDColor BlueLight;
  KDColor GreenLight;
  KDColor YellowLight;
  KDColor MagentaLight;
  KDColor TurquoiseLight;
  KDColor PinkLight;
  KDColor OrangeLight;
  KDColor VioletLight;
  KDColor MintLight;
  KDColor PYTHON_BlueishGray;
  KDColor PYTHON_Red;
  KDColor PYTHON_GrayLight;
  // Salty-OS specific
  KDColor TextColor;
  KDColor TextColorHover;
  KDColor WallpaperColor;
};

constexpr PaletteValues k_defaultPaletteValues = {
    .WallScreen = Palette::WallScreen,
    .WallScreenDark = Palette::WallScreenDark,
    .White = Palette::White,
    .Black = Palette::Black,
    .BoxTitleBackground = Palette::BoxTitleBackground,
    .GrayWhite = Palette::GrayWhite,
    .GrayMiddle = Palette::GrayMiddle,
    .Select = Palette::Select,
    .SelectDark = Palette::SelectDark,
    .YellowDark = Palette::YellowDark,
    .PurpleBright = Palette::PurpleBright,
    .GrayDarkMiddle = Palette::GrayDarkMiddle,
    .GrayDark = Palette::GrayDark,
    .GrayVeryDark = Palette::GrayVeryDark,
    .GrayBright = Palette::GrayBright,
    .PurpleDark = Palette::PurpleDark,
    .Red = Palette::Red,
    .GrayDarkest = Palette::GrayDarkest,
    .YellowLightBattery = Palette::YellowLightBattery,
    .LowBattery = Palette::LowBattery,
    .Blue = Palette::Blue,
    .Green = Palette::Green,
    .Magenta = Palette::Magenta,
    .Turquoise = Palette::Turquoise,
    .Pink = Palette::Pink,
    .Orange = Palette::Orange,
    .Violet = Palette::Violet,
    .Mint = Palette::Mint,
    .RedLight = Palette::RedLight,
    .BlueLight = Palette::BlueLight,
    .GreenLight = Palette::GreenLight,
    .YellowLight = Palette::YellowLight,
    .MagentaLight = Palette::MagentaLight,
    .TurquoiseLight = Palette::TurquoiseLight,
    .PinkLight = Palette::PinkLight,
    .OrangeLight = Palette::OrangeLight,
    .VioletLight = Palette::VioletLight,
    .MintLight = Palette::MintLight,
    .PYTHON_BlueishGray = Palette::PYTHON_BlueishGray,
    .PYTHON_Red = Palette::PYTHON_Red,
    .PYTHON_GrayLight = Palette::PYTHON_GrayLight,
    .TextColor = Palette::White,
    .TextColorHover = Palette::Black,
    .WallpaperColor = Palette::White,
};

}  // namespace Escher
