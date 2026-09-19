# Dreamer Help

Option-click any jack, knob, switch, light or module title in VCV Rack, and a note says what it does.

Drop the **Help** module anywhere in the patch and the gesture works on every module in the rack, not only on this one. The module has no inputs and no outputs; it does nothing to the sound. Its one switch turns the gesture off again, because Option-click is not ours — other plugins use it, and Rack uses Option-drag to pan. Another combination can be chosen in its right-click menu.

The note says what a control does, what a jack expects, and what a module needs before it will do anything at all. That last one is the most useful thing in here: a module that comes up silent because a level defaults to zero, a jack that is dead until a file is loaded, an expander that must sit on a particular side.

## What is in the database

One entry per module: a line for each control, the module's own description, notes for anything true of the module rather than of one control, and its context menu items. For jacks it also records what the port expects — whether it takes a polyphonic cable, what voltage moves it over its full travel, whether it is read as a level or as an edge, what a negative voltage does, and what an unpatched jack reads.

Every one of those facts carries a citation: a file and line in the maker's published source, or the page of their manual that states it. **A fact with no citation does not belong in this database.** That rule is the only thing that makes the data reviewable by anyone other than its author.

Where a plugin publishes no source, voltage ranges are not given, and the plugin's file says so. Those are the entries most worth a contribution, and the maker is the person best placed to make it.

See [ABOUT.md](ABOUT.md) for what the project is, how the database was made, and how it is proposed to be maintained.

## Help files

Help is read from JSON files, one per module. A maker can ship their own in their plugin, and a user can write one for a module whose maker has not; either comes before this plugin's database. [docs/help-files.md](docs/help-files.md) has the format, where the files go, and `tools/helpscan.py`, which writes them from `//?` comments in a plugin's source.

## Contributing to the database

Each module has two files, in folders named after the plugin's slug — the same name as its folder under `Rack2/plugins-<platform>/`: its help in `data/help/`, which ships, and the research behind it in `data/research/`, which does not. Fix an entry, add a missing one, or fill in a range nobody could verify, and open a pull request.

[CONTRIBUTING.md](CONTRIBUTING.md) has the whole file format with an annotated example, how to find the file for a module you are looking at, what counts as a citation — including a test you ran yourself — and the one rule about quoting that will otherwise bite you.

Checks run automatically on every pull request: the file has to parse, match the schema, and carry a citation for every fact. Those failures come back to you within a minute, so nothing waits on a maintainer reading code.

## Building

```
make            build the plugin
make install    build and install into Rack's user plugins folder
make validate   check the database
make helptext   bring each help file's expects up to date with its research
```

A build needs nothing but the Rack SDK. `make helptext` brings the one derived part of each help file — what each jack expects — up to date with its research, and a pull request where the two disagree fails its checks.

## Licence

GPL-3.0-or-later, the same as Rack.

The database describes other people's modules. Each entry cites the source it was read from, and those sources carry their makers' own licences; nothing of theirs is reproduced here beyond the facts a citation points at.
