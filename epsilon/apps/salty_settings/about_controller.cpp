#include "about_controller.h"

#include <escher/theme/salty_palette.h>
#include <ion.h>
#include <ion/device_name.h>
#include <ion/external_apps.h>
#include <string.h>

#include "../apps_container.h"
#include "../shortcuts.h"
#include "theme_helpers.h"

#if PLATFORM_DEVICE
// Defined by the linker script, see userland_shared.ld
extern char _external_apps_RAM_start;
extern char _external_apps_RAM_end;
#endif

namespace SaltySettings {

using namespace Ion::Device::Board::Config;

/* Shortcut key names come from the macros of shortcuts.h, so they follow any
 * compile-time remapping: "Ion::Keyboard::Key::Alpha" -> "Alpha". */
#define SALTY_STRINGIFY_(x) #x
#define SALTY_STRINGIFY(x) SALTY_STRINGIFY_(x)

static const char* ShortKeyName(const char* qualifiedName) {
  const char* last = qualifiedName;
  for (const char* c = qualifiedName; *c != 0; c++) {
    if (*c == ':') {
      last = c + 1;
    }
  }
  return last;
}

const char* AboutController::title() const {
  return I18n::translate(I18n::Message::SaltySettingsAbout);
}

void AboutController::describe(RowPicker* p) const {
  using M = I18n::Message;

  p->section(M::SaltySettingsSectionOs);
  p->row(M::SaltySettingsOsVersion).str(SALTY_INFO_OS_VERSION);
  p->row(M::SaltySettingsEpsilonSource).str(SALTY_INFO_EPSILON_SOURCE_VERSION);
  p->row(M::SaltySettingsKernelTarget).str(SALTY_INFO_KERNEL_VERSION);
  p->row(M::SaltySettingsCommit).str(SALTY_INFO_COMMIT);
  p->row(M::SaltySettingsBuildDate).str(__DATE__);
  p->row(M::SaltySettingsPlatform).str(SALTY_INFO_PLATFORM);

  p->section(M::SaltySettingsSectionDevice);
  const char* deviceName = Ion::DeviceName::read();
  if (deviceName != nullptr && strlen(deviceName) > 0) {
    p->row(M::SaltySettingsDeviceName).str(deviceName);
  }
  p->row(M::SaltySettingsSerial).str(Ion::serialNumber());
  p->row(M::SaltySettingsKernelRunning).str(Ion::softwareVersion());
  p->row(M::SaltySettingsKernelCommit).str(Ion::commitHash());
  p->row(M::SaltySettingsPcb).str(Ion::pcbVersion());
  p->row(M::SaltySettingsBootloader).str(Ion::runningBootloader());

  p->section(M::SaltySettingsSectionThemeArea);
  const bool valid = ThemeManager::isValid();
  p->row(M::SaltySettingsStatus)
      .message(valid ? M::SaltySettingsValid : M::SaltySettingsInvalid);
  p->row(M::SaltySettingsAddress).str("0x").hex(ThemeAreaStart, 8);
  p->row(M::SaltySettingsCapacity).size(ThemeAreaSize);
  if (valid) {
    const uint32_t format =
        reinterpret_cast<const uint32_t*>(ThemeAreaStart)[1];
    Line formatLine = p->row(M::SaltySettingsFormat);
    formatLine.str("v").dec(format);
    if (format != ThemeAreaHeaderVersion) {
      formatLine.str(" (v").dec(ThemeAreaHeaderVersion).str(")");
    }
    p->row(M::SaltySettingsInstalledThemes)
        .dec(ThemeManager::themeCount() - 1)
        .str(" / ")
        .dec(ThemeCount);
    char name[k_maxThemeNameSize];
    ThemeName(ThemeManager::selectedTheme(), name);
    p->row(M::SaltySettingsActiveTheme).str(name);
    p->row(M::SaltySettingsSlots)
        .dec(Escher::SaltyPalette::numberOfSlots())
        .str(" + ")
        .dec(ThemeIconCount);
  }

  p->section(M::SaltySettingsSectionMemory);
#if PLATFORM_DEVICE
  p->row(M::SaltySettingsExternalRam)
      .size(static_cast<uint32_t>(&_external_apps_RAM_end -
                                  &_external_apps_RAM_start));
#endif
  Line externalApps = p->row(M::SaltySettingsExternalApps);
  externalApps.dec(Ion::ExternalApps::numberOfApps(false))
      .str(" (")
      .message(AppsContainer::sharedAppsContainer()->externalAppsHidden()
                   ? M::SaltySettingsHidden
                   : M::SaltySettingsShown)
      .str(")");

  p->section(M::SaltySettingsSectionShortcuts);
  p->row(M::SaltySettingsShortcutHold)
      .str(ShortKeyName(SALTY_STRINGIFY(SALTY_SHORTCUT_PREFIX_KEY_1)))
      .str(" + ")
      .str(ShortKeyName(SALTY_STRINGIFY(SALTY_SHORTCUT_PREFIX_KEY_2)));
  p->row(M::SaltySettingsShortcutPreviousTheme)
      .str(ShortKeyName(SALTY_STRINGIFY(SALTY_SHORTCUT_KEY_PREVIOUS_THEME)));
  p->row(M::SaltySettingsShortcutNextTheme)
      .str(ShortKeyName(SALTY_STRINGIFY(SALTY_SHORTCUT_KEY_NEXT_THEME)));
  p->row(M::SaltySettingsShortcutToggleExternalApps)
      .str(ShortKeyName(
          SALTY_STRINGIFY(SALTY_SHORTCUT_KEY_TOGGLE_EXTERNAL_APPS)));
  p->row(M::SaltySettingsShortcutOpenSettings)
      .str(ShortKeyName(SALTY_STRINGIFY(SALTY_SHORTCUT_KEY_OPEN_SETTINGS)));
}

}  // namespace SaltySettings
