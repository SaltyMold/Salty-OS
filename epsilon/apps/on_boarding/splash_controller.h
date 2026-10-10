#pragma once

#include <escher/view_controller.h>

#include "splash_view.h"

namespace OnBoarding {

class SplashController : public Escher::ViewController {
 public:
  SplashController(Escher::Responder* parentResponder);
  Escher::View* view() override { return &m_view; }
  bool handleEvent(Ion::Events::Event event) override { return false; }

 private:
  SplashView m_view;
};

}  // namespace OnBoarding
