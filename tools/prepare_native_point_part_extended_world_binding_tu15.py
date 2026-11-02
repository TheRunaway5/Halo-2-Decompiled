#!/usr/bin/env python3
"""Stage the independently qualified extended authored-function point helper."""
import argparse,difflib,hashlib,json,shutil
from pathlib import Path
from prepare_native_phantom_deletion_world_binding_tu15 import change
ROOT=Path(__file__).resolve().parents[1]
CORE=('native_tu15_function_evaluation_extended','native_tu15_function_curves','native_tu15_function_control_point')
FIELDS={
 'native_contact_point_part_function_refusals':'part.function.unsupported_kinds+part.function.unsupported_power_profiles+part.function.curves.unsupported_types+part.function.curves.unsupported_inputs',
 'native_contact_point_part_function_fp_refusals':'part.function.unsupported_fp_profiles+part.function.curves.unsupported_fp_profiles'}
def sha(raw):return hashlib.sha256(raw).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--base-stage',required=True,type=Path);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);stage=a.output/'staged';shutil.copytree(a.base_stage/'staged',stage);oldpins={};patch=[]
 header='include/halo2/native_tu15_contact_point_binding.hpp';candidate=ROOT/'analysis/physics/native-point-part-extended-header-stage-20261102/staged'/header
 names=(header,'src/physics/native_tu15_world_integration_residuals.cpp','cmake/PhysicsLiveWorldTu15.cmake')
 for name in names:
  path=stage/name;raw=path.read_bytes();text=raw.decode();oldpins[name]=sha(raw)
  if name==header:text=candidate.read_text()
  elif name.endswith('world_integration_residuals.cpp'):
   for field,expression in FIELDS.items():
    before='activity.part.function.'+('unsupported_kinds'if field.endswith('function_refusals')else'unsupported_fp_profiles')
    text=change(text,'stats->'+field+'+='+before+';','stats->'+field+'+='+'+'.join('activity.'+term for term in expression.split('+'))+';')
  else:text=change(text,'    src/core/native_tu15_function_evaluation.cpp)','    '+'\n    '.join('src/core/'+n+'.cpp'for n in CORE)+')')
  path.write_text(text);patch+=difflib.unified_diff(raw.decode().splitlines(True),text.splitlines(True),fromfile='a/'+name,tofile='b/'+name)
 new=['include/halo2/native_tu15_part_contact_binding_extended.hpp',*['include/halo2/'+n+'.hpp'for n in CORE],*['src/core/'+n+'.cpp'for n in CORE]]
 for name in new:
  target=stage/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,target)
 hashes={str(p.relative_to(stage)):sha(p.read_bytes())for p in stage.rglob('*')if p.is_file()};(a.output/'native-point-part-extended-binding.diff').write_text(''.join(patch))
 proof=ROOT/'analysis/physics/native-point-part-gameplay-extended-composition-20261102-third/report.json';r=json.loads(proof.read_text());assert r['passed']and r['unbound_service_prefixes']==0
 assert r['staged_point_header_sha256']==hashes[header]
 for name in new:assert r['source_hashes'][name]==hashes[name],name
 (a.output/'source-pins.json').write_text(json.dumps(dict(base_stage=str(a.base_stage.resolve()),base_stage_pins_sha256=sha((a.base_stage/'source-pins.json').read_bytes()),base_hashes=oldpins,staged_hashes=hashes,changed_aggregate_fields=FIELDS,whole_gameplay_proof=str(proof.relative_to(ROOT)),whole_gameplay_proof_sha256=sha(proof.read_bytes()),production_applied=False,live_qualified=False,scope='Exact extendedpoint helper/three native authoredmath units; both153wholegameplay profiles/full221sites and44positive11kind callers. SameDATA scratch+currentFP/depth; normalfinite numericaldomain, exceptional math/FP profiles explicitlyrefuse andaggregate into existing diagnosticfields. All prior shared source/statfields preserved; no CPUfallback.'),indent=2,sort_keys=True)+'\n');print(a.output)
if __name__=='__main__':raise SystemExit(main())
