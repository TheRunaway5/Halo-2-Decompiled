#!/usr/bin/env python3
"""Compare private TU1.5 E0900 flight property query with unchanged original i386."""
import argparse,copy,hashlib,json,os,platform,shlex,subprocess
from dataclasses import dataclass
from pathlib import Path
from typing import Callable
from oracle_smg_firing_1_1 import Xbe
from validate_native_projectile_function_query_frame_tu15 import boundary_packet,corpus as projectile_inputs
from validate_native_tu15_prt_helpers import PIN,physical_original

ROOT=Path(__file__).resolve().parents[1]
FRAME='src/gameplay/objects/native_tu15_flight_property_query_frame.hpp'
TEST='tests/native_tu15_flight_property_query_frame_tests.cpp'
ENTRY=0xe0900
REGIONS=((0x46a000,4096),(0x4ec000,4096),(0x4f0000,4096),(0x61000000,16384),(0x62000000,4096))

@dataclass(frozen=True)
class FrameProfile:
    entry:int
    sites:int
    frame:str
    driver:str
    kernel:str
    regions:tuple[tuple[int,int],...]
    inputs:Callable
    before_x87:tuple[tuple[int,int],...]
    mutants:tuple[tuple[str,str,str],...]
    # A store with residual live x87 state needs its actual next-PC trace.
    after_x87:tuple[tuple[int,int],...]=()
    extra_pins:tuple[str,...]=()
    image_sha256:str=PIN
    # Optional complete direct child edges for integer-only compositions.
    child_edges:tuple[tuple[int,int,int,int],...]=()
    explicit_reads:tuple[tuple[int,bytes],...]=()

def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()

def corpus(image):
    rows=projectile_inputs(image,True)[:144]
    keys=(0x6000542,0x70000c9,0xc0005a4,0xc0005a4,0,0xc0005a4)
    limits=(0x3f800000,0x40000000,0x3f000000,0,0x80000000,0xbf800000,0x7f800000,0x7fc01234)
    values=(0,0x80000000,0x3e800000,0xbe800000,0x40000000,0x40400000,
            0x00000001,0x7f7fffff,0x7f800000,0x7fc0a55a,0x7fa05678,0x3f800000)
    for n,row in enumerate(rows):
        banks={bank['address']:bytearray.fromhex(bank['before'])for bank in row['banks']}
        def locate(address,width):
            for base,data in banks.items():
                if base<=address<=base+len(data)-width:return data,address-base
            raise ValueError('E0900 fixture address escaped bank')
        def get(address):
            data,at=locate(address,4);return int.from_bytes(data[at:at+4],'little')
        def put(address,value,width=4):
            data,at=locate(address,width);data[at:at+width]=value.to_bytes(width,'little')
        key=keys[n%len(keys)];sp=row['gp'][4];i=n//len(keys)
        slots=get(get(0x4ecbd0)+0x44);body=get((slots+(row['matrix']&0xffff)*12+8)&0xffffffff)
        definition=get(get(0x4f0414)+(get(body)&0xffff)*16+8)
        row.update(id=n,name=f'flight-query-{n}',entry=ENTRY,source=key,max_steps=150)
        put(sp+8,key)
        put(body+0x3dc,2 if i%2==0 else 1,1)
        put(body+0x39d,1 if i%2==0 else 0,1)
        limit=limits[(i+n%6)%len(limits)] if n>=48 else limits[i%3]
        put(definition+0x2dc,limit)
        for axis,offset in enumerate((0x88,0x8c,0x90)):
            put(body+offset,values[(i+3*axis+n%6)%len(values)] if n>=48 else values[(i+axis)%6])
        if key==0x6000542 and i%4==0:
            # Declared mapped DATA variation reaches the negative clamp.
            put(0x46a0d0,0xbf800000)
        if key==0xc0005a4:row['live_x87']=row['live_x87'][:i%4]
        row['banks']=[dict(address=base,before=banks[base].hex())for base,_ in REGIONS]
    return rows

