#!/usr/bin/env python3
"""Unchanged TU1.1 team score, ranking and lethal-attribution instructions.

Membership bytes are explicit session inputs, not an invented team allocator.
The salted players are slots 0, 3 and 15. Persistent profile writes are disabled
and replicated engine entities are NONE. The preceding oracle's named session
and event notification leaves remain supplied. Attribution supplies the separate
history/death event writer, then executes the actual Slayer death callback.
No score, ranking, relationship or engine-completion result is supplied.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ESI, UC_X86_REG_ESP
from oracle_mp_spawn import ROOT, VERSIONS, CACHE_PIN, Cache, ENGINE_GLOBALS, PLAYERS, OBJECTS
from oracle_mp_spawn_lifecycle import TIME
from oracle_mp_spawn_slayer_accounting import SlayerOracle, STAT, RANK, NONE

SLOTS = (0, 3, 15)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


class TeamOracle(SlayerOracle):
    def hook(self, g, address, size, user):
        if address == 0x1ed7b0:
            sp = g.reg_read(UC_X86_REG_ESP)
            self.accounting_events.append(dict(kind='original_team_stat_request',
                entry='001ed7b0', field=g.reg_read(UC_X86_REG_EAX),
                team=self.u(sp + 4), delta=self.u(sp + 8)))
        super().hook(g, address, size, user)

    def fixture(self, cache, memberships=(0, 0, 1), scores=(0, 0, 0),
                teams=(0,) * 8, mask=3, scoring=0, limit=0):
        super().fixture(cache, SLOTS)
        self.w(self.options + 0x184, 1)
        self.w(self.options + 0x1e0, scoring)
        self.w(self.options + 0x18c, limit)
        # 241500 enumerates the original retained team mask at globals+0E.
        self.sh(ENGINE_GLOBALS + 0xe, mask)
        for slot, member, score in zip(SLOTS, memberships, scores):
            self.g.mem_write(PLAYERS + slot * 0x21c + 0xc0, bytes([member & 255]))
            self.sh(STAT + 4 + slot * 0x1c, score)
        for team, score in enumerate(teams):
            # The actual LEA team*9 then *2 establishes the 18-byte stride.
            self.sh(STAT + 0x1c4 + team * 0x12, score)

    def team_scores(self):
        return [self.h(STAT + 0x1c4 + team * 0x12) for team in range(8)]

    def player_scores(self):
        return [self.h(STAT + 4 + slot * 0x1c) for slot in SLOTS]


def input_row(members, scores, teams, scoring, limit):
    return dict(memberships=list(members), player_scores=list(scores),
                team_scores=list(teams), scoring=scoring, limit=limit)


def result_row(o):
    ends = [e for e in o.accounting_events if e['kind'] == 'original_end_request']
    return dict(player_scores=o.player_scores(), team_scores=o.team_scores(),
                changed=any(e['kind'] == 'original_team_stat_request' and e['field'] == 0
                    for e in o.accounting_events),
                end=len(ends) != 0, winner=int(ends[0]['winner'], 16) if ends else NONE,
                status=o.u(ENGINE_GLOBALS + 0xc04), events=list(o.accounting_events))


def scores(o, cache):
    rows = []
    for scoring in range(4):
        for start, delta, limit in ((0, 1, 0), (0, -1, 0), (49, 1, 50),
                (49, 3, 50), (51, -1, 50), (25, 65536, 25),
                (26, 65535, 25), (24, -65535, 25), (29999, 10, 30000),
                (-29999, -10, 0), (0, 32768, 0), (0, -32769, 0)):
            members, players, teams = (0, 0, 1), (start, start - 1, 6), (start, 6, 0, 0, 0, 0, 0, 0)
            o.fixture(cache, members, players, teams, scoring=scoring, limit=limit)
            supplied = input_row(members, players, teams, scoring, limit)
            applied = o.call(0x15ce00, (o.datums[0],), ((UC_X86_REG_EAX, delta & NONE),)) & 255
            rows.append(dict(case='player_write', input=supplied, slot=0,
                delta=delta, applied=applied, result=result_row(o)))
    # Original1A2BA0 aggregate scans exclude flag2 and membership NONE.
    for scoring in (0, 1, 2, 3):
        for members, players, flags in (((0, 0, 1), (3, 7, 2), (1, 3, 1)),
                ((0, -1, 1), (3, 99, 2), (1, 1, 1)),
                ((-1, -1, 1), (3, 7, 2), (1, 1, 1))):
            teams = (0,) * 8
            o.fixture(cache, members, players, teams, scoring=scoring, limit=5)
            for slot, flag in zip(SLOTS, flags):o.sh(PLAYERS + slot * 0x21c + 2, flag)
            supplied = input_row(members, players, teams, scoring, 5)
            applied = o.call(0x15ce00, (o.datums[0],), ((UC_X86_REG_EAX, 1),)) & 255
            rows.append(dict(case='member_flags', input=supplied, flags=list(flags),
                slot=0, delta=1, applied=applied, result=result_row(o)))
    # Direct team writer: prospective i32 and stored/clamped i16 can differ.
    for scoring in range(4):
        for start, delta in ((0, 1), (0, -1), (26, 65535), (25, 65536),
                (24, -65535), (29999, 10), (-29999, -10),
                (30000, 2147483647), (-30000, -2147483648)):
            members, players, teams = (0, 0, 1), (3, 7, 2), (start, 2, 0, 0, 0, 0, 0, 0)
            o.fixture(cache, members, players, teams, scoring=scoring, limit=25)
            supplied = input_row(members, players, teams, scoring, 25)
            o.call(0x15cc90, (0, delta))
            rows.append(dict(case='team_write', input=supplied, team=0,
                delta=delta, result=result_row(o)))
    for team in range(8):
        members, players, teams = (team, team, -1), (0, 2, 9), tuple(10 + n for n in range(8))
        o.fixture(cache, members, players, teams, mask=1 << team, limit=0)
        supplied = input_row(members, players, teams, 0, 0)
        applied = o.call(0x15ce00, (o.datums[3],), ((UC_X86_REG_EAX, 1),)) & 255
        rows.append(dict(case='team_index', input=supplied, slot=3, delta=1,
            applied=applied, result=result_row(o)))
    return rows


def rankings(o, cache):
    rows = []
    for mask in (0, 1, 3, 0x80, 0xff):
        for teams in ((0,) * 8, (4, 5, 1, 1, 1, 1, 1, 1),
                (5, 5, 1, 1, 1, 1, 1, 1), (-2, -1, -3, -4, -5, -6, -7, -8),
                (0, 0, 0, 0, 0, 0, 0, 9)):
            o.fixture(cache, teams=teams, mask=mask)
            before = o.engine_state(), o.team_scores()
            o.call(0x241500, (0, 0), ((UC_X86_REG_ESI, RANK),))
            count = o.h(RANK + 0x6a)
            ordered = [o.h(RANK + 0x40 + n * 2) for n in range(count)]
            codes = list(o.g.mem_read(RANK + 0x60, count))
            # No teams: 15C970 reads an unwritten ranking-buffer short. Do
            # not turn residual emulated stack bytes into gameplay authority.
            winner = o.call(0x15c970) if mask else None
            assert (o.engine_state(), o.team_scores()) == before
            rows.append(dict(mask=mask, team_scores=list(teams), winner=winner,
                ordered_teams=ordered, rank_codes=codes))
    return rows


def attribution(o, cache):
    rows = []
    for incoming in (None, 0, 3, 15):
        for hits in ([], [(15, .5, 199, 2)], [(0, .9, 199, 2), (15, .2, 199, 3)],
                     [(15, .9, 199, 2), (0, .5, 199, 3)]):
            o.fixture(cache)
            victim = o.datums[3]
            o.w(OBJECTS + 0x13c, victim)
            o.w(PLAYERS + 3 * 0x21c + 0x2c, 0x10000)
            for slot, amount, tick, cause in hits:o.history_hit(o.datums[slot], amount, tick, cause)
            o.w(TIME + 8, 200);o.skip_writer = True;o.accounting_events = []
            owner = NONE if incoming is None else o.datums[incoming]
            member = NONE if incoming is None else (1 if incoming == 15 else 0)
            history = o.history_state();before = o.engine_state()
            o.call(0x1ed960, (STAT, 0x10000, owner, member, 1))
            writer = next(e for e in o.accounting_events
                if e['kind'] == 'supplied_history_and_death_event_writer_boundary')
            credit_events = list(o.accounting_events);after_credit = o.engine_state()
            o.skip_writer = False;o.accounting_events = []
            o.call(0x2c2350, (int(writer['attacker'], 16), victim, writer['relationship_flag'], NONE))
            rows.append(dict(incoming=owner, incoming_membership=member,
                victim=victim, memberships=[0, 0, 1], history=history,
                before=before, after_credit=after_credit, credit_events=credit_events,
                writer=writer, result=result_row(o)))
    return rows


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output', type=Path, default=ROOT / 'analysis/multiplayer/game-variants/team-slayer-original.json')
    p.add_argument('--fixture', type=Path)
    args = p.parse_args()
    c = Cache(ROOT / 'assets/disc/maps/lockout.map');assert sha(c.path) == CACHE_PIN
    o = TeamOracle()
    score_rows, rank_rows, attribution_rows = scores(o, c), rankings(o, c), attribution(o, c)
    listing = []
    md = Cs(CS_ARCH_X86, CS_MODE_32)
    for start, end in ((0x15cc90, 0x15cf70), (0x1ed7b0, 0x1ed840),
                      (0x15c970, 0x15cc90), (0x241500, 0x241800),
                      (0x241880, 0x241cd0), (0x241e00, 0x241ed0),
                      (0x253f10, 0x253f22), (0x1e2f50, 0x1e2ff0),
                      (0x1a2ba0, 0x1a2c60)):
        listing.append(f'RANGE {start:08x}..{end:08x}')
        listing.extend(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}'
            for i in md.disasm(o.x.read(start, end - start), start))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    listing_path = args.output.with_suffix('.instructions.txt')
    listing_path.write_text('\n'.join(listing) + '\n')
    dependencies = {name:sha(ROOT / name) for name in ('tools/oracle_mp_spawn.py',
        'tools/oracle_mp_spawn_lifecycle.py', 'tools/oracle_mp_spawn_ownership.py',
        'tools/oracle_mp_spawn_slayer_accounting.py')}
    report = dict(scope=__doc__, tool_sha256=sha(__file__), dependencies=dependencies,
        executable_sha256=VERSIONS['tu11']['pin'], cache_sha256=CACHE_PIN,
        instructions_sha256=sha(listing_path), slots=list(SLOTS),
        datums={slot:(7 + 5 * slot) << 16 | slot for slot in SLOTS},
        score_rows=score_rows, rank_rows=rank_rows, attribution_rows=attribution_rows,
        summary=dict(score_rows=len(score_rows), rank_rows=len(rank_rows),
                     attribution_rows=len(attribution_rows)))
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    if args.fixture:
        lines = ['# Unchanged TU1.1 team Slayer instructions', '# xbe ' + VERSIONS['tu11']['pin']]
        for r in score_rows:
            v, out = r['input'], r['result']
            direct = r['case'] == 'team_write'
            lines.append(' '.join(map(str, ['S', int(direct), r.get('team', r.get('slot')),
                v['scoring'], v['limit'], r['delta'], *v['memberships'],
                *r.get('flags', [1, 1, 1]),
                *v['player_scores'], *v['team_scores'], *out['player_scores'],
                *out['team_scores'], int(out['changed']), int(out['end']), out['winner']])))
        for r in rank_rows:
            if r['winner'] is None:continue
            lines.append(' '.join(map(str, ['R', r['mask'], *r['team_scores'], r['winner']])))
        for r in attribution_rows:
            out = r['writer']
            fields = ['A', r['incoming'], r['incoming_membership'], len(r['history'])]
            for h in r['history']:
                fields.extend([int(h['player'], 16), h['timestamp'], int(h['damage_bits'], 16), h['cause']])
            fields.extend([int(out['attacker'], 16), out['relationship_flag'],
                *[r['after_credit']['stats'][str(slot)]['fields'][2] for slot in SLOTS],
                *[r['after_credit']['stats'][str(slot)]['player_counter_1c4'] for slot in SLOTS],
                *[r['after_credit']['stats'][str(slot)]['fields'][1] for slot in SLOTS],
                *[r['after_credit']['stats'][str(slot)]['fields'][8] for slot in SLOTS],
                *r['result']['player_scores'], *r['result']['team_scores']])
            lines.append(' '.join(map(str, fields)))
        args.fixture.parent.mkdir(parents=True, exist_ok=True)
        args.fixture.write_text('\n'.join(lines) + '\n')
    print(json.dumps(report['summary']))


if __name__ == '__main__':main()
