#!/usr/bin/env python3
"""Private full136EA0 controller proof; no production/default installation."""
import argparse,copy,hashlib,json,os,platform,random,re,struct,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
from validate_native_tu15_prt_helpers import PIN
from native_bsp_admission_original import observe as physical_original
UNITS=('src/rendering/shadows/source_shadow_frame_tu15_generated.cpp','src/rendering/effects/source_effect_particle_draw_tu15_generated.cpp')
BODY_PINS=('1fc7838d8c537d77c5935a78652f6c203ade61af00585bd084425a4b786f015d','6811c5b82e28b056480e35315d05ccc20226c7b7c837e41d7bd9b725c89bf0e0')
REGIONS=((0x46a000,4096),(0x4c6000,4096),(0x4ec000,4096),(0x4f2000,4096),(0x4f4000,4096),
         (0x559000,16384),(0x61000000,196608),(0x60000000,16384),(0x62000000,16384))
INPUTS=(*UNITS,'include/halo2/native_tu15_bsp_admission_kernel.hpp','src/rendering/native_tu15_bsp_admission.hpp','tests/native_tu15_bsp_admission_tests.cpp','tools/validate_native_bsp_admission_tu15.py','tools/native_geometry_visibility_original.py','tools/validate_native_tu15_prt_helpers.py','tools/observe_native_campaign_ai_vector_math_tu15.cpp','src/rendering/native_tu15_geometry_visibility.hpp','src/rendering/native_tu15_cluster_frustum_abi.hpp','src/rendering/native_tu15_sphere_frustum_abi.hpp','src/rendering/native_tu15_bsp_queue_find.hpp','src/rendering/native_tu15_fog_membership.hpp','src/rendering/native_tu15_visibility_association.hpp','src/physics/runtime/native_tu15_raw_x87_scope.hpp','src/physics/runtime/native_tu15_sse_mmx_scope.hpp','src/physics/runtime/physics_contact_runtime_internal.hpp','src/physics/runtime/physics_world_runtime_internal.hpp')
INPUTS+=('tools/native_bsp_admission_replacements.py','include/halo2/native_tu15_geometry_visibility_kernel.hpp','include/halo2/native_tu15_cluster_frustum_kernel.hpp','include/halo2/native_tu15_sphere_frustum_kernel.hpp','include/halo2/native_tu15_visibility_association_kernel.hpp','include/halo2/native_tu15_fog_membership_kernel.hpp','include/halo2/native_tu15_id_search_kernel.hpp','include/halo2/native_tu15_bsp_surface_2d_kernel.hpp','src/physics/runtime/native_tu15_mopp_ray_caller_frame.hpp','src/physics/runtime/physics_step_internal.hpp')
INPUTS+=('tools/native_bsp_admission_original.py','tests/native_tu15_bsp_admission_guards.inc','tools/native_bsp_admission_mutations.py','tools/specialize_native_operands.py')
def digest(value):return hashlib.sha256(value.encode()).hexdigest()
def observe_batches(image,rows,directory,**options):
    directory.mkdir();cases=[];authorities=[];instructions=0
    for index in range(0,len(rows),64):
        owned=directory/str(index//64);owned.mkdir()
        inputs=[]
        for row in rows[index:index+64]:
            if row.get('original_stop'):
                row=dict(row,fault=row['original_stop'])
            inputs.append(row)
        result,authority=physical_original(image,inputs,owned,**options)
        cases.extend(result['cases']);instructions+=result['original_instructions'];authorities.append(authority)
    return dict(cases=cases,original_instructions=instructions),dict(batch_size=64,batches=authorities,original_instructions=instructions,scope='Identical authenticated original CPU observer per owned finite input batch')
def prepare(scratch,require_binding=False):
    from generated_routine_ownership import function_bodies
    from audit_native_execution_ownership import unique_function
    import native_bsp_admission_replacements as binding
    from specialize_native_operands import specialize
    text='';receipts={};parent=None
    helpers='''
static void run_171070(WorldState&s){if(::capture_child)::capture_child(s,0x171070u);if(!halo2::native_fog_membership_detail::bridge(s))throw std::runtime_error("Genuine fog owner refused");}
static void run_136e10(WorldState&s){if(::capture_child)::capture_child(s,0x136e10u);if(!halo2::native_visibility_association_detail::bridge(s))throw std::runtime_error("Genuine mask owner refused");}
static void run_135970(WorldState&s){if(::capture_child)::capture_child(s,0x135970u);if(!halo2::native_geometry_visibility_detail::run_with(s,[](auto&child,auto kind,auto&before){const bool frustum=kind==halo2::native_geometry_visibility_detail::Child::frustum;if(::capture_child)::capture_child(child,frustum?0x167ef0u:0x166c00u);if(!(frustum?halo2::native_cluster_frustum_abi_detail::bridge(child,before):halo2::native_bsp_queue_find_detail::bridge(child,before)))throw std::runtime_error("Genuine geometry descendant refused");}))throw std::runtime_error("Genuine geometry owner refused");}
static void dispatch(WorldState&s,std::uint32_t target,std::uint32_t,bool){if(target==0x136e10u)run_136e10(s);else throw std::runtime_error("Unknown genuine BSP admission child");}
'''
    for index,(unit,pin)in enumerate(zip(UNITS,BODY_PINS,strict=True)):
        source=(ROOT/unit).read_text();match,body=binding.authenticate(source)
        if digest(body)!=pin:raise ValueError('Complete136EA0 owner drift '+unit)
        if not index:parent=body;text+=unique_function(source,'sign_extend')+'\n'
        resolver=unique_function(source,'selected')
        # The entire owning resolver is retained; original complete body pin
        # remains separate from source-file provenance in this private proof.
        bound=binding.replace_bsp_admission(source)
        if require_binding and bound!=source:raise ValueError('Production136EA0 default missing')
        if binding.replace_bsp_admission(bound)!=bound:raise ValueError('136EA0 default installer is not idempotent')
        prepared,_=specialize(bound);target=lambda value:next(body for sig,body in function_bodies(value)if sig[1]=='136ea0')
        for name,owner in(('retained',binding.ordered_reference(body)),('bound',target(bound)),('prepared',target(prepared))):
            instrumented,count=re.subn(r'(?m)^((?:L[0-9a-f]+: )?)(?=if\(s\.services\.instruction\)|s\.instruction_literal\()',r'\1::retained_statement();',owner)
            if count!=376:raise ValueError('Incomplete376site BSP admission body '+name)
            text+='namespace '+('other_'if index else'')+name+' {\n'+resolver+'\n'+helpers+instrumented+'\n}\n'
        receipts[unit]=dict(body_sha256=pin,resolver_sha256=digest(resolver),sites=count,bound_body_sha256=digest(target(bound)),prepared_body_sha256=digest(target(prepared)))
    (scratch/'bsp_admission_bodies.inc').write_text(text);return receipts,parent
def relative_depths(body):
    lines=[]
    for line in body.splitlines():
        match=re.search(r'services\.instruction\(0x([0-9a-f]+)u\)',line)
        if match:lines.append((int(match[1],16),line))
    indices={pc:n for n,(pc,_)in enumerate(lines)};todo=[(0,0)];seen=set();depths={}
    while todo:
        index,depth=todo.pop()
        if(index,depth)in seen:continue
        seen.add((index,depth));pc,line=lines[index];depths.setdefault(pc,set()).add(depth)
        after=depth+line.count('s.fp.push(')-line.count('s.fp.pop(')-line.count('s.compare_and_pop(')
        if after<0 or after>2:raise ValueError('Unexpected parent x87 path')
        if 'return;'in line:continue
        jump=re.search(r'goto L([0-9a-f]+)',line)
        if jump:todo.append((indices[int(jump[1],16)],after))
        if not jump or '} if('in line or ');if('in line: # retained lines use conditional if after observation
            if index+1<len(lines):todo.append((index+1,after))
    if any(len(values)!=1 for values in depths.values()):raise ValueError('Ambiguous parent x87 topology')
    return{pc:next(iter(values))for pc,values in depths.items()}
def corpus(image,limit=None):
    rng=random.Random(0x136ea0);rows=[];live=('0000000000000080ff3f','35c26821a2da0fc90040','45230100000000c0ff7f')
    for family in range(41):
      for variant in range(8):
        banks=[bytearray(image.read(base,size))if base==0x46a000 else bytearray(size)for base,size in REGIONS]
        context=0x61001000+variant;descriptor=0x61005000+variant;bsp=0x61009000+variant;clusters=0x6100a000+variant;instances=0x6100c000+variant;definitions=0x6100e000+variant;queue=0x61010000+variant;instance_queue=0x61010100+variant;aux=0x61013000+variant;sp=0x62002000+variant
        # Use the secondary-view address and caller stack on one variant of
        # each family to exercise the native transport against the same i386 body.
        secondary=variant==0
        if secondary:context,sp=0x5598f8,0x60002000
        return_pc=0x134caa if secondary else 0x63000000
        def put(at,value,width=4):
            for(base,size),bank in zip(REGIONS,banks):
                if base<=at<=base+size-width:bank[at-base:at-base+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little');return
            raise ValueError('Unbacked BSP fixture '+hex(at))
        def get(at):
            for(base,size),bank in zip(REGIONS,banks):
                if base<=at<=base+size-4:return int.from_bytes(bank[at-base:at-base+4],'little')
            raise ValueError('Unbacked BSP input')
        bits=lambda value:struct.unpack('<I',struct.pack('<f',value))[0]
        cluster_count=0 if family==0 else 1 if family<4 else 2 if family<12 else 3
        flags=0x20 if family==1 else (0x200 if family%3 else 0)|(0x80 if family==7 else 0)|(1 if family in(10,11,18,19)else 0)
        put(context,descriptor);put(context+12,queue);put(context+24,instance_queue);put(context+0x2a60,flags);put(context+0x2a7c,(0,1,2,7)[family%4]);put(context+0x2a74,bits(12.));put(context+0x2ad0,aux if flags&1 else 0)
        put(context+0x2a78,0);put(context+0xa5e,510 if family==15 else 0,2);put(descriptor+0xa6c,cluster_count,2)
        for axis in range(3):put(context+0x2a68+axis*4,bits(0.));put(context+0x2a88+axis*4,bits(1. if axis==0 else 0.))
        put(context+0x2a94,bits((-4.,0.,4.)[variant%3]))
        put(0x4ecc18,bsp);put(0x4ecc20,0x61017000+variant);put(0x4c67ac,0x12340002 if family%2 else 0xffffffff);put(0x4f2d98,0x10 if family==17 else 0);put(0x4f44ec,0x12345678)
        put(bsp+0xa0,clusters);put(bsp+0x144,instances);put(bsp+0x13c,definitions);put(bsp+0x68,0x61018000+variant);put(0x61017000+variant+0x34c,0x61019000+variant);put(0x61019000+variant+2*16+8,0x12340002)
        for header,base in((queue,0x61011000+variant),(instance_queue,0x61012000+variant),(aux+12,0x61014000+variant),(aux+24,0x61015000+variant)):
            if header in(aux+12,aux+24):put(header,base);header=base;base+=0x100
            put(header,1 if family==8 else 8);put(header+4,0,2)
            for offset,delta in((8,0),(12,0x40),(16,0x80),(20,0xc0)):put(header+offset,base+delta)
        # Prior cluster IDs and auxiliary object IDs are backed independent
        # queues; the actual retained algorithm decides every match/outcome.
        put(0x61014000+variant+4,0 if family==10 else 3,2)
        for index in range(3):put(0x61014140+variant+index*4,index if family!=11 else index+5)
        for index in range(3):
            entry=descriptor+0xa6e+index*26;row=clusters+index*0xb0;put(entry,index,2)
            put(row+0x24,0 if family==2 else 1,2);put(row+0x70,0xff if family==16 else 2,1)
            for axis in range(3):put(row+0x54+axis*8,bits(-1.+index));put(row+0x58+axis*8,bits(1.+index))
            count=0 if family<4 else 1 if family<12 else 2;put(row+0x98,count);put(row+0x9c,0x61016000+variant+index*16)
            for item in range(count):put(0x61016000+variant+index*16+item*2,(index+item)%3,2)
            instance=instances+index*88;put(instance+0x34,index,2);put(definitions+index*200+0x24,0 if family==6 else 1,2)
            for axis in range(3):put(instance+0x3c+axis*4,bits(index*0.25 if axis==0 else 0.))
            put(instance+0x48,bits(1.));put(0x4f44f0+index*4,0)
        # Mode0 child frustum uses the real descriptor's empty first count;
        # nonzero containing ranges exercise the actual184690 child too.
        put(descriptor,1 if family>=20 else 0,2)
        for group in range(6):put(descriptor+0xa70+group*2,1 if family>=20 and not group else 0,2);put(descriptor+0xa7c+group*2,0,2)
        frustum=descriptor+0x1974
        for axis in range(3):put(frustum+0x60+axis*8,bits(-10.));put(frustum+0x64+axis*8,bits(10.))
        for plane in range(6):
            values=[0.,0.,0.,9.];values[plane//2]=1. if plane%2==0 else -1.
            for lane,value in enumerate(values):put(frustum+0x78+plane*16+lane*4,bits(value))
        # Authored descriptor groups overlap the first cluster row. Restore
        # its actual index after the independent frustum range metadata.
        put(descriptor+0xa6e,0,2)
        if family==9:put(instance_queue,1)
        if family==12:put(context+0x2a7c,7)
        if family==13:put(context+0x2a60,flags|0x80)
        if family==14:put(context+0x2a7c,1);put(context+0x2a74,bits(0.1))
        if family==18:put(queue+12,context+0x2a60)
        if family==19:
            put(instance_queue+12,instance_queue+4)
            # Keep a real counter alias backed across the entire transaction:
            # after the first publication every later occurrence is a repeat
            # of that actual datum, rather than corrupting a stream pointer.
            for cluster in range(3):
                for item in range(2):put(0x61016000+variant+cluster*16+item*2,0,2)
        if family in(24,25):
            # Raw signed authored indices remain distinct from the validated
            # public API. Both addresses and the wrapped marker bank are real.
            authored=-1 if family==24 else 511;put(descriptor+0xa6c,1,2);put(descriptor+0xa6e,authored,2)
            row=clusters+authored*176;put(row+0x24,1,2);put(row+0x98,0);put(row+0x70,0xff,1)
            for axis in range(3):put(row+0x54+axis*8,bits(-1.));put(row+0x58+axis*8,bits(1.))
        if family==26:
            for index in range(3):
                for axis in range(3):put(clusters+index*176+0x54+axis*8,0x7fc01234+axis);put(clusters+index*176+0x58+axis*8,0x7fc05678+axis)
        if family==27:
            put(context+0x2a60,flags|0x200)
            for axis in range(3):put(context+0x2a88+axis*4,0x7fc01234+axis);put(instances+axis*88+0x3c,0x7fc05678+axis)
        if family==28:
            for index in range(3):put(clusters+index*176+0x54,0xff800000);put(clusters+index*176+0x58,0x7f800000)
        if family==29:
            for index in range(3):
                put(instances+index*88+0x48,1);put(instances+index*88+0x3c,0x80000001);put(clusters+index*176+0x58,2)
        if family==30:
            for index in range(3):
                for axis in range(3):put(clusters+index*176+0x54+axis*8,0x80000000);put(clusters+index*176+0x58+axis*8,0)
        if family==31:put(context+0x2a7c,0);put(context+0x2a60,flags|0x80);put(descriptor,0,2)
        if family==32:put(queue+8,sp-0x4c)
        if family==33:put(instance_queue+8,sp-0x44)
        if family==34:put(queue+16,sp-0x28)
        if family==35:put(instance_queue+12,context+0x2a60)
        if family==36:put(context+0xa5e,512,2);put(context+0x2a7c,7)
        if family>=37:
            put(context+0x2a7c,7);put(0x4f44f0,0x12345678)
            put(instance_queue+4,0 if family==37 else 0xffff if family==38 else 3,2)
            for index in range(3):put(0x61012040+variant+index*4,10+index);put(0x610120c0+variant+index*2,0,2)
        if family==40:
            put(context+0x2a60,0x200);put(context+0x2a94,bits(1.));put(context+0x2a7c,7)
            for index in range(3):
                for axis in(1,2):put(clusters+index*176+0x54+axis*8,bits(0.));put(clusters+index*176+0x58+axis*8,bits(0.))
                put(clusters+index*176+0x54,bits(-1.));put(clusters+index*176+0x58,bits(1.))
                put(instances+index*88+0x3c,bits(0.));put(instances+index*88+0x48,bits(1.))
        gp=[rng.getrandbits(32)for _ in range(8)];gp[4]=sp
        put(sp,return_pc);put(sp+4,context);put(sp+8,rng.getrandbits(32))
        depth=variant%4;cw=(0x7f,0x27f,0x37f)[variant%3]|((variant%4)<<10);status=((variant%8)<<11)|0x4500
        rows.append(dict(id=len(rows),name=f'family{family}-variant{variant}',entry=0x136ea0,gp=gp,eflags=0x202|(0x400 if variant%2 else 0)|(rng.getrandbits(12)&0x8d5),matrix=get(sp+4),source=get(sp+8),max_steps=100000,xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],before_fp=[cw,status,0x1f80|((family%4)<<13)],live_x87=list(live[:depth]),banks=[dict(address=base,before=bytes(bank).hex())for(base,_),bank in zip(REGIONS,banks)],stop_pc=return_pc,secondary_stack=secondary))
        if limit and len(rows)>=limit:return rows
    return rows
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--xbe',type=Path,required=True);parser.add_argument('--report',type=Path,required=True);parser.add_argument('--compiler',default='g++');parser.add_argument('--sanitizers',action='store_true');parser.add_argument('--limit',type=int);parser.add_argument('--native-only',action='store_true');parser.add_argument('--prefixes',action='store_true');parser.add_argument('--require-binding',action='store_true');parser.add_argument('--semantic-mutants',action='store_true');args=parser.parse_args()
    if sys.platform!='linux'or platform.machine().lower()not in('x86_64','amd64','i386','i686')or not args.xbe.is_file():return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);pins={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest()for name in INPUTS};source,parent=prepare(scratch,args.require_binding)
    sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
    image=Xbe(args.xbe);assert hashlib.sha256(image.data).hexdigest()==PIN
    import oracle_native_campaign_screen_lifecycle_tu15 as common
    original_metadata=common.metadata
    def metadata(image):
        result=original_metadata(image);code=common._Code(image,'.text')
        for site in result['sites']:
            if code.at(site['instruction']).mnemonic in('ret','pop'):site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
        return result
    common.metadata=metadata;entries=(0x136ea0,0x135970,0x136e10,0x171070,0x167ef0,0x184690,0x166c00);common.ENTRIES=entries;sites=metadata(image)['sites'];code=common._Code(image,'.text')
    io=sorted({site['instruction']for site in sites if site['operands']or code.at(site['instruction']).mnemonic in('push','call')});write_sites=[site['instruction']for site in sites if any(o['write']for o in site['operands'])or code.at(site['instruction']).mnemonic in('push','call')]
    rmw_sites=[site['instruction']for site in sites if any(o['write']and o['read']for o in site['operands'])]
    extra=sorted(set(entries[1:])-set(io));deltas=relative_depths(parent);deltas.update({0x135a87:1,0x135a89:1,0x135a8f:2,0x135a92:2,0x135a98:3,0x135ab3:1,0x135ab7:1})
    calls=[dict(entry=pc,gp=True,xmm=True,x87_delta=deltas.get(pc,0))for pc in sorted(set(io)|set(extra))];rows=corpus(image,args.limit);physical,authority=observe_batches(image,rows,scratch/'normal',entries=entries,regions=REGIONS,trace_calls=calls,trace_extended=True)
    prefix_authority=None;witnesses=set()
    if args.prefixes:
        prefixes=[]
        alias_sites={0x137100,0x137312,0x137357,0x13735f,0x137369,0x13738b,0x1373ab,0x1373e3,0x1373e4,0x1373e5,0x1373e6,0x1373ea}
        child_sites={0x135970,0x136e12,0x171070,0x167ef7,0x184690,0x166c02}
        for row,observed in zip(rows,physical['cases'],strict=True):
            depth=len(row['live_x87']);family=int(row['name'].split('-')[0][6:])
            if depth not in(0,3):continue
            domain=family if family in(19,32,33,37,38,39)else'ordinary'
            for phase,key in(('before_read','reads'),('before_write','writes'),('after_write','writes')):
                occurrences={}
                for ordinal,event in enumerate(observed[key]):
                    pc=event['instruction'];occurrence=occurrences.get(pc,0);occurrences[pc]=occurrence+1
                    if not 0x136ea0<=pc<=0x1373ea and pc not in child_sites:continue
                    if domain!='ordinary'and pc not in alias_sites:continue
                    actual_phase='before_instruction'if phase=='before_write'and pc in rmw_sites else phase
                    witness=(actual_phase,pc,depth,domain)
                    if witness in witnesses:continue
                    witnesses.add(witness);fault=dict(phase=actual_phase,ordinal=occurrence if actual_phase=='before_instruction'else ordinal)
                    if actual_phase=='before_instruction':fault['instruction']=pc
                    prefix=copy.deepcopy(row);prefix.update(id=len(rows)+len(prefixes),name=row['name']+f'-{actual_phase}-{ordinal}',fault=fault);prefixes.append(prefix)
                    if actual_phase=='before_instruction'and domain=='ordinary':
                        reads=[item for item in observed['reads']if item['instruction']==pc]
                        provider=copy.deepcopy(row);provider.update(id=len(rows)+len(prefixes),name=row['name']+f'-provider_before_write-{ordinal}',fault=dict(phase='before_write',ordinal=ordinal),original_stop=fault,provider_rmw_read=reads[occurrence]);prefixes.append(provider)
        stopped,prefix_authority=observe_batches(image,prefixes,scratch/'prefix',entries=entries,regions=REGIONS,trace_calls=calls,trace_extended=True);rows+=prefixes;physical['cases']+=stopped['cases']
    for row,observed in zip(rows,physical['cases'],strict=True):
        row.update({key:observed[key]for key in('boundary_gp','boundary_eflags','boundary_xmm','after_fp','reads','writes')});row['expected_pc']=observed['boundary']['instruction']if row.get('fault')else observed['calls'][-1]['entry'];row['after_live_x87']=observed['boundary_live_x87'];row['extra_child_calls']=[];row['calls']=[]
        if row.get('provider_rmw_read'):row['reads']=row['reads']+[row['provider_rmw_read']]
        for call_index,call in enumerate(observed['calls']):
            assert call.pop('arguments')==[]
            if row.get('fault',{}).get('phase')=='before_instruction'and call_index+1==len(observed['calls']):continue
            row['extra_child_calls'if call['entry']in extra else'calls'].append(call)
            if call['entry']in rmw_sites and not(row.get('fault',{}).get('phase')=='before_read'and call_index+1==len(observed['calls'])):row['calls'].append(call.copy())
        for bank,after in zip(row['banks'],observed['banks'],strict=True):bank['after']=after['after']
    fixture=scratch/'fixture.json';binary=scratch/'bsp-tests';command=[args.compiler,'-std=c++20','-O2','-Wno-ignored-attributes','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(scratch),'-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(ROOT/'tests/native_tu15_bsp_admission_tests.cpp'),'-o',str(binary)]
    if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
    compiled=subprocess.run(command,capture_output=True,text=True)
    if compiled.returncode:raise ValueError('BSP controller compile failed: '+compiled.stderr[-6000:])
    outcomes=[];tested=None
    for index in range(0,len(rows),64):
        fixture.write_text(json.dumps(dict(cases=rows[index:index+64],write_sites=write_sites,rmw_sites=rmw_sites,extra_child_entries=extra),separators=(',',':')))
        tested=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',**({'HALO2_BSP_ADMISSION_NATIVE_ONLY':'1'}if args.native_only else{})))
        if tested.returncode:break
        outcomes.append(json.loads(tested.stdout))
    checks=[]
    if tested.returncode==0 and not args.native_only:
        for option,env in(('--guards',{}),('--reference-guards',{'HALO2_REFERENCE_BSP_ADMISSION':'1'})):
            done=subprocess.run([str(binary),option],capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',**env));checks.append(dict(option=option,passed=done.returncode==0,diagnostic=(done.stdout+done.stderr)[-4000:]))
        normal=[row for row in rows if'fault'not in row];forced=[]
        for index in range(0,len(normal),64):
            fixture.write_text(json.dumps(dict(cases=normal[index:index+64],write_sites=write_sites,rmw_sites=rmw_sites,extra_child_entries=extra),separators=(',',':')))
            done=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',HALO2_REFERENCE_BSP_ADMISSION='1',HALO2_BSP_ADMISSION_REFERENCE_TEST='1'))
            if done.returncode:break
            forced.append(json.loads(done.stdout))
        checks.append(dict(option='positive_forced_reference',passed=done.returncode==0,diagnostic=(done.stdout+done.stderr)[-4000:],counts={key:True if key=='passed'else sum(item[key]for item in forced)for key in forced[0]}if forced else{}))
    from native_bsp_admission_mutations import check as check_mutants
    mutants=check_mutants(scratch,command,rows,dict(write_sites=write_sites,rmw_sites=rmw_sites,extra_child_entries=extra))if args.semantic_mutants and tested.returncode==0 and all(item['passed']for item in checks)else[]
    changed=[name for name,pin in pins.items()if hashlib.sha256((ROOT/name).read_bytes()).hexdigest()!=pin]
    report=dict(passed=tested.returncode==0 and not changed and all(item['passed']for item in checks),private_unbound=not args.require_binding,owned_scratch=str(scratch),source_sha256=pins,changed_inputs=changed,source_authority=source,physical_cases=len(rows),executed_sites=sorted({hex(pc)for row in physical['cases']for pc in row['executed_sites']}),physical_original=authority,rmw_scope='Normal RMW retains one backed read plus one backed write at the exact pre-instruction ABI. Atomic CPU beforeInstruction prefixes execute neither access. Separately declared atomic-provider write failures retain their additional real read and compare CPU-atomic GP/flags/FP/banks without dropping that provider event.',provider_rmw_additional_reads=sum(bool(row.get('provider_rmw_read'))for row in rows),additional_source_checks=checks,compiled_semantic_mutants=mutants,compiler=args.compiler,sanitizers=args.sanitizers,diagnostic=(tested.stdout+tested.stderr)[-16000:],retained_large_artifacts=[])
    report['reference_parent_included']=not args.native_only
    report.update(prefix_cases=sum('fault'in row for row in rows),physical_prefix_original=prefix_authority,prefix_witnesses=len(witnesses))
    if tested.returncode==0:report['native']={key:True if key=='passed'else sum(outcome[key]for outcome in outcomes)for key in outcomes[0]}
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(tested.stdout+tested.stderr);return 0 if report['passed']else 1
if __name__=='__main__':
    try:raise SystemExit(main())
    except Exception as error:
        if '--report'in sys.argv:
            report=Path(sys.argv[sys.argv.index('--report')+1]);report.parent.mkdir(parents=True,exist_ok=True);report.write_text(json.dumps(dict(passed=False,private_unbound=True,owned_scratch=os.environ.get('HALO2_TEST_SCRATCH'),error=str(error)[-10000:],retained_large_artifacts=[]),indent=2)+'\n')
        raise
