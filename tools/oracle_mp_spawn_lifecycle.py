#!/usr/bin/env python3
"""Original-instruction death-history, head-marker and collision-admission cases.

Uses the pinned images and world fixtures from oracle_mp_spawn. No executable
bytes are patched. Tick and death-writer prefixes execute actual instructions;
marker lookup/composition executes complete original routines with real cache
markers and supplied current node matrices. Collision callback cases supply only
the shape getType virtual service. Shape/world broadphase and narrowphase are
not a live collision oracle.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

from capstone import Cs, CS_ARCH_X86, CS_MODE_32
import capstone
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW

from oracle_mp_spawn import (Oracle, ROOT, VERSIONS, Cache, MAPS, ENGINE_GLOBALS,
    PLAYERS, OPTIONS, OBJECTS, HEADERS, POINT, bitstr, f32, CACHE_PIN)

EXTRA = 0x4000000
TYPE_SERVICE = EXTRA + 0x3ff000
TABLE, DEFINITION, MODEL, MODE, GROUPS, MARKERS = [EXTRA+n for n in (0,0x110000,0x111000,0x112000,0x113000,0x114000)]
TIME, MARKER_OUT, CALLBACK, BODY, ROOT_BODY, ENTITY, PROPERTIES, SHAPE, VTABLE, METADATA = [EXTRA+n for n in (0x120000,0x121000,0x122000,0x123000,0x124000,0x125000,0x126000,0x127000,0x128000,0x129000)]
CAPSULES, SPHERES = EXTRA+0x12a000, EXTRA+0x12b000
ADDR = {
    'base': dict(tick=0x162a30, writer=0x15ce70, marker=0xb8d30, time=0x510c54,
        callback=0x1c5400, marker_find=0x1d8f00, marker_compose=0x1d8f50),
    'tu11': dict(tick=0x1640d0, writer=0x15e4d0, marker=0xb9c20, time=0x518444,
        callback=0x1c8d30, marker_find=0x1dc8f0, marker_compose=0x1dc940),
}


class LifecycleOracle(Oracle):
    def __init__(self, version):
        super().__init__(version)
        self.extra = ADDR[version]
        self.g.mem_map(EXTRA, 0x400000)
        self.marker_calls = []
        self.type_calls = []
        self.shape_type = 0
        self.shape_hit = None
        self.shape_requests = []

    def hook(self, uc, address, size, unused):
        sp = uc.reg_read(UC_X86_REG_ESP)
        if hasattr(self, 'extra') and address == self.extra['marker']:
            self.marker_calls.append(dict(unit=f'{self.u(sp+4):08x}',
                marker=f'{self.u(sp+8):08x}', capacity=self.u(sp+12),
                flag=uc.reg_read(UC_X86_REG_ECX)&255,
                output=f'{uc.reg_read(UC_X86_REG_EBX):08x}'))
        elif address == TYPE_SERVICE:
            self.type_calls.append(dict(shape=f'{uc.reg_read(UC_X86_REG_ECX):08x}',
                supplied_type=self.shape_type))
            uc.reg_write(UC_X86_REG_EAX, self.shape_type)
            uc.reg_write(UC_X86_REG_EIP, self.u(sp))
            uc.reg_write(UC_X86_REG_ESP, sp+4)
        elif address == self.a['shape'] and self.shape_hit is not None:
            matrix=uc.reg_read(UC_X86_REG_EAX)
            values=self.floats(matrix,13)
            self.shape_requests.append(dict(transform=values,transform_bits=[bitstr(v) for v in values],
                shape=f'{self.u(sp+4):08x}',excluded=f'{self.u(sp+8):08x}',
                filter=self.u(sp+12),supplied_hit=self.shape_hit))
            uc.reg_write(UC_X86_REG_EAX,int(self.shape_hit))
            uc.reg_write(UC_X86_REG_EIP,self.u(sp))
            uc.reg_write(UC_X86_REG_ESP,sp+16)

    def fixture(self, cache):
        self.setup(cache)
        self.g.mem_write(EXTRA, bytes(0x400000))
        self.marker_calls=[]; self.type_calls=[]
        self.shape_hit=None;self.shape_requests=[]
        self.w(self.extra['time'], TIME)
        self.sh(TIME+2, 30); self.w(TIME+8, 200)
        self.w(ENGINE_GLOBALS+0x70,100)
        self.sh(ENGINE_GLOBALS+0x6c,1); self.w(ENGINE_GLOBALS+0xc04,1)
        for i in range(16): self.w(ENGINE_GLOBALS+0x2c+i*4,-1)
        self.sh(PLAYERS+0x1ac,-1)

    def history(self, slot):
        p=ENGINE_GLOBALS+0x558+slot*0x18
        return dict(active=self.g.mem_read(p,1)[0],
            position=self.floats(p+4,3), initial_gate=self.h(p+0x10), remaining=self.h(p+0x12))

    def ticks(self, cache):
        rows=[]
        cases=[(1,150,3),(1,150,1),(1,150,0),(1,150,-1),(1,150,-32768),
            (1,150,32767),(1,0,3),(1,0,0),(0,150,3),(0,150,1),(1,-1,3)]
        for mode in (0,4):
            for slot in (0,3,15):
                for active,gate,remaining in cases:
                    self.fixture(cache); self.w(OPTIONS+0xc,mode)
                    p=ENGINE_GLOBALS+0x558+slot*0x18
                    self.g.mem_write(p,bytes([active]));self.sh(p+0x10,gate);self.sh(p+0x12,remaining)
                    for j,v in enumerate((.25,-2,75)):self.f(p+4+j*4,v)
                    self.w(PLAYERS+slot*0x21c+0x164,777)
                    before=self.history(slot)
                    self.call(self.extra['tick'],registers=[(UC_X86_REG_EAX,0x10000|slot)])
                    after=self.history(slot)
                    expected=dict(before)
                    if gate:
                        expected['remaining']=max(remaining-1,0)
                        if expected['remaining']==0:expected.update(active=0,initial_gate=0,remaining=0)
                    assert after==expected,(self.version,mode,slot,before,after,expected)
                    assert self.u(PLAYERS+slot*0x21c+0x164)==777
                    rows.append(dict(mode=mode,player_datum=f'{0x10000|slot:08x}',
                        before=before,after=after,live_respawn_countdown_before=777,
                        live_respawn_countdown_after=777))
        return rows

    def biped(self, cache, tag, transform='identity', fallback=None):
        """Real authored tag/marker bytes, explicitly supplied current node poses."""
        b=tag['offset']; htag=cache.tags[cache.u(b+0x38)]; h=htag['offset']
        mtag=cache.tags[cache.u(h+4)]; m=mtag['offset']
        self.w(self.a['tags'],TABLE)
        for t,p in ((tag,DEFINITION),(htag,MODEL),(mtag,MODE)):
            self.w(TABLE+(int(t['datum'],16)&0xffff)*16+8,p)
        self.g.mem_write(DEFINITION,bytes(cache.data[b:b+0x340]))
        self.g.mem_write(MODEL,bytes(cache.data[h:h+0x10]))
        self.g.mem_write(MODE,bytes(cache.data[m:m+0x60]))
        groups=cache.block(m+0x58,0xc)
        self.w(MODE+0x5c,GROUPS)
        cursor=MARKERS
        heads=[]
        for i,q in enumerate(groups):
            self.g.mem_write(GROUPS+i*12,bytes(cache.data[q:q+12]))
            rows=cache.block(q+4,0x24)
            self.w(GROUPS+i*12+8,cursor)
            for r in rows:
                self.g.mem_write(cursor,bytes(cache.data[r:r+0x24]))
                if cache.u(q)==0x04000095:
                    heads.append(dict(file_offset=f'{r:08x}',node=cache.data[r+2],
                        position=list(struct.unpack_from('<3f',cache.data,r+4)),
                        quaternion=list(struct.unpack_from('<4f',cache.data,r+0x10))))
                cursor+=0x24
        assert len(heads)==1
        if fallback=='no_model':self.w(DEFINITION+0x38,-1)
        if fallback=='no_marker':
            for i,q in enumerate(groups):
                if cache.u(q)==0x04000095:self.w(GROUPS+i*12,0x0badf00d)
        node_count=cache.u(m+0x48)
        self.w(HEADERS+8,OBJECTS);self.w(OBJECTS,int(tag['datum'],16));self.w(OBJECTS+0x14,-1)
        self.sh(OBJECTS+0x114,node_count*52);self.sh(OBJECTS+0x116,0x400);self.sh(OBJECTS+0x11a,0x300)
        for n in range(node_count):
            rotation=[1,0,0,0,1,0,0,0,1] if transform=='identity' else [0,1,0,-1,0,0,0,0,1]
            matrix=[1 if transform=='identity' else 2]+rotation+[10+n,20+n*2,30+n*3]
            self.g.mem_write(OBJECTS+0x400+n*52,struct.pack('<13f',*matrix))
        node=0 if fallback else heads[0]['node']
        node_matrix=self.floats(OBJECTS+0x400+node*52,13)
        local=[0,0,0] if fallback else heads[0]['position']
        expected=[]
        for k in range(3):
            value=f32(f32(f32(node_matrix[1+k]*local[0])+f32(node_matrix[4+k]*local[1]))
                +f32(node_matrix[7+k]*local[2]))
            expected.append(f32(f32(value*node_matrix[0])+node_matrix[10+k]))
        return dict(tag=cache.reference(int(tag['datum'],16)),head_marker=heads[0],
            transform=transform,fallback=fallback,node_matrix=node_matrix,
            expected_world_head=expected)

    def marker_cases(self, cache):
        rows=[]
        tags=[t for t in cache.tags.values() if t['group']=='bipd' and t['name'].endswith('_mp')]
        for tag in tags:
            for transform,fallback in [('identity',None),('rotate_scale',None),
                    ('identity','no_model'),('identity','no_marker')]:
                self.fixture(cache);inputs=self.biped(cache,tag,transform,fallback)
                result=self.call(self.extra['marker'],(0x10000,0x04000095,1),
                    [(UC_X86_REG_EBX,MARKER_OUT),(UC_X86_REG_ECX,0)])&0xffff
                position=self.floats(MARKER_OUT+0x60,3)
                assert position==inputs['expected_world_head'],(self.version,inputs,position)
                assert result==(0 if fallback else 1)
                rows.append(dict(**inputs,result_count=result,position=position,
                    position_bits=[bitstr(v) for v in position],
                    full_marker_bytes=bytes(self.g.mem_read(MARKER_OUT,0x70)).hex(),
                    service_calls=self.marker_calls))
        return rows

    def writer_cases(self, cache):
        md=Cs(CS_ARCH_X86,CS_MODE_32)
        # Stop immediately after the original store to the second history short,
        # before subsequent notification/score/death-event services.
        store=next(i for i in md.disasm(self.x.read(self.extra['writer'],0x600),self.extra['writer'])
            if i.mnemonic=='mov' and i.op_str=='word ptr [esi + 0x12], ax')
        stop=store.address+store.size
        tag=next(t for t in cache.tags.values() if t['group']=='bipd' and t['name'].endswith('masterchief_mp'))
        rows=[]
        cases=[('no_unit',5,0,None,None),('head_identity',5,0,'identity',None),
            ('head_rotate_scale',5,0,'rotate_scale',None),('head_missing_model',5,0,'identity','no_model'),
            ('head_missing_marker',5,0,'identity','no_marker'),('minimum',0,0,None,None),
            ('negative_interval',-1,0,None,None),('penalty',5,7,None,None),
            ('short_snapshot_wrap',1093,0,None,None)]
        for case,seconds,penalty,transform,fallback in cases:
            self.fixture(cache)
            self.w(OPTIONS+0x1bc,seconds);self.w(PLAYERS+0x168,penalty)
            inputs=None
            if transform:
                inputs=self.biped(cache,tag,transform,fallback);self.w(PLAYERS+0x2c,0x10000)
            self.call(self.extra['writer'],(0xffffffff,0x10000,0,0),end=stop)
            state=self.history(0)
            delay=max(30,seconds*30+penalty)
            short=struct.unpack('<h',struct.pack('<H',delay&0xffff))[0]
            assert self.u(PLAYERS+0x164)==delay
            assert state['initial_gate']==state['remaining']==short
            assert self.u(PLAYERS+0x168)==0
            assert state['active']==int(transform is not None)
            expected=inputs['expected_world_head'] if inputs else [0,0,500]
            assert state['position']==expected
            rows.append(dict(case=case,tick_rate=30,variant_interval_seconds=seconds,
                input_penalty_ticks=penalty,attacker='ffffffff',victim='00010000',
                input_marker_world=inputs,history_after=state,live_respawn_countdown=delay,
                prefix_start=f'{self.extra["writer"]:08x}',prefix_stop=f'{stop:08x}',
                marker_calls=self.marker_calls))
        return rows

    def callback_cases(self, cache):
        rows=[]
        cases=[('ordinary_shape',1,True,0,-1,77,0),('non_body_root',0,True,0,-1,77,0),
            ('absent_entity',1,False,0,-1,77,0),('same_excluded_component',1,True,0,77,77,0),
            ('different_excluded_component',1,True,0,88,77,0),('metadata_sentinel_1',1,True,1,-1,77,0),
            ('metadata_sentinel_16',1,True,16,-1,77,0),('metadata_material_ff',1,True,METADATA,-1,77,255),
            ('metadata_material_0',1,True,METADATA,-1,77,0),('shape_type_24_material_0',1,True,METADATA,-1,77,0),
            ('missing_component_property',1,True,0,-1,None,0),('exclude_default_component_zero',1,True,0,0,None,0)]
        for parents in (0,2):
            for case,root_type,has_entity,metadata,excluded,component,material in cases:
                self.fixture(cache)
                self.shape_type=24 if case.startswith('shape_type_24') else 3
                self.w(CALLBACK+0xc,excluded);self.w(BODY,SHAPE)
                root=BODY
                for i in range(parents):
                    next_body=ROOT_BODY+i*0x100
                    self.w(root+0xc,next_body);root=next_body
                self.w(root+0x18,root_type);self.w(root+0x20,ENTITY if has_entity else 0)
                if component is not None:
                    self.w(ENTITY+0x30,PROPERTIES);self.w(ENTITY+0x34,1)
                    self.w(PROPERTIES,0x2001);self.w(PROPERTIES+4,component)
                self.w(SHAPE,VTABLE);self.w(VTABLE+0x14,TYPE_SERVICE);self.w(SHAPE+8,metadata)
                self.g.mem_write(METADATA+0x1e,bytes([material]))
                self.call(self.extra['callback'],(0,BODY),[(UC_X86_REG_ECX,CALLBACK)])
                has_material=self.shape_type!=24 and metadata not in (0,*range(1,17)) and material!=255
                expected=root_type==1 and has_entity and not has_material and (excluded==-1 or excluded!=(component or 0))
                found=self.g.mem_read(CALLBACK+4,1)[0];found_other=self.g.mem_read(CALLBACK+8,1)[0]
                assert bool(found)==bool(found_other)==expected
                rows.append(dict(case=case,parent_count=parents,root_type=root_type,has_entity=has_entity,
                    shape_type=self.shape_type,metadata=f'{metadata:08x}',metadata_material=material,
                    excluded_component=excluded,component_property=component,found=found,found_other=found_other,
                    virtual_shape_type_service_calls=self.type_calls))
        return rows

    def shape_request_cases(self, cache):
        rows=[]
        tags=[t for t in cache.tags.values() if t['group']=='bipd' and t['name'].endswith('_mp')]
        for tag in tags:
            for point in ((0,0,0),(10,-20,30),(0,0,-2)):
                for hit in (False,True):
                    self.fixture(cache);self.biped(cache,tag)
                    q=cache.block(tag['offset']+0x28c,0x50)
                    self.g.mem_write(CAPSULES,b''.join(bytes(cache.data[r:r+0x50]) for r in q))
                    self.w(DEFINITION+0x290,CAPSULES)
                    for i,value in enumerate(point):self.f(POINT+i*4,value)
                    self.shape_hit=hit
                    accepted=self.call(self.a['collision'],registers=[(UC_X86_REG_EAX,int(tag['datum'],16)),
                        (UC_X86_REG_ECX,POINT)])&255
                    request=self.shape_requests[0]
                    assert accepted==int(not hit)
                    assert request['shape']==f'{CAPSULES+0x20:08x}'
                    assert request['excluded']=='ffffffff' and request['filter']==9
                    assert request['transform']==[1,1,0,0,0,1,0,0,0,1,point[0],point[1],f32(point[2]+f32(.0701))]
                    rows.append(dict(tag=cache.reference(int(tag['datum'],16)),position=list(point),
                        accepted=accepted,source_capsule_file_offset=f'{q[0]:08x}',
                        source_capsule_bytes=bytes(cache.data[q[0]:q[0]+0x50]).hex(),request=request))
        self.fixture(cache)
        accepted=self.call(self.a['collision'],registers=[(UC_X86_REG_EAX,0xffffffff),(UC_X86_REG_ECX,POINT)])&255
        assert accepted==1 and not self.shape_requests
        rows.append(dict(tag=None,accepted=accepted,requests=[]))
        return rows


def inspect_shapes(cache):
    result=[]
    for tag in cache.tags.values():
        if tag['group']!='bipd':continue
        p=tag['offset']
        capsules=[]
        for q in cache.block(p+0x28c,0x50):
            capsules.append(dict(file_offset=f'{q:08x}',raw=bytes(cache.data[q:q+0x50]).hex(),
                radius=cache.f(q+0x2c),radius_bits=bitstr(cache.f(q+0x2c)),
                endpoint_a=list(struct.unpack_from('<3f',cache.data,q+0x30)),
                endpoint_b=list(struct.unpack_from('<3f',cache.data,q+0x40))))
        spheres=[dict(file_offset=f'{q:08x}',raw=bytes(cache.data[q:q+0x80]).hex())
            for q in cache.block(p+0x294,0x80)]
        result.append(dict(tag=cache.reference(int(tag['datum'],16)),file_offset=f'{p:08x}',
            standing_height=cache.f(p+0x268),crouching_height=cache.f(p+0x26c),radius=cache.f(p+0x270),
            first_spawn_query_shape='capsule' if capsules else 'translated_sphere',
            capsules=capsules,spheres=spheres))
    at=cache.u(0x170)+cache.u(cache.u(0x16c)+0x95*4)
    return dict(marker_stringid='04000095',marker_string_file_offset=f'{at:08x}',
        marker_text=cache.text(at),bipeds=result)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer/integration/spawn-source/lifecycle-report.json')
    args=parser.parse_args()
    oracles={v:LifecycleOracle(v) for v in VERSIONS}
    cache=Cache(ROOT/'assets/disc/maps/lockout.map')
    assert hashlib.sha256(cache.data).hexdigest()==CACHE_PIN
    rows={v:dict(ticks=o.ticks(cache),markers=o.marker_cases(cache),writers=o.writer_cases(cache),
        collision_callbacks=o.callback_cases(cache),shape_requests=o.shape_request_cases(cache)) for v,o in oracles.items()}
    for kind in rows['base']:
        # Prefix addresses differ between images; the complete state contract matches.
        def clean(value):
            if isinstance(value,dict):return {k:clean(v) for k,v in value.items()
                if k not in ('prefix_start','prefix_stop','output')}
            if isinstance(value,list):return [clean(v) for v in value]
            return value
        assert clean(rows['base'][kind])==clean(rows['tu11'][kind]),kind
    o=oracles['tu11']
    # The actual active-tick entry 139750 uses these arguments at 13975A..139764.
    o.call(0x327866,(0x9001f,0xfffff))
    control=o.g.reg_read(UC_X86_REG_FPCW)
    assert control&0x300==0x200 and control&0xc00==0
    maps=[]
    paths=[ROOT/f'assets/disc/maps/{n}.map' for n in MAPS]
    paths+=sorted((ROOT/'assets/multiplayer-map-pack/demos/dlc').glob('*/*.map'))
    for path in paths:
        c=Cache(path)
        maps.append(dict(map=path.name,sha256=hashlib.sha256(c.data).hexdigest(),**inspect_shapes(c)))
        c.data.close();c.stream.close()
    report=dict(passed=True,oracle_tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        imported_oracle_tool_sha256=hashlib.sha256((ROOT/'tools/oracle_mp_spawn.py').read_bytes()).hexdigest(),
        dependencies=dict(python=sys.version,unicorn=unicorn.__version__,capstone=capstone.__version__),
        authorities={v:dict(path=a['path'],sha256=a['pin']) for v,a in VERSIONS.items()},
        fixture_cpu_state=dict(x87_control_word='027f',mxcsr='00001f80'),
        active_tick_control87_case=dict(entry='00327866',arguments=['0009001f','000fffff'],
            resulting_control_word=f'{control:04x}',precision_control_bits=f'{control&0x300:03x}',
            rounding_control_bits=f'{control&0xc00:03x}',caller='00139750',callsite='00139764'),
        scope='Actual original history ticker, death-writer prefix, complete marker lookup/composition, spawn-shape helper and final contact callback execute. Current animated-node transforms and entity/contact worlds are explicit fixtures. Callback shape getType and world shape-query hit are supplied services. No world broadphase, collision filtering, narrowphase, full writer notifications or whole game tick is executed.',
        case_counts={k:sum(len(r[k]) for r in rows.values()) for k in rows['base']},
        rows=rows,maps=maps)
    args.output.parent.mkdir(parents=True,exist_ok=True)
    md=Cs(CS_ARCH_X86,CS_MODE_32)
    ranges={'base':[(0x162a30,0x162b10),(0x15ce70,0x15d210),(0xb8ca0,0xb8ee0),
        (0x1d8f00,0x1d9110),(0x1c5400,0x1c54c0)],
        'tu11':[(0x1640d0,0x1641b0),(0x15e4d0,0x15e86d),(0xb9b90,0xb9dd0),
        (0x1dc8f0,0x1dcb10),(0x1c8b40,0x1c8de0),(0x311f10,0x311f80),
        (0x15d813,0x15d8d2),(0x139750,0x1398f0),(0x327866,0x3278ae)]}
    for v,blocks in ranges.items():
        o=oracles[v];lines=[f'{v} {o.a["pin"]}']
        for start,end in blocks:
            data=o.x.read(start,end-start)
            lines.append(f'\nRANGE {start:08x}..{end:08x} SHA256 {hashlib.sha256(data).hexdigest()}')
            lines.extend(f'{i.address:08x} {i.mnemonic} {i.op_str}' for i in md.disasm(data,start))
        (args.output.parent/f'{v}-lifecycle-instructions.txt').write_text('\n'.join(lines)+'\n')
    args.output.write_text(json.dumps(report,indent=2,allow_nan=False)+'\n')
    print(json.dumps(dict(passed=True,case_counts=report['case_counts'],authored_maps=len(maps))))


if __name__=='__main__':main()
