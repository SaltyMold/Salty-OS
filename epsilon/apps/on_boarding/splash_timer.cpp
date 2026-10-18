#include "splash_timer.h"

#include <apps/apps_container.h>

#include "app.h"

namespace OnBoarding {

bool SplashTimer::fire() {
  /* A timer fires periodically, but the splash screen only has to be left
   * once. */
  if (m_didFire) {
    return false;
  }
  m_didFire = true;
  // The logo is followed by the tutorial, which leaves the app when it is over
  static_cast<App*>(Escher::App::app())->showTutorial();
  return true;
}

}  // namespace OnBoarding
