#pragma once

#include <escher/timer.h>

namespace OnBoarding {

/* Leaves the splash screen once it has been displayed for about one second.
 *
 * The timer belongs to the App::Snapshot (and not to the App) because its
 * fire method switches to another app, which destroys the on boarding App. */
class SplashTimer : public Escher::Timer {
 public:
  SplashTimer() : Timer(k_numberOfTicks) {}
  void restart() {
    m_didFire = false;
    reset();
  }

 private:
  bool fire() override;

  constexpr static uint32_t k_duration = 1000;  // In milliseconds
  // Timers have a granularity of TickDuration: the splash lasts 0.9 to 1.2s
  constexpr static uint32_t k_numberOfTicks =
      (k_duration + Timer::TickDuration - 1) / Timer::TickDuration;

  bool m_didFire = false;
};

}  // namespace OnBoarding
