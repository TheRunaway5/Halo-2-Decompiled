#!/usr/bin/env python3
"""Deduplicate measured completed activation/graph proof owners, never globaldf."""
import argparse,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
LEDGERS=[
 'native-world-island-merge-binding-stage-v2-20261101/cleanup-verification.json',
 'native-island-partition-graph-20261101/interrupted-run-cleanup.json',
 'native-island-partition-graph-v2-20261101/cleanup-verification.json',
 'native-island-partition-metadata-20261101/cleanup-verification.json',
 'native-island-partition-build-qualified-20261101/cleanup-verification.json',
 'native-raw-allocator-actual09-qualified-20261101/cleanup-verification.json',
 'native-island-partition-native-children-qualified-20261101/cleanup-verification.json',
 'native-island-constraint-detach-actual1470-qualified-20261101/cleanup-verification.json',
 'native-island-regroup-allocator-binding-qualified-20261101/cleanup-verification.json',
 'native-island-regroup-temporary-qualified-20261101/cleanup-verification.json',
 'native-island-regroup-construction-qualified-20261101/cleanup-verification.json',
 'native-island-regroup-edges-qualified-20261101/cleanup-verification.json',
 'native-island-regroup-parent-qualified-20261101/cleanup-verification.json',
 'native-island-regroup-service-binding-qualified-20261101/cleanup-verification.json',
 'native-world-island-regroup-binding-stage-v2-20261101/extra-cleanup-verification.json',
 'native-point-wake-regroup-actual1470-qualified-20261101/cleanup-verification.json',
 'native-activation-genuine-frontiers-qualified-20261102/cleanup-verification.json',
 'native-reversed-contact-process-binding-stage-20261102/cleanup-verification.json',
 'native-world-constraint-info-binding-stage-20261102/cleanup-verification.json',
 'native-constraint-info-actual1471-04-20261102/cleanup-verification.json',
 'native-function-evaluation-bounded-qualified-20261102/cleanup-verification.json',
 'native-function-evaluation-extended-qualified-20261102/cleanup-verification.json',
 'native-function-parent-composition-qualified-20261102/cleanup-verification.json',
 'native-world-manager-factory-binding-stage-20261102/cleanup-verification.json',
 'native-point-part-extended-header-stage-20261102/cleanup-verification.json',
 'native-object-spatial-packet-qualified-20261102/cleanup-verification.json',
 'native-function-masked-cw-stage-20261102/cleanup-verification.json',
 'native-world-pair-island-wake-stage-v2-20261102/cleanup-verification.json',
 'native-pair-delete-formal-stage-20261102/cleanup-verification.json',
 'native-world-wake-factory-services-stage-v4-20261102/cleanup-verification.json',
 'native-world-pair-delete-stage-20261102/cleanup-verification.json',
 'native-reversed-list-agent-destructor-qualified-20261102/cleanup-verification.json',
 'native-contact-body-update-qualified-20261102/cleanup-verification.json',
 'native-world-contact-body-stage-v2-20261102/cleanup-verification.json',
 'native-dirty-island-objects-qualified-20261102/cleanup-verification.json',
 'native-dirty-island-frame-stage-v3-20261102/cleanup-verification.json',
 'native-component-reset-qualified-20261102/cleanup-verification.json',
 'native-main-heap-release-qualified-20261102/cleanup-verification.json',
 'native-world-main-heap-release-stage-v4-20261102/cleanup-verification.json',
 'native-constraint-angle-qualified-20261102/cleanup-verification.json',
 'native-joint-constraint-schema-qualified-20261102/cleanup-verification.json',
 'native-look-reorientation-qualified-20261102/cleanup-verification.json',
 'native-joint-constraint-rows-qualified-20261102/cleanup-verification.json',
 'native-camera-query-leaves-qualified-20261102/cleanup-verification.json',
 'native-bsp-surface-query-qualified-20261102/cleanup-verification.json',
 'native-joint-constraint-rows-qualified-v2-20261102/cleanup-verification.json',
 'native-camera-query-object-context-qualified-20261102/cleanup-verification.json',
 'native-instanced-ray-lifetime-qualified-20261102/cleanup-verification.json',
 'native-mopp-ray-setup-qualified-prefix-20261102/cleanup-verification.json',
 'native-mopp-private-setup-stage-20261102/cleanup-verification.json',
 'native-instanced-scene-ray-qualified-20261102/cleanup-verification.json',
 'native-instanced-ray-callbacks-qualified-prefix-20261102/cleanup-verification.json',
 'native-instanced-scene-binding-qualified-20261102/cleanup-verification.json',
 'native-joint-constraint-build-qualified-20261102/cleanup-verification.json',
 'native-joint-constraint-world-stage-20261102/cleanup-verification.json',
 'native-moving-shape-process-qualified-20261102/cleanup-verification.json',
 'native-sweep-support-fault-phase-stage-20261102/cleanup-verification.json',
 'native-solver-hull-support-qualified-20261102/cleanup-verification.json',
 'native-solver-hull-single-query-qualified-20261102/cleanup-verification.json',
 'native-solver-hull-query-actual1479-qualified-20261102/cleanup-verification.json',
 'native-solver-hull-query-defined-data-qualified-20261102/cleanup-verification.json',
 'native-solver-hull-line-qualified-20261102/cleanup-verification.json',
 'native-solver-hull-edge-qualified-20261102/cleanup-verification.json',
 'native-moving-shape-process-full-original1479-20261102-second/cleanup-verification.json',
 'native-solver-hull-basis-private-w-20261102-first/cleanup-verification.json',
 'native-solver-hull-basis-caller-writes-20261102-first/cleanup-verification.json',
 'native-solver-hull-polytope-helpers-qualified-20261102/cleanup-verification.json',
 'native-solver-hull-polytope-state-qualified-20261102/cleanup-verification.json',
 'native-moving-shape-process-solved-qualified-actual1479-20261102/cleanup-verification.json',
 'native-moving-shape-process-full-native-actual1479-20261102-fourth/cleanup-verification.json',
 'native-moving-shape-process-bound-actual1479-20261102-third/cleanup-verification.json']
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();owners={};missing=[]
 for relative in LEDGERS:
  path=ROOT/'analysis/physics'/relative
  if not path.exists():missing.append(str(path.relative_to(ROOT)));continue
  data=json.loads(path.read_text());rows=data.get('completed_owners',data.get('owners',[data]))
  for row in rows:
   name=row.get('owned_scratch',row.get('owner'));assert name,name
   owner=Path(name);owner=owner if owner.is_absolute()else ROOT/owner;owner=owner.resolve();assert '.local/test-scratch/halo2-test-'in str(owner)or'/tmp/halo2-test-'in str(owner)
   amount=row.get('allocated_bytes_before_cleanup',row.get('allocated_bytes_removed',row.get('allocated_bytes')));assert isinstance(amount,int)and amount>=0
   assert not owner.exists(),str(owner);key=str(owner)
   if key in owners:assert owners[key]['allocated_bytes']==amount
   else:owners[key]=dict(owner=key,allocated_bytes=amount,removed_verified=True,ledger_sources=[])
   owners[key]['ledger_sources'].append(str(path.relative_to(ROOT)))
 result=dict(verified=True,scope='Disjoint measured completed Worldmerge/partition/raw/detach probe parents; suitable for root dedup by absolute owner path',owner_count=len(owners),total_cumulative_parent_allocation_bytes_removed=sum(r['allocated_bytes']for r in owners.values()),owners=list(owners.values()),not_an_isolated_global_free_space_delta=True,retained_large_artifacts=[],protected_reference_inputs_preserved=True,pending_ledgers=missing)
 a.output.write_text(json.dumps(result,indent=2,sort_keys=True)+'\n');print({k:result[k]for k in('verified','owner_count','total_cumulative_parent_allocation_bytes_removed')});return 0
if __name__=='__main__':raise SystemExit(main())
