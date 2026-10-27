#!/usr/bin/env python3
"""Original TU1.5 start-match host eligibility and deferral controls.

Requires tools/run_test.py ownership. Complete original readiness, connected
team counting, local host-policy test and cached logon-task lookup execute on
synthetic data. No instruction/callee stubs or native implementation imports.
The initial host-leave transaction, policy download/validation and
mode-change publication remain outside these controls.
"""
import argparse
import csv
import hashlib
import json
import os
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path[:0] = [str(ROOT / 'tools'), str(ROOT / 'tools/reference/halo2-decompiled/tools')]
from oracle_game_variants import Source, STACK
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from functions import _Code, _trace
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBP, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDI, UC_X86_REG_EIP, UC_X86_REG_ESI, UC_X86_REG_ESP, UC_X86_REG_EDX
from capstone import Cs, CS_ARCH_X86, CS_MODE_32


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report-output', type=Path, required=True)
    args = parser.parse_args()
    Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    source = Source({'path': str(NEW_DEFAULT), 'pin': NEW_PIN, 'globals': 0x51d048})
    memory = source.g
    state, game, pool, task, blob, owner = 0x1100000, 0x1101000, 0x1120000, 0x1121000, 0x1122000, 0x1123000
    memory.mem_map(state, 0x40000)

    def write(address, value):
        memory.mem_write(address, struct.pack('<I', value & 0xffffffff))

    def byte(address, value):
        memory.mem_write(address, bytes([value]))

    def read_byte(address):
        return bytes(memory.mem_read(address, 1))[0]

    def reset(*, teams=(0, 1), team_required=1, local_check=1, local_flag=0,
              logon=1, debug_policy=0, second_connected=True, online=1, current_peer=0):
        memory.mem_write(state, bytes(0x40000))
        write(game + 0x741c, 5)
        write(game + 0x54, 2)
        write(game + 0x72d8, current_peer)
        write(game + 0xf4, 3)
        write(game + 0xf4 + 0x10c, 3 if second_connected else 0)
        write(game + 0x111c, 3)
        write(game + 0xec, local_flag)
        for index, team in enumerate(teams):
            write(game + 0x112c + index * 0x13c, index)
            byte(game + 0x1244 + index * 0x13c, team & 0xff)
        byte(state + 0x14, 1)  # Initial host-leave attempt already issued.
        write(state + 0x10, 3)
        write(state + 0x18, 1000)
        byte(0x51d048, 1)
        write(0x51d04c, 1000)
        byte(0x4dc5bc, online)
        write(0x4dc03c, debug_policy)
        # A valid completed original logon-task record avoids platform calls.
        write(0x4736a0, 0x10000)
        write(0x4dc05c, pool)
        write(pool + 0x38, 1)
        write(pool + 0x24, 0x40)
        write(pool + 0x44, task)
        write(task, 1)
        write(task + 4, 1)
        write(task + 0x10, logon)
        return team_required, local_check

    cases = [
        ('two_connected_teams', {}, (1, 0)),
        ('one_connected_team', dict(teams=(0, 0)), (0, 1)),
        ('no_valid_second_team', dict(teams=(0, -1)), (0, 1)),
        ('second_team_peer_incomplete', dict(second_connected=False), (0, 1)),
        ('solo_hopper_does_not_require_teams', dict(teams=(0, 0), team_required=0), (1, 0)),
        ('team_check_disabled_by_configuration', dict(teams=(0, 0), debug_policy=1), (1, 0)),
        ('local_host_policy_bit31', dict(local_flag=0x80000000), (0, 1)),
        ('local_policy_requires_successful_logon', dict(local_flag=0x80000000, logon=0), (1, 0)),
        ('local_policy_check_argument_off', dict(local_flag=0x80000000, local_check=0), (1, 0)),
        ('local_check_disabled_by_configuration', dict(local_flag=0x80000000, debug_policy=0x100), (1, 0)),
        ('offline_ignores_online_eligibility_checks', dict(teams=(0, 0), local_flag=0x80000000, online=0), (1, 0)),
        ('current_peer_exception', dict(current_peer=1, second_connected=False), (1, 0)),
    ]
    controls = []
    for label, inputs, expected in cases:
        team_required, local_check = reset(**inputs)
        result = source.call(0x720d0, (state + 0x16, local_check),
                             ((UC_X86_REG_ESI, game), (UC_X86_REG_EDX, state + 0x14),
                              (UC_X86_REG_EDI, state + 0x15), (UC_X86_REG_EAX, team_required))) & 255
        actual = result, read_byte(state + 0x16)
        assert actual == expected, (label, actual, expected)
        controls.append(dict(name=label, inputs=inputs, ready=bool(result), defer=bool(actual[1])))

    # A downloaded per-user policy record's bit1 supplies local policy state;
    # the original parser and binary search execute, with one synthetic key.
    policy_controls = []
    for flags in range(4):
        reset()
        write(blob, 1)
        write(blob + 4, 1)
        write(blob + 8, 0x12345678)
        write(blob + 12, 0xabcdef)
        write(blob + 16, flags)
        write(owner + 0xc20, 0x12345678)
        write(owner + 0xc24, 0xabcdef)
        source.call(0x25dcb6, (0x60008,), ((UC_X86_REG_EAX, blob), (UC_X86_REG_ECX, owner)))
        assert read_byte(0x4da118) == ((flags >> 1) & 1)
        assert read_byte(0x4fadae) == (flags & 1)
        policy_controls.append(dict(record_flags=flags, local_host_policy=read_byte(0x4da118),
                                    other_policy_bit0=read_byte(0x4fadae)))

    def boundary(start, stops, registers):
        hit = []

        def observe(_memory, address, _size, _context):
            if address in stops:
                hit.append(address)
                memory.emu_stop()

        hook = memory.hook_add(UC_HOOK_CODE, observe)
        try:
            for register, value in registers:
                memory.reg_write(register, value)
            memory.emu_start(start, 0x3000000, count=10000)
        finally:
            memory.hook_del(hook)
        assert hit, (hex(start), hex(memory.reg_read(UC_X86_REG_EIP)))
        return hit[0]

    routes = []
    for label, inputs, _expected in cases[:10]:
        team_required, _ = reset(**inputs)
        byte(STACK + 0x12, team_required)
        write(STACK + 0x14, owner)
        hit = boundary(0x7645e, {0x764af, 0x76543}, [(UC_X86_REG_ESP, STACK),
                       (UC_X86_REG_EBP, state), (UC_X86_REG_ESI, game)])
        expected_defer = label in ('one_connected_team', 'no_valid_second_team',
                                   'second_team_peer_incomplete', 'local_host_policy_bit31',
                                   'local_policy_check_argument_off')
        # The real caller always passes local_check=1, overriding the helper-
        # only test's local_check=0 input in the last named case.
        assert (hit == 0x76543) == expected_defer, (label, hex(hit))
        routes.append(dict(name=label, branch=hex(hit),
                           session_mode_request=memory.reg_read(UC_X86_REG_EDI) if hit == 0x76543 else None,
                           start_clock=struct.unpack('<I', memory.mem_read(state + 0x18, 4))[0]))
        if hit == 0x76543:
            assert routes[-1]['session_mode_request'] == 6 and routes[-1]['start_clock'] == 0

    # Follow the actual policy output through the original local property
    # producer, then stage that produced member field for its original reader.
    # Wire transport/membership publication is not substituted or executed.
    policy_chain = []
    for flags in range(4):
        reset(team_required=0)
        write(blob, 1)
        write(blob + 4, 1)
        write(blob + 8, 0x12345678)
        write(blob + 12, 0xabcdef)
        write(blob + 16, flags)
        write(owner + 0xc20, 0x12345678)
        write(owner + 0xc24, 0xabcdef)
        source.call(0x25dcb6, (0x60008,), ((UC_X86_REG_EAX, blob), (UC_X86_REG_ECX, owner)))
        write(0x4dc5c8, 0x1234)
        boundary(0x66a66, {0x66ab8}, [(UC_X86_REG_ESP, STACK)])
        published = struct.unpack('<I', memory.mem_read(STACK + 0x84, 4))[0]
        assert published == 0x1234 | (((flags >> 1) & 1) << 31)
        write(game + 0xec, published)
        ready = source.call(0x720d0, (state + 0x16, 1),
                            ((UC_X86_REG_ESI, game), (UC_X86_REG_EDX, state + 0x14),
                             (UC_X86_REG_EDI, state + 0x15), (UC_X86_REG_EAX, 0))) & 255
        assert ready == int(not (flags & 2))
        policy_chain.append(dict(record_flags=flags, published_member_flags=f'{published:08x}',
                                 ready=bool(ready), defer=bool(read_byte(state + 0x16))))

    # Fresh entry clears +16. It cannot legitimately inherit the apparent
    # instant-start branch from an unrelated prior state allocation.
    reset()
    write(state + 8, owner)
    write(owner + 0x38, game)
    write(game + 0x741c, 0)  # No active session, so original seed helper exits.
    byte(state + 0x16, 0xff)
    source.call(0x765b0, (0, 0, 0), ((UC_X86_REG_ECX, state),))
    assert read_byte(state + 0x16) == 0

    listings = {}
    code = _Code(source.image, '.text')
    with (ROOT / 'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv').open() as inventory:
        starts = {int(row['address'], 16) for row in csv.DictReader(inventory, delimiter='\t')}
    for entry in (0x720d0, 0x721b0, 0x62de0, 0x62d70, 0x25dcb6, 0x669a0):
        body = _trace(code, entry, starts)
        listings[f'{entry:08x}'] = ''.join(
            f'{pc:08x} {code.at(pc).bytes.hex()} {code.at(pc).mnemonic} {code.at(pc).op_str}\n'
            for pc in sorted(body.code))
    report = dict(passed=True, tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  original_xbe_sha256=NEW_PIN, scope=__doc__, readiness_controls=controls,
                  policy_controls=policy_controls, policy_producer_consumer_controls=policy_chain,
                  fresh_entry_clears_defer=True, caller_routes=routes, original_listings=listings,
                  executed_distinct_instructions=len(source.executed - {0x764af, 0x76543, 0x66ab8}),
                  conclusions=[
                      'The flag at start-match state+0x16 is a deferral result, not an ordinary instant-start policy.',
                      'The online host requires two distinct valid player teams on fully-connected peers when its hopper requires teams.',
                      'A successful-logon local peer policy bit31 also causes deferral; the local publisher sources it from per-user policy record bit1.',
                      'A fresh deferral result returns false, clears the start clock and requests B session mode6; mode4 is not reached.',
                      'The deferral check is reachable in online matchmade team hoppers; it cannot be excluded as a custom-game-only path.',
                  ])
    encoded = json.dumps(report, indent=2) + '\n'
    assert len(encoded.encode()) <= 1024 * 1024
    args.report_output.parent.mkdir(parents=True, exist_ok=True)
    args.report_output.write_text(encoded)
    print(json.dumps(dict(passed=True, readiness_controls=len(controls), policy_controls=len(policy_controls),
                          caller_routes=len(routes), report=str(args.report_output))))


if __name__ == '__main__':
    main()
