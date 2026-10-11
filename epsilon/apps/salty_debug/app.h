#pragma once

#include <escher/app.h>

#include "debug_controller.h"

namespace SaltyDebug {

/* Salty-OS debug screen: shows everything the userland knows about itself and
 * about every theme of the Theme Area. It is not listed on the home screen: it
 * is opened with a shortcut (see apps/shortcuts.h). */
class App : public Escher::App {
 public:
  class Descriptor : public Escher::App::Descriptor {
   public:
    I18n::Message name() const override;
    I18n::Message upperName() const override;
  };
  class Snapshot : public Escher::App::Snapshot {
   public:
    App* unpack(Escher::Container* container) override;
    const Descriptor* descriptor() const override;
  };

 private:
  App(Snapshot* snapshot);
  DebugController m_debugController;
};

}  // namespace SaltyDebug
