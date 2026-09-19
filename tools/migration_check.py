#!/usr/bin/env python3
"""The migration test: does data/help say exactly what the compiled table said?

    python3 tools/migration_check.py path/to/old/HelpText.cpp

Reads the last compiled table and the new tree, and compares, for every module, what the plugin
would show: the module's first line, its notes, its menu settings, the text for every input,
output and parameter, and the phrase saying what each jack expects. They must be identical; a
conversion that quietly dropped or reattached a sentence would be invisible any other way.
"""
import json
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TREE = os.path.join(ROOT, 'data', 'help')
NOTE = 'Note \u2014 '
MENU = 'Menu \u2014 '


def cstring(body):
    out, i = [], 0
    while i < len(body):
        c = body[i]
        if c == '\\':
            n = body[i + 1]
            out.append({'n': '\n', '"': '"', '\\': '\\'}[n])
            i += 2
        else:
            out.append(c)
            i += 1
    return ''.join(out)


STR = r'"((?:[^"\\]|\\.)*)"'


def parse_old(path):
    src = open(path, encoding='utf-8').read()
    pool_block = re.search(r'HELP_PROP_TEXT\[\] = \{\n(.*?)\n\};', src, re.S).group(1)
    pool = [cstring(m) for m in re.findall(STR, pool_block)]
    arrays = {}
    for m in re.finditer(r'static const char\* const (L\d+)\[\] = \{\n(.*?)\};', src, re.S):
        arrays[m.group(1)] = [cstring(x) for x in re.findall(r'^\t' + STR + r',$', m.group(2), re.M)]
    for m in re.finditer(r'static const (?:short|signed char) (\w+)\[\] = \{([^}]*)\};', src):
        arrays[m.group(1)] = [int(x) for x in m.group(2).split(',') if x.strip()]
    table = re.search(r'const HelpEntry HELP\[\] = \{\n(.*?)\n\};', src, re.S).group(1)
    modules = {}
    for row in table.split('\n'):
        m = re.match(r'\t\{' + STR + ', ' + STR + r', (L\d+), \d+, (.*)\},$', row)
        if not m:
            continue
        cells = [c.strip() for c in m.group(4).split(',')]
        names = [cells[i] for i in range(0, len(cells), 2)]
        get = lambda n: arrays.get(n, []) if n != 'NULL' else []
        modules[(cstring(m.group(1)), cstring(m.group(2)))] = {
            'lines': arrays[m.group(3)], 'in': get(names[0]), 'out': get(names[1]),
            'param': get(names[2]), 'pr': get(names[5]), 'po': get(names[6]), 'pool': pool}
    return modules


def shown_old(e):
    """Everything the old build could show for one module, keyed by where it is shown."""
    lines = e['lines']
    got = {'description': lines[0] if lines else '',
           'notes': [l[len(NOTE):] for l in lines[1:] if l.startswith(NOTE)],
           'menu': [l[len(MENU):] for l in lines[1:] if l.startswith(MENU)]}
    for kind, key in (('in', 'inputs'), ('out', 'outputs'), ('param', 'params')):
        for i, at in enumerate(e[kind]):
            if at >= 0:
                got['%s %d' % (key, i)] = lines[at]
    for kind, key in (('pr', 'inputs'), ('po', 'outputs')):
        for i, at in enumerate(e[kind]):
            if at >= 0:
                got['expects %s %d' % (key, i)] = e['pool'][at]
    return got


def shown_new(doc):
    got = {'description': doc.get('description', ''),
           'notes': doc.get('notes', []), 'menu': doc.get('menu', [])}
    for key in ('inputs', 'outputs', 'params'):
        for entry in doc.get(key, []):
            for i in entry['ids']:
                got['%s %d' % (key, i)] = entry['text']
    for key, table in (doc.get('expects') or {}).items():
        for i, text in table.items():
            got['expects %s %s' % (key, i)] = text
    return got


def main():
    if len(sys.argv) != 2:
        raise SystemExit(__doc__)
    old = parse_old(sys.argv[1])
    bad = 0
    compared = 0
    for (plugin, model), e in sorted(old.items()):
        path = os.path.join(TREE, plugin, model + '.json')
        if not os.path.isfile(path):
            print('missing: %s/%s' % (plugin, model))
            bad += 1
            continue
        a, b = shown_old(e), shown_new(json.load(open(path, encoding='utf-8')))
        compared += 1
        if a != b:
            bad += 1
            for k in sorted(set(a) | set(b)):
                if a.get(k) != b.get(k):
                    print('%s/%s %s:\n  old %r\n  new %r' % (plugin, model, k, a.get(k), b.get(k)))
                    break
    extra = sum(len(files) for _, _, files in os.walk(TREE)) - len(old)
    print('%d modules compared, %d differ, %d files in the tree that the table did not have'
          % (compared, bad, extra))
    return 1 if bad or extra else 0


if __name__ == '__main__':
    sys.exit(main())
