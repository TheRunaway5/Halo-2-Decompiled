"""Shared physical-original proof driver for the Shadow frame owners.

Each owner's validator (tools/validate_native_<name>_tu15.py) supplies a Spec
and a corpus. The unchanged TU1.5 instructions of the owner (and of its
declared genuine children) run single-stepped on physical i386. Each run
stops before a separately owned callee; a deterministic stub supplies that
callee's ABI and storage effects and the original resumes at the return
address. The stub's effect (registers, flags, XMM, MXCSR and byte patches) is
recorded in the fixture and replayed by Stub in
tests/native_tu15_shadow_frame_harness.hpp, after the harness has compared the
callee's complete entry ABI. The native route is compared at every ordered
read and store (complete GP/EFLAGS/XMM/MXCSR/x87 packet), at every required
entry, at the final ABI and over every backed byte.

Outputs stay in HALO2_TEST_SCRATCH; only the JSON report is kept.
"""
import copy,hashlib,importlib,json,os,platform,re,subprocess,sys
from dataclasses import dataclass,field
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
UNIT='src/rendering/shadows/source_shadow_frame_tu15_generated.cpp'
COMMON_INPUTS=(UNIT,'tools/native_shadow_frame_oracle.py','tests/native_tu15_shadow_frame_harness.hpp',
 'src/physics/runtime/native_tu15_object_tick_systems.hpp','src/physics/runtime/native_tu15_raw_x87_scope.hpp','src/physics/runtime/native_tu15_mopp_ray_caller_frame.hpp',
 'src/physics/runtime/native_tu15_required_owner_policy.hpp','src/physics/runtime/native_tu15_census_admission.hpp','src/physics/runtime/physics_contact_runtime_internal.hpp',
 'src/physics/runtime/physics_world_runtime_internal.hpp','src/physics/runtime/physics_step_internal.hpp',
 'tools/native_bsp_admission_original.py','tools/native_geometry_visibility_original.py','tools/validate_native_tu15_prt_helpers.py','tools/observe_native_campaign_ai_vector_math_tu15.cpp')
LIVE=('0000000000000080ff3f','35c26821a2da0fc90040','45230100000000c0ff7f')

@dataclass
class Callee:
    sites:tuple            # call sites (the return address is site+5)
    argument_bytes:int     # popped by the callee (stdcall) besides the return address
    effect:object=None     # effect(ctx): storage/register effects beyond the common clobbers

@dataclass
class Spec:
    name:str               # file stem, e.g. 'prt_partition'
    label:str
    entry:int
    binding:str            # binder module
    replace:str            # binder installer
    body_pin:str
    owned:dict
    entries:tuple          # bodies the physical observer may execute
    callees:dict           # entry -> Callee (stopped before, stubbed)
    regions:tuple
    inputs:tuple           # owner-specific inputs (pinned)
    reference_variable:str
    reference_test_variable:str
    census_test_variable:str
    census_range:tuple
    leaves:tuple=()        # in-unit bodies copied beside the owner (uninstrumented)
    helpers:str=''         # C++ beside the copies (dispatch and genuine children)
    unreachable:tuple=()
    x87_deltas:dict=field(default_factory=dict)
    mutants:tuple=()       # (name, path, old, new)
    guard_mutants:tuple=()
    max_rounds:int=64
    free_x87_topology:bool=False
    undefined_flag_scope:tuple=()

def digest(value):return hashlib.sha256(value.encode()).hexdigest()
def entry_name(entry):return format(entry,'x')

