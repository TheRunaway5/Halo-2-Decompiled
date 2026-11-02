#!/usr/bin/env python3
"""Lexical crash-audit candidate census; this does not prove reachability.

Use tools/run_test.py. Runtime/source owner audits must classify the actual
caller exception fence before any site is called an engine crash.
"""
import argparse
from collections import Counter
import hashlib
import json
import os
from pathlib import Path
import re


SITE = re.compile(r'\b(throw\s+[\w:]+|unqualified_\w+_service)\s*\(\s*"((?:\\.|[^"\\])*)"')
KEYWORDS = re.compile(r'preload|admitt|select|actual|qualified|owner|unavailable|unresolved|lifetime|stale|missing', re.I)


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if 'HALO2_TEST_SCRATCH' not in os.environ:
        parser.error('Use tools/run_test.py')
    source = args.source.resolve(strict=True)
    args.output.mkdir(parents=True, exist_ok=True)
    rows, inputs, skipped, areas = [], {}, [], Counter()
    for folder in ('src', 'include/halo2'):
        for path in sorted((source/folder).rglob('*')):
            if path.is_symlink() or not path.is_file() or path.suffix not in ('.cpp', '.hpp', '.inc'):
                continue
            name = path.relative_to(source).as_posix()
            if 'generated' in path.name:
                skipped.append(name)
                continue
            raw = path.read_bytes()
            inputs[name] = sha(raw)
            text = raw.decode(errors='replace')
            area = '/'.join(path.relative_to(source).parts[:2])
            areas[area] += 1
            for match in SITE.finditer(text):
                message = match[2]
                rows.append(dict(file=name, line=text.count('\n', 0, match.start())+1,
                                 mechanism=match[1], message=message,
                                 ownership_candidate=bool(KEYWORDS.search(message)),
                                 classification='unclassified_lexical_site'))
    (args.output/'exception-sites.tsv').write_text('file\tline\tmechanism\tmessage\townership_candidate\n'+''.join(
        '\t'.join((r['file'], str(r['line']), r['mechanism'], r['message'].replace('\t',' '),
                   str(r['ownership_candidate']).lower()))+'\n' for r in rows))
    (args.output/'exception-source-hashes.json').write_text(json.dumps(inputs, indent=2)+'\n')
    report = dict(completed=True, source=str(source), scope=__doc__, helper_sha256=sha(Path(__file__).read_bytes()),
                  files_scanned=len(inputs), files_by_area=dict(areas), generated_files_excluded=skipped,
                  literal_sites=len(rows), ownership_candidates=sum(r['ownership_candidate'] for r in rows),
                  sites_by_area=dict(Counter('/'.join(Path(r['file']).parts[:2]) for r in rows)),
                  reachability_proven=False, caught_vs_escaping_classification_proven=False)
    (args.output/'exception-census.json').write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps({key: report[key] for key in ('completed','files_scanned','literal_sites','ownership_candidates')}))


if __name__ == '__main__':
    main()
