#!/usr/bin/env python3
"""Owned native actor-query build and semantic mutation controls.

Run through tools/run_test.py. The official CMake worker builds the original
native closure once; each changed query object is privately compiled/linked
inside that same scratch owner. Production source is never changed.
"""
import argparse,gzip,hashlib,json,os,shlex,subprocess,sys
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
SOURCE=ROOT/'src/campaign/ai/native_campaign_ai_actor_path_query_tu15.cpp'
TARGET='native_campaign_ai_area_perform_tu15_tests'
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report',type=Path,required=True);args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);build=scratch/'build'
    report=dict(passed=False,scratch=str(scratch),controls=[],retained_large_artifacts=[])
    args.report.parent.mkdir(parents=True,exist_ok=True)
    def save():args.report.write_text(json.dumps(report,indent=2)+'\n')
    source=SOURCE.read_text();report['source_sha256']=sha(SOURCE)
    try:
        command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--target',TARGET,
            '--filter','^native_campaign_ai_(area_perform|character_object|actor_path_query|object_navigation)_tu15$',
            '--cmake-arg=-DCMAKE_EXPORT_COMPILE_COMMANDS=ON','--output',str(args.report.parent/'native'),'--jobs','4']
        report['build_command']=command;save()
        result=subprocess.run(command,cwd=ROOT);report['native_exit_code']=result.returncode;save()
        if result.returncode:return result.returncode
        commands=json.loads((build/'compile_commands.json').read_text())
        unit=next(c for c in commands if Path(c['file'])==SOURCE and f'{TARGET}.dir' in c['command'])
        compile_command=shlex.split(unit['command']);original_object=compile_command[compile_command.index('-o')+1]
        link=shlex.split((build/f'CMakeFiles/{TARGET}.dir/link.txt').read_text())
        fixtures={}
        for key,name in (('query','actor_path_query'),('caller','area_perform')):
            path=ROOT/f'tests/fixtures/native_campaign_ai_{name}_tu15.json.gz'
            report.setdefault('fixture_sha256',{})[str(path.relative_to(ROOT))]=sha(path)
            fixtures[key]=scratch/f'{key}.json';fixtures[key].write_bytes(gzip.decompress(path.read_bytes()))
        controls=(
            ('actor_stride','*0x888u','*0x880u','query'),
            ('cache_guard','if(!(abi.gp[0]&255u))','if(abi.gp[0]&255u)','query'),
            ('cache_frame_width','p.w16(0x275eeau','p.w32(0x275eeau','query'),
            ('query_clear_length','n<20;++n','n<19;++n','query'),
            ('radius_comparison','if(Simd(fp).above','if(!Simd(fp).above','query'),
            ('signed_action_boundary','sx16(p.r16(row+0x86u))<3','sx16(p.r16(row+0x86u))<=3','query'),
            ('flying_frame_width','p.w16(0x2014a2u','p.w32(0x2014a2u','query'),
            ('flying_valid_flag','p.w8(0x2014b3u,out+0x10u,1)','p.w8(0x2014b3u,out+0x10u,0)','query'),
            ('unit_eligibility','if(abi.gp[0]&255u) {\n                        push','if(!(abi.gp[0]&255u)) {\n                        push','query'),
            ('caller_query_pointer','child.gp[3]=out;','child.gp[3]=out+4u;','caller'),
            ('caller_owner_binding','const auto result=native_campaign_ai_evaluate_goals_tu15',
             'context.path_query={};const auto result=native_campaign_ai_evaluate_goals_tu15','caller'))
        for name,before,after,key in controls:
            assert before in source,name
            changed=source.replace(before,after,1);cpp=scratch/(name+'.cpp');obj=scratch/(name+'.o');exe=scratch/name
            cpp.write_text(changed)
            compile_args=[str(cpp) if arg==str(SOURCE) else arg for arg in compile_command]
            compile_args[compile_args.index('-o')+1]=str(obj)
            compile_args+=['-I',str(SOURCE.parent)]
            linked=[str(obj) if arg==original_object else arg for arg in link]
            linked[linked.index('-o')+1]=str(exe)
            for args_ in (compile_args,linked):
                result=subprocess.run(args_,cwd=build,text=True,capture_output=True,timeout=180)
                assert result.returncode==0,result.stderr[-4000:]
            result=subprocess.run([str(exe),str(fixtures[key])],cwd=ROOT,text=True,capture_output=True,timeout=30)
            row=dict(name=name,rejected=result.returncode!=0,exit_code=result.returncode,
                     source_sha256=sha(cpp),diagnostic=(result.stdout+result.stderr)[-2000:])
            report['controls'].append(row);save();print(name,'rejected' if row['rejected'] else 'SURVIVED',flush=True)
            for path in (cpp,obj,exe):path.unlink()
        report['source_unchanged']=sha(SOURCE)==report['source_sha256']
        report['passed']=report['source_unchanged'] and all(r['rejected'] for r in report['controls'])
        return 0 if report['passed'] else 1
    finally:save()

if __name__=='__main__':raise SystemExit(main())
