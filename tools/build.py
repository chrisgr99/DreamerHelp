#!/usr/bin/env python3
"""Keeps what each jack expects, in data/help, in step with the research behind it.

    python3 tools/build.py            rewrite any help file whose "expects" has fallen behind
    python3 tools/build.py --check    report them and change nothing (the pull-request check)

TWO FILES PER MODULE. data/help/<Plugin>/<Module>.json is the help, in the format makers use,
and ships with the plugin. data/research/<Plugin>/<Module>.json is what it was worked out from
and does not ship: each jack's family, the facts about what it expects and the citation for each,
the writer's comments. The one thing the help file takes from the research is the short phrase
under each jack — range, shape, polyphony — which is derived here from the facts and the jack's
own line, by the rules below, so that changing a rule does not mean rewriting a hundred files.
"""
import json
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
HELP = os.path.join(ROOT, 'data', 'help')
RESEARCH = os.path.join(ROOT, 'data', 'research')


# WHAT A JACK EXPECTS, AS FACTS RATHER THAN A SENTENCE.
#
# The Rack forum has been asking for a year how to know what an input port wants — a sensible
# voltage range, whether the signal is continuous or stepped, whether it takes polyphony — and
# resorting to scopes and test rigs for the answer. Two of those three fall straight out of the
# family we already recorded for 13,711 input ports, so they are derived here rather than stored:
# the data keeps only what a person established by hand, and changing a rule below does not mean
# rewriting a hundred files.
#
# THE LINE THIS WILL NOT CROSS. A volt-per-octave port IS one volt per octave and IS continuous;
# that is what the standard means. A trigger or gate port is stepped because the signal is a state
# rather than a value — but its HEIGHT is not decided by the family, since 5V and 10V gates are
# both common, so no range is claimed. Audio and CV get continuity and nothing else: both have a
# conventional range, and "conventionally" would be doing all the work in that sentence. A port's
# real range is a fact about the module, and this project has already been bitten once by numbers
# that were plausible rather than checked.
#
# AND THE FAMILY IS NOT ENOUGH ON ITS OWN FOR THE RANGE. The families were decided for COLOUR,
# and colour tolerates a port that is pitch-ish: Bidoo's pErCO takes an attenuated frequency
# modulation whose effect depends on where a knob sits, and Impromptu's Chord-Key reads 2V as the
# twenty-fifth chord. Both are fairly drawn as pitch. Neither is one volt per octave, and saying
# so would be the same failure as a plausible constant. So the range is claimed only where the
# port's OWN LINE says per-octave — which is a fact somebody read off a manual, not an inference
# from a colour. See PER_OCTAVE below.
DERIVED = {
    'pitch':   {'step': 'continuous'},
    # A TRIGGER PORT IS STEPPED BY DEFINITION — but only if it really is one. Of 4,204
    # trigger-tagged inputs, 3,705 lines talk about gates, edges, pulses or clocks and 499 do not;
    # NYSTHI's LOGAN20 has one described as "one of the twenty voltages written into each row of
    # the file", which is a continuous CV wearing a trigger colour because the colour was chosen
    # for a jack's company rather than its contents. So this family, like CV, waits for the line
    # to agree — see STEPPED below.
    'trigger': {},
    'audio':   {'step': 'continuous'},
    # CV GETS NOTHING, and this is the same trap the range fell into. A family was chosen to
    # COLOUR a jack, and "control voltage" covers both a filter cutoff and a selector that reads
    # one chord type per volt — AaronStatic's ChordCV is exactly that. Continuous is true of most
    # of them and false of enough to matter, and there is no way to tell which from the colour. So
    # the largest family in the library says nothing about its shape until somebody establishes
    # it per port.
    'cv':      {},
    # MPX carries a note stream and not a voltage at all, so all three are the wrong question.
    # Listed to say so deliberately rather than by omission.
    'mpx':     {},
}


