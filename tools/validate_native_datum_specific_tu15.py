#!/usr/bin/env python3
"""Actual16E860 shared record initialization and complete retained ABI."""
import argparse,copy,hashlib,json,os,platform,random,re,struct,subprocess,sys
from pathlib import Path
from generated_routine_ownership import function_bodies
from validate_native_tu15_prt_helpers import PIN,physical_original
from native_datum_initialize_replacements import replace_datum_initialize,canonical_body as canonical_initializer
from native_datum_specific_replacements import canonical_body,replace_datum_specific
from validate_native_datum_initialize_tu15 import partial_source

ROOT=Path(__file__).resolve().parents[1]
COPIES=('src/physics/physics_live_support_tu15_generated.cpp','src/rendering/shadows/source_shadow_frame_tu15_generated.cpp')
REGIONS=((0x1000000,32768),(0x62000000,16384))
INPUTS=COPIES+('include/halo2/native_tu15_datum_initialize_kernel.hpp','src/physics/runtime/native_tu15_datum_specific.hpp','tools/native_datum_initialize_replacements.py','tools/native_datum_specific_replacements.py','include/halo2/native_tu15_datum_specific_kernel.hpp','tests/native_tu15_datum_specific_abi_tests.cpp','tools/validate_native_datum_specific_tu15.py')
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()

