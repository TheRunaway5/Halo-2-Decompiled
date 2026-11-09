#!/usr/bin/env python3
"""Source-only bootstrap routing audit, distinguishing actual native handoffs."""
from pathlib import Path
import hashlib,json,os
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'analysis/physics/native-outside-frame-scope-audit-20261102'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 assert not(OUT/'bootstrap-report.json').exists();assert sha(NEW_DEFAULT)==NEW_PIN
 code=_Code(Xbe(NEW_DEFAULT),'.text');owners=[]
 for owner in(0x11ef70,0x1255c0,0x171120,0x2190c0,0x1f2c70):
  body=complete_body(code,owner);p=OUT/f'{owner:08x}.txt';p.write_text(''.join(f'{pc:x} {code.at(pc).mnemonic} {code.at(pc).op_str}\n'for pc in sorted(body.code)));owners.append(dict(owner=owner,sites=len(body.code),listing_sha256=sha(p)))
 names=['src/physics/physics_live_world_tu15.cpp','src/physics/physics_live_support_tu15_generated.cpp','src/runtime/source_map_world_support_tu15_generated.cpp','src/physics/runtime/physics_contact_runtime_generated.cpp','src/physics/runtime/physics_world_runtime_generated.cpp','src/rendering/models/native_tu15_render_object_ownership_binding.cpp']
 r=dict(passed=True,source_audit_only=True,original_sha256=NEW_PIN,owners=owners,source_sha256={n:sha(ROOT/n)for n in names},
  findings=[dict(owner='11EF70',route='Host initialize_map→run_actual→required→physics_live_support generated run11ef70',scope='Resident collision/physical-model/vehicle definition pass remains generated; child native handoffs do not close the owner.'),
   dict(owner='1F2C70',route='11F02C child dispatch; actual1F2C70 maps to canonical1EE380 in generated contact/world runtime',scope='Shape-reference fixup remains generated. No matching explicit native setup-owner interception found in current dispatcher/installed handoffs.'),
   dict(owner='1255C0',route='Initial/campaign BSP publication→physics_live_support generated run1255c0',scope='Resource publication includes owned physical BSP input and remains an open setup owner.'),
   dict(owner='171120',route='Host instanced collision prep→physics_live_support generated run171120→24FF70',scope='Instanced shape/MOPP setup parent remains generated.'),
   dict(owner='2190C0',route='Host body association→source_map_world_support owns_entry→generated run2190c0',scope='BSP-load association/exclusion initialization remains generated.'),
   dict(owner='158290',route='Actual native_tu15_render_object_ownership_owns_entry interception in run_actual/required',scope='Correction: the call_actual spelling is not evidence of lifted execution for this owner. Its native handoff/declared FP domain is separate from157FE0 and generated look callers.')],
  parent_audit_sha256=sha(OUT/'report.json'),no_canonical_or_cohort_writes=True,owned_scratch=os.environ['HALO2_TEST_SCRATCH'],temporary_allocated_bytes_before_cleanup=0,retained_large_artifacts=[],scope='Bounded read-only routing/source audit, no bootstrap kernel conversion or live native claim. Physics-specific map setup is in the all-physics goal.')
 (OUT/'bootstrap-report.json').write_text(json.dumps(r,indent=2)+'\n');print(sha(OUT/'bootstrap-report.json'))
if __name__=='__main__':main()
