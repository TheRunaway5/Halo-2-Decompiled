#!/usr/bin/env python3
"""Compare completion CPU state using unchanged TU1.5 code and declared banks.

These are bounded caller-state cases, not a saved Xbox campaign session.
External owners stop before executing: no profile, loader, timer, statistics or
network result is supplied. Every observed store and retained bank is recorded.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

GAME, TIME, ROSTER, POOL, RECORDS, BITS, OPTIONS, NETWORK, SCENARIO = (
    0x2100000, 0x2102000, 0x2103000, 0x2104000, 0x2105000,
    0x210a000, 0x210b000, 0x210d000, 0x2115000)
STACK, STOP = 0x21fff00, 0x3000000
REGS = [getattr(R, 'UC_X86_REG_' + name) for name in ('EAX', 'ECX', 'EDX', 'EBX', 'ESP', 'EBP', 'ESI', 'EDI')]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--recovery', type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists():
        raise ValueError('Preserve existing original execution evidence')
    args.output.mkdir(parents=True)
    image = Xbe(NEW_DEFAULT)
    if hashlib.sha256(image.data).hexdigest() != NEW_PIN:
        raise ValueError('Expected original English TU1.5 executable')
    generation = json.loads((args.recovery / 'generation.json').read_text())
    external = {int(row['target'], 16) for row in generation['external_owners'].values()
                if row['target'].startswith('0x')}
    guest = Uc(UC_ARCH_X86, UC_MODE_32)
    guest.mem_map(0x10000, 0x600000)
    guest.mem_map(0x2100000, 0x100000)
    guest.mem_map(STOP, 4096)
    for section in image.sections:
        guest.mem_write(section.va, image.section_bytes(section))
    rows, stores, boundary, visited = [], [], [], set()

    def instruction(machine, pc, size, unused):
        if pc in external:
            boundary.append(pc)
            machine.emu_stop()
        else:
            visited.add(pc)

    def write(machine, access, address, size, value, unused):
        stores.append(dict(pc=machine.reg_read(R.UC_X86_REG_EIP), address=address,
                           before=bytes(machine.mem_read(address, size)).hex(),
                           written=(value & ((1 << (size * 8)) - 1)).to_bytes(size, 'little').hex()))

    guest.hook_add(UC_HOOK_CODE, instruction)
    guest.hook_add(UC_HOOK_MEM_WRITE, write)

    def case(name, entry, *, game=True, active=1, kind=1, current=7, rate=30,
             finished=0, seen=0, roster=(), network_active=0, network_state=0,
             epoch_match=True, overrides=None, arguments=(0,1,2), string=None):
        banks = {}

        def bank(address, data):
            banks[address] = bytearray(data)

        def word(address, value):
            bank(address, struct.pack('<I', value & 0xffffffff))

        bank(GAME, bytes(0x1200))
        struct.pack_into('<I', banks[GAME], 8, kind)
        struct.pack_into('<I', banks[GAME], 0x20, current & 0xffffffff)
        banks[GAME][0x1120] = active
        banks[GAME][0x1128] = finished
        banks[GAME][0x1129] = 1
        struct.pack_into('<I', banks[GAME], 0x112c, 0x98765432)
        bank(TIME, struct.pack('<HhI', 0, rate, 0))
        bank(ROSTER, bytes((i * 13 + 7) & 255 for i in range(0x100)))
        bank(POOL, bytes(0x50))
        struct.pack_into('<I', banks[POOL], 0x24, 0x200)
        struct.pack_into('<I', banks[POOL], 0x38, 40)
        struct.pack_into('<I', banks[POOL], 0x44, RECORDS)
        struct.pack_into('<I', banks[POOL], 0x48, BITS)
        bank(RECORDS, bytes((i * 7 + 3) & 255 for i in range(40 * 0x200)))
        bitmap = sum(1 << slot for slot in roster)
        bank(BITS, bitmap.to_bytes(8, 'little'))
        bank(OPTIONS, bytes((i * 17 + 11) & 255 for i in range(0x1118)))
        if string is not None:
            text=string.encode('latin1')+b'\0'
            banks[OPTIONS][:len(text)]=text
        bank(NETWORK, bytes(0x7420))
        struct.pack_into('<I', banks[NETWORK], 0x741c, network_state)
        struct.pack_into('<I', banks[NETWORK], 0x50, 0x1234)
        struct.pack_into('<I', banks[NETWORK], 0x72d8, 0x1234 if epoch_match else 0x5678)
        bank(SCENARIO, bytes(0x110))
        bank(0x21fe000, bytes(0x2000))
        struct.pack_into('<4I', banks[0x21fe000], STACK - 0x21fe000, STOP, *arguments)
        word(0x4f3218, GAME if game else 0)
        word(0x51df54, TIME)
        word(0x4f54f0, ROSTER)
        word(0x4f54f4, POOL)
        word(0x4ecc20, SCENARIO)
        word(0x535f34, NETWORK)
        bank(0x535f00, bytes([network_active]))
        bank(0x556e06, bytes([seen]))
        bank(0x46a2b8, image.read(0x46a2b8, 4))
        bank(0x4f9c6d, bytes(0x1123))
        gp = [(i + 1) * 0x11111111 for i in range(8)]
        gp[4] = STACK
        gp[0] = OPTIONS
        for register, value in (overrides or {}).items():
            gp[register] = value
        before = {f'{address:08x}': data.hex() for address, data in banks.items()}
        for address, data in banks.items():
            guest.mem_write(address, bytes(data))
        for register, value in zip(REGS, gp):
            guest.reg_write(register, value)
        xmm = [[(n + 1) * 0x10203 + i for i in range(4)] for n in range(8)]
        for n, words in enumerate(xmm):
            guest.reg_write(getattr(R, 'UC_X86_REG_XMM' + str(n)),
                            int.from_bytes(struct.pack('<4I', *words), 'little'))
        guest.reg_write(R.UC_X86_REG_EFLAGS, 2)
        guest.reg_write(R.UC_X86_REG_FPCW, 0x027f)
        guest.reg_write(R.UC_X86_REG_MXCSR, 0x1f80)
        guest.reg_write(R.UC_X86_REG_FPSW, 0)
        guest.reg_write(R.UC_X86_REG_FPTAG, 0xffff)
        stores.clear()
        boundary.clear()
        guest.emu_start(entry, STOP, count=200000)
        terminal = guest.reg_read(R.UC_X86_REG_EIP)
        if terminal != STOP and boundary != [terminal]:
            raise ValueError('Original completion did not return or reach its declared owner boundary')
        result = dict(name=name, entry=entry, gp=gp, xmm=xmm, before=before,
            after={f'{address:08x}': bytes(guest.mem_read(address, len(data))).hex() for address, data in banks.items()},
            gp_after=[guest.reg_read(r) for r in REGS],
            xmm_after=[list(struct.unpack('<4I', guest.reg_read(getattr(R, 'UC_X86_REG_XMM' + str(n))).to_bytes(16, 'little'))) for n in range(8)],
            eflags_after=guest.reg_read(R.UC_X86_REG_EFLAGS),
            external_boundary=boundary[:], stores=stores[:])
        filename = f'{len(rows):03d}-{name}.json'
        (args.output / filename).write_text(json.dumps(result, indent=2) + '\n')
        rows.append(dict(name=name, file=filename, entry=entry, external_boundary=boundary[:]))

    for name, settings in (
        ('absent-game', dict(game=False)), ('inactive-game', dict(active=0)),
        ('other-game-kind', dict(kind=3)), ('multiplayer-first', dict(kind=2)),
        ('multiplayer-already-finished', dict(kind=2, finished=1)),
        ('campaign-without-map-id', dict(current=-1)), ('campaign-profile-boundary', dict(current=7))):
        case(name, 0x13b3c0, **settings)
    for rate in (15, 24, 30, 60, 120):
        case('finish-timer-' + str(rate), 0x13b4a0, rate=rate)
    case('no-progress-map', 0x193257, overrides={6: 0xffffffff})
    for index, roster in enumerate(((), (0,), (2,), (31, 32, 39), tuple(range(16)), tuple(range(17)), tuple(range(4, 24)))):
        case('transfer-roster-' + str(index), 0x154f80, roster=roster)
    for name, settings in (
        ('statistics-already-seen', dict(seen=1)), ('statistics-no-game', dict(game=False)),
        ('statistics-inactive', dict(active=0)), ('statistics-other-kind', dict(kind=2)),
        ('statistics-owner-boundary', dict())):
        case(name, 0x12e490, **settings)
    case('network-absent', 0x64fd0)
    for state in (0, 1, 2, 3, 8, 9):
        case('network-state-' + str(state), 0x64fd0, network_active=1, network_state=state)
    case('network-epoch-mismatch', 0x64fd0, network_active=1, network_state=3, epoch_match=False)
    case('queue-actual-options', 0x191580)
    case('queue-null-options', 0x191580, overrides={0: 0})
    case('default-transition-alloca-boundary', 0x191fb0)
    for index,(text,needle) in enumerate((('',0),('',92),('basename',92),
        ('scenarios\\solo\\00a_introduction\\00a_introduction',92),
        ('a\\b\\c\\',92),('aaaa',97),('abc',97),('abc',99),('abc',0),
        ('abc',255),('a\xffb\xff',0xab0000ff))):
        case('original-path-search-'+str(index),0x32c0a0,string=text,arguments=(OPTIONS,needle,2))
    report = dict(scope=__doc__, xbe_sha256=NEW_PIN, instruction_patches=[],
                  supplied_external_results=[], external_targets=sorted(external),
                  covered_instructions=sorted(visited), cases=rows,
                  complete_campaign_completion_verified=False)
    (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print('PASS', len(rows), 'unchanged completion caller cases;', len(visited),
          'executed instruction addresses; external owners stop without supplied results')


if __name__ == '__main__':
    main()
