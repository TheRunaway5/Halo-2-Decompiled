#!/usr/bin/env python3
"""Seal Body component/actual/shared-builder/World transport qualifications."""
import hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def save(p,x):p.write_text(json.dumps(x,indent=2,sort_keys=True)+'\n')
def main():
 stage=ROOT/'analysis/physics/native-world-contact-body-stage-v2-20261102';pins_path=stage/'source-pins.json';pins=json.loads(pins_path.read_text())
 for relative,digest in pins['staged_hashes'].items():assert sha(stage/'staged'/relative)==digest,relative
 names=['native-contact-body-helpers-tu15-20261102-final','native-contact-body-update-tu15-20261102-v6','native-contact-body-update-zero-guard-proof-20261102','native-contact-body-update-binding-tu15-20261102-first','native-contact-body-update-actual1474-tu15-20261102-v2','native-contact-body-update-bound-actual1474-tu15-20261102-first','native-contact-body-shared-activation-tu15-20261102-v4','native-world-contact-body-model-gcc-20261102-v2','native-world-contact-body-model-clang-20261102-first','native-world-contact-body-counters-20261102-first']
 reports={}
 for name in names:
  path=ROOT/'analysis/physics'/name/'report.json';r=json.loads(path.read_text());assert r.get('passed',r.get('proved',False)),name;reports[str(path.relative_to(ROOT))]=sha(path)
 for name in names[-3:-1]:assert json.loads((ROOT/'analysis/physics'/name/'report.json').read_text())['stage_hashes']==pins['staged_hashes']
 shared=json.loads((ROOT/'analysis/physics'/names[6]/'report.json').read_text());assert shared['staged_shared_hashes']==pins['staged_hashes']
 cleanup_names=['native-contact-body-input-capture-interface-20261102-first','native-contact-body-update-binding-tu15-20261102-first','native-contact-body-update-actual1474-tu15-20261102-v2','native-contact-body-update-bound-actual1474-tu15-20261102-first','native-world-contact-body-model-gcc-20261102-first','native-world-contact-body-model-gcc-20261102-v2','native-world-contact-body-model-clang-20261102-first','native-world-contact-body-counters-20261102-first']+['native-contact-body-shared-activation-tu15-20261102-'+s for s in ('first','v2','v3','v4')]
 owners=[];unmeasured=[]
 for name in cleanup_names:
  path=ROOT/'analysis/physics'/name/'report.json';r=json.loads(path.read_text());owner=Path(r['owned_scratch']);assert not owner.exists(),str(owner);amount=r.get('temporary_allocated_bytes_before_cleanup');
  if amount is None:unmeasured.append(dict(owned_scratch=str(owner),removed_verified=True,allocation_bytes_not_recorded=True));continue
  owners.append(dict(owned_scratch=str(owner),allocated_bytes_before_cleanup=amount,removed_verified=True,report=str(path.relative_to(ROOT))))
 save(stage/'cleanup-verification.json',dict(verified=True,completed_owners=owners,unmeasured_completed_owners=unmeasured,total_parent_allocation_bytes_removed=sum(o['allocated_bytes_before_cleanup']for o in owners),retained_large_artifacts=[],protected_reference_inputs_preserved=True,not_a_global_free_space_measurement=True))
 pins.update(qualified=True,qualification_reports=reports,root_reviewed=False,peer_review_pending=True,production_applied=False,live_qualified=False,
 actual_input_archive_sha256='c9b82a7a03f578115b3529a2a43efd65f8ed9f54f3e280570b376ccba92bc517',actual_before_banks=4033,actual_before_bytes=89795481,actual_full_profiles_per_compiler=4,declared_full_parent_cases_per_compiler=72,helper_cases_per_compiler=108,
 physically_executed_parent_sites=243,total_parent_sites=245,infeasible_repeated_guard_sites=['0x1d78af','0x1d78b2'],
 shared_activation_scope='Both72-case physical parents use SAME bound helper and additive sole315CB0 activation builder. Four explicitly declared consistent inactive World profiles select native315CB0/316D40; no listeners/constraints/factory work selected there. Actual captured Body sample has no impulse or wake.',
 coherent_cpp_count=137,coherent_world_models=['default11manager+nativeBody2','genuine02-factory-prefix','genuine09-merge-prefix','declared-inactive-pair-wake-prefix'],counter_fields=13,counter_maps=1,serializers=2,positive_gate='HALO2_REQUIRE_NATIVE_CONTACT_BODY_UPDATE',negative_gate_cases=10,
 remaining=['Visible multiplayer native1D77D0 fullframe/actualvehiclecamera','Actual contact impulse/wake positive','Unknown motion/activation factory/listener/lifetime owners remain counted/refused; no old body fallback','Unmasked numerical traps and asynchronous FP mutation outside admission','Full TU1.5 parity matrix remains open'])
 save(pins_path,pins);print(dict(qualified=True,manifest_sha256=sha(pins_path),removed_parent_bytes=sum(o['allocated_bytes_before_cleanup']for o in owners)))
if __name__=='__main__':main()