def prepare(spec,scratch,require_binding=False):
    from generated_routine_ownership import function_bodies
    from audit_native_execution_ownership import unique_function
    binding=importlib.import_module(spec.binding);install=getattr(binding,spec.replace)
    from specialize_native_operands import specialize
    source=(ROOT/UNIT).read_text();match,body=binding.authenticate(source)
    if digest(body)!=spec.body_pin:raise ValueError('Complete '+spec.label+' owner drift')
    bound=install(source)
    if require_binding and bound!=source:raise ValueError('Production '+spec.label+' default missing')
    if install(bound)!=bound:raise ValueError(spec.label+' default installer is not idempotent')
    prepared,_=specialize(bound);resolver=unique_function(source,'selected')
    text=unique_function(source,'sign_extend')+'\n';receipts=dict(body_sha256=spec.body_pin,resolver_sha256=digest(resolver))
    pick=lambda value,entry:next(item for sig,item in function_bodies(value)if sig[1]==entry)
    for name,unit in(('retained',source),('bound',bound),('prepared',prepared)):
        owner=body if name=='retained'else pick(unit,entry_name(spec.entry))
        instrumented,count=re.subn(r'(?m)^((?:L[0-9a-f]+: )?)(?=if\(s\.services\.instruction\)|s\.instruction_literal\()',r'\1::retained_statement();',owner)
        if count!=binding.SITES:raise ValueError('Incomplete '+spec.label+' body '+name)
        leaves=''.join('\n'+pick(unit,entry_name(leaf))for leaf in spec.leaves)
        declarations=''.join(f'static void run_{entry_name(leaf)}(WorldState&);\n'for leaf in spec.leaves)
        text+='namespace '+name+' {\n'+resolver+'\n'+declarations+spec.helpers+instrumented+leaves+'\n}\n'
        receipts[name+'_body_sha256']=digest(owner)
    (scratch/(spec.name+'_bodies.inc')).write_text(text);return receipts

# ---------------------------------------------------------------- stubs
class Context:
    """The stopped callee's view of the case: storage, registers, mode."""
    def __init__(self,store,gp,row,call,mode,target,site,entry_gp):
        self.store=store;self.gp=gp;self.row=row;self.call=call;self.mode=mode;self.target=target;self.site=site;self.entry_gp=entry_gp
    def locate(self,at,width):
        for base,bank in self.store.items():
            if base<=at<=base+len(bank)-width:return bank,at-base
        raise ValueError('Stub storage is unbacked '+hex(at))
    def get(self,at,width=4):bank,offset=self.locate(at,width);return int.from_bytes(bank[offset:offset+width],'little')
    def put(self,at,value,width=4):bank,offset=self.locate(at,width);bank[offset:offset+width]=(value&((1<<(8*width))-1)).to_bytes(width,'little')
def stub(spec,case,observed,row,call):
    """The continuation after the deterministic stub, and its recorded effect."""
    target=observed['stopped_at'];callee=spec.callees[target];mode=row['stub']
    store={bank['address']:bytearray.fromhex(after['after'])for bank,after in zip(case['banks'],observed['banks'],strict=True)}
    original={address:bytes(bank)for address,bank in store.items()}
    gp=list(observed['boundary_gp']);sp=gp[4];entry_gp=list(gp)
    ctx=Context(store,gp,row,call,mode,target,None,entry_gp)
    site=ctx.get(sp)-5
    if site not in callee.sites:raise ValueError('Required return address differs '+hex(site))
    ctx.site=site;which=sorted(spec.callees).index(target)
    seed=(0x51000000+0x101*call+mode+0x10000*which)&0xffffffff
    gp[4]=sp+4+callee.argument_bytes;gp[0]=seed;gp[1]=~seed&0xffffffff;gp[2]=seed^0x00ff00ff
    eflags=(observed['boundary_eflags']&0x400)|0x202|((0x11*call+0x41*mode)&0x8d5)
    xmm=[[(lane^(0x9e3779b9*(call+1)+r*4+k))&0xffffffff for k,lane in enumerate(item)]for r,item in enumerate(observed['boundary_xmm'])]
    fp=list(observed['after_fp'])
    if mode&1:fp[2]|=0x20
    ctx.fp=fp;ctx.live=list(observed['boundary_live_x87'])
    if callee.effect:callee.effect(ctx)
    live=ctx.live
    patches=[]
    for address,bank in store.items():
        before=original[address];n=0
        while n<len(bank):
            if bank[n]==before[n]:n+=1;continue
            start=n
            while n<len(bank)and bank[n]!=before[n]:n+=1
            patches.append([address+start,bytes(bank[start:n]).hex()])
    effect=dict(entry=target,site=site,gp=gp,eflags=eflags,xmm=xmm,mxcsr=fp[2],x87_status=fp[1],live_x87=live,patches=patches)
    banks=[dict(address=bank['address'],before=bytes(store[bank['address']]).hex())for bank in case['banks']]
    esp=gp[4]
    return dict(case,entry=site+5,gp=gp,eflags=eflags,xmm=xmm,before_fp=fp,live_x87=list(live),banks=banks,return_pc=ctx.get(esp),matrix=ctx.get(esp+4),source=ctx.get(esp+8)),effect
