SOURCES_apps += $(addprefix $(PATH_apps)/salty_settings/, \
  about_controller.cpp \
  app.cpp \
  info_list_controller.cpp \
  main_controller.cpp \
  section_cell.cpp \
  theme_detail_controller.cpp \
  themes_controller.cpp \
)

$(call i18n_with_universal,salty_settings/i18n/base)

# salty_info.h holds everything that comes from the repository (versions from
# the Makefile, git commit, slot and icon names from the themes folder).
$(OUTPUT_DIRECTORY)/$(PATH_apps)/salty_info.h: $(PATH_apps)/salty_settings/generate_info.sh $(SALTY_THEMES_DIRECTORY)/epsilon/palette.txt $(SALTY_THEMES_DIRECTORY)/generate_theme.py FORCE | $$(@D)/.
	$(call rule_label,SALTYINFO)
	sh $< $@ "$(SALTY_OS_VERSION)" "$(APP_VERSION)" "$(SALTY_EPSILON_SOURCE_VERSION)" "$(COMMIT_HASH)" "$(PLATFORM)" $(SALTY_THEMES_DIRECTORY)/epsilon/palette.txt $(SALTY_THEMES_DIRECTORY)/generate_theme.py
