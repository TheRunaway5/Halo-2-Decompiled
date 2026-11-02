#!/usr/bin/env python3
"""Seal the bounded candidate/private-DATA proof; no canonical source writes."""
import hashlib,json,shutil
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
STAGE=ROOT/'analysis/physics/native-contact-candidates-private-data-stage-v6-20261102'
PROOFS=(
'native-contact-candidates-bound-private-actual1479-20261102-sixth/report.json',
'native-contact-candidates-bound-public-default-actual1479-20261102-second/report.json',
'native-contact-candidates-public-compatibility-20261102-third/component/report.json',
'native-contact-array-reserve-private-public-20261102-first/component/report.json',
'native-contact-geometry-private-public-20261102-first/component/report.json',
'native-contact-inverse-geometry-private-public-20261102-first/component/report.json',
'native-contact-bounds-private-public-20261102-first/component/report.json',
'native-contact-cache-shapes-private-public-20261102-first/component/report.json',
'native-contact-key-query-private-public-20261102-first/component/report.json',
'native-contact-surface-private-public-20261102-first/component/report.json',
'native-mopp-aabb-code-dispatch-20261102-fourth/report.json')
TOOLS=('tools/prepare_native_contact_candidates_private_data_tu15.py',
'tools/validate_native_contact_candidates_bound_actual_tu15.py',
'tools/observe_native_contact_candidates_bound_actual_tu15.cpp',
'tools/native_contact_candidate_private_data_checks_tu15.hpp',
'tools/validate_native_contact_private_public_compatibility_tu15.py',
'tools/validate_native_mopp_aabb_dispatch_tu15.py',
'tools/seal_native_contact_candidates_binding_tu15.py')
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 manifest=STAGE/'source-pins.json';data=json.loads(manifest.read_text());assert not data['qualified']
 if not(STAGE/'prequalification-source-pins.json').exists():shutil.copyfile(manifest,STAGE/'prequalification-source-pins.json')
 proofs=[];owners={}
 for name in PROOFS:
  path=ROOT/'analysis/physics'/name;report=json.loads(path.read_text());assert report['passed'],name
  public_default=report.get('public_shared_algorithm_default_domain',False)
  for field in ('source_hashes','source_sha256'):
   hashes=report.get(field,{})
   if not isinstance(hashes,dict):continue
   for rel,pin in hashes.items():
    translated='src/physics/'+rel if rel.startswith('native_tu15_')and rel.endswith('.cpp')else rel
    if translated not in data['files']:continue
    expected=data['files'][translated]['after']
    if name.startswith('native-mopp-aabb-code-dispatch-')and translated in ('src/physics/native_tu15_mopp_aabb_traversal.cpp','src/physics/native_tu15_collision_component.cpp'):
     source=(STAGE/'staged'/translated).read_text()
     if translated.endswith('mopp_aabb_traversal.cpp'):
      at=source.index('    void run(');body=source[at:source.index('\n};',at)];control_pin=report['native_control_sha256']
     else:
      at=source.index('bool native_tu15_collision_component_enabled(');body=source[at:source.index('NativeTu15ContactComponent native_tu15_resolve_surface_component(',at)];control_pin=report['native_component_control_sha256']
     assert hashlib.sha256(body.encode()).hexdigest()==control_pin,(name,rel,'native-control scope changed')
     continue
    if public_default and translated=='include/halo2/native_tu15_contact_candidates_binding.hpp':
     source=(STAGE/'staged'/translated).read_text();before='        native_tu15_update_contact_candidates_defined_data(memory,producer';assert source.count(before)==1
     expected=hashlib.sha256(source.replace(before,'        native_tu15_update_contact_candidates(memory,producer').encode()).hexdigest()
    assert pin==expected,(name,rel)
  owner=report.get('owned_scratch');receipt=path.parent.parent/'overlay-receipt.json';wrappers=[]
  if '/component/'in str(path)and receipt.exists():
   r=json.loads(receipt.read_text());owner=r['owned_scratch'];removed=r['temporary_allocated_bytes_before_cleanup'];wrappers.append(dict(path=str(receipt.relative_to(ROOT)),sha256=sha(receipt)))
  else:removed=report.get('temporary_allocated_bytes_before_cleanup',report.get('temporary_allocated_bytes_before_owner_cleanup'))
  assert owner and not Path(owner).exists(),(name,owner);owners.setdefault(owner,removed)
  proofs.append(dict(path=str(path.relative_to(ROOT)),sha256=sha(path),passed=True,owned_scratch=owner,owner_absent=True,removed_allocated_regular_file_bytes=removed,wrapper_receipts=wrappers,relevant_source_inputs_equal_final_stage=True,immutable_CODE_proof_control_subtrees_equal=name.startswith('native-mopp-aabb-code-dispatch-'),declared_public_default_call_override=public_default))
 for rel,pin in data['files'].items():assert sha(STAGE/'staged'/rel)==pin['after']
 source_pins={}
 for rel in TOOLS:
  source_pins[rel]=sha(ROOT/rel);target=STAGE/'proof-source'/rel;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/rel,target)
 peer=ROOT/'analysis/physics/native-mopp-aabb-code-dispatch-independent-peer-review-20261102/review.json';assert json.loads(peer.read_text())['passed']
 data.update(qualified=True,root_reviewed=False,independent_peer_reviewed=False,
  qualification_scope='Selected actual304 BEFORE-derived ordinary2F36F0 component invocation with native-only providers,4FP profiles/compiler, all89,795,481 captured bytes/634orderedphysicsDATAreads/114retained SourcePC stores+FP; typed retained-only private DATA ownership/definedness/copy contract. No captured2F ABI, productionWorld, live delivery, allprofiles or generic raw private observation claim.',
  proofs=proofs,proof_source_hashes=source_pins,selected_native_provider_closure_qualified=True,
  public_default_compatibility_qualified=True,owned_private_definition_controls_per_compiler=38,
  actual_semantic_mutants_rejected=['wrong_transform','opaque_entry_read','upper_publication'],
  immutable_CODE_fetches_only=[0x309960,0x309967,0x1f76ba],
  immutable_CODE_independent_peer=dict(path=str(peer.relative_to(ROOT)),sha256=sha(peer)),
  captured_bank_exclusions=[],physics_DATA_read_exclusions=[],canonical_source_edits=False,
  world_or_live_admission=False,
  caller_obligations=['One noncopied owned private arena/view survives all synchronized sibling calls','Complete retained input/output extents must be declared and disjoint from private ranges','All native providers receive the SAME guarded memory view','Allocator/presized output extents must refuse private aliases','Typed retained-only observer; raw private preimages/address faults and arbitrary reentry not admitted'],
  remaining=['World/provider composition, counters/gates and fresh visible fullcase delivery','Genuine303EE0 constructor field lifetimes and record-growth unknown escapes owned byRoot','Selected actual2F agent release0; otherfactory/owner/profiles remain named/native-scope refusals','Public compatibility retains its original-child service proof scope; no native-only promotion','Inherited unsupported FP/alias/private-radius/invalid MOPP/property domains remain explicit'],
  cleanup=dict(disjoint_owners=len(owners),allocated_regular_file_bytes_removed=sum(x for x in owners.values()if x is not None),all_owners_absent=True,retained_large_artifacts=[]))
 manifest.write_text(json.dumps(data,indent=2,sort_keys=True)+'\n')
 (STAGE/'QUALIFICATION.md').write_text('''# Native candidate service and private DATA qualification

- [x] Actual304 typed BEFORE:4033 banks/89,795,481B. The16B ordinary second payload follows Source304 fields, including3043B5 parent. No capturedGP or after-image enters native code.
- [x] Native2F36F0 closure:4FP profiles/compiler,634 ordered physicsDATA reads,114 retained SourcePC stores/preimages, fullRAM andFP.3 keys/resolutions/filters/createdagents;0refusal/0originalCPUchildren.
- [x]38 definedness/alias/observer/copy/refusal controls and3 actual semantic mutants/compiler. Undefined local bits never acquire a value or escape into retained memory.
- [x] Public/default path:42 prior candidate cases/3mutants, actual4FP;758geometry,269inverse,334bounds,64reserve including8faults,540key-query,120surface,70cache/native-shape cases. Existing narrower fault/alias limits remain.
- [x] Only3 authenticated immutableCODE fetches use native control flow.108opcode classes/all256bytevalues and5valid key-kinds checked;4mutants/independentpeerGREEN. No captured byte or physicsDATA read excluded. Raw failed ledger evidence preserved.
- [x] All proof owners removed; no new large artifact retained. Required archives/videos/source layers protected.
- [ ] Independent final source/spec peer; World delivery/counters/live workflows.
- [ ] Root303EE0 constructor/record lifetime and query continuation; no full304 claim.

The private arena is an operation owner and must not be copied. Its complete retained extent list and native allocator/presized-output guards are caller obligations. All consumers use the same view. Raw private preimage/address-fault or arbitrary reentry/mutation contracts are unqualified. Existing numeric APIs retain fully provided ordinary DATA and default observers.
''')
 print(sha(manifest),manifest)
if __name__=='__main__':main()
