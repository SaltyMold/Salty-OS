#ifndef ION_DEVICE_KERNEL_DRIVERS_POWER_WAKE_POLICY_H
#define ION_DEVICE_KERNEL_DRIVERS_POWER_WAKE_POLICY_H

#include <stdint.h>

namespace Ion {
namespace Device {
namespace Power {

/* Decides whether a hardware wake-up event must turn the device on. Pure logic
 * (no hardware access) so that it can be unit tested on a host.
 *
 * The wake-up event can come from the power key, from the USB plug detection
 * or from the charging pin. The device must turn on when:
 *  - the power key is down, and is the only key down: the user is pressing it
 *    now. The press is the action, so releasing the key later does nothing.
 *  - the device was unplugged and is now plugged.
 * In every other case (power key already released, key release bounce, USB
 * unplugged, end of charge) the device must go back to sleep.
 *
 * The previous test was `(scan & ~OnOff) == 0`, which is also true when NO key
 * is down: any wake-up event that was not a key press (a bounce on the key
 * release, an unplug, the end of the charge) turned the device back on. */
inline bool shouldWakeUp(uint64_t keyboardState, uint64_t onOffKeyState, bool wasPlugged, bool isPluggedNow) {
  if (keyboardState == onOffKeyState) {
    return true;
  }
  return !wasPlugged && isPluggedNow;
}

}
}
}

#endif
