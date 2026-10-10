#pragma once

#include <escher/app.h>

#include "splash_controller.h"
#include "splash_timer.h"

namespace OnBoarding {

class App : public Escher::App {
 public:
  class Snapshot : public Escher::App::Snapshot {
   public:
    App* unpack(Escher::Container* container) override;
    const Descriptor* descriptor() const override;

    /* The timer lives in the Snapshot because leaving the splash screen
     * switches to another app, which destroys the on boarding App. */
    SplashTimer* splashTimer() { return &m_splashTimer; }

   private:
    SplashTimer m_splashTimer;
  };

  void willBecomeInactive() override;
  void didBecomeActive(Escher::Window* window) override;
  int numberOfTimers() override { return 1; }
  Escher::Timer* timerAtIndex(int i) override;

 private:
  App(Snapshot* snapshot);
  Snapshot* snapshot() {
    return static_cast<Snapshot*>(Escher::App::snapshot());
  }
  SplashController m_splashController;
};

}  // namespace OnBoarding
