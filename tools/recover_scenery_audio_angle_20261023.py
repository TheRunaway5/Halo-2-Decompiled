#!/usr/bin/env python3
"""Recover only the reached original18B580 callee and its two18F9B0 edges."""
import difflib,hashlib,json,os,re,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(data):return hashlib.sha256(data).hexdigest()
def main():
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    out=ROOT/'analysis/combat-presentation-audit-20261022/repair-scenery-audio/scenery-angle-recovery';out.mkdir(exist_ok=False)
    relative='src/audio/source_object_loop_audio_tu15_generated.cpp';destination=ROOT/relative;before=destination.read_bytes()
    generated=scratch/'object-loop-angle.cpp';interpreter=ROOT/'.tools/python/bin/python'
    command=[str(interpreter if interpreter.is_file() else sys.executable),str(ROOT/'tools/generate_object_loop_audio_tu15.py'),
        '--output',str(generated),'--directory',str(scratch/'angle-recovery'),'--report-output',str(out/'generation.json')]
    result=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
    if result.returncode:(out/'failure.log').write_text(result.stdout[-16384:]);return result.returncode
    after=generated.read_bytes();pattern=r'(static void run_([0-9a-f]+)\(WorldState &s\) \{\n.*?\n\}\n)'
    bodies=lambda data:{name:body for body,name in re.findall(pattern,data.decode(),re.S)}
    old=bodies(before);new=bodies(after);assert set(new)==set(old)|{'18b580'}
    changed=[name for name in old if old[name]!=new[name]];assert changed==['18f9b0'],changed
    expected=old['18f9b0']
    for site in (0x18fc46,0x18fc6a):
        call=f'{{const auto target=0x18b580u;s.push(0x{site:x}u,0x{site+5:x}u);dispatch(s,target,0x{site:x}u,false);}}'
        assert expected.count(call)==1;expected=expected.replace(call,f's.push(0x{site:x}u,0x{site+5:x}u);run_18b580(s);')
    assert expected==new['18f9b0']
    patch=''.join(difflib.unified_diff(before.decode().splitlines(True),after.decode().splitlines(True),fromfile='a/'+relative,tofile='b/'+relative))
    (out/'change.patch').write_text(patch);assert destination.read_bytes()==before;destination.write_bytes(after)
    receipt=dict(scope=__doc__,path=relative,before_sha256=sha(before),after_sha256=sha(after),
        existing_bodies_preserved_except_two_call_edges=True,added_original_body='0018b580',
        native_readiness_or_voice_results_supplied=False,launcher_installed=False)
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt),flush=True);return 0
if __name__=='__main__':raise SystemExit(main())
