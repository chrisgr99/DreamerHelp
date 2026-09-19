# About this project

## What it is

A help system for VCV Rack that answers questions about *other people's* modules.

Drop the Help module into a patch and option-click any jack, knob, switch or module title anywhere in the rack. A note appears saying what that control does. It works on every module in the patch, not only on this one, and the module has no inputs, no outputs and no effect on the sound.

Behind it is a database covering **4,189 modules across 376 plugins** — every module the author had installed when it was written.

## Why it exists

Rack has tooltips: the name a maker gives a control in `configParam` appears when you hover over it. That layer is useful and it is also thin. A name tells you a knob is called "Shape". It does not tell you that the module is silent until you raise a level that defaults to zero, that a jack replaces its knob rather than adding to it, or that an input is read as an edge rather than a level, so a slowly rising voltage will never trigger it.

Those are the things that make a module seem broken when it is working exactly as designed. They are usually in the manual, and manuals are read rarely and at the wrong moment. This puts the answer where the question is asked.

## What an entry contains

For each module:

- **A first line** saying what the module is.
- **A line for each control** — every knob, switch, button, jack and light that can be clicked.
- **`Note` lines** for anything true of the module rather than of one control. The most valuable of these say what a module needs before it will do anything at all: a file to load, a clock to patch, a device to choose, an expander to place on a particular side, a switch that comes up off.
- **`Menu` lines**, one per context menu item, named exactly as the menu prints them.

For each jack, where the code settles it:

- **whether it takes a polyphonic cable**;
- **what voltage moves it over its full travel** — not what it will physically accept, since nothing in Rack bounds a cable, but the span over which it does its work;
- **whether it is read as a continuous value or as a state** — a level or an edge;
- **what a negative voltage does** — ignored, subtracted from a knob until the total hits its floor, or carried through;
- **what an unpatched jack reads**, where something is normalled into it;
- **the clamp on the knob-plus-input total**, where there is one.

## The rule everything rests on

**Every fact carries a citation.** A file and a line in the maker's published source, the page of a manual that states it, or a description of the test that established it — say what was patched in and what came out. A fact that cannot be pointed at, or repeated, does not go in.

This is not bookkeeping. It is what makes the database reviewable by anyone other than whoever wrote it. A claim that a jack takes ±5V cannot be checked by a stranger; "line 212 clamps it to ±5V" can be checked in under a minute, by a person or by a machine. Without that rule a contribution can only be accepted on trust, and a database accepted on trust degrades quietly.

A citation must say what the code **does**, not merely where it lives. `Foo.cpp:212` alone is not a citation — it names a place, and nobody reading it can tell whether the writer read the line or guessed from the shape of the file around it.

**A blank is often the correct answer.** An audio input usually has no meaningful voltage range: you patch what you like and the module multiplies it. A jack that passes through whatever it is given has no family and no range. A control bounded only in seconds, or steps, or hertz, settles nothing about voltage. Fabricating a plausible figure is worse than leaving the field empty, because an empty field invites somebody to look and a figure does not.

## What is not covered, and why

**Plugins that publish no source.** For those, voltage ranges are not given, and the file says so in a `verification` notice at the top. About forty files carry it at present, and the number moves as coverage is contributed. Those entries are the ones most worth a contribution, and the maker of the plugin is the person best placed to make it.

**Modules added to the library since the database was written.** A new plugin has no entry until someone writes one. The help says so plainly rather than guessing — a module with no entry gets a note saying there is none, not an invented description.

**Anything that depends on a setting the reader cannot see.** Where a control changes meaning with a mode switch, the entry lists the possibilities and names the switch, rather than trying to report which one is in force. The reader can then discover that a variation exists, which they could not if the help silently described only the current state.

## How the database was made

Module by module, over a long stretch of reading, with the assistance of AI — the same disclosure that appears at the foot of every help card.

