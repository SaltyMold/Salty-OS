#pragma once

#include <escher/view_controller.h>

#include "on_boarding_view.h"

namespace OnBoarding {

/* Shows the Salty-OS logo, then (see showTutorial()) the tutorial. */
class SplashController : public Escher::ViewController {
 public:
  SplashController(Escher::Responder* parentResponder);
  Escher::View* view() override { return &m_view; }
  bool handleEvent(Ion::Events::Event event) override;

  // Replaces the logo by the tutorial, and shows the title bar
  void showTutorial();

 private:
  /* While a key of the tutorial page is held, redraws the page with its image
   * lowered and waits for the key to be released. */
  void followPressedKeys();
  // Goes to the startup prompt if there is one, to the Home app otherwise
  void leave();

  OnBoardingView m_view;
};

}  // namespace OnBoarding
