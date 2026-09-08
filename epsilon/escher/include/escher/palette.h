#pragma once

#include <kandinsky/color.h>
#include <stddef.h>

#include <array>

namespace Escher {

class Palette {
 public:
  // --- Runtime-overridable colors -------------------------------------
  // These used to be `constexpr static`, which meant they lived in
  // flash/rodata and could never change. They are now plain mutable
  // statics (still with the same compiled-in defaults below), so they
  // live in RAM (.data) and can be overwritten at runtime - see
  // ApplyPalette() below, called by ThemeManager when a themed palette
  // is flashed. Everywhere that read `Palette::TextColor` etc. keeps
  // working unchanged.
  static inline KDColor YellowDark = KDColor::RGB24(0xff0000);
  static inline KDColor YellowLightBattery = KDColor::RGB24(0xff0000);
  static inline KDColor YellowLight = KDColor::RGB24(0xff0000);
  static inline KDColor PurpleBright = KDColor::RGB24(0xb56b9a);
  static inline KDColor PurpleDark = KDColor::RGB24(0x70405f);
  static inline KDColor GrayWhite = KDColor::RGB24(0xfff8fb);
  static inline KDColor GrayBright = KDColor::RGB24(0xffedf4);
  static inline KDColor GrayMiddle = KDColor::RGB24(0xf4dce8);
  static inline KDColor GrayDarkMiddle = KDColor::RGB24(0xd9a8bd);
  static inline KDColor GrayDark = KDColor::RGB24(0xc48fa8);
  static inline KDColor GrayVeryDark = KDColor::RGB24(0x9b687f);
  static inline KDColor GrayDarkest = KDColor::RGB24(0x4a2939);
  static inline KDColor Select = KDColor::RGB24(0xf5c6d9);
  static inline KDColor SelectDark = KDColor::RGB24(0xe49ab8);
  static inline KDColor WallScreen = KDColor::RGB24(0xfff7fb);
  static inline KDColor WallScreenDark = KDColor::RGB24(0xf8dfea);
  static inline KDColor PopUpTitleBackground = KDColor::RGB24(0xb86b96);
  static inline KDColor LowBattery = KDColor::RGB24(0xf04470);
  static inline KDColor Red = KDColor::RGB24(0xf05276);
  static inline KDColor RedLight = KDColor::RGB24(0xffd6df);
  static inline KDColor Magenta = KDColor::RGB24(0xf05b9b);
  static inline KDColor MagentaLight = KDColor::RGB24(0xffd8e9);
  static inline KDColor Turquoise = KDColor::RGB24(0x69c9c2);
  static inline KDColor TurquoiseLight = KDColor::RGB24(0xd5f3ef);
  static inline KDColor Pink = KDColor::RGB24(0xff82ad);
  static inline KDColor PinkLight = KDColor::RGB24(0xffdce8);
  static inline KDColor Blue = KDColor::RGB24(0x8c9fe8);
  static inline KDColor BlueLight = KDColor::RGB24(0xe0e5fa);
  static inline KDColor Orange = KDColor::RGB24(0xf5a36c);
  static inline KDColor OrangeLight = KDColor::RGB24(0xffe1c9);
  static inline KDColor Green = KDColor::RGB24(0x7fc995);
  static inline KDColor GreenLight = KDColor::RGB24(0xdff4e5);
  static inline KDColor Brown = KDColor::RGB24(0xa9826d);
  static inline KDColor Purple = KDColor::RGB24(0x9b5b91);
  static inline KDColor BlueishGray = KDColor::RGB24(0x9ba8b8);
  static inline KDColor Cyan = KDColor::RGB24(0x70d9d2);
  static inline KDColor Violet = KDColor::RGB24(0xb77ce8);
  static inline KDColor VioletLight = KDColor::RGB24(0xe8d5fa);
  static inline KDColor Mint = KDColor::RGB24(0x71cdb5);
  static inline KDColor MintLight = KDColor::RGB24(0xd4f3e8);

  static inline KDColor TextColor = KDColor::RGB24(0xffa6c8);
  static inline KDColor TextColorHover = KDColor::RGB24(0xffffff);

  static inline KDColor WallpaperColor = KDColor::RGB24(0xff0000);

  // DataColor/DataColorLight can no longer be `constexpr` (their
  // initializers reference the now-mutable colors above, so they're not
  // constant expressions anymore), but they stay in RAM as plain arrays
  // and are still readable/indexable exactly like before.
  static inline KDColor DataColor[] = {Red,     Blue,      Green, YellowDark,
                                        Magenta, Turquoise, Pink,  Orange,
                                        Violet,  Mint};
  static inline KDColor DataColorLight[] = {
      RedLight,       BlueLight, GreenLight,  YellowLight, MagentaLight,
      TurquoiseLight, PinkLight, OrangeLight, VioletLight, MintLight};

  static constexpr size_t numberOfDataColors() { return std::size(DataColor); }
  static constexpr size_t numberOfLightDataColors() {
    return std::size(DataColorLight);
  }
  static KDColor nextDataColor(int* colorIndex);

  // --- Palette override (theme support) -------------------------------
  // Overwrites the first `count` themable color slots (in the fixed order
  // of s_paletteSlots below) with `colors`. If `count` is smaller than
  // the number of slots, the remaining slots keep whatever value they
  // currently have (their compiled-in default, or a previous theme's
  // value if one was already applied). This order must stay in sync with
  // the palette.txt slot order documented in generate_theme.py.
  static void ApplyPalette(const KDColor* colors, size_t count);
  static constexpr size_t numberOfPaletteSlots() {
    return std::size(s_paletteSlots);
  }

 private:
  // Order = palette.txt slot order. Pointers (not indices) so ApplyPalette
  // doesn't need a big switch statement - taking the address of a static
  // data member is a compile-time constant even though the member itself
  // is mutable, so this table can live fully in .rodata while the KDColor
  // values it points to live in RAM.
  static inline KDColor* const s_paletteSlots[] = {
      &YellowDark,      &YellowLightBattery, &YellowLight,
      &PurpleBright,    &PurpleDark,
      &GrayWhite,       &GrayBright,         &GrayMiddle,
      &GrayDarkMiddle,  &GrayDark,           &GrayVeryDark, &GrayDarkest,
      &Select,          &SelectDark,
      &WallScreen,      &WallScreenDark,
      &PopUpTitleBackground,
      &LowBattery,
      &Red,             &RedLight,
      &Magenta,         &MagentaLight,
      &Turquoise,       &TurquoiseLight,
      &Pink,            &PinkLight,
      &Blue,            &BlueLight,
      &Orange,          &OrangeLight,
      &Green,           &GreenLight,
      &Brown,
      &Purple,
      &BlueishGray,
      &Cyan,
      &Violet,          &VioletLight,
      &Mint,            &MintLight,
      &TextColor,       &TextColorHover,
      &WallpaperColor,
  };
};

}  // namespace Escher