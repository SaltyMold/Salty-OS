#pragma once

namespace Ion {
namespace Device {
namespace USB {
namespace Config {

// You want to overwrite the kernel? Go for it!

constexpr static const char* InterfaceFlashStringDescriptorAuthenticatedSlotA =
    "@Flash/0x90000000/61*064Kg,64*064Kg";
constexpr static const char* InterfaceFlashStringDescriptorAuthenticatedSlotB =
    "@Flash/0x90000000/08*004Kg,01*032Kg,63*064Kg/0x90400000/61*064Kg";
constexpr static const char* InterfaceFlashStringDescriptorThirdPartySlotA =
    "@Flash/0x90000000/61*064Kg";
constexpr static const char* InterfaceFlashStringDescriptorThirdPartySlotB =
    "@Flash/0x90400000/61*064Kg";
constexpr static const char* InterfaceSRAMStringDescriptor =
    "@SRAM/0x24000000/01*252Ke";

}  // namespace Config
}  // namespace USB
}  // namespace Device
}  // namespace Ion
