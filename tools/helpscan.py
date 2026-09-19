#!/usr/bin/env python3
"""Writes a plugin's help files from marked comments in its source code.

    python3 helpscan.py src/*.cpp                write help/<ModuleSlug>.json for each module
    python3 helpscan.py --out res/help src/*.cpp write them somewhere else
    python3 helpscan.py --check src/*.cpp        report what would be written, write nothing

Python 3 and its standard library, nothing else, so it runs wherever the Rack SDK's own
helper.py does. See design/help-database.md in DreamerHelp for the file format and the reasons.

THE MARKERS. A line comment beginning with //? :

    configParam(TEMPO_PARAM, 30.f, 300.f, 120.f, "Tempo", " BPM");
    //? Sets the tempo when no clock is patched, 30 to 300 beats per minute.
    //? With a clock patched, shows the tempo measured from the clock.

    //?module Plays a chord chart as an MPX stream and as pitch voltages.
    //?note A new module has no chart in it and plays nothing until a song is chosen.

- //? lines directly after a configure call (configParam, configSwitch, configButton,
  configInput, configOutput, configLight) are that control's help. Plain comments and blank
  lines between the call and them are skipped; any other code ends the search.
- //?module is the module's description; //?note starts one note. Further //? lines continue
  whichever of these came last. They belong to the module whose code they are directly above,
  with only comments and blank lines between; otherwise to the module whose code they are in.
- Lines are joined with a space, so the help can be wrapped in the source to any width.
- A call inside a loop whose first argument is an ENUMS run plus a counter, or a name plus the
  loop's counter, covers every control the loop configures, as one entry.

IT NEVER GUESSES. A //? line it cannot attach, or a control it cannot turn into a number, is
reported with its file and line, and left out.
"""
import argparse
import json
import os
import re
import sys

CALL = re.compile(r'\b(configParam|configSwitch|configButton|configInput|configOutput|configLight)'
                  r'\s*(?:<[^>]*>)?\s*\(')
KIND = {'configParam': 'params', 'configSwitch': 'params', 'configButton': 'params',
        'configInput': 'inputs', 'configOutput': 'outputs', 'configLight': 'lights'}
ENUM_KIND = [('params', re.compile(r'param', re.I)), ('inputs', re.compile(r'input', re.I)),
             ('outputs', re.compile(r'output', re.I)), ('lights', re.compile(r'light', re.I))]
STRUCT = re.compile(r'\bstruct\s+(\w+)\s*(?:final\s*)?:\s*(?:public\s+)?(?:rack::)?(?:engine::)?'
                    r'Module\b')
OUTSIDE_CTOR = re.compile(r'\b(\w+)::\1\s*\(')
ENUM = re.compile(r'\benum\s+(?:class\s+)?(\w+)\s*(?::\s*\w+\s*)?\{(.*?)\}', re.S)
MODEL = re.compile(r'createModel\s*<\s*([\w:]+)\s*,\s*[\w:]+\s*>\s*\(\s*"([^"]+)"')
CONST = re.compile(r'(?:#define\s+(\w+)\s+(\d+)\b|\b(?:static\s+)?(?:constexpr|const)\s+'
                   r'(?:unsigned\s+|signed\s+)?(?:int|size_t|short|long)\s+(\w+)\s*=\s*(\d+)\s*;)')
LOOP = re.compile(r'\bfor\s*\(\s*(?:int|size_t|unsigned|auto)?\s*(\w+)\s*=\s*(\w+)\s*;\s*\1\s*'
                  r'(<=|<)\s*([\w:]+)')
STRING = re.compile(r'"((?:[^"\\]|\\.)*)"')


class Problem(Exception):
    pass


def strip_comments(text):
    """The text with comments blanked out, the same length, so positions still line up."""
    out = []
    i = 0
    n = len(text)
    while i < n:
        if text.startswith('//', i):
            j = text.find('\n', i)
            j = n if j < 0 else j
            out.append(' ' * (j - i))
            i = j
        elif text.startswith('/*', i):
            j = text.find('*/', i + 2)
            j = n if j < 0 else j + 2
            out.append(re.sub(r'[^\n]', ' ', text[i:j]))
            i = j
        elif text[i] == '"':
            m = STRING.match(text, i)
            j = m.end() if m else i + 1
            out.append(text[i:j])
            i = j
        else:
            out.append(text[i])
            i += 1
    return ''.join(out)


