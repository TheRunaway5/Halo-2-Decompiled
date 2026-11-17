#!/usr/bin/env python3
"""Private full135B60 object admission proof against the physical original.

Unchanged TU1.5 instructions of 135B60, its inlined leaves (135AE0, 135620,
166B60, C0CA0) and the genuine 135970/167EF0/184690/166C00/136E10 tree run
single-stepped on physical i386. The native route is compared at every ordered
read and store (complete GP/EFLAGS/XMM/MXCSR/x87 packet), at genuine child
entries, at the final ABI and over every backed byte.
"""
import argparse,copy,hashlib,json,os,platform,random,re,struct,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
from validate_native_tu15_prt_helpers import PIN
from native_bsp_admission_original import observe as physical_original
UNIT='src/rendering/shadows/source_shadow_frame_tu15_generated.cpp'
BODY_PIN='c50fe20e2266eefaa092ac7b7c7098daa0a738906defd02a5669fc2fa0c4d4cc'
REGIONS=((0x46a000,4096),(0x4ea000,0x3000),(0x61000000,0x20000),(0x62000000,16384))
LEAVES=('135ae0','135620','166b60','c0ca0')
OWNED={0x135b60:0x135db4,0x135ae0:0x135b5a,0x135620:0x135686,0x166b60:0x166bb0,0xc0ca0:0xc0cc7}
INPUTS=(UNIT,'include/halo2/native_tu15_object_admission_kernel.hpp','src/rendering/native_tu15_object_admission.hpp','tests/native_tu15_object_admission_tests.cpp','tests/native_tu15_object_admission_guards.inc','tools/validate_native_object_admission_tu15.py','tools/native_object_admission_replacements.py',
 'src/rendering/native_tu15_bsp_admission.hpp','include/halo2/native_tu15_bsp_admission_kernel.hpp','src/rendering/native_tu15_geometry_visibility.hpp','include/halo2/native_tu15_geometry_visibility_kernel.hpp','src/rendering/native_tu15_cluster_frustum_abi.hpp','include/halo2/native_tu15_cluster_frustum_kernel.hpp','src/rendering/native_tu15_sphere_frustum_abi.hpp','include/halo2/native_tu15_sphere_frustum_kernel.hpp','src/rendering/native_tu15_bsp_queue_find.hpp','include/halo2/native_tu15_id_search_kernel.hpp','src/rendering/native_tu15_visibility_association.hpp','include/halo2/native_tu15_visibility_association_kernel.hpp','src/rendering/native_tu15_fog_membership.hpp','include/halo2/native_tu15_fog_membership_kernel.hpp','include/halo2/native_tu15_bsp_surface_2d_kernel.hpp',
 'src/physics/runtime/native_tu15_raw_x87_scope.hpp','src/physics/runtime/native_tu15_sse_mmx_scope.hpp','src/physics/runtime/native_tu15_mopp_ray_caller_frame.hpp','src/physics/runtime/physics_contact_runtime_internal.hpp','src/physics/runtime/physics_world_runtime_internal.hpp','src/physics/runtime/physics_step_internal.hpp',
 'tools/native_bsp_admission_original.py','tools/native_geometry_visibility_original.py','tools/validate_native_tu15_prt_helpers.py','tools/observe_native_campaign_ai_vector_math_tu15.cpp')
