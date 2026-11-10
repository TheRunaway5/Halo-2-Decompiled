#!/usr/bin/env python3
"""Private authenticated source-copy/default-route qualification for30D1A0."""
import argparse,hashlib,json,os,re,subprocess
from pathlib import Path
import native_simplex_face_refine_replacements as binding
import specialize_native_operands as operand_spec
from validate_native_simplex_face_refine_tu15 import ROOT,PINS,reference
from validate_native_simplex_direction_tu15 import sha

def prepare(scratch,require_binding=False):
    blocks=[];receipts=[];sources=[];generation=[]
    for owner,path in zip(('primary','death'),PINS,strict=True):sources.append((owner,(ROOT/path).read_text(),require_binding))
    for owner,script,flag,output_directory in [('primary-fresh','generate_physics_live_support_tu15.py','--manifest','primary.json'),('death-fresh','generate_death_cycle_runtime_tu15.py','--directory','death-generation')]:
        output=scratch/(owner+'.cpp');command=[str(ROOT/'.tools/python/bin/python3'),'-B',str(ROOT/'tools'/script),'--output',str(output),flag,str(scratch/output_directory)];result=subprocess.run(command,capture_output=True,text=True,timeout=180)
        if result.returncode:raise ValueError('Fresh full face-refine source generation failed: '+owner+'\n'+result.stderr[-7000:])
        sources.append((owner,output.read_text(),require_binding));generation.append(dict(owner=owner,command=command,whole_sha256=sha(output)))
    for owner,original,require_installed in sources:
        source=binding.replace_simplex_face_refine(original)
        if require_installed and source!=original:raise ValueError('Production face-refine overlay not installed: '+owner)
        if binding.remove_simplex_face_refine(source)!=binding.remove_simplex_face_refine(original):raise ValueError('Face-refine source overlay not reversible')
        for value in(source,operand_spec.specialize(source)[0]):
            variant=len(blocks);binding.installed_entries(value);match,resolver,dispatcher=binding.context(value)
            body=match[0];at=body.index('{')+1+len(binding.PREFIX);body=body[:at]+f'\n++literal_entries[{variant}];'+body[at:];body=body.replace('run_30d1a0(',f'bound_{variant}(',1)
            changes={'body':value[:match.start()]+match[0].replace('0x14u','0x10u',1)+value[match.end():],'include':value.replace(binding.INCLUDE,'',1),'prefix':value.replace(binding.PREFIX,'',1),'missing_body':value[:match.start()]+value[match.end():],'resolver':value.replace(resolver,resolver.replace('0x46a0e8u','0x46a0ecu',1),1),'dispatcher':value.replace(dispatcher,dispatcher.replace('bool tail){','bool tail){if(false)return;',1),1)}
            controls={}
            for key,changed in changes.items():
                if changed==value:raise ValueError('Face-refine context mutation seam drift: '+key)
                try:binding.installed_entries(changed)
                except ValueError as error:controls[key]=str(error)
                else:raise ValueError('Face-refine context drift accepted: '+key)
            if variant%2==0 and binding.replace_simplex_face_refine(value)!=value:raise ValueError('Face-refine overlay not idempotent')
            if operand_spec.specialize(value)[0]!=operand_spec.specialize(source)[0]:raise ValueError('Face-refine preparation not idempotent')
            blocks.append(body);receipts.append(dict(variant=variant,owner=owner,prepared=bool(variant%2),complete_input_sha256=binding.digest(value),body_before_test_counter_sha256=binding.digest(match[0]),strict_context_controls=controls))
    reference(scratch)
    prefix='''static std::array<unsigned,8>literal_entries{};
static std::uint32_t selected(std::uint32_t value,SourcePhysicsContactImage image){if(image!=SourcePhysicsContactImage::TitleUpdate15)throw std::runtime_error("Wrong private face-refine source image");return value;}
static std::uint32_t sign_extend(std::uint32_t value,unsigned width){return reference::sign_extend(value,width);}
'''
    text=prefix+'\n'.join(blocks)+'\nstatic void run(WorldState&s,unsigned variant){switch(variant){case 0:bound_0(s);return;case 1:bound_1(s);return;case 2:bound_2(s);return;case 3:bound_3(s);return;case 4:bound_4(s);return;case 5:bound_5(s);return;case 6:bound_6(s);return;case 7:bound_7(s);return;default:throw std::runtime_error("Unknown private face-refine source variant");}}\n';(scratch/'simplex_face_refine_routes.inc').write_text(text)
    return dict(source_variants=receipts,full_generators=generation,test_instrumentation='One C++ literal-body-entry counter after each authenticated prefix; original i386 text is unmodified',source_copy_sha256=hashlib.sha256(text.encode()).hexdigest(),production_bound=require_binding)

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--compiler',default='c++');p.add_argument('--sanitizers',action='store_true');p.add_argument('--require-binding',action='store_true');args=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,production_bound=False,scratch_owner=str(scratch),retained_large_artifacts=[])
    paths=[*PINS,'src/physics/runtime/native_tu15_simplex_face_refine_source_owner.hpp','tools/native_simplex_face_refine_replacements.py','tests/native_tu15_simplex_face_refine_routes_tests.cpp','tests/native_tu15_simplex_face_refine_tests.cpp','tools/specialize_native_operands.py','tools/generate_physics_live_support_tu15.py','tools/generate_death_cycle_runtime_tu15.py','tools/validate_native_simplex_face_refine_tu15.py',str(Path(__file__).relative_to(ROOT))];pending=list(paths);seen=set(paths)
    while pending:
        current=ROOT/pending.pop()
        for header in re.findall(r'^#include "([^"\n]+)"',current.read_text(),re.M):
            target=next((v for v in(current.parent/header,ROOT/'include'/header,ROOT/'src'/header)if v.is_file()),None)
            if target is None:continue
            relative=str(target.relative_to(ROOT))
            if relative not in seen:seen.add(relative);pending.append(relative)
    pins={path:sha(ROOT/path)for path in sorted(seen)}
    try:
        report['preparation']=prepare(scratch,args.require_binding);binary=scratch/'face-refine-routes';command=[args.compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_simplex_face_refine_routes_tests.cpp'),'-o',str(binary)]
        if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        report['command']=command;result=subprocess.run(command,capture_output=True,text=True)
        if result.returncode:raise ValueError('Face-refine routes compile failed:\n'+result.stderr[-10000:])
        environment=os.environ.copy();environment.pop('HALO2_REFERENCE_SIMPLEX_FACE_REFINE',None);report['routes']={}
        for mode in ('default','reference','unrelated-controls'):
            env=environment.copy()
            if mode=='reference':env['HALO2_REFERENCE_SIMPLEX_FACE_REFINE']='1'
            if mode=='unrelated-controls':
                for name in ('MATRIX_LEAVES','SIMPLEX_TRIANGLE','POLYGON_SUPPORT','POLYGON_FEATURES','RAW_ALLOCATOR'):env['HALO2_REFERENCE_'+name]='1'
            probe=subprocess.run([str(binary)],capture_output=True,text=True,env=env)
            if probe.returncode:raise ValueError('Face-refine source routing failed '+mode+':\n'+probe.stderr[-6000:])
            report['routes'][mode]=json.loads(probe.stderr.strip().splitlines()[-1])
        probe=subprocess.run([str(binary),'--refusals'],capture_output=True,text=True,env=environment)
        if probe.returncode:raise ValueError('Face-refine admission failed:\n'+probe.stderr[-6000:])
        report['refusals']=json.loads(probe.stderr.strip().splitlines()[-1]);report['source_sha256']=pins;report['sources_unchanged']=all(sha(ROOT/path)==pin for path,pin in pins.items())
        if not report['sources_unchanged']:raise ValueError('Face-refine routing input drift')
        report['production_bound']=args.require_binding;report['passed']=True
    except Exception as error:report['error']=str(error)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in('passed','production_bound','error')if k in report}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
