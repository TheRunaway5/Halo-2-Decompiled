#!/usr/bin/env python3
"""Run unchanged lens query instructions on host i386 from declared inputs.

The native implementation's expected outputs are never original inputs.
This checks Unicorn's floating-point limitations against real CPU execution;
it does not establish Xbox GPU or final framebuffer behavior.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from generate_lens_query_tu15 import QueryCode
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT
import oracle_physics_game_frame_hardware_20261011 as hardware


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--input', type=Path, required=True)
    p.add_argument('--bodies', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--fixture', type=Path, required=True)
    a = p.parse_args()
    assert not a.output.exists() and not a.fixture.exists()
    fixture = json.loads(a.input.read_text())
    paired = json.loads(a.bodies.read_text())
    # Authentic unchanged caller returns are harness stops, never patched
    # opcodes. The static initializer has a pointer-table caller and uses the
    # already-owned visibility return solely as a declared ABI stop router.
    stops = {0x2e520:0x20ceb,0x20b90:0x2ded9,0x20750:0x2c2fc,
             0x20d60:0x2ea7a,0x3803b0:0x2ea7a,0x2e3d0:0x2ea2e,
             0x2f2b0:0x2ed4c,0x48090:0x2f22c,0x166a80:0x2ecbb,0x2e9c0:0x20ceb,
             0x1301e0:0x130183,0x1ccd0:0x2e14a,0x13660:0x2bcb0}
    cases = []
    for index, case in enumerate(fixture['cases']):
        entry=case.get('entry',fixture['entry']);stop=stops[entry]
        banks = []
        for reference in case['initial_banks']:
            bank = fixture['banks'][reference]
            raw = bytearray.fromhex(bank['hex'])
            sp = case['entry_gp'][4]
            if bank['address'] <= sp < bank['address'] + len(raw):
                struct.pack_into('<I', raw, sp-bank['address'], stop)
            banks.append(dict(address=bank['address'], hex=raw.hex()))
        cases.append(dict(id=index, entry=entry, return_address=stop, exit_entry=stop,
            initial_banks=banks, exit={}, entry_abi=dict(gp=case['entry_gp'],
                eflags=0x202, mxcsr=0x1f80, fs_base=0, x87_control_word=0x27f,
                x87_status=0, x87_tag=0xffff, x87_values=[], xmm=['0'*32]*8)))
    hardware._Code = lambda image, section: QueryCode(image)
    entries=sorted({case['entry'] for case in cases})
    if len(entries)==1:
        result = hardware.execute(dict(image='1.5', cases=cases), paired, a.output)
    else:
        a.output.mkdir(parents=True)
        groups=[hardware.execute(dict(image='1.5',cases=[c for c in cases if c['entry']==entry]),
            paired,a.output/f'{entry:08x}') for entry in entries]
        result=dict(groups[0])
        result['cases']=sorted((c for group in groups for c in group['cases']),key=lambda c:c['id'])
        result['single_stepped_original_instructions']=sum(g['single_stepped_original_instructions'] for g in groups)
        result['original_hardware_binary_sha256']={f'{entry:08x}':group['original_hardware_binary_sha256'] for entry,group in zip(entries,groups)}
    banks, bank_ids, rows = [], {}, []

    def reference(address, raw):
        key = address, raw.hex()
        if key not in bank_ids:
            bank_ids[key] = len(banks)
            banks.append(dict(address=address, hex=key[1]))
        return bank_ids[key]

    differences = []
    code=QueryCode(Xbe(NEW_DEFAULT))
    for old, actual in zip(fixture['cases'], result['cases']):
        stop=stops[old.get('entry',fixture['entry'])]
        state = actual['exit']
        assert state['gp'] == old['exit_gp']
        assert state['xmm'] == old['exit_xmm']
        assert (state['eflags'] & old['eflags_mask']) == (old['eflags'] & old['eflags_mask'])
        assert actual['instructions'] == old['instructions']
        assert not state['x87_values'] and state['x87_control_word'] == 0x27f
        if state['x87_status'] != old['x87_status']:
            differences.append(dict(map=old.get('map'), variant=old['variant'],
                unicorn=old['x87_status'], hardware=state['x87_status']))
        if state['mxcsr'] != old['mxcsr']:
            assert state['mxcsr']^old['mxcsr']==0x20
            differences.append(dict(variant=old['variant'],kind='Unicorn omits MXCSR sticky precision flag',
                unicorn=old['mxcsr'],hardware=state['mxcsr']))
        before = actual['initial_banks']
        after = {bank['address']: bytes.fromhex(bank['hex']) for bank in actual['final_banks']}
        # Compare retained outputs already checked by the emulator, excluding
        # only the explicitly changed return router word at the caller's SP.
        for index in old['final_banks']:
            bank = fixture['banks'][index]
            p, expected = bank['address'], bytearray.fromhex(bank['hex'])
            sp = old['entry_gp'][4]
            if p <= sp < p + len(expected):
                struct.pack_into('<I', expected, sp-p, stop)
            owner = next((q, data) for q, data in after.items() if q <= p and p+len(expected) <= q+len(data))
            retained=owner[1][p-owner[0]:p-owner[0]+len(expected)]
            if retained!=expected:
                for mismatch,(left,right) in enumerate(zip(retained,expected)):
                    if left==right:
                        continue
                    address=p+mismatch
                    writer=next(w for w in reversed(actual['writes'])
                        if w['address']<=address<w['address']+len(bytes.fromhex(w['written'])))
                    cw=(code.at(writer['instruction']).mnemonic=='fnstcw' and address==writer['address'] and left^right==0x40)
                    # These fmod FNSTSW observations retain a real sticky
                    # precision flag which Unicorn omits. Expected native
                    # fixtures keep the hardware bytes without any masking.
                    fxam=(writer['instruction'] in (0x331f01,0x331f19) and
                        code.at(writer['instruction']).mnemonic=='fnstsw' and
                        address==writer['address'] and left^right==0x20)
                    assert cw or fxam,dict(variant=old['variant'],address=hex(address),hardware=left,unicorn=right,writer=writer)
                    differences.append(dict(variant=old['variant'],address=hex(address),
                        instruction=hex(writer['instruction']),kind='Unicorn omits fixed x87 control-word bit6' if cw else 'Unicorn fmod FNSTSW omits sticky precision flag',
                        hardware=left,unicorn=right))
        row = {key: value for key, value in old.items() if key not in ('initial_banks', 'final_banks')}
        row.update(x87_status=state['x87_status'], mxcsr=state['mxcsr'],
            initial_banks=[reference(b['address'], bytes.fromhex(b['hex'])) for b in before],
            final_banks=[reference(b['address'], bytes.fromhex(b['hex'])) for b in actual['final_banks']])
        rows.append(row)
    a.fixture.write_text(json.dumps(dict(entry=fixture['entry'], original_sha256=fixture['original_sha256'],
        oracle_backend=result['oracle_backend'], banks=banks, cases=rows), separators=(',', ':')) + '\n')
    report = dict(passed=True, cases=len(rows), differences_from_unicorn=differences,
        original_sha256=result['original_sha256'],
        single_stepped_original_instructions=result['single_stepped_original_instructions'],
        original_hardware_binary_sha256=result['original_hardware_binary_sha256'],
        instruction_patches=result['original_instruction_patches'],
        native_expected_data_used=result['native_expected_data_used'],
        fixture_sha256=hashlib.sha256(a.fixture.read_bytes()).hexdigest(),
        full_goal_complete=False, boundary=__doc__)
    (a.output/'comparison.json').write_text(json.dumps(report, indent=2) + '\n')
    print('PASS unchanged host i386 query owners:', len(rows), 'cases;', len(differences), 'Unicorn FP differences')


if __name__ == '__main__':
    main()