def digest(value):return hashlib.sha256(value.encode()).hexdigest()
def prepare(scratch,require_binding=False):
    from generated_routine_ownership import function_bodies
    from audit_native_execution_ownership import unique_function
    import native_object_admission_replacements as binding
    from specialize_native_operands import specialize
    source=(ROOT/UNIT).read_text();match,body=binding.authenticate(source)
    if digest(body)!=BODY_PIN:raise ValueError('Complete135B60 owner drift')
    bound=binding.replace_object_admission(source)
    if require_binding and bound!=source:raise ValueError('Production135B60 default missing')
    if binding.replace_object_admission(bound)!=bound:raise ValueError('135B60 default installer is not idempotent')
    prepared,_=specialize(bound);resolver=unique_function(source,'selected')
    helpers='''
static void run_135970(WorldState&s){if(::capture_child)::capture_child(s,0x135970u);if(!halo2::native_geometry_visibility_detail::run_with(s,[](auto&child,auto kind,auto&before){const bool frustum=kind==halo2::native_geometry_visibility_detail::Child::frustum;if(::capture_child)::capture_child(child,frustum?0x167ef0u:0x166c00u);if(!(frustum?halo2::native_cluster_frustum_abi_detail::bridge(child,before):halo2::native_bsp_queue_find_detail::bridge(child,before)))throw std::runtime_error("Genuine geometry descendant refused");}))throw std::runtime_error("Genuine geometry owner refused");}
static void run_136e10(WorldState&s){if(::capture_child)::capture_child(s,0x136e10u);if(!halo2::native_visibility_association_detail::bridge(s))throw std::runtime_error("Genuine association owner refused");}
static void dispatch(WorldState&s,std::uint32_t target,std::uint32_t,bool){if(target==0x135970u)run_135970(s);else if(target==0x136e10u)run_136e10(s);else throw std::runtime_error("Unknown genuine object admission child");}
'''
    text=unique_function(source,'sign_extend')+'\n';receipts=dict(body_sha256=BODY_PIN,resolver_sha256=digest(resolver))
    pick=lambda value,entry:next(item for sig,item in function_bodies(value)if sig[1]==entry)
    for name,unit in(('retained',source),('bound',bound),('prepared',prepared)):
        owner=body if name=='retained'else pick(unit,'135b60')
        leaves=[pick(unit,leaf)for leaf in LEAVES]
        instrumented,count=re.subn(r'(?m)^((?:L[0-9a-f]+: )?)(?=if\(s\.services\.instruction\)|s\.instruction_literal\()',r'\1::retained_statement();',owner)
        if count!=209:raise ValueError('Incomplete209site object admission body '+name)
        declarations=''.join(f'static void run_{leaf}(WorldState&);\n'for leaf in LEAVES)
        text+='namespace '+name+' {\n'+resolver+'\n'+declarations+helpers+instrumented+'\n'+'\n'.join(leaves)+'\n}\n'
        receipts[name+'_body_sha256']=digest(owner);receipts[name+'_leaf_sha256']={leaf:digest(item)for leaf,item in zip(LEAVES,leaves)}
    (scratch/'object_admission_bodies.inc').write_text(text);return receipts
