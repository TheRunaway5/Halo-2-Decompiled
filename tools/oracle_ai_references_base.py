"""Whole base-retail squad initialization and packed AI reference iteration."""
from pathlib import Path
import argparse
import hashlib
import json
import random
import struct
import sys

from tag_match_memory_budget import enter_memory_budget

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output-dir', type=Path, required=True)
parser.add_argument('--fixture', type=Path)
args = parser.parse_args()
contained = enter_memory_budget(args.output_dir, 60)
if contained is not None:
    raise SystemExit(contained)
root = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(root / 'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import *

x = Xbe(Path('/home/eric/.local/share/halo2-native/cache-reset-backups/20261012T201413Z/assets/disc/default.xbe'))
pin = hashlib.sha256(x.data).hexdigest()
assert pin == '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
g = Uc(UC_ARCH_X86, UC_MODE_32)
g.mem_map(0x10000, 0x600000)
for section in x.sections:
    g.mem_write(section.va, x.section_bytes(section))
g.mem_map(0x1000000, 0x200000)
g.mem_map(0x2000000, 0x10000)
SQUADS, GROUPS, ACTORS = 0x1000000, 0x1000100, 0x1000200
SQUAD_ROWS, GROUP_ROWS, ACTOR_ROWS = 0x1001000, 0x1010000, 0x1020000
SCENARIO, SQUAD_DEFS, GROUP_DEFS = 0x10b0000, 0x10b1000, 0x10c1000
STARTS, GLOBALS, ITERATOR = 0x10d0000, 0x10d8000, 0x10d8100
PALETTE, TAGS, CHARACTER, UNIT, CREATURE, PARENT, GENERAL = [0x10e0000 + i*0x1000 for i in range(7)]
STACK, STOP = 0x200ff00, 0x2000000

def write(address, value, size=4):
    g.mem_write(address, (value & ((1 << (size*8))-1)).to_bytes(size, 'little'))

def read(address, size=4):
    return int.from_bytes(g.mem_read(address, size), 'little')

def call(address, registers):
    write(STACK, STOP)
    g.reg_write(UC_X86_REG_ESP, STACK)
    for register, value in registers.items():
        g.reg_write(register, value)
    g.emu_start(address, STOP, count=2000000)
    assert g.reg_read(UC_X86_REG_EIP) == STOP, hex(g.reg_read(UC_X86_REG_EIP))
    return g.reg_read(UC_X86_REG_EAX)

def pool(address, name, capacity, stride, rows, bitmap):
    g.mem_write(address, bytes(0x4c))
    g.mem_write(address, name.encode() + b'\0')
    write(address+0x20, capacity)
    write(address+0x24, stride)
    write(address+0x29, 1, 1)
    write(address+0x44, rows)
    write(address+0x48, bitmap)
    call(0x16b7a0, {UC_X86_REG_ESI: address})

for address, value in [(0x51e9d8, SQUADS), (0x51e9dc, GROUPS),
                       (0x4f55f0, ACTORS), (0x4e0350, SCENARIO),
                       (0x4f55d0, GLOBALS), (0x4e3b44, TAGS)]:
    write(address, value)

rng = random.Random(0x272b702009d0)
cases = []
for case in range(40):
    pool(SQUADS, 'squad', 335, 0x98, SQUAD_ROWS, 0x10a0000)
    pool(GROUPS, 'squad group', 100, 0x38, GROUP_ROWS, 0x10a0100)
    pool(ACTORS, 'actor', 256, 0x888, ACTOR_ROWS, 0x10a0200)
    g.mem_write(SCENARIO, bytes(0x200))
    g.mem_write(GLOBALS, bytes(0x100))
    write(GLOBALS+1, 1, 1)
    write(GLOBALS+0x14, 0xffffffff)
    group_count = 6 if case < 38 else (100 if case == 38 else 0)
    squad_count = 9 if case < 38 else (335 if case == 38 else 0)
    parents = [-1 if i == 0 or rng.randrange(5) == 0 else rng.randrange(i)
               for i in range(group_count)]
    if case == 0:
        parents = [-1, 0, 0, 1, 1, 2]
    elif case % 2:
        order = list(range(group_count))
        rng.shuffle(order)
        permuted = [-1]*group_count
        for i, parent in enumerate(parents):
            permuted[order[i]] = -1 if parent == -1 else order[parent]
        parents = permuted
    write(SCENARIO+0x158, group_count)
    write(SCENARIO+0x15c, GROUP_DEFS)
    write(SCENARIO+0x160, squad_count)
    write(SCENARIO+0x164, SQUAD_DEFS)
    # A real-shaped character palette: exercise authored teams, unit before
    # creature, inherited general type fallback, and absent definitions.
    write(SCENARIO+0x178, 1)
    write(SCENARIO+0x17c, PALETTE)
    write(PALETTE+4, 0xe1000000 if case % 7 != 0 else 0xffffffff)
    for i, address in enumerate((CHARACTER, UNIT, CREATURE, PARENT)):
        write(TAGS+i*16+8, address)
    g.mem_write(CHARACTER, bytes(0x40))
    g.mem_write(PARENT, bytes(0x40))
    write(CHARACTER+8, 0xe1000003 if case % 2 else 0xffffffff)
    write(PARENT+8, 0xffffffff)
    write(PARENT+0x10, 0xe1000001)
    write(CHARACTER+0x10, 0xe1000001 if case % 3 else 0xffffffff)
    write(CHARACTER+0x18, 0xe1000002 if case % 4 else 0xffffffff)
    write(UNIT+0xc0, (case % 9) if case % 5 else 0, 1)
    write(CREATURE+0xc0, (case % 8) if case % 6 else 0, 1)
    general_owner = PARENT if case % 2 else CHARACTER
    write(general_owner+0x34, 1 if case % 11 else 0)
    write(general_owner+0x38, GENERAL)
    actor_type = (-1, 0, 6, 14, 19, 20)[case % 6]
    write(GENERAL+4, actor_type, 2)
    squads = []
    for i in range(group_count):
        g.mem_write(GROUP_DEFS+i*0x24, bytes(0x24))
        write(GROUP_DEFS+i*0x24+0x20, parents[i], 2)
    for i in range(squad_count):
        address = SQUAD_DEFS+i*0x74
        g.mem_write(address, bytes(0x74))
        parent = (i % group_count) if case == 0 else rng.randrange(-1, group_count)
        flags = rng.choice((0, 0x100, 0x200, 0x300, 0xffffffff))
        authored_team = rng.choice((0, 0, 0, 1, 3, 7, 0xff))
        write(address+0x20, flags)
        write(address+0x24, authored_team, 1)
        write(address+0x26, parent, 2)
        write(address+0x36, 0 if i % 7 else -1, 2)
        names = [0x01000000 + i*4 + j//2 for j in range(3)]
        # Three authored entries; duplicate SIDs distinguish name matching
        # from starting-location ordinals.
        start_address = STARTS+i*0x12c
        # Only the first nine squads need starts; the capacity boundary has
        # none, keeping this owner independent of unrelated scenario storage.
        if case >= 38:
            names = []
        write(address+0x48, len(names))
        write(address+0x4c, start_address)
        for j, name in enumerate(names):
            g.mem_write(start_address+j*0x64, bytes(0x64))
            write(start_address+j*0x64, name)
        squads.append(dict(flags=flags, parent=parent,
                           authored_team=authored_team, starting_locations=names))
    # Whole initialization: original reset/allocation/constructors/tree calls.
    call(0x200b30, {})
    for i, squad in enumerate(squads):
        squad['resolved_team'] = read(SQUAD_ROWS+i*0x98+0x76, 1)
        character = read(PALETTE+4) if i % 7 else 0xffffffff
        local_team, general_type = 0, -1
        if character != 0xffffffff:
            definition = read(TAGS+(character & 0xffff)*16+8)
            preferred = read(definition+0x10)
            if preferred == 0xffffffff:
                preferred = read(definition+0x18)
            if preferred != 0xffffffff:
                local_team = read(read(TAGS+(preferred & 0xffff)*16+8)+0xc0, 1)
            properties = call(0x1e4990, {UC_X86_REG_ECX: character})
            if properties:
                general_type = int.from_bytes(g.mem_read(properties+4, 2), 'little', signed=True)
        squad['team_inputs'] = dict(authored_team=squad['authored_team'],
                                    local_unit_team=local_team,
                                    general_type=general_type)
    squad_hex = bytes(g.mem_read(SQUAD_ROWS, squad_count*0x98)).hex()
    group_hex = bytes(g.mem_read(GROUP_ROWS, group_count*0x38)).hex()
    actors = []
    if case < 38:
        for i in range(35):
            datum = call(0x16b840, {UC_X86_REG_EDX: ACTORS})
            record = ACTOR_ROWS+(datum & 0xffff)*0x888
            squad = rng.randrange(squad_count)
            name = rng.choice(squads[squad]['starting_locations']+[0x0100ffff])
            unit = 0xe26f0000+i if i % 4 else 0xffffffff
            perception = 0xe1700000+i if i % 7 == 0 else 0xffffffff
            next_actor = read(SQUAD_ROWS+squad*0x98+0x68)
            # Supply already-published owner records; this oracle does not
            # replace or claim the original birth/activation routines.
            write(record+0x18, unit)
            write(record+0x1c, perception)
            write(record+0x20, next_actor)
            write(record+0x30, squad)
            write(record+0x38, name)
            write(record+9, i % 2, 1)  # Iteration must not filter vitality/flags.
            write(SQUAD_ROWS+squad*0x98+0x68, datum)
            actors.append(dict(datum=datum, squad=squad, name=name, unit=unit,
                               perception=perception, next=next_actor,
                               flag9=i % 2))
    queries = []
    references = list(range(squad_count)) + [0x40000000+i for i in range(group_count)]
    if actors:
        references += [0x80000000+i for i in range(len(actors))]
        references += [0xc0000000+(i << 16)+j for i in range(squad_count)
                       for j in (0, 1, 2, 3, 0x8000, 0xffff)]
        references += [0xc0000000+(squad_count << 16), 0xffffffff]
        # Slot35 is zero and unallocated: data_datum_index reads its raw salt.
        references += [0x80000023]
    for active in (True, False):
        write(GLOBALS+1, int(active), 1)
        for reference in references:
            actor = call(0x272b70, {UC_X86_REG_ECX: reference})
            g.mem_write(ITERATOR, bytes(0x30))
            call(0x272d90, {UC_X86_REG_ECX: reference, UC_X86_REG_ESI: ITERATOR})
            result = []
            for _ in range(257):
                pointer = call(0x272e20, {UC_X86_REG_ESI: ITERATOR})
                if not pointer:
                    break
                assert ACTOR_ROWS <= pointer < ACTOR_ROWS+256*0x888
                result.append(read(ITERATOR+8))
            else:
                raise AssertionError('Original actor iterator did not terminate')
            queries.append(dict(reference=reference, active=active,
                                actor=actor, actors=result))
    cases.append(dict(groups=parents, squads=squads, actors=actors,
                      squad_records_hex=squad_hex, group_records_hex=group_hex,
                      queries=queries))

report = dict(passed=True, xbe_sha256=pin,
              scope='Whole200B30/200C20/2009D0/205280/16B7A0/16B840/1E4990 initialization and whole272B70/272D90/272E20/204D30/204D70/204DB0/204E10 reference iteration. Already-published actor/unit/perception links are explicit fixture inputs; birth, AI update and object-list production are separate owners.',
              cases=cases)
(args.output_dir/'ai-references-original.json').write_text(json.dumps(report, indent=2)+'\n')
print(f'PASS {len(cases)} complete original initialization banks and '+
      f'{sum(len(c["queries"]) for c in cases)} original reference queries')
if args.fixture:
    args.fixture.write_text('# Base-retail squad and AI reference golden\n\n'+
        report['scope']+'\nThe constructor API receives the canonical importer resolved team.\n'+
        'This is base CPU evidence; gameplay integration and TU1.5 visuals remain separate.\n\n'+
        'Regenerate with `.tools/python/bin/python tools/oracle_ai_references_base.py` '+
        'using fresh `--output-dir` and this path as `--fixture`.\n\n```json\n'+
        json.dumps(report, indent=2)+'\n```\n')
