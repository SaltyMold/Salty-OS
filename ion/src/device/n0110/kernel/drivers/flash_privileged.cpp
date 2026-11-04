#include <kernel/drivers/board.h>
#include <kernel/drivers/exam_mode.h>
#include <kernel/drivers/flash_policy.h>
#include <shared/drivers/cache.h>
#include <shared/drivers/config/board.h>
#include <shared/drivers/config/external_flash.h>
#include <shared/drivers/external_flash_write.h>
#include <shared/drivers/flash_privileged.h>
#include <shared/drivers/flash_write_with_interruptions.h>

namespace Ion {
namespace Device {
namespace Flash {

using Board::Config::PersistingBytesAStartAddress;
using Board::Config::PersistingBytesBStartAddress;

/* Build the policy layout for the running slot. Computed once per SVC (the
 * exam mode state is read from the flash, which is slow to do per sector). */
static FlashPolicy::Layout currentLayout() {
  const bool slotA = Board::isRunningSlotA();
  const uint32_t slotStart = slotA ? Board::Config::SlotAStartAddress : Board::Config::SlotBStartAddress;
  const uint32_t otherSlotStart = slotA ? Board::Config::SlotBStartAddress : Board::Config::SlotAStartAddress;
  const uint32_t slotLength = ExternalFlash::Config::TotalSize / 2;
  const uint32_t runningPB = slotA ? PersistingBytesAStartAddress : PersistingBytesBStartAddress;
  const uint32_t otherPB = slotA ? PersistingBytesBStartAddress : PersistingBytesAStartAddress;

  FlashPolicy::Layout l;
  l.numberOfSectors = TotalNumberOfSectors();
  l.runningPersistingBytesSector = SectorAtAddress(runningPB);
  l.otherPersistingBytesSector = SectorAtAddress(otherPB);
  l.otherSlotFirstSector = SectorAtAddress(otherSlotStart);
  l.otherSlotLastSector = SectorAtAddress(otherSlotStart + slotLength - 1);
  l.examModeActive = ExamMode::isActive();

  // External apps: from the userland header, clamped to the allowed area.
  UserlandHeader * header = Board::userlandHeader();
  if (header->isValid()) {
    uint32_t appsStart = reinterpret_cast<uint32_t>(header->externalAppsAddressFlash());
    uint32_t appsSize = header->externalAppsSizeFlash();
    if (appsSize > 0) {
      l.externalAppsFirstSector = SectorAtAddress(appsStart);
      l.externalAppsLastSector = SectorAtAddress(appsStart + appsSize - 1);
    } else {
      l.externalAppsFirstSector = 1;
      l.externalAppsLastSector = 0;
    }
  } else {
    l.externalAppsFirstSector = 1;
    l.externalAppsLastSector = 0;
  }
  // Allowed area: after the kernel image, up to the end of the flash.
  int firstAllowed = SectorAtAddress(slotStart + Board::Config::KernelImageSize);
  int lastAllowed = TotalNumberOfSectors() - 1;
  FlashPolicy::clampExternalApps(l, firstAllowed, lastAllowed);
  return l;
}

bool ForbiddenSector(int i) {
  return !FlashPolicy::isSectorWritable(currentLayout(), i);
}

bool MassEraseEnable() {
  return false;
}

void MassEraseWithInterruptions(bool handleInterrupts) {
  if (handleInterrupts) {
    Board::shutdownInterruptions();
  }
  FlashPolicy::Layout l = currentLayout();
  for (int i = 0; i < TotalNumberOfSectors(); i++) {
    if (FlashPolicy::isSectorWritable(l, i)) {
      ExternalFlash::EraseSector(i);
    }
  }
  Cache::invalidateDCache();
  if (handleInterrupts) {
    Board::initInterruptions();
  }
}

bool EraseSectorWithInterruptions(int i, bool handleInterrupts) {
  if (handleInterrupts) {
    Board::shutdownInterruptions();
  }
  bool allowed = FlashPolicy::isSectorWritable(currentLayout(), i);
  if (allowed) {
    ExternalFlash::EraseSector(i);
  }
  Cache::invalidateDCache();
  if (handleInterrupts) {
    Board::initInterruptions();
  }
  return allowed;
}

bool WriteMemoryWithInterruptions(uint8_t * destination, const uint8_t * source,
                                  size_t length, bool handleInterrupts) {
  const bool slotA = Board::isRunningSlotA();
  const uint32_t slotStart = slotA ? Board::Config::SlotAStartAddress : Board::Config::SlotBStartAddress;
  const uint32_t userlandStart = slotStart + Board::Config::UserlandOffset + sizeof(UserlandHeader);
  // The source must be readable by the userland and must not be in the flash
  // we are about to write.
  bool valid = FlashPolicy::sourceIsValid(
      reinterpret_cast<uint32_t>(source), length,
      Board::Config::UserlandSRAMAddress, Board::Config::UserlandSRAMLength,
      userlandStart, ExternalFlash::Config::EndAddress);
  if (!valid || !IncludesAddress(reinterpret_cast<uint32_t>(destination))) {
    return false;
  }
  uint32_t last = reinterpret_cast<uint32_t>(destination) + length - 1;
  if (last < reinterpret_cast<uint32_t>(destination) || !IncludesAddress(last)) {
    return false;
  }
  if (handleInterrupts) {
    Board::shutdownInterruptions();
  }
  FlashPolicy::Layout l = currentLayout();
  int firstSector = SectorAtAddress(reinterpret_cast<uint32_t>(destination));
  int lastSector = SectorAtAddress(last);
  bool allowed = firstSector >= 0 && lastSector >= 0;
  for (int i = firstSector; allowed && i <= lastSector; i++) {
    allowed = FlashPolicy::isSectorWritable(l, i);
  }
  if (allowed) {
    ExternalFlash::WriteMemory(destination, source, length);
  }
  Cache::invalidateDCache();
  if (handleInterrupts) {
    Board::initInterruptions();
  }
  return allowed;
}

}
}
}
