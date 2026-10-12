#!/usr/bin/env python3
"""Original valid graph/unit constructor, animation update and posed collision.

Resident animation bytes and reserved object heap capacity are explicit services.
The original startup state resolves its own animation; no clip is substituted.
Movement/action request production remains a separately recorded input boundary.
"""
import argparse
import csv
import hashlib
import json
import struct
from pathlib import Path

from oracle_spartan_default_pose_1_1 import PoseTrace, ROOT, PIN, CACHE_PIN, UNIT, UI, POSE_OFFSET, PERM_OFFSET
from oracle_smg_firing_1_1 import (
    Cache, Xbe, _Code, _trace, HEADERS, TIME, SEED, GAME, WORLD,
    UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI,
    UC_X86_REG_EDI, UC_X86_REG_EBX, UC_X86_REG_ESP, UC_X86_REG_EIP,
)
MANAGER_OFFSET = 0x3800
DESCRIPTOR, FAILURE, REQUEST = 0x10F9000, 0x10FA000, 0x10FB000
DEFAULT_ORACLE_PIN = 'd3a8a87bcadd810ff0e47beb57f0b03fa1fc592164cb64cfe8819eccf7c6a0d4'


class LivePoseTrace(PoseTrace):
    def __init__(self,game_mode=5):
        super().__init__()
        self.game_mode=game_mode
        self.g.mem_write(GAME+12,bytes([game_mode]));self.w(WORLD+8,4 if game_mode==5 else 1)
        self.live_active=False; self.live_events=[]; self.live_returns=[]
        self.resident={}; self.live_metadata={}; self.failures=[]
        c=Cache(ROOT/'assets/disc/maps/lockout.map')
        model=self.pose_metadata['model'];graph=c.tags[c.u(model['offset']+0x14)]
        sid=lambda value:c.text(c.u(368)+c.u(c.u(364)+(value&0xFFFF)*4))
        self.live_metadata['graph']=graph
        self.live_metadata['animations']=[{'index':i,'file_offset':at,'name':sid(c.u(at)),
            'frame_count':c.h(at+0x14),'resource_group_index':c.u(at+0x24),
            'resource_group_offset':c.u(at+0x28),'raw_hex':bytes(c.data[at:at+0x6C]).hex()}
            for i,at in enumerate(c.block(graph['offset']+0x2C,0x6C))]
        self.live_metadata['model_variants']=[]
        for i,v in enumerate(c.block(model['offset']+0x50,0x38)):
            record={'index':i,'name':sid(c.u(v)),'file_offset':v,'raw_hex':bytes(c.data[v:v+0x38]).hex(),'regions':[]}
            for j,r in enumerate(c.block(v+0x14,0x14)):
                region={'index':j,'name':sid(c.u(r)),'file_offset':r,'raw_hex':bytes(c.data[r:r+0x14]).hex(),'permutations':[]}
                for k,p in enumerate(c.block(r+8,0x20)):
                    region['permutations'].append({'index':k,'name':sid(c.u(p)),'file_offset':p,
                        'raw_hex':bytes(c.data[p:p+0x20]).hex(),
                        'states':[{'file_offset':s,'raw_hex':bytes(c.data[s:s+0x18]).hex()} for s in c.block(p+0xC,0x18)]})
                record['regions'].append(region)
            self.live_metadata['model_variants'].append(record)
        # Only map groups whose original resource descriptor names this cache.
        # Missing/shared requests fail explicitly; no alternate file is guessed.
        at=0x5000000; self.g.mem_map(at,0x1000000)
        for i,off in enumerate(c.block(graph['offset']+0xAC,20)):
            size,pointer=c.u(off+4),c.u(off+8)
            if pointer>>30:continue
            assert pointer+size<=len(c.data)
            raw=bytes(c.data[pointer:pointer+size]);self.g.mem_write(at,raw)
            self.resident[off+c.magic]={'resident_pointer':at,'index':i,'size':size,
                'file_offset':pointer,'descriptor_hex':bytes(c.data[off:off+20]).hex(),
                'raw_sha256':hashlib.sha256(raw).hexdigest()}
            at+=(size+4095)&~4095
        assert at<=0x6000000
        c.data.close();c.stream.close()
        self.services[0x18FD70]=('reserved object arena heap growth succeeds, no relocation',1,12)
        self.sh(UNIT+0x128,0x90);self.sh(UNIT+0x12A,MANAGER_OFFSET)
        self.g.mem_write(UNIT+MANAGER_OFFSET,bytes(0x90))
        # Existing arena data and authored pose allocations are explicit inputs;
        # BD270 allocates/zeros new extension blocks itself after this boundary.
        self.sh(HEADERS+12+6,0x3A00)
        self.g.mem_write(DESCRIPTOR,bytes(0xC4));self.w(DESCRIPTOR,int(self.metadata['spartan']['datum'],16))
        self.sh(DESCRIPTOR+0x64,-1)
        # F1420's exact default request at F1C72..F1C8A: current,current,0.
        self.g.mem_write(REQUEST,struct.pack('<3I',0x07000101,0x07000101,0));self.g.mem_write(FAILURE,bytes(4))

    def snapshot(self):
        extension=self.u(UNIT+0x33E,2);extra=self.u(UNIT+0x342,2);control=self.u(UNIT+0x346,2)
        manager=UNIT+MANAGER_OFFSET
        return {'manager_hex':bytes(self.g.mem_read(manager,0x90)).hex(),
            'primary_graph_datum':f'{self.u(manager):08x}',
            'primary_packed_animation':f'{self.u(manager+4):08x}',
            'primary_frame_bits':f'{self.u(manager+0x1C):08x}',
            'primary_frame':self.fv(manager+0x1C),'primary_flags':f'{self.u(manager+0x12,2):04x}',
            'object_state_hex':bytes(self.g.mem_read(UNIT,0x34C)).hex(),
            'unit_extension_offset':extension,'unit_extension_hex':bytes(self.g.mem_read(UNIT+extension,0x124)).hex(),
            'unit_extra_offset':extra,'unit_extra_hex':bytes(self.g.mem_read(UNIT+extra,0x58)).hex(),
            'unit_control_offset':control,'unit_control_hex':bytes(self.g.mem_read(UNIT+control,0x44)).hex(),
            'local_transforms_hex':bytes(self.g.mem_read(UNIT+0x3000,self.node_count*32)).hex(),
            'current_node_matrices_hex':[bytes(self.g.mem_read(UNIT+POSE_OFFSET+i*52,52)).hex() for i in range(self.node_count)],
            'region_state_hex':bytes(self.g.mem_read(UNIT+PERM_OFFSET,self.region_count*10)).hex(),
            'rng_seed':f'{self.u(SEED):08x}'}

    def hook(self,g,a,size,user):
        if getattr(self,'live_active',False):
            sp=g.reg_read(UC_X86_REG_ESP)
            for ret,row in reversed(self.live_returns[:]):
                if ret==a:
                    row['return_eax']=f'{g.reg_read(UC_X86_REG_EAX):08x}'
                    self.live_returns.remove((ret,row));break
            if a in (0x1CE8C0,0x1CE9B0,0x1CEAC0,0x1D0A60,0x1CFDE0,0x1DEB60,
                     0x1CEE00,0xC52C0,0xBD270,0x110A20,0x110BF0,0x114BD0,
                     0x1D0ED0,0xE7E60,0x112E10,0x1CF100,0x1CA2B0,0x1CA090,
                     0x1C9F50,0xBDF80,0xBEA50,0x1D0F80,0x1CA950,
                     0x1E11B0,0x280030,0x27FB50,0x27FB10,0x113830,0xBB6E0,
                     0xBB5E0,0x16F0E0,0x16F550,0x16F760):
                row={'phase':self.active,'entry':f'{a:08x}','registers':self.registers(),
                    'stack':[f'{self.u(sp+4+i*4):08x}' for i in range(7)]}
                if a==0x1CA950:row['channel_hex']=bytes(g.mem_read(g.reg_read(UC_X86_REG_EBX),32)).hex()
                if a==0x27FB10:
                    data=self.u(0x50BC70)
                    row['source_sampler_dispatch']={'codec_pointer':f'{data:08x}',
                        'codec_id':self.u(data,1),'codec_header_hex':bytes(g.mem_read(data,32)).hex(),
                        'sampling_mode':self.u(0x50BC40),'node_count':self.u(0x50BC4C),
                        'sample_frame':self.u(0x50BC54),'next_frame':self.u(0x50BC58),
                        'interpolation_bits':f'{self.u(0x50BC5C):08x}',
                        'decoder_targets':[f'{self.u(at):08x}' for at in (0x50BC74,0x50BC78,0x50BC7C)],
                        'component_mask_pointers':[f'{self.u(at):08x}' for at in (0x50BC80,0x50BC84,0x50BC88)]}
                self.live_events.append(row);self.live_returns.append((self.u(sp),row))
            if a==0x124F30:
                descriptor=g.reg_read(UC_X86_REG_EAX)
                assert descriptor in self.resident,('unmapped original resource descriptor',hex(descriptor))
                resource=self.resident[descriptor]
                self.live_events.append({'phase':self.active,'service':'00124f30',
                    'description':'supplied resident exact local-cache bytes; resource scheduler not executed',
                    'descriptor_pointer':f'{descriptor:08x}',
                    'tag_argument':f'{self.u(sp+4):08x}',**resource})
                g.reg_write(UC_X86_REG_EAX,resource['resident_pointer'])
                g.reg_write(UC_X86_REG_ESP,sp+8);g.reg_write(UC_X86_REG_EIP,self.u(sp))
                return
        super().hook(g,a,size,user)

    def mem_write(self,g,access,address,size,value,user):
        if getattr(self,'live_active',False) and address==SEED+4:
            self.events.append({'tick':self.tick,'phase':self.active,
                'local_rng_write_instruction':f'{g.reg_read(UC_X86_REG_EIP):08x}',
                'before':f'{self.u(SEED+4):08x}','after':f'{value&0xFFFFFFFF:08x}'})
        super().mem_write(g,access,address,size,value,user)

    def initialize(self):
        # Original default region init remains independent/frozen.  The valid
        # graph manager and type initializer below are new original executions.
        self.sh(UNIT+0x12A,-1);super().execute();self.sh(UNIT+0x12A,MANAGER_OFFSET)
        self.live_active=True;manager=UNIT+MANAGER_OFFSET
        self.active='original_manager_constructor';self.run(0x1CE8C0,regs=((UC_X86_REG_EAX,manager),))
        before=self.snapshot()
        self.active='original_manager_bind'
        self.run(0x1CEAC0,(1,),((UC_X86_REG_ESI,manager),
            (UC_X86_REG_EDI,int(self.live_metadata['graph']['datum'],16)),
            (UC_X86_REG_EDX,int(self.pose_metadata['model']['datum'],16))))
        assert self.g.reg_read(UC_X86_REG_EAX)&0xFF==1
        bound=self.snapshot()
        # B8A30 sets bit11 after a successful1CEAC0 before unit constructors.
        self.w(UNIT+4,self.u(UNIT+4)|0x800)
        self.active='original_unit_type_constructor';start=len(self.events)
        self.run(0xC52C0,(UI,DESCRIPTOR,FAILURE))
        assert self.g.reg_read(UC_X86_REG_EAX)&0xFF==1
        initialized=self.snapshot();services=[e for e in self.events[start:] if 'service' in e]
        assert len(services)==3 and all(e['service']=='0018fd70' for e in services)
        clip=(self.u(manager+4)>>16)&0xFFFF
        selected=self.live_metadata['animations'][clip]
        return {'manager_before_bind':before,'manager_bound':bound,'unit_initialized':initialized,
                'selected_animation':selected,'original_type_constructor_services':services,
                'supplied_spawn_descriptor_hex':bytes(self.g.mem_read(DESCRIPTOR,0xC4)).hex(),
                'supplied_initial_object_extent':'3a00','source_allocated_fields':[
                    {'field':f'{f:04x}','size':self.u(UNIT+f,2),'offset':self.u(UNIT+f+2,2)} for f in (0x33C,0x340,0x344)]}

    def execute_live(self,ticks=45):
        initial=self.initialize();rows=[];manager=UNIT+MANAGER_OFFSET
        for tick in range(ticks):
            self.tick=tick;self.w(TIME+8,tick);start=len(self.live_events);ordinary_start=len(self.events)
            before=self.snapshot()
            self.active='original_manager_blend_tick';self.run(0x1D0ED0,regs=((UC_X86_REG_ESI,manager),))
            after_blend=self.snapshot()
            self.active='original_active_action_dispatch';self.run(0xE7E60,(UI,))
            after_actions=self.snapshot()
            self.active='original_unit_animation_tick';self.run(0x112E10,(UI,REQUEST))
            after_animation=self.snapshot()
            self.active='original_valid_graph_current_pose';self.pose_active=True
            self.run(0xBDF80,(UI,));self.pose_active=False
            after_pose=self.snapshot()
            services=[e for e in self.events[ordinary_start:] if 'service' in e]
            assert not services,services
            rows.append({'tick':tick,'supplied_request_hex':bytes(self.g.mem_read(REQUEST,12)).hex(),
                'before':before,'after_blend':after_blend,'after_actions':after_actions,'after_animation':after_animation,'after_pose':after_pose,
                'live_event_range':[start,len(self.live_events)]})
        self.live_active=False
        query=self.query((-2.,0.,.7),(4.,0.,0.),list(bytes(self.g.mem_read(UNIT+PERM_OFFSET,4))))
        centroid_queries=[]
        for bsp in self.metadata['bsps']:
            node=bsp['node_index'];matrix=struct.unpack('<13f',self.g.mem_read(UNIT+POSE_OFFSET+node*52,52))
            point=bsp['authored_vertex_average_fixture']
            # Explicit ray fixtures derived from original output matrices and
            # authored vertex averages. Python constructs inputs only; original
            # instructions supply every query expected result.
            center=[matrix[10+k]+matrix[0]*sum(matrix[1+j*3+k]*point[j] for j in range(3)) for k in range(3)]
            direction=[matrix[0]*matrix[1+k] for k in range(3)]
            q=self.query(tuple(center[k]-2*direction[k] for k in range(3)),
                         tuple(4*direction[k] for k in range(3)),
                         list(bytes(self.g.mem_read(UNIT+PERM_OFFSET,4))))
            centroid_queries.append({'target_region':bsp['region_index'],'target_node':node,
                'fixture_authority':'Explicit posed-node-axis ray through authored vertex-average fixture',
                'original_query':q})
        return {'initialization':initial,'rows':rows,'events':self.live_events,
            'options_state':self.u(GAME+8),'options_mode':self.game_mode,'simulation_world_state':self.u(WORLD+8),
            'produced_pose_collision':query,'produced_pose_centroid_queries':centroid_queries,'summary':{'ticks':len(rows),
                'distinct_primary_frame_bits':len({r['after_animation']['primary_frame_bits'] for r in rows}),
                'distinct_pose_outputs':len({r['after_pose']['local_transforms_hex'] for r in rows}),
                'resident_resource_calls':sum('service' in e for e in self.live_events),
                'posed_centroid_queries':len(centroid_queries),'posed_centroid_hits':sum(q['original_query']['returned_hit'] for q in centroid_queries),
                'rng_stores':sum('rng_write_instruction' in e for e in self.events),
                'local_rng_stores':sum('local_rng_write_instruction' in e for e in self.events)},
            'supplied_engine_boundaries':['Existing explicit object placement/arena and inactive parent/physics/attachment',
                'Three successful reserved-capacity heap growth calls; original BD270 allocates and zeroes fields',
                'Resident exact local animation groups returned at124F30; original resource scheduler not executed',
                'Exact F1420 default request current,current,0 before E7E60/112E10; upstream physics/control state producers not executed'],
            'scope':'Actual original valid manager and unit initialization, original animation clock/state lookup, original animation sampler/unit local callback/world matrices/posed collision. Explicit startup/current request slice. Not ordinary equipped moving multiplayer player selection, complete physics, resource scheduler or Xbox reciprocal hardware proof.'}

    def regions(self):
        self.initialize();rows=[]
        for name,entry,args in [('death_flag_set',0xBB6E0,(UI,-1,2,-1)),
                               ('death_flag_clear',0xBB6E0,(UI,-1,2,0))]+[
                               (f'explicit_damage_state-{region}-{state}',0xBB5E0,(UI,region,state,0))
                               for region in range(4) for state in range(6)]:
            self.active=name;before=self.snapshot();start=len(self.events)
            self.run(entry,args);after=self.snapshot()
            services=[e for e in self.events[start:] if 'service' in e];assert not services,services
            rows.append({'name':name,'entry':f'{entry:08x}','arguments':[f'{v&0xFFFFFFFF:08x}' for v in args],
                         'input_authority':'Original death/reset flag call arguments' if name.startswith('death') else 'Explicit varied damage-state argument fixture; not claimed live event',
                         'before_region_hex':before['region_state_hex'],'after_region_hex':after['region_state_hex'],
                         'unchanged_region_state':before['region_state_hex']==after['region_state_hex'],
                         'rng_before':before['rng_seed'],'rng_after':after['rng_seed']})
        return {'cases':rows,'events':self.live_events,'model_variants':self.live_metadata['model_variants'],
                'options_state':self.u(GAME+8),'options_mode':self.game_mode,'simulation_world_state':self.u(WORLD+8),
                'scope':'Executed original region state selectors on actual stock default Spartan variant. Damage state0..5 are explicitly varied fixtures; death/reset arguments come from original D8070/D73E0 callers. No general variant or dismemberment claim.'}


