#pragma once

#include <kandinsky/color.h>
#include <stddef.h>

#include <array>

namespace Escher {

class Palette {
 public:
  constexpr static KDColor YellowDark = KDColor::RGB24(0xffa6c8);
  constexpr static KDColor YellowLightBattery = KDColor::RGB24(0xffd9a8);
  constexpr static KDColor YellowLight = KDColor::RGB24(0xfff1d9);
  constexpr static KDColor PurpleBright = KDColor::RGB24(0xb56b9a);
  constexpr static KDColor PurpleDark = KDColor::RGB24(0x70405f);
  constexpr static KDColor GrayWhite = KDColor::RGB24(0xfff8fb);
  constexpr static KDColor GrayBright = KDColor::RGB24(0xffedf4);
  constexpr static KDColor GrayMiddle = KDColor::RGB24(0xf4dce8);
  constexpr static KDColor GrayDarkMiddle = KDColor::RGB24(0xd9a8bd);
  constexpr static KDColor GrayDark = KDColor::RGB24(0xc48fa8);
  constexpr static KDColor GrayVeryDark = KDColor::RGB24(0x9b687f);
  constexpr static KDColor GrayDarkest = KDColor::RGB24(0x4a2939);
  constexpr static KDColor Select = KDColor::RGB24(0xf5c6d9);
  constexpr static KDColor SelectDark = KDColor::RGB24(0xe49ab8);
  constexpr static KDColor WallScreen = KDColor::RGB24(0xfff7fb);
  constexpr static KDColor WallScreenDark = KDColor::RGB24(0xf8dfea);
  constexpr static KDColor PopUpTitleBackground = KDColor::RGB24(0xb86b96);
  constexpr static KDColor LowBattery = KDColor::RGB24(0xf04470);
  constexpr static KDColor Red = KDColor::RGB24(0xf05276);
  constexpr static KDColor RedLight = KDColor::RGB24(0xffd6df);
  constexpr static KDColor Magenta = KDColor::RGB24(0xf05b9b);
  constexpr static KDColor MagentaLight = KDColor::RGB24(0xffd8e9);
  constexpr static KDColor Turquoise = KDColor::RGB24(0x69c9c2);
  constexpr static KDColor TurquoiseLight = KDColor::RGB24(0xd5f3ef);
  constexpr static KDColor Pink = KDColor::RGB24(0xff82ad);
  constexpr static KDColor PinkLight = KDColor::RGB24(0xffdce8);
  constexpr static KDColor Blue = KDColor::RGB24(0x8c9fe8);
  constexpr static KDColor BlueLight = KDColor::RGB24(0xe0e5fa);
  constexpr static KDColor Orange = KDColor::RGB24(0xf5a36c);
  constexpr static KDColor OrangeLight = KDColor::RGB24(0xffe1c9);
  constexpr static KDColor Green = KDColor::RGB24(0x7fc995);
  constexpr static KDColor GreenLight = KDColor::RGB24(0xdff4e5);
  constexpr static KDColor Brown = KDColor::RGB24(0xa9826d);
  constexpr static KDColor Purple = KDColor::RGB24(0x9b5b91);
  constexpr static KDColor BlueishGray = KDColor::RGB24(0x9ba8b8);
  constexpr static KDColor Cyan = KDColor::RGB24(0x70d9d2);
  constexpr static KDColor Violet = KDColor::RGB24(0xb77ce8);
  constexpr static KDColor VioletLight = KDColor::RGB24(0xe8d5fa);
  constexpr static KDColor Mint = KDColor::RGB24(0x71cdb5);
  constexpr static KDColor MintLight = KDColor::RGB24(0xd4f3e8);

  constexpr static KDColor TextColor = KDColor::RGB24(0xffa6c8);
  constexpr static KDColor TextColorHover = KDColor::RGB24(0xffffff);


  constexpr static KDColor DataColor[] = {Red,     Blue,      Green, YellowDark,
                                          Magenta, Turquoise, Pink,  Orange,
                                          Violet,  Mint};
  constexpr static KDColor DataColorLight[] = {
      RedLight,       BlueLight, GreenLight,  YellowLight, MagentaLight,
      TurquoiseLight, PinkLight, OrangeLight, VioletLight, MintLight};

  constexpr static size_t numberOfDataColors() { return std::size(DataColor); }
  constexpr static size_t numberOfLightDataColors() {
    return std::size(DataColorLight);
  }
  static KDColor nextDataColor(int* colorIndex);
};

}  // namespace Escher