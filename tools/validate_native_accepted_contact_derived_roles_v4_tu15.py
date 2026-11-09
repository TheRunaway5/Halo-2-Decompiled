#!/usr/bin/env python3
"""Native-only malformed-pointer controls on genuine topology, exact V4 helper."""
import hashlib,inspect,json,shutil,sys
from pathlib import Path
import validate_native_accepted_body_contact_scalar_controls_v4_tu15 as v4
ROOT=Path(__file__).resolve().parents[1]
def main():
 code=inspect.getsource(v4.main);marker=' scalar.observer_source=lambda:source;scalar.STAGE=STAGE'
 assert code.count(marker)==1
 code=code.replace(marker,''' line=' if(mutation.empty())for(const auto&control:meta.at("declared_scalar_controls"))'
 begin=source.index(line);source=source[:begin]+source[source.index('\\n',begin):]
 source=source.replace(' if(numeric_mutant)check(mutant_profiles',ALIASES+' if(numeric_mutant)check(mutant_profiles')
'''+marker)
 probe=ROOT/'tools/observe_native_accepted_contact_derived_roles_v4_tu15.inc';pin=hashlib.sha256(probe.read_bytes()).hexdigest();tool=hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
 env=dict(v4.__dict__,ALIASES=probe.read_text());exec(code,env)
 try:env['main']()
 finally:
  path=Path(sys.argv[sys.argv.index('--output')+1]);report=path/'report.json'
  if report.is_file():
   r=json.loads(report.read_text());r.update(scope=__doc__,alias_controls_are_not_original_or_actual_gameplay_inputs=True,alias_probe_sha256=pin,alias_validator_sha256=tool,alias_sources_unchanged=hashlib.sha256(probe.read_bytes()).hexdigest()==pin and hashlib.sha256(Path(__file__).read_bytes()).hexdigest()==tool);report.write_text(json.dumps(r,indent=2,sort_keys=True)+'\n')
   for source in (probe,Path(__file__)):
    target=path/'source/tools'/source.name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(source,target)
if __name__=='__main__':main()
