#!/usr/bin/env python3
"""Private prepared-source type-query transport; no production binding.

Actual complete parent and E0900 callback bodies own the retained comparison.
The dispatch fixture admits only that genuine callback; no child result is
supplied. Raw callback formats and seven other children remain separate work.
"""
import argparse,hashlib,json,os,platform,re,shlex,subprocess
from pathlib import Path
from specialize_native_operands import specialize
from native_flight_property_query_replacements import canonical_body as canonical_flight
from native_object_type_query_replacements import canonical_body as canonical_type
import validate_native_object_type_query_frame_tu15 as physical
from oracle_smg_firing_1_1 import Xbe
ROOT=Path(__file__).resolve().parents[1]
UNIT='src/physics/physics_live_support_tu15_generated.cpp'
FRAME='src/gameplay/objects/native_tu15_object_type_query_source_frame.hpp'
TEST='tests/native_tu15_object_type_query_source_frame_tests.cpp'
PINS={0x10de40:'12cde1f032681088a6ab567a5f360e66e28bb509ae903b7c620886f8a1ef190d',
      0xe0900:'8439613527b310d55ffdb2a7ce3e6f7abc0379f0ff8a0ed945343a23662f44c7'}
FRONTIERS=(0x10de46,0x10de49,0x10de4d,0x10de51,0x10de5f,0x10de66,0x10de6d,0x10de70,
           0x10de74,0x10de76,0x10de82,0x10de89,0x10de91,0x10de92,0x10de93,0x10de94,
           0x10de95,0x10de97,0x10de9f,0x10dea0,0x10deae,0x10deb0,0x10deb1,0x10deb2)
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def bodies(source):
    result={}
    for entry,canonical in ((0x10de40,canonical_type),(0xe0900,canonical_flight)):
        found=re.findall(r'^static void run_'+format(entry,'x')+r'\(WorldState &s\) \{.*?^\}',source,re.M|re.S)
        if len(found)!=1:raise ValueError('Missing/ambiguous complete type-query subject '+hex(entry))
        result[entry]=canonical(found[0])
    return result
