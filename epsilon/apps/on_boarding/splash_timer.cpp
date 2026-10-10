#include "splash_timer.h"

#include <apps/apps_container.h>

namespace OnBoarding {

bool SplashTimer::fire() {
  /* A timer fires periodically, but the splash screen only has to be left
   * once. This also prevents displaying the prompt several times. */
  if (m_didFire) {
    return false;
  }
  m_didFire = true;

  AppsContainer* appsContainer = AppsContainer::sharedAppsContainer();
  if (appsContainer->promptController()) {
    Escher::App::app()->displayModalViewController(
        appsContainer->promptController(), KDGlyph::k_alignCenter,
        KDGlyph::k_alignCenter);
  } else {
    appsContainer->switchToBuiltinApp(appsContainer->homeAppSnapshot());
  }
  return true;
}

}  // namespace OnBoarding
