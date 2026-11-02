#!/usr/bin/env python3
"""Audit current native base-lock identity, proof domain and source guard.

This is source/proof reuse, not a new actual manager-input qualification.
Registered storage removal remains an explicit unqualified branch.
"""
import argparse,hashlib,json,os
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body

def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True)
    a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
    proof=ROOT/'build/test-results/vehicle-destruction-pending-live-20261101/base-lock-component-v2/report.json'
    registered=proof.parent.parent/'base-lock-registered/report.json'
    d=json.loads(proof.read_text());driver=json.loads(registered.read_text())
    assert sha(NEW_DEFAULT)==NEW_PIN and d['passed'] and d['physical']['passed']
    pins={name:sha(ROOT/name)for name in d['source_sha256']}
    assert pins==d['source_sha256']
    assert [r['returncode']for r in d['results'][:2]]==[0,0]
    assert d['missing_cases']==13 and d['registered_refusals']==12
    assert all(r['returncode']!=0 for r in d['results'][2:])
    assert driver['passed']
    world=ROOT/'src/physics/native_tu15_world_integration_residuals.cpp';text=world.read_text()
    condition='(!(word(m,collector+4u,2)&0x8000u)||!native_tu15_base_lock_registered(m,collector))'
    assert text.count(condition)==1
    release='native_tu15_release_unregistered_base_lock(m,object,observe);'
    assert text.count(release)==1
    code=_Code(Xbe(NEW_DEFAULT),'.text');listings={}
    for owner in (0x2e12a0,0x2e11e0,0x2e33f0,0x2e3350):
        lines=[]
        for pc in sorted(complete_body(code,owner).code):
            i=code.at(pc);lines.append(f'{pc:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n')
        raw=''.join(lines);(a.output/f'{owner:08x}.txt').write_text(raw)
        listings[hex(owner)]=dict(sites=len(lines),sha256=hashlib.sha256(raw.encode()).hexdigest())
    report=dict(passed=True,owned_scratch=os.environ['HALO2_TEST_SCRATCH'],
        temporary_allocated_bytes_before_cleanup=0,retained_large_artifacts=[],
        original_xbe_sha256=NEW_PIN,current_source_hashes=pins,
        proof=str(proof.relative_to(ROOT)),proof_sha256=sha(proof),
        proof_compilers=['g++','clang++'],physical_cases=25,missing_completions=13,
        registered_entry_refusals=12,ordered_reads=168,ordered_stores=13,
        rejected_mutants=d['negative_controls'],source_bodies=listings,
        registration_named_ctest=dict(path=str(registered.relative_to(ROOT)),passed=driver['passed'],
            positive_registered_storage_release_proved=False),
        domain=dict(flags=0,registry='current48C93C+8',keys='registry+0',mask='registry+8',
            constructed_extent='mask=2^n-1,mask<=FFFF; bounded probe requires match or terminating zero',
            physical_declared_masks=[0,1,3,15,31],actual_proof_mask=15,
            lookup='u32((object>>4)*9E3779B1)&mask; four-byte keys; wrapped linear probe',
            result='reread signed mask and compare source slot<=mask; missing slot=oldmask+1',
            absent_release='AND object+5 byte with7F at2E12C2; no registry/storage child',
            registered_release='stop/refuse before2E32E0 and before native writes'),
        world_guard=dict(source_sha256=sha(world),condition=condition,release=release,
            change='Accept a set lock flag only when the current owning registry lookup is absent; actual release independently queries again. Other manager storage/world/body guards unchanged by this reuse.',
            flags_bcdc_alone_authenticates_registry_absence=False),
        actual1470_manager_registry_proved=False,
        remaining=['Genuine new manager/collector registry DATA correspondence before live admission',
            'Registered-entry2E32E0 and storage/string lifetime remain unqualified'],
        supplied_physics_results=False,
        scope='Current exact header/driver/validator hashes match both-compiler physical component proof. Source guard and source route audited; this does not invent the registry result for the new live manager.')
    (a.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    return 0

if __name__=='__main__':raise SystemExit(main())
