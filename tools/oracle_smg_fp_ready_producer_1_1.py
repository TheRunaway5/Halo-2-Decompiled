#!/usr/bin/env python3
"""Separate normal-wrapper ready producer, with no supplied animation request.

The frozen FP-clock suite's ready control includes a direct request-0 E9D50
admission probe which falls back before the wrapper selects the real ready.
This independent fixture removes that probe without modifying that freeze.
Only the current inventory index is an explicit admission boundary. Original
C7E90, E9D50,1040F0,EFE30 and16A356 produce the timed equip and FP selection.
"""
import argparse,hashlib,json,struct
from pathlib import Path
from oracle_smg_fp_clock_1_1 import ClockTrace,ROOT,OUT,dump_instructions
from oracle_smg_firing_1_1 import PIN,CACHE_PIN,UNIT,PLAYER_DATA

CLOCK_PIN='7d5548e26d3503ea1057aef3202a9285d7e9fa431d0a943c372788b64282b5d6'

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--ticks',type=int,default=25)
    ap.add_argument('--output',type=Path,default=OUT/'ready-producer/report.json')
    ap.add_argument('--instructions',type=Path);a=ap.parse_args()
    assert hashlib.sha256((ROOT/'tools/oracle_smg_fp_clock_1_1.py').read_bytes()).hexdigest()==CLOCK_PIN
    t=ClockTrace();initial=t.fp_state(pose=True)
    t.g.mem_write(UNIT+0x212,b'\xff')
    # Idle input here is not a supplied animation. C7E90 re-queries original
    # current/requested inventory state and produces E9D50's real request.
    rows=t.execute_fp(a.ticks,'idle')
    assert not t.setup_transitions
    calls=[c for c in t.fp_calls if c['entry']=='000e9d50']
    assert len(calls)==1 and calls[0]['phase']=='original_unit_wrapper'
    assert calls[0]['stack'][:4]==['00010001','07000101','00000000','00000000']
    assert rows[0]['after_unit']['hands'][0]['channel_identifier']=='001dffff'
    assert rows[0]['after_unit']['weapon']['blocking_ticks']==15
    result={'xbe_sha256':PIN,'cache_sha256':CACHE_PIN,'clock_dependency_sha256':CLOCK_PIN,
        'oracle_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        'scenario':'normal_wrapper_ready','authored':t.authored_fp(),'resources':t.fp_resources,
        'initial_registered_instant_loadout':initial,
        'supplied_admission_boundary':{'unit_offset':'212','before':0,'after':255,
            'scope':'Existing allocated equipped inventory and requested index0 retained; current index alone supplied NONE to exercise the original normal wrapper timed re-equip.'},
        'external_inputs':t.fp_external_inputs,'rows':rows,'calls':t.fp_calls,'writes':t.fp_stores,
        'executed_addresses':[f'{v:08x}'for v in sorted(t.fp_executed)],
        'projectile_requests':[e for e in t.events if e.get('service')=='000b8a30'],
        'local_rng_stores':[e for e in t.events if'local_rng_write_instruction'in e],
        'supplied_services':{f'{v:08x}':{'name':n,'return':r,'cleanup':s}for v,(n,r,s)in t.services.items()},
        'actual_first_person_clock_executed':True,'actual_first_person_decode_executed':True,
        'xbox_matrix_bit_parity_verified':False,'native_gameplay_update_applied':False,
        'scope':'Exact unchanged original normal unit wrapper produces its request, timed ready and registered FP selection. No supplied animation request or direct E9D50 invocation. The single current-index-NONE admission boundary, inherited world/heap/resource services and unavailable audio playback remain explicit. This is not the initial instant CE510 loadout.'}
    if a.instructions:result['instruction_inventory']=dump_instructions(t,a.instructions.resolve())
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({'path':str(a.output),'sha256':hashlib.sha256(a.output.read_bytes()).hexdigest(),
        'ticks':len(rows),'calls':len(t.fp_calls),'writes':len(t.fp_stores)}))

if __name__=='__main__':main()
