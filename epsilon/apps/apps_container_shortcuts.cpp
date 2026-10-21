#include "apps_container.h"
#include "exam_mode_manager.h"
#include "shortcuts.h"
#include "theme_manager.h"

#if !PLATFORM_DEVICE
#include <ion/timing.h>
#endif

/* Salty-OS keybinds handling. The keys themselves are assigned in
 * shortcuts.h: this file only knows about Shortcuts::Action values. */

void AppsContainer::handleShortcuts() {
  const Shortcuts::ActiveSet active(Ion::Keyboard::scan());

  if (active.isNewlyActive(Shortcuts::Action::ToggleExternalApps,
                           m_activeShortcuts)) {
    toggleExternalApps();
  }

  if (active.isNewlyActive(Shortcuts::Action::OpenSettings, m_activeShortcuts)) {
    toggleSettingsApp();
  }

#if !PLATFORM_DEVICE
  /* Simulator only: it dispatches many events while keys are held and redraws
   * instantly, so a held theme shortcut would run through every theme in a
   * fraction of a second. While the shortcut stays held, themes change at most
   * every 300 ms. Once released, the next press changes the theme at once. */
  constexpr uint64_t k_themeChangeDelayMs = 300;
  static uint64_t s_nextThemeChangeTime = 0;
  if (!active.isActive(Shortcuts::Action::PreviousTheme) &&
      !active.isActive(Shortcuts::Action::NextTheme)) {
    s_nextThemeChangeTime = 0;
  }
  const bool themeChangeAllowed = Ion::Timing::millis() >= s_nextThemeChangeTime;
#else
  const bool themeChangeAllowed = true;
#endif

  /* Previous theme has priority. If there is no previous theme, the next one
   * is still selected when both shortcuts are held. */
  if (themeChangeAllowed) {
    bool themeChanged =
        active.isActive(Shortcuts::Action::PreviousTheme) && changeTheme(-1);
    if (!themeChanged && active.isActive(Shortcuts::Action::NextTheme)) {
      themeChanged = changeTheme(+1);
    }
#if !PLATFORM_DEVICE
    if (themeChanged) {
      s_nextThemeChangeTime = Ion::Timing::millis() + k_themeChangeDelayMs;
    }
#else
    (void)themeChanged;
#endif
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

void AppsContainer::applyPendingThemeArea() {
  if (!ThemeManager::applyPendingThemeArea()) {
    return;
  }
  /* The Salty-OS settings app displays the list of themes: leave it so that
   * it never shows (or opens) a theme that does not exist anymore. */
  if (activeApp() != nullptr &&
      activeApp()->snapshot() == saltySettingsAppSnapshot()) {
    switchToBuiltinApp(homeAppSnapshot());
  }
  resetShiftAlphaStatus();
  ThemeManager::refreshTheme(&m_window);
}

void AppsContainer::toggleExternalApps() {
  m_externalAppsHidden = !m_externalAppsHidden;
  if (activeApp() != nullptr && activeApp()->snapshot() == homeAppSnapshot()) {
    Home::App::app()->snapshot()->selectCellAtLocation(0, 0);
    Home::App::app()->reloadAppCells();
    window()->redraw(true);
  }
}

/* Opens the Salty-OS settings, or goes back to the home screen if it is
 * already displayed. Not available during the onboarding, the hardware tests
 * or in exam mode. */
void AppsContainer::toggleSettingsApp() {
  if (activeApp() == nullptr || ExamModeManager::ExamMode().isActive()) {
    return;
  }
  Escher::App::Snapshot* current = activeApp()->snapshot();
  if (current == onBoardingAppSnapshot() ||
      current == hardwareTestAppSnapshot()) {
    return;
  }
  switchToBuiltinApp(current == saltySettingsAppSnapshot()
                         ? homeAppSnapshot()
                         : saltySettingsAppSnapshot());
}
