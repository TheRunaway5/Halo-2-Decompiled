#!/usr/bin/env python3
"""Actual16E860 shared record initialization and complete retained ABI."""
import argparse,copy,hashlib,json,os,platform,random,re,struct,subprocess,sys
from pathlib import Path
from generated_routine_ownership import function_bodies
from validate_native_tu15_prt_helpers import PIN,physical_original
from native_datum_initialize_replacements import canonical_body,replace_datum_initialize

ROOT=Path(__file__).resolve().parents[1]
COPIES=('src/physics/physics_live_support_tu15_generated.cpp','src/ui/source_ui_script_startup_tu15_generated.cpp','src/rendering/shadows/source_shadow_frame_tu15_generated.cpp')
REGIONS=((0x1000000,32768),(0x62000000,16384))
INPUTS=COPIES+('include/halo2/native_tu15_datum_initialize_kernel.hpp','src/physics/runtime/native_tu15_datum_specific.hpp','tools/native_datum_initialize_replacements.py','tests/native_tu15_datum_initialize_abi_tests.cpp','tools/validate_native_datum_initialize_tu15.py')
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()

def partial_source(image,scratch):
    from types import SimpleNamespace
    import generate_physics_live_support_tu15 as generator
    import oracle_native_campaign_screen_lifecycle_tu15 as common
    from native_datum_initialize_replacements import PHYSICAL_ALLOCATION_ENTRIES,PHYSICAL_ALLOCATION_DISPATCH_PIN
    entries=tuple(int(value,16)for value in PHYSICAL_ALLOCATION_ENTRIES);code=common._Code(image,'.text')
    bodies={entry:generator.complete_body(code,entry)for entry in entries};instructions={};addresses={}
    for body in bodies.values():
        if body.tables:raise ValueError('Physical-allocation datum context has unexpected tables')
        for pc in sorted(body.code):
            instruction=code.at(pc);instructions[f'{pc:08x}']=pc;addresses[pc]=pc;addresses[pc+instruction.size]=pc+instruction.size
            for operand in instruction.operands:
                if operand.type==2:addresses[operand.imm&0xffffffff]=operand.imm&0xffffffff
                if operand.type==3:addresses[operand.mem.disp&0xffffffff]=operand.mem.disp&0xffffffff
    paired=dict(image='English TU1.5',original_sha256=PIN,entries={f'{entry:08x}':entry for entry in entries},body_sites={f'{entry:08x}':sorted(body.code)for entry,body in bodies.items()},instruction_map=instructions,address_map={f'{p:08x}':value for p,value in addresses.items()},routines=[dict(entry=entry,tables=[],byte_tables=[])for entry in entries])
    directory=scratch/'physical-allocation-context';directory.mkdir();pairing=directory/'bodies.json';pairing.write_text(json.dumps(paired));output=directory/'context.cpp'
    generator.generate(SimpleNamespace(output=output,manifest=directory/'generation.json',correspondence=pairing,additional_entries=pairing),paired,complete_listings=True)
    source=output.read_text();dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)[0]
    if hashlib.sha256(dispatcher.encode()).hexdigest()!=PHYSICAL_ALLOCATION_DISPATCH_PIN:raise ValueError('Physical-allocation exact generated dispatcher changed')
    return source,dict(entries=list(PHYSICAL_ALLOCATION_ENTRIES),complete_dispatcher_sha256=PHYSICAL_ALLOCATION_DISPATCH_PIN,generated_sha256=sha(output))

