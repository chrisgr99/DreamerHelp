# Contributing to the help database

The database is `data/plugins/<PluginSlug>.yaml`, one file per VCV plugin, named after the plugin's slug in its `plugin.json`. Edit the file for the plugin you know about and open a pull request.

## Finding the right file

Files are named after the **plugin** slug, not the module's name on the panel. Three ways to find it:

- **The folder name.** Rack unpacks each plugin into `Rack2/plugins-<platform>/<PluginSlug>/`, so the folder is the slug. On a Mac that is `~/Library/Application Support/Rack2/plugins-mac-arm64/`.
- **The library page.** `library.vcvrack.com/<PluginSlug>` — the slug is in the address.
- **Search this repository** for the module's name. Module slugs are the keys under `modules:`, and they usually resemble the name on the panel.

Inside the file, a module is a key under `modules:` — its **module** slug, which is what that plugin's own `plugin.json` calls it. That is often not the name printed on the panel: Bidoo's reverb is on the panel as REI and in `plugin.json` as `REI`, but plenty of makers use an internal name for one and a display name for the other.

## The one rule that will bite you

**Quote every string value with single quotes. Always, even when it looks unnecessary.**

```yaml
lines:
  - 'Off'          # correct
  - Off            # WRONG — YAML reads this as the boolean false
```

YAML silently converts several bare words and numbers into other types. `Off`, `No` and `on` become booleans. `2.10` becomes the number 2.1 and loses its trailing zero. `012` becomes 12. `~` and `NULL` become nothing at all. Menu items in real modules are called things like "Off", so this is not hypothetical.

A blanket quote-everything rule needs no judgement, which is why it is the rule rather than a list of exceptions. The checks will catch a lapse, but they will catch it after you have pushed.

**Values that are genuinely not text stay bare.** `poly` is a real boolean and the line numbers in the tag maps are real integers, so they are written `true`, `false` and `3` — never `'true'` or `'3'`. The validator checks their types and will reject a quoted one. The rule is *quote every string*, not *quote everything*:

```yaml
'poly': true        # a boolean — bare
'in': {'0': 3}      # an index and a line number — bare
'range': '0 to 10V' # text — quoted
```

Long prose goes in a block scalar:

```yaml
notes: >-
  The four CV jacks replace their knobs rather than adding to them, so a patched
  cable resting at 0V silences the control while the knob shows its old position.
```

## What a file looks like

```yaml
'plugin': 'Bidoo'                    # the plugin slug; matches the filename
'source': 'https://github.com/…'     # where the entries were established from
'read': '2026-09-13'                 # when, so drift is datable
'modules':
  'REI':                             # the module slug
    'lines':
      - 'Reverb with a freeze control and a pitch shifter in the tail'
      - 'Sets how large the space sounds, from a small room to a hall'
      - 'Note — nothing is heard until something is patched to IN L.'
      - 'Menu — Oversampling: off, 2x, 4x — trades CPU for fewer artefacts.'

    # WHICH LINE DESCRIBES WHICH CONTROL. The key is the control's index; the
    # value is the position in `lines` above, counting from zero.
    'param': {'0': 1}
    'in':    {'0': 2}
    'out':   {'0': 3}

    # What each jack carries, for colouring: audio, cv, trigger or pitch.
    # Leave a jack out where it carries whatever you patch — a mult, a router.
    'family':
      'in':  {'0': 'audio'}
      'out': {'0': 'audio'}

    # What each jack expects. Every field needs a `why`; see below.
    'props':
      'in':
        '0':
          'poly': true
          'range': '0 to 10V'
          'step': 'continuous'
          'negative': 'subtracts'
          'normal': 'the jack above it'
          'sumRange': '0 to 1V'
          'why': 'REI.cpp:84 adds this voltage to the knob and clamps the sum to 0 and 1'

    # Anything that will not fit a field: mode-dependent behaviour, a maker's
    # bug, why something was left blank.
    'notes': 'The pitch shifter runs only while FREEZE is on.'
```

**Every one of those keys is optional except `lines`.** A module with no jacks needs no `in`, `out`, `family` or `props`.

**The first line is what the module is.** The rest are one per control. A line beginning `Note — ` is about the module rather than a control; a line beginning `Menu — ` is one context menu item, named exactly as the menu prints it.

**The tag maps are the part to be careful with.** They point a control's index at a line by position, so inserting a line in the middle shifts every line after it and silently re-points the maps. Add at the end, or fix the maps.

A control with no widget on the panel gets no tag and no line — no click can ever reach it.

## Every fact carries a citation

Each port field has a `why` saying where the fact came from — a file and a line in the maker's published source, the page of a manual that states it, or the test you ran to find out (see below):

```yaml
props:
  in:
    '0':
      poly: true
      range: '0 to 10V'
      step: 'continuous'
      why: 'Foo.cpp:212 reads the jack with getPolyVoltage, divides it by ten and adds it to the knob, the total clamped to 0 and 1'
```

A citation must say what the code **does**, not merely where it lives. `Foo.cpp:212` alone is not a citation — nobody reading it can tell whether you read the line or guessed from the shape of the file. Name the mechanism: what is read, what it is scaled or compared against, what it is added to.

This is what makes a pull request reviewable. A claim that a jack takes ±5V cannot be checked by anyone; "line 212 clamps it to ±5V" can be checked in seconds, by a person or by a machine.

## Observation is a valid source

Not every fact can be cited to a file. Where a plugin publishes no source, the only way to establish what a jack does may be to patch something into it and watch what happens. That is a legitimate basis and the database accepts it.

It is held to the same standard as a code citation, and for the same reason: **say what you did and what happened, so that somebody else can repeat it.**

```yaml
why: 'observed — a sixteen-channel cable into IN comes out of OUT with all sixteen channels'
why: 'observed — a 1V trigger does not fire it; it fires at about 2V'
why: 'observed — with nothing patched to IN the output sits at 5V, not 0V'
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

Where the two readings differ — a jack clamped to ±10V but scaled so ±1V already covers the control — **the range is the span**, ±1V, and the wider clamp goes in `notes`.

Every figure in these fields is volts at the jack. A clamp expressed in a parameter's own units must be converted to the voltage that reaches it, or left out.

Four spellings only, so the column can be scanned rather than read: `0 to 10V`, `±5V`, `1V per octave`, `high above 2V`. Use whatever number is the fact.

## Leaving a field blank is often correct

A blank is not a gap to be filled for its own sake. An audio input usually has no range — you patch what you like and the module multiplies it. A jack that passes whatever you give it has no family and no range. A control that is only bounded in seconds, or steps, or hertz, settles nothing about voltage.

**Do not fabricate a plausible figure.** A wrong value is worse than a blank, because a blank invites somebody to look and a figure does not.

## Plugins with no published source

Some files carry a `verification` notice saying that voltage ranges are not published for that plugin because no public source is available. Those entries are the ones most worth a contribution, and their makers are the people best placed to make it. Supply the range and a citation, and remove the notice when nothing in the file needs it any more.

## Before you push

```
make validate
```

The same checks run automatically on your pull request, so you will hear about a failure either way — but hearing it locally is faster.
