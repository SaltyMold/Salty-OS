SOURCES_apps += $(addprefix $(PATH_apps)/on_boarding/,$(addsuffix :-test, \
  app.cpp \
  splash_controller.cpp \
  splash_timer.cpp \
  splash_view.cpp \
  startup_prompt_controller.cpp \
  tutorial_view.cpp \
))

# salty_os.png: plain (non themed) image inlined by the escher inliner
$(call all_objects_for,$(PATH_apps)/on_boarding/splash_controller.cpp): $(OUTPUT_DIRECTORY)/$(PATH_apps)/on_boarding/salty_os.cpp
$(call all_objects_for,$(PATH_apps)/on_boarding/splash_controller.cpp): SFLAGS += -I$(OUTPUT_DIRECTORY)/$(PATH_apps)/on_boarding
$(OUTPUT_DIRECTORY)/$(PATH_apps)/on_boarding/salty_os.h: $(OUTPUT_DIRECTORY)/$(PATH_apps)/on_boarding/salty_os.cpp ;
SOURCES_apps += $(call generated_sources_for,$(PATH_apps)/on_boarding/salty_os.cpp:-test)

# Images of the keys shown by the tutorial (keys/*.png), inlined like salty_os.png
_on_boarding_keys := left_parenthesis right_parenthesis shift alpha xnt backspace
_on_boarding_keys_cpp := $(foreach k,$(_on_boarding_keys),$(OUTPUT_DIRECTORY)/$(PATH_apps)/on_boarding/keys/$(k).cpp)
$(call all_objects_for,$(PATH_apps)/on_boarding/tutorial_view.cpp): $(_on_boarding_keys_cpp)
$(call all_objects_for,$(PATH_apps)/on_boarding/tutorial_view.cpp): SFLAGS += -I$(OUTPUT_DIRECTORY)/$(PATH_apps)/on_boarding
$(foreach k,$(_on_boarding_keys),$(OUTPUT_DIRECTORY)/$(PATH_apps)/on_boarding/keys/$(k).h): $(OUTPUT_DIRECTORY)/$(PATH_apps)/on_boarding/keys/%.h: $(OUTPUT_DIRECTORY)/$(PATH_apps)/on_boarding/keys/%.cpp ;
SOURCES_apps += $(foreach k,$(_on_boarding_keys),$(call generated_sources_for,$(PATH_apps)/on_boarding/keys/$(k).cpp:-test))

$(call i18n_without_universal,on_boarding/i18n/base)
