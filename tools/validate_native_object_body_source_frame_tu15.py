#!/usr/bin/env python3
"""Private exact retained-owner continuation at salted lookup provider frontiers."""
import argparse,hashlib,json,os,platform,re,subprocess
from pathlib import Path
from specialize_native_operands import specialize
from validate_native_object_body_frame_tu15 import corpus
ROOT=Path(__file__).resolve().parents[1]
UNIT='src/gameplay/units/source_unit_view_reset_tu15_generated.cpp'
BODY_PIN='8fc06927ea71a7373c2f01b38037dfd22c5211a7e11976f9d386000de9ed9dfb'
FRONTIERS=(0xbcdf1,0xbcdf2,0xbce01,0xbce04,0xbce13,0xbce19,0xbce1c,
           0xbce36,0xbce41,0xbce46,0xbce47,0xbce48)
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def prepare(scratch):
    import native_object_body_replacements as binding
    current=binding.remove_object_body((ROOT/UNIT).read_text());pattern=re.compile(r'static void run_bcdf0\(WorldState &s\) \{.*?\n\}',re.S)
    raw=pattern.findall(current)
    if len(raw)!=1 or hashlib.sha256(raw[0].encode()).hexdigest()!=BODY_PIN:
        raise ValueError('Complete actual unit-view lookup owner drift')
    prepared,receipt=specialize(current);bodies=pattern.findall(prepared)
    if len(bodies)!=1:raise ValueError('Prepared actual lookup missing/ambiguous')
    body=bodies[0];start=body.index('{')+1
    switch='\nswitch(resume){case 0u:break;'+''.join(f'case 0x{pc:x}u:goto N{pc:x};'for pc in FRONTIERS)+'default:throw std::runtime_error("Invalid private object lookup continuation");}\n'
    for pc in FRONTIERS:
        needle=f's.instruction_literal(0x{pc:x}u);'
        if body.count(needle)!=1:raise ValueError('Prepared source frontier missing '+hex(pc))
        body=body.replace(needle,f'N{pc:x}: '+needle,1)
    body=re.sub(r's\.instruction_literal\((0x[0-9a-f]+u)\);',r'coverage.insert(\1);s.instruction_literal(\1);',body)
    body=body[:start]+switch+body[start:]
    body=body.replace('static void run_bcdf0(WorldState &s)','static void retained(WorldState &s,std::uint32_t resume)',1)
    helper=re.findall(r'^std::uint32_t sign_extend\([^\n]+\{.*?^\}',current,re.M|re.S)
    if len(helper)!=1:raise ValueError('Complete source sign-extension helper missing')
    (scratch/'object_body_source_reference.inc').write_text('static std::set<std::uint32_t>coverage;\n'+helper[0]+'\n'+body+'\n')
    return dict(current_body_sha256=BODY_PIN,prepared_body_sha256=hashlib.sha256(bodies[0].encode()).hexdigest(),
        continuation_sites=[f'{pc:x}'for pc in FRONTIERS],fixture_sha256=hashlib.sha256(body.encode()).hexdigest(),
        retained_semantic_operations_changed=False,preparation_receipt=receipt.get('source_sha256')if isinstance(receipt,dict)else None)
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--compiler',action='append');p.add_argument('--sanitizers',action='store_true')
    p.add_argument('--semantic-mutants',action='store_true')
    args=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    if platform.system()!='Linux' or platform.machine().lower()not in('x86_64','amd64','i386','i686'):return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);args.output.mkdir(parents=True,exist_ok=False)
    names=[UNIT,'src/gameplay/objects/native_tu15_object_body_source_frame.hpp',
        'src/gameplay/objects/native_tu15_object_body_frame.hpp','include/halo2/native_tu15_object_body_kernel.hpp',
        'tests/native_tu15_object_body_source_frame_tests.cpp','tools/validate_native_object_body_source_frame_tu15.py',
        'tests/native_tu15_mopp_ray_frame_test_memory.hpp','tools/specialize_native_operands.py',
        'tools/native_object_body_replacements.py']
    pins={name:sha(ROOT/name)for name in names};report=dict(passed=False,production_bound=False,
        source_before=pins,owned_scratch=str(scratch),retained_large_artifacts=[])
    try:
        report['source_qualification']=prepare(scratch)
        fixture=scratch/'inputs.json';fixture.write_text(json.dumps(corpus(),separators=(',',':')))
        results=[]
        for compiler in args.compiler or('g++','clang++'):
            binary=scratch/Path(compiler).name
            command=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
                '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'tests'),'-I'+str(scratch),
                str(ROOT/'tests/native_tu15_object_body_source_frame_tests.cpp'),'-o',str(binary)]
            if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
            done=subprocess.run(command,capture_output=True,text=True)
            if done.returncode:raise ValueError('Source lookup frame compile failed: '+done.stderr[-7000:])
            done=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True)
            if done.returncode:raise ValueError('Source lookup frame comparison failed: '+(done.stdout+done.stderr)[-10000:])
            mutations=[]
            if args.semantic_mutants:
                relative='src/gameplay/objects/native_tu15_object_body_source_frame.hpp';source=(ROOT/relative).read_text()
                definitions=(
                    ('ignore-returned-policy','if(!continue_(s_))throw SourceContinuation{sites_(next),owner_};','if(false)throw SourceContinuation{sites_(next),owner_};'),
                    ('late-push-esp','at(pc);s_.gp[4]-=4u;store_source(pc,s_.gp[4],value);','at(pc);store_source(pc,s_.gp[4]-4u,value);s_.gp[4]-=4u;'),
                    ('wrong-product-overflow-flag','g[0]=s_.flags.product(s_.reg(0,4,0),s_.reg(1,4,0));','g[0]=s_.flags.product(s_.reg(0,4,0),s_.reg(1,4,0));s_.flags.of=!s_.flags.of;'),
                    ('cache-datum-across-provider','bool in_bounds(std::uint32_t,std::uint32_t){\n        auto&g=s_.gp;g[1]=g[6];','bool in_bounds(std::uint32_t datum,std::uint32_t){\n        auto&g=s_.gp;g[1]=datum;'),
                    ('replay-completed-rows-read','checkpoint(0xbce19u);return g[0];','checkpoint(0xbce16u);return g[0];'))
                for name,old,new in definitions:
                    if source.count(old)!=1:raise ValueError('Source lookup defect seam drift '+name)
                    overlay=scratch/(Path(compiler).name+'-'+name);target=overlay/relative
                    target.parent.mkdir(parents=True);target.write_text(source.replace(old,new,1))
                    changed=list(command);changed.insert(1,'-I'+str(overlay/'src'));changed[changed.index('-o')+1]=str(overlay/'bad')
                    built=subprocess.run(changed,capture_output=True,text=True)
                    if built.returncode:raise ValueError('Source lookup defect compile failed '+name+': '+built.stderr[-5000:])
                    bad=subprocess.run([str(overlay/'bad'),str(fixture)],capture_output=True,text=True)
                    diagnostic=(bad.stdout+bad.stderr)[-5000:]
                    if not bad.returncode or 'Object-body source continuation differs'not in diagnostic:
                        raise ValueError('Source lookup defect survived/unrecognized '+name+': '+diagnostic)
                    mutations.append(dict(name=name,rejected=True,diagnostic=diagnostic))
            results.append(dict(compiler=compiler,command=command,result=json.loads(done.stdout),compiled_mutations=mutations))
        report.update(passed=True,compilers=results)
    except BaseException as error:report['error']=repr(error);raise
    finally:
        report['source_after']={name:sha(ROOT/name)for name in names}
        if report['source_after']!=pins:report.update(passed=False,source_drift=True)
        (args.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    return 0
if __name__=='__main__':raise SystemExit(main())
