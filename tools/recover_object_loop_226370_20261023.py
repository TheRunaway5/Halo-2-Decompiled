#!/usr/bin/env python3
"""Recover one reached original callee without rewriting other CPU bodies.

Run through tools/run_test.py. Production source changes only after the pinned
XBE lowerer succeeds and every pre-existing function except the one call site
is byte-identical. Hardware services remain required when reached.
"""
import difflib,hashlib,json,os,re,subprocess,sys
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'analysis/combat-presentation-audit-20261022/repair-scenery-audio'
REL='src/audio/source_object_loop_audio_tu15_generated.cpp'
def sha(data):return hashlib.sha256(data).hexdigest()

def main():
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    pins=json.loads((OUT.parent/'repair-followup-20261023/source-hashes.json').read_text())
    destination=ROOT/REL;before=destination.read_bytes()
    assert sha(before)==pins[REL],'Root object-loop owner changed outside this recovery'
    generated=scratch/'object-loop.cpp'
    report=OUT/'recovery-226370-root.json'
    interpreter=ROOT/'.tools/python/bin/python'
    command=[str(interpreter if interpreter.is_file() else sys.executable),str(ROOT/'tools/generate_object_loop_audio_tu15.py'),
        '--output',str(generated),'--directory',str(scratch/'recovery'),'--report-output',str(report)]
    result=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
    if result.returncode:
        (OUT/'recovery-226370-error.log').write_text(result.stdout[-16384:]);return result.returncode
    after=generated.read_bytes()
    pattern=r'(static void run_([0-9a-f]+)\(WorldState &s\) \{\n.*?\n\}\n)'
    bodies=lambda data:{name:body for body,name in re.findall(pattern,data.decode(),re.S)}
    old=bodies(before);new=bodies(after)
    assert set(new)==set(old)|{'226370'}
    changed=[name for name in old if old[name]!=new[name]]
    assert changed==['22c560'],changed
    old_call='{const auto target=0x226370u;s.push(0x22c7d3u,0x22c7d8u);dispatch(s,target,0x22c7d3u,false);}'
    new_call='s.push(0x22c7d3u,0x22c7d8u);run_226370(s);'
    assert old['22c560'].count(old_call)==1
    assert old['22c560'].replace(old_call,new_call)==new['22c560']
    patch=''.join(difflib.unified_diff(before.decode().splitlines(True),after.decode().splitlines(True),
        fromfile='a/'+REL,tofile='b/'+REL))
    (OUT/'original-callee-226370.patch').write_text(patch)
    assert destination.read_bytes()==before
    destination.write_bytes(after)
    receipt=dict(path=REL,before_sha256=sha(before),after_sha256=sha(after),
        generator_report=str(report),generator_report_sha256=sha(report.read_bytes()),
        original_body_added='00226370',existing_body_count=len(old),
        existing_body_changes=['0022c560: unchanged caller now reaches its complete original callee'],
        other_existing_bodies_identical=True,device_or_voice_results_supplied=False,
        production_runtime_installed=False,generated_files_scratch=str(scratch))
    (OUT/'original-callee-226370-source-change.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(receipt),flush=True);return 0

if __name__=='__main__':raise SystemExit(main())
