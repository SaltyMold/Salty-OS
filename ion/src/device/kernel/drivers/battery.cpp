#include <drivers/battery.h>
#include <drivers/config/battery.h>
#include <kernel/drivers/battery_level.h>

namespace Ion {
namespace Device {
namespace Battery {

Ion::Battery::Charge level(bool forceLevelUpdate) {
  static uint8_t previousLevel = BatteryLevel::k_initialLevel;
  previousLevel = BatteryLevel::next(voltage(), isCharging(), previousLevel, forceLevelUpdate);
  return static_cast<Ion::Battery::Charge>(previousLevel);
}

}
}
}
