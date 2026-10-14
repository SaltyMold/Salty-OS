#pragma once

#include <escher/buffer_text_view.h>
#include <escher/menu_cell.h>
#include <escher/metric.h>
#include <escher/selectable_list_view_controller.h>
#include <escher/stack_view_controller.h>
#include <kandinsky/coordinate.h>
#include <string.h>

#include "info_text.h"
#include "section_cell.h"
#include "swatch_view.h"

namespace SaltySettings {

/* A scrollable list of "label: value" rows grouped under section titles, as
 * the About page of the Settings app. Subclasses only describe their rows. */
constexpr static int k_infoLabelSize = 33;  // OneLineBufferTextView<Large>
constexpr static int k_infoValueSize = 46;  // OneLineBufferTextView<Small>
// The longest list is the detail of a theme (about 75 rows)
constexpr static int k_maxInfoRows = 80;

struct InfoRow {
  bool isSection;
  bool hasColor;  // The row shows a swatch of `rgb`
  KDCoordinate height;  // Measured once by rebuildRows()
  uint32_t rgb;
  char label[k_infoLabelSize];  // Title of the section for a section
  char value[k_infoValueSize];
};

/* Describes the rows of a list into a cache: each call to section() or row()
 * adds one row. The Line returned by row() writes the value of the row (it
 * silently drops everything if the cache is full). */
class RowPicker {
 public:
  RowPicker(InfoRow* rows, int capacity)
      : m_rows(rows), m_capacity(capacity), m_count(0) {}

  void section(I18n::Message title) { section(I18n::translate(title)); }
  void section(const char* title) {
    InfoRow* r = next();
    if (r != nullptr) {
      r->isSection = true;
      Line(r->label, k_infoLabelSize).str(title);
    }
  }
  Line row(I18n::Message label) { return row(I18n::translate(label)); }
  Line row(const char* label) { return fill(next(), label); }
  // Same as row() with a color square next to the value
  Line colorRow(const char* label, uint32_t rgb) {
    InfoRow* r = next();
    if (r != nullptr) {
      r->hasColor = true;
      r->rgb = rgb;
    }
    return fill(r, label);
  }
  int count() const { return m_count < m_capacity ? m_count : m_capacity; }

 private:
  InfoRow* next() {
    InfoRow* r = m_count < m_capacity ? &m_rows[m_count] : nullptr;
    if (r != nullptr) {
      memset(r, 0, sizeof(InfoRow));
    }
    m_count++;
    return r;
  }
  Line fill(InfoRow* r, const char* label) {
    if (r == nullptr) {
      return Line(nullptr, 0);
    }
    Line(r->label, k_infoLabelSize).str(label);
    return Line(r->value, k_infoValueSize);
  }
  InfoRow* m_rows;
  int m_capacity;
  int m_count;
};

class InfoListController
    : public Escher::SelectableListViewController<
          Escher::StandardMemoizedListViewDataSource> {
 public:
  /* `rows` is the cache of the rows, shared by all the lists since only one is
   * displayed at a time. It has room for k_maxInfoRows rows. */
  InfoListController(Escher::Responder* parentResponder, InfoRow* rows)
      : SelectableListViewController(parentResponder),
        m_rows(rows),
        m_numberOfRows(0) {}

  // ViewController
  void initView() override;
  void viewWillAppear() override;
  void viewDidDisappear() override;
  bool handleEvent(Ion::Events::Event event) override;

  // ListViewDataSource
  int numberOfRows() const override;
  KDCoordinate nonMemoizedRowHeight(int row) override;
  Escher::HighlightCell* reusableCell(int index, int type) override;
  int reusableCellCount(int type) const override;
  int typeAtRow(int row) const override;
  void fillCellForRow(Escher::HighlightCell* cell, int row) override;
  bool canSelectCellAtRow(int row) override;

 protected:
  // Describes every row of the list, see RowPicker
  virtual void describe(RowPicker* picker) const = 0;
  // Computes the rows once, the list then only reads the cache
  void rebuildRows();

  Escher::StackViewController* stackController() const {
    return static_cast<Escher::StackViewController*>(parentResponder());
  }

 private:
  constexpr static int k_defaultCellType = 0;
  constexpr static int k_sectionCellType = 1;
  constexpr static int k_swatchCellType = 2;
  /* Sections are never adjacent to each other and each info row is higher than
   * a section: a few section cells are enough. */
  constexpr static int k_numberOfSectionCells = 6;
  constexpr static int k_numberOfInfoCells =
      Escher::Metric::MinimalNumberOfScrollableRowsToFillDisplayHeight(
          Escher::AbstractMenuCell::k_minimalLargeFontCellHeight);

  using InfoCell = Escher::MenuCell<
      Escher::OneLineBufferTextView<KDFont::Size::Large>,
      Escher::OneLineBufferTextView<KDFont::Size::Small>>;

  using SwatchCell = Escher::MenuCell<
      Escher::OneLineBufferTextView<KDFont::Size::Large>,
      Escher::OneLineBufferTextView<KDFont::Size::Small>, SwatchView>;

  InfoRow* m_rows;
  int m_numberOfRows;
  InfoCell m_infoCells[k_numberOfInfoCells];
  SwatchCell m_swatchCells[k_numberOfInfoCells];
  SectionCell m_sectionCells[k_numberOfSectionCells];
};

}  // namespace SaltySettings
