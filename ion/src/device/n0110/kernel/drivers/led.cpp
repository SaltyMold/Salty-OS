#include <drivers/battery.h>
#include <drivers/led.h>
#include <ion/usb.h>
#include <kernel/drivers/led.h>

namespace Ion {
namespace Device {
namespace LED {

KDColor updateColorWithPlugAndCharge() {
  KDColor ledColor = getColor();
  if (isLocked()) {
    /* The userland locked the LED (exam mode): do not override its color with
     * the plugged/charging state. */
    return ledColor;
  }
  if (USB::isPlugged()) {
    ledColor = Battery::isCharging() ? KDColorOrange : KDColorGreen;
  } else {
    ledColor = KDColorBlack;
  }
  setColorSecure(ledColor);
  return ledColor;
}

}
}
}