# THE WAYS THE ENTRIES ACTUALLY SAY IT, counted rather than imagined: "volt per octave", "a volt
# to the octave", "an octave per volt", "one octave per volt", "V/OCT". A port whose own line says
# one of these has had somebody read a manual and write it down, which is the standard the rest of
# this project holds to.
PER_OCTAVE = re.compile(
    r'volt[- ]per[- ]octave|V/OCT|(?:a|one|1) ?V(?:olt)? (?:to|per) the octave'
    r'|(?:an?|one|1) octave per volt|1V per octave', re.I)

# AND THE RANGES ALREADY WRITTEN DOWN, IN THE SENTENCES THEY ARE HIDING IN.
#
# About a twelfth of the input lines state a range while saying something else — "Scales that
# channel's level, 0V to 10V", "A rising edge above 1V restarts every track". Somebody read those
# off a manual, so they are as good as anything a fresh scan would produce, and better than a
# convention. They are simply in prose, where nothing can sort them or compare them between
# modules, which is the whole of what the forum is asking for.
#
# WHOSE RANGE IT IS, THOUGH, IS NOT GUESSED. A line about a jack often mentions a knob or an
# output in the same breath and the figure may belong to either. So a figure counts only when it
# is in the line's opening clause with nothing else that could own it — the lines are written to
# be heard, so their clauses are marked plainly and a semicolon is a real boundary. Everything
# else is left alone for somebody to settle.
# THE LANGUAGE OF A SIGNAL THAT IS A STATE RATHER THAN A VALUE. A line that talks about edges,
# gates, pulses, clocks or a threshold is describing something stepped, whoever wrote it.
STEPPED = re.compile(
    r'rising edge|falling edge|\bedge\b|\btrigger|\bgate\b|\bgates\b|\bpulse|\bclock'
    r'|counted as (?:true|1)|counts as true|above \d+(?:\.\d+)?\s?V|\bhigh\b|\btoggl|\blatch', re.I)

NUM = r'[-+−]?\d+(?:\.\d+)?'
SYMMETRIC = re.compile(r'±\s?(%s)\s?V' % NUM)
SPAN = re.compile(r'(%(n)s)\s?V?\s*(?:to|-|–)\s*(%(n)s)\s?V' % {'n': NUM})
THRESHOLD = re.compile(r'(?:above|at least|over)\s+(%s)\s?V' % NUM, re.I)
OTHER = re.compile(r'\bknob|\bslider|\bswitch|\boutput|\bbutton|\battenuverter|\bmenu\b', re.I)
BREAK = re.compile(r';|—| – ')


def range_in_line(line, family):
    """The voltage range this line states for its own port, or None.

    Conservative by design: one figure, in the opening clause, with nothing else in that clause it
    could belong to. A line with two ranges in it is somebody's careful sentence about two modes,
    and picking one of them would be inventing a fact."""
    first = BREAK.split(line)[0]
    if OTHER.search(first):
        return None
    hits = SYMMETRIC.findall(first)
    if len(hits) == 1 and not SPAN.search(first):
        return '±%sV' % hits[0]
    spans = SPAN.findall(first)
    if len(spans) == 1:
        lo, hi = spans[0]
        # ONE SPELLING FOR ONE FACT. The entries write a symmetric pair four different ways —
        # "-5 to 5V", "-5V to +5V", "-10 to +10V", "±5V" — and a column somebody is scanning for
        # a match wants them to look the same. Collapse them all to the ± form.
        try:
            if float(lo.replace('−', '-')) == -float(hi.replace('+', '')):
                return '±%sV' % hi.lstrip('+')
        except ValueError:
            pass
        return '%s to %sV' % (lo.rstrip('V'), hi)
    if spans:
        return None
    # A gate's height is what the forum most wants, and a threshold is how the lines state it.
    if family == 'trigger':
        hits = THRESHOLD.findall(first)
        if len(hits) == 1:
            return 'high above %sV' % hits[0]
    return None


