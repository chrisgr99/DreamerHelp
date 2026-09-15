# Dreamer Help — see README.md
#
#   make                 build the plugin
#   make install         build and copy into Rack's user plugins folder
#   make helptext        regenerate src/HelpText.cpp from data/plugins/*.yaml
#   make validate        check the database against the schema and the style rules
#   make dist            build a .vcvplugin package for distribution

RACK_DIR ?= ../Rack-SDK

SOURCES += $(wildcard src/*.cpp)

DISTRIBUTABLES += res
DISTRIBUTABLES += $(wildcard LICENSE*)

# THE GENERATED TABLE IS A BUILD PRODUCT OF THE DATABASE, and make should know it. Anything
# that edits a YAML file makes the table stale, and a stale table is invisible — the plugin
# builds and runs and simply shows yesterday's text. Declaring the dependency means a build
# after an edit regenerates without anyone remembering to.
HELP_YAML := $(wildcard data/plugins/*.yaml)

src/HelpText.cpp: $(HELP_YAML) tools/build.py
	@echo "regenerating src/HelpText.cpp from $(words $(HELP_YAML)) plugin files"
	@python3 tools/build.py

.PHONY: helptext
helptext:
	@python3 tools/build.py

.PHONY: validate
validate:
	@python3 tools/validate.py

include $(RACK_DIR)/plugin.mk
