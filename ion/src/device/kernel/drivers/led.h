#ifndef ION_SRC_DEVICE_KERNEL_DRIVERS_LED_H
#define ION_SRC_DEVICE_KERNEL_DRIVERS_LED_H

#include <kandinsky/color.h>
#include <shared/drivers/led.h>

namespace Ion {
namespace Device {
namespace LED {

void setColorSecure(KDColor c);

/* While locked, the color cannot be changed (neither by the userland nor by
 * the plug/charge updates). The userland locks the LED when the exam mode is
 * active, to keep the exam mode color on. */
void setLock(bool locked);
bool isLocked();

}
}
}

#endif
