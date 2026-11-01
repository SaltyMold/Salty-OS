#ifndef ION_DEVICE_KERNEL_DRIVERS_BATTERY_LEVEL_H
#define ION_DEVICE_KERNEL_DRIVERS_BATTERY_LEVEL_H

#include <stdint.h>

/* Battery level computation (pure logic, unit tested on a host).
 * Thresholds and hysteresis come from the kernel 26.4.0: there are 6 levels and
 * the thresholds are higher while the battery is charging (the voltage is then
 * inflated by the charger).
 * Unless forced, the level can only go down while discharging and only go up
 * while charging, which avoids flickering of the battery icon. */
namespace Ion {
namespace Device {
namespace BatteryLevel {

constexpr int k_numberOfThresholds = 5;
constexpr float k_thresholdsOnBattery[k_numberOfThresholds] = {3.60f, 3.72f, 3.77f, 3.85f, 4.00f};
constexpr float k_thresholdsCharging[k_numberOfThresholds] = {3.64f, 3.74f, 3.79f, 3.87f, 4.02f};
constexpr uint8_t k_initialLevel = 5;  // FULL

inline uint8_t levelForVoltage(float voltage, bool charging) {
  const float * thresholds = charging ? k_thresholdsCharging : k_thresholdsOnBattery;
  for (int i = 0; i < k_numberOfThresholds; i++) {
    if (voltage < thresholds[i]) {
      return i;
    }
  }
  return k_numberOfThresholds;
}

inline uint8_t next(float voltage, bool charging, uint8_t previous, bool forceLevelUpdate) {
  uint8_t level = levelForVoltage(voltage, charging);
  if (!forceLevelUpdate) {
    if (!charging) {
      if (previous <= level) {
        return previous;
      }
    } else if (level <= previous) {
      return previous;
    }
  }
  return level;
}

}
}
}

#endif
