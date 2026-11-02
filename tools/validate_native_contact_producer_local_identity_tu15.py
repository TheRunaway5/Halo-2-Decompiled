#!/usr/bin/env python3
"""Requalify whole304350 with its typed304387 producer DATA store staged."""
import inspect,sys,json
from pathlib import Path
import validate_native_contact_producer_cohort_tu15 as parent
def main():
 index=sys.argv.index('--stage');stage=Path(sys.argv[index+1]).resolve();del sys.argv[index:index+2]
 pins=json.loads((stage/'source-pins.json').read_text());source=stage/'staged/src/physics/native_tu15_contact_producer.cpp'
 assert parent.sha(source)==pins['staged_hashes']['src/physics/native_tu15_contact_producer.cpp']
 text=inspect.getsource(parent.main);needle="source=ROOT/'src/physics/native_tu15_contact_producer.cpp'";assert text.count(needle)==1;text=text.replace(needle,'source=staged_producer')
 needle="'-I'+str(ROOT/'include'),str(observer)";assert text.count(needle)==1
 text=text.replace(needle,"'-I'+str(ROOT/'include'),'-I'+str(ROOT/'src/physics'),str(observer)")
 env=dict(parent.__dict__,__file__=__file__,staged_producer=source);exec(text,env);result=env['main']()
 out=Path(sys.argv[sys.argv.index('--output')+1])/'report.json';r=json.loads(out.read_text());r.update(scope=__doc__,staged_manifest_sha256=parent.sha(stage/'source-pins.json'),remaining_services_execute_original_cpu=True,native_only_world_claim=False,constructor_preimage_claim=False,producer_local_data='Only304387 stores ordinary typed producer.identity; no GP/SIMD/control image supplied. Earlier borrowed constructor bytes remain genuine unknown inputs.');parent.save(out,r);return result
if __name__=='__main__':raise SystemExit(main())
