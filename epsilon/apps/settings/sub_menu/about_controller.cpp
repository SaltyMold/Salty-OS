#include "about_controller.h"

#include <apps/apps_container.h>
#include <apps/exam_mode_manager.h>
#include <apps/salty_info.h>
#include <apps/shortcuts.h>
#include <apps/theme_manager.h>
#include <assert.h>
#include <ion/device_name.h>
#include <ion/external_apps.h>
#include <ion/src/shared/events.h>
#include <string.h>

#include <cmath>

#include "about_text.h"

#if PLATFORM_DEVICE
// Defined by the linker script, see userland_shared.ld
extern char _external_apps_flash_start;
extern char _external_apps_flash_end;
extern char _external_apps_RAM_start;
extern char _external_apps_RAM_end;
#endif

using namespace Escher;

namespace Settings {

/* Shortcut key names come from the macros of shortcuts.h, so they follow any
 * compile-time remapping: "Ion::Keyboard::Key::Alpha" -> "Alpha". */
#define SALTY_STRINGIFY_(x) #x
#define SALTY_STRINGIFY(x) SALTY_STRINGIFY_(x)

static const char* ShortKeyName(const char* qualifiedName) {
  const char* last = qualifiedName;
  for (const char* c = qualifiedName; *c != 0; c++) {
    if (*c == ':') {
      last = c + 1;
    }
  }
  return last;
}

// Size of the value of a row: OneLineBufferTextView<Small>
constexpr static int k_valueSize = 46;

AboutController::AboutController(Responder* parentResponder)
    : GenericSubController(parentResponder),
      m_numberOfRows(0),
      m_showKernelCommit(false),
      m_showSaltyOsCommit(false),
#ifdef HARDWARE_CHECKUP
      m_hardwareCheckupPopUpController(
          Escher::Invocation(
              [](void* context, void* sender) {
                AppsContainer* appsContainer =
                    AppsContainer::sharedAppsContainer();
                appsContainer->switchToBuiltinApp(
                    appsContainer->hardwareCheckupAppSnapshot());
                return true;
              },
              this),
          I18n::Message::HardwareCheckupLaunch),
#endif
      m_hardwareTestPopUpController(
          Escher::Invocation(
              [](void* context, void* sender) {
                AppsContainer* appsContainer =
                    AppsContainer::sharedAppsContainer();
                appsContainer->switchToBuiltinApp(
                    appsContainer->hardwareTestAppSnapshot());
                return true;
              },
              this),
          I18n::Message::HardwareTestLaunch) {
  m_selectableListView.setDelegate(this);
}

bool AboutController::isSelectedRowOfType(CellType type) {
  int row = selectedRow();
  return row >= 0 && row < m_numberOfRows && cellTypeAtRow(row) == type;
}

bool AboutController::handleEvent(Ion::Events::Event event) {
  if (m_messageTreeModel->label() == I18n::Message::About &&
      !ExamModeManager::ExamMode().isActive()) {
    /* We hide here the activation for the hardware tests.
     * In the menu "About", on any cell,
     * - press '6' for default tests
     * - press '1' for variant (tests all keys on keyboard)
     * Also prevent hardware test in exam mode so that the LED can't be
     * accessed. */
    bool isDefaultKey = event == Ion::Events::Six ||
                        event == Ion::Events::LowerT ||
                        event == Ion::Events::UpperT;
    bool isVariantKey = event == Ion::Events::One ||
                        event == Ion::Events::LowerW ||
                        event == Ion::Events::UpperW;
    if (isDefaultKey || isVariantKey) {
      AppsContainer::sharedAppsContainer()->setHardwareTestVariant(
          isDefaultKey ? HardwareTest::Variant::Default
                       : HardwareTest::Variant::TestAllKeys);
      m_hardwareTestPopUpController.presentModally();
      return true;
    }
  }
#ifdef HARDWARE_CHECKUP
  /* We hide here the activation of the hardware checkup app: in the menu
   * "about", by clicking on '7' on the hardware checkup row. */
  if (event == Ion::Events::Seven &&
      m_messageTreeModel->label() == I18n::Message::About &&
      isSelectedRowOfType(CellType::Version) &&
      !ExamModeManager::ExamMode().isActive()) {
    // Prevent hardware checkup in exam mode.
    m_hardwareCheckupPopUpController.presentModally();
    return true;
  }
#endif
  if (event == Ion::Events::OK || event == Ion::Events::EXE) {
    /* When pressing OK on a version, the row switches between the version
     * and the commit (patch level) it was built from. */
    if (isSelectedRowOfType(CellType::Version)) {
      m_showKernelCommit = !m_showKernelCommit;
      m_selectableListView.reloadData(false);
      return true;
    }
    if (isSelectedRowOfType(CellType::SaltyOsVersion)) {
      m_showSaltyOsCommit = !m_showSaltyOsCommit;
      m_selectableListView.reloadData(false);
      return true;
    }
#if TERMS_OF_USE
    if (isSelectedRowOfType(CellType::TermsOfUse)) {
      Ion::Events::openURL(I18n::translate(I18n::Message::TermsOfUseLink));
      return true;
    }
#endif
    return false;
  }
  return GenericSubController::handleEvent(event);
}

void AboutController::initView() {
  /* Called each time the page is opened, before the first row is selected:
   * the Theme Area might have changed since last time. */
  updateRows();
  GenericSubController::initView();
}

int AboutController::initialSelectedRow() const {
  // The first row is the title of a section
  for (int row = 0; row < m_numberOfRows; row++) {
    if (!IsSection(cellTypeAtRow(row))) {
      return row;
    }
  }
  return 0;
}

int AboutController::numberOfRows() const { return m_numberOfRows; }

int AboutController::typeAtRow(int row) const {
  assert(row >= 0 && row < m_numberOfRows);
  return IsSection(cellTypeAtRow(row)) ? k_sectionCellType : k_rowCellType;
}

void AboutController::listViewDidChangeSelectionAndDidScroll(
    SelectableListView* l, int previousRow, KDPoint previousOffset,
    bool withinTemporarySelection) {
  assert(l == &m_selectableListView);
  if (withinTemporarySelection) {
    return;
  }
  /* Escher only scrolls enough to display the selected row: when going up to
   * the first row of a section, its title would stay hidden above it. */
  const int row = l->selectedRow();
  if (row > 0 && row < m_numberOfRows && IsSection(cellTypeAtRow(row - 1))) {
    l->scrollToCell(row - 1);
  }
}

bool AboutController::canSelectCellAtRow(int row) {
  return row >= 0 && row < m_numberOfRows &&
         typeAtRow(row) != k_sectionCellType;
}

HighlightCell* AboutController::reusableCell(int index, int type) {
  assert(index >= 0);
  if (type == k_sectionCellType) {
    assert(index < k_numberOfSectionCells);
    return &m_sectionCells[index];
  }
  assert(type == k_rowCellType && index < k_numberOfRowCells);
  return &m_rowCells[index];
}

int AboutController::reusableCellCount(int type) const {
  return type == k_sectionCellType ? k_numberOfSectionCells
                                   : k_numberOfRowCells;
}

KDCoordinate AboutController::nonMemoizedRowHeight(int row) {
  assert(row >= 0 && row < m_numberOfRows);
  if (IsSection(cellTypeAtRow(row))) {
    return AboutSectionCell::k_height;
  }
  /* The height depends on the text: a value that does not fit next to its
   * label is displayed below it. It is measured on a cell filled with the
   * content of the row, not on one of the reusable cells that can currently
   * be displaying another row. */
  AboutCell tempCell;
  return protectedNonMemoizedRowHeight(&tempCell, row);
}

void AboutController::fillCellForRow(HighlightCell* cell, int row) {
  assert(row >= 0 && row < m_numberOfRows);
  const CellType type = cellTypeAtRow(row);
  const I18n::Message label =
      m_messageTreeModel->childAtIndex(Row(type))->label();
  if (IsSection(type)) {
    static_cast<AboutSectionCell*>(cell)->textView()->setText(
        I18n::translate(label));
    return;
  }
  char buffer[k_valueSize];
  AboutCell* aboutCell = static_cast<AboutCell*>(cell);
  aboutCell->label()->setMessage(label);
  aboutCell->subLabel()->setText(valueOfCellType(type, buffer, k_valueSize));
}

bool AboutController::isCellTypeShown(CellType type) const {
  switch (type) {
    case CellType::FCCID:
      return strncmp(Ion::fccId(), "NA", 3) != 0;
    case CellType::ThemeAreaFormat:
    case CellType::InstalledThemes:
      // Nothing to read in a Theme Area that is not valid
      return ThemeManager::isValid();
    case CellType::ExternalAppsStorage:
    case CellType::ExternalAppsRam:
      // These areas only exist on the calculator
#if PLATFORM_DEVICE
      return true;
#else
      return false;
#endif
    default:
      return true;
  }
}

void AboutController::updateRows() {
  m_numberOfRows = 0;
  for (int i = 0; i < k_totalNumberOfCell; i++) {
    if (isCellTypeShown(static_cast<CellType>(i))) {
      m_rowTypes[m_numberOfRows++] = i;
    }
  }
  m_showKernelCommit = false;
  m_showSaltyOsCommit = false;
}

const char* AboutController::valueOfCellType(CellType type, char* buffer,
                                             int size) const {
  using namespace Ion::Device::Board::Config;
  AboutText text(buffer, size);
  switch (type) {
    // Calculator
    case CellType::Version:
      return m_showKernelCommit ? Ion::commitHash() : Ion::softwareVersion();
    case CellType::SaltyOsVersion:
      return m_showSaltyOsCommit ? SALTY_INFO_COMMIT : SALTY_INFO_OS_VERSION;
    case CellType::EpsilonBaseVersion:
      return SALTY_INFO_EPSILON_SOURCE_VERSION;
    case CellType::BuildDate: {
      // __DATE__ is "Mmm dd yyyy" (dd is padded with a space): "08 Nov 2026"
      const char* date = __DATE__;
      const char day[3] = {date[4] == ' ' ? '0' : date[4], date[5], 0};
      const char month[4] = {date[0], date[1], date[2], 0};
      text.str(day).str(" ").str(month).str(" ").str(date + 7);
      break;
    }
    case CellType::Platform:
      return SALTY_INFO_PLATFORM;
    case CellType::SerialNumber:
      return Ion::serialNumber();
    case CellType::FCCID:
      return Ion::fccId();
    case CellType::PcbRevision:
      return Ion::pcbVersion();
    case CellType::Bootloader:
      return Ion::runningBootloader();
#if TERMS_OF_USE
    case CellType::TermsOfUse:
      return "";
#endif

    // Themes
    case CellType::ThemeAreaStatus:
      return I18n::translate(ThemeManager::isValid()
                                 ? I18n::Message::AboutThemeValid
                                 : I18n::Message::AboutThemeInvalid);
    case CellType::ThemeAreaAddress:
      text.str("0x").hex(static_cast<uint32_t>(ThemeManager::areaBase()), 8);
      break;
    case CellType::ThemeAreaCapacity:
      text.size(ThemeAreaSize);
      break;
    case CellType::ThemeAreaFormat: {
      const uint32_t format =
          reinterpret_cast<const uint32_t*>(ThemeManager::areaBase())[1];
      text.str("v").dec(format);
      if (format != ThemeAreaHeaderVersion) {
        // The firmware expects another format: show it
        text.str(" (v").dec(ThemeAreaHeaderVersion).str(")");
      }
      break;
    }
    case CellType::InstalledThemes:
      // themeCount() includes the virtual default theme
      text.dec(ThemeManager::themeCount() - 1).str("/").dec(ThemeCount);
      break;

    // External apps
    case CellType::ExternalAppsCount:
      text.dec(Ion::ExternalApps::numberOfApps(false));
      break;
    case CellType::ExternalAppsStatus:
      return I18n::translate(
          AppsContainer::sharedAppsContainer()->externalAppsHidden()
              ? I18n::Message::AboutAppsHidden
              : I18n::Message::AboutAppsShown);
    case CellType::ExternalAppsStorage:
#if PLATFORM_DEVICE
      text.size(static_cast<uint32_t>(&_external_apps_flash_end -
                                      &_external_apps_flash_start));
#endif
      break;
    case CellType::ExternalAppsRam:
#if PLATFORM_DEVICE
      text.size(static_cast<uint32_t>(&_external_apps_RAM_end -
                                      &_external_apps_RAM_start));
#endif
      break;

    // Shortcuts
    case CellType::ShortcutHold:
      text.str(ShortKeyName(SALTY_STRINGIFY(SALTY_SHORTCUT_PREFIX_KEY_1)))
          .str(" + ")
          .str(ShortKeyName(SALTY_STRINGIFY(SALTY_SHORTCUT_PREFIX_KEY_2)));
      break;
    case CellType::ShortcutNextTheme:
      return ShortKeyName(SALTY_STRINGIFY(SALTY_SHORTCUT_KEY_NEXT_THEME));
    case CellType::ShortcutPreviousTheme:
      return ShortKeyName(SALTY_STRINGIFY(SALTY_SHORTCUT_KEY_PREVIOUS_THEME));
    case CellType::ShortcutToggleExternalApps:
      return ShortKeyName(
          SALTY_STRINGIFY(SALTY_SHORTCUT_KEY_TOGGLE_EXTERNAL_APPS));

    default:
      // The sections have no value
      assert(false);
      break;
  }
  return buffer;
}

}  // namespace Settings
