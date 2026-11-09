#!/usr/bin/env python3
"""Complete BF0C0 DATA/ABI, source-default and unchanged-original qualification."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import struct
import re
from generated_routine_ownership import function_bodies
from native_model_default_pose_replacements import canonical_body as pose_body
from native_node_descriptor_lookup_replacements import canonical_body as lookup_body
from native_node_resource_lifecycle_replacements import canonical_body as lifecycle_body
from native_object_node_validation_support import admission
from native_object_node_rebuild_replacements import canonical_body as object_node_body

ROOT=Path(__file__).resolve().parents[1]
SOURCE='src/physics/physics_live_support_tu15_generated.cpp'
PINS={
    'b87b0':'e17db6b696190912c4580a562d4ed5709062587f887ef9e137c0615b5bfd3feb',
    'bac00':'2f6181562d97688dc9a82ed07b74ebc8902bbeffdaeb5f769a83b053324c0f7d',
    'bf0c0':'8bf592452daafe279643d1d0a060a3cc4daaed26add47339ffc0aa6ac702b950',
    'bfc70':'6af47eae3cd0efd8498ed33181d0544da481ce8f77ed48ff1a706fffe1da82d0',
    'c15d0':'1f82fe69b7a3829cfa44f82d2f94e8d019535780caf543ac285f687ac2ee36d0',
    'c1cc0':'4c4ec1fcffdb336a1814a65d87495bb69d024041f50fff7906a2ce7f1b809a5c',
    'c1e50':'ecf61974477692ad62a238f9fb40652ff2c304fe77dc191be20d177d11dbf393',
    'c1eb0':'aa087828975176afa333100f0c9948512d121b7d51c1515829a0ef8dcf36012b',
    'c1f00':'74ec02a49102483fbaeb033e22c62770bc847a72c1f870c1d4698068a4f07afa',
    'c1f40':'951b740c0c282fdb615742d459a5e420157f711d21a668257d14b85dfef67c54',
    'e3bc0':'b66ed0be3ef3e658a7d9376c0e846ef53079a090fc31578e7aeb3a88c299a092',
    '10e180':'451de91dc1b2f0da4934288dafa906869fe5bc53f4307f350971256f8aecfac9',
    '144cf0':'e680e131154417f038174f1534479663acbac718ce59da3e04e151a632000b88',
    '145560':'759d452a7d5ff296864ccd8e823526bdcb3e7fe3ee57ae7222624201619ddece',
    '1707a0':'20588ec7ed106df5ca1b3ddebf3bff8f407ab846e1cc68947773c456c4684de4',
    '1dbbd0':'d826db4f4102ffd958ad61ec4ed40bb7d2d0b65bccd4fba4eb64651e3aca5b50',
    'b3a50':'18c3ea134bece48950e839a24f5bf93db535376e21852d7c1a49d60cebe637d8',
    'b3ae0':'fe7e3e95e3c3f3df10ffcc1bb78485e6c03dbccd924c32e7adccc697f0b616e8',
    'b3b70':'6934b3e1e032a288d946089daab11b8054860b4d095dbcec64087738e255e48a',
    'b8780':'a9ebd40c6156f79b7d0347658c21346f31c8267d381475364827dc9468fc9085',
    'bfb90':'566e4cb12c806b179051d6db181c2938d691490ab271f8930b918243aa50bba1',
    '141010':'224711807b116d67f7613f7411a02fdef5a51fbfbcadbe1f149f7ae656965530',
    '1e3150':'2cf839a496fd2c54ea06fa5b8f6ff3e035971e0ae8e8e50e74272aa294f2b63e',
    '1e5c80':'7cf1b9b1a510000321239f60c3d2b5a4dbc34d63165cde5a0daf09eb586a5160',
    'adcd0':'8308d639cb2a5de597db3c3e85dec6553185384e6e58adfe074431c268986310'}
BOUNDARIES=('1dbfd0','1d5840','bf9a0','10e100','1e1720','1f4920','1e3270','1e5af0','126200')
def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()

def prediction_provenance(path):
    from research_mp_sandbox_update_functions import Xbe,NEW_PIN
    if digest(path)!=NEW_PIN:raise ValueError('Wrong TU1.5 reference image')
    image=Xbe(path);opcodes={0x69229:'c70554c04d0008585500',0x85eaf:'c74028b45f4800',
        0xadcd0:'568b74240c578b7c241483c668b90a000000f3a55fb0015ec20c00'}
    for address,raw in opcodes.items():
        if image.read(address,len(bytes.fromhex(raw))).hex()!=raw:raise ValueError('Original prediction registration/copy opcode drift')
    words={0x485fb4:0x45dc78,0x45dd00:0xadcd0}
    for address,value in words.items():
        if struct.unpack('<I',image.read(address,4))[0]!=value:raise ValueError('Original registered prediction vtable drift')
    return dict(original_xbe_sha256=NEW_PIN,actual_registry_type=9,registered_object='485fb4',vtable='45dc78',slot='45dd00',actual_owner='adcd0',
        authenticated_opcode_bytes={f'{key:x}':value for key,value in opcodes.items()},
        authenticated_original_words={f'{key:x}':f'{value:x}'for key,value in words.items()},
        original_owner_bytes=27,original_owner_sha256=hashlib.sha256(image.read(0xadcd0,27)).hexdigest(),supplied_original_outcomes=False)

def recover(scratch):
    source=(ROOT/SOURCE).read_text();bodies={match[1]:body for match,body in function_bodies(source)if match[1]in PINS}
    for name in ('bf0c0','bfb90','b8780','b87b0'):bodies[name]=object_node_body(name,bodies[name])
    bodies['1707a0']=pose_body(bodies['1707a0'])
    bodies['1e3150']=lookup_body(bodies['1e3150']);bodies['1e5c80']=lifecycle_body(bodies['1e5c80'])
    bodies['adcd0']=next(body for match,body in function_bodies((ROOT/'tests/native_tu15_object_prediction_copy_reference.inc').read_text())if match[1]=='adcd0')
    for name,pin in PINS.items():
        if hashlib.sha256(bodies[name].encode()).hexdigest()!=pin:raise ValueError('Complete canonical owner '+name+' changed')
    helpers=source[source.index('__m128 native_load_half('):source.index('long double native_fpatan(')]
    if hashlib.sha256(helpers.encode()).hexdigest()!='1e2353f8b322f6cb4b9509626c6490add0a6b1b36ce3f26116eeb8a43819dc4c':raise ValueError('Canonical half-access helper drift')
    text=''.join(f'static void run_{name}(WorldState&);\n'for name in (*PINS,*BOUNDARIES))
    text+='static void dispatch(WorldState&,std::uint32_t,std::uint32_t,bool);\n'
    text+=helpers+'\nstd::uint32_t sign_extend(std::uint32_t v,unsigned n){return n==1?std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(v)))):std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(v))));}\n'
    for name,body in bodies.items():
        if name!='bf0c0':
            begin=body.index('{\n')+2;prefix=f'    note(s,0x{name}u);\n'
            if name=='adcd0':prefix+='if(use_native_prediction&&native_object_prediction_copy_detail::bridge(s)){++current->native_prediction_calls;return;}\n'
            if name in ('bfb90','b8780','b87b0'):
                prefix+=f'if(use_native_animation&&native_object_animation_detail::bridge(s,0x{name}u,[](auto&v,std::uint32_t owner,std::uint32_t site){{dispatch(v,owner,site,false);}}))return;\n'
            body=body[:begin]+prefix+body[begin:]
        text+=body+'\n'
    for name in BOUNDARIES:text+=f'static void run_{name}(WorldState&s){{note(s,0x{name}u);throw Boundary{{0x{name}u}};}}\n'
    text+='static void dispatch(WorldState&s,std::uint32_t target,std::uint32_t,bool){switch(target){'
    for name in (*PINS,*BOUNDARIES):text+=f'case 0x{name}u:run_{name}(s);return;'
    text+='default:note(s,target);throw Boundary{target};}}\n'
    (scratch/'object_node_rebuild_reference.inc').write_text(text)
    return dict(whole_canonical_body_sha256=PINS,actual_child_entry_stops=list(BOUNDARIES),reference_native_prefixes_removed=['1707a0','1e3150','1e5c80'],supplied_child_outcomes=False,
        prediction_registered_type=dict(registry_publication_site='69229',registry='555808',entry_publication_site='85eaf',entry_offset='28',descriptor='485fb4',vtable='45dc78',vtable_slot='45dd00',actual_owner='adcd0'))

REGIONS=tuple((base,4096)for base in (0x45d000,0x45e000,0x45f000,0x46a000,0x474000,0x485000,0x4dc000,0x4e8000,0x4ec000,0x4f0000,0x51d000,0x52b000,0x553000,0x555000,0x56e000))+((0x61000000,262144),(0x62000000,16384))
ARGUMENTS={0xbf0c0:1,0xbac00:1,0x145560:1,0x10e180:1,0x1e3150:1,0x1e5c80:1,0xb3a50:1,
    0x1707a0:2,0x144cf0:2,0x1e1720:2,0x1dbbd0:3,0xbfb90:3,0xadcd0:3,0x10e100:3,
    0xbfc70:5,0x1dbfd0:5,0xbf9a0:6,0x1d5840:7}
DIRECT_CHILDREN={0xbac00,0xe3bc0,0x1dbbd0,0x1707a0,0xc15d0,0xc1cc0,0xc1f40,0xc1eb0,0xc1f00,
    0xbfb90,0xc1e50,0xb87b0,0xb8780,0xb3ae0,0xb3b70,0xbfc70,0x144cf0,0x145560,0x10e180}

def hardware(args,scratch,binary):
    from validate_native_tu15_prt_helpers import physical_original,PIN
    from validate_native_object_node_hierarchy_tu15 import HALF_STORES
    import oracle_native_campaign_screen_lifecycle_tu15 as common
    from research_mp_sandbox_update_functions import Xbe
    image=Xbe(args.xbe);rows=[];requests=[]
    generated=subprocess.run([str(binary),'--emit-fixtures'],capture_output=True)
    if generated.returncode:raise RuntimeError('Controller genuine fixture export failed: '+generated.stderr.decode()[-4000:])
    raw=generated.stdout;at=0
    def words(count):
        nonlocal at
        result=struct.unpack_from('<'+'I'*count,raw,at);at+=count*4;return result
    for _ in range(words(1)[0]):
        mode,df,fault,boundary=words(4);abi=words(44);live=[]
        for _ in range(words(1)[0]):live.append(raw[at:at+10].hex());at+=10
        banks=[]
        for base,size in REGIONS:banks.append(raw[at:at+size]);at+=size
        # Feedback tests remain in the independent canonical cohort. Physical
        # inputs execute unchanged source and receive no observer mutation.
        if mode in (18,19,20,21,22,24,25,26,34)or fault:continue
        def get(address,width=4):
            for (base,size),bank in zip(REGIONS,banks):
                if base<=address and address+width<=base+size:return int.from_bytes(bank[address-base:address-base+width],'little')
            raise ValueError('Controller exported word escaped bank')
        sp=abi[4];rows.append(dict(id=len(rows),name=f'controller{mode}-DF{df}',entry=0xbf0c0,gp=list(abi[:8]),eflags=abi[8],matrix=get(sp+4),source=get(sp+8),
            before_fp=[abi[11],abi[10],abi[9]],live_x87=live,xmm=[list(abi[12+n*4:16+n*4])for n in range(8)],max_steps=100000,
            stop_before_entries=[int(name,16)for name in BOUNDARIES],banks=[dict(address=base,before=bank.hex())for (base,_),bank in zip(REGIONS,banks)]))
        requests.append((abi,live,banks,boundary))
    if at!=len(raw):raise ValueError('Controller fixture export protocol drift')
    original_metadata=common.metadata;authored=[]
    def metadata_with_real_half_stores(owner_image):
        metadata=original_metadata(owner_image)
        for site in metadata['sites']:
            pc=site['instruction']
            if pc not in HALF_STORES:continue
            if site['bytes']!=HALF_STORES[pc]or owner_image.read(pc,len(bytes.fromhex(site['bytes']))).hex()!=HALF_STORES[pc]:raise ValueError('Controller matrix half-store opcode drift')
            if len(site['operands'])!=1 or site['operands'][0]['bytes']!=8:raise ValueError('Controller matrix half-store operand drift')
            authored.append(dict(instruction=f'{pc:x}',bytes=site['bytes'],original_metadata=dict(site['operands'][0]),read=False,write=True));site['operands'][0].update(read=False,write=True)
        if len(authored)!=len(HALF_STORES):raise ValueError('Controller matrix half-store ownership drift')
        return metadata
    common.metadata=metadata_with_real_half_stores
    entries=tuple(int(name,16)for name in PINS)
    diagnostic_sites=(0xbfbda,0xbfbdf,0xbfbe7,0xb8791,0xb8796,0xb87a1)
    try:
        observed,proof=physical_original(image,rows,scratch,entries=entries,regions=REGIONS,
            trace_calls=[dict(entry=e,arguments=ARGUMENTS.get(e,0),gp=True,xmm=True)for e in (*entries,*diagnostic_sites)if e!=0xbf0c0],trace_extended=True)
    finally:common.metadata=original_metadata
    diagnostic=[dict(name=rows[case['id']]['name'],packets=[call for call in case['calls']if call['entry']in diagnostic_sites or call['entry']==0x1e3150])for case in observed['cases']]
    (scratch/'controller-child-shift-diagnostics.json').write_text(json.dumps(diagnostic))
    code=common._Code(image,'.text');inputs,expected,actual=(scratch/name for name in ('parent.in','parent.expected','parent.actual'))
    segments=[];packets=0;stops=0;returned=0
    def emit_abi(out,gp,flags,fp,xmm):
        out.write(struct.pack('<12I',*gp,flags,fp[2],fp[1],fp[0]))
        for lane in xmm:out.write(struct.pack('<4I',*lane))
    with inputs.open('wb')as inp,expected.open('wb')as out:
        for case,request in zip(observed['cases'],requests,strict=True):
            abi,live,banks,boundary=request;name=rows[case['id']]['name']
            if not case['completed']or not case['live_x87_unchanged']:raise ValueError('Physical controller failed complete FP/normal boundary')
            if case['stopped_at']!=(boundary or 0x63000000):raise ValueError('Physical/canonical genuine child boundary differs '+name)
            inp.write(struct.pack('<44I',*abi));inp.write(struct.pack('<I',len(live)))
            for value in live:inp.write(bytes.fromhex(value))
            inp.write(b''.join(banks));segments.append((out.tell(),name,'return_abi'))
            emit_abi(out,case['boundary_gp'],case['boundary_eflags'],case['after_fp'],case['boundary_xmm'])
            out.write(b''.join(bytes.fromhex(bank['after'])for bank in case['banks']));out.write(struct.pack('<I',len(case['writes'])))
            for event in case['writes']:
                data=bytes.fromhex(event['written']);out.write(struct.pack('<III',event['instruction'],event['address'],len(data)));out.write(bytes.fromhex(event['before']));out.write(data)
            calls=[call for call in case['calls']if call['entry']not in diagnostic_sites]
            if boundary:
                stops+=1
                def retained_word(address):
                    for bank in case['banks']:
                        base=bank['address'];data=bytes.fromhex(bank['after'])
                        if base<=address and address+4<=base+len(data):return int.from_bytes(data[address-base:address-base+4],'little')
                    raise ValueError('Physical genuine callee argument escaped bank')
                returned_to=retained_word(case['boundary_gp'][4]);sites=[pc for pc in case['executed_sites']if code.at(pc).mnemonic=='call'and pc+code.at(pc).size==returned_to]
                if len(sites)!=1:raise ValueError('Controller genuine boundary caller PC not authenticated')
                final_pc=sites[0]
                calls=[*calls,dict(entry=boundary,gp=case['boundary_gp'],eflags=case['boundary_eflags'],fp=case['after_fp'],xmm=case['boundary_xmm'],live_x87=live,
                    arguments=[retained_word(case['boundary_gp'][4]+4+k*4)for k in range(ARGUMENTS.get(boundary,0))])]
            else:
                returned+=1;sites=set(case['executed_sites'])&{0xbf5c0,0xbf62f}
                if len(sites)!=1:raise ValueError('Controller final return-site drift')
                final_pc=sites.pop()
            out.write(struct.pack('<4I',final_pc,boundary,0,len(calls)))
            for call in calls:
                segments.append((out.tell(),name,f"callee_{call['entry']:x}"));out.write(struct.pack('<I',call['entry']));emit_abi(out,call['gp'],call['eflags'],call['fp'],call['xmm'])
                out.write(struct.pack('<I',len(call['live_x87'])))
                for value in call['live_x87']:out.write(bytes.fromhex(value))
                out.write(struct.pack('<I',len(call['arguments'])*4))
                for value in call['arguments']:out.write(struct.pack('<I',value))
                packets+=1
    with inputs.open('rb')as inp,actual.open('wb')as out:result=subprocess.run([str(binary),'--physical'],stdin=inp,stdout=out,stderr=subprocess.PIPE)
    if result.returncode:raise RuntimeError('Native complete parent physical probe failed: '+result.stderr.decode()[-6000:])
    original_stream=expected.read_bytes()
    def compare(path,allow_nested=False):
        candidate=path.read_bytes();failures=[];parent_actual=hashlib.sha256();parent_original=hashlib.sha256();direct_packets=0;nested_packets=0
        if len(candidate)!=len(original_stream):raise ValueError('Whole parent packet framing/length mismatch')
        for index,segment in enumerate(segments):
            end=segments[index+1][0]if index+1<len(segments)else len(candidate)
            left,right=candidate[segment[0]:end],original_stream[segment[0]:end]
            entry=int(segment[2].split('_')[1],16)if segment[2].startswith('callee_')else None
            if entry is None or entry in DIRECT_CHILDREN:
                parent_actual.update(left);parent_original.update(right);direct_packets+=entry is not None
                if left!=right:
                    first=next(i for i,(x,y)in enumerate(zip(left,right))if x!=y)
                    raise ValueError(f'Whole parent physical mismatch segment{segment},relative{first}: {left[first:first+32].hex()} vs{right[first:first+32].hex()}')
            else:
                nested_packets+=1
                if left!=right:
                    detail=next(row for row in diagnostic if row['name']==segment[1])
                    failures.append(dict(case=segment[1],entry=f'{entry:x}',native_packet=left.hex(),original_packet=right.hex(),
                        original_diagnostic_packets=[{key:call[key]for key in('entry','gp','eflags','arguments')}for call in detail['packets']]))
        if failures and not allow_nested:raise ValueError('Native default retains unqualified nested child differences: '+str(failures[:1]))
        return dict(direct_parent_comparison_sha256=parent_actual.hexdigest(),original_direct_parent_comparison_sha256=parent_original.hexdigest(),
            exact_direct_parent_callee_packets=direct_packets,observed_nested_child_packets=nested_packets,
            full_transitive_child_packets_verified=not failures,unqualified_nested_child_packet_differences=failures)
    compared=compare(actual);copy_routes=[]
    from native_object_node_validation_support import CONTROLS
    environment={key:value for key,value in os.environ.items()if key not in CONTROLS}
    for label in ('primary','frame'):
        for control in ('default','parent-reference','child-reference'):
            env=dict(environment)
            if control=='parent-reference':env['HALO2_REFERENCE_OBJECT_NODE_REBUILD']='1'
            if control=='child-reference':env.update(HALO2_REFERENCE_OBJECT_ANIMATION='1',HALO2_REFERENCE_OBJECT_PREDICTION_COPY='1')
            path=scratch/f'{label}-{control}.actual'
            with inputs.open('rb')as inp,path.open('wb')as out:run=subprocess.run([str(binary),'--physical-'+label],stdin=inp,stdout=out,stderr=subprocess.PIPE,env=env)
            if run.returncode:raise RuntimeError('Private source-copy route failed: '+run.stderr.decode()[-4000:])
            activity=json.loads(run.stderr)
            if activity['native_parent_entries']!=(0 if control=='parent-reference'else len(rows)):raise ValueError('Object node source-copy parent control bypassed')
            if control=='parent-reference'and not activity['literal_parent_sites']:raise ValueError('Parent reference did not execute retained original body')
            if control!='parent-reference'and activity['literal_parent_sites']:raise ValueError('Default parent entered literal body')
            if control=='child-reference':
                if activity['native_animation_entries']or activity['native_prediction_entries']or not activity['literal_animation_sites']or not activity['literal_prediction_sites']:raise ValueError('Child reference controls were not independent')
            elif not activity['native_animation_entries']or not activity['native_prediction_entries']or activity['literal_animation_sites']or activity['literal_prediction_sites']:raise ValueError('Default source-copy child bypassed native bridge')
            copy_routes.append(dict(copy=label,control=control,activity=activity,comparison=compare(path,control=='child-reference'),
                native_complete_stream_sha256=digest(path),original_complete_stream_sha256=digest(expected)))
    executed={pc for case in observed['cases']for pc in case['executed_sites']};parent=next(body for match,body in function_bodies((ROOT/SOURCE).read_text())if match[1]=='bf0c0')
    parent_sites={int(pc,16)for pc in re.findall(r'services.instruction\((0x[0-9a-f]+)u\)',parent)}
    return dict(passed=True,cases=len(rows),whole_parent_returns=returned,genuine_child_entry_stops=stops,full_callee_packets=packets,
        parent_sites_executed=len(executed&parent_sites),parent_sites_unexecuted=[f'{pc:x}'for pc in sorted(parent_sites-executed)],
        comparison_bytes=actual.stat().st_size,native_complete_stream_sha256=digest(actual),original_complete_stream_sha256=digest(expected),
        **compared,private_default_copy_routes=copy_routes,
        physical_original=proof,authored_half_store_operands=authored,
        full_normal_abi_fp_banks_ordered_stores_and_callee_entries=True,native_prediction_copy=True,
        physical_pre_io_and_page_fault_atomicity_verified=False)

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--compiler',default='c++');parser.add_argument('--sanitizers',action='store_true');parser.add_argument('--hardware',action='store_true');parser.add_argument('--require-binding',action='store_true');parser.add_argument('--xbe',type=Path,required=True);args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Run through tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);paths=(SOURCE,'include/halo2/native_tu15_object_node_rebuild_kernel.hpp',
        'src/physics/runtime/native_tu15_object_node_rebuild_abi.hpp','include/halo2/native_tu15_object_node_hierarchy_kernel.hpp',
        'src/physics/runtime/native_tu15_object_node_hierarchy_abi.hpp','src/physics/runtime/native_tu15_matrix_leaves.hpp',
        'tests/native_tu15_object_node_rebuild_tests.cpp','tests/native_tu15_object_prediction_copy_reference.inc',
        'tests/native_tu15_object_node_admission_checks.hpp','tools/native_object_node_validation_support.py',
        'tools/native_object_node_copy_validation.py','tools/native_object_node_rebuild_replacements.py','src/campaign/ai/source_ai_frame_tu15_generated.cpp',
        'include/halo2/native_tu15_word_copy_kernel.hpp','src/physics/runtime/native_tu15_object_prediction_copy.hpp',
        'src/physics/runtime/native_tu15_object_node_admission.hpp',
        'include/halo2/native_tu15_object_animation_delivery_kernel.hpp','src/physics/runtime/native_tu15_object_animation_delivery_abi.hpp',
        'tools/validate_native_tu15_prt_helpers.py','tools/oracle_native_campaign_screen_lifecycle_tu15.py','tools/observe_native_campaign_ai_vector_math_tu15.cpp',
        'tools/native_model_default_pose_replacements.py','tools/native_node_descriptor_lookup_replacements.py','tools/native_node_resource_lifecycle_replacements.py',str(Path(__file__).relative_to(ROOT)))
    pins={name:digest(ROOT/name)for name in paths};report=dict(passed=False,production_bound=args.require_binding,source_sha256=pins,scratch_owner=str(scratch),retained_large_artifacts=[])
    try:
        report['prediction_dynamic_owner_provenance']=prediction_provenance(args.xbe)
        report['reference']=recover(scratch);binary=scratch/'object-node-rebuild'
        from native_object_node_copy_validation import prepare as prepare_default_copies
        report['private_default_copy_sources']=prepare_default_copies(scratch,(*PINS,*BOUNDARIES))
        if args.require_binding and not all(copy['source_already_installed']for copy in report['private_default_copy_sources']):raise ValueError('Complete object node source default binding missing')
        command=[args.compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_object_node_rebuild_tests.cpp'),'-o',str(binary)]
        if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        report['compiler_command']=command;subprocess.run(command,check=True)
        report['admission']=admission(binary,'HALO2_REFERENCE_OBJECT_NODE_REBUILD')
        result=subprocess.run([str(binary)],capture_output=True,text=True);report['stderr']=result.stderr[-10000:]
        if result.returncode:raise RuntimeError('Controller seam failed: '+report['stderr'])
        report['canonical_seam']=json.loads(result.stdout)
        report['source_hashes_unchanged']=pins=={name:digest(ROOT/name)for name in paths}
        if not report['source_hashes_unchanged']:raise ValueError('Controller proof input changed during validation')
        report.update(passed=True,source_controller_protocol_verified=True,unchanged_original_hardware_verified=False,
            whole_parent_default_routing_verified=bool(args.require_binding and args.hardware),
            complete_production_dispatcher_executed=False,native_child_dependency_closure_verified=False,
            physical_fault_atomicity_verified=False)
        if args.hardware:report['hardware']=hardware(args,scratch,binary);report['unchanged_original_hardware_verified']=True
        if pins!={name:digest(ROOT/name)for name in paths}:raise ValueError('Controller proof inputs changed during hardware validation')
    except Exception as error:report['error']=str(error);raise
    finally:
        args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(report['canonical_seam']));return 0

if __name__=='__main__':raise SystemExit(main())