def corpus(image):
    rng=random.Random(0x135b60);rows=[];live=('0000000000000080ff3f','35c26821a2da0fc90040','45230100000000c0ff7f')
    bits=lambda value:struct.unpack('<I',struct.pack('<f',value))[0]
    for family in range(31):
      for variant in range(8):
        banks=[bytearray(image.read(base,size))if base==0x46a000 else bytearray(rng.randbytes(size))for base,size in REGIONS]
        def put(at,value,width=4):
            for(base,size),bank in zip(REGIONS,banks):
                if base<=at<=base+size-width:bank[at-base:at-base+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little');return
            raise ValueError('Unbacked object admission fixture '+hex(at))
        def get(at):
            for(base,size),bank in zip(REGIONS,banks):
                if base<=at<=base+size-4:return int.from_bytes(bank[at-base:at-base+4],'little')
            raise ValueError('Unbacked object admission input')
        v=variant;context=0x61001000+v;descriptor=0x61004000+v;queue=0x61006000+v;ids=0x61006100+v;flags=0x61006200+v;words=0x61006300+v;assoc=0x61006400+v
        table=0x61007000+v;data=0x61007100+v;record=0x61008000+v;aux=0x61009000+v;auxq=0x61009100+v;auxitems=0x61009200+v;sp=0x62002000+v
        index=0x800 if family==24 else(1,3,5,7)[v%4];handle=0x12340000|index if family!=27 else 0xabcd0000|index;stamp=0x5a000000+family
        stamped=family<11
        # Object table, record and its two bounding spheres.
        put(0x4ecbd0,table);put(table+0x44,data);put(data+index*12+8,record);put(0x4eabcc,stamp)
        # Family 24: the 0x800 stamp slot is the object table pointer itself.
        if family!=24:put(0x4eabd0+index*4,stamp if stamped else stamp^(1<<(v%31)))
        put(record+4,(0x80000000 if family!=18 else 0)|rng.getrandbits(31));put(record+6,v%2,1)
        for sphere in(0x30,0x50):
            for axis in range(3):put(record+sphere+axis*4,bits((0.,1.5,-2.)[(v+axis)%3]*(1 if sphere==0x30 else 0.5)))
            put(record+sphere+12,bits((1.,2.,0.5,4.)[v%4]))
        # Context: descriptor, object queue, flags, volume, sphere selector,
        # 135970 mode, plane and auxiliary owner.
        mode=(2,0,1,7)[v%4]if family>=11 else 2
        clip=0x200 if family in(12,15,23,26)or(family>=11 and v%2)else 0
        force=0x80 if family==14 else 0;auxbit=1 if family in(20,21)else 0
        put(context,descriptor);put(context+0x14,queue);put(context+0x2a60,clip|force|auxbit|(0x100 if v%3==0 else 0));put(context+0x2a78,1 if family==15 or(family>=11 and v%4==3)else 0)
        put(context+0x2a7c,mode);put(context+0xa5e,(0,1,3,511)[v%4]if family!=19 else(512,0xffff,600,512)[v%4],2)
        for axis in range(3):put(context+0x2a68+axis*4,bits(0.));put(context+0x2a88+axis*4,bits(1. if axis==(v%3)else 0.))
        put(context+0x2a74,bits((10.,1.,100.,0.5)[v%4]));put(context+0x2a94,bits((-4.,0.,4.,0.5,-0.5,-1.5,1.5,9.)[v]))
        put(context+0x2ad0,aux if family in(20,21)or v==5 else 0);put(aux+0x14,auxq);put(auxq+4,(0,1,3,2)[v%4],2);put(auxq+12,auxitems)
        for item in range(3):put(auxitems+item*4,handle if(item==1 and family==20)else handle^0x10000)
        # Frustum descriptor for mode0, as in the 135970 proof.
        put(descriptor,0 if family==13 else 1,2)
        for group in range(6):put(descriptor+0xa70+group*2,1 if not group else 0,2);put(descriptor+0xa7c+group*2,0,2)
        frustum=descriptor+0x1974
        for axis in range(3):put(frustum+0x60+axis*8,bits(-10.));put(frustum+0x64+axis*8,bits(10.))
        for plane in range(6):
            values=[0.,0.,0.,(9.,0.,-5.)[v%3]];values[plane//2]=1. if plane%2==0 else -1.
            for lane,value in enumerate(values):put(frustum+0x78+plane*16+lane*4,bits(value))
        # Object queue rows: ids, flags, unused dwords and association rows.
        capacity=8;count=(0,3,4,2,5,1,3,6)[v]if stamped else(0,2,7,3)[v%4]
        if family==6:count=0
        if family==16:count=capacity-1
        put(queue,capacity);put(queue+4,count,2);put(queue+8,flags);put(queue+12,ids);put(queue+16,words);put(queue+20,assoc)
        match=(1,0,2,3)[v%4]
        for row in range(8):
            ident=handle if(row==match and family not in(4,8))else handle^(0x10000*(row+1))
            if family==7 and row>match:ident=handle
            if family==8:ident=(handle&0xffff)|0x55550000
            put(ids+row*4,ident);put(flags+row*2,(0x800 if family==2 else 0)|rng.choice((0,0x100,0x400)),2)
            put(assoc+row*2,0xffff if family==3 else(row*3)%11 if family!=9 else 0xfff0-row,2)
        # An unmatched cursor (-1) probes the rows just before both streams.
        put(flags-2,rng.getrandbits(16),2);put(assoc-2,(v*5)%13,2)
        arg1=(0,1,0x1f,0x20,0x3f,2,0x21,5)[v]if family!=9 else(0x8001,0xfffe,0x8000,0xffff)[v%4]
        arg2=1 if family in(1,2,3,7,9)or(stamped and v%2)else 0;arg3=1 if family in(1,4,6,8)or(stamped and v%3==0)else(v%2 if not stamped else 0)
        if family==5:arg2=arg3=0
        cluster=arg1 if arg1<0x8000 else arg1-0x10000
        if not stamped and 0<=cluster<0x60:put(descriptor+0xa6e+cluster*26+2,1,2);put(descriptor+0xa6e+cluster*26+14,0,2)
        # Aliases that the real ordering must survive.
        if family==10:put(queue+12,ids);put(queue+8,ids)       # flags stream aliases the id stream
        if family==25:put(queue+16,sp-0x30)                     # appended dword lands in the live frame
        if family==22:put(context+0x2a7c,1);put(context+0x2a74,bits(0.25));put(context+0x2a68,bits(50.))
        if family==23:
            put(context+0x2a94,0x7fc01234+v);put(context+0x2a60,0x200)
        if family==17:put(context+0x2a7c,1);put(context+0x2a74,bits(100.))
        if family==26:put(context+0x2a7c,0)
        if family==28:put(context+0x2a90,0x80000000);put(context+0x2a8c,0x00000001);put(context+0x2a88,0xff800000 if v%2 else 0x7f800000);put(context+0x2a60,0x200)
        if family==29:put(context+0x2a78,1);put(record+6,0xfe|(v%2),1)
        if family==30:
            # The appended zero dword lands on the stamp slot; C0CA0 then
            # finds its stamp already current (stamp value zero).
            put(0x4eabcc,0);put(0x4eabd0+index*4,0x1000|v);put(queue+16,0x4eabd0+index*4-count*4)
        put(sp,0x63000000);put(sp+4,handle);put(sp+8,arg1|(rng.getrandbits(16)<<16));put(sp+12,arg2|(rng.getrandbits(8)<<8));put(sp+16,arg3|(rng.getrandbits(8)<<8))
        gp=[rng.getrandbits(32)for _ in range(8)];gp[0]=context;gp[4]=sp
        depth=v%4;cw=(0x7f,0x27f,0x37f)[v%3]|((v%4)<<10);status=((v%8)<<11)|0x4500
        rows.append(dict(id=len(rows),name=f'family{family}-variant{v}',entry=0x135b60,gp=gp,eflags=0x202|(0x400 if v%2 else 0)|(rng.getrandbits(12)&0x8d5),matrix=get(sp+4),source=get(sp+8),max_steps=100000,xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],before_fp=[cw,status,0x1f80|((family%4)<<13)|(0,0x40,0x8000,0x8040)[v%4]],live_x87=list(live[:depth]),banks=[dict(address=base,before=bytes(bank).hex())for(base,_),bank in zip(REGIONS,banks)]))
    return rows
DEFINITIONS=(
 ('ignore-merged-row','include/halo2/native_tu15_object_admission_kernel.hpp','if(!view.row_merged()&&view.row_associated())','if(view.row_associated())'),
 ('secondary-common-epilogue','include/halo2/native_tu15_object_admission_kernel.hpp','return view.finish(View::unmatched_epilogue);','return view.finish(View::common_epilogue);'),
 ('stamp-full-queue','include/halo2/native_tu15_object_admission_kernel.hpp','if(!view.append())return view.finish(View::common_epilogue);','view.append();'),
 ('unordered-side-takes-both','src/rendering/native_tu15_object_admission.hpp','if(!clipping||!s_.flags.cf){','if(s_.flags.zf||!s_.flags.cf){'),
 ('wide-row-flags','src/rendering/native_tu15_object_admission.hpp','write(0x166b84u,g[0]+g[2]*2u,g[6]&0xffffu,2);','write(0x166b84u,g[0]+g[2]*2u,g[6],4);'),
 ('discard-last-probed-row','src/rendering/native_tu15_object_admission.hpp','g[3]=g[0];byte(2,found);','if(found)g[3]=g[0];byte(2,found);'),
 ('association-wrong-bit','src/rendering/native_tu15_object_admission.hpp','g[2]=s_.flags.right(g[2],31u,4,false);','g[2]=s_.flags.right(g[2],30u,4,false);'),
 ('swapped-record-flag','src/rendering/native_tu15_object_admission.hpp','mask(2,0x200u);','mask(2,0x400u);'),
 ('signed-distance-radius','src/rendering/native_tu15_object_admission.hpp','asm volatile("flds %0;fabs"::"m"(value):"memory","st");','asm volatile("flds %0"::"m"(value):"memory","st");'),
 ('software-row-shift','src/rendering/native_tu15_object_admission.hpp','const auto shifted=native_bsp_admission_detail::doubled(signed_half(g[3]),s_.flags.bits());g[0]=shifted.first;s_.flags.load(shifted.second);','g[0]=s_.flags.shift(signed_half(g[3]),1u,4);'),
 ('cleared-callback-admission','src/rendering/native_tu15_object_admission.hpp','else if(s.instruction_cursor||s.literal_service_sites||s.observe)return false;','else if(s.observe)return false;'),
 ('census-without-switch','src/rendering/native_tu15_object_admission.hpp','if(native_census_detail::uncursored(s,0xc0ca0u,0x166bafu)){','if(!s.instruction_cursor&&s.services.instruction){'),
)
GUARD_MUTANTS=('cleared-callback-admission','census-without-switch')
def mutants(scratch,command,rows,metadata):
    outcomes=[]
    for name,path,old,new in DEFINITIONS:
        text=(ROOT/path).read_text()
        if text.count(old)!=1:raise ValueError('Unique object admission mutant witness missing '+name)
        overlay=scratch/('mutant-'+name);target=overlay/Path(path).relative_to(Path(path).parts[0]);target.parent.mkdir(parents=True);target.write_text(text.replace(old,new,1))
        binary=overlay/'tests';mutated=list(command);mutated[mutated.index('-o')+1]=str(binary);mutated.insert(1,'-I'+str(overlay))
        if name not in GUARD_MUTANTS:mutated.insert(1,'-DHALO2_OBJECT_MUTATION_CORE_ONLY=1')
        compiled=subprocess.run(mutated,capture_output=True,text=True)
        if compiled.returncode:raise ValueError('Object admission mutant did not compile '+name+': '+compiled.stderr[-4000:])
        tests=['--guards']if name in GUARD_MUTANTS else[]
        if not tests:
            normal=[row for row in rows if'fault'not in row]
            for index in range(0,len(normal),64):
                fixture=overlay/(str(index//64)+'.json');fixture.write_text(json.dumps(dict(metadata,cases=normal[index:index+64]),separators=(',',':')));tests.append(str(fixture))
        rejected=False;diagnostic=''
        for test in tests:
            done=subprocess.run([str(binary),test],capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
            if done.returncode:rejected=True;diagnostic=(done.stdout+done.stderr)[-1500:];break
        if not rejected:raise ValueError('Compiled object admission mutant survived '+name)
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
    common.metadata=metadata;entries=(0x135b60,0x135970,0x167ef0,0x184690,0x166c00,0x135ae0,0x135620,0x136e10,0x166b60,0xc0ca0);common.ENTRIES=entries
    sites=metadata(image)['sites'];code=common._Code(image,'.text')
    io=sorted({site['instruction']for site in sites if site['operands']or code.at(site['instruction']).mnemonic in('push','call')})
    write_sites=[site['instruction']for site in sites if any(o['write']for o in site['operands'])or code.at(site['instruction']).mnemonic in('push','call')]
    rmw_sites=[site['instruction']for site in sites if any(o['write']and o['read']for o in site['operands'])]
    # Only genuine bridged children publish native entry packets; inlined
    # leaves are covered by their first ordered access.
    extra=sorted({0x135970,0x136e10,0x167ef0,0x166c00,0x184690}-set(io))
    if 0x184690 in extra:raise ValueError('184690 entry needs an explicit native entry witness')
    deltas={0x135b46:1,0x135a87:1,0x135a89:1,0x135a8f:2,0x135a92:2,0x135a98:3,0x135ab3:1,0x135ab7:1}
    calls=[dict(entry=pc,gp=True,xmm=True,x87_delta=deltas.get(pc,0))for pc in sorted(set(io)|set(extra))]
    rows=corpus(image)
    if args.limit:rows=rows[:args.limit]
    directory=scratch/'normal';directory.mkdir();physical,authority=physical_original(image,rows,directory,entries=entries,regions=REGIONS,trace_calls=calls,trace_extended=True)
    prefix_authority=None;witnesses=set()
    if args.prefixes:
        prefixes=[]
        for row,observed in zip(rows,physical['cases'],strict=True):
            depth=len(row['live_x87']);family=int(row['name'].split('-')[0][6:])
            if depth not in(0,3):continue
            domain=family if family in(10,24,25)else'ordinary'
            for phase,key in(('before_read','reads'),('before_write','writes'),('after_write','writes')):
                occurrences={}
                for ordinal,event in enumerate(observed[key]):
                    pc=event['instruction'];occurrence=occurrences.get(pc,0);occurrences[pc]=occurrence+1
                    if not any(start<=pc<end for start,end in OWNED.items())and pc not in(0x135970,0x136e12):continue
                    actual='before_instruction'if phase=='before_write'and pc in rmw_sites else phase
                    witness=(actual,pc,depth,domain)
                    if witness in witnesses:continue
                    witnesses.add(witness);fault=dict(phase=actual,ordinal=occurrence if actual=='before_instruction'else ordinal)
                    if actual=='before_instruction':fault['instruction']=pc
                    prefix=copy.deepcopy(row);prefix.update(id=len(rows)+len(prefixes),name=row['name']+f'-{actual}-{ordinal}',fault=fault);prefixes.append(prefix)
        directory=scratch/'prefix';directory.mkdir();stopped,prefix_authority=physical_original(image,prefixes,directory,entries=entries,regions=REGIONS,trace_calls=calls,trace_extended=True)
        rows+=prefixes;physical['cases']+=stopped['cases']
    for row,observed in zip(rows,physical['cases'],strict=True):
        row.update({key:observed[key]for key in('boundary_gp','boundary_eflags','boundary_xmm','after_fp','reads','writes')})
        row['expected_pc']=observed['boundary']['instruction']if row.get('fault')else observed['calls'][-1]['entry'];row['after_live_x87']=observed['boundary_live_x87'];row['extra_child_calls']=[];row['calls']=[]
        fault=row.get('fault',{})
        for call_index,call in enumerate(observed['calls']):
            assert call.pop('arguments')==[]
            last=call_index+1==len(observed['calls'])
            if fault.get('phase')=='before_instruction'and last:continue
            row['extra_child_calls'if call['entry']in extra else'calls'].append(call)
            if call['entry']in rmw_sites and not(fault.get('phase')=='before_read'and last):row['calls'].append(call.copy())
        for bank,after in zip(row['banks'],observed['banks'],strict=True):bank['after']=after['after']
    executed={pc for row in physical['cases']for pc in row['executed_sites']}
    owned_sites={site['instruction']for site in sites if any(start<=site['instruction']<end for start,end in OWNED.items())}
    unexecuted=sorted(owned_sites-executed)
    binary=scratch/'object-tests';command=[args.compiler,'-std=c++20','-O2','-Wno-ignored-attributes','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(scratch),'-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'tests'),str(ROOT/'tests/native_tu15_object_admission_tests.cpp'),'-o',str(binary)]
    if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
    compiled=subprocess.run(command,capture_output=True,text=True)
    if compiled.returncode:raise ValueError('Object admission compile failed: '+compiled.stderr[-6000:])
    meta=dict(write_sites=write_sites,rmw_sites=rmw_sites,extra_child_entries=extra);fixture=scratch/'fixture.json'
    def run_all(selected,env):
        outcomes=[];done=None
        for index in range(0,len(selected),64):
            fixture.write_text(json.dumps(dict(meta,cases=selected[index:index+64]),separators=(',',':')))
            done=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',**env))
            if done.returncode:return False,(done.stdout+done.stderr)[-12000:],outcomes
            outcomes.append(json.loads(done.stdout))
        return True,'',outcomes
    passed,diagnostic,outcomes=run_all(rows,{})
    checks=[]
    if passed:
        for option,env in(('--guards',{}),('--reference-guards',{'HALO2_REFERENCE_OBJECT_ADMISSION':'1'})):
            done=subprocess.run([str(binary),option],capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',**env));checks.append(dict(option=option,passed=done.returncode==0,diagnostic=(done.stdout+done.stderr)[-4000:]))
        ok,text,forced=run_all([row for row in rows if'fault'not in row],{'HALO2_REFERENCE_OBJECT_ADMISSION':'1','HALO2_OBJECT_ADMISSION_REFERENCE_TEST':'1'})
        checks.append(dict(option='positive_forced_reference',passed=ok,diagnostic=text,counts={key:sum(item[key]for item in forced)for key in forced[0]if key!='passed'}if forced else{}))
        # HALO2_CENSUS_NATIVE_BRIDGES diagnostic route: uncursored full callbacks.
        done=subprocess.run([str(binary),'--census-guards'],capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));checks.append(dict(option='--census-guards',passed=done.returncode==0,diagnostic=(done.stdout+done.stderr)[-4000:]))
        ok,text,census=run_all([row for row in rows if'fault'not in row],{'HALO2_OBJECT_ADMISSION_CENSUS_TEST':'1'})
        checks.append(dict(option='census_route',passed=ok and bool(census)and sum(item['census_route_comparisons']for item in census)==sum('fault'not in row for row in rows),diagnostic=text,counts={key:sum(item[key]for item in census)for key in census[0]if key!='passed'}if census else{}))
    mutated=mutants(scratch,command,rows,meta)if args.semantic_mutants and passed and all(item['passed']for item in checks)else[]
    changed=[name for name,pin in pins.items()if hashlib.sha256((ROOT/name).read_bytes()).hexdigest()!=pin]
    report=dict(passed=passed and not changed and all(item['passed']for item in checks),private_unbound=not args.require_binding,owned_scratch=str(scratch),source_sha256=pins,changed_inputs=changed,source_authority=source,physical_cases=len(rows),normal_cases=sum('fault'not in row for row in rows),prefix_cases=sum('fault'in row for row in rows),prefix_witnesses=len(witnesses),
        owned_sites=len(owned_sites),unexecuted_owned_sites=[hex(pc)for pc in unexecuted],physical_original=authority,physical_prefix_original=prefix_authority,additional_source_checks=checks,compiled_semantic_mutants=mutated,compiler=args.compiler,sanitizers=args.sanitizers,diagnostic=diagnostic,retained_large_artifacts=[])
    if passed:report['native']={key:sum(item[key]for item in outcomes)for key in outcomes[0]if key!='passed'}
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({key:report[key]for key in('passed','physical_cases','normal_cases','prefix_cases','owned_sites','unexecuted_owned_sites','changed_inputs')}));print(diagnostic[-3000:])
    return 0 if report['passed']else 1
if __name__=='__main__':
    try:raise SystemExit(main())
    except Exception as error:
        if '--report'in sys.argv:
            report=Path(sys.argv[sys.argv.index('--report')+1]);report.parent.mkdir(parents=True,exist_ok=True);report.write_text(json.dumps(dict(passed=False,owned_scratch=os.environ.get('HALO2_TEST_SCRATCH'),error=str(error)[-10000:],retained_large_artifacts=[]),indent=2)+'\n')
        raise