def polarity_of(rng):
    """Unipolar or bipolar, worked out from a range that is already established.

    A range says this outright — 0 to 10V is unipolar, ±5V is bipolar — so nobody should have to
    write both down, and a port with a range is not asked for its polarity. The field exists for
    the other case, which is commoner than it sounds: code that centres a signal, or clamps
    asymmetrically, often shows the polarity plainly while pinning no range at all.

    A volt-per-octave port is bipolar. Nothing in the standard says so, but C4 is 0V and every
    note below it is negative, which is the whole point of the convention."""
    if not rng:
        return None
    if rng.startswith('±') or rng == '1V per octave':
        return 'bipolar'
    if rng.startswith('high above '):
        return 'unipolar'
    hit = SPAN.search(rng)
    if hit:
        try:
            return 'bipolar' if float(hit.group(1).replace('−', '-')) < 0 else 'unipolar'
        except ValueError:
            return None
    return None


def expects_of(doc, research, key):
    """One short phrase per jack of one kind, by number, where there is anything to say.

    Order is fixed — range, then shape, then polyphony — so that the line can be read at a glance
    and heard in one piece, and so that a missing fact reads as a gap rather than as a different
    fact."""
    facts = ((research.get('facts') or {}).get(key)) or {}
    lines = {}
    for entry in doc.get(key) or []:
        for i in entry.get('ids', []):
            lines[str(i)] = entry.get('text', '')
    out = {}
    for port, fact in facts.items():
        known = dict(fact)
        family = known.pop('family', None)
        got = dict(DERIVED.get(family) or {})
        # The port's own line is the evidence for a range, not its colour.
        line = lines.get(port)
        if line is not None:
            if family == 'trigger' and STEPPED.search(line):
                got['step'] = 'stepped'
            if family == 'pitch' and PER_OCTAVE.search(line):
                got['range'] = '1V per octave'
            else:
                found = range_in_line(line, family)
                if found:
                    got['range'] = found
        got.update(known)
        poly = got.get('poly')
        # POLARITY ONLY WHERE THE RANGE DOES NOT ALREADY SAY IT. "0 to 10V · unipolar" says
        # one thing twice and spends a third of the line doing it; the word earns its place
        # exactly when no range could be pinned down.
        rng = got.get('range')
        # A NEGATIVE VOLTAGE SWINGING THROUGH IS WHAT BIPOLAR MEANS, so these two fields are one
        # fact asked twice, and `negative` is the better half: it tells a jack that ignores a
        # negative voltage apart from one that subtracts it from a knob, where "unipolar" would
        # cover both.
        neg = got.get('negative')
        polar = got.get('polarity') or (
            'bipolar' if neg == 'swings' else 'unipolar' if neg == 'ignored' else None
        ) or polarity_of(rng)
        parts = [rng, None if rng else polar, got.get('step'),
                 None if poly is None else ('polyphonic' if poly else 'one channel only')]
        parts = [p for p in parts if p]
        if not parts:
            continue
        # THE QUICK LINE STAYS QUICK. Four facts in a fixed order is what makes a column of these
        # scannable, so anything more goes on a second line underneath, in plain words — its own
        # paragraph, so it can be clicked and heard on its own and skipped by somebody who only
        # wanted the first.
        more = []
        if got.get('normal'):
            more.append('unpatched it reads %s' % got['normal'])
        if neg == 'ignored':
            more.append('a negative voltage does nothing here')
        elif neg == 'subtracts':
            more.append('a negative voltage subtracts from the knob')
        if got.get('sumRange'):
            more.append('the knob and this input are summed and the total held to %s'
                        % got['sumRange'])
        text = ' · '.join(parts)
        if more:
            text += '\n\n' + '; '.join(more) + '.'
        out[port] = text
    return dict(sorted(out.items(), key=lambda kv: int(kv[0])))


NOTE = 'Note \u2014 '
MENU = 'Menu \u2014 '


