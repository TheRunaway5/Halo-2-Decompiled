#!/usr/bin/env python3
"""Private full136A80 model draw submission proof against the physical original.

Unchanged TU1.5 instructions of 136A80 run single-stepped on physical i386.
Each run stops before the separately owned 2CBD0 draw unit. A deterministic
stub (identical to Stub in tests/native_tu15_model_draw_submission_tests.cpp)
then supplies that callee's ABI and storage effects, and the original resumes
at 136CE5. The native route is compared at every ordered read and store
(complete GP/EFLAGS/XMM/MXCSR/x87 packet), at every 2CBD0 entry, at the final
ABI and over every backed byte.
"""
import argparse,copy,hashlib,json,os,platform,random,re,struct,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
from validate_native_tu15_prt_helpers import PIN
from native_bsp_admission_original import observe as physical_original
UNIT='src/rendering/shadows/source_shadow_frame_tu15_generated.cpp'
BODY_PIN='e409f2bf9b13e7f2e4cdbf1e66ed58997fab6055968a8d4a637c61686ff9407e'
REGIONS=((0x4ec000,0x1000),(0x4f0000,0x1000),(0x61000000,0x20000),(0x62000000,0x4000))
OWNED={0x136a80:0x136d47}
ENTRIES=(0x136a80,)
CALLEE=0x2cbd0
RESUME=0x136ce5
INPUTS=(UNIT,'include/halo2/native_tu15_model_draw_submission_kernel.hpp','src/rendering/native_tu15_model_draw_submission.hpp','tests/native_tu15_model_draw_submission_tests.cpp','tests/native_tu15_model_draw_submission_guards.inc','tools/validate_native_model_draw_submission_tu15.py','tools/native_model_draw_submission_replacements.py',
 'src/physics/runtime/native_tu15_raw_x87_scope.hpp','src/physics/runtime/native_tu15_mopp_ray_caller_frame.hpp','src/physics/runtime/native_tu15_required_owner_policy.hpp','src/physics/runtime/native_tu15_census_admission.hpp','src/physics/runtime/physics_contact_runtime_internal.hpp','src/physics/runtime/physics_world_runtime_internal.hpp','src/physics/runtime/physics_step_internal.hpp',
 'tools/native_bsp_admission_original.py','tools/native_geometry_visibility_original.py','tools/validate_native_tu15_prt_helpers.py','tools/observe_native_campaign_ai_vector_math_tu15.cpp')
def digest(value):return hashlib.sha256(value.encode()).hexdigest()
def prepare(scratch,require_binding=False):
    from generated_routine_ownership import function_bodies
    from audit_native_execution_ownership import unique_function
    import native_model_draw_submission_replacements as binding
    from specialize_native_operands import specialize
    source=(ROOT/UNIT).read_text();match,body=binding.authenticate(source)
    if digest(body)!=BODY_PIN:raise ValueError('Complete136A80 owner drift')
    bound=binding.replace_model_draw_submission(source)
    if require_binding and bound!=source:raise ValueError('Production136A80 default missing')
    if binding.replace_model_draw_submission(bound)!=bound:raise ValueError('136A80 default installer is not idempotent')
    prepared,_=specialize(bound);resolver=unique_function(source,'selected')
    text=unique_function(source,'sign_extend')+'\n';receipts=dict(body_sha256=BODY_PIN,resolver_sha256=digest(resolver))
    pick=lambda value,entry:next(item for sig,item in function_bodies(value)if sig[1]==entry)
    for name,unit in(('retained',source),('bound',bound),('prepared',prepared)):
        owner=body if name=='retained'else pick(unit,'136a80')
        instrumented,count=re.subn(r'(?m)^((?:L[0-9a-f]+: )?)(?=if\(s\.services\.instruction\)|s\.instruction_literal\()',r'\1::retained_statement();',owner)
        if count!=binding.SITES:raise ValueError('Incomplete201site model draw submission body '+name)
        text+='namespace '+name+' {\n'+resolver+'\n'+instrumented+'\n}\n'
        receipts[name+'_body_sha256']=digest(owner)
    (scratch/'model_draw_submission_bodies.inc').write_text(text);return receipts
