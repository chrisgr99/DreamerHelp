# Changelog

Newest first.

## 2.0.1

Still marked pre-release.

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

The first release. Marked pre-release: the plugin works and the database is complete, but nothing here has been through anyone else's hands yet.

### Added
- **Help** — one module, no inputs and no outputs. Option-click any jack, knob, switch or module title anywhere in the rack and a note says what that control does. The switch turns the gesture off again, because Option-click is not ours: other plugins use it and Rack uses Option-drag to pan.
- A database of **4,189 modules across 376 plugins**: a line for each control, what a module needs before it will do anything, its context menu, and for each jack whether it takes a polyphonic cable, what voltage moves it over its full travel, whether it is read as a level or an edge, what a negative voltage does and what an unpatched jack reads.
- Every fact carries a citation, so a claim can be checked by someone who did not write it. Where a plugin publishes no source, voltage ranges are not given and the file says so.
- The help can read itself aloud on macOS, from the module's right-click menu. Rack publishes nothing to the accessibility API, so this is the only way the text can be heard.

### Known
- The database is a snapshot of one person's installed library. A plugin added since has no entry, and the help says so rather than guessing.
- Descriptions are keyed to port and parameter indices. A maker who inserts a jack in the middle of an enum shifts every index after it, and the entry will then describe the wrong control until it is updated.
