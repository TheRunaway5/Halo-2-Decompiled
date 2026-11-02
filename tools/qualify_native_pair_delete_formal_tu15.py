#!/usr/bin/env python3
"""Qualify bounded actual-DATA320260 cohorts; record every unexercised site."""
import hashlib,json
from collections import Counter
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
ROOT=Path(__file__).resolve().parents[1]
STAGE=ROOT/'analysis/physics/native-pair-delete-formal-stage-20261102'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def save(p,x):p.write_text(json.dumps(x,indent=2,sort_keys=True)+'\n')
def main():
 path=ROOT/'analysis/physics/native-pair-delete-formal-tu15-20261102-v3/report.json';report=json.loads(path.read_text());assert report['passed'] and sha(NEW_DEFAULT)==NEW_PIN
 pins=json.loads((STAGE/'source-pins.json').read_text())
 for name,digest in pins['staged_hashes'].items():assert sha(STAGE/'staged'/name)==digest,name
 code=_Code(Xbe(NEW_DEFAULT),'.text');body=complete_body(code,0x320260);compilers={};sites=None
 for compiler in('g++','clang++'):
  rows=[json.loads(p.read_text())for p in(path.parent/compiler/'evidence').glob('child*.json')];assert len(rows)==36 and all(r['passed']for r in rows)
  seen=set().union(*(set(r['executed_sites'])for r in rows));assert sites is None or sites==seen;sites=seen
  children=Counter(hex(c['entry'])for r in rows for c in r['original_callbacks'])
  compilers[compiler]=dict(whole_public_returns=36,source_sites_executed=len(seen),persistent_compared_bytes=sum(r['persistent_compared_bytes']for r in rows),genuine_original_child_calls=dict(children),
   no_captured_delete_caller_ABI=True,declared_input_profiles=['cached first/last/scan','missing record','genuine320DA0-produced pending first/second island','genuine other-island contact records first/last'])
 missing=sorted(set(body.code)-sites);save(STAGE/'unexercised-sites.json',dict(source_sites_total=len(body.code),executed=len(sites),missing=[dict(address=p,assembly=code.at(p).mnemonic+' '+code.at(p).op_str)for p in missing]))
 owners=[]
 for label in('native-pair-delete-tu15-20261102-first','native-pair-delete-formal-tu15-20261102-first','native-pair-delete-formal-tu15-20261102-v2','native-pair-delete-formal-tu15-20261102-v3'):
  p=ROOT/'analysis/physics'/label/'report.json';r=json.loads(p.read_text());assert not Path(r['owned_scratch']).exists();owners.append(dict(owned_scratch=r['owned_scratch'],allocated_bytes_before_cleanup=r['temporary_allocated_bytes_before_cleanup'],removed_verified=True,report=str(p.relative_to(ROOT))))
 save(STAGE/'cleanup-verification.json',dict(verified=True,completed_owners=owners,total_parent_allocation_bytes_removed=sum(r['allocated_bytes_before_cleanup']for r in owners),retained_large_artifacts=[],protected_reference_inputs_preserved=True,not_a_global_free_space_measurement=True))
 pins.update(qualified=True,qualification_scope='Bounded full-return public320260 actual-DATA cohorts; not universal branch/callback/fault/alias or live closure.',
  qualified_API='native_tu15_delete_pair(memory,handler,pair,NativeTu15PairDeletionScratch{formal_DATA},fp,services,observer)',
  old_no_scratch_prototype_qualified=False,report=str(path.relative_to(ROOT)),report_sha256=sha(path),compilers=compilers,
  source_sites_total=len(body.code),source_sites_executed=len(sites),unexercised_sites=len(missing),
  rejected_mutants=2,source_formal_DATA_store='3203C9 preserves actual current second handle in ordinary caller-owned4B DATA; no guest stack/GP input.',
  original_pending_rows='Real unchanged320DA0 preparation produces pending rows before the independent320260 comparison; no invented physics rows or supplied delete results.',
  remaining=['Remaining source branches listed in unexercised-sites.json','Exact World production builder composition with native2F3100/323220 child services and fail-closed unknowns','World wiring/counters/coherent models','Actual visible multiplayer/full-frame comparison'],production_applied=False,live_qualified=False)
 save(STAGE/'source-pins.json',pins);print(dict(qualified=True,scope='bounded',sites=len(sites),total=len(body.code),removed_bytes=sum(r['allocated_bytes_before_cleanup']for r in owners)))
if __name__=='__main__':main()