def prepare(scratch):
    current=(ROOT/UNIT).read_text()
    # Specialize the genuine whole module with sibling prefixes intact. Only
    # the two extracted reference bodies are reversed for this capsule.
    raw=bodies(current)
    if {entry:hashlib.sha256(body.encode()).hexdigest()for entry,body in raw.items()}!=PINS:raise ValueError('Complete retained type-query subject changed')
    prepared,_=specialize(current);compiled=bodies(prepared);body=compiled[0x10de40]
    for entry,count in((0x10de40,43),(0xe0900,76)):
        if len(re.findall(r's\.instruction_literal\(0x[0-9a-f]+u\);',compiled[entry]))!=count:raise ValueError('Prepared subject callback format changed')
    start=body.index('{')+1
    switch='\nswitch(resume){case 0u:break;'+''.join(f'case 0x{pc:x}u:goto N{pc:x};'for pc in FRONTIERS)+'default:throw std::runtime_error("Invalid private type-query continuation");}\n'
    for pc in FRONTIERS:
        needle=f's.instruction_literal(0x{pc:x}u);'
        if body.count(needle)!=1:raise ValueError('Missing exact type-query frontier '+hex(pc))
        body=body.replace(needle,f'N{pc:x}: '+needle,1)
    body=re.sub(r's\.instruction_literal\((0x[0-9a-f]+u)\);',r'coverage.insert(\1);s.instruction_literal(\1);',body)
    body=body[:start]+switch+body[start:]
    body=body.replace('static void run_10de40(WorldState &s)','static void retained(WorldState &s,std::uint32_t resume)',1)
    helper=re.findall(r'^std::uint32_t sign_extend\([^\n]+\{.*?^\}',current,re.M|re.S)
    if len(helper)!=1:raise ValueError('Complete source sign-extension helper missing')
    child=compiled[0xe0900]
    dispatcher='static void dispatch(WorldState&s,std::uint32_t target,std::uint32_t site,bool tail){if(target!=0xe0900u||site!=0x10de95u||tail)throw std::runtime_error("Undeclared type-query callback");run_e0900(s);}\n'
    (scratch/'object_type_query_source_reference.inc').write_text('static std::set<std::uint32_t>coverage;\n'+helper[0]+'\n'+child+'\n'+dispatcher+body+'\n')
    return dict(raw_body_sha256={hex(k):v for k,v in PINS.items()},prepared_body_sha256={hex(k):hashlib.sha256(v.encode()).hexdigest()for k,v in compiled.items()},continuation_sites=[f'{pc:x}'for pc in FRONTIERS],retained_semantic_operations_changed=False,source_dispatch_fixture='Only genuine E0900; other owners explicitly rejected, no supplied result')
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--xbe',type=Path,required=True)
    p.add_argument('--compiler',action='append');p.add_argument('--sanitizers',action='store_true');p.add_argument('--semantic-mutants',action='store_true');a=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Use tools/run_test.py')
    if platform.system()!='Linux' or platform.machine().lower()not in('x86_64','amd64','i386','i686')or not a.xbe.is_file():return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);a.output.mkdir(parents=True,exist_ok=False)
    names=[UNIT,FRAME,TEST,'include/halo2/native_tu15_object_type_query_kernel.hpp','src/gameplay/objects/native_tu15_object_type_query_frame.hpp',
           'tests/native_tu15_mopp_ray_frame_test_memory.hpp','tools/validate_native_object_type_query_source_frame_tu15.py',
           'tools/validate_native_object_type_query_frame_tu15.py','tools/specialize_native_operands.py',
           'tools/native_flight_property_query_replacements.py','tools/native_object_type_query_replacements.py']
    pins={name:sha(ROOT/name)for name in names};headers={path.resolve():sha(path)for tree in('include','src','tests')for path in(ROOT/tree).rglob('*')if path.suffix in('.hpp','.h','.inc','.cpp')}
    report=dict(passed=False,production_bound=False,scope=__doc__,source_before=pins,owned_scratch=str(scratch),retained_large_artifacts=[],source_contract='Prepared literals only; actual matching State/Memory providers, finite genuine E0900 paths')
    try:
        if sha(a.xbe)!=physical.PIN:raise ValueError('Wrong original TU1.5 reference input')
        report['source_qualification']=prepare(scratch)
        physical.REGIONS=tuple(sorted((*physical.REGIONS,(0x46a000,4096),(0x4f0000,4096))))
        fixture=scratch/'inputs.json';fixture.write_text(json.dumps(physical.corpus(Xbe(a.xbe),True),separators=(',',':')));results=[]
        report['fixture_sha256']=sha(fixture)
        report['immutable_banks_predecoded_once_per_row']=True
        for compiler in a.compiler or('g++','clang++'):
            binary=scratch/Path(compiler).name;dep=binary.with_suffix('.d')
            command=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'tests'),'-I'+str(scratch),str(ROOT/TEST),'-o',str(binary),'-MD','-MF',str(dep)]
            if a.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
            built=subprocess.run(command,capture_output=True,text=True)
            if built.returncode:raise ValueError('Type-query source compile failed '+built.stderr[:3500]+built.stderr[-3500:])
            for value in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
                path=Path(value).resolve()
                if path in headers:pins[str(path.relative_to(ROOT))]=headers[path]
            tested=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True)
            if tested.returncode:raise ValueError('Type-query source comparison failed '+(tested.stdout+tested.stderr)[-10000:])
            mutations=[]
            if a.semantic_mutants:
                source=(ROOT/FRAME).read_text()
                for label,old,new in(
                    ('ignore-policy','if(!continue_(s_))throw SourceContinuation{next,owner_};','if(false)throw SourceContinuation{next,owner_};'),
                    ('late-push-esp','at(pc);s_.gp[4]-=4u;store(pc,s_.gp[4],value);','at(pc);store(pc,s_.gp[4]-4u,value);s_.gp[4]-=4u;'),
                    ('replay-child','call_(s_,target,0x10de95u,false);checkpoint(0x10de97u);','call_(s_,target,0x10de95u,false);checkpoint(0x10de95u);'),
                    ('retroactive-observer','if(observed&&s_.observe)','if(s_.observe)'),
                    ('swallow-foreign','if(resume.owner!=&token)throw;','if(false)throw;')):
                    if source.count(old)!=1:raise ValueError('Missing source defect seam '+label)
                    overlay=scratch/(Path(compiler).name+'-'+label);target=overlay/FRAME;target.parent.mkdir(parents=True);target.write_text(source.replace(old,new,1));changed=list(command);changed.insert(1,'-I'+str(overlay/'src'));changed[changed.index('-o')+1]=str(overlay/'bad')
                    built=subprocess.run(changed,capture_output=True,text=True)
                    if built.returncode:raise ValueError('Source defect compile failed '+label+': '+built.stderr[-5000:])
                    tested_bad=subprocess.run([str(overlay/'bad'),str(fixture)],capture_output=True,text=True);diagnostic=(tested_bad.stdout+tested_bad.stderr)[-5000:]
                    if not tested_bad.returncode or 'Type-query source continuation differs'not in diagnostic:raise ValueError('Source defect survived/unrecognized '+label+': '+diagnostic)
                    mutations.append(dict(name=label,rejected=True,diagnostic=diagnostic))
            result=json.loads(tested.stdout)
            expected={'source_comparisons':33136,'actual_retained_continuations':8028,
                      'provider_observer_failure_prefixes':2740,'full_provider_observer_packets':1715382,
                      'positive_normal_returns':256,'retained_coverage_sites':43}
            if any(result.get(key)!=value for key,value in expected.items()):
                raise ValueError('Type-query seed-cache changed the qualified case/event coverage '+json.dumps(result))
            results.append(dict(compiler=compiler,command=command,result=result,compiled_mutations=mutations))
        report.update(passed=True,compilers=results)
    except BaseException as error:report['error']=repr(error);raise
    finally:
        report['source_after']={name:sha(ROOT/name)for name in pins}
        if report['source_after']!=pins:report.update(passed=False,source_drift=True)
        report['subject_after']={hex(entry):hashlib.sha256(body.encode()).hexdigest()for entry,body in bodies((ROOT/UNIT).read_text()).items()}
        if report['subject_after']!={hex(k):v for k,v in PINS.items()}:report.update(passed=False,subject_drift=True)
        (a.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
