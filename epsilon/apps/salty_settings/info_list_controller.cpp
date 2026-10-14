#include "info_list_controller.h"

#include <assert.h>
#include <string.h>

using namespace Escher;

namespace SaltySettings {

void InfoListController::initView() {
  ViewController::initView();
  m_selectableListView.resetSizeAndOffsetMemoization();
  setOffset(KDPointZero);
}

void InfoListController::viewWillAppear() {
  ViewController::viewWillAppear();
  rebuildRows();
  m_selectableListView.resetSizeAndOffsetMemoization();
  m_selectableListView.reloadData();
  // Start on the first row that can be selected
  for (int row = 0; row < numberOfRows(); row++) {
    if (canSelectCellAtRow(row)) {
      selectRow(row);
      return;
    }
  }
}

void InfoListController::viewDidDisappear() {
  m_selectableListView.deselectTable();
}

bool InfoListController::handleEvent(Ion::Events::Event event) {
  if (event == Ion::Events::Left) {
    stackController()->pop();
    return true;
  }
  return false;
}

void InfoListController::rebuildRows() {
  RowPicker picker(m_rows, k_maxInfoRows);
  describe(&picker);
  m_numberOfRows = picker.count();
  /* The memoization of Escher only covers a few rows: the heights of all the
   * other ones are asked again at each scroll, and measuring a cell is slow
   * (this is what made long lists lag, flicker and even crash). So every row
   * is measured here, once, and nonMemoizedRowHeight() only reads the result. */
  InfoCell infoCell;
  SwatchCell swatchCell;
  for (int row = 0; row < m_numberOfRows; row++) {
    const int type = typeAtRow(row);
    if (type == k_sectionCellType) {
      m_rows[row].height = SectionCell::k_height;
    } else {
      m_rows[row].height = protectedNonMemoizedRowHeight(
          type == k_swatchCellType ? static_cast<HighlightCell*>(&swatchCell)
                                   : static_cast<HighlightCell*>(&infoCell),
          row);
    }
  }
}

int InfoListController::numberOfRows() const { return m_numberOfRows; }

int InfoListController::typeAtRow(int row) const {
  assert(row >= 0 && row < m_numberOfRows);
  if (m_rows[row].isSection) {
    return k_sectionCellType;
  }
  return m_rows[row].hasColor ? k_swatchCellType : k_defaultCellType;
}

bool InfoListController::canSelectCellAtRow(int row) {
  return typeAtRow(row) != k_sectionCellType;
}

KDCoordinate InfoListController::nonMemoizedRowHeight(int row) {
  assert(row >= 0 && row < m_numberOfRows);
  return m_rows[row].height;
}

HighlightCell* InfoListController::reusableCell(int index, int type) {
  assert(index >= 0);
  if (type == k_sectionCellType) {
    assert(index < k_numberOfSectionCells);
    return &m_sectionCells[index];
  }
  assert(index < k_numberOfInfoCells);
  if (type == k_swatchCellType) {
    return &m_swatchCells[index];
  }
  assert(type == k_defaultCellType);
  return &m_infoCells[index];
}

int InfoListController::reusableCellCount(int type) const {
  return type == k_sectionCellType ? k_numberOfSectionCells
                                   : k_numberOfInfoCells;
}

void InfoListController::fillCellForRow(HighlightCell* cell, int row) {
  assert(row >= 0 && row < m_numberOfRows);
  const InfoRow& description = m_rows[row];
  if (description.isSection) {
    static_cast<SectionCell*>(cell)->textView()->setText(description.label);
    return;
  }
  if (description.hasColor) {
    SwatchCell* swatchCell = static_cast<SwatchCell*>(cell);
    swatchCell->label()->setText(description.label);
    swatchCell->subLabel()->setText(description.value);
    swatchCell->accessory()->setColor(KDColor::RGB24(description.rgb));
    return;
  }
  InfoCell* infoCell = static_cast<InfoCell*>(cell);
  infoCell->label()->setText(description.label);
  infoCell->subLabel()->setText(description.value);
}

}  // namespace SaltySettings
