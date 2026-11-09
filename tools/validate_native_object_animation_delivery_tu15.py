#!/usr/bin/env python3
"""Complete BFB90/B8780/B87B0 typed DATA/ABI versus unchanged i386."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import subprocess
from generated_routine_ownership import function_bodies
from native_node_descriptor_lookup_replacements import canonical_body as lookup_body
from native_node_resource_lifecycle_replacements import canonical_body as lifecycle_body
from validate_native_object_node_rebuild_tu15 import PINS as PARENT_PINS
from native_object_node_validation_support import admission
from native_object_node_rebuild_replacements import canonical_body as object_node_body

ROOT=Path(__file__).resolve().parents[1]
SOURCE='src/physics/physics_live_support_tu15_generated.cpp'
FAMILY=('bfb90','b8780','b87b0')
PINS={name:PARENT_PINS[name]for name in (*FAMILY,'141010','1e3150','1e5c80')}
PINS.update(bf9a0='5ed92d5c400d1d91d15ec3a2dc112bc112cc44e823511a21bdea8941b2f47e58',
    **{'10e100':'f0710916f8d2e4cd5c72f043b512ba14d90bbdfbb7f90b207a446707cb87e469'})
BOUNDARIES=('1d5840','1e1720','1e5af0','1e3270','126200','1ce810','1cf210','1d36c0','bcb70')
REGIONS=tuple((base,4096)for base in (0x46a000,0x474000,0x4ec000,0x4f0000,0x51d000))+((0x61000000,65536),(0x62000000,16384))
ARGUMENTS={0x1e3150:1,0x1e5c80:1,0x1d5840:7,0xbf9a0:6,0x10e100:3,0x1e1720:2}
def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()

def recover(scratch):
    source=(ROOT/SOURCE).read_text();bodies={match[1]:body for match,body in function_bodies(source)if match[1]in PINS}
    for name in FAMILY:bodies[name]=object_node_body(name,bodies[name])
    bodies['1e3150']=lookup_body(bodies['1e3150']);bodies['1e5c80']=lifecycle_body(bodies['1e5c80'])
    for name,pin in PINS.items():
        if hashlib.sha256(bodies[name].encode()).hexdigest()!=pin:raise ValueError('Complete animation reference owner drift '+name)
    text=''.join(f'static void run_{name}(WorldState&);\n'for name in (*PINS,*BOUNDARIES))+'static void dispatch(WorldState&,std::uint32_t,std::uint32_t,bool);\n'
    text+='std::uint32_t sign_extend(std::uint32_t v,unsigned n){return n==1?std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(v)))):std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(v))));}\n'
    for name,body in bodies.items():
        if name not in FAMILY:
            at=body.index('{\n')+2;body=body[:at]+f'note(s,0x{name}u);\n'+body[at:]
        text+=body+'\n'
    for name in BOUNDARIES:text+=f'static void run_{name}(WorldState&s){{note(s,0x{name}u);throw Boundary{{0x{name}u}};}}\n'
    text+='static void dispatch(WorldState&s,std::uint32_t owner,std::uint32_t,bool){switch(owner){'+''.join(f'case 0x{name}u:run_{name}(s);return;'for name in (*PINS,*BOUNDARIES))+'default:note(s,owner);throw Boundary{owner};}}\n'
    (scratch/'object_animation_delivery_reference.inc').write_text(text)
    return dict(whole_canonical_body_sha256=PINS,genuine_residual_entries=list(BOUNDARIES),source_prefixes_stripped=['1e3150','1e5c80'],supplied_child_outcomes=False)

def hardware(args,scratch,binary):
    from validate_native_tu15_prt_helpers import physical_original,PIN
    from research_mp_sandbox_update_functions import Xbe
    image=Xbe(args.xbe)
    if digest(args.xbe)!=PIN:raise ValueError('Wrong original TU1.5 image')
    original=subprocess.run([str(binary),'--emit-fixtures'],capture_output=True)
    if original.returncode:raise RuntimeError('Animation genuine-input construction failed: '+original.stderr.decode()[-5000:])
    raw=original.stdout;at=0;rows=[];requests=[];names=[]
    def words(count):
        nonlocal at
        result=struct.unpack_from('<'+'I'*count,raw,at);at+=count*4;return result
    for n in range(words(1)[0]):
        owner,mode,df,stop,boundary,pc=words(6);abi=words(44);live=[]
        for _ in range(words(1)[0]):live.append(raw[at:at+10].hex());at+=10
        banks=[]
        for _,size in REGIONS:banks.append(raw[at:at+size]);at+=size
        def get(address,width=4):
            for (base,size),bank in zip(REGIONS,banks):
                if base<=address and address+width<=base+size:return int.from_bytes(bank[address-base:address-base+width],'little')
            raise ValueError('Animation argument escaped backed caller bank')
        sp=abi[4];rows.append(dict(id=n,name=f'{owner:x}-{mode}-DF{df}',entry=owner,gp=list(abi[:8]),eflags=abi[8],matrix=get(sp+4),source=get(sp+8),
            before_fp=[abi[11],abi[10],abi[9]],live_x87=live,xmm=[list(abi[12+k*4:16+k*4])for k in range(8)],max_steps=10000,
            stop_before_entries=[int(name,16)for name in BOUNDARIES]+([stop]if stop else[]),banks=[dict(address=base,before=bank.hex())for (base,_),bank in zip(REGIONS,banks)]))
        requests.append((owner,stop,boundary,pc,abi,live,banks))
    if at!=len(raw):raise ValueError('Animation input protocol drift')
    entries=tuple(int(name,16)for name in PINS)
    observed,proof=physical_original(image,rows,scratch,entries=entries,regions=REGIONS,
        trace_calls=[dict(entry=entry,arguments=ARGUMENTS.get(entry,0),gp=True,xmm=True)for entry in entries if f'{entry:x}'not in FAMILY],trace_extended=True)
    inputs,expected,actual=(scratch/name for name in ('animation.in','animation.expected','animation.actual'));segments=[];packets=0;stops=0;returns=0
    def emit_abi(out,gp,flags,fp,xmm):
        out.write(struct.pack('<12I',*gp,flags,fp[2],fp[1],fp[0]))
        for lane in xmm:out.write(struct.pack('<4I',*lane))
    with inputs.open('wb')as inp,expected.open('wb')as out:
        for case,request in zip(observed['cases'],requests,strict=True):
            owner,stop,boundary,pc,abi,live,banks=request;name=rows[case['id']]['name']
            if not case['completed']or not case['live_x87_unchanged']or case['stopped_at']!=(boundary or 0x63000000):raise ValueError('Animation original return/owner boundary mismatch '+name)
            inp.write(struct.pack('<2I44I',owner,stop,*abi));inp.write(struct.pack('<I',len(live)))
            for value in live:inp.write(bytes.fromhex(value))
            inp.write(b''.join(banks));segments.append((out.tell(),name,'result'))
            emit_abi(out,case['boundary_gp'],case['boundary_eflags'],case['after_fp'],case['boundary_xmm'])
            out.write(b''.join(bytes.fromhex(bank['after'])for bank in case['banks']));out.write(struct.pack('<I',len(case['writes'])))
            for event in case['writes']:
                data=bytes.fromhex(event['written']);out.write(struct.pack('<III',event['instruction'],event['address'],len(data)));out.write(bytes.fromhex(event['before']));out.write(data)
            calls=case['calls']
            if boundary:
                stops+=1
                def get_after(address):
                    for bank in case['banks']:
                        data=bytes.fromhex(bank['after']);base=bank['address']
                        if base<=address and address+4<=base+len(data):return int.from_bytes(data[address-base:address-base+4],'little')
                    raise ValueError('Animation genuine child argument not backed')
                calls=[*calls,dict(entry=boundary,gp=case['boundary_gp'],eflags=case['boundary_eflags'],fp=case['after_fp'],xmm=case['boundary_xmm'],live_x87=live,
                    arguments=[get_after(case['boundary_gp'][4]+4+k*4)for k in range(ARGUMENTS.get(boundary,0))])]
            else:returns+=1
            out.write(struct.pack('<3I',pc,boundary,len(calls)))
            for call in calls:
                segments.append((out.tell(),name,f"callee_{call['entry']:x}"));out.write(struct.pack('<I',call['entry']));emit_abi(out,call['gp'],call['eflags'],call['fp'],call['xmm'])
                out.write(struct.pack('<I',len(call['live_x87'])))
                for value in call['live_x87']:out.write(bytes.fromhex(value))
                out.write(struct.pack('<I',len(call['arguments'])*4))
                for value in call['arguments']:out.write(struct.pack('<I',value))
                packets+=1
    with inputs.open('rb')as inp,actual.open('wb')as out:result=subprocess.run([str(binary),'--physical'],stdin=inp,stdout=out,stderr=subprocess.PIPE)
    if result.returncode:raise RuntimeError('Native animation proof failed: '+result.stderr.decode()[-5000:])
    if digest(expected)!=digest(actual):
        a,b=actual.read_bytes(),expected.read_bytes();first=next((i for i,(x,y)in enumerate(zip(a,b))if x!=y),min(len(a),len(b)));segment=next(s for s in reversed(segments)if s[0]<=first)
        raise ValueError(f'Animation hardware mismatch byte{first},segment{segment},relative{first-segment[0]}: {a[first:first+32].hex()} vs{b[first:first+32].hex()}')
    executed={pc for case in observed['cases']for pc in case['executed_sites']};source=(ROOT/SOURCE).read_text();covered={}
    for match,body in function_bodies(source):
        if match[1]not in FAMILY:continue
        sites={int(pc,16)for pc in re.findall(r'services.instruction\((0x[0-9a-f]+)u\)',body)}
        covered[match[1]]=dict(total=len(sites),executed=len(sites&executed),unexecuted=[f'{pc:x}'for pc in sorted(sites-executed)])
    return dict(passed=True,cases=len(rows),normal_returns=returns,genuine_child_entry_stops=stops,full_callee_packets=packets,family_site_coverage=covered,
        full_result_gp_flags_xmm_fp_live80_banks_ordered_stores_and_callee_packets=True,native_family_instruction_callbacks=0,
        comparison_bytes=actual.stat().st_size,comparison_sha256=digest(actual),original_xbe_sha256=PIN,physical_original=proof,
        supplied_child_outcomes=False,physical_pre_io_and_page_fault_atomicity_verified=False)

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--xbe',type=Path,required=True);parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--compiler',default='c++');parser.add_argument('--sanitizers',action='store_true');parser.add_argument('--require-binding',action='store_true');args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Run through tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);paths=(SOURCE,'include/halo2/native_tu15_object_animation_delivery_kernel.hpp','src/physics/runtime/native_tu15_object_animation_delivery_abi.hpp','src/physics/runtime/native_tu15_object_node_admission.hpp',
        'tests/native_tu15_object_animation_delivery_tests.cpp','tests/native_tu15_object_node_admission_checks.hpp','tools/native_object_node_validation_support.py','tools/validate_native_object_node_rebuild_tu15.py','tools/native_node_descriptor_lookup_replacements.py','tools/native_node_resource_lifecycle_replacements.py',
        'tools/native_object_node_rebuild_replacements.py','src/campaign/ai/source_ai_frame_tu15_generated.cpp','src/physics/runtime/physics_world_runtime_internal.hpp','src/physics/runtime/native_tu15_matrix_leaves.hpp','tools/validate_native_tu15_prt_helpers.py',
        'tools/oracle_native_campaign_screen_lifecycle_tu15.py','tools/observe_native_campaign_ai_vector_math_tu15.cpp',str(Path(__file__).relative_to(ROOT)))
    pins={name:digest(ROOT/name)for name in paths};report=dict(passed=False,production_bound=args.require_binding,source_sha256=pins,scratch_owner=str(scratch),retained_large_artifacts=[])
    try:
        report['canonical_reference']=recover(scratch);binary=scratch/'object-animation-delivery'
        if args.require_binding:
            from native_object_node_rebuild_replacements import installed_entries
            report['binding']=[dict(source=path,entries=installed_entries((ROOT/path).read_text()))for path in (SOURCE,'src/campaign/ai/source_ai_frame_tu15_generated.cpp')]
            if any(set(copy['entries'])!={'bf0c0',*FAMILY}for copy in report['binding']):raise ValueError('Complete native animation source copies missing')
        command=[args.compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),
            str(ROOT/'tests/native_tu15_object_animation_delivery_tests.cpp'),'-o',str(binary)]
        if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        report['compiler_command']=command;subprocess.run(command,check=True);report['admission']=admission(binary,'HALO2_REFERENCE_OBJECT_ANIMATION');report['hardware']=hardware(args,scratch,binary)
        report['source_hashes_unchanged']=pins=={name:digest(ROOT/name)for name in paths}
        if not report['source_hashes_unchanged']:raise ValueError('Animation proof inputs changed during validation')
        report.update(passed=True,normal_and_declared_real_owner_hardware_verified=True,retained_fault_protocol_verified=False,production_child_closure_verified=False)
    except Exception as error:report['error']=str(error);raise
    finally:
        args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(report['hardware']['family_site_coverage']));return 0
if __name__=='__main__':raise SystemExit(main())
