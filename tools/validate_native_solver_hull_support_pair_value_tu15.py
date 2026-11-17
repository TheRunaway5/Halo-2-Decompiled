#!/usr/bin/env python3
"""Private shared public313390 DATA compatibility; legacy helper remains untouched."""
import argparse,hashlib,json,os,platform,re,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
FUNCTION=re.compile(r'void native_tu15_solver_hull_support_pair\(.*?\n\}',re.S)
INCLUDE='#include "halo2/native_tu15_solver_hull_support_pair_value.hpp"\n'
OLD_PIN='5978c7ed212272757fd7157d1708aa57579bef134f7e85886064a6a495bdb476'
NEW_PIN='361ab84b1cdbfda01017c5bfc848616eceee77800b91a318dd4e7e1851f763a7'
REFERENCE_PIN='b017b72933d21652e4fb1f177754f09204a337fb38391d4ef48baf04adfa3ba7'
OLD='void native_tu15_solver_hull_support_pair(SourcePhysicsContactMemory&memory,\n    std::uint32_t context,std::uint32_t direction,std::uint32_t point_pair,\n    std::uint32_t transformed_direction,NativeTu15PhysicsFrameFp&fp,unsigned depth,\n    const NativeTu15SolverHullSupportServices&services,\n    const SourcePhysicsContactRecordObserver&observe,NativeTu15SolverHullSupportStats*stats){\n    if(stats)++stats->pair_calls;\n    try{\n#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)\n        using namespace joint_constraint_numeric;\n        if(depth)throw std::runtime_error("Native solver hull support requires qualified empty x87 DATA");\n        // Admit the native FP policy before any source DATA is accessed.\n        {Environment validate(fp,depth);}\n        Memory m(memory,observe);\n        const auto support=[&](std::uint32_t shape,std::uint32_t input,std::uint32_t output){\n            const auto table=m.word(shape),owner=m.word(table+0x30u);\n            if(stats)++stats->shape_support_calls;\n            if(!services.support)throw std::runtime_error("Native solver hull requires its actual shape-support service");\n            services.support({shape},{owner},input,output,fp);\n        };\n        support(m.word(context+0x40u),direction,point_pair);\n        std::uint32_t second_shape;\n        {\n            Environment phase(fp,depth);\n            const auto z=m.vector(context+0x20u),sign=m.vector(0x461960u),input=m.vector(direction);\n            const auto x=m.vector(context),y=m.vector(context+0x10u);second_shape=m.word(context+0x44u);\n            const auto negative=xor_bits(input,sign);\n            const auto xy0=_mm_shuffle_ps(x,y,0x44),xy1=_mm_shuffle_ps(x,y,0xee);\n            const auto zz0=_mm_shuffle_ps(z,z,0x44),zz1=_mm_shuffle_ps(z,z,0xee);\n            const auto ty=_mm_shuffle_ps(xy0,zz0,0xdd);const auto yterm=multiply(ty,lane<0x55u>(negative));\n            const auto tx=_mm_shuffle_ps(xy0,zz0,0x88);const auto xterm=multiply(tx,lane<0u>(negative));\n            const auto tz=_mm_shuffle_ps(xy1,zz1,0x88);const auto zterm=multiply(tz,lane<0xaau>(negative));\n            m.vector(0x31341du,transformed_direction,add(add(xterm,yterm),zterm));\n        }\n        support(second_shape,transformed_direction,point_pair+0x10u);\n        {\n            Environment phase(fp,depth);\n            const auto local=m.vector(point_pair+0x10u),translation=m.vector(context+0x30u),z=m.vector(context+0x20u);\n            const auto zterm=multiply(z,lane<0xaau>(local));const auto translated=add(zterm,translation);\n            const auto y=m.vector(context+0x10u);const auto yterm=multiply(y,lane<0x55u>(local));\n            const auto x=m.vector(context);const auto xterm=multiply(x,lane<0u>(local));\n            m.vector(0x313461u,point_pair+0x20u,add(add(xterm,yterm),translated));\n        }\n        if(stats)++stats->pair_completed;\n#else\n        (void)memory;(void)context;(void)direction;(void)point_pair;(void)transformed_direction;(void)fp;(void)depth;(void)services;(void)observe;\n        throw std::runtime_error("Exact native solver hull backend unavailable");\n#endif\n    }catch(...){if(stats)++stats->refusals;throw;}\n}'
NEW='void native_tu15_solver_hull_support_pair(SourcePhysicsContactMemory&memory,\n    std::uint32_t context,std::uint32_t direction,std::uint32_t point_pair,\n    std::uint32_t transformed_direction,NativeTu15PhysicsFrameFp&fp,unsigned depth,\n    const NativeTu15SolverHullSupportServices&services,\n    const SourcePhysicsContactRecordObserver&observe,NativeTu15SolverHullSupportStats*stats){\n    if(stats)++stats->pair_calls;\n    try{\n#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)\n        using namespace joint_constraint_numeric;\n        if(depth)throw std::runtime_error("Native solver hull support requires qualified empty x87 DATA");\n        // Preserve the existing value-only FP admission and two phase lifetimes.\n        {Environment validate(fp,depth);}\n        using View=native_solver_hull_support_pair_value_detail::View<Memory,Environment>;\n        View view{{memory,observe},fp,depth,services,stats,context,direction,point_pair,transformed_direction};\n        View::Data data;native_tu15_solver_hull_support_pair_kernel(view,data);\n        if(stats)++stats->pair_completed;\n#else\n        (void)memory;(void)context;(void)direction;(void)point_pair;(void)transformed_direction;(void)fp;(void)depth;(void)services;(void)observe;\n        throw std::runtime_error("Exact native solver hull backend unavailable");\n#endif\n    }catch(...){if(stats)++stats->refusals;throw;}\n}'
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def public_overlay(source):
    matches=list(FUNCTION.finditer(source))
    if len(matches)!=1:raise ValueError('Missing or ambiguous complete public support-pair function')
    m=matches[0];pin=hashlib.sha256(m[0].encode()).hexdigest()
    if pin==OLD_PIN and INCLUDE not in source:
        return INCLUDE+source[:m.start()]+NEW+source[m.end():]
    if pin==NEW_PIN and source.count(INCLUDE)==1:return source
    raise ValueError('Complete public313390 function/header drift')
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--xbe',type=Path,required=True);p.add_argument('--compiler',default='c++');p.add_argument('--sanitizers',action='store_true');p.add_argument('--mutations',action='store_true');p.add_argument('--require-binding',action='store_true');a=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    if platform.system()!='Linux'or platform.machine().lower()not in('x86_64','amd64','i386','i686')or not a.xbe.is_file():
        print('SKIP shared public pair requires Linux x86 and its authenticated original image');return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);names=['src/physics/native_tu15_solver_hull_support.cpp','src/physics/native_tu15_contact_shape.cpp','include/halo2/native_tu15_solver_hull_support_pair_value.hpp','tests/reference/native_tu15_solver_hull_support_pair_value_before.inc','tests/native_tu15_solver_hull_support_pair_value_tests.cpp','tools/validate_native_solver_hull_support_pair_frame_tu15.py','tools/native_solver_hull_support_pair_zero_inputs.py','tools/native_solver_hull_support_pair_shape_inputs.py',str(Path(__file__).relative_to(ROOT))];pending=list(names);seen=set(names)
    while pending:
        current=ROOT/pending.pop()
        for header in re.findall(r'^#include "([^"\n]+)"',current.read_text(),re.M):
            target=next((v for v in(current.parent/header,ROOT/'include'/header,ROOT/'src'/header,ROOT/'tests'/header)if v.is_file()),None)
            if target is None:continue
            relative=str(target.relative_to(ROOT))
            if relative not in seen:seen.add(relative);pending.append(relative)
    pins={name:sha(ROOT/name)for name in sorted(seen)};report=dict(passed=False,production_bound=False,scratch_owner=str(scratch),source_sha256=pins,retained_large_artifacts=[])
    try:
        from research_mp_sandbox_update_functions import Xbe
        from validate_native_tu15_prt_helpers import PIN
        from validate_native_solver_hull_support_pair_frame_tu15 import inputs
        from native_solver_hull_support_pair_zero_inputs import mixed_inputs
        from native_solver_hull_support_pair_shape_inputs import box_inputs
        if sha(a.xbe)!=PIN or sha(ROOT/'tests/reference/native_tu15_solver_hull_support_pair_value_before.inc')!=REFERENCE_PIN:raise ValueError('Public pair original/reference identity drift')
        image=Xbe(a.xbe);rows=box_inputs(image,mixed_inputs(image,inputs(image)))
        # This backed ordinary pair makes only the translation addition raise
        # invalid: first owner zero, second owner box with positive local Z.
        import copy,struct
        row=copy.deepcopy(rows[0]);row['id']=len(rows);row['name']='value-translation-before-Y-invalid';raw=bytearray.fromhex(row['banks'][0]['before']);base=row['banks'][0]['address'];context=row['gp'][1]
        def put(address,*values):struct.pack_into('<'+'I'*len(values),raw,address-base,*values)
        put(context,0x3f800000,0,0,0);put(context+16,0,0x3f800000,0,0);put(context+32,0,0,0x7f800000,0);put(context+48,0,0,0xff800000,0);put(row['matrix'],0,0,0xbf800000,0);put(0x61005000,0x4613a0);put(0x61006000,0x462090);put(0x61006010,0x3f800000,0x3f800000,0x3f800000,0);row['banks'][0]['before']=raw.hex();row['before_fp']=[0x37f,0x4100,0x1f80];rows.append(row)
        input_path=scratch/'inputs.json';input_path.write_text(json.dumps(rows,separators=(',',':')));report['input_sha256']=sha(input_path)
        original=(ROOT/'src/physics/native_tu15_solver_hull_support.cpp').read_text();candidate=public_overlay(original)
        if a.require_binding and candidate!=original:raise ValueError('Shared public support-pair DATA is not installed')
        source=scratch/'support-pair.cpp';source.write_text(candidate);report['public_candidate_sha256']=sha(source)
        command=[a.compiler,'-std=c++20','-Wno-ignored-attributes','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'src/physics'),'-I'+str(ROOT/'tests'),str(ROOT/'tests/native_tu15_solver_hull_support_pair_value_tests.cpp'),str(source),str(ROOT/'src/physics/native_tu15_contact_shape.cpp'),'-o',str(scratch/'value')]
        if a.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        report['command']=command;built=subprocess.run(command,capture_output=True,text=True)
        if built.returncode:raise ValueError('Shared public pair compile failed: '+built.stderr[-9000:])
        with input_path.open('rb')as inp:run=subprocess.run([str(scratch/'value')],stdin=inp,capture_output=True,text=True)
        if run.returncode:raise ValueError('Shared public pair differs: '+run.stderr[-9000:])
        report['value']=json.loads(run.stdout.strip().splitlines()[-1])
        if a.mutations:
            relative='halo2/native_tu15_solver_hull_support_pair_value.hpp';header=(ROOT/'include'/relative).read_text();report['semantic_mutants']={}
            for name,old,new in [('wrong_public_Y_schedule','translation_before_second_y=true','translation_before_second_y=false'),('wrong_public_result_destination','d.point_pair+0x20u,d.result','d.point_pair+0x10u,d.result')]:
                if header.count(old)!=1:raise ValueError('Public support-pair mutation seam changed '+name)
                overlay=scratch/name;target=overlay/relative;target.parent.mkdir(parents=True);target.write_text(header.replace(old,new,1));mutated=command.copy();mutated.insert(mutated.index('-I'+str(ROOT/'include')),'-I'+str(overlay));mutated[-1]=str(overlay/'value')
                built=subprocess.run(mutated,capture_output=True,text=True)
                if built.returncode:raise ValueError('Public support-pair mutant compile failed '+name+': '+built.stderr[-6000:])
                with input_path.open('rb')as inp:run=subprocess.run([str(overlay/'value')],stdin=inp,capture_output=True,text=True)
                if not run.returncode:raise ValueError('Public support-pair mutant survived '+name)
                report['semantic_mutants'][name]=dict(rejected=True,diagnostic=run.stderr[-7000:])
        report.update(passed=True,production_bound=a.require_binding)
    except Exception as error:report['error']=str(error)
    report['sources_unchanged']=all(sha(ROOT/name)==value for name,value in pins.items())
    if not report['sources_unchanged']:report.update(passed=False,error='Public support-pair input drift')
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in('passed','production_bound','error')if k in report}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
