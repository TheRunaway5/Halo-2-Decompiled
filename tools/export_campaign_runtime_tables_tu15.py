#!/usr/bin/env python3
"""Export bounded original campaign-runtime dispatch data, never branch results.

Use tools/run_test.py. The same whole-function CFG reader used by the source
module generator establishes table widths/counts from the original guards.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct

from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_campaign_runtime_tu15 import ENTRIES


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--manifest',type=Path,required=True)
    args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    assert args.output.resolve().is_relative_to(scratch)
    image=Xbe(NEW_DEFAULT)
    assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    code=_Code(image,'.text');tables={}
    for entry in ENTRIES:
        body=_trace(code,entry,set(ENTRIES))
        for address,width,count in body.tables:
            assert width==4 and count>0
            row=tables.setdefault((address,count),dict(address=address,count=count,owners=[],
                original_hex=image.read(address,count*4).hex()))
            row['owners'].append(entry)
    lines=['// Authenticated bounded TU1.5 campaign program tables; source code chooses every index.']
    for (address,count),row in sorted(tables.items()):
        for index,value in enumerate(struct.unpack('<'+'I'*count,bytes.fromhex(row['original_hex']))):
            lines.append(f'share_immutable_word(memory,0x{address+index*4:x}u,0x{value:x}u,'+
                '"native TU1.5 authored bounded campaign dispatch target");')
    args.output.write_text('\n'.join(lines)+'\n')
    report=dict(original_xbe_sha256=NEW_PIN,tables=list(tables.values()),
        total_bytes=sum(count*4 for _,count in tables),source_instruction_changes=False,
        supplied_branch_indices_or_results=False,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        data_inc_sha256=hashlib.sha256(args.output.read_bytes()).hexdigest(),
        scratch=str(scratch),large_artifacts_retained=[])
    args.manifest.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(tables=len(tables),bytes=report['total_bytes'])),flush=True)


if __name__=='__main__':main()