def refresh_expects(doc, research):
    """Puts the derived phrases into a help file, in place. Returns whether anything changed."""
    expects = {}
    for key in ('inputs', 'outputs'):
        got = expects_of(doc, research, key)
        if got:
            expects[key] = got
    before = doc.get('expects')
    if expects:
        doc['expects'] = expects
    else:
        doc.pop('expects', None)
    # "expects" is kept last, where a reader expects the least-read part of a file to be.
    if 'expects' in doc:
        doc['expects'] = doc.pop('expects')
    return before != doc.get('expects')


def dump(doc):
    """A file as a person would lay it out: one field to a line, each entry of a list on a line
    of its own, and a table of tables — facts and expects — one row to a line, so a control's
    numbers and its text are read together. Plain JSON either way; the layout is only for the
    reader, and for a diff that shows one changed entry as one changed line."""
    one = lambda v: json.dumps(v, ensure_ascii=False)
    out = ['{']
    keys = list(doc)
    for k, key in enumerate(keys):
        value = doc[key]
        comma = ',' if k + 1 < len(keys) else ''
        if isinstance(value, list) and value:
            out.append('  %s: [' % one(key))
            for i, item in enumerate(value):
                out.append('    %s%s' % (one(item), ',' if i + 1 < len(value) else ''))
            out.append('  ]' + comma)
        elif isinstance(value, dict) and value:
            out.append('  %s: {' % one(key))
            inner = list(value)
            for i, sub in enumerate(inner):
                last = ',' if i + 1 < len(inner) else ''
                v = value[sub]
                if isinstance(v, dict) and v and all(isinstance(x, dict) for x in v.values()):
                    out.append('    %s: {' % one(sub))
                    rows = list(v)
                    for j, row in enumerate(rows):
                        out.append('      %s: %s%s' % (one(row), one(v[row]),
                                                       ',' if j + 1 < len(rows) else ''))
                    out.append('    }' + last)
                elif isinstance(v, dict) and v:
                    out.append('    %s: {' % one(sub))
                    rows = list(v)
                    for j, row in enumerate(rows):
                        out.append('      %s: %s%s' % (one(row), one(v[row]),
                                                       ',' if j + 1 < len(rows) else ''))
                    out.append('    }' + last)
                else:
                    out.append('    %s: %s%s' % (one(sub), one(v), last))
            out.append('  }' + comma)
        else:
            out.append('  %s: %s%s' % (one(key), one(value), comma))
    out.append('}')
    return '\n'.join(out) + '\n'


def modules():
    """Every help file, as (plugin, module, help path, research path)."""
    for plugin in sorted(os.listdir(HELP)):
        folder = os.path.join(HELP, plugin)
        if not os.path.isdir(folder):
            continue
        for name in sorted(os.listdir(folder)):
            if name.endswith('.json') and not name.startswith('.'):
                model = name[:-5]
                yield (plugin, model, os.path.join(folder, name),
                       os.path.join(RESEARCH, plugin, name))


def main():
    check = '--check' in sys.argv[1:]
    stale = []
    count = 0
    for plugin, model, path, rpath in modules():
        count += 1
        with open(path, encoding='utf-8') as f:
            doc = json.load(f)
        research = {}
        if os.path.isfile(rpath):
            with open(rpath, encoding='utf-8') as f:
                research = json.load(f)
        before = dump(doc)
        refresh_expects(doc, research)
        after = dump(doc)
        if after != before:
            stale.append('%s/%s' % (plugin, model))
            if not check:
                with open(path, 'w', encoding='utf-8') as f:
                    f.write(after)
    if check:
        for s_ in stale:
            print('stale: data/help/%s.json' % s_)
        print('%d modules, %d with expects out of step' % (count, len(stale)))
        return 1 if stale else 0
    print('%d modules, %d updated' % (count, len(stale)))
    return 0


if __name__ == '__main__':
    sys.exit(main())
