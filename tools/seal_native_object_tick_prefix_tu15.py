#!/usr/bin/env python3
"""Seal selected B8F40 semantic parent evidence without admitting its children."""
from pathlib import Path
import ast,hashlib,json,shutil,sys
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def save(p,d):p.write_text(json.dumps(d,indent=2,sort_keys=True)+'\n')
def main():
 out=ROOT/'analysis/physics/native-object-tick-qualified-prefix-20261102';out.mkdir(exist_ok=False)
 draft=ROOT/'analysis/physics/native-object-tick-stage-20261102/staged'
 names=['src/physics/native_tu15_object_tick.cpp','include/halo2/native_tu15_object_tick.hpp','include/halo2/native_tu15_physics_frame.hpp','include/halo2/physics_contact_record.hpp']
 for name in names:
  src=draft/name if (draft/name).is_file()else ROOT/name;dst=out/'source'/name;dst.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(src,dst)
 proofs={};reports=[]
 for n in ['native-object-tick-parent-tu15-20261102-second','native-object-tick-services-tu15-20261102-first','native-object-tick-context-20261102-first']:
  p=ROOT/'analysis/physics'/n/'report.json';d=json.loads(p.read_text());assert d['passed']is True and not d.get('error');assert not Path(d['owned_scratch']).exists();proofs[str(p.relative_to(ROOT))]=sha(p);reports.append(d)
 physical,controls=reports[:2]
 for name,h in physical['source_sha256'].items():assert sha(out/'source'/name)==h
 # Recreate the exact retained driver from its generating AST, without
 # running the original, allocating fixtures, or compiling a new program.
 validator=ROOT/'tools/validate_native_object_tick_tu15.py';tree=ast.parse(validator.read_text());main=next(x for x in tree.body if isinstance(x,ast.FunctionDef)and x.name=='main');body=next(x for x in main.body if isinstance(x,ast.Try)).body
 start=next(i for i,x in enumerate(body)if isinstance(x,ast.Assign)and any(isinstance(t,ast.Name)and t.id=='driver'for t in x.targets));end=next(i for i in range(start,len(body))if isinstance(body[i],ast.Expr)and "write_text(driver)"in ast.unparse(body[i]))
 env={'ROOT':ROOT,'BACKEND':'unchanged-tu15-native-object-tick-parent-v1'}
 exec(compile(ast.fix_missing_locations(ast.Module(body=body[start:end],type_ignores=[])),str(validator),'exec'),env)
 driver=out/'proof-source/driver.cpp';driver.parent.mkdir();driver.write_text(env['driver']);assert sha(driver)==physical['generated_driver_sha256']
 tools=['tools/validate_native_object_tick_tu15.py','tools/validate_native_object_tick_services_tu15.py','tools/validate_native_island_agent_phase_tu15.py','tools/observe_native_constraint_step_tu15.cpp','tests/native_tu15_wrapped_contact_process_tests.cpp', 'tools/seal_native_object_tick_prefix_tu15.py']
 for name in tools:
  dst=out/'proof-source'/name;dst.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,dst)
 authority={}
 for owner in(0xb8f40,0xbe4a0,0x10e630):
  src=ROOT/'analysis/physics/native-outside-frame-scope-audit-20261102'/f'{owner:08x}.txt';dst=out/src.name;shutil.copyfile(src,dst);authority[dst.name]=sha(dst)
 readme='''# Selected native B8F40 object scheduler\n\nThe sole semantic parent caches the original scene list/count before the busy\nstore, rereads the retained header after each update, and keeps the second\niterator pool identity while resolving bodies through the current global pool.\nNormal cleanup uses the actual Source exit PC. Exceptions preserve completed\nstores, including the busy flag. No GP/stack reconstruction, instruction\ndispatch, original executor or default child outcome enters the public API.\n\nBoth compilers pass102 independent physical profiles, including40 genuine\noperand-fault/poststore observer prefixes,1852 ordered Source reads and156\nstores/preimages with complete declared banks and FP. Three mutants per\ncompiler reject wrong busy value, wrong sparse bitmap word and a value-\npreserving missing current global read. The exact generated driver is retained\nand hash-matched to the physical report. The earlier report-metadata NameError\nis preserved as a failed run; its numerical logs are not substituted for pass.\n\nCoverage is109/117 parent instructions. Eight instructions atB906B..B9079\n(the negative-index cleanup) remain unexecuted. Full declared returns use the\nunchanged Source BE4A0 active/flag20 early-return domain; other BE4A0 and\n10E630 paths stop at their genuine entries. The bounded native test provider\nimplements only that authenticated read-only early return. Its use qualifies\nneither the full125-site updater nor the69-site finalizer.\n\nEight additional protocol controls per compiler cover attempts/completions,\nmissing/throwing typed services, observer failures after the initial/final\nstore and changed current cleanup owner. These declared callbacks test the API\nprotocol only; they do not supply a native physics outcome or child proof.\n\nNo actual scheduler input, wider child graph, generated-caller liveness, native\nproduction binding, canonical integration or live object coverage is admitted.\nThe two inherited declaration headers are frozen for subsequent composition;\nthe physical report explicitly pins the two semantic units, while the later\nprotocol proof pins all four headers/units. No broader coherent World claim.\n'''
 (out/'README.md').write_text(readme)
 failed=ROOT/'analysis/physics/native-object-tick-parent-tu15-20261102-first/report.json';assert not json.loads(failed.read_text())['passed']
 ledger_path=ROOT/'analysis/physics/native-activation-owned-followups-20261102/cleanup-verification.json';ledger=json.loads(ledger_path.read_text());seen={r['owned_scratch']for r in ledger['completed_owners']}
 completed=[]
 for p in [failed,ROOT/'analysis/physics/native-object-tick-parent-tu15-20261102-second/report.json',ROOT/'analysis/physics/native-object-tick-services-tu15-20261102-first/report.json']:
  r=json.loads(p.read_text());owner=r['owned_scratch'];assert not Path(owner).exists();row=dict(owned_scratch=owner,allocated_bytes_before_cleanup=r['temporary_allocated_bytes_before_cleanup'],removed_verified=True,report=str(p.relative_to(ROOT)),report_sha256=sha(p));completed.append(row)
  if owner not in seen:ledger['completed_owners'].append(row);seen.add(owner)
 ledger['scope']='Owned source audits, Clang archive-model and B8F40 probes only; MP GCC/live and Root/foreign owners excluded';save(ledger_path,ledger)
 save(out/'cleanup-verification.json',dict(verified=True,completed_owners=completed,retained_large_artifacts=[]))
 manifest=dict(qualified=True,qualification_scope='Selected declared B8F40 parent domain and genuine child boundaries only',production_applied=False,world_qualified=False,live_qualified=False,root_reviewed=False,source_sha256={n:sha(out/'source'/n)for n in names},proofs=proofs,proof_source_sha256={str(p.relative_to(out)):sha(p)for p in(out/'proof-source').rglob('*')if p.is_file()},authority=authority,original_sha256=physical['original_sha256'],profiles=physical['profiles'],fault_or_observer_profiles=physical['fault_or_observer_profiles'],parent_sites=117,executed_parent_sites=physical['executed_parent_sites'],unexecuted_parent_sites=[hex(p)for p in physical['unexecuted_parent_sites']],ordered_reads=physical['ordered_reads'],ordered_stores=physical['ordered_stores'],compiler_mutants=physical['results'],service_protocol_controls_per_compiler=8,retained_failed_report={str(failed.relative_to(ROOT)):sha(failed)},readme_sha256=sha(out/'README.md'),cleanup_sha256=sha(out/'cleanup-verification.json'),retained_large_artifacts=[])
 save(out/'source-pins.json',manifest);print(sha(out/'source-pins.json'))
if __name__=='__main__':main()
