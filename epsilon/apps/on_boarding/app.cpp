#include "app.h"

#include <apps/apps_container.h>
#include <apps/exam_mode_manager.h>
#include <assert.h>

using namespace Escher;

namespace OnBoarding {

App* App::Snapshot::unpack(Container* container) {
  return new (container->currentAppBuffer()) App(this);
}

constexpr static App::Descriptor sDescriptor;

const App::Descriptor* App::Snapshot::descriptor() const {
  return &sDescriptor;
}

App::App(Snapshot* snapshot)
    : ::App(snapshot, &m_splashController),
      m_splashController(&m_modalViewController) {}

Timer* App::timerAtIndex(int i) {
  assert(i == 0);
  return snapshot()->splashTimer();
}

void App::willBecomeInactive() {
  Ion::Power::selectStandbyMode(false);
  Ion::Events::setSpinner(true);
  ::App::willBecomeInactive();
}

void App::didBecomeActive(Window* window) {
  ::App::didBecomeActive(window);
  // Disable spinner
  Ion::Events::setSpinner(false);
  if (ExamModeManager::ExamMode().color() == KDColorBlack) {
    // Forbid standby in exam mode with led since it disables the led
    Ion::Power::selectStandbyMode(true);
  }
  // The splash screen is displayed for about one second
  snapshot()->splashTimer()->restart();
}

}  // namespace OnBoarding
