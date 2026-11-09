#!/usr/bin/env python3
"""V4 SAME helper: Source operand roles, selected callback and scalar controls."""
import hashlib,json,os,shutil,sys
from pathlib import Path
import validate_native_accepted_body_contact_scalar_controls_v3_tu15 as scalar
ROOT=Path(__file__).resolve().parents[1]
STAGE=ROOT/'analysis/physics/native-accepted-body-contact-semantic-stage-v4-20261103/staged'
def main():
 original=scalar.observer_source;source=original()
 source=source.replace('activity.row_appends==1&&activity.body_updates==1&&activity.lookups==2', 'activity.row_appends==1&&activity.row_initializations==1&&activity.body_updates==1&&activity.lookups==2&&activity.lookup_completions==2')
 marker='check(!scope.active()&&binding.attempts==1&&binding.completed==unsigned(!failed)&&binding.refusals==unsigned(failed),"Bound native activity/scope restore differs");'
 assert source.count(marker)==1
 source=source.replace(marker,marker+'''check(!binding.role_refusals&&!binding.metadata.calls,"Unexpected role refusal or mass scaling in selected non-scaling profiles");
   if(failed){const auto pc=fault.at("source_instruction").get<std::uint32_t>();
    if(pc==0x1d87bdu||pc==0x1d87e9u||pc==0x1d76b6u)check(activity.row_appends==1u&&!activity.row_initializations,"Row completion counted before the initializer returned");
    if(pc==0x1d781du||pc==0x1d7820u||pc==0x1d78f2u||pc==0x20d8f9u||pc==0x22e0d0u)check(activity.row_initializations==1u,"Completed row initializer not counted");
    if(pc==0x22e0d0u)check(activity.lookups==2u&&activity.lookup_completions==2u,"Normal lookup completions missing at following fault");}
''')
 scalar.observer_source=lambda:source;scalar.STAGE=STAGE
 previous=scalar.base.UNITS;scalar.base.UNITS=(*previous,'native_tu15_island_partition_metadata.cpp')
 pin=hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
 try:scalar.main()
 finally:
  scalar.base.UNITS=previous;p=Path(sys.argv[sys.argv.index('--output')+1]);report=p/'report.json'
  if report.is_file():
   value=json.loads(report.read_text());value.update(scope=__doc__,same_role_decorator=True,v4_validator_sha256=pin,v4_validator_unchanged=hashlib.sha256(Path(__file__).read_bytes()).hexdigest()==pin);report.write_text(json.dumps(value,indent=2,sort_keys=True)+'\n');target=p/'source/tools'/Path(__file__).name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(__file__,target)
if __name__=='__main__':main()