def line_of(text, pos):
    return text.count('\n', 0, pos) + 1


def number(expr, consts):
    """An integer written as a number, a named constant, or simple arithmetic on those —
    `3 * 8`, `NUM_STEPS + 1` — as enums and loops write them. Anything else is reported."""
    expr = expr.strip()
    if re.fullmatch(r'\d+', expr):
        return int(expr)
    if expr in consts:
        return consts[expr]
    known = re.sub(r'\b[A-Za-z_]\w*\b',
                   lambda m: str(consts[m.group(0)]) if m.group(0) in consts else m.group(0), expr)
    if re.fullmatch(r'[\d\s+\-*/()]+', known):
        try:
            return int(eval(known.replace('/', '//'), {'__builtins__': {}}))
        except (SyntaxError, ZeroDivisionError, TypeError):
            pass
    raise Problem('cannot work out the value of "%s"' % expr)


def parse_enum(body, consts):
    """Names to numbers, and ENUMS runs to (first, count), counting as the compiler does."""
    members, runs = {}, {}
    at = 0
    parts, depth, cur = [], 0, ''
    for c in body:
        if c == '(':
            depth += 1
        elif c == ')':
            depth -= 1
        if c == ',' and depth == 0:
            parts.append(cur)
            cur = ''
        else:
            cur += c
    parts.append(cur)
    for part in parts:
        part = part.strip()
        if not part:
            continue
        m = re.fullmatch(r'ENUMS\s*\(\s*(\w+)\s*,\s*(.+?)\s*\)', part, re.S)
        if m:
            count = number(m.group(2), consts)
            runs[m.group(1)] = (at, count)
            members[m.group(1)] = at
            at += count
            continue
        m = re.fullmatch(r'(\w+)\s*(?:=\s*(.+))?', part, re.S)
        if not m:
            raise Problem('cannot read the enum entry "%s"' % part)
        if m.group(2):
            value = m.group(2).strip()
            at = members[value] if value in members else number(value, consts)
        members[m.group(1)] = at
        at += 1
    return members, runs


def call_end(clean, start):
    """Where the call opened at `start` (the parenthesis) closes, and the text of its arguments."""
    depth = 0
    for i in range(start, len(clean)):
        c = clean[i]
        if c == '(':
            depth += 1
        elif c == ')':
            depth -= 1
            if depth == 0:
                return i, clean[start + 1:i]
    raise Problem('a configure call that never closes')


def first_argument(args):
    depth = 0
    for i, c in enumerate(args):
        if c in '([{<':
            depth += 1
        elif c in ')]}>':
            depth -= 1
        elif c == ',' and depth == 0:
            return args[:i]
    return args


class Module:
    def __init__(self, struct):
        self.struct = struct
        self.enums = {}        # kind -> (members, runs)
        self.description = []
        self.notes = []
        self.controls = {'params': [], 'inputs': [], 'outputs': [], 'lights': []}


