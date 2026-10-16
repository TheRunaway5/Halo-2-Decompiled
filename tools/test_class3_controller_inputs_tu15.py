#!/usr/bin/env python3
"""Compare declared neutral/commanded input in real TU1.5 campaign physics worlds."""
import argparse, concurrent.futures, hashlib, json, math, os, subprocess
from pathlib import Path

CASES={
    'enforcer':('06a_sentinelwalls',68,0),
    'gravity_throne':('05b_deltatowers',0,0),
    'gravity_throne_no_flare':('00a_introduction',10,0),
    'phantom':('06a_sentinelwalls',2,0),
    'pelican':('03b_newmombasa',3,0),
}
PIN='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
def sha(p):
    with Path(p).open('rb') as stream:return hashlib.file_digest(stream,'sha256').hexdigest()

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--runtime',required=True,type=Path)
    p.add_argument('--xbe',required=True,type=Path)
    p.add_argument('--maps',required=True,type=Path)
    p.add_argument('--output',required=True,type=Path)
    p.add_argument('--vehicle',choices=['all',*CASES],default='all')
    a=p.parse_args();a.runtime=a.runtime.resolve();a.xbe=a.xbe.resolve();a.maps=a.maps.resolve();a.output=a.output.resolve()
    assert sha(a.xbe)==PIN
    a.output.mkdir(parents=True,exist_ok=False);before=sha(a.runtime)
    def run_case(name):
        map_name,placement,bsp=CASES[name];map_path=a.maps/(map_name+'.map')
        output=a.output/name;output.mkdir()
        env={k:v for k,v in os.environ.items() if not k.startswith('HALO2_TU15_CAMPAIGN_')}
        env.update(HALO2_TU15_CAMPAIGN_PLACEMENT=str(placement),HALO2_TU15_CAMPAIGN_BSP=str(bsp),
            HALO2_TU15_CAMPAIGN_OBSERVE_VEHICLE='1',HALO2_TU15_CAMPAIGN_UNIT_INPUT='1',
            HALO2_TU15_CAMPAIGN_NATIVE_UNIT_CONTROLLER='1')
        results={};exits={};commands={}
        for mode in ['commanded','neutral']:
            case_env=dict(env)
            if mode=='neutral':case_env['HALO2_TU15_CAMPAIGN_UNIT_INPUT_NEUTRAL']='1'
            command=[str(a.runtime),str(a.xbe),str(map_path),str(output/mode)];commands[mode]=command
            with (output/(mode+'.log')).open('w') as log:
                exits[mode]=subprocess.run(command,env=case_env,stdout=log,stderr=subprocess.STDOUT,timeout=180).returncode
            report=output/mode/'report.json'
            results[mode]=json.loads(report.read_text()) if report.exists() else {}
        x=results['commanded'].get('steps',[]);y=results['neutral'].get('steps',[])
        prefix=len(x)==len(y)==90 and x[:30]==y[:30]
        deltas=[math.dist(v['position'],w['position']) for v,w in zip(x,y)]
        response=max(deltas,default=0)
        admission=results['commanded'].get('unit_control_admission',[])
        consumed=any(r['entry']==0xf48c0 and r['entering'] and r['throttle']==[1,0,.5] for r in admission)
        passed=all(v==0 for v in exits.values()) and prefix and consumed and response>0
        verdict=dict(vehicle=name,passed=passed,commands=commands,exit_codes=exits,
            map_sha256=sha(map_path),placement=placement,bsp=bsp,
            exact_neutral_prefix_ticks=30 if prefix else 0,
            maximum_commanded_neutral_position_difference=response,
            input_consumed_by_original_class3_controller=consumed,
            position_differences_by_tick=deltas,
            errors={k:v.get('error') for k,v in results.items()},
            boundary='Declared initial cached-BSP observer starts and controller inputs. Original CE440 admits the controller; original C93D0/C92C0 deliver its input. Full retained source frames produce all motion. No supplied physical flags, contact, force, velocity, solver result or post-step position. This is not campaign AI, visible multiplayer or complete physics parity.')
        (output/'comparison.json').write_text(json.dumps(verdict,indent=2)+'\n');return verdict
    names=list(CASES) if a.vehicle=='all' else [a.vehicle]
    with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:results=list(pool.map(run_case,names))
    stable=sha(a.runtime)==before
    report=dict(reference='English TU1.5 only',passed=stable and all(r['passed'] for r in results),
        runtime=str(a.runtime),runtime_sha256=before,runtime_stable=stable,xbe_sha256=PIN,
        cases=results,full_physics_parity=False)
    (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items() if k!='cases'}))
    for r in results:print(r['vehicle'],'PASS' if r['passed'] else 'RED',r['maximum_commanded_neutral_position_difference'])
    return 0 if report['passed'] else 1
if __name__=='__main__':raise SystemExit(main())
