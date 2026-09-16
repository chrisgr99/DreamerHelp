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

# THE GENERATED TABLE IS COMMITTED, AND THE BUILD MUST NOT REGENERATE IT.
#
# It was a make dependency on the YAML at first, so that editing the data could not leave a
# stale table behind. That is wrong for two reasons, and the second is the serious one.
#
# A fresh `git clone` gives every file the same timestamp, so make cannot tell which is newer
# and regenerates on the first build — which failed in the VCV plugin toolchain container,
# where there is no PyYAML.
#
# And it made building the plugin depend on Python at all. Anyone building from source needs
# only the Rack SDK, and that has to include whoever builds it for the library.
#
# Staleness is caught where it belongs instead: .github/workflows/check.yml regenerates the
# table and fails if that produces a diff, so a table that has fallen behind its data is a red
# cross on a pull request rather than yesterday's text shipping quietly.
.PHONY: helptext
helptext:
	@python3 tools/build.py

.PHONY: validate
validate:
	@python3 tools/validate.py

include $(RACK_DIR)/plugin.mk
