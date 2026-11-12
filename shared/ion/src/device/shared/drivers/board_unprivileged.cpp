#include <config/board.h>
#include <ion/authentication.h>
#include <shared/drivers/board_shared.h>
#include <shared/drivers/kernel_header.h>
#include <shared/drivers/salty_kernel_header.h>
#include <shared/drivers/userland_header.h>

namespace Ion {
namespace Device {
namespace Board {

KernelHeader* kernelHeader() {
  uint32_t slotOffset =
      isRunningSlotA() ? Config::SlotAOffset : Config::SlotBOffset;
  return reinterpret_cast<KernelHeader*>(
      Config::KernelVirtualOrigin + slotOffset + Config::SignedPayloadLength);
}

bool isSaltyKernel() {
  /* The salty kernel puts an 8-byte header (SLTY magic twice) at a fixed
   * offset of its slot. On the official kernel these bytes are code or
   * vector table, so both words matching is a reliable signature. It is a
   * plain read of the kernel flash, like kernelHeader() above. */
  uint32_t slotOffset =
      isRunningSlotA() ? Config::SlotAOffset : Config::SlotBOffset;
  const volatile uint32_t* header = reinterpret_cast<const volatile uint32_t*>(
      Config::KernelVirtualOrigin + slotOffset + SaltyKernelHeader::Offset);
  return header[0] == SaltyKernelHeader::Magic &&
         header[1] == SaltyKernelHeader::Magic;
}

UserlandHeader* userlandHeader() {
  return userlandHeader(isRunningSlotA() ? Slot::A : Slot::B);
}

UserlandHeader* userlandHeader(Slot slot) {
  uint32_t slotOffset =
      slot == Slot::A ? Config::SlotAOffset : Config::SlotBOffset;
  return reinterpret_cast<UserlandHeader*>(Config::UserlandVirtualOrigin +
                                           slotOffset);
}

uint32_t userlandStart() {
  return userlandStart(isRunningSlotA() ? Slot::A : Slot::B);
}

uint32_t userlandStart(Slot slot) {
  return reinterpret_cast<uint32_t>(userlandHeader(slot)) +
         sizeof(UserlandHeader);
}

uint32_t userlandEnd(Slot slot) {
  return reinterpret_cast<uint32_t>(userlandHeader(slot)) +
         Config::UserlandLength;
}

uint32_t securityLevel() {
  if (Authentication::clearanceLevel() ==
      Authentication::ClearanceLevel::ThirdParty) {
    return 0;
  }
  return *(reinterpret_cast<uint32_t*>(isRunningSlotA() ? Config::SlotAOrigin
                                                        : Config::SlotBOrigin));
}

}  // namespace Board
}  // namespace Device
}  // namespace Ion
