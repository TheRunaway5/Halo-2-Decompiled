#!/usr/bin/env python3
"""OriginalBDC40 roots from all ten paused object inputs, without tuning.
Names are mapped by exact original scenario position/definition identity;
native tests independently load production LevelContent and its scale rule.
"""
from pathlib import Path
import hashlib,json,struct,sys
from oracle_multiplayer_hud_positions import Retail,ROOT,PIN
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX
D=ROOT/'analysis/frontend/profile-regression';snapshot=D/'original-model-actual-fourway-ui-node-bank.json';rows=json.loads(snapshot.read_text())['objects'];assets=json.loads((D/'model-scene-assets.json').read_text())['objects'];r=Retail();out=[];records=[]
for row in rows:
 header=bytes.fromhex(row['object_header_hex']);flags=struct.unpack_from('<I',header,4)[0];assert not flags&0x400 and struct.unpack_from('<I',header,0x14)[0]==0xffffffff
 assert struct.unpack_from('<I',header,0xd4)[0]==0xffffffff or not header[0xd8]&1
 raw=bytes.fromhex(row['object_root_hex']);assert raw==header[0x64:0xa4]
 position=raw[:12];matched=[a for a in assets if a['datum']==row['definition'] and struct.pack('<3f',*a['position'])==position];assert len(matched)==1
 name=matched[0]['name'];source=r.alloc(raw);target=r.alloc(bytes(52));scale=struct.unpack_from('<I',raw,0x3c)[0]
 r.run(0xbdc40,argv=[source,scale,0,0,target],registers=[(UC_X86_REG_EAX,source+0x18),(UC_X86_REG_ECX,source+0xc),(UC_X86_REG_EBX,0)])
 result=bytes(r.g.mem_read(target,52));out.append(' '.join([name,row['definition'],*(f'{x:08x}'for x in struct.unpack('<13I',result))]));records.append(dict(name=name,definition=row['definition'],object=row['object'],captured_inputs_hex=raw.hex(),original_root_hex=result.hex(),scenario_authored_position=matched[0]['position'],scenario_authored_rotation=matched[0]['rotation'],scenario_authored_scale=matched[0]['scale']))
assert len(out)==10 and len({x['name']for x in records})==10
fixture=D/'frontend-model-roots-original.tsv';fixture.write_text('# OriginalBDC40 from captured unparented/unmirrored/nonsynced model roots; floatwords untouched.\n'+'\n'.join(out)+'\n');report=dict(xbe_sha256=PIN,snapshot=str(snapshot.relative_to(ROOT)),snapshot_sha256=hashlib.sha256(snapshot.read_bytes()).hexdigest(),fixture_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),method=__doc__,records=records);(D/'frontend-model-roots-original.json').write_text(json.dumps(report,indent=2)+'\n');print('10 original model roots frozen')