def inventory(directory):
    directory=directory.resolve()
    directory.mkdir(parents=True,exist_ok=True)
    x=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');assert hashlib.sha256(x.data).hexdigest()==PIN
    code=_Code(x,'.text');starts={int(r['address'],16) for r in csv.DictReader((ROOT/'analysis/map-pack-update/functions.tsv').open(),delimiter='\t')}
    entries=(0x1CE8C0,0x1CE9B0,0x1CEAC0,0x1D0A60,0x1CFDE0,0x1DEB60,0x1CEE00,
        0xC52C0,0xBD270,0x110A20,0x110BF0,0x114BD0,0x112E10,0x1CF100,0x1CA2B0,
        0x1CA090,0x1C9F50,0x1E11B0,0x1E1530,0x124F30,0x280030,0x27FB50,0x27FB10,
        0x113830,0xF1420,0xE7E60,0xBB6E0,0xBB5E0,0x16F0E0,0xD8070,0xD73E0,
        0x280670,0x2806E0,0x280750,0x2807C0,0x280830)
    report_path=ROOT/'analysis/multiplayer/map-pack/spartan-live-pose-1.1-oracle.json'
    if report_path.exists():
        report=json.loads(report_path.read_text());assert report['xbe_sha256']==PIN
        actual_targets={int(target,16) for e in report['events'] if 'source_sampler_dispatch' in e
            for target in e['source_sampler_dispatch']['decoder_targets']}
        entries=tuple(sorted(set(entries)|actual_targets))
    rows=[]
    for start in entries:
        f=_trace(code,start,starts|set(entries));inst=[code.at(a) for a in sorted(f.code)]
        p=directory/f'{start:08x}.asm';p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n' for i in inst))
        rows.append({'start':f'{start:08x}','end_exclusive':f'{f.end:08x}','instruction_count':len(inst),
            'listing':str(p.relative_to(ROOT)),'listing_sha256':hashlib.sha256(p.read_bytes()).hexdigest(),
            'reachable_instruction_bytes_sha256':hashlib.sha256(b''.join(i.bytes for i in inst)).hexdigest(),
            'direct_calls':[{'site':f'{a:08x}','target':f'{b:08x}'} for a,b,k in f.sites if k=='call']})
    (directory/'inventory.json').write_text(json.dumps({'xbe_sha256':PIN,'oracle_tool_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'routines':rows},indent=2)+'\n')


def main():
    assert hashlib.sha256((ROOT/'tools/oracle_spartan_default_pose_1_1.py').read_bytes()).hexdigest()==DEFAULT_ORACLE_PIN
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer/map-pack/spartan-live-pose-1.1-oracle.json')
    p.add_argument('--ticks',type=int,default=75);p.add_argument('--regions',action='store_true')
    p.add_argument('--mode',type=int,choices=(1,5),default=5)
    p.add_argument('--instruction-inventory',type=Path);args=p.parse_args()
    if args.instruction_inventory:inventory(args.instruction_inventory);return
    t=LivePoseTrace(args.mode);report=t.regions() if args.regions else t.execute_live(args.ticks)
    report.update(xbe_sha256=PIN,cache_sha256=CACHE_PIN,
        oracle_tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),default_oracle_dependency_sha256=DEFAULT_ORACLE_PIN,
        authored_graph=t.live_metadata['graph'])
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report.get('summary',{'cases':len(report['cases'])}) if 'cases' in report else report['summary']))


if __name__=='__main__':main()
