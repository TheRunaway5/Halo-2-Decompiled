#!/usr/bin/env python3
"""Authenticate the actual authored area-choice descriptor and bool consumer."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
from oracle_smg_firing_1_1 import Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import complete_body


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    assert Path(os.environ['HALO2_TEST_SCRATCH']).is_dir()
    image = Xbe(NEW_DEFAULT)
    assert hashlib.sha256(image.data).hexdigest() == NEW_PIN
    code = _Code(image, '.text')
    body = complete_body(code, 0x1b0cd0)
    assert 0x1b0d1c in body.code
    for pc, operation, operands in ((0x1b0d1c, 'call', 'eax'),
        (0x1b0d1e, 'mov', 'dl, al'), (0x1b0d20, 'test', 'dl, dl')):
        instruction = code.at(pc)
        assert (instruction.mnemonic, instruction.op_str) == (operation, operands)
    assert int.from_bytes(image.read(0x48a9d8+0x18, 4), 'little') == 0x1bbec0
    references = []
    for section in image.sections:
        raw = bytes(image.section_bytes(section));offset = 0
        while (offset := raw.find(struct.pack('<I', 0x1bbec0), offset)) >= 0:
            references.append(dict(section=section.name, address=hex(section.va+offset)))
            offset += 1
    assert references == [dict(section='.data', address='0x48a9f0')], references
    instructions = [code.at(pc) for pc in sorted(body.code)]
    fp = [i for i in instructions if any(i.reg_name(o.reg).startswith(('xmm', 'mm', 'st('))
            for o in i.operands if o.type == 1) or i.mnemonic.startswith('f')]
    assert not fp
    penalty=code.at(0x1bbda6)
    assert (penalty.mnemonic,penalty.op_str)==('fmul','qword ptr [0x46a328]')
    section=image.section('.rdata')
    assert section.va <= 0x46a328 and 0x46a330 <= section.va+section.rsize
    assert image.read(0x46a328,8).hex()=='9a9999999999b93f'
    report = dict(passed=True, original_sha256=NEW_PIN, root='0x1b0cd0',
        caller=[dict(pc=hex(i.address), bytes=i.bytes.hex(), operation=i.mnemonic,
                     operands=i.op_str) for i in instructions],
        descriptor=dict(address='0x48a9d8', slot='0x18', target='0x1bbec0'),
        literal_entry_references=references,
        distance_penalty=dict(instruction='0x1bbda6',address='0x46a328',bytes=8,
            section=section.name,original_bytes='9a9999999999b93f'),
        contract=dict(arguments='Actual entry SP+4 actor and SP+8 retained request',
            result='AL copied to DL; TEST DL kills callback flags',
            stack='Actual indirect CALL pushes1B0D1E; real1BBBD0 RET8 advances entrySP by12',
            floating_point='Actual callback changes hardware status; older x87 values and policy retained. Whole1B0CD0 has no floating-point/SIMD/MMX operands.'),
        source_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        limits=['Exact1B0D1C descriptor transport only; other numeric callers remain unqualified',
                'Physical caller packets and native transport validation are separate evidence'],
        retained_large_artifacts=[])
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2)+'\n')
    print('Authenticated1B0CD0 complete bool consumer and48A9D8+18 area-choice descriptor')


if __name__ == '__main__':
    main()
