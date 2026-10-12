#include "splash_controller.h"

#include "salty_os.h"

namespace OnBoarding {

SplashController::SplashController(Escher::Responder* parentResponder)
    : ViewController(parentResponder) {
  m_view.setImage(ImageStore::SaltyOs);
}

}  // namespace OnBoarding
