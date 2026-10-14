#include "main_controller.h"

using namespace Escher;

namespace SaltySettings {

MainController::MainController(Responder* parentResponder)
    : UniformSelectableListController(parentResponder),
      m_aboutController(parentResponder, m_rows),
      m_themesController(parentResponder, m_rows) {
  cell(k_aboutRow)->label()->setMessage(I18n::Message::SaltySettingsAbout);
  cell(k_themesRow)->label()->setMessage(I18n::Message::SaltySettingsThemes);
}

void MainController::viewWillAppear() {
  ViewController::viewWillAppear();
  m_selectableListView.reloadData();
}

bool MainController::handleEvent(Ion::Events::Event event) {
  if (event == Ion::Events::OK || event == Ion::Events::EXE ||
      event == Ion::Events::Right) {
    ViewController* next = selectedRow() == k_aboutRow
                               ? static_cast<ViewController*>(&m_aboutController)
                               : &m_themesController;
    stackController()->push(next);
    return true;
  }
  return false;
}

}  // namespace SaltySettings
