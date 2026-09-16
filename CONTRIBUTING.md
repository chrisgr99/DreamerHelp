# Contributing to the help database

The database is `data/plugins/<PluginSlug>.yaml`, one file per VCV plugin, named after the plugin's slug in its `plugin.json`. Edit the file for the plugin you know about and open a pull request.

## The one rule that will bite you

**Quote every string value with single quotes. Always, even when it looks unnecessary.**

```yaml
lines:
  - 'Off'          # correct
  - Off            # WRONG — YAML reads this as the boolean false
```

YAML silently converts several bare words and numbers into other types. `Off`, `No` and `on` become booleans. `2.10` becomes the number 2.1 and loses its trailing zero. `012` becomes 12. `~` and `NULL` become nothing at all. Menu items in real modules are called things like "Off", so this is not hypothetical.

A blanket quote-everything rule needs no judgement, which is why it is the rule rather than a list of exceptions. The checks will catch a lapse, but they will catch it after you have pushed.

Long prose goes in a block scalar:

```yaml
notes: >-
  The four CV jacks replace their knobs rather than adding to them, so a patched
  cable resting at 0V silences the control while the knob shows its old position.
```

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
