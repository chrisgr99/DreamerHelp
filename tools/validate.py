#!/usr/bin/env python3
"""Checks the help files in data/help and the research in data/research against the rules.

What can be checked mechanically is checked here, so that what has to be read by a person is only
the wording.

    python3 tools/validate.py            # every plugin
    python3 tools/validate.py Venom      # one of them
"""
import json
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
HELP = os.path.join(ROOT, 'data', 'help')
RESEARCH = os.path.join(ROOT, 'data', 'research')
PLUGINS = os.path.expanduser(
    os.environ.get('RACK_PLUGINS', '~/Library/Application Support/Rack2/plugins-mac-arm64'))
# Whether coverage can be checked at all here. See where this is used.
HAVE_RACK = os.path.isdir(PLUGINS)

# A word pointing at something the reader cannot see. The whole entry is heard one line at a time.
LEANING = re.compile(
    r'\b(still|as (on|it does|above|with)|identical to|a subset of|same as|as the \w+ does)\b',
    re.I)
# The sideways way of saying a limit. "up to ten seconds" is a range and is fine; "up to the knob"
# is a relationship dressed as one.
SIDEWAYS = re.compile(r'up to (the|its|whatever)\b', re.I)
SELLING = re.compile(r'\b(simply|just|easy|easily|powerful|versatile|perfect(?! balance))\b',
                     re.I)
# A thing doing what only a person does. It reads as writing rather than as fact, and it is
# usually covering for not having said what the control does — "where the wave sits in the mix"
# was standing in for "the phase of that wave".
PERSON = re.compile(
    r'\b(sits?|lives?|listens?|wants?|knows?|decides?|remembers?|thinks?|sees?|watches?|waits?'
    r'|likes|prefers|happily|tells you|asks)\b', re.I)
# Words that point instead of saying. If a line needs one, it has not been written yet.
VAGUE = re.compile(r'\b(somehow|sort of|kind of|handles|deals with|takes care of|affects how)\b',
                   re.I)
# THE ONLY SPELLINGS A RANGE MAY TAKE, so the column can be compared down its length rather than
# read line by line. Matches what tools/build.py already produces from the lines it harvests.
RANGE_OK = re.compile(
    r'^(?:±\d+(?:\.\d+)?V'
    r'|-?\d+(?:\.\d+)? to [-+]?\d+(?:\.\d+)?V'
    r'|1V per octave'
    r'|high above \d+(?:\.\d+)?V)$')

# A line reached by clicking the control does not need to name it. The heading on the note
# already does, and the reader is pointing at the thing.
LABELLED = re.compile(r'^[A-Z0-9][^a-z]{0,20} — ')
# A line that recites a row to somebody who clicked one thing in it.
GROUPED = re.compile(
    r'^(the )?(two|three|four|five|six|seven|eight|ten|twelve|sixteen)'
    r' (jacks|knobs|inputs|outputs|buttons|ports|rows|columns|sliders)\b', re.I)


# Rack's own Core is not a plugin folder: it is built into the application, and its manifest
# lives inside the app bundle.
CORE = ('/Applications/VCV Rack 2 Free.app/Contents/Resources/Core.json')


def models_of(slug):
    path = os.path.join(PLUGINS, slug, 'plugin.json')
    if slug == 'Core' and os.path.exists(CORE):
        path = CORE
    if not os.path.exists(path):
        return None
    with open(path) as f:
        return [m['slug'] for m in json.load(f).get('modules', [])]


HELP_KEYS = {'plugin', 'module', 'description', 'params', 'inputs', 'outputs', 'lights',
             'notes', 'menu', 'expects'}
RESEARCH_KEYS = {'plugin', 'module', 'facts', 'unplaced', 'comment', 'read', 'source'}
PLUGIN_KEYS = {'plugin', 'source', 'read', 'verification', 'comment', 'plugin_notes', 'partial'}
CONTROL_KEYS = {'params', 'inputs', 'outputs', 'lights'}
FAMILIES = ('audio', 'cv', 'trigger', 'pitch', 'mpx')
FACT_FIELDS = ('poly', 'step', 'range', 'negative', 'normal', 'sumRange', 'polarity')


