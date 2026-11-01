#ifndef ION_DEVICE_KERNEL_DRIVERS_FLASH_POLICY_H
#define ION_DEVICE_KERNEL_DRIVERS_FLASH_POLICY_H

/* Which external flash sectors the userland may erase/write (through the SVC).
 * Pure logic, without hardware access, so that it can be unit tested on a host.
 *
 *  Always writable:
 *   - the persisting bytes sector of the running slot (exam mode state),
 *   - the external apps area of the running slot.
 *  Writable only when NO exam mode is active:
 *   - the whole other slot (DFU update of the other slot) and its persisting
 *     bytes sector.
 *  Never writable: the kernel and the userland of the running slot, anything
 *  that is not an external flash sector.
 *
 * All the sectors are indices (see Flash::SectorAtAddress). */
namespace Ion {
namespace Device {
namespace FlashPolicy {

struct Layout {
  int numberOfSectors;
  int runningPersistingBytesSector;
  int otherPersistingBytesSector;
  int externalAppsFirstSector;  // inclusive. First > Last means "no external apps"
  int externalAppsLastSector;   // inclusive
  int otherSlotFirstSector;     // inclusive
  int otherSlotLastSector;      // inclusive
  bool examModeActive;
};

inline bool isSectorWritable(const Layout & l, int i) {
  if (i < 0 || i >= l.numberOfSectors) {
    return false;
  }
  if (i == l.runningPersistingBytesSector) {
    return true;
  }
  if (l.examModeActive && i == l.otherPersistingBytesSector) {
    /* Never let the exam mode of the other slot be cleared, even if the
     * external apps area (taken from the userland header) covers its sector. */
    return false;
  }
  if (i >= l.externalAppsFirstSector && i <= l.externalAppsLastSector) {
    return true;
  }
  if (l.examModeActive) {
    return false;
  }
  if (i == l.otherPersistingBytesSector) {
    return true;
  }
  return i >= l.otherSlotFirstSector && i <= l.otherSlotLastSector;
}

/* The external apps range comes from the userland header (flash). It is
 * clamped so that a corrupted header can never open the kernel region of the
 * running slot (kernel image + its persisting bytes). firstAllowed/lastAllowed
 * are the sectors between the end of that region and the end of the flash. */
inline void clampExternalApps(Layout & l, int firstAllowed, int lastAllowed) {
  if (l.externalAppsFirstSector < firstAllowed) {
    l.externalAppsFirstSector = firstAllowed;
  }
  if (l.externalAppsLastSector > lastAllowed) {
    l.externalAppsLastSector = lastAllowed;
  }
}

/* The source of a flash write must not live in the external flash sector being
 * written, nor anywhere the userland could not read: it has to be in the
 * userland RAM or in the userland flash of the running slot. */
inline bool sourceIsValid(unsigned int src, unsigned int length,
                          unsigned int ramStart, unsigned int ramLength,
                          unsigned int userlandFlashStart, unsigned int userlandFlashEnd) {
  if (length == 0) {
    return false;
  }
  if (src + length < src) {
    return false;  // overflow
  }
  if (src >= ramStart && src + length <= ramStart + ramLength) {
    return true;
  }
  return src >= userlandFlashStart && src + length <= userlandFlashEnd;
}

}
}
}

#endif
