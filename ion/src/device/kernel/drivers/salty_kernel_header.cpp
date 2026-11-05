#include <shared/drivers/salty_kernel_header.h>

namespace Ion {
namespace Device {

constexpr SaltyKernelHeader __attribute__((section(".salty_kernel_header"), used)) k_saltyKernelHeader;

}
}
