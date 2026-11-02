#!/usr/bin/env python3
"""Find genuine query writers to the next constructor scalar20; original only."""
import argparse,json,os,subprocess,sys
from pathlib import Path
from native_contact_process_before_input_tu15 import NativeContactProcessBeforeInput,sha
from build_native_tu15_constraint_before_reference import build_constraint_reference,save
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
ROOT=Path(__file__).resolve().parents[1]
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--offset',type=lambda s:int(s,0),action='append');p.add_argument('--phase',choices=['candidate','query'],default='query');a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False)
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],native_input_supplied=False)
    try:
        archive=ROOT/'analysis/multiplayer/physics-live-tu15-20261012/mcc-native-reorientation-joint-original-v1479-20261102/iteration-01-readonly-process-304350-driving-vehicle-camera/required-native-contact-process-304350-before/actual-native-before.tar.zst'
        before=NativeContactProcessBeforeInput(archive,'5ef6aba031fc184d390fcc65ad6da055e16a279212232da4252a36e0b69d5c9c',scratch/'before',0x304350)
        reference=build_constraint_reference(before,scratch/'reference');save(scratch/'input.json',before.input)
        names=['tools/observe_native_contact_candidate_entry_tu15.cpp','tools/observe_native_contact_constructor_query_alias_tu15.cpp',str(Path(__file__).relative_to(ROOT))];pins={n:sha(ROOT/n)for n in names}
        for name in names:
            destination=a.output/'source'/name;destination.parent.mkdir(parents=True,exist_ok=True);destination.write_bytes((ROOT/name).read_bytes())
        source=(ROOT/names[0]).read_text().split('int main(int argc,char**argv)')[0]+(ROOT/names[1]).read_text();(scratch/'observer.cpp').write_text(source)
        command=['g++','-std=c++20','-O2',str(scratch/'observer.cpp'),'-o',str(scratch/'observer')];r=subprocess.run(command,capture_output=True,text=True);assert r.returncode==0,r.stderr[-5000:]
        assert sha(NEW_DEFAULT)==NEW_PIN;code=_Code(Xbe(NEW_DEFAULT),'.text');sites={}
        for owner in(0x304350,0x303ee0,0x2e4fe0,0x2f8630,0x30e930,0x30de60,0x309060,0x309150,0x2f36f0,0x2f3140,0x1f6d10,0x2e4550,0x1f5fe0):
            for pc in complete_body(code,owner).code:
                i=code.at(pc);sites[pc+i.size]=dict(instruction=pc,bytes=i.bytes.hex(),mnemonic=i.mnemonic,operands=i.op_str)
        results=[]
        for mode in range(4):
            for offset in a.offset or [0x20]:
                assert 0<=offset<=0x7c and offset%4==0
                target=a.output/f'mode{mode}-offset{offset:x}.json';r=subprocess.run([str(scratch/'observer'),reference['elf'],str(scratch/'input.json'),str(mode),str(offset),a.phase,str(target)],capture_output=True,text=True);assert r.returncode==0,r.stdout+r.stderr
                row=json.loads(target.read_text());assert row['passed']
                for event in row['events']:
                    event['authenticated_source_instruction']=sites.get(event['after_pc']);assert event['authenticated_source_instruction'],event
                save(target,row);results.append(dict(mode=mode,offset=offset,report=str(target),sha256=sha(target),events=len(row['events'])))
        assert all(sha(ROOT/n)==pin for n,pin in pins.items())
        report.update(passed=True,actual_input=before.receipt,source_hashes=pins,results=results,original_sha256=NEW_PIN,scope='Original-only hardware last-writer trace. No trace bytes/GP/callback result feed native; exact semantic field alias remains to be independently established.')
    except BaseException as e:report['error']=str(e)or repr(e);raise
    finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
if __name__=='__main__':main()