def load(path, problems):
    try:
        with open(path, encoding='utf-8') as f:
            doc = json.load(f)
    except ValueError as e:
        problems.append('%s: will not parse: %s' % (os.path.relpath(path, ROOT), e))
        return None
    if not isinstance(doc, dict):
        problems.append('%s: not an object' % os.path.relpath(path, ROOT))
        return None
    return doc


def check_fact(at, one, unverified, problems):
    """WHAT A JACK EXPECTS, WHICH IS A SET OF FIELDS AND NOT A SENTENCE.

    Forty agents writing free text into the same slot is forty schemas, and the build can only
    read one. So the shape is policed here: which keys exist, what kind of value each takes, and
    — for the range — one spelling of one fact, because a column somebody is scanning for a match
    is worthless if 0-10V and "0 to 10 volts" are both in it."""
    if not isinstance(one, dict):
        problems.append('%s: not an object' % at)
        return
    for rawkey, value in one.items():
        # THE MANUAL-CHECK PASS adds a doc-prefixed twin beside a field wherever the maker's own
        # documentation states something else — docRange beside range. The measured value is
        # never touched, so both figures sit in the data and the twin is held to the same rules.
        key = rawkey
        if rawkey.startswith('doc') and len(rawkey) > 3 and rawkey[3].isupper():
            key = rawkey[3].lower() + rawkey[4:]
            if key not in FACT_FIELDS:
                problems.append('%s: "%s" is not a field here' % (at, rawkey))
                continue
            if key not in one:
                problems.append('%s: "%s" with no %s beside it' % (at, rawkey, key))
                continue
        if key == 'family':
            if value not in FAMILIES:
                problems.append('%s: family is %r, not one of %s' % (at, value, ', '.join(FAMILIES)))
        elif key == 'poly':
            if not isinstance(value, bool):
                problems.append('%s: poly is %r, not true or false' % (at, value))
        elif key == 'step':
            if value not in ('continuous', 'stepped'):
                problems.append('%s: step is %r, not continuous or stepped' % (at, value))
        elif key == 'range':
            if not isinstance(value, str) or not RANGE_OK.match(value):
                problems.append('%s: range is %r; write it as "0 to 10V", "±5V", '
                                '"1V per octave" or "high above 1V"' % (at, value))
        elif key == 'negative':
            # Clamped away before use; subtracted from a knob until the total hits its floor;
            # or carried through. Three behaviours that all look alike from outside.
            if value not in ('ignored', 'subtracts', 'swings'):
                problems.append('%s: negative is %r, not ignored, subtracts or swings' % (at, value))
        elif key == 'normal':
            # WHAT AN UNPATCHED JACK READS. Usually a voltage, sometimes another jack, so this is
            # the one field that cannot take a closed vocabulary — it is held short instead.
            if not isinstance(value, str) or not value or len(value) > 48:
                problems.append('%s: normal is %r; a short phrase, 48 characters at most'
                                % (at, value))
            elif value.endswith('.'):
                problems.append('%s: normal is a phrase, not a sentence: %r' % (at, value))
        elif key == 'sumRange':
            if not isinstance(value, str) or not RANGE_OK.match(value):
                problems.append('%s: sumRange is %r; the same spellings as range' % (at, value))
        elif key == 'polarity':
            if value not in ('unipolar', 'bipolar'):
                problems.append('%s: polarity is %r, not unipolar or bipolar' % (at, value))
        elif key != 'why':
            problems.append('%s: "%s" is not a field here' % (at, key))
    facts = [k for k in one if k not in ('family', 'why')]
    if facts and 'why' not in one and not unverified:
        # WHERE IT CAME FROM, OR IT DID NOT HAPPEN. This project has already shipped fabricated
        # constants once; a figure with no source beside it is exactly what that looked like.
        # The one way out is the notice in the plugin's research, which says outright that no
        # public source exists — and such a plugin's jacks carry no voltage figures at all.
        problems.append('%s: no "why" saying where this was established, and no "verification" '
                        'notice for the plugin' % at)
    # A THRESHOLD IS NOT A CONTINUOUS QUANTITY. "high above 2V" says the jack is read as a state,
    # and `continuous` says the voltage itself is the value; the pair cannot both be right.
    if str(one.get('range', '')).startswith('high above') and one.get('step') == 'continuous':
        problems.append('%s: range is a threshold but step says continuous; a jack read as a '
                        'state is stepped' % at)


