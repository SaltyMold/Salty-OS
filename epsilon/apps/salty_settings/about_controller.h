#pragma once

#include "info_list_controller.h"

namespace SaltySettings {

/* Everything the userland knows about itself: versions, build, device, Theme
 * Area, memory and shortcuts. */
class AboutController : public InfoListController {
 public:
  AboutController(Escher::Responder* parentResponder, InfoRow* rows)
      : InfoListController(parentResponder, rows) {}
  const char* title() const override;

 private:
  void describe(RowPicker* picker) const override;
};

}  // namespace SaltySettings
