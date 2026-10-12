#!/usr/bin/env python3
"""Original normalization epsilon cases and pinned clear-range callback inputs.

Does not rewrite existing frozen suites. Range outputs are the pinned original
instruction report; normalization executes the original helper unchanged.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

from oracle_smg_firing_1_1 import ROOT, PIN, CACHE_PIN, Trace, UC_X86_REG_EAX, UC_X86_REG_XMM0

VECTOR = 0x10F8000
RANGE_REPORT = ROOT/'analysis/multiplayer/map-pack/smg-impact-suite/clear_range_sequence-mode1.json'
RANGE_PIN = 'd7c91437c6aed6fc641ddfc811481b0c54389e88f35757551460d8110d192413'


def bits(value):
    return struct.unpack('<I',struct.pack('<f',value))[0]


def fields(snapshot):
    raw=bytes.fromhex(snapshot['projectile_hex'])
    output={'object_hex':snapshot['projectile_hex'],
            'header_hex':snapshot['projectile_header_hex']}
    for offset in (0x64,0x70,0x7C,0x88):
        output[f'vector_{offset:03x}']={'bits':[f'{v:08x}' for v in struct.unpack_from('<3I',raw,offset)],
                                      'values':list(struct.unpack_from('<3f',raw,offset))}
    for offset in (0x12C,0x15C,0x160,0x164,0x168,0x16C,0x170,0x180,0x184,0x188):
        output[f'field_{offset:03x}_bits']=f'{struct.unpack_from("<I",raw,offset)[0]:08x}'
    for offset in (0x130,0x132,0x1AA):
        output[f'field_{offset:03x}_int16']=struct.unpack_from('<h',raw,offset)[0]
    return output


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer/map-pack/projectile-boundaries-1.1-oracle.json')
    args=p.parse_args()
    trace=Trace(1)
    epsilon=trace.u(0x46504C)
    cases=[]
    inputs=[('zero',(0,0,0)),('below',(epsilon-1,0,0)),('equal',(epsilon,0,0)),
            ('above',(epsilon+1,0,0)),('negative_equal',(epsilon|0x80000000,0,0)),
            ('diagonal',(epsilon,epsilon,epsilon)),('ordinary',(bits(400.),bits(.1),bits(-.2)))]
    for name,vector in inputs:
        trace.active='original_normalization_helper'
        trace.g.mem_write(VECTOR,struct.pack('<3I',*vector))
        trace.run(0x30BD0,regs=((UC_X86_REG_EAX,VECTOR),))
        cases.append({'name':name,'input_bits':[f'{v:08x}' for v in vector],
                      'output_vector':trace.vec(VECTOR),
                      'returned_length_bits':f'{trace.g.reg_read(UC_X86_REG_XMM0)&0xFFFFFFFF:08x}'})
    raw=RANGE_REPORT.read_bytes()
    assert hashlib.sha256(raw).hexdigest()==RANGE_PIN
    original=json.loads(raw)
    assert original['xbe_sha256']==PIN and original['cache_sha256']==CACHE_PIN
    output={'xbe_sha256':PIN,'cache_sha256':CACHE_PIN,
            'tool_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
            'normalization':{'entry':'00030bd0','fpcw':'027f','mxcsr':'00001f80',
                             'epsilon_va':'0046504c','epsilon_bits':f'{epsilon:08x}',
                             'epsilon_float':trace.fv(0x46504C),'cases':cases},
            'clear_range':{'source_report':str(RANGE_REPORT.relative_to(ROOT)),
                           'source_report_sha256':RANGE_PIN,
                           'source_tool_sha256':original['oracle_tool_sha256'],
                           'entry':'000fcec0','callback_args':['00010002'],
                           'tick_rate':30,'dt_bits':f'{bits(1/30):08x}',
                           'gravity_fixture_bits':'00000000',
                           'initial_distance_fixture':None,
                           'authored_projectile':original['authored']['projectile'],
                           'rows':[{'tick':r['tick'],'before':fields(r['before']),
                                    'after':fields(r['after'])} for r in original['motion_rows']],
                           'collision_queries':[e for e in original['impact_events'] if 'collision_service' in e],
                           'deletion_postpass':original['original_removal_postpass']},
            'scope':'Original30BD0 runs unchanged. Range rows are copied from separately pinned original callback output with explicit clear collision/services and actual authored SMG; no native implementation supplies expected values. Full world and live frame order are outside scope.'}
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(output,indent=2)+'\n')
    print(json.dumps({'normalization_cases':len(cases),'range_ticks':len(output['clear_range']['rows'])}))


if __name__=='__main__':main()
