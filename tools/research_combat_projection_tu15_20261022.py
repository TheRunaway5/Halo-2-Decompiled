#!/usr/bin/env python3
"""Read original TU1.5 combat projection/attachment owners without mutation."""
import argparse
from collections import deque
import csv
import hashlib
import json
import os
from pathlib import Path

from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from capstone.x86 import X86_OP_IMM, X86_OP_MEM
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from oracle_smg_firing_1_1 import Xbe, _Code, _trace, ROOT


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output', type=Path, required=True)
    ap.add_argument('--entry', type=lambda s: int(s, 0), action='append', default=[])
    ap.add_argument('--skip-references', action='store_true')
    args = ap.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        ap.error('Use tools/run_test.py')
    assert hashlib.sha256(NEW_DEFAULT.read_bytes()).hexdigest() == NEW_PIN
    image = Xbe(NEW_DEFAULT)
    code = _Code(image, '.text')
    md = Cs(CS_ARCH_X86, CS_MODE_32)
    md.detail = True
    md.skipdata = True
    starts = {int(row['address'], 16) for row in csv.DictReader(
        (ROOT/'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv').open(), delimiter='\t')}
    def line(i):
        return f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}'
    references = []
    previous = deque(maxlen=18)
    pending = []
    for i in (() if args.skip_references else md.disasm(code.bytes, code.lo)):
        if not i.id:
            continue
        if i.mnemonic == 'call' and len(i.operands) == 1 and i.operands[0].type == X86_OP_IMM and code.inside(i.operands[0].imm):
            starts.add(i.operands[0].imm)
        encoded = line(i)
        for r, remaining in pending:
            r['context'].append(encoded)
        pending = [(r, remaining-1) for r, remaining in pending if remaining > 1]
        call = i.mnemonic == 'call' and i.operands[0].type == X86_OP_IMM and i.operands[0].imm == 0x3d2b0
        bank = any(o.type == X86_OP_MEM and o.mem.disp == 0x4ce3f4 for o in i.operands)
        if call or bank:
            row = {'site': f'{i.address:08x}', 'first_person_setup_call': call,
                'model_count_reference': bank,
                'context': list(previous) + [encoded]}
            references.append(row)
            pending.append((row, 19))
        previous.append(encoded)
    args.output.mkdir(parents=True, exist_ok=True)
    entries = set(args.entry)
    assert entries <= starts, 'Use actual TU1.5 function entries from the qualified inventory'
    functions = []
    for entry in sorted(entries):
        f = _trace(code, entry, starts | entries)
        ins = [code.at(p) for p in sorted(f.code)]
        data = b''.join(i.bytes for i in ins)
        target = args.output / f'{entry:08x}.asm'
        target.write_text('\n'.join(line(i) for i in ins)+'\n')
        functions.append({'entry': f'{entry:08x}', 'instructions': len(ins),
            'instruction_bytes_sha256': hashlib.sha256(data).hexdigest(),
            'calls': [f'{p:08x}' for p in sorted(f.calls)], 'listing': str(target)})
    report = {'original_xbe_sha256': NEW_PIN, 'tool_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        'scope': 'Read-only original instruction references and full control-flow listings; no execution or GPU outcome supplied',
        'references': references, 'functions': functions}
    (args.output/'report.json').write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps({'references': [{k:v for k,v in r.items() if k != 'context'} for r in references],
                      'functions': functions}))


if __name__ == '__main__':
    main()