def stub(case,observed,mode,call):
    """Return the 136CE5 continuation after the deterministic 2CBD0 stub."""
    banks=[dict(address=bank['address'],before=after['after'])for bank,after in zip(case['banks'],observed['banks'],strict=True)]
    store={bank['address']:bytearray.fromhex(bank['before'])for bank in banks}
    def locate(at):
        for base,bank in store.items():
            if base<=at<=base+len(bank)-4:return bank,at-base
        raise ValueError('2CBD0 stub storage is unbacked '+hex(at))
    def get(at):bank,offset=locate(at);return int.from_bytes(bank[offset:offset+4],'little')
    def put(at,value):bank,offset=locate(at);bank[offset:offset+4]=(value&0xffffffff).to_bytes(4,'little')
    gp=list(observed['boundary_gp']);sp=gp[4]
    if get(sp)!=RESUME:raise ValueError('2CBD0 return address differs')
    seed=0x51000000+0x101*call+mode
    gp[4]=sp+0x58;gp[0]=seed;gp[1]=~seed&0xffffffff;gp[2]=seed^0x00ff00ff
    eflags=(observed['boundary_eflags']&0x400)|0x202|((0x11*call+0x41*mode)&0x8d5)
    xmm=[[(lane^(0x9e3779b9*(call+1)+r*4+k))&0xffffffff for k,lane in enumerate(row)]for r,row in enumerate(observed['boundary_xmm'])]
    fp=list(observed['after_fp'])
    if mode&1:fp[2]|=0x20
    if mode==2:gp[7]^=0x40
    if mode==3 and call==0:put(gp[3]+gp[6],get(gp[3]+gp[6])-1)
    if mode==4:put(gp[4]+0x18,get(gp[4]+0x18)+1)
    for bank in banks:bank['before']=bytes(store[bank['address']]).hex()
    esp=gp[4]
    return dict(case,entry=RESUME,gp=gp,eflags=eflags,xmm=xmm,before_fp=fp,live_x87=list(observed['boundary_live_x87']),banks=banks,return_pc=get(esp),matrix=get(esp+4),source=get(esp+8))
def entry_packet(observed):
    return dict(entry=CALLEE,gp=observed['boundary_gp'],eflags=observed['boundary_eflags'],xmm=observed['boundary_xmm'],fp=observed['after_fp'],live_x87=observed['boundary_live_x87'])
def segmented(image,rows,scratch,calls,label):
    """Run every row to completion (or its fault) through 2CBD0 stub resumes."""
    state=[dict(segments=[],inputs=[],stops=[])for _ in rows];active=[]
    for index,row in enumerate(rows):
        case={key:row[key]for key in('name','entry','gp','eflags','xmm','before_fp','live_x87','banks','matrix','source','max_steps')}
        case['banks']=[dict(address=bank['address'],before=bank['before'])for bank in row['banks']];case['stop_before_entries']=[CALLEE]
        if 'segment_input' in row:case=copy.deepcopy(row['segment_input'])
        if 'fault' in row:case['fault']=row['segment_fault']
        active.append((index,case))
    rounds=0;authority=None;instructions=0
    while active:
        directory=scratch/f'{label}-segment{rounds}';directory.mkdir()
        cases=[]
        for n,(_,case) in enumerate(active):case=dict(case,id=n);cases.append(case)
        result,authority=physical_original(image,cases,directory,entries=ENTRIES,regions=REGIONS,trace_calls=calls,trace_extended=True)
        instructions+=result.get('original_instructions',0);following=[]
        for(index,case),observed in zip(active,result['cases'],strict=True):
            item=state[index];item['segments'].append(observed);item['inputs'].append(case)
            if observed.get('completed')and observed.get('stopped_at')==CALLEE:
                if 'fault' in case:raise ValueError('Prefix segment reached 2CBD0 before its fault')
                item['stops'].append(entry_packet(observed))
                following.append((index,stub(case,observed,rows[index]['stub'],len(item['stops'])-1)))
        active=following;rounds+=1
        if rounds>16:raise ValueError('Model draw submission corpus exceeds its 2CBD0 call budget')
    return state,authority,rounds,instructions