def entry_packet(observed):
    return dict(entry=observed['stopped_at'],gp=observed['boundary_gp'],eflags=observed['boundary_eflags'],xmm=observed['boundary_xmm'],fp=observed['after_fp'],live_x87=observed['boundary_live_x87'])
TOPOLOGY='if(finished){check(fp.cwd==entry_fp.cwd&&((fp.swd^entry_fp.swd)&0x3800u)==0&&fp.ftw==entry_fp.ftw,"Original path start changed live x87 topology");'
def observer_with_free_topology(observe):
    """The pinned observer requires every completed segment to end with the
    x87 topology it started with. A segment that stops before a callee
    consuming ST(0) (_ftol2) ends one register deeper; a case declaring
    free_x87_topology skips that check only (its live registers at the stop
    are reported by the boundary extension and compared in the harness).
    The extension is applied to the private observer copy after the pinned
    helpers' own checks."""
    import native_bsp_admission_original as owner
    inner=owner.physical_original
    def physical(image,rows,scratch,**options):
        current=Path.read_text
        def read(path,*args,**kwargs):
            text=current(path,*args,**kwargs)
            if path.name=='observe_native_campaign_ai_vector_math_tu15.cpp':
                if text.count(TOPOLOGY)!=1:raise ValueError('Observer x87 topology boundary changed')
                text=text.replace(TOPOLOGY,TOPOLOGY.replace('if(finished){','if(finished&&!c.value("free_x87_topology",false)){',1),1)
            return text
        Path.read_text=read
        try:return inner(image,rows,scratch,**options)
        finally:Path.read_text=current
    def run(image,rows,scratch,**options):
        owner.physical_original=physical
        try:return observe(image,rows,scratch,**options)
        finally:owner.physical_original=inner
    return run
def segmented(spec,image,rows,scratch,calls,label):
    """Run every row to completion (or its fault) through stub resumes."""
    from native_bsp_admission_original import observe
    physical_original=observer_with_free_topology(observe)if spec.free_x87_topology else observe
    state=[dict(segments=[],inputs=[],stops=[],effects=[])for _ in rows];active=[]
    for index,row in enumerate(rows):
        case={key:row[key]for key in('name','entry','gp','eflags','xmm','before_fp','live_x87','banks','matrix','source','max_steps')}
        case['banks']=[dict(address=bank['address'],before=bank['before'])for bank in row['banks']];case['stop_before_entries']=sorted(spec.callees)
        if spec.free_x87_topology:case['free_x87_topology']=True
        if 'segment_input' in row:case=copy.deepcopy(row['segment_input'])
        if 'fault' in row:case['fault']=row['segment_fault']
        active.append((index,case))
    rounds=0;authority=None;instructions=0
    while active:
        directory=scratch/f'{label}-segment{rounds}';directory.mkdir()
        cases=[dict(case,id=n)for n,(_,case)in enumerate(active)]
        result,authority=physical_original(image,cases,directory,entries=spec.entries,regions=spec.regions,trace_calls=calls,trace_extended=True)
        instructions+=result.get('original_instructions',0);following=[]
        for(index,case),observed in zip(active,result['cases'],strict=True):
            item=state[index];item['segments'].append(observed);item['inputs'].append(case)
            if observed.get('completed')and observed.get('stopped_at')in spec.callees:
                if 'fault' in case:raise ValueError('Prefix segment reached a required callee before its fault')
                item['stops'].append(entry_packet(observed))
                resumed,effect=stub(spec,case,observed,rows[index],len(item['stops'])-1)
                item['effects'].append(effect);following.append((index,resumed))
        active=following;rounds+=1
        if rounds>spec.max_rounds:raise ValueError(spec.label+' corpus exceeds its required call budget')
    return state,authority,rounds,instructions