def scan(path, modules, problems):
    text = open(path, encoding='utf-8', errors='replace').read()
    clean = strip_comments(text)
    lines = text.split('\n')

    consts = {}
    for m in CONST.finditer(clean):
        if m.group(1):
            consts[m.group(1)] = int(m.group(2))
        else:
            consts[m.group(3)] = int(m.group(4))

    # WHERE EACH MODULE'S CODE BEGINS, so a call and a marker can be given to the right one.
    starts = []
    for m in STRUCT.finditer(clean):
        starts.append((m.start(), m.group(1)))
    for m in OUTSIDE_CTOR.finditer(clean):
        starts.append((m.start(), m.group(1)))
    starts.sort()
    for _, name in starts:
        modules.setdefault(name, Module(name))

    # THE ENUMS, given to the module whose code they are in.
    for m in ENUM.finditer(clean):
        owner = owner_at(starts, m.start())
        if not owner:
            continue
        for kind, pattern in ENUM_KIND:
            if pattern.search(m.group(1)):
                try:
                    modules[owner].enums[kind] = parse_enum(m.group(2), consts)
                except Problem as e:
                    problems.append('%s:%d: %s' % (path, line_of(clean, m.start()), e))
                break

    # THE CALLS, and where each ends, by line.
    calls = {}
    for m in CALL.finditer(clean):
        try:
            close, args = call_end(clean, m.end() - 1)
        except Problem as e:
            problems.append('%s:%d: %s' % (path, line_of(clean, m.start()), e))
            continue
        calls[line_of(clean, close)] = (m.group(1), args, line_of(clean, m.start()), m.start())

    # THE MARKERS, line by line.
    target = None        # what the next //? line continues: ('control', entry) or a list
    pending = []         # module-level blocks waiting for the module that follows them
    last_call = None     # the call whose help may follow, and until which line
    for number_, raw in enumerate(lines, 1):
        stripped = raw.strip()
        if number_ in calls:
            last_call = calls[number_]
            target = None
            continue
        if stripped.startswith('//?'):
            body = stripped[3:]
            if body.startswith('module') and (len(body) == 6 or not body[6].isalnum()):
                block = ['description', [body[6:].strip()], number_]
                pending.append(block)
                target = block[1]
                last_call = None
                continue
            if body.startswith('note') and (len(body) == 4 or not body[4].isalnum()):
                block = ['note', [body[4:].strip()], number_]
                pending.append(block)
                target = block[1]
                last_call = None
                continue
            body = body.strip()
            if target is not None:
                target.append(body)
                continue
            if last_call:
                entry = attach(path, last_call, clean, starts, modules, problems)
                last_call = None
                if entry is not None:
                    entry['text'].append(body)
                    target = entry['text']
                continue
            problems.append('%s:%d: a //? line with no configure call above it' % (path, number_))
            continue
        if stripped == '' or (stripped.startswith('//') and not stripped.startswith('//?')):
            continue
        # ANY OTHER CODE ENDS WHAT A //? LINE COULD CONTINUE.
        target = None
        last_call = None

    # MODULE-LEVEL BLOCKS go to the module whose code they are directly above, with nothing but
    # comments and blank lines between; otherwise to the module whose code they are inside; and
    # before any module's code, to the first.
    for kind, texts, at in pending:
        pos = sum(len(l) + 1 for l in lines[:at - 1])
        after = next(((start, name) for start, name in starts if start > pos), None)
        owner = None
        if after and clean[pos:after[0]].strip() == '':
            owner = after[1]
        if not owner:
            owner = owner_at(starts, pos) or (starts[0][1] if starts else None)
        if not owner:
            problems.append('%s:%d: a //?%s with no module in this file' % (path, at, kind if kind == 'note' else 'module'))
            continue
        text = ' '.join(t for t in texts if t)
        if kind == 'description':
            modules[owner].description.append(text)
        else:
            modules[owner].notes.append(text)
    return consts


def owner_at(starts, pos):
    owner = None
    for start, name in starts:
        if start <= pos:
            owner = name
    return owner


