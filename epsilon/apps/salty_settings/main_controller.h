#pragma once

#include <escher/chevron_view.h>
#include <escher/menu_cell.h>
#include <escher/message_text_view.h>
#include <escher/selectable_list_view_controller.h>
#include <escher/stack_view_controller.h>

#include "about_controller.h"
#include "themes_controller.h"

namespace SaltySettings {

/* Root of the app: two folders, "About" and "Themes". */
class MainController
    : public Escher::UniformSelectableListController<
          Escher::MenuCell<Escher::MessageTextView, Escher::EmptyCellWidget,
                           Escher::ChevronView>,
          2> {
 public:
  MainController(Escher::Responder* parentResponder);
  bool handleEvent(Ion::Events::Event event) override;
  void viewWillAppear() override;

 private:
  constexpr static int k_aboutRow = 0;
  constexpr static int k_themesRow = 1;

  Escher::StackViewController* stackController() const {
    return static_cast<Escher::StackViewController*>(parentResponder());
  }

  // Cache of the rows of the About page and of the theme pages
  InfoRow m_rows[k_maxInfoRows];
  AboutController m_aboutController;
  ThemesController m_themesController;
};

}  // namespace SaltySettings
