# Dreamer Help

Option-click any jack, knob, switch or module title in VCV Rack, and a note says what it does.

Drop the **Help** module anywhere in the patch and the gesture works on every module in the rack, not only on this one. The module has no inputs and no outputs; it does nothing to the sound. Its one switch turns the gesture off again, because Option-click is not ours — other plugins use it, and Rack uses Option-drag to pan.

The note says what a control does, what a jack expects, and what a module needs before it will do anything at all. That last one is the most useful thing in here: a module that comes up silent because a level defaults to zero, a jack that is dead until a file is loaded, an expander that must sit on a particular side.

## What is in the database

One entry per module: a line for each control, the module's own description, notes for anything true of the module rather than of one control, and its context menu items. For jacks it also records what the port expects — whether it takes a polyphonic cable, what voltage moves it over its full travel, whether it is read as a level or as an edge, what a negative voltage does, and what an unpatched jack reads.

Every one of those facts carries a citation: a file and line in the maker's published source, or the page of their manual that states it. **A fact with no citation does not belong in this database.** That rule is the only thing that makes the data reviewable by anyone other than its author.

Where a plugin publishes no source, voltage ranges are not given, and the plugin's file says so. Those are the entries most worth a contribution, and the maker is the person best placed to make it.

See [ABOUT.md](ABOUT.md) for what the project is, how the database was made, and how it is proposed to be maintained.

## Contributing

The database is in `data/plugins/`, one YAML file per VCV plugin. Fix an entry, add a missing one, or fill in a range nobody could verify, and open a pull request. See [CONTRIBUTING.md](CONTRIBUTING.md) for the format and the one rule about quoting that will otherwise bite you.

Checks run automatically on every pull request: the file has to parse, match the schema, and carry a citation for every fact. Those failures come back to you within a minute, so nothing waits on a maintainer reading code.

## Building

```
make            build the plugin
make install    build and install into Rack's user plugins folder
make validate   check the database
make helptext   regenerate the generated table by hand
```

`src/HelpText.cpp` is generated from the database and is committed, so a build needs nothing but the Rack SDK. Editing a YAML file makes it stale, and `make` regenerates it automatically — the dependency is declared in the Makefile.

## Licence

GPL-3.0-or-later, the same as Rack.

The database describes other people's modules. Each entry cites the source it was read from, and those sources carry their makers' own licences; nothing of theirs is reproduced here beyond the facts a citation points at.
