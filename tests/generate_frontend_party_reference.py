#!/usr/bin/env python3
"""Extract captured and current party callers for an owned native comparison."""
import argparse
import hashlib
import json
from pathlib import Path


def definition(text, token):
    position = text.index(token)
    start = text.rfind('\n', 0, position) + 1
    end = text.index('{', position) + 1
    depth = 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--reference', type=Path, required=True)
    parser.add_argument('--current', type=Path, required=True)
    parser.add_argument('--before', type=Path, required=True)
    parser.add_argument('--after', type=Path, required=True)
    args = parser.parse_args()
    path = args.reference / 'frontend_party.cpp.reference'
    digest = hashlib.sha256(path.read_bytes()).hexdigest()
    provenance = json.loads((args.reference / 'provenance.json').read_text())
    if provenance['source_pins']['src/ui/frontend_party.cpp'] != digest:
        raise ValueError('Captured native party caller differs from its pinned input')
    methods = ['MenuState::invite_notice()', 'MenuState::online_party_members()', 'MenuState::online_party_column(']
    old = [definition(path.read_text(), token) for token in ['frontend_party_notification_sounds(', *methods]]
    current = [definition(args.current.read_text(), token) for token in methods]
    for target, source, bodies in [(args.before, path, old), (args.after, args.current, current)]:
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text('// Native source SHA256 ' + hashlib.sha256(source.read_bytes()).hexdigest() + '\n' + '\n'.join(bodies) + '\n')


if __name__ == '__main__':
    main()