For each plugin, the starting point is whatever its maker published: their source at the version actually installed, where there is source; their manual, README or documentation site otherwise. Finding the right version matters more than it sounds. Makers routinely leave a version string alone across dozens of commits, so the release that shipped has to be identified rather than assumed — otherwise an entry describes a build nobody has.

Alongside that, three things were read together:

- **A scan of the running rack**, recording every control's position, its configured name and whether it has a widget at all. This is what attaches a description to the right control. Index order is very often not panel order — rows are reversed, enums are scattered, and some ports are declared but never placed on the panel, so no click can ever reach them.
- **The rendered panel**, with each control's index drawn onto it. The code knows a jack as index 3, the panel knows it as a hole with a label, and nothing connects the two. That picture is what lets a writer say "the jack printed CLOCK is input 7" instead of counting down a column and hoping.
- **The maker's own words** — their manual, their port descriptions, their menu strings — which say what a control is *for* in a way code does not.

Where a maker's documentation and their code disagree, the entry follows the code and records the disagreement. That happens more often than you would expect, usually because a manual describes an earlier version.

**Every entry was then checked a second time, by a different reader.** That pass was not a formality: it corrected values, rewrote citations that named a place without saying what the code did, and found faults in modules that the first reading had accepted. Some of what it found were faults in the modules themselves rather than in the descriptions — dead jacks, controls wired to the wrong thing, switches labelled backwards. Those are recorded in the entry rather than quietly smoothed over, because a reader who cannot make a control work is better served by being told it does nothing.

Mechanical checks run over the whole database continuously: the schema, the permitted spellings for a range, a citation on every fact, and contradictions between fields that cannot both be true. The library validates clean.

## How it is maintained

The database is two JSON files per module: its help in `data/help/<PluginSlug>/<ModuleSlug>.json`, which ships, and the research it was worked out from, with a citation for every fact, in `data/research/`, which does not.

**Anyone can contribute by pull request.** Fix a description, add a missing module, supply a range nobody could verify. `CONTRIBUTING.md` has the format, the schema, and the one rule about quoting that will otherwise catch you out.

**Checks run automatically on every pull request**, before a maintainer looks at anything. The file has to parse, match the schema, spell its ranges in one of the permitted forms, and carry a citation for every fact. Those failures come back within a minute with the file and line, so they are fixed by their author rather than queued for someone else.

**`CODEOWNERS` routes a change to the right reviewer.** A maker who takes on their own plugin's file is requested automatically on changes to it, so nobody has to review everything.

**Review is about truth, not mechanics.** By the time a contribution reaches a person, it parses and carries citations. What is left to judge is whether the claims are correct — which means opening the cited line and reading it.

## How it is updated

**The database ships inside the plugin.** `data/help/` is committed, one JSON file per module, so a build needs nothing but the Rack SDK, and a user gets database updates the same way they get any other plugin update — through the VCV library, automatically. There is no separate download and no version skew between the plugin and its data.

That makes the release cadence the update cadence. New plugins appear in the library continuously, so a release whenever a batch of new coverage is ready is the natural rhythm. It also gives every contribution a window in which to be caught: nothing reaches users until a build ships.

## A maker's own file

A plugin can carry its own help, in a `help` folder beside its panel art, and the Help module prefers it over the database, item by item. The file is always the right version, because it ships with the code it describes; and it gives a maker a place to say what only they know, without a pull request to anyone. A user can write the same kind of file for a module whose maker has not, and send it to be folded into the database. See [docs/help-files.md](docs/help-files.md) and [design/help-database.md](design/help-database.md).

## A note on scope

This describes other people's modules, and it is not a substitute for their manuals. The aim of a line is to tell you enough to use a control, and to tell you when there is something surprising about it that is worth looking up. Where a maker's documentation says something the code does not do, the entry follows the code and records the disagreement — not to correct anyone, but because a reader patching a cable needs what the module actually does.
