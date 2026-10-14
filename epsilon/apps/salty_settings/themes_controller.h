#pragma once

#include <escher/buffer_text_view.h>
#include <escher/chevron_view.h>
#include <escher/menu_cell.h>
#include <escher/metric.h>
#include <escher/selectable_list_view_controller.h>
#include <escher/stack_view_controller.h>

#include "theme_detail_controller.h"

namespace SaltySettings {

/* One folder per theme of the Theme Area (plus the default theme). Opening a
 * folder shows everything about the theme. */
class ThemesController
    : public Escher::SelectableListViewController<
          Escher::StandardMemoizedListViewDataSource> {
 public:
  ThemesController(Escher::Responder* parentResponder, InfoRow* rows);

  // ViewController
  const char* title() const override;
  void initView() override;
  void viewWillAppear() override;
  void viewDidDisappear() override;
  bool handleEvent(Ion::Events::Event event) override;

  // ListViewDataSource
  int numberOfRows() const override;
  KDCoordinate nonMemoizedRowHeight(int row) override;
  Escher::HighlightCell* reusableCell(int index, int type) override;
  int reusableCellCount(int type) const override;
  int typeAtRow(int row) const override { return 0; }
  void fillCellForRow(Escher::HighlightCell* cell, int row) override;

 private:
  constexpr static int k_numberOfCells =
      Escher::Metric::MinimalNumberOfScrollableRowsToFillDisplayHeight(
          Escher::AbstractMenuCell::k_minimalLargeFontCellHeight);

  using ThemeCell = Escher::MenuCell<
      Escher::OneLineBufferTextView<KDFont::Size::Large>,
      Escher::OneLineBufferTextView<KDFont::Size::Small>,
      Escher::ChevronView>;

  Escher::StackViewController* stackController() const {
    return static_cast<Escher::StackViewController*>(parentResponder());
  }

  ThemeCell m_cells[k_numberOfCells];
  ThemeDetailController m_detailController;
};

}  // namespace SaltySettings
