#!/usr/bin/env python3
"""Differential replay of preserved C++ and Rust accepted-state parsers."""
import argparse
from collections import Counter
import copy
import hashlib
import json
import os
from pathlib import Path
import subprocess

HANDLE = '00000000-0000-0000-0000-000000000001'
OTHER = '00000000-0000-0000-0000-000000000002'


def corpus():
    member = dict(handle=HANDLE, name='Pilot', ready_playlist='', self=True, leader=True, online=True)
    party = dict(party=dict(version=7, leader=True, members=[member], outgoing=[]), invitations=[])
    extended = copy.deepcopy(party)
    extended['party'].update(countdown=None, privacy=dict(mode=0, revision=1, context=HANDLE))
    extended['party']['members'][0].update(appearance='000102030100000000', postgame=True,
                                         local_players=[dict(name='Guest', guest=True, appearance=None)])
    follower = copy.deepcopy(extended)
    follower['party']['leader'] = False
    follower['party']['members'][0]['leader'] = False
    follower['party']['members'].append(dict(member, handle=OTHER, name='Leader', self=False, leader=True, postgame=False))
    follower['party']['countdown'] = dict(serial=2, seconds=5, mode=1, complete=False, requester=HANDLE)
    clan = dict(clan=dict(id='legacy-clan', name='Clan', role=3,
                         members=[dict(id='legacy-member', name='Pilot', role=3, self=True)],
                         outgoing=[dict(id='invited', name='Invited')], messages=[dict(id='message', name='Pilot', body='Hello')]),
                invitations=[dict(id='incoming', name='Other Clan')])
    friends = dict(friend_tag='Pilot#12345678', friends=[dict(id=OTHER, name='Friend', friend_tag='Friend#87654321',
                                                           presence='online', state='friend', party_joinable=True, party_context=OTHER)])
    bases = [('party', party), ('party', extended), ('party', follower), ('party', dict(party=None, invitations=[])),
             ('social', clan), ('social', dict(clan=None, invitations=[])), ('friends', friends),
             ('friends', dict(friend_tag='Pilot#12345678', friends=[], search_result=dict(id=OTHER, name='Found', friend_tag='Found#87654321', presence='offline')))]
    for presence in ['offline', 'online', 'playing']:
        value = copy.deepcopy(clan)
        value['clan']['members'][0]['presence'] = presence
        bases.append(('social', value))
    for relationship in ['friend', 'incoming', 'outgoing', 'blocked']:
        for presence in ['offline', 'online', 'playing']:
            value = copy.deepcopy(friends)
            value['friends'][0].update(state=relationship, presence=presence, party_joinable=False, party_context=None)
            bases.append(('friends', value))
    for mode in [0, 1]:
        for complete in [False, True]:
            bases.append(('countdown', dict(serial=1, seconds=0 if complete else 10, mode=mode, complete=complete,
                                             requester=HANDLE if mode else None)))
    bases += [('countdown', None), ('countdown', dict(serial=2**63-1, seconds=None, mode=0, complete=True, requester=None))]
    large_party = copy.deepcopy(extended)
    large_party['party']['members'] += [dict(member, handle=f'{i:036}', name=f'Peer {i}', self=False, leader=False) for i in range(2, 17)]
    bases.append(('party', large_party))
    large_clan = copy.deepcopy(clan)
    large_clan['clan']['members'] += [dict(id=f'{i:036}', name=f'Member {i}', role=0, self=False) for i in range(2, 101)]
    bases.append(('social', large_clan))
    large_friends = dict(friend_tag='Pilot#12345678', friends=[dict(id=f'{i:036}', name=f'Friend {i}',
                         friend_tag=f'Friend {i}#12345678', presence='online', state='friend') for i in range(250)])
    bases.append(('friends', large_friends))

    def paths(value, path=()):
        yield path
        if isinstance(value, dict):
            for key, child in value.items():
                yield from paths(child, path+(key,))
        elif isinstance(value, list):
            for i, child in enumerate(value):
                yield from paths(child, path+(i,))

    def replace(value, path, new):
        if not path:
            return copy.deepcopy(new)
        value = copy.deepcopy(value)
        parent = value
        for key in path[:-1]:
            parent = parent[key]
        parent[path[-1]] = copy.deepcopy(new)
        return value

    replacements = [None, False, True, -1, 0, 1, 2, 3, 10, 11, 16, 17, 2**63-1, 2**63, 2**64-1,
                    1.0, '1', '', 'x', '\x00', '\x7f', [], {}, 'é'*33, 'x'*129]
    seen = set()

    def emit(kind, root, mutation):
        row = dict(kind=kind, root=root)
        encoded = json.dumps(row, sort_keys=True, separators=(',', ':'), ensure_ascii=False)
        if encoded not in seen:
            seen.add(encoded)
            return dict(input=row, mutation=mutation)

    for base_index, (kind, base) in enumerate(bases):
        row = emit(kind, base, f'base-{base_index}')
        if row:
            yield row
        # Full-capacity valid cases stay in the corpus; mutate bounded small
        # cases exhaustively without producing a quadratic duplicate bank.
        if base_index >= len(bases)-3:
            continue
        for path in paths(base):
            current = base
            for key in path:
                current = current[key]
            for replacement in replacements:
                row = emit(kind, replace(base, path, replacement), 'type-or-boundary')
                if row:
                    yield row
            if isinstance(current, dict):
                for key in current:
                    changed = copy.deepcopy(current)
                    del changed[key]
                    row = emit(kind, replace(base, path, changed), 'missing-field')
                    if row:
                        yield row
                row = emit(kind, replace(base, path, dict(current, unknown=True)), 'extra-field')
                if row:
                    yield row
            if isinstance(current, list) and current:
                for count in [2, 3, 4, 16, 17, 20, 21, 100, 101, 250, 251]:
                    repeated = [copy.deepcopy(current[0]) for _ in range(count)]
                    row = emit(kind, replace(base, path, repeated), 'duplicate-or-count-boundary')
                    if row:
                        yield row


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--reference', type=Path, required=True)
    parser.add_argument('--rust-source', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    report = {'passed': False, 'scope': 'Modern C++ accepted-state compatibility; not Xbox Live or original algorithm proof',
              'comparison': 'acceptance and sanitized values; parser-library error text excluded'}
    try:
        cargo = ['cargo', 'build', '--locked', '--offline', '--manifest-path', str(args.rust_source/'Cargo.toml'),
                 '--example', 'account_snapshot_replay']
        env = dict(os.environ, CARGO_TARGET_DIR=str(scratch/'cargo'), CARGO_BUILD_JOBS='2',
                   RUSTUP_TOOLCHAIN='1.96.0', RUSTUP_AUTO_INSTALL='0')
        build = subprocess.run(cargo, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=150)
        report['build_exit_code'] = build.returncode
        assert build.returncode == 0, build.stdout.decode(errors='replace')[-5000:]
        rows = list(corpus())
        inputs = ''.join(json.dumps(r['input'], separators=(',', ':'), ensure_ascii=False)+'\n' for r in rows).encode()
        report['cases'] = len(rows)
        report['input_sha256'] = hashlib.sha256(inputs).hexdigest()
        report['kinds'] = dict(Counter(r['input']['kind'] for r in rows))
        report['mutations'] = dict(Counter(r['mutation'] for r in rows))
        answers = []
        rust_binary = scratch/'cargo/debug/examples'/('account_snapshot_replay.exe' if os.name == 'nt' else 'account_snapshot_replay')
        for name, binary in [('cpp', args.reference), ('rust', rust_binary)]:
            run = subprocess.run([str(binary)], input=inputs, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=90)
            assert run.returncode == 0, f'{name} replay failed: {run.stderr.decode(errors="replace")[-2000:]}'
            lines = run.stdout.splitlines()
            parsed = [json.loads(line) for line in lines if line.startswith(b'{')]
            assert len(parsed) == len(rows), f'{name} reply count differed'
            report[name] = dict(binary_sha256=hashlib.sha256(binary.read_bytes()).hexdigest(),
                                output_sha256=hashlib.sha256(run.stdout).hexdigest(),
                                native_preamble=[line.decode(errors='replace')[:256] for line in lines if not line.startswith(b'{')][:8],
                                accepted=sum(r['accepted'] for r in parsed), rejected=sum(not r['accepted'] for r in parsed))
            answers.append(parsed)
        mismatches = [i for i, pair in enumerate(zip(*answers)) if pair[0] != pair[1]]
        report['mismatches'] = len(mismatches)
        report['first_mismatches'] = [dict(index=i, input=rows[i]['input'], cpp=answers[0][i], rust=answers[1][i]) for i in mismatches[:5]]
        assert not mismatches, f'{len(mismatches)} parser mismatches; see bounded report'
        report['passed'] = True
    finally:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        encoded = json.dumps(report, indent=2).encode()
        assert len(encoded) <= 1048576
        args.output.write_bytes(encoded+b'\n')
    print(f'PASS C++/Rust accepted-state replay: {len(rows)} cases, {report["cpp"]["accepted"]} accepted, {report["cpp"]["rejected"]} rejected')


if __name__ == '__main__':
    main()
