# Contributing to the help database

Each module has two files, both named after the plugin and module slugs:

- **`data/help/<PluginSlug>/<ModuleSlug>.json`** is the help itself, in the same format makers use for their own help files, and it ships with the plugin. [docs/help-files.md](docs/help-files.md) describes the format.
- **`data/research/<PluginSlug>/<ModuleSlug>.json`** is what the help was worked out from, and it does not ship: what each jack carries and expects, with a citation for every fact, and anything a reviewer should know. Each plugin also has `data/research/<PluginSlug>/.plugin.json`, saying where its entries were read.

Edit the files for the module you know about and open a pull request.

## Finding the right files

Folders are named after the **plugin** slug, not the module's name on the panel. Three ways to find it:

- **The folder name.** Rack unpacks each plugin into `Rack2/plugins-<platform>/<PluginSlug>/`, so the folder is the slug. On a Mac that is `~/Library/Application Support/Rack2/plugins-mac-arm64/`.
- **The library page.** `library.vcvrack.com/<PluginSlug>` — the slug is in the address.
- **Search this repository** for the module's name.

Inside the folder, each file is named after the **module** slug — what that plugin's own `plugin.json` calls it. That is often not the name printed on the panel: plenty of makers use an internal name for one and a display name for the other.

## The help file

```json
{
  "plugin": "Bidoo",
  "module": "REI",
  "description": "Reverb with a freeze control and a pitch shifter in the tail",
  "params": [
    {"ids": [0], "text": "Sets how large the space sounds, from a small room to a hall"}
  ],
  "inputs": [
    {"ids": [0, 1], "text": "Audio into the reverb, left and right"}
  ],
  "notes": [
    "nothing is heard until something is patched to IN L."
  ],
  "menu": [
    "Oversampling: off, 2x, 4x — trades CPU for fewer artefacts."
  ],
  "expects": {
    "inputs": {"0": "0 to 10V · continuous · polyphonic"}
  }
}
```

**The description is what the module is.** Each entry under `params`, `inputs`, `outputs` and `lights` is one line of text and the numbers of every control it describes. `notes` are about the module rather than a control; each `menu` line is one context menu item, named exactly as the menu prints it, then what it does.

**Do not edit `expects` by hand.** It is worked out from the research file by `make helptext`, and the checks fail a pull request where it has fallen behind.

A control with no widget on the panel gets no entry — no click can ever reach it.

## The research file

```json
{
  "plugin": "Bidoo",
  "module": "REI",
  "facts": {
    "inputs": {
      "0": {"family": "audio", "poly": true, "range": "0 to 10V", "step": "continuous", "negative": "subtracts", "normal": "the jack above it", "sumRange": "0 to 1V", "why": "REI.cpp:84 adds this voltage to the knob and clamps the sum to 0 and 1"}
    },
    "outputs": {
      "0": {"family": "audio"}
    }
  },
  "comment": "The pitch shifter runs only while FREEZE is on."
}
```

**`family`** is what a jack carries, for colouring in Clarity: `audio`, `cv`, `trigger` or `pitch`. Leave it out where a jack carries whatever you patch — a mult, a router.

**The other fields are what a jack expects**, and every one of them needs a `why`; see below. **`comment`** is anything that will not fit a field: mode-dependent behaviour, a maker's bug, why something was left blank.

Every field is optional. A module with no jacks needs no research file at all.

## Every fact carries a citation

Each port field has a `why` saying where the fact came from — a file and a line in the maker's published source, the page of a manual that states it, or the test you ran to find out (see below):

```json
"0": {"poly": true, "range": "0 to 10V", "step": "continuous", "why": "Foo.cpp:212 reads the jack with getPolyVoltage, divides it by ten and adds it to the knob, the total clamped to 0 and 1"}
```

A citation must say what the code **does**, not merely where it lives. `Foo.cpp:212` alone is not a citation — nobody reading it can tell whether you read the line or guessed from the shape of the file. Name the mechanism: what is read, what it is scaled or compared against, what it is added to.

This is what makes a pull request reviewable. A claim that a jack takes ±5V cannot be checked by anyone; "line 212 clamps it to ±5V" can be checked in seconds, by a person or by a machine.

## Observation is a valid source

Not every fact can be cited to a file. Where a plugin publishes no source, the only way to establish what a jack does may be to patch something into it and watch what happens. That is a legitimate basis and the database accepts it.

It is held to the same standard as a code citation, and for the same reason: **say what you did and what happened, so that somebody else can repeat it.**

```json
"why": "observed — a sixteen-channel cable into IN comes out of OUT with all sixteen channels"
"why": "observed — a 1V trigger does not fire it; it fires at about 2V"
"why": "observed — with nothing patched to IN the output sits at 5V, not 0V"
```

Begin it with `observed —` so a reader can tell at a glance which kind of evidence it is. That matters when somebody later finds the source: a fact resting on a test may need re-checking against the code, and a fact resting on a line of code does not.

What is not acceptable is a claim with no method behind it. "I tested it" cannot be repeated by anyone. Neither can "this is how it behaves". The test is the citation.

Two cautions worth knowing, because both have produced wrong entries before now:

- **A module's behaviour can depend on a setting you are not looking at.** A jack that ignores negative voltage in one mode may carry it in another. Say which mode you tested in, or test them all.
- **An absence is hard to observe.** "Nothing happens when I patch this" is evidence that nothing happened in the case you tried, which is not the same as the jack being dead. Where you are claiming a control does nothing at all, say what you varied while finding out.

## What `range` means

**The voltage that moves the jack over its full travel.** Not what it will physically accept — nothing in Rack bounds a cable, so every jack accepts anything and saying so tells a reader nothing.

Three kinds of evidence settle it:

- a clamp on the jack — the clamp is the span;
- a scaling into a bounded destination — `v * 0.1f` added to a knob running 0 to 1 is swept end to end by 10V, so the range is `0 to 10V` even though nothing clamps the jack;
- a clamp on a knob-plus-jack total the jack enters unscaled — then `sumRange` carries the clamp.

Where the two readings differ — a jack clamped to ±10V but scaled so ±1V already covers the control — **the range is the span**, ±1V, and the wider clamp goes in `comment`.

Every figure in these fields is volts at the jack. A clamp expressed in a parameter's own units must be converted to the voltage that reaches it, or left out.

Four spellings only, so the column can be scanned rather than read: `0 to 10V`, `±5V`, `1V per octave`, `high above 2V`. Use whatever number is the fact.

## Leaving a field blank is often correct

A blank is not a gap to be filled for its own sake. An audio input usually has no range — you patch what you like and the module multiplies it. A jack that passes whatever you give it has no family and no range. A control that is only bounded in seconds, or steps, or hertz, settles nothing about voltage.

**Do not fabricate a plausible figure.** A wrong value is worse than a blank, because a blank invites somebody to look and a figure does not.

## Plugins with no published source

Some plugins' `.plugin.json` carries a `verification` notice saying that voltage ranges are not published for that plugin because no public source is available. Those entries are the ones most worth a contribution, and their makers are the people best placed to make it. Supply the range and a citation, and remove the notice when nothing in the plugin needs it any more.

## Before you push

```
make helptext
make validate
```

The first brings each help file's `expects` up to date with its research; the second checks everything. The same checks run automatically on your pull request, so you will hear about a failure either way — but hearing it locally is faster.

**A maker describing their own plugin** does not need this repository at all: ship help files in your own plugin instead, and they come before the database. See [docs/help-files.md](docs/help-files.md).