def mutants(spec,scratch,command,rows,metadata):
    outcomes=[]
    for name,path,old,new in spec.mutants:
        text=(ROOT/path).read_text()
        if text.count(old)!=1:raise ValueError('Unique '+spec.label+' mutant witness missing '+name)
        overlay=scratch/('mutant-'+name);target=overlay/Path(path).relative_to(Path(path).parts[0]);target.parent.mkdir(parents=True);target.write_text(text.replace(old,new,1))
        binary=overlay/'tests';mutated=list(command);mutated[mutated.index('-o')+1]=str(binary);mutated.insert(1,'-I'+str(overlay))
        if name not in spec.guard_mutants:mutated.insert(1,'-DHALO2_SHADOW_FRAME_MUTATION_CORE_ONLY=1')
        compiled=subprocess.run(mutated,capture_output=True,text=True)
        if compiled.returncode:raise ValueError(spec.label+' mutant did not compile '+name+': '+compiled.stderr[-4000:])
        tests=['--guards']if name in spec.guard_mutants else[]
        if not tests:
            ordered=[row for row in rows if'fault'not in row]+[row for row in rows if'fault'in row]
            for index in range(0,len(ordered),64):
                fixture=overlay/(str(index//64)+'.json');fixture.write_text(json.dumps(dict(metadata,cases=ordered[index:index+64]),separators=(',',':')));tests.append(str(fixture))
        rejected=False;diagnostic=''
        for test in tests:
            done=subprocess.run([str(binary),test],capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
            if done.returncode:rejected=True;diagnostic=(done.stdout+done.stderr)[-1500:];break
        outcomes.append(dict(name=name,rejected=rejected,diagnostic=diagnostic))
    survivors=[item['name']for item in outcomes if not item['rejected']]
    if survivors:raise ValueError('Compiled '+spec.label+' mutants survived '+', '.join(survivors))
    return outcomes

def row(name,entry,gp,eflags,xmm,before_fp,live_x87,banks,stub_mode,regions,sp_value,max_steps=200000):
    return dict(name=name,entry=entry,gp=gp,eflags=eflags,xmm=xmm,before_fp=before_fp,live_x87=live_x87,stub=stub_mode,max_steps=max_steps,
        matrix=sp_value[0],source=sp_value[1],banks=[dict(address=base,before=bytes(bank).hex())for(base,_),bank in zip(regions,banks)])

def main(spec,corpus,argv=None):
    import argparse
    parser=argparse.ArgumentParser(description=spec.label+' physical-original proof');parser.add_argument('--xbe',type=Path,required=True);parser.add_argument('--report',type=Path,required=True);parser.add_argument('--compiler',default='g++');parser.add_argument('--sanitizers',action='store_true');parser.add_argument('--prefixes',action='store_true');parser.add_argument('--require-binding',action='store_true');parser.add_argument('--semantic-mutants',action='store_true');parser.add_argument('--limit',type=int);parser.add_argument('--families',type=str);args=parser.parse_args(argv)
    try:return run(spec,corpus,args)
    except Exception as error:
        args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(dict(passed=False,owned_scratch=os.environ.get('HALO2_TEST_SCRATCH'),error=str(error)[-10000:],retained_large_artifacts=[]),indent=2)+'\n')
        raise

def run(spec,corpus,args):
    from validate_native_tu15_prt_helpers import PIN
    if sys.platform!='linux'or platform.machine().lower()not in('x86_64','amd64','i386','i686')or not args.xbe.is_file():return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);inputs=tuple(dict.fromkeys(COMMON_INPUTS+spec.inputs))
    pins={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest()for name in inputs};source=prepare(spec,scratch,args.require_binding)
    sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
    image=Xbe(args.xbe);assert hashlib.sha256(image.data).hexdigest()==PIN
    import oracle_native_campaign_screen_lifecycle_tu15 as common
    original_metadata=common.metadata
    def metadata(image):
        result=original_metadata(image);code=common._Code(image,'.text')
        for site in result['sites']:
            instruction=code.at(site['instruction']);mnemonic=instruction.mnemonic
            if mnemonic in('ret','pop'):site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
            if mnemonic.startswith('prefetch'):site['operands']=[]
            # Capstone marks the m64 destinations of MOVHPS/MOVLPS as reads.
            if mnemonic in ('movhps','movlps') and bytes(instruction.bytes[:2]) in (b'\x0f\x17', b'\x0f\x13'):
                if len(site['operands'])!=1 or site['operands'][0]['bytes']!=8:
                    raise ValueError('Original packed-half store metadata drift')
                site['operands'][0].update(read=False,write=True)
            # Capstone leaves access=0 on memory operands of `test [mem], reg`.
            for o in site['operands']:
                if not o['read']and not o['write']:o['read']=True
        return result
    common.metadata=metadata;common.ENTRIES=spec.entries
    sites=metadata(image)['sites'];code=common._Code(image,'.text')
    for site in sites:
        if code.at(site['instruction']).mnemonic in('fstp','fst','fistp','fist','fisttp'):
            for o in site['operands']:o.update(read=False,write=True)
    io=sorted({site['instruction']for site in sites if site['operands']or code.at(site['instruction']).mnemonic in('push','call')})
    write_sites=[site['instruction']for site in sites if any(o['write']for o in site['operands'])or code.at(site['instruction']).mnemonic in('push','call')]
    rmw_sites=[site['instruction']for site in sites if code.at(site['instruction']).mnemonic not in('push','call')and any(o['read']for o in site['operands'])and any(o['write']for o in site['operands'])]
    repeat_sites={site['instruction']for site in sites if any(p in(0xf2,0xf3)for p in code.at(site['instruction']).prefix)and code.at(site['instruction']).mnemonic.startswith('rep')}
    calls=[dict(entry=pc,gp=True,xmm=True,x87_delta=spec.x87_deltas.get(pc,0))for pc in io]
    rows=corpus(image)
    for index,item in enumerate(rows):item['id']=index
    if args.families:
        chosen={int(item)for item in args.families.split(',')};rows=[item for item in rows if int(item['name'].split('-')[0][6:])in chosen]
    if args.limit:rows=rows[:args.limit]
    normal,authority,rounds,instructions=segmented(spec,image,rows,scratch,calls,'normal')
    prefix_authority=None;witnesses=set();prefixes=[];prefix_state=[];prefix_rounds=0
    def flatten(item):
        return dict(reads=[r for segment in item['segments']for r in segment['reads']],writes=[w for segment in item['segments']for w in segment['writes']],calls=[c for segment in item['segments']for c in segment['calls']])
    if args.prefixes:
        for base,item in zip(rows,normal,strict=True):
            depth=len(base['live_x87']);domain=base.get('domain','ordinary')
            if depth not in(0,3):continue
            reads=writes=0
            for number,(segment,case) in enumerate(zip(item['segments'],item['inputs'],strict=True)):
                for phase,key in(('before_read','reads'),('before_write','writes'),('after_write','writes')):
                    for ordinal,event in enumerate(segment[key]):
                        pc=event['instruction']
                        if not any(start<=pc<end for start,end in spec.owned.items()):continue
                        actual='before_instruction'if phase=='before_write'and pc in rmw_sites else phase
                        witness=(actual,pc,depth,domain)
                        if witness in witnesses:continue
                        witnesses.add(witness)
                        if actual=='before_instruction':
                            prior=sum(1 for earlier in item['segments'][:number]for c in earlier['calls']if c['entry']==pc)
                            step=sum(1 for earlier in segment[key][:ordinal]if earlier['instruction']==pc)
                            fault=dict(phase=actual,instruction=pc,ordinal=prior+step);segment_fault=dict(phase=actual,instruction=pc,ordinal=step)
                        else:
                            offset=reads if phase=='before_read'else writes
                            fault=dict(phase=actual,ordinal=offset+ordinal);segment_fault=dict(phase=actual,ordinal=ordinal)
                        prefix=copy.deepcopy(base);prefix.update(id=len(rows)+len(prefixes),name=base['name']+f'-{actual}-{number}-{ordinal}',fault=fault,segment_fault=segment_fault,segment_number=number,segment_input=copy.deepcopy(case),parent=base['name'])
                        prefixes.append(prefix)
                reads+=len(segment['reads']);writes+=len(segment['writes'])
        prefix_state,prefix_authority,prefix_rounds,prefix_instructions=segmented(spec,image,prefixes,scratch,calls,'prefix');instructions+=prefix_instructions
    cases=[];by_name={item['name']:index for index,item in enumerate(rows)}
    for base,item in list(zip(rows,normal,strict=True))+list(zip(prefixes,prefix_state,strict=True)):
        if 'fault' in base:
            parent=normal[by_name[base['parent']]];number=base['segment_number']
            item=dict(segments=parent['segments'][:number]+item['segments'],stops=parent['stops'][:number],effects=parent['effects'][:number],inputs=parent['inputs'][:number]+item['inputs'])
        final=item['segments'][-1];flat=flatten(item);fault=base.get('fault',{})
        expected=dict(base);[expected.pop(key,None)for key in('segment_input','segment_fault','segment_number','parent')]
        expected.update(boundary_gp=final['boundary_gp'],boundary_eflags=final['boundary_eflags'],boundary_xmm=final['boundary_xmm'],after_fp=final['after_fp'],reads=flat['reads'],writes=flat['writes'],after_live_x87=final['boundary_live_x87'],callee_calls=item['stops'],stub_effects=item['effects'])
        expected['expected_pc']=final['boundary']['instruction']if fault else flat['calls'][-1]['entry'];expected['calls']=[]
        for call_index,call in enumerate(flat['calls']):
            call=dict(call);assert call.pop('arguments',[])==[]
            last=call_index+1==len(flat['calls'])
            if fault.get('phase')=='before_instruction'and last:continue
            # A REP string instruction entered with ECX=0 performs no access,
            # so it has no pre-I/O packet (the tracer still stops on it).
            if call['entry']in repeat_sites and call['gp'][1]==0:continue
            expected['calls'].append(call)
            if call['entry']in rmw_sites and not(fault.get('phase')=='before_read'and last):expected['calls'].append(call.copy())
        expected['banks']=[dict(address=bank['address'],before=bank['before'],after=after['after'])for bank,after in zip(base['banks'],final['banks'],strict=True)]
        cases.append(expected)
    executed={pc for item in normal for segment in item['segments']for pc in segment['executed_sites']}
    owned_sites={site['instruction']for site in sites if any(start<=site['instruction']<end for start,end in spec.owned.items())}
    unexecuted=sorted(owned_sites-executed-set(spec.unreachable))
    test=ROOT/'tests'/f'native_tu15_{spec.name}_tests.cpp'
    binary=scratch/(spec.name+'-tests');command=[args.compiler,'-std=c++20','-O2','-Wno-ignored-attributes','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(scratch),'-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'tests'),str(test),'-o',str(binary)]
    if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
    compiled=subprocess.run(command,capture_output=True,text=True)
    if compiled.returncode:raise ValueError(spec.label+' compile failed: '+compiled.stderr[-6000:])
    meta=dict(write_sites=write_sites,rmw_sites=rmw_sites);fixture=scratch/'fixture.json'
    def run_all(selected,env):
        outcomes=[]
        for index in range(0,len(selected),64):
            fixture.write_text(json.dumps(dict(meta,cases=selected[index:index+64]),separators=(',',':')))
            done=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',**env))
            if done.returncode:return False,(done.stdout+done.stderr)[-12000:],outcomes
            outcomes.append(json.loads(done.stdout))
        return True,'',outcomes
    passed,diagnostic,outcomes=run_all(cases,{})
    checks=[]
    if passed:
        for option,env in(('--guards',{}),('--reference-guards',{spec.reference_variable:'1'})):
            done=subprocess.run([str(binary),option],capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',**env));checks.append(dict(option=option,passed=done.returncode==0,diagnostic=(done.stdout+done.stderr)[-4000:]))
        ok,text,forced=run_all([item for item in cases if'fault'not in item],{spec.reference_variable:'1',spec.reference_test_variable:'1'})
        checks.append(dict(option='positive_forced_reference',passed=ok,diagnostic=text,counts={key:sum(item[key]for item in forced)for key in forced[0]if key!='passed'}if forced else{}))
        done=subprocess.run([str(binary),'--census-guards'],capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));checks.append(dict(option='--census-guards',passed=done.returncode==0,diagnostic=(done.stdout+done.stderr)[-4000:]))
        ok,text,census=run_all([item for item in cases if'fault'not in item],{spec.census_test_variable:'1'})
        checks.append(dict(option='census_route',passed=ok and bool(census)and sum(item['census_route_comparisons']for item in census)==sum('fault'not in item for item in cases),diagnostic=text,counts={key:sum(item[key]for item in census)for key in census[0]if key!='passed'}if census else{}))
    mutated=mutants(spec,scratch,command,cases,meta)if args.semantic_mutants and passed and all(item['passed']for item in checks)else[]
    changed=[name for name,pin in pins.items()if hashlib.sha256((ROOT/name).read_bytes()).hexdigest()!=pin]
    stubbed={hex(entry):sum(1 for item in cases if'fault'not in item for call in item['callee_calls']if call['entry']==entry)for entry in sorted(spec.callees)}
    report=dict(passed=passed and not changed and all(item['passed']for item in checks)and not unexecuted,private_unbound=not args.require_binding,owned_scratch=str(scratch),source_sha256=pins,changed_inputs=changed,source_authority=source,physical_cases=len(cases),normal_cases=sum('fault'not in item for item in cases),prefix_cases=sum('fault'in item for item in cases),prefix_witnesses=len(witnesses),
        segment_rounds=rounds,prefix_segment_rounds=prefix_rounds,stubbed_required_calls=stubbed,original_instructions=instructions,
        owned_sites=len(owned_sites),declared_unreachable=[hex(pc)for pc in spec.unreachable],unexecuted_owned_sites=[hex(pc)for pc in unexecuted],physical_original=authority,physical_prefix_original=prefix_authority,additional_source_checks=checks,compiled_semantic_mutants=mutated,compiler=args.compiler,sanitizers=args.sanitizers,diagnostic=diagnostic,retained_large_artifacts=[])
    if passed:report['native']={key:sum(item[key]for item in outcomes)for key in outcomes[0]if key!='passed'}
    if spec.undefined_flag_scope:
        report['undefined_flag_comparison_scope'] = spec.undefined_flag_scope
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({key:report[key]for key in('passed','physical_cases','normal_cases','prefix_cases','stubbed_required_calls','owned_sites','unexecuted_owned_sites','changed_inputs')}));print(diagnostic[-3000:])
    return 0 if report['passed']else 1