def attach(path, call, clean, starts, modules, problems):
    """The entry a call's help goes into, with the control numbers it covers, or None."""
    func, args, line, pos = call
    owner = owner_at(starts, pos)
    if not owner:
        problems.append('%s:%d: %s outside any module' % (path, line, func))
        return None
    mod = modules[owner]
    kind = KIND[func]
    first = first_argument(args).strip()
    first = re.sub(r'\b\w+::', '', first)
    m = re.fullmatch(r'(\w+)\s*(?:\+\s*(\w+))?', first)
    if not m:
        problems.append('%s:%d: cannot tell which control "%s" is' % (path, line, first))
        return None
    name, counter = m.group(1), m.group(2)
    members, runs = mod.enums.get(kind, ({}, {}))
    if name not in members:
        problems.append('%s:%d: "%s" is not in %s\'s list of %s' % (path, line, name, owner, kind))
        return None
    if counter is None:
        ids = [members[name]]
    elif name in runs:
        base, count = runs[name]
        ids = list(range(base, base + count))
    else:
        # A NAME PLUS A LOOP'S COUNTER: the loop just above says how many.
        back = clean[max(0, pos - 400):pos]
        loops = [l for l in LOOP.finditer(back) if l.group(1) == counter]
        if not loops:
            problems.append('%s:%d: "%s + %s" with no loop over %s just above it'
                            % (path, line, name, counter, counter))
            return None
        loop = loops[-1]
        try:
            lo = number(loop.group(2), {})
            hi = number(re.sub(r'\b\w+::', '', loop.group(4)), runs_and_consts(mod, clean))
        except Problem as e:
            problems.append('%s:%d: %s' % (path, line, e))
            return None
        if loop.group(3) == '<=':
            hi += 1
        ids = [members[name] + i for i in range(lo, hi)]
    label = STRING.search(args)
    entry = {'ids': ids, 'text': []}
    if label and not counter:
        entry['name'] = label.group(1)
    mod.controls[kind].append(entry)
    return entry


def runs_and_consts(mod, clean):
    out = {}
    for m in CONST.finditer(clean):
        if m.group(1):
            out[m.group(1)] = int(m.group(2))
        else:
            out[m.group(3)] = int(m.group(4))
    return out


def dump(doc):
    """The same layout as DreamerHelp's own files: one entry of a list to a line."""
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
        else:
            out.append('  %s: %s%s' % (one(key), one(value), comma))
    out.append('}')
    return '\n'.join(out) + '\n'


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('files', nargs='+', help='the plugin\'s source files')
    ap.add_argument('--out', default='help', help='the folder to write into (default: help)')
    ap.add_argument('--plugin', help='the plugin slug (default: read from plugin.json)')
    ap.add_argument('--check', action='store_true', help='report only; write nothing')
    opts = ap.parse_args()

    plugin = opts.plugin
    if not plugin and os.path.isfile('plugin.json'):
        plugin = json.load(open('plugin.json')).get('slug')
    if not plugin:
        raise SystemExit('no plugin slug: run from the plugin folder, or pass --plugin')

    modules, problems, slugs = {}, [], {}
    for path in opts.files:
        scan(path, modules, problems)
        text = strip_comments(open(path, encoding='utf-8', errors='replace').read())
        for m in MODEL.finditer(text):
            slugs[m.group(1).split('::')[-1]] = m.group(2)

    written = 0
    for struct, mod in sorted(modules.items()):
        has = mod.description or mod.notes or any(mod.controls.values())
        if not has:
            continue
        slug = slugs.get(struct)
        if not slug:
            problems.append('%s has help but no createModel naming its slug' % struct)
            continue
        doc = {'plugin': plugin, 'module': slug}
        if mod.description:
            doc['description'] = ' '.join(mod.description)
        for kind in ('params', 'inputs', 'outputs', 'lights'):
            entries = []
            for e in mod.controls[kind]:
                text = ' '.join(t for t in e['text'] if t)
                if not text:
                    continue
                entry = {'ids': e['ids']}
                if e.get('name'):
                    entry['name'] = e['name']
                entry['text'] = text
                entries.append(entry)
            if entries:
                doc[kind] = entries
        if mod.notes:
            doc['notes'] = mod.notes
        target = os.path.join(opts.out, slug + '.json')
        if opts.check:
            print('would write %s' % target)
        else:
            os.makedirs(opts.out, exist_ok=True)
            with open(target, 'w', encoding='utf-8') as f:
                f.write(dump(doc))
            print('wrote %s' % target)
        written += 1

    for p in problems:
        print(p, file=sys.stderr)
    print('%d module%s, %d problem%s' % (written, '' if written == 1 else 's',
                                        len(problems), '' if len(problems) == 1 else 's'),
          file=sys.stderr)
    return 1 if problems else 0


if __name__ == '__main__':
    sys.exit(main())
