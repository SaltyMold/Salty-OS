#pragma once

#include <apps/shared/pop_up_controller.h>
#include <escher/buffer_text_view.h>
#include <escher/menu_cell.h>
#include <escher/message_text_view.h>
#include <escher/metric.h>

#include "about_section_cell.h"
#include "generic_sub_controller.h"

namespace Settings {

class AboutController : public GenericSubController,
                       public Escher::SelectableListViewDelegate {
 public:
  AboutController(Escher::Responder* parentResponder);
  bool handleEvent(Ion::Events::Event event) override;
  void initView() override;
  int numberOfRows() const override;
  KDCoordinate nonMemoizedRowHeight(int row) override;
  Escher::HighlightCell* reusableCell(int index, int type) override;
  int reusableCellCount(int type) const override;
  int typeAtRow(int row) const override;
  void fillCellForRow(Escher::HighlightCell* cell, int row) override;
  bool canSelectCellAtRow(int row) override;
  void listViewDidChangeSelectionAndDidScroll(
      Escher::SelectableListView* l, int previousRow, KDPoint previousOffset,
      bool withinTemporarySelection) override;

  /* The order of this enum is the order of the rows, and the one of the
   * labels in s_modelAboutChildren (main_controller.cpp). The rows are grouped
   * under the title of a section, that cannot be selected. */
  enum class CellType : uint8_t {
    SectionSoftware = 0,
    SaltyOsVersion,  // OK shows the commit of Salty-OS instead
    EpsilonBaseVersion,
    BuildDate,
    Version,  // The kernel. OK shows its commit instead
    SectionCalculator,
    Platform,
    SerialNumber,
    FCCID,
    PcbRevision,
    Bootloader,
#if TERMS_OF_USE
    TermsOfUse,
#endif
    SectionThemes,
    ThemeAreaStatus,
    ThemeAreaAddress,
    ThemeAreaCapacity,
    ThemeAreaFormat,
    InstalledThemes,
    SectionExternalApps,
    ExternalAppsCount,
    ExternalAppsStatus,
    ExternalAppsStorage,
    ExternalAppsRam,
    SectionShortcuts,
    ShortcutHold,
    ShortcutNextTheme,
    ShortcutPreviousTheme,
    ShortcutToggleExternalApps,
    NumberOfCells
  };

  constexpr static int k_totalNumberOfCell =
      static_cast<int>(CellType::NumberOfCells);
  constexpr static int Row(CellType type) { return static_cast<int>(type); }

 private:
  using AboutCell =
      Escher::MenuCell<Escher::MessageTextView,
                       Escher::OneLineBufferTextView<>>;
  constexpr static int k_rowCellType = 0;
  constexpr static int k_sectionCellType = 1;
  constexpr static int k_numberOfSectionCells = 5;
  /* A cell is not tied to a row: the table gives the cells back and forth to
   * the rows that are displayed. Everything shown in a cell must therefore be
   * computed from its row, in fillCellForRow(). */
  constexpr static int k_numberOfRowCells =
      Escher::Metric::MinimalNumberOfScrollableRowsToFillDisplayHeight(
          Escher::AbstractMenuCell::k_minimalLargeFontCellHeight);

  int initialSelectedRow() const override;
  static bool IsSection(CellType type) {
    return type == CellType::SectionSoftware ||
           type == CellType::SectionCalculator ||
           type == CellType::SectionThemes ||
           type == CellType::SectionExternalApps ||
           type == CellType::SectionShortcuts;
  }
  // Some rows are hidden: row r of the list is cell type m_rowTypes[r]
  CellType cellTypeAtRow(int row) const {
    return static_cast<CellType>(m_rowTypes[row]);
  }
  bool isCellTypeShown(CellType type) const;
  void updateRows();
  bool isSelectedRowOfType(CellType type);
  // The text of a row, written in buffer when it has to be built
  const char* valueOfCellType(CellType type, char* buffer, int size) const;

  int m_numberOfRows;
  uint8_t m_rowTypes[k_totalNumberOfCell];
  // Pressing OK on the version rows switches them to the matching commit
  bool m_showKernelCommit;
  bool m_showSaltyOsCommit;
  AboutCell m_rowCells[k_numberOfRowCells];
  AboutSectionCell m_sectionCells[k_numberOfSectionCells];
#ifdef HARDWARE_CHECKUP
  Shared::MessagePopUpController m_hardwareCheckupPopUpController;
#endif
  Shared::MessagePopUpController m_hardwareTestPopUpController;
};

}  // namespace Settings
