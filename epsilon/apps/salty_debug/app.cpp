#include "app.h"

#include <apps/apps_container.h>
#include <apps/i18n.h>
#include <assert.h>

namespace SaltyDebug {

I18n::Message App::Descriptor::name() const { return upperName(); }

I18n::Message App::Descriptor::upperName() const {
  return I18n::Message::SaltyDebugAppCapital;
}

App* App::Snapshot::unpack(Escher::Container* container) {
  return new (container->currentAppBuffer()) App(this);
}

constexpr static App::Descriptor sDescriptor;

const App::Descriptor* App::Snapshot::descriptor() const {
  return &sDescriptor;
}

App::App(Snapshot* snapshot) : Escher::App(snapshot, &m_debugController) {}

}  // namespace SaltyDebug
