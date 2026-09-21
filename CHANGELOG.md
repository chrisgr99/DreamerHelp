# Changelog

Newest first.

## 2.1.2 — 21 September 2026

### Changed
- **The brand is now Dreamer Development**, so this plugin and the Clarity plugin group under one maker in the library and in the module browser.
- The manifest carries a plugin description, an author email and address, and a link to these notes.
- A help note for Clarity's new Click to arm a knob button.

## 2.1.1 — 19 September 2026

### Changed
- The help for Clarity and Test Gear follows their 2.1.0: Tooltip readability, Knob clarity, the scope's slow time bases and resize handles, Mute, Oscillator, Constant voltage and the frequency meter.

## 2.1.0 — 19 September 2026

### Added
- Help is read from JSON files, one per module, instead of being compiled into the plugin. For every item, a maker's own file in their plugin's `help` folder comes first, then a user's file in the Rack user folder, then this plugin's database. Each note says when its text came from a maker's or a user's file.
- `tools/helpscan.py`, which writes a plugin's help files from `//?` comments placed after each control's configure call in its source.
- Export help files for: in the right-click menu, a starting file for every module of a plugin in the rack, listing every control with the current text beside it.
- Reload help files, in the right-click menu, for somebody editing a file.
- Send this help to Dreamer Help, on a module's note when a user's file is in use: the file goes on the clipboard and a GitHub issue opens ready for it.
- Ask for help with: the gesture can be changed from option-click in the right-click menu.
- Lights answer too, with the maker's own name and description for them where there is no help text.

### Fixed
- Controls on a panel inside a module's panel were never found. Venom's Envelope Factory puts each stage on one; reported by its maker.
- On Windows, a new reading started over the one still speaking. It now stops the old one first.

### Changed
- The plugin is about a thirtieth of its former size, since the text is no longer compiled into it.
- The database is JSON, two files per module: the help itself in `data/help`, in the same format makers use, which ships; and the research behind it in `data/research` — each jack's family, the facts about what it expects and a citation for each — which does not. The YAML is gone, and nothing needs PyYAML any more.

## 2.0.1

### Fixed
- Option-drag pans the rack again
- Panel lettering, which was missing entirely
- Long control and module names wrap to a second line instead of running off the edge
- A note taller than the window is clamped to it and scrolls, with a scroll bar and a chevron at the foot

### Added
- Speak help on click, as a lit button on the panel: click a line of an open note to hear it
- Speech uses the system voice. Windows and Linux are supported but untested; where no voice is available, nothing is said
- Note text size, 80 to 250 percent, in the right-click menu. Kept for you, not with the patch
- An entry for the Help module itself

### Changed
- Help on and off is a lit push button, on a four HP panel with a green border

## 2.0.0

The first release.

### Added
- **Help** — one module, no inputs and no outputs. Option-click any jack, knob, switch or module title anywhere in the rack and a note says what that control does. The switch turns the gesture off again, because Option-click is not ours: other plugins use it and Rack uses Option-drag to pan.
- A database of **4,189 modules across 376 plugins**: a line for each control, what a module needs before it will do anything, its context menu, and for each jack whether it takes a polyphonic cable, what voltage moves it over its full travel, whether it is read as a level or an edge, what a negative voltage does and what an unpatched jack reads.
- Every fact carries a citation, so a claim can be checked by someone who did not write it. Where a plugin publishes no source, voltage ranges are not given and the file says so.
- The help can read itself aloud on macOS, from the module's right-click menu. Rack publishes nothing to the accessibility API, so this is the only way the text can be heard.

### Known
- The database is a snapshot of one person's installed library. A plugin added since has no entry, and the help says so rather than guessing.
- Descriptions are keyed to port and parameter indices. A maker who inserts a jack in the middle of an enum shifts every index after it, and the entry will then describe the wrong control until it is updated.
