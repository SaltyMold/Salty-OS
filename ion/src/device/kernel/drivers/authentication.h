#ifndef ION_DEVICE_KERNEL_DRIVERS_AUTHENTICATION_H
#define ION_DEVICE_KERNEL_DRIVERS_AUTHENTICATION_H

#include <ion/authentication.h>
#include <stdint.h>

namespace Ion {
namespace Device {
namespace Authentication {

/* Custom kernel: there is no signature verification. The userland is always
 * considered official, which is required for the exam mode to be effective
 * (ExamMode::get() ignores the persisting bytes of a non-NumWorks userland). */
Ion::Authentication::ClearanceLevel clearanceLevel();

}
}
}

#endif
