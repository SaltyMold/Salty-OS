#pragma once

#include <apps/shared/shared_app.h>

#include "main_controller.h"

namespace SaltySettings {

/* Salty-OS settings: the information about the userland ("About") and about
 * every theme of the Theme Area ("Themes"), laid out like the Settings app.
 * It is not listed on the home screen: it is opened with a shortcut (see
 * apps/shortcuts.h). */
class App : public Shared::SharedApp {
 public:
  class Descriptor : public Escher::App::Descriptor {
   public:
    I18n::Message name() const override;
    I18n::Message upperName() const override;
  };
  class Snapshot : public Shared::SharedApp::Snapshot {
   public:
    App* unpack(Escher::Container* container) override;
    const Descriptor* descriptor() const override;
  };

 private:
  App(Snapshot* snapshot);
  MainController m_mainController;
  Escher::StackViewController::Default m_stackViewController;
};

}  // namespace SaltySettings
