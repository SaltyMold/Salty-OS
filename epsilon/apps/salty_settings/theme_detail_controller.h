#pragma once

#include "info_list_controller.h"
#include "theme_helpers.h"

namespace SaltySettings {

/* Every piece of information about one theme: general variables, wallpaper,
 * palette (all the slots with their color) and icons. Theme 0 is the virtual
 * default theme, theme t > 0 is the flashed theme t - 1. */
class ThemeDetailController : public InfoListController {
 public:
  ThemeDetailController(Escher::Responder* parentResponder, InfoRow* rows)
      : InfoListController(parentResponder, rows), m_theme(0) {
    m_title[0] = 0;
  }
  const char* title() const override { return m_title; }
  void setTheme(int theme);

 private:
  void describe(RowPicker* picker) const override;

  int m_theme;
  char m_title[k_maxThemeNameSize];
};

}  // namespace SaltySettings
