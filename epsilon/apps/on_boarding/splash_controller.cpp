#include "splash_controller.h"

#include <apps/apps_container.h>
#include <ion/keyboard.h>
#include <ion/timing.h>

#include "salty_os.h"

namespace OnBoarding {

SplashController::SplashController(Escher::Responder* parentResponder)
    : ViewController(parentResponder) {
  m_view.splashView()->setImage(ImageStore::SaltyOs);
}

void SplashController::showTutorial() {
  if (m_view.isShowingTutorial()) {
    return;
  }
  AppsContainer::sharedAppsContainer()->showTitleBar(
      I18n::Message::OnBoardingTutorialTitle);
  m_view.showTutorial();
}

void SplashController::leave() {
  AppsContainer* appsContainer = AppsContainer::sharedAppsContainer();
  if (appsContainer->promptController()) {
    Escher::App::app()->displayModalViewController(
        appsContainer->promptController(), KDGlyph::k_alignCenter,
        KDGlyph::k_alignCenter);
  } else {
    /* Switches to another app, which destroys the on boarding app: nothing
     * must be done after this. */
    appsContainer->switchToBuiltinApp(appsContainer->homeAppSnapshot());
  }
}

void SplashController::followPressedKeys() {
  /* There is no event when a key is released, and the run loop only wakes up
   * every few hundred milliseconds: the keys are polled here instead, so that
   * the image follows the key. The wait is bounded to never block the OS. */
  constexpr int k_stepDuration = 15;       // In milliseconds
  constexpr int k_maxDuration = 3000;      // In milliseconds
  TutorialView* tutorial = m_view.tutorialView();
  AppsContainer* appsContainer = AppsContainer::sharedAppsContainer();
  if (tutorial->setPressedKeys(Ion::Keyboard::scan())) {
    appsContainer->redrawWindow();
  }
  int elapsed = 0;
  while (tutorial->hasPressedKeys() && elapsed < k_maxDuration) {
    Ion::Timing::msleep(k_stepDuration);
    elapsed += k_stepDuration;
    if (tutorial->setPressedKeys(Ion::Keyboard::scan())) {
      appsContainer->redrawWindow();
    }
  }
  if (tutorial->setPressedKeys(Ion::Keyboard::State(0))) {
    appsContainer->redrawWindow();
  }
}

bool SplashController::handleEvent(Ion::Events::Event event) {
  if (!m_view.isShowingTutorial()) {
    return false;
  }
  TutorialView* tutorial = m_view.tutorialView();
  if (event.isKeyPress()) {
    followPressedKeys();
  }
  /* The tutorial cannot be skipped: Home and Back are swallowed, and only OK
   * ends it. */
  if (tutorial->isLastPage()) {
    if (event == Ion::Events::OK) {
      leave();
      return true;
    }
  } else if (event == Ion::Events::OK || event == Ion::Events::EXE ||
             event == Ion::Events::Right) {
    tutorial->setPage(tutorial->page() + 1);
    return true;
  }
  if (event == Ion::Events::Back || event == Ion::Events::Left) {
    if (!tutorial->isFirstPage()) {
      tutorial->setPage(tutorial->page() - 1);
    }
    return true;
  }
  if (event == Ion::Events::Home) {
    return true;
  }
  return false;
}

}  // namespace OnBoarding