def check_line(where, line, problems, control=None, is_menu=False):
    """The style rules, on one piece of text somebody will read or hear."""
    if not isinstance(line, str) or not line:
        problems.append('%s: empty or not text' % where)
        return
    # A line reached by clicking the control does not need to name it.
    if control and LABELLED.match(line):
        problems.append('%s names the control it describes: "%s"' % (where, line.split(' — ')[0]))
    # ONLY WHEN THE LINE COUNTS THE VERY THING IT IS ATTACHED TO. "The four outputs", on an
    # output, recites the row; "the two inputs added together", on an output, describes it.
    hit = GROUPED.match(line)
    if control and hit:
        kinds = {'inputs': 'inputs', 'ports': 'inputs', 'outputs': 'outputs',
                 'knobs': 'params', 'buttons': 'params', 'sliders': 'params'}
        if kinds.get(hit.group(3).lower()) == control:
            problems.append('%s recites a row to somebody who clicked one of it: "%s"'
                            % (where, line[:60]))
    if len(line) > 200:
        problems.append('%s: %d characters, too long to hear in one piece' % (where, len(line)))
    # A MENU ITEM'S NAME IS THE MAKER'S WORDS, NOT OURS: "<the item as the menu prints it> —
    # what it does". The name is quoted exactly, so the style rules read only what follows it.
    ours = line
    if is_menu and ' — ' in line:
        ours = ' — '.join(line.split(' — ')[1:])
    for rule, why in ((LEANING, 'leans on something the reader cannot see'),
                      (SIDEWAYS, 'says a relationship sideways'),
                      (SELLING, 'sells rather than says'),
                      (PERSON, 'gives a thing a person\'s verb'),
                      (VAGUE, 'points instead of saying')):
        hit = rule.search(ours)
        if hit:
            problems.append('%s %s: "%s"' % (where, why, hit.group(0)))


