#!/usr/bin/env python3
"""Complete-return rejection of the historical281890 coefficient reversal."""
import hashlib,json,os,shutil,sys
from pathlib import Path
import validate_native_accepted_body_contact_scalar_controls_v3_tu15 as scalar
ROOT=Path(__file__).resolve().parents[1]
def main():
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']);variant=scratch/'reversed-source';shutil.copytree(scalar.STAGE,variant)
 unit=variant/'src/physics/native_tu15_accepted_body_contact.cpp';text=unit.read_text();old='first_coefficient,second_coefficient,std::uint16_t(first_material),std::uint16_t(second_material)';new='second_coefficient,first_coefficient,std::uint16_t(first_material),std::uint16_t(second_material)';assert text.count(old)==1;unit.write_text(text.replace(old,new))
 original_observer=scalar.observer_source;source=original_observer()
 source=source.replace('const std::string mutation=argc==7?argv[6]:"";', 'const std::string mutation="reversed_products";')
 source=source.replace('numeric_mutant=mutation=="material"','numeric_mutant=mutation=="reversed_products"||mutation=="material"')
 source=source.replace('if(mutation.empty())for(const auto&control:meta.at("declared_scalar_controls"))', 'cases=J::array();for(const auto&control:meta.at("declared_scalar_controls"))if(control.at("name")=="unequal_products")')
 scalar.observer_source=lambda:source;scalar.STAGE=variant
 pin=hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
 try:scalar.main()
 finally:
  report=Path(sys.argv[sys.argv.index('--output')+1])/'report.json'
  if report.is_file():
   value=json.loads(report.read_text());value.update(scope=__doc__,historical_reverse_is_mutant=True,qualified_native_source=False,reversal_validator_sha256=pin,validator_unchanged=hashlib.sha256(Path(__file__).read_bytes()).hexdigest()==pin);report.write_text(json.dumps(value,indent=2,sort_keys=True)+'\n');target=report.parent/'source/tools'/Path(__file__).name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(__file__,target)
if __name__=='__main__':main()
