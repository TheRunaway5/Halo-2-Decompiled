#!/usr/bin/env python3
"""Original default Spartan region selection and current-node pose production.

Uses the actual bipd/hlmt/mode tags. The original animation manager is explicitly
inactive, with no physical body, parent object or unit attachment. This executes
the normal default-pose fallback, not ordinary live animation selection.
"""
import argparse
import csv
import hashlib
import json
import struct
from pathlib import Path

from oracle_spartan_collision_1_1 import (
    CollisionTrace, ROOT, PIN, CACHE_PIN, UNIT, UI, POSE_OFFSET, PERM_OFFSET,
    Xbe, Cache, _Code, _trace, UC_X86_REG_EAX, UC_X86_REG_ECX,
    UC_X86_REG_EDX, UC_X86_REG_ESP,
)
LOCAL_OFFSET, UNIT_EXTENSION_OFFSET = 0x3000, 0x3A00
COLLISION_ORACLE_PIN = '99beb6fc599d06086fe8321a2b7346b1e494c77d60112c8622c2d57a85830e8e'


class PoseTrace(CollisionTrace):
    def __init__(self,position=(0.,0.,0.),forward=(1.,0.,0.),up=(0.,0.,1.),scale=1.,mirror=False):
        super().__init__()
        self.pose_active=False;self.pose_events=[];self.pose_returns=[]
        c=Cache(ROOT/'assets/disc/maps/lockout.map')
        spartan=c.tags[int(self.metadata['spartan']['datum'],16)]
        model=c.tags[c.u(spartan['offset']+0x38)]
        mode=c.tags[c.u(model['offset']+4)]
        sid=lambda value:c.text(c.u(368)+c.u(c.u(364)+(value&0xFFFF)*4))
        self.pose_metadata={'spartan':spartan,'model':model,'render_model':mode,
                            'default_variant_string_id':f'{c.u(spartan["offset"]+0x30):08x}',
                            'default_variant_name':sid(c.u(spartan['offset']+0x30)),
                            'nodes':[]}
        for i,at in enumerate(c.block(mode['offset']+0x48,0x60)):
            self.pose_metadata['nodes'].append({'index':i,'file_offset':at,
                'name':sid(c.u(at)),'parent_index':c.h(at+4),
                'first_child_index':c.h(at+6),'next_sibling_index':c.h(at+8),
                'raw_record_hex':bytes(c.data[at:at+0x60]).hex(),
                'default_local_quaternion_hex':bytes(c.data[at+0x18:at+0x28]).hex(),
                'default_local_translation_hex':bytes(c.data[at+0xC:at+0x18]).hex()})
        assert len(self.pose_metadata['nodes'])==self.node_count
        c.data.close();c.stream.close()
        self.sh(UNIT+0x114,self.node_count*0x34)
        self.sh(UNIT+0x112,LOCAL_OFFSET)
        self.sh(UNIT+0x12A,-1)
        self.sh(UNIT+0x33E,UNIT_EXTENSION_OFFSET)
        self.w(UNIT+UNIT_EXTENSION_OFFSET+0x28,-1)
        self.sh(UNIT+0x11C,0)
        self.w(UNIT+4,0x400 if mirror else 0)
        self.w(UNIT+0xB4,-1);self.w(UNIT+0x14,-1)
        self.g.mem_write(UNIT+0x34B,b'\x00')
        self.f(UNIT+0xA0,scale)
        for at,values in ((0x64,position),(0x70,forward),(0x7C,up)):
            self.g.mem_write(UNIT+at,struct.pack('<3f',*values))
        self.g.mem_write(UNIT+LOCAL_OFFSET,b'\xee'*(self.node_count*32))
        self.g.mem_write(UNIT+PERM_OFFSET,b'\xee'*(self.region_count*10))
        self.fixture={'position':self.vec(UNIT+0x64),'forward':self.vec(UNIT+0x70),
                      'up':self.vec(UNIT+0x7C),'scale_bits':f'{self.u(UNIT+0xA0):08x}',
                      'object_flags':f'{self.u(UNIT+4):08x}',
                      'animation_manager_offset':-1,'unit34b':0,
                      'local_transform_offset':LOCAL_OFFSET,
                      'node_matrix_offset':POSE_OFFSET,'node_matrix_bytes':self.node_count*52,
                      'region_state_offset':PERM_OFFSET,'region_state_bytes':self.region_count*10,
                      'no_parent_or_physics':True,'unit_attachment_datum':'ffffffff',
                      'auxiliary_resource_count':0,'region_mask_argument':'00000000',
                      'variant_override_string_id':'00000000'}

    def hook(self,g,a,size,user):
        if getattr(self,'pose_active',False):
            sp=g.reg_read(UC_X86_REG_ESP)
            for ret,row,out,count in reversed(self.pose_returns[:]):
                if ret==a:
                    row['return_eax']=f'{g.reg_read(UC_X86_REG_EAX):08x}'
                    if out:row['output_hex']=bytes(g.mem_read(out,count)).hex()
                    self.pose_returns.remove((ret,row,out,count));break
            if a in (0xBB430,0xBB480,0xBB580,0x16EC00,0x16ED00,0x16F550,
                     0x16F760,0xBDF80,0x16F3C0,0xBEB30,0x143960,
                     0x1441D0,0x10D260,0x114580,0xC0490):
                row={'entry':f'{a:08x}','registers':self.registers(),
                     'stack':[f'{self.u(sp+4+i*4):08x}' for i in range(6)]}
                out,count=0,0
                if a==0x16F3C0:
                    out,count=self.u(sp+8),self.node_count*32
                    row['authored_mode_pointer']=f'{self.u(sp+4):08x}'
                if a==0x143960:
                    out,count=self.u(sp+4),52
                    row['input_transform_hex']=bytes(g.mem_read(self.u(sp+8),32)).hex()
                if a==0x1441D0:
                    out,count=self.u(sp+4),52
                    row['parent_matrix_hex']=bytes(g.mem_read(g.reg_read(UC_X86_REG_ECX),52)).hex()
                    row['local_matrix_hex']=bytes(g.mem_read(g.reg_read(UC_X86_REG_EDX),52)).hex()
                if out:
                    row['output_pointer']=f'{out:08x}'
                    row['output_before_hex']=bytes(g.mem_read(out,count)).hex()
                self.pose_events.append(row)
                self.pose_returns.append((self.u(sp),row,out,count))
        super().hook(g,a,size,user)

    def execute(self):
        start=len(self.events);self.pose_active=True
        self.active='original_default_variant'
        self.run(0xBB430,regs=((UC_X86_REG_EAX,UI),(UC_X86_REG_ECX,0)))
        selected=self.u(UNIT+0xB1,1)
        self.active='original_initial_region_selection';self.run(0xBB480,(UI,0))
        region=bytes(self.g.mem_read(UNIT+PERM_OFFSET,self.region_count*10)).hex()
        self.active='original_default_pose_production';self.run(0xBDF80,(UI,))
        self.pose_active=False
        services=[e for e in self.events[start:] if 'service' in e]
        assert not services,services
        matrices=[bytes(self.g.mem_read(UNIT+POSE_OFFSET+i*52,52)).hex()
                  for i in range(self.node_count)]
        return {'fixture':self.fixture,'authored_metadata':self.pose_metadata,
                'selected_variant_index':selected,'region_state_hex':region,
                'active_model_permutations':list(bytes.fromhex(region)[:self.region_count]),
                'original_local_transforms_hex':bytes(self.g.mem_read(UNIT+LOCAL_OFFSET,self.node_count*32)).hex(),
                'current_node_matrices_hex':matrices,
                'original_pose_events':self.pose_events,
                'rng_writes':[e for e in self.events[start:] if 'rng_write_instruction' in e],
                'external_services_executed':services}