def corpus():
    rng=random.Random(0x16e770);rows=[]
    live=('0000000000000080ff3f','00000000000000000080','35c26821a2da0fc90040','45230100000000c0ff7f','0100000000000080ff7f','0000000000000080ff7f','00000000000000800040','00000000000000800140')
    def add(name,index=0,generation=1,stride=4,backwards=False,capacity=None,tag=0,pool_offset=None,record_offset=None,bitmap_field=None,record_field=None,counter=None):
        n=len(rows);banks=[bytearray(rng.randbytes(size))for _,size in REGIONS];sp=0x62002000+n%4
        pool=0x1000100 if pool_offset is None else sp-pool_offset
        record=0x1002000 if record_offset is None else sp+record_offset
        if record_field is not None:record=pool+record_field
        records=(record-index*stride)&0xffffffff;bitmap_word=0x1001800 if bitmap_field is None else pool+bitmap_field
        bitmap=(bitmap_word-(index>>5)*4)&0xffffffff
        def put(at,value,width=4):
            for (base,size),bank in zip(REGIONS,banks):
                if base<=at and at+width<=base+size:bank[at-base:at-base+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little');return
            raise ValueError('Specific datum input escaped declared banks')
        def get(at):
            for (base,size),bank in zip(REGIONS,banks):
                if base<=at<=base+size-4:return struct.unpack_from('<I',bank,at-base)[0]
            raise ValueError('Specific datum stack input escaped declared banks')
        gp=[rng.getrandbits(32)for _ in range(8)];gp[0]=(generation<<16)|index;gp[2]=pool;gp[4]=sp
        if record_offset in (-4,-8,-12):gp[{-4:3,-8:7,-12:6}[record_offset]]&=0xffff0000
        if pool_offset==0x4c:gp[7]=records;gp[3]=bitmap
        if pool_offset==0x54:gp[6]=bitmap
        for offset,value in ((0x20,index+1 if capacity is None else capacity),(0x24,stride),(0x38,index if n%2 else index+2),(0x3c,(0,0xffffffff,0x7fffffff,0x80000000)[n%4]),(0x44,records),(0x48,bitmap)):put(pool+offset,value)
        put(pool+0x40,(0,0xfffe,0xffff)[n%3]if counter is None else counter,2)
        if bitmap_field is None:put(bitmap_word,rng.getrandbits(32))
        put(record,tag,2);put(sp,0x63000000)
        rows.append(dict(id=n,name=name,entry=0x16e770,gp=gp,matrix=get(sp+4),source=get(sp+8),eflags=0x202|(rng.getrandbits(12)&0x8d5)|(0x400 if backwards else 0),
            xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],before_fp=[(0x7f,0x27f,0x37f)[n%3]|((n%4)<<10),((n%8)<<11)|(n%64)|((n*0x100)&0x4700),0x1f80|((n%4)<<13)|(n%64)],
            live_x87=list(live[:(0,1,4,8)[n%4]]),max_steps=10000,banks=[dict(address=base,before=bank.hex())for (base,_),bank in zip(REGIONS,banks)]))
    for index in (0,1,31,32,255,65535):
        for stride in (0,1,2,3,4,5,8):
            for generation in (0,1,0x7fff,0x8000,0xfffe,0xffff):
                for backwards in (False,True):add(f'index-{index}-stride-{stride}-generation-{generation:x}-DF{int(backwards)}',index,generation,stride,backwards)
    for backwards in (False,True):
        for capacity in (0,0xffffffff,0x80000000):add(f'bounds-{capacity:x}-DF{int(backwards)}',capacity=capacity,backwards=backwards)
        for tag in (1,0x8000,0xffff):add(f'occupied-{tag:x}-DF{int(backwards)}',tag=tag,backwards=backwards)
        for offset in (-4,-8,-12,-20,-24,-28):add(f'record-stack-{offset}-DF{int(backwards)}',record_offset=offset,backwards=backwards,generation=0x1234,counter=0xfffe)
        for offset in (0x4c,0x54,0x58):add(f'pool-stack-{offset:x}-DF{int(backwards)}',pool_offset=offset,backwards=backwards)
        for field in (0x38,0x3c,0x44):add(f'bitmap-field-{field:x}-DF{int(backwards)}',index=1,bitmap_field=field,backwards=backwards)
        for field in (0x38,0x3c,0x40):add(f'record-field-{field:x}-DF{int(backwards)}',record_field=field,backwards=backwards)
    for mode in range(48):
        row=copy.deepcopy(rows[mode%24]);n=len(rows);row.update(id=n,name=f'masked-neutral-controls-{mode:x}')
        row['before_fp']=[(0x7f,0x27f,0x37f)[n%3]|((n%4)<<10),(n%8)<<11,0x1f80|((n%4)<<13)|(0x40 if n&1 else 0)|(0x8000 if n&2 else 0)]
        row['live_x87']=['0000000000000080ff3f']*(0,1,4,8)[n%4];rows.append(row)
    return rows

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--xbe',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--compiler',default='c++');p.add_argument('--sanitizers',action='store_true');p.add_argument('--require-binding',action='store_true');args=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    if sys.platform!='linux'or platform.machine().lower()not in ('x86_64','amd64','i386','i686')or not args.xbe.is_file():return 77
    if sha(args.xbe)!=PIN:raise ValueError('Wrong original TU1.5 image')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,candidate_unbound=not args.require_binding,scratch_owner=str(scratch),retained_large_artifacts=[])
    try:
        pins={path:sha(ROOT/path)for path in INPUTS};parts=[];rejections=0
        for index,path in enumerate(COPIES):
            source=(ROOT/path).read_text();current=next(b for m,b in function_bodies(source)if m[1]=='16e770');original=canonical_body(current);bound=replace_datum_specific(replace_datum_initialize(source))
            if args.require_binding and bound!=source:raise ValueError('Initializer native production binding missing: '+path)
            if replace_datum_specific(bound)!=bound:raise ValueError('Initializer binding not idempotent')
            selected=next(line for line in source.splitlines()if line.startswith('std::uint32_t selected('))
            from native_datum_initialize_replacements import PREFIX as child_prefix,INCLUDE as child_include
            from native_datum_specific_replacements import PREFIX as parent_prefix
            child=next(b for m,b in function_bodies(bound)if m[1]=='16e860');mutated_child=child.replace('s.gp[2]','s.gp[3]',1)
            dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)[0]
            mutants=[source.replace(dispatcher,dispatcher.replace('run_16e770(s);','run_16e860(s);',1),1),bound.replace(child_prefix,'',1),bound.replace(child,mutated_child,1),bound.replace(child_include,'',1),bound.replace(parent_prefix,parent_prefix.replace('::bridge','::changed_bridge'),1)]
            if 'selected('in original:mutants.append(source.replace(selected,selected.replace('return p;','return p+1u;',1),1))
            for mutant in mutants:
                if mutant in(source,bound):raise ValueError('Missing complete admission mutation witness')
                try:replace_datum_specific(mutant)
                except ValueError:rejections+=1
                else:raise ValueError('Changed requested allocator routing/source admitted')

            for shape in (original,re.sub(r'selected\((0x[0-9a-f]+u),s\.image\)',r'\1',original)):
                candidate=source.replace(current,shape,1);replace_datum_specific(replace_datum_initialize(candidate))
                for version in (candidate,replace_datum_specific(replace_datum_initialize(candidate))):
                    body=next(b for m,b in function_bodies(version)if m[1]=='16e770');changed=body.replace('s.gp[2]','s.gp[3]',1)
                    if changed==body:raise ValueError('Initializer mutation witness missing')
                    try:replace_datum_specific(version.replace(body,changed,1))
                    except ValueError:rejections+=1
                    else:raise ValueError('Changed complete initializer body admitted')
            child=next(b for m,b in function_bodies(bound)if m[1]=='16e860');parent=next(b for m,b in function_bodies(bound)if m[1]=='16e770');declaration='static void run_16e860(WorldState&);static void dispatch(WorldState&s,std::uint32_t,std::uint32_t,bool){run_16e860(s);}'
            parts.append(f'namespace bound{index} {{\n'+selected+'\n'+declaration+'\n'+child+'\n'+parent+'\n}\n')
            parts.append(f'namespace reference{index} {{\n'+selected+'\n'+declaration+'\n'+canonical_initializer(child)+'\n'+original+'\n}\n')
        from oracle_smg_firing_1_1 import Xbe
        partial,partial_proof=partial_source(Xbe(args.xbe),scratch)
        selected_partial=next(line for line in partial.splitlines()if line.startswith('std::uint32_t selected('))
        partial_dispatch=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',partial)[0]
        mutants=[partial.replace('run_16e860(s);','run_16e770(s);',1),partial.replace('case 0x12fe00u:run_12fe00(s);return;','',1),partial.replace('switch(target){','switch(target){case 0u:run_16e860(s);return;',1),partial.replace('default:s.required(site,target,tail);','default:s.required(site+1u,target,tail);',1),partial+'\nstatic void run_0(WorldState &s) {\n}\n']
        for mutant in mutants:
            if mutant==partial:raise ValueError('Partial-context mutation witness missing')
            try:replace_datum_initialize(mutant)
            except ValueError:rejections+=1
            else:raise ValueError('Changed exact physical-allocation context admitted')
        declaration='static void run_16e860(WorldState&);static void dispatch(WorldState&s,std::uint32_t,std::uint32_t,bool){run_16e860(s);}'
        parts.append('namespace partial {\n'+selected_partial+'\n'+declaration+'\n'+next(b for m,b in function_bodies(partial)if m[1]=='16e860')+'\n'+next(b for m,b in function_bodies(partial)if m[1]=='16e770')+'\n}\n')
        (scratch/'datum_specific_bodies.inc').write_text(''.join(parts))
        import oracle_native_campaign_screen_lifecycle_tu15 as common
        from oracle_smg_firing_1_1 import Xbe
        original_metadata=common.metadata
        def metadata(image):
            result=original_metadata(image);code=common._Code(image,'.text')
            for site in result['sites']:
                instruction=code.at(site['instruction'])
                if instruction.mnemonic=='pop'or instruction.mnemonic.startswith('ret'):site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
            return result
        common.metadata=metadata;image=Xbe(args.xbe);rows=corpus()
        sites=(0x16e770,0x16e771,0x16e783,0x16e788,0x16e78b,0x16e78c,0x16e794,0x16e79a,0x16e7a5,0x16e7b2,0x16e7b4,0x16e7b7,0x16e7bd,0x16e7c0,0x16e7c6,0x16e7c9,0x16e7ce,0x16e7d1,0x16e7d7,0x16e7da,0x16e7db,0x16e7dc,0x16e7dd,0x16e7de,0x16e7e2,0x16e7e3,0x16e7e4,0x16e7e8,0x16e7e9,0x16e860,0x16e863,0x16e866,0x16e86e,0x16e875,0x16e877,0x16e87b,0x16e87e,0x16e882,0x16e887,0x16e888,0x16e88b,0x16e891)
        observed,proof=physical_original(image,rows,scratch,entries=(0x16e770,0x16e860),regions=REGIONS,trace_calls=[dict(entry=pc,arguments=0,gp=True,xmm=True,x87_delta=0)for pc in sites],trace_extended=True)
        prefixes=[]
        for seed in (0,24,48,84,90,504,510,516,518,520,522,524):
            out=observed['cases'][seed]
            if not out['completed']:raise ValueError('Prefix seed did not return')
            for phase,events in (('before_read',out['reads']),('before_write',out['writes']),('after_write',out['writes'])):
                for ordinal in range(len(events)):
                    row=copy.deepcopy(rows[seed]);row.update(id=len(rows)+len(prefixes),name=row['name']+f'-{phase}-{ordinal}',fault=dict(phase=phase,ordinal=ordinal));prefixes.append(row)
        (scratch/'prefix-original').mkdir()
        stopped,prefix_proof=physical_original(image,prefixes,scratch/'prefix-original',entries=(0x16e770,0x16e860),regions=REGIONS,trace_calls=[dict(entry=pc,arguments=0,gp=True,xmm=True,x87_delta=0)for pc in sites],trace_extended=True,preserve_x87_prefix=True)
        original_count=len(rows);rows+=prefixes;observed['cases']+=stopped['cases']
        cases=[]
        for row,out in zip(rows,observed['cases'],strict=True):
            if out['completed']==('fault'in row)or not out['live_x87_unchanged']:raise ValueError('Original initializer failed normal/prefix/live80 contract')
            case=copy.deepcopy(row)
            for key in ('boundary_gp','boundary_eflags','boundary_xmm','after_fp','reads','writes'):case[key]=out[key]
            case['boundary']=out['boundary'];case['packets']=[{key:packet[key]for key in ('entry','arguments','gp','eflags','xmm','fp','live_x87')}for packet in out['calls']];case['final_pc']=out['calls'][-1]['entry']
            for before,after in zip(case['banks'],out['banks'],strict=True):before['after']=after['after']
            if row['name'].startswith('record-stack-')and'fault'not in row:
                entered=next((packet for packet in out['calls']if packet['entry']==0x16e860),None)
                if entered is None:raise ValueError('Saved-word alias fixture did not allocate')
                returned=next(packet for packet in out['calls']if packet['entry']==0x16e7ce)
                offset=int(row['name'].split('-DF')[0].removeprefix('record-stack-'))
                if offset==-20 and(returned['gp'][3]&65535)!=0xfffe:raise ValueError('Child saved-generation alias did not change the genuine result')
                if offset==-24 and returned['gp'][7]!=0xfffe:raise ValueError('Child saved-index alias did not change the genuine result')
            cases.append(case)
        fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(cases=cases,explicit_read_sites=sorted({e['instruction']for row in observed['cases']for e in row['reads']})),separators=(',',':')))
        binary=scratch/'initializer-tests';command=[args.compiler,'-std=c++20','-O2','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_datum_specific_abi_tests.cpp'),'-o',str(binary)]
        if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        compile=subprocess.run(command,capture_output=True,text=True)
        if compile.returncode:raise ValueError('Initializer compile failed: '+compile.stderr[-6000:])
        run=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,timeout=120)
        if run.returncode:raise ValueError('Initializer mismatch: '+run.stderr[-6500:])
        control=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,timeout=120,env=dict(os.environ,HALO2_REFERENCE_DATUM_SPECIFIC='1',HALO2_REFERENCE_DATUM_INITIALIZE='1',HALO2_DATUM_SPECIFIC_REFERENCE_TEST='1'))
        if control.returncode:raise ValueError('Forced requested allocation reference failed: '+control.stderr[-5000:])
        reference_control=json.loads(control.stdout)
        if not reference_control.get('native_refused')or not reference_control.get('callbacks')or not reference_control.get('parent_entries')or not reference_control.get('child_entries'):raise ValueError('Forced reference did not execute genuine parent/initializer continuations')
        if any(sha(ROOT/path)!=pin for path,pin in pins.items()):raise ValueError('Initializer proof input changed')
        executed={pc for row in observed['cases']for pc in row['executed_sites']}
        report.update(json.loads(run.stdout),source_sha256=pins,physical_allocation_context=partial_proof,binding_rejections=rejections,reference_control=reference_control,open_domains=['WorldState loader rejects unmasked FP before entry','Active MMX remains retained','Observer preimage read failures and hardware page/partial-store atomicity not qualified','Retained w(flags...) write-fault ordering is not the hardware prefix oracle'],physical_original=proof,prefix_physical_original=prefix_proof,original_normal_cases=original_count,original_prefix_cases=len(prefixes),original_sites_executed=len(executed));print(run.stdout.strip())
    except BaseException as error:report.update(passed=False,error=str(error));raise
    finally:args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    return 0
if __name__=='__main__':raise SystemExit(main())
