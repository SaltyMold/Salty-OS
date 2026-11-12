#pragma once

#include <shared/drivers/kernel_header.h>
#include <shared/drivers/userland_header.h>
#include <stdint.h>

namespace Ion {
namespace Device {
namespace Board {

enum class Slot : bool {
  A,
  B,
};

KernelHeader* kernelHeader();
UserlandHeader* userlandHeader();
UserlandHeader* userlandHeader(Slot slot);
uint32_t userlandStart();
uint32_t userlandStart(Slot slot);
uint32_t userlandEnd(Slot slot);
uint32_t writableSRAMEndAddress();
uint32_t writableSRAMStartAddress();
bool isRunningSlotA();
/* True if the running kernel is the Salty kernel (SLTY header present). */
bool isSaltyKernel();
uint32_t securityLevel();

}  // namespace Board
}  // namespace Device
}  // namespace Ion
