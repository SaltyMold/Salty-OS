#include <kernel/drivers/led.h>
#include <drivers/config/led.h>
#include <drivers/config/clocks.h>

namespace Ion {
namespace Device {
namespace LED {

using namespace Regs;

static bool sLocked = false;

void setLock(bool locked) {
  sLocked = locked;
}

bool isLocked() {
  return sLocked;
}

void setColorSecure(KDColor c) {
  if (sLocked) {
    return;
  }
  setColor(c);
}

}
}
}
