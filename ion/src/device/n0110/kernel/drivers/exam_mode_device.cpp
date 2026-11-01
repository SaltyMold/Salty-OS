#include <kernel/drivers/exam_mode.h>
#include <drivers/config/board.h>
#include <drivers/board_unprivileged.h>

namespace Ion {
namespace Device {
namespace ExamMode {

bool isActive() {
  uint32_t pb = Board::isRunningSlotA() ? Board::Config::PersistingBytesAStartAddress : Board::Config::PersistingBytesBStartAddress;
  const uint8_t * examStart = reinterpret_cast<const uint8_t *>(pb + Board::Config::DeviceNameSectorSize);
  const uint8_t * examEnd = reinterpret_cast<const uint8_t *>(pb + Board::Config::PersistingBytesSectorSize);
  return rawIsActive(currentRawValue(examStart, examEnd));
}

}
}
}
