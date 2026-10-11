#include "apps_container.h"
#include "shortcuts.h"
#include "theme_manager.h"

/* Salty-OS keybinds handling. The keys themselves are assigned in
 * shortcuts.h: this file only knows about Shortcuts::Action values. */

void AppsContainer::handleShortcuts() {
  const Shortcuts::ActiveSet active(Ion::Keyboard::scan());

  if (active.isNewlyActive(Shortcuts::Action::ToggleExternalApps,
                           m_activeShortcuts)) {
    toggleExternalApps();
  }

  /* Previous theme has priority. If there is no previous theme, the next one
   * is still selected when both shortcuts are held. */
  bool themeChanged =
      active.isActive(Shortcuts::Action::PreviousTheme) && changeTheme(-1);
  if (!themeChanged && active.isActive(Shortcuts::Action::NextTheme)) {
    changeTheme(+1);
  }

  m_activeShortcuts = active;
}

bool AppsContainer::changeTheme(int offset) {
  int targetTheme = ThemeManager::selectedTheme() + offset;
  if (targetTheme < 0 || targetTheme >= ThemeManager::themeCount()) {
    return false;
  }
  ThemeManager::selectTheme(targetTheme);
  resetShiftAlphaStatus();
  ThemeManager::refreshTheme(&m_window);
  return true;
}

void AppsContainer::toggleExternalApps() {
  m_externalAppsHidden = !m_externalAppsHidden;
  if (activeApp() != nullptr && activeApp()->snapshot() == homeAppSnapshot()) {
    Home::App::app()->snapshot()->selectCellAtLocation(0, 0);
    Home::App::app()->reloadAppCells();
    window()->redraw(true);
  }
}