def corpus():
    rng=random.Random(0x16e860);rows=[]
    live=('0000000000000080ff3f','00000000000000000080','35c26821a2da0fc90040','45230100000000c0ff7f','0100000000000080ff7f','0000000000000080ff7f','00000000000000800040','00000000000000800140')
    def add(name,stride,salt,backwards=False,pool_offset=None,record_offset=None,record_field=None):
        n=len(rows);banks=[bytearray(rng.randbytes(size))for _,size in REGIONS];sp=0x62002000+n%4
        pool=0x1000100 if pool_offset is None else sp-pool_offset
        record=0x1002000 if record_offset is None else sp+record_offset
        if record_field is not None:record=pool+record_field
        def put(at,value,width=4):
            for (base,size),bank in zip(REGIONS,banks):
                if base<=at and at+width<=base+size:bank[at-base:at-base+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little');return
            raise ValueError('Initializer input escaped declared banks')
        def get(at):
            for (base,size),bank in zip(REGIONS,banks):
                if base<=at<=base+size-4:return struct.unpack_from('<I',bank,at-base)[0]
            raise ValueError('Initializer stack input escaped declared banks')
        gp=[rng.getrandbits(32)for _ in range(8)];gp[2]=pool;gp[4]=sp;gp[6]=record
        if pool_offset==0x44:gp[3]=(gp[3]&0xffff0000)|0xfffe
        if pool_offset==0x48:gp[7]=(gp[7]&0xffff0000)|0xfffe
        put(pool+0x24,stride);put(pool+0x40,salt,2);put(sp,0x63000000)
        rows.append(dict(id=n,name=name,entry=0x16e860,gp=gp,matrix=get(sp+4),source=get(sp+8),eflags=0x202|(rng.getrandbits(12)&0x8d5)|(0x400 if backwards else 0),
            xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],before_fp=[(0x7f,0x27f,0x37f)[n%3]|((n%4)<<10),((n%8)<<11)|(n%64)|((n*0x100)&0x4700),0x1f80|((n%4)<<13)|(n%64)],
            live_x87=list(live[:(0,1,4,8)[n%4]]),max_steps=10000,banks=[dict(address=base,before=bank.hex())for (base,_),bank in zip(REGIONS,banks)]))
    for stride in (0,1,2,3,4,5,7,8,12,31,32):
        for salt in (0,1,0x7fff,0x8000,0xfffe,0xffff):
            for backwards in (False,True):add(f'clear-{stride}-salt-{salt:x}-DF{int(backwards)}',stride,salt,backwards)
    for backwards in (False,True):
        for offset in (0x28,0x2c,0x44,0x48):add(f'pool-stack-{offset:x}-DF{int(backwards)}',5,0xfffe,backwards,pool_offset=offset)
        for offset in (-32,-16,-12,-8,-4,4,8):add(f'record-stack-{offset}-DF{int(backwards)}',4,0xfffe,backwards,record_offset=offset)
        for field in (0x24,0x40):
            for stride in (0,3,8):add(f'record-field-{field:x}-stride-{stride}-DF{int(backwards)}',stride,0xfffe,backwards,record_field=field)
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
            source=(ROOT/path).read_text();current=next(b for m,b in function_bodies(source)if m[1]=='16e860');original=canonical_body(current);bound=replace_datum_initialize(source)
            if args.require_binding and bound!=source:raise ValueError('Initializer native production binding missing: '+path)
            if replace_datum_initialize(bound)!=bound:raise ValueError('Initializer binding not idempotent')
            selected=next(line for line in source.splitlines()if line.startswith('std::uint32_t selected('))
            from native_datum_initialize_replacements import PREFIX
            dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)[0]
            mutants=[source.replace(dispatcher,dispatcher.replace('run_16e860(s);','run_16e770(s);',1),1),bound.replace(PREFIX,PREFIX.replace('::initialize','::changed_initialize'),1)]
            if 'selected('in original:mutants.append(source.replace(selected,selected.replace('return p;','return p+1u;',1),1))
            for mutant in mutants:
                if mutant in(source,bound):raise ValueError('Missing initializer admission mutation witness')
                try:replace_datum_initialize(mutant)
                except ValueError:rejections+=1
                else:raise ValueError('Changed initializer owner/source admitted')

            for shape in (original,re.sub(r'selected\((0x[0-9a-f]+u),s\.image\)',r'\1',original)):
                candidate=source.replace(current,shape,1);replace_datum_initialize(candidate)
                for version in (candidate,replace_datum_initialize(candidate)):
                    body=next(b for m,b in function_bodies(version)if m[1]=='16e860');changed=body.replace('s.gp[2]','s.gp[3]',1)
                    if changed==body:raise ValueError('Initializer mutation witness missing')
                    try:replace_datum_initialize(version.replace(body,changed,1))
                    except ValueError:rejections+=1
                    else:raise ValueError('Changed complete initializer body admitted')
            parts.append(f'namespace bound{index} {{\n'+selected+'\n'+next(b for m,b in function_bodies(bound)if m[1]=='16e860')+'\n}\n')
            parts.append(f'namespace reference{index} {{\n'+selected+'\n'+original+'\n}\n')
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
        parts.append('namespace partial {\n'+selected_partial+'\n'+next(b for m,b in function_bodies(partial)if m[1]=='16e860')+'\n}\n')
        (scratch/'datum_initialize_bodies.inc').write_text(''.join(parts))
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
        sites=(0x16e860,0x16e863,0x16e866,0x16e86e,0x16e875,0x16e877,0x16e87b,0x16e87e,0x16e882,0x16e887,0x16e888,0x16e88b,0x16e891)
        observed,proof=physical_original(image,rows,scratch,entries=(0x16e860,),regions=REGIONS,trace_calls=[dict(entry=pc,arguments=0,gp=True,xmm=True,x87_delta=0)for pc in sites],trace_extended=True)
        cases=[]
        for row,out in zip(rows,observed['cases'],strict=True):
            if not out['completed']or not out['live_x87_unchanged']:raise ValueError('Original initializer failed normal/live80 contract')
            case=copy.deepcopy(row)
            for key in ('boundary_gp','boundary_eflags','boundary_xmm','after_fp','reads','writes'):case[key]=out[key]
            case['packets']=[{key:packet[key]for key in ('entry','arguments','gp','eflags','xmm','fp','live_x87')}for packet in out['calls']];case['final_pc']=out['calls'][-1]['entry']
            for before,after in zip(case['banks'],out['banks'],strict=True):before['after']=after['after']
            cases.append(case)
        fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(cases=cases,explicit_read_sites=sorted({e['instruction']for row in observed['cases']for e in row['reads']})),separators=(',',':')))
        binary=scratch/'initializer-tests';command=[args.compiler,'-std=c++20','-O2','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_datum_initialize_abi_tests.cpp'),'-o',str(binary)]
        if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        compile=subprocess.run(command,capture_output=True,text=True)
        if compile.returncode:raise ValueError('Initializer compile failed: '+compile.stderr[-6000:])
        run=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,timeout=120)
        if run.returncode:raise ValueError('Initializer mismatch: '+run.stderr[-6500:])
        control=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,timeout=120,env=dict(os.environ,HALO2_REFERENCE_DATUM_INITIALIZE='1',HALO2_DATUM_INITIALIZE_REFERENCE_TEST='1'))
        if control.returncode:raise ValueError('Forced initializer reference failed: '+control.stderr[-5000:])
        reference_control=json.loads(control.stdout)
        if not reference_control.get('native_refused')or not reference_control.get('callbacks')or not reference_control.get('child_entries'):raise ValueError('Forced reference did not execute genuine initializer')
        if any(sha(ROOT/path)!=pin for path,pin in pins.items()):raise ValueError('Initializer proof input changed')
        executed={pc for row in observed['cases']for pc in row['executed_sites']}
        report.update(json.loads(run.stdout),source_sha256=pins,physical_allocation_context=partial_proof,binding_rejections=rejections,reference_control=reference_control,physical_original=proof,original_sites_executed=len(executed));print(run.stdout.strip())
    except BaseException as error:report.update(passed=False,error=str(error));raise
    finally:args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    return 0
if __name__=='__main__':raise SystemExit(main())