def corpus():
    rng=random.Random(0x136a80);rows=[];live=('0000000000000080ff3f','35c26821a2da0fc90040','45230100000000c0ff7f')
    for family in range(16):
      for v in range(8):
        banks=[bytearray(rng.randbytes(size))for _,size in REGIONS]
        def put(at,value,width=4):
            for(base,size),bank in zip(REGIONS,banks):
                if base<=at<=base+size-width:bank[at-base:at-base+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little');return
            raise ValueError('Unbacked model draw submission fixture '+hex(at))
        def raw(at,size):
            for(base,length),bank in zip(REGIONS,banks):
                if base<=at<=base+length-size:return bytes(bank[at-base:at-base+size])
            raise ValueError('Unbacked model draw submission input')
        obj=0x61001200+4*v;sp=0x62002000+4*(v%4);cells=0x61009000+v*0x40;table=0x61008000+v*0x10;data=0x61008100+v*0x80
        recs=0x6100a000+v*0x400;owners=0x6100c000+v*0x40;shadows=0x6100e000+v*0x400
        # Per-family structure: section count, argument, matching first part,
        # owner/copy selectors, part counts and direction flag.
        sections=(1,2,3)[v%3];arg=(0,1,0x101,0xff00)[v%4];same=[True,True,True];owned=(0,1,2,1)[v%4];unit=[1,1,1];shadow=[0,0,0]
        parts=[(1,2,0,3)[v%4],(2,1,1,0)[v%4],(1,3,2,1)[v%4]];df=False;bit1=None;override=None
        if family==0:sections=(0,0xffff,0x8000,0,0xfffe,0,0x8001,0)[v];parts=[1,1,1]
        if family==1:sections=1;parts=[(0,1,2,3,0xfffffffe,1,2,1)[v],0,0];arg=v%2
        if family==2:sections=2;arg=1;same=[True,True,True]
        if family==3:sections=3;arg=(1,0x101,0xff01,0x7f)[v%4];same=[True,v%2==0,v%2==1]
        if family==4:sections=3;arg=(0,0x100,0xff00,0)[v%4];same=[True,False,False]
        if family==5:owned=1;unit=[(3,0,0x82,1)[v%4]]*3;shadow=[1,1,1]
        if family==6:owned=1;unit=[2,2,2];shadow=[0,0,0]
        if family==7:owned=1;unit=[2,2,2];shadow=[(1,0x100,0x8000,0xffff)[v%4]]*3;sections=(1,2)[v%2]
        if family==8:owned=1;unit=[2,2,2];shadow=[1,1,1];sections=1;df=True;parts=[(1,2,1,3)[v%4],0,0]
        if family==9:override=(0,1,2,3)[v%4];parts=[3,3,3]
        if family==10:bit1=v%2==0;parts=[2,2,2]
        if family==11:owned=(0,2,0x101,0xffffffff)[v%4]
        if family==12:sections=3;arg=0;owned=1;unit=[1,2,(2,1)[v%2]];shadow=[0,(1,7)[v%2],(0,1)[v%2]];same=[True,False,False]
        if family==13:df=v%2==1;parts=[2,1,1]
        if family==14:parts=[3,(0,2,3,1)[v%4],2];sections=(2,3)[v%2]
        if family==15:sections=2;arg=0;owned=1;unit=[2,2,2];shadow=[1,(0,1)[v%2],1];parts=[1,1,0]
        put(obj+0x166,sections,2);put(obj+0x1c4,owned)
        # Section part lists: equal first-part handles share one owner row;
        # owner families give every section its own row.
        handles=[]
        for section in range(3):
            handle=0xabcd0000|(1 if same[section]else 1+section)
            if family in(5,6,7,8,12,15):handle=0xabcd0000|(1+section)
            handles.append(handle);put(cells+4*section,handle);put(obj+0x48+4*section,cells+4*section);put(obj+0x18+4*section,parts[section])
        # Owner tables: 0x4ECBD0 -> data rows -> unit record; 0x4F0414 ->
        # shadow rows -> geometry with its 0x290 halfword.
        put(0x4ecbd0,table);put(table+0x44,data);put(0x4f0414,owners)
        for slot in range(4):
            record=recs+slot*0x100;put(data+slot*12+8,record);put(record,0x55550000|slot);put(record+0xaa,1,1)
            geometry=shadows+slot*0x300;put(owners+slot*16+8,geometry);put(geometry+0x290,0,2)
        for section in range(3):
            slot=handles[section]&0xffff;record=recs+slot*0x100;put(record+0xaa,unit[section]&0xff,1)
            put(shadows+slot*0x300+0x290,shadow[section],2)
        flags=rng.getrandbits(32)
        if bit1 is not None:flags=(flags&~2)|(2 if bit1 else 0)
        put(obj+0x170,flags)
        if override is not None:
            for section in range(3):
                for part in range(4):put(obj+0x75+0x10*section+part,(0xff if(override==0 or(override==2 and part%2))else(part*7+override)&0xff),1)
        # A DF=1 copy runs downward over the caller frame and below it; the
        # record's preceding dwords then become sane locals, so the original
        # keeps executing the declared structure.
        frame=((sp-4)&~7)-0x214-12
        if df:
            put(obj-4,obj+0x48);put(obj-0x10,obj+0x48);put(obj-0x18,obj);put(obj-0x1c,0);put(obj-0x20,0);put(obj-0x24,0);put(obj-0x2c,0)
        # Any later section that reads the frame copy sees the record's bytes.
        image=raw(obj,0x1e4)
        for offset in range(0,0x1e4,4):put(frame+0x38+offset,int.from_bytes(image[offset:offset+4],'little'))
        put(sp,0x63000000);put(sp+4,arg|(rng.getrandbits(16)<<16 if family!=3 else 0));put(sp+8,rng.getrandbits(32))
        gp=[rng.getrandbits(32)for _ in range(8)];gp[0]=obj;gp[4]=sp
        depth=(v+family)%4;cw=(0x7f,0x27f,0x37f)[v%3]|((v%4)<<10);status=((v%8)<<11)|0x4500
        eflags=0x202|(0x400 if df else 0)|(rng.getrandbits(12)&0x8d5)
        rows.append(dict(id=len(rows),name=f'family{family}-variant{v}',entry=0x136a80,gp=gp,eflags=eflags,matrix=int.from_bytes(raw(sp+4,4),'little'),source=int.from_bytes(raw(sp+8,4),'little'),max_steps=100000,xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],before_fp=[cw,status,0x1f80|((family%4)<<13)|(0,0x40,0x8000,0x8040)[v%4]],live_x87=list(live[:depth]),stub=v%5,banks=[dict(address=base,before=bytes(bank).hex())for(base,_),bank in zip(REGIONS,banks)]))
    return rows
