#include "themes_controller.h"

#include <assert.h>

#include "theme_helpers.h"

using namespace Escher;

namespace SaltySettings {

ThemesController::ThemesController(Responder* parentResponder, InfoRow* rows)
    : SelectableListViewController(parentResponder),
      m_detailController(parentResponder, rows) {}

const char* ThemesController::title() const {
  return I18n::translate(I18n::Message::SaltySettingsThemes);
}

void ThemesController::initView() {
  ViewController::initView();
  m_selectableListView.resetSizeAndOffsetMemoization();
  selectRow(0);
  setOffset(KDPointZero);
}

void ThemesController::viewWillAppear() {
  ViewController::viewWillAppear();
  // The active theme can have changed (shortcuts) since last time
  m_selectableListView.reloadData();
}

void ThemesController::viewDidDisappear() {
  m_selectableListView.deselectTable();
}

bool ThemesController::handleEvent(Ion::Events::Event event) {
  if (event == Ion::Events::Left) {
    stackController()->pop();
    return true;
  }
  if (event == Ion::Events::OK || event == Ion::Events::EXE ||
      event == Ion::Events::Right) {
    m_detailController.setTheme(selectedRow());
    stackController()->push(&m_detailController);
    return true;
  }
  return false;
}

int ThemesController::numberOfRows() const { return ThemeManager::themeCount(); }

KDCoordinate ThemesController::nonMemoizedRowHeight(int row) {
  ThemeCell tempCell;
  return protectedNonMemoizedRowHeight(&tempCell, row);
}

HighlightCell* ThemesController::reusableCell(int index, int type) {
  assert(type == 0 && index >= 0 && index < k_numberOfCells);
  return &m_cells[index];
}

int ThemesController::reusableCellCount(int type) const {
  return k_numberOfCells;
}

void ThemesController::fillCellForRow(HighlightCell* cell, int row) {
  ThemeCell* themeCell = static_cast<ThemeCell*>(cell);
  char name[k_maxThemeNameSize];
  ThemeName(row, name);
  themeCell->label()->setText(name);
  themeCell->subLabel()->setText(
      ThemeManager::selectedTheme() == row
          ? I18n::translate(I18n::Message::SaltySettingsActive)
          : "");
}

}  // namespace SaltySettings