FLIGHT=FrameProfile(ENTRY,76,FRAME,TEST,'include/halo2/native_tu15_flight_property_query_kernel.hpp',REGIONS,corpus,
    ((0xe0975,1),(0xe097b,2),(0xe0999,1),(0xe099f,1)),(
        ('skip-sqrt','0xde,0xc1,0xd9,0xfa,0xdd,0xdb','0xde,0xc1,0x90,0x90,0xdd,0xdb'),
        ('wrong-divide','fdivs %0','fmuls %0'),
        ('missed-spill-pop','fp_.stage_single(true)','fp_.stage_single(false)'),
        ('skip-live-limit-reread','const auto limit=read(0xe0999u,s_.gp[2]+0x2dcu);',
         'const auto limit=std::bit_cast<std::uint32_t>(_mm_cvtss_f32(s_.xmm[0]));'),
        ('early-spill-retirement','Base::store(0xe099fu,s_.gp[4]+8u,pending.bits,[&]{fp_.commit(pending);},true);',
         'fp_.commit(pending);Base::store(0xe099fu,s_.gp[4]+8u,pending.bits,[]{},true);'),
    ))

def main(profile=FLIGHT):
    label=f'{profile.entry:X}'
    p=argparse.ArgumentParser(description=f'Compare private {label} with unchanged original i386.')
    p.add_argument('--xbe',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--compiler',action='append');p.add_argument('--sanitizers',action='store_true');p.add_argument('--prefixes',action='store_true')
    p.add_argument('--observers',action='store_true');p.add_argument('--semantic-mutants',action='store_true')
    a=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Use tools/run_test.py')
    if platform.system()!='Linux' or platform.machine().lower()not in('x86_64','amd64','i386','i686')or not a.xbe.is_file():return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);a.output.mkdir(parents=True,exist_ok=False)
    names=(profile.frame,profile.driver,profile.kernel,'tests/native_tu15_object_property_frame_test_harness.hpp',
        'tools/validate_native_flight_property_query_frame_tu15.py','tools/validate_native_projectile_function_query_frame_tu15.py',
        'tools/validate_native_tu15_prt_helpers.py','tools/oracle_native_campaign_screen_lifecycle_tu15.py',
        'tools/observe_native_campaign_ai_vector_math_tu15.cpp',*profile.extra_pins)
    pins={name:sha(ROOT/name) for name in names}
    headers={path.resolve():sha(path) for tree in ('include','src','tests') for path in (ROOT/tree).rglob('*') if path.suffix in ('.hpp','.h','.inc')}
    report=dict(passed=False,production_bound=False,source_before=pins,owned_scratch=str(scratch),retained_large_artifacts=[])
    try:
        if sha(a.xbe)!=profile.image_sha256:raise ValueError('Wrong original image for '+label)
        report['image_sha256']=profile.image_sha256
        import oracle_native_campaign_screen_lifecycle_tu15 as common
        image=Xbe(a.xbe);code=common._Code(image,'.text');initial=common.metadata
        entries=(profile.entry,*dict.fromkeys(target for _,target,_,_ in profile.child_edges))
        for pc,target,resume,_ in profile.child_edges:
            raw=image.read(pc,5)
            if raw[0]!=0xe8 or pc+5!=resume or resume+int.from_bytes(raw[1:],'little',signed=True)!=target:
                raise ValueError(f'Original {label} direct child edge changed at {pc:x}')
        explicit_reads=dict(profile.explicit_reads)
        def metadata(image):
            result=initial(image)
            for site in result['sites']:
                pc=site['instruction'];instruction=code.at(pc)
                if instruction.mnemonic in ('pop','ret'):
                    site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
                if pc in explicit_reads:
                    if bytes(instruction.bytes)!=explicit_reads[pc] or len(site['operands'])!=1 or site['operands'][0]['write']:
                        raise ValueError(f'Original {label} explicit read changed at {pc:x}')
                    site['operands'][0]['read']=True
            return result
        common.metadata=metadata;common.ENTRIES=entries
        sites=metadata(image)['sites']
        child_sites={target:count for _,target,_,count in profile.child_edges}
        call_pcs={pc for pc,_,_,_ in profile.child_edges}
        if len(sites)!=profile.sites+sum(child_sites.values()) or {
            site['instruction'] for site in sites if code.at(site['instruction']).mnemonic=='call'
        }!=call_pcs:
            raise ValueError(f'Original {label} authenticated site and child extent changed')
        deltas=dict(profile.before_x87)
        traces=[dict(entry=site['instruction'],arguments=0,gp=True,xmm=True,x87_delta=deltas.get(site['instruction'],0))
                for site in sites if site['operands'] or code.at(site['instruction']).mnemonic in ('push','call')]
        rows=profile.inputs(image);normal=len(rows)
        def observe(input_rows,folder,balanced=False):
            return physical_original(image,input_rows,folder,entries=entries,regions=profile.regions,
                trace_calls=traces,trace_extended=True,preserve_x87_prefix=balanced)
        observed,authority=observe(rows,scratch);report['authority']=authority
        if profile.child_edges:
            genuine={}
            for pc,target,resume,count in profile.child_edges:
                total=0
                for case in observed['cases']:
                    history=case['executed_sites'];starts=history.count(pc)
                    if starts!=history.count(target) or starts!=history.count(resume):
                        raise ValueError(f'Original {label} child start or return changed')
                    total+=starts
                if not total:raise ValueError(f'Original {label} corpus did not execute its genuine child')
                genuine[hex(pc)]=dict(target=hex(target),resume=hex(resume),returns=total,sites=count)
            report['genuine_direct_children']=genuine
        if a.prefixes:
            stopped=[]
            for row,actual in zip(rows[:30],observed['cases'][:30],strict=True):
                for phase,key in (('before_read','reads'),('before_write','writes')):
                    for ordinal in range(len(actual[key])):
                        candidate=copy.deepcopy(row);candidate.update(id=len(stopped),name=row['name']+f'-{phase}-{ordinal}',
                            fault=dict(phase=phase,ordinal=ordinal));stopped.append(candidate)
            folder=scratch/'provider-prefixes';folder.mkdir()
            prefix,receipt=observe(stopped,folder);rows+=stopped;observed['cases']+=prefix['cases'];report['prefix_authority']=receipt
        provider_prefix_count=len(rows)-normal
        if a.observers:
            stopped=[];origins=[]
            for index,(row,actual) in enumerate(zip(rows[:normal],observed['cases'][:normal],strict=True)):
                for ordinal in range(len(actual['writes'])):
                    candidate=copy.deepcopy(row)
                    candidate.update(id=len(stopped),name=row['name']+f'-observer-throw-{ordinal}',
                        fault=dict(phase='after_write',ordinal=ordinal),observer=True)
                    stopped.append(candidate);origins.append((index,ordinal,actual['writes'][ordinal]['instruction']))
            next_reads=dict(profile.after_x87);all_retired=[None]*len(stopped);authority={}
            for group,balanced in (('balanced',True),('live-x87',False)):
                indices=[i for i,(_,_,pc)in enumerate(origins)if (pc not in next_reads)==balanced]
                if not indices:continue
                folder=scratch/('retired-stores-'+group);folder.mkdir()
                retired,authority[group]=observe([stopped[i]for i in indices],folder,balanced)
                for i,case in zip(indices,retired['cases'],strict=True):all_retired[i]=case
            states=[[]for _ in range(normal)]
            for row,actual,(index,ordinal,pc) in zip(stopped,all_retired,origins,strict=True):
                if actual['completed'] or actual['boundary']['phase']!='after_write' or actual['boundary']['instruction']!=pc or len(actual['writes'])!=ordinal+1:
                    raise ValueError(f'Original {label} retired store frontier changed')
                packet=boundary_packet(actual,row,pc)
                if pc in next_reads:
                    # The other value remains live after this spill. Recover
                    # it from the actual original next instruction, before
                    # that read can touch memory, and reconcile both runs.
                    following=[p for p in observed['cases'][index]['calls']if p['entry']==next_reads[pc]]
                    if len(following)!=1 or any(following[0][k]!=packet[k]for k in ('gp','eflags','xmm','fp')):
                        raise ValueError(f'Original {label} live x87 observer frontier changed')
                    packet['live_x87']=following[0]['live_x87']
                states[index].append(packet)
                row['observer_states']=copy.deepcopy(states[index])
            for row,state in zip(rows[:normal],states,strict=True):row.update(observer=True,observer_states=state)
            rows+=stopped;observed['cases']+=all_retired;report['observer_authority']=authority
            report['original_retired_store_boundaries']=len(stopped)
        coverage=set()
        for row,actual in zip(rows,observed['cases'],strict=True):
            if actual['completed']!=('fault' not in row):raise ValueError('Original completion changed '+row['name'])
            coverage.update(actual['executed_sites'])
            for packet in actual['calls']:
                if packet.pop('arguments')!=[]:raise ValueError('Unexpected original call arguments')
            pc=actual['boundary']['instruction'] if 'fault'in row else actual['executed_sites'][-1]
            expected=boundary_packet(actual,row,pc)
            if 'fault'in row and row['fault']['phase']!='after_write':
                packet=actual['calls'][-1]
                if any(packet[key]!=expected[key]for key in ('entry','gp','eflags','xmm','fp')):
                    raise ValueError('Original fault and exact pre-I/O x87 observation differ')
                expected['live_x87']=packet['live_x87']
            elif 'fault'in row:
                expected['live_x87']=row['observer_states'][-1]['live_x87']
            row.update(expected=expected,reads=actual['reads'],writes=actual['writes'],calls=actual['calls'])
            for bank,after in zip(row['banks'],actual['banks'],strict=True):bank['after']=after['after']
        missing=sorted({site['instruction']for site in sites}-coverage)
        report['uncovered_sites']=[hex(pc)for pc in missing]
        if missing:raise ValueError(f'Original {label} site coverage incomplete '+str(report['uncovered_sites']))
        report.update(original_sites=profile.sites,covered_sites=len(coverage)-sum(child_sites.values()),original_returns=normal,
            original_provider_prefixes=provider_prefix_count,original_total_prefixes=len(rows)-normal)
        if profile.child_edges:report['combined_covered_sites']=len(coverage)
        fixture=scratch/'fixture.json';fixture.write_text(json.dumps(rows,separators=(',',':')));report['compilers']=[]
        for compiler in a.compiler or ('g++','clang++'):
            binary=scratch/Path(compiler).name;dep=binary.with_suffix('.d')
            command=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
                *('-I'+str(ROOT/tree) for tree in ('include','src','tests')),str(ROOT/profile.driver),'-o',str(binary),'-MD','-MF',str(dep)]
            if a.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
            built=subprocess.run(command,capture_output=True,text=True,timeout=600)
            if built.returncode:raise ValueError(f'{label} query compile failed: '+built.stderr[-7000:])
            for item in shlex.split(dep.read_text().replace(chr(92)+'\n',' ').split(':',1)[1]):
                path=Path(item).resolve()
                if path in headers:pins[str(path.relative_to(ROOT))]=headers[path]
            result=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,timeout=1200)
            if result.returncode:raise ValueError(f'{label} query comparison failed: '+(result.stdout+result.stderr)[-9000:])
            mutations=[]
            if a.semantic_mutants:
                original=(ROOT/profile.frame).read_text()
                for defect,old,new in profile.mutants:
                    if original.count(old)!=1:raise ValueError(f'Missing {label} defect seam '+defect)
                    overlay=scratch/(Path(compiler).name+'-'+defect);target=overlay/profile.frame;target.parent.mkdir(parents=True)
                    target.write_text(original.replace(old,new,1));changed=list(command);mutant=overlay/'bad'
                    changed.insert(1,'-I'+str(overlay/'src'));changed[changed.index('-o')+1]=str(mutant)
                    built=subprocess.run(changed,capture_output=True,text=True,timeout=600)
                    if built.returncode:raise ValueError(f'{label} defect failed to compile '+defect+': '+built.stderr[-6000:])
                    tested=subprocess.run([str(mutant),str(fixture)],capture_output=True,text=True,timeout=1200)
                    detail=(tested.stdout+tested.stderr)[-6000:]
                    if not tested.returncode or 'differ'not in detail:raise ValueError(f'{label} defect survived/unrecognized '+defect+': '+detail)
                    mutations.append(dict(name=defect,rejected=True,diagnostic=detail))
            report['compilers'].append(dict(compiler=compiler,command=command,result=json.loads(result.stdout),compiled_mutations=mutations))
        report['passed']=True
    except BaseException as error:report['error']=repr(error);raise
    finally:
        report['source_after']={name:sha(ROOT/name) for name in pins}
        if report['source_after']!=pins:report.update(passed=False,source_drift=True)
        (a.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    return 0 if report['passed'] else 1

if __name__=='__main__':raise SystemExit(main())