DEFINITIONS=(
 ('ignore-skip-argument','src/rendering/native_tu15_model_draw_submission.hpp','byte(2,read(0x136ad5u,g[5]+8u,1));s_.flags.logic(g[2]&255u,1);if(s_.flags.zf)return false;','byte(2,read(0x136ad5u,g[5]+8u,1));s_.flags.logic(g[2]&255u,1);'),
 ('submit-skipped-section','include/halo2/native_tu15_model_draw_submission_kernel.hpp','if(view.skip_section())continue;','view.skip_section();'),
 ('forward-only-copy','src/rendering/native_tu15_model_draw_submission.hpp','const auto delta=(s_.other_flags&0x400u)?0xfffffffcu:4u;','const auto delta=4u;'),
 ('copy-observed-before-retire','src/rendering/native_tu15_model_draw_submission.hpp','store(0x136b4du,g[7],value,[&]{g[6]+=delta;g[7]+=delta;--g[1];},true);}','write(0x136b4du,g[7],value,4,true);g[6]+=delta;g[7]+=delta;--g[1];}'),
 ('narrow-override','src/rendering/native_tu15_model_draw_submission.hpp','if(s_.flags.zf)write(0x136c18u,g[4]+0x24u,0xffffffffu);','if(s_.flags.zf)write(0x136c18u,g[4]+0x24u,0xffu);'),
 ('swapped-side-bytes','src/rendering/native_tu15_model_draw_submission.hpp','g[0]=g[1];g[0]=s_.flags.right(g[0],3u,4,false);','g[0]=g[1];g[0]=s_.flags.right(g[0],4u,4,false);'),
 ('part-cursor-stride','src/rendering/native_tu15_model_draw_submission.hpp','g[2]=s_.flags.plus(g[2],2u,4);','g[2]=s_.flags.plus(g[2],4u,4);'),
 ('call-before-return-address','src/rendering/native_tu15_model_draw_submission.hpp','push(0x136ce0u,0x136ce5u);s_.required(0x136ce0u,0x2cbd0u,false);','s_.gp[4]-=4u;s_.required(0x136ce0u,0x2cbd0u,false);'),
 ('software-owner-shift','src/rendering/native_tu15_model_draw_submission.hpp','const auto row=owner_row(g[1],s_.flags.bits());g[1]=row.first;s_.flags.load(row.second);','g[1]=s_.flags.logic(g[1]&0xffffu,4);g[1]=s_.flags.shift(g[1],4u,4);'),
 ('copy-keeps-record','src/rendering/native_tu15_model_draw_submission.hpp','write(0x136b7bu,g[4]+0x210u,0u);g[6]=g[4]+0x38u;','write(0x136b7bu,g[4]+0x210u,0u);'),
 ('uncertified-required-owner','src/rendering/native_tu15_model_draw_submission.hpp','if(!native_required_owner_policy_detail::preserves_instruction_policy(s.services))return false;','if(!s.services.required_owner)return false;'),
 ('census-without-switch','src/rendering/native_tu15_model_draw_submission.hpp','if(native_census_detail::uncursored(s,0x136a80u,0x136d46u)){','if(!s.instruction_cursor&&s.services.instruction){'),
)
GUARD_MUTANTS=('uncertified-required-owner','census-without-switch')
def mutants(scratch,command,rows,metadata):
    outcomes=[]
    for name,path,old,new in DEFINITIONS:
        text=(ROOT/path).read_text()
        if text.count(old)!=1:raise ValueError('Unique model draw submission mutant witness missing '+name)
        overlay=scratch/('mutant-'+name);target=overlay/Path(path).relative_to(Path(path).parts[0]);target.parent.mkdir(parents=True);target.write_text(text.replace(old,new,1))
        binary=overlay/'tests';mutated=list(command);mutated[mutated.index('-o')+1]=str(binary);mutated.insert(1,'-I'+str(overlay))
        if name not in GUARD_MUTANTS:mutated.insert(1,'-DHALO2_DRAW_SUBMISSION_MUTATION_CORE_ONLY=1')
        compiled=subprocess.run(mutated,capture_output=True,text=True)
        if compiled.returncode:raise ValueError('Model draw submission mutant did not compile '+name+': '+compiled.stderr[-4000:])
        tests=['--guards']if name in GUARD_MUTANTS else[]
        if not tests:
            # Normal rows first, then the fault prefixes (the observed-retire
            # mutant only differs at an after-write boundary).
            ordered=[row for row in rows if'fault'not in row]+[row for row in rows if'fault'in row]
            for index in range(0,len(ordered),64):
                fixture=overlay/(str(index//64)+'.json');fixture.write_text(json.dumps(dict(metadata,cases=ordered[index:index+64]),separators=(',',':')));tests.append(str(fixture))
        rejected=False;diagnostic=''
        for test in tests:
            done=subprocess.run([str(binary),test],capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
            if done.returncode:rejected=True;diagnostic=(done.stdout+done.stderr)[-1500:];break
        if not rejected:raise ValueError('Compiled model draw submission mutant survived '+name)
        outcomes.append(dict(name=name,rejected=True,diagnostic=diagnostic))
    return outcomes
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--xbe',type=Path,required=True);parser.add_argument('--report',type=Path,required=True);parser.add_argument('--compiler',default='g++');parser.add_argument('--sanitizers',action='store_true');parser.add_argument('--prefixes',action='store_true');parser.add_argument('--require-binding',action='store_true');parser.add_argument('--semantic-mutants',action='store_true');parser.add_argument('--limit',type=int);args=parser.parse_args()
    if sys.platform!='linux'or platform.machine().lower()not in('x86_64','amd64','i386','i686')or not args.xbe.is_file():return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);pins={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest()for name in INPUTS};source=prepare(scratch,args.require_binding)
    sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
    image=Xbe(args.xbe);assert hashlib.sha256(image.data).hexdigest()==PIN
    import oracle_native_campaign_screen_lifecycle_tu15 as common
    original_metadata=common.metadata
    def metadata(image):
        result=original_metadata(image);code=common._Code(image,'.text')
        for site in result['sites']:
            if code.at(site['instruction']).mnemonic in('ret','pop'):site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
        return result
    common.metadata=metadata;common.ENTRIES=ENTRIES
    sites=metadata(image)['sites'];code=common._Code(image,'.text')
    io=sorted({site['instruction']for site in sites if site['operands']or code.at(site['instruction']).mnemonic in('push','call')})
    write_sites=[site['instruction']for site in sites if any(o['write']for o in site['operands'])or code.at(site['instruction']).mnemonic in('push','call')]
    # One step that both reads and stores (RMW operand or MOVSD's separate
    # source/destination) publishes two native packets and stops atomically.
    rmw_sites=[site['instruction']for site in sites if code.at(site['instruction']).mnemonic not in('push','call')and any(o['read']for o in site['operands'])and any(o['write']for o in site['operands'])]
    if 0x136b4d not in rmw_sites:raise ValueError('MOVSD dual-access site metadata changed')
    calls=[dict(entry=pc,gp=True,xmm=True,x87_delta=0)for pc in io]
    rows=corpus()
    if args.limit:rows=rows[:args.limit]
    normal,authority,rounds,instructions=segmented(image,rows,scratch,calls,'normal')
    prefix_authority=None;witnesses=set();prefixes=[];prefix_state=[];prefix_rounds=0
    def flatten(item):
        return dict(reads=[r for segment in item['segments']for r in segment['reads']],writes=[w for segment in item['segments']for w in segment['writes']],calls=[c for segment in item['segments']for c in segment['calls']])
    if args.prefixes:
        for row,item in zip(rows,normal,strict=True):
            depth=len(row['live_x87']);family=int(row['name'].split('-')[0][6:])
            if depth not in(0,3):continue
            domain=family if family in(8,13,15)else'ordinary'
            reads=writes=0
            for number,(segment,case) in enumerate(zip(item['segments'],item['inputs'],strict=True)):
                for phase,key in(('before_read','reads'),('before_write','writes'),('after_write','writes')):
                    for ordinal,event in enumerate(segment[key]):
                        pc=event['instruction']
                        actual='before_instruction'if phase=='before_write'and pc in rmw_sites else phase
                        witness=(actual,pc,depth,domain)
                        if witness in witnesses:continue
                        witnesses.add(witness)
                        if actual=='before_instruction':
                            prior=sum(1 for earlier in item['segments'][:number]for c in earlier['calls']if c['entry']==pc)
                            # The native hook runs once per executed step.
                            step=sum(1 for earlier in segment[key][:ordinal]if earlier['instruction']==pc)
                            fault=dict(phase=actual,instruction=pc,ordinal=prior+step);segment_fault=dict(phase=actual,instruction=pc,ordinal=step)
                        else:
                            base=reads if phase=='before_read'else writes
                            fault=dict(phase=actual,ordinal=base+ordinal);segment_fault=dict(phase=actual,ordinal=ordinal)
                        prefix=copy.deepcopy(row);prefix.update(id=len(rows)+len(prefixes),name=row['name']+f'-{actual}-{number}-{ordinal}',fault=fault,segment_fault=segment_fault,segment_number=number,segment_input=copy.deepcopy(case))
                        prefixes.append(prefix)
                reads+=len(segment['reads']);writes+=len(segment['writes'])
        prefix_state,prefix_authority,prefix_rounds,prefix_instructions=segmented(image,prefixes,scratch,calls,'prefix');instructions+=prefix_instructions
    cases=[]
    for row,item in list(zip(rows,normal,strict=True))+list(zip(prefixes,prefix_state,strict=True)):
        if 'fault' in row:
            # Earlier segments come from the normal run of the same row.
            parent=normal[next(index for index,base in enumerate(rows)if base['name']==row['name'].split('-before')[0].split('-after')[0])]
            number=row['segment_number']
            item=dict(segments=parent['segments'][:number]+item['segments'],stops=parent['stops'][:number],inputs=parent['inputs'][:number]+item['inputs'])
        final=item['segments'][-1];flat=flatten(item);fault=row.get('fault',{})
        expected=dict(row);expected.pop('segment_input',None);expected.pop('segment_fault',None);expected.pop('segment_number',None)
        expected.update(boundary_gp=final['boundary_gp'],boundary_eflags=final['boundary_eflags'],boundary_xmm=final['boundary_xmm'],after_fp=final['after_fp'],reads=flat['reads'],writes=flat['writes'],after_live_x87=final['boundary_live_x87'],callee_calls=item['stops'])
        expected['expected_pc']=final['boundary']['instruction']if fault else flat['calls'][-1]['entry'];expected['calls']=[]
        for call_index,call in enumerate(flat['calls']):
            call=dict(call);assert call.pop('arguments',[])==[]
            last=call_index+1==len(flat['calls'])
            if fault.get('phase')=='before_instruction'and last:continue
            expected['calls'].append(call)
            if call['entry']in rmw_sites and not(fault.get('phase')=='before_read'and last):expected['calls'].append(call.copy())
        expected['banks']=[dict(address=bank['address'],before=bank['before'],after=after['after'])for bank,after in zip(row['banks'],final['banks'],strict=True)]
        cases.append(expected)
    executed={pc for item in normal for segment in item['segments']for pc in segment['executed_sites']}
    owned_sites={site['instruction']for site in sites if any(start<=site['instruction']<end for start,end in OWNED.items())}
    unexecuted=sorted(owned_sites-executed)
    binary=scratch/'draw-submission-tests';command=[args.compiler,'-std=c++20','-O2','-Wno-ignored-attributes','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(scratch),'-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'tests'),str(ROOT/'tests/native_tu15_model_draw_submission_tests.cpp'),'-o',str(binary)]
    if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
    compiled=subprocess.run(command,capture_output=True,text=True)
    if compiled.returncode:raise ValueError('Model draw submission compile failed: '+compiled.stderr[-6000:])
    meta=dict(write_sites=write_sites,rmw_sites=rmw_sites);fixture=scratch/'fixture.json'
    def run_all(selected,env):
        outcomes=[];done=None
        for index in range(0,len(selected),64):
            fixture.write_text(json.dumps(dict(meta,cases=selected[index:index+64]),separators=(',',':')))
            done=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',**env))
            if done.returncode:return False,(done.stdout+done.stderr)[-12000:],outcomes
            outcomes.append(json.loads(done.stdout))
        return True,'',outcomes
    passed,diagnostic,outcomes=run_all(cases,{})
    checks=[]
    if passed:
        for option,env in(('--guards',{}),('--reference-guards',{'HALO2_REFERENCE_MODEL_DRAW_SUBMISSION':'1'})):
            done=subprocess.run([str(binary),option],capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',**env));checks.append(dict(option=option,passed=done.returncode==0,diagnostic=(done.stdout+done.stderr)[-4000:]))
        ok,text,forced=run_all([row for row in cases if'fault'not in row],{'HALO2_REFERENCE_MODEL_DRAW_SUBMISSION':'1','HALO2_DRAW_SUBMISSION_REFERENCE_TEST':'1'})
        checks.append(dict(option='positive_forced_reference',passed=ok,diagnostic=text,counts={key:sum(item[key]for item in forced)for key in forced[0]if key!='passed'}if forced else{}))
        # HALO2_CENSUS_NATIVE_BRIDGES diagnostic route: uncursored full callbacks.
        done=subprocess.run([str(binary),'--census-guards'],capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));checks.append(dict(option='--census-guards',passed=done.returncode==0,diagnostic=(done.stdout+done.stderr)[-4000:]))
        ok,text,census=run_all([row for row in cases if'fault'not in row],{'HALO2_DRAW_SUBMISSION_CENSUS_TEST':'1'})
        checks.append(dict(option='census_route',passed=ok and bool(census)and sum(item['census_route_comparisons']for item in census)==sum('fault'not in row for row in cases),diagnostic=text,counts={key:sum(item[key]for item in census)for key in census[0]if key!='passed'}if census else{}))
    mutated=mutants(scratch,command,cases,meta)if args.semantic_mutants and passed and all(item['passed']for item in checks)else[]
    changed=[name for name,pin in pins.items()if hashlib.sha256((ROOT/name).read_bytes()).hexdigest()!=pin]
    callees=sum(len(row['callee_calls'])for row in cases if'fault'not in row)
    report=dict(passed=passed and not changed and all(item['passed']for item in checks),private_unbound=not args.require_binding,owned_scratch=str(scratch),source_sha256=pins,changed_inputs=changed,source_authority=source,physical_cases=len(cases),normal_cases=sum('fault'not in row for row in cases),prefix_cases=sum('fault'in row for row in cases),prefix_witnesses=len(witnesses),
        segment_rounds=rounds,prefix_segment_rounds=prefix_rounds,stubbed_2cbd0_calls=callees,original_instructions=instructions,
        owned_sites=len(owned_sites),unexecuted_owned_sites=[hex(pc)for pc in unexecuted],physical_original=authority,physical_prefix_original=prefix_authority,additional_source_checks=checks,compiled_semantic_mutants=mutated,compiler=args.compiler,sanitizers=args.sanitizers,diagnostic=diagnostic,retained_large_artifacts=[])
    if passed:report['native']={key:sum(item[key]for item in outcomes)for key in outcomes[0]if key!='passed'}
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({key:report[key]for key in('passed','physical_cases','normal_cases','prefix_cases','stubbed_2cbd0_calls','owned_sites','unexecuted_owned_sites','changed_inputs')}));print(diagnostic[-3000:])
    return 0 if report['passed']else 1
if __name__=='__main__':
    try:raise SystemExit(main())
    except Exception as error:
        if '--report'in sys.argv:
            report=Path(sys.argv[sys.argv.index('--report')+1]);report.parent.mkdir(parents=True,exist_ok=True);report.write_text(json.dumps(dict(passed=False,owned_scratch=os.environ.get('HALO2_TEST_SCRATCH'),error=str(error)[-10000:],retained_large_artifacts=[]),indent=2)+'\n')
        raise
