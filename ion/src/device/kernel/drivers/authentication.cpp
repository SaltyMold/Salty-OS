#include <kernel/drivers/authentication.h>

namespace Ion {
namespace Device {
namespace Authentication {

Ion::Authentication::ClearanceLevel clearanceLevel() {
  return Ion::Authentication::ClearanceLevel::NumWorks;
}

}
}
}