def inventory(directory):
    directory=directory.resolve()
    directory.mkdir(parents=True,exist_ok=True)
    x=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
    assert hashlib.sha256(x.data).hexdigest()==PIN
    code=_Code(x,'.text');starts={int(r['address'],16) for r in csv.DictReader(
        (ROOT/'analysis/map-pack-update/functions.tsv').open(),delimiter='\t')}
    entries=(0xB8A30,0xBB430,0xBB480,0xBB580,0x16EC00,0x16ED00,0x16F550,
             0x16F760,0xBDF10,0xBDF80,0x16F3C0,0xBEB30,0x143960,
             0x1441D0,0x10D260,0x114580,0xC0490,0xBD360,0xBD710,
             0xBEA50,0x1D0ED0,0xC52C0,0x110A20,0x113830,
             0x139750,0xB7E00,0xB7F50,0xB8040)
    rows=[]
    for start in entries:
        f=_trace(code,start,starts|set(entries));inst=[code.at(i) for i in sorted(f.code)]
        p=directory/f'{start:08x}.asm'
        p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n' for i in inst))
        rows.append({'start':f'{start:08x}','end_exclusive':f'{f.end:08x}',
                     'instruction_count':len(inst),'listing':str(p.relative_to(ROOT)),
                     'listing_sha256':hashlib.sha256(p.read_bytes()).hexdigest(),
                     'reachable_instruction_bytes_sha256':hashlib.sha256(b''.join(i.bytes for i in inst)).hexdigest(),
                     'direct_calls':[{'site':f'{a:08x}','target':f'{b:08x}'} for a,b,k in f.sites if k=='call']})
    (directory/'inventory.json').write_text(json.dumps({'xbe_sha256':PIN,
        'oracle_tool_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        'routines':rows},indent=2)+'\n')


def main():
    assert hashlib.sha256((ROOT/'tools/oracle_spartan_collision_1_1.py').read_bytes()).hexdigest()==COLLISION_ORACLE_PIN
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer/map-pack/spartan-default-pose-1.1-oracle.json')
    p.add_argument('--instruction-inventory',type=Path)
    args=p.parse_args()
    if args.instruction_inventory:inventory(args.instruction_inventory);return
    rows=[]
    for position in ((0.,0.,0.),(10.,20.,30.)):
        for forward in ((1.,0.,0.),(0.,1.,0.),(-1.,0.,0.)):
            for scale,mirror in ((1.,False),(2.,False),(.5,False),(1.,True)):
                trace=PoseTrace(position,forward,(0.,0.,1.),scale,mirror)
                row=trace.execute();row['name']=f'pos{position}-forward{forward}-scale{scale}-mirror{mirror}'
                # Same original collision consumer now receives the produced
                # node matrices and produced active permutation bytes.
                row['posed_collision']=trace.query((position[0]-2.,position[1],position[2]+.7),
                                                    (4.,0.,0.),row['active_model_permutations'])
                rows.append(row)
    report={'xbe_sha256':PIN,'cache_sha256':CACHE_PIN,
            'oracle_tool_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
            'collision_oracle_dependency_sha256':COLLISION_ORACLE_PIN,
            'case_count':len(rows),'cases':rows,
            'scope':'Actual authored default variant/region/node records and unchanged original default local pose→world matrices→posed collision. Animation manager, parent, physics and unit attachment explicitly inactive. No external engine services executed. Full ordinary animated multiplayer state/phase producers remain unresolved.'}
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({'cases':len(rows),'node_matrices':len(rows)*33,
                      'posed_hits':sum(row['posed_collision']['returned_hit'] for row in rows)}))


if __name__=='__main__':main()
