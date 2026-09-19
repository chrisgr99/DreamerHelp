# Dreamer Help — see README.md
#
#   make                 build the plugin
#   make install         build and copy into Rack's user plugins folder
#   make helptext        bring each help file's "expects" up to date with data/research
#   make validate        check the database against the schema and the style rules
#   make dist            build a .vcvplugin package for distribution

RACK_DIR ?= ../Rack-SDK

SOURCES += $(wildcard src/*.cpp)

DISTRIBUTABLES += res
DISTRIBUTABLES += $(wildcard LICENSE*)

# THE HELP SHIPS AS FILES, read one module at a time — see design/help-database.md. data/help is
# the help itself and ships; data/research is what it was worked out from and does not. Building
# the plugin needs nothing but the Rack SDK. The one derived part of a help file, what each jack
# expects, is refreshed from the research by `make helptext`, and .github/workflows/check.yml
# fails a pull request whose files have fallen behind.
DISTRIBUTABLES += data/help

.PHONY: helptext
helptext:
	@python3 tools/build.py

.PHONY: validate
validate:
	@python3 tools/validate.py

include $(RACK_DIR)/plugin.mk