def check_plugin(plugin):
    problems = []
    folder = os.path.join(HELP, plugin)
    meta_path = os.path.join(RESEARCH, plugin, '.plugin.json')
    meta = load(meta_path, problems) if os.path.isfile(meta_path) else None
    if meta is None:
        problems.append('%s: no research/%s/.plugin.json saying where it was read' % (plugin, plugin))
        meta = {}
    for k in meta:
        if k not in PLUGIN_KEYS:
            problems.append('%s/.plugin.json: "%s" is not a field here' % (plugin, k))
    if not meta.get('source'):
        problems.append('%s: no source URL' % plugin)
    # WHERE THE FIGURES COULD NOT COME FROM. A plugin whose voltage ranges were only ever
    # established from its compiled binary carries a notice saying so, which stands in for a `why`
    # on every fact below, and its jacks carry no range at all.
    unverified = isinstance(meta.get('verification'), str) and bool(meta['verification'].strip())

    models = sorted(n[:-5] for n in os.listdir(folder) if n.endswith('.json') and not n.startswith('.'))
    installed = models_of(plugin)
    partial = bool(meta.get('partial'))
    if partial or (installed is None and not HAVE_RACK):
        # NOTHING TO CHECK AGAINST, AND THAT IS NOT THE CONTRIBUTOR'S FAULT. Coverage is confirmed
        # against the installed plugin's own manifest, which exists on a machine running Rack and
        # not on the machine running CI.
        pass
    elif installed is None:
        problems.append('%s is not installed, so coverage cannot be checked' % plugin)
    else:
        missing = [m for m in installed if m not in models]
        extra = [m for m in models if m not in installed]
        if missing:
            problems.append('%s: %d models with no help: %s'
                            % (plugin, len(missing), ', '.join(missing[:8])))
        if extra:
            problems.append('%s: %d help files for models that are not installed: %s'
                            % (plugin, len(extra), ', '.join(extra[:8])))

    for model in models:
        where = '%s/%s' % (plugin, model)
        doc = load(os.path.join(folder, model + '.json'), problems)
        if doc is None:
            continue
        if doc.get('plugin') != plugin or doc.get('module') != model:
            problems.append('%s: plugin and module say %s/%s' % (where, doc.get('plugin'),
                                                                 doc.get('module')))
        for k in doc:
            if k not in HELP_KEYS:
                problems.append('%s: "%s" is not a field of a help file' % (where, k))
        if not doc.get('description'):
            problems.append('%s: no description' % where)
        else:
            check_line('%s description' % where, doc['description'], problems)
        count = 1
        for key in CONTROL_KEYS:
            seen = set()
            for n, entry in enumerate(doc.get(key) or []):
                at = '%s %s entry %d' % (where, key, n + 1)
                ids = entry.get('ids') if isinstance(entry, dict) else None
                if not isinstance(ids, list) or not ids or not all(
                        isinstance(i, int) and not isinstance(i, bool) and i >= 0 for i in ids):
                    problems.append('%s: ids must be a list of control numbers' % at)
                    continue
                for i in ids:
                    if i in seen:
                        problems.append('%s: %s %d is described twice' % (at, key, i))
                    seen.add(i)
                count += 1
                check_line(at, entry.get('text'), problems, control=key)
        for i, line in enumerate(doc.get('notes') or []):
            count += 1
            check_line('%s note %d' % (where, i + 1), line, problems)
        for i, line in enumerate(doc.get('menu') or []):
            count += 1
            check_line('%s menu %d' % (where, i + 1), line, problems, is_menu=True)
        # A GENEROUS CEILING, NOT A TARGET, to catch a runaway generator: the largest modules
        # have sixty-odd controls of one kind.
        if count > 200:
            problems.append('%s: %d lines, which is more than any module has controls'
                            % (where, count))

        rpath = os.path.join(RESEARCH, plugin, model + '.json')
        if not os.path.isfile(rpath):
            continue
        research = load(rpath, problems)
        if research is None:
            continue
        for k in research:
            if k not in RESEARCH_KEYS:
                problems.append('%s research: "%s" is not a field here' % (where, k))
        for i, line in enumerate(research.get('unplaced') or []):
            check_line('%s unplaced %d' % (where, i + 1), line, problems)
        for key, ports in (research.get('facts') or {}).items():
            if key not in ('inputs', 'outputs'):
                problems.append('%s research: facts has "%s", which is not inputs or outputs'
                                % (where, key))
                continue
            for port, one in (ports or {}).items():
                check_fact('%s facts %s %s' % (where, key, port), one, unverified, problems)
    for name in os.listdir(os.path.join(RESEARCH, plugin)) if os.path.isdir(os.path.join(RESEARCH, plugin)) else []:
        if name.endswith('.json') and not name.startswith('.') and name[:-5] not in models:
            problems.append('%s: research for %s, which has no help file' % (plugin, name[:-5]))
    return problems


def main():
    only = sys.argv[1] if len(sys.argv) > 1 else None
    problems = []
    plugins = 0
    for plugin in sorted(os.listdir(HELP)):
        if not os.path.isdir(os.path.join(HELP, plugin)):
            continue
        if only and not plugin.lower().startswith(only.lower()):
            continue
        plugins += 1
        problems += check_plugin(plugin)
    for p in problems:
        print(p)
    print('%d plugin(s), %d problem(s)' % (plugins, len(problems)))
    return 1 if problems else 0


if __name__ == '__main__':
    sys.exit(main())
