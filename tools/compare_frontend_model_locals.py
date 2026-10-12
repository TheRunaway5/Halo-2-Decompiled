#!/usr/bin/env python3
"""Compare retained native raw local channels to actual paused original locals.

No world transform, reciprocal seed, IK or tolerance is involved. Actual
post-admission local banks and controller-frame bits are independent inputs.
"""
import argparse
import hashlib
import json
import struct
import subprocess
from pathlib import Path
from inspect_multiplayer import Cache, ROOT

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('snapshots', nargs='+', type=Path)
parser.add_argument('--binary', type=Path, default=ROOT/'build-frontend/frontend_model_locals_tests')
parser.add_argument('--output', type=Path, required=True)
args=parser.parse_args()
cache=Cache(ROOT/'assets/disc/maps/mainmenu.map')
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def floats(raw):return struct.unpack('<'+str(len(raw)//4)+'f', raw)
rows,inputs=[],[]
for path in args.snapshots:
    report=json.loads(path.read_text())
    objects=report.get('objects',[report])
    if 'sampler_calls' in report:
        assert report['before']['primary_hex']==report['after']['primary_hex']
        entry=report['sampler_calls'][0]
        assert int(entry['frame_word'],16)==struct.unpack_from('<I',bytes.fromhex(report['after']['primary_hex']),28)[0]
        assert int(entry['stack_args'][6],16)&255==0 and int(entry['weight_word'],16)==0x3f800000
        objects=[dict(report['after'],object=report['object'],definition=report['definition'],
            primary_raw_hex=report['after']['primary_hex'],object_header_hex=report['after']['header_hex'],
            explicit_mask=list(struct.unpack('<8I',bytes.fromhex(report['mask_hex']))))]
    for obj in objects:
        primary=bytes.fromhex(obj.get('primary_raw_hex') or obj['channels'][0]['raw_hex'])
        graph,packed=struct.unpack_from('<2I', primary)
        assert packed&0xffff==0xffff
        frame=struct.unpack_from('<I', primary, 0x1c)[0]
        flags=struct.unpack_from('<I', bytes.fromhex(obj['object_header_hex']),4)[0]
        unit=int(obj['definition'],16)
        mask=obj.get('explicit_mask',[])
        if not mask and flags&(1<<30):
            definition=cache.tags[unit]['offset'];model=cache.tags[cache.u(definition+0x38)]['offset']
            mask=[(~cache.u(model+0xa4+i*4))&0xffffffff for i in range(8)]
        inputs.append(' '.join(f'{v:x}' for v in [unit,graph,packed>>16,frame,len(mask),1,*mask]))
        rows.append(dict(snapshot=str(path.resolve()),snapshot_sha256=digest(path),object=obj['object'],
            unit=f'{unit:08x}',graph=f'{graph:08x}',clip=packed>>16,frame_bits=f'{frame:08x}',
            flags=f'{flags:08x}',mask=mask,local_bank_hex=obj['local_bank_hex']))
binary_before=digest(args.binary)
actual=subprocess.run([str(args.binary.resolve()),'--local-probe',str(ROOT/'assets/disc/maps')],
    input='\n'.join(inputs)+'\n',text=True,capture_output=True,check=True).stdout.splitlines()
assert digest(args.binary)==binary_before and len(actual)==len(rows)
for row,line in zip(rows,actual):
    values=[int(v,16) for v in line.split()];count=values[0];assert len(values)==1+count*9
    expected=bytes.fromhex(row.pop('local_bank_hex'));assert len(expected)==count*32
    nodes=[]
    for n in range(count):
        sid,*words=values[1+n*9:1+(n+1)*9]
        name=cache.text(cache.u(368)+cache.u(cache.u(364)+(sid&0xffff)*4))
        a=struct.pack('<8I',*words);e=expected[n*32:(n+1)*32]
        aw=struct.unpack('<8I',a);ew=struct.unpack('<8I',e)
        errors=[abs(x-y) for x,y in zip(floats(a),floats(e))]
        nodes.append(dict(index=n,name=name,name_id=f'{sid:08x}',
            admitted=not row['mask'] or bool(row['mask'][n//32]&(1<<(n%32))),
            actual_words=[f'{v:08x}' for v in aw],expected_words=[f'{v:08x}' for v in ew],
            differing_words=sum(x!=y for x,y in zip(aw,ew)),max_component_error=max(errors),
            quaternion_error=max(errors[:4]),translation_error=max(errors[4:7]),scale_error=errors[7]))
    admitted=[n for n in nodes if n['admitted']]
    row['nodes']=nodes;row['differing_words']=sum(n['differing_words'] for n in admitted)
    row['differing_unadmitted_words']=sum(n['differing_words'] for n in nodes if not n['admitted'])
    row['maximum_component_error']=max(n['max_component_error'] for n in admitted)
    nongrip=[n for n in admitted if not any(s in n['name'] for s in ('finger','thumb','ring','index','pinky'))]
    row['maximum_nongrip_quaternion_error']=max(n['quaternion_error'] for n in nongrip)
    row['first_nongrip_difference']=next((n for n in nongrip if n['differing_words']),None)
args.output.write_text(json.dumps(dict(comparison_binary_sha256=binary_before,records=rows,
    scope='Actual captured local bank versus native raw primary+grip, at retained frame/mask. No world transform, RCPSS, IK or tolerance acceptance.'),indent=2)+'\n')
for row in rows:print(json.dumps({k:v for k,v in row.items() if k not in ('nodes','first_nongrip_difference')}))
