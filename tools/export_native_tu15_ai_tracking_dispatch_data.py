#!/usr/bin/env python3
"""Export the original four-row AI tracking predicate/handler interface.

274FA0 starts its descending predicate search at row3, advances by-12 and
uses the current tracking kind as its lower bound. Its fallback indexes the
matching handler column at twelve-byte stride. Export only the eight callback
identity bytes in each authenticated ID0..3 row, without row headers, results,
instructions, raw state or a claim that the callbacks have been qualified.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import struct
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

FRAME_PIN='0faad001eff14edaaa31df28eddcde059ab855a5a3639fff4b81e3526ee15308'


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--fixture',type=Path,required=True);ap.add_argument('--report',type=Path,required=True)
    ap.add_argument('--output',type=Path,required=True);ap.add_argument('--frame-source',type=Path,required=True)
    ap.add_argument('--verify',action='store_true');args=ap.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();assert args.output.resolve().is_relative_to(scratch)
    raw=args.frame_source.read_bytes();assert hashlib.sha256(raw).hexdigest()==FRAME_PIN
    entries={int(p,16) for p in re.findall(r'static void run_([0-9a-f]+)\(WorldState &s\)',raw.decode())}
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    code=_Code(image,'.text');body=_trace(code,0x274fa0,entries)
    expected={0x2752b8:'bf03000000',0x2752c3:'bbf4d64700',0x2752e4:'83eb0c',
        0x2752e9:'7fdd',0x2752fd:'8d0c40',0x275307:'ff148dd4d64700'}
    evidence=[]
    for pc,bits in expected.items():
        ins=code.at(pc);assert pc in body.code and bytes(ins.bytes).hex()==bits,(hex(pc),bytes(ins.bytes).hex())
        evidence.append([pc,bits,ins.mnemonic+' '+ins.op_str])
    rows=[];lines=['// Original four-row AI tracking predicate/handler identities; row headers excluded.']
    for index in range(4):
        address=0x47d6cc+index*12;raw=image.read(address,12);identity,predicate,handler=struct.unpack('<3I',raw)
        assert identity==index and code.inside(handler) and (predicate==0 or code.inside(predicate))
        data=raw[4:12];rows.append(dict(index=index,address=address+4,data_hex=data.hex(),predicate=predicate,handler=handler,
            predicate_compiled=predicate==0 or predicate in entries,handler_compiled=handler in entries))
        lines.append(f'literal(memory,0x{address+4:x}u,std::array<std::uint8_t,8>{{'+','.join(f'0x{v:02x}' for v in data)+'});')
    output=('\n'.join(lines)+'\n').encode();args.output.write_bytes(output)
    fixture=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,pinned_frame_sha256=FRAME_PIN,rows=rows,
        source_extent_evidence=evidence,instruction_patches=[],supplied_dependency_returns=[])
    data=(json.dumps(fixture,separators=(',',':'))+'\n').encode()
    if args.verify or args.fixture.exists():assert args.fixture.read_bytes()==data,'Preserve original tracking dispatch reference'
    else:args.fixture.parent.mkdir(parents=True,exist_ok=True);args.fixture.write_bytes(data)
    report=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,rows=4,immutable_bytes=32,
        fixture_bytes=len(data),fixture_sha256=hashlib.sha256(data).hexdigest(),output_sha256=hashlib.sha256(output).hexdigest(),
        tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),full_callback_behavior_qualified=False,
        uncompiled_handlers=[r['handler'] for r in rows if not r['handler_compiled']],
        runtime_reference_required=False)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))


if __name__=='__main__':main()
