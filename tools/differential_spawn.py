#!/usr/bin/env python3
"""Compare native score and chooser math with bounded original spawn traces.

Influence gathering/collision services are supplied inputs in this comparison.
"""
import argparse
import ctypes as c
import hashlib
import json
from pathlib import Path
import struct
from differential_weapon_error import Vector
ROOT = Path(__file__).resolve().parents[1]
class Influence(c.Structure):
    _fields_ = [('position', Vector)] + [(k, c.c_float) for k in ('inner','outer','upper','lower','weight')]
def digest(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def bits(v): return f'{struct.unpack("<I",struct.pack("<f",v))[0]:08x}'
def influences(rows):
    return (Influence*len(rows))(*(Influence(Vector(*x[:3]),*x[3:]) for x in rows))
def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--library',type=Path,default=ROOT/'build-multiplayer/libhalo2_weapon_kernels.so')
    p.add_argument('--report',type=Path,default=ROOT/'analysis/multiplayer/integration/spawn-source/report.json')
    p.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer/integration/spawn-differential.json')
    p.add_argument('--export-map-fixtures',type=Path,
                   help='Write original map-dependent comparison data outside version control')
    args=p.parse_args(); report=json.loads(args.report.read_text())
    if not report['passed']: raise ValueError('Original spawn oracle failed')
    lib=c.CDLL(str(args.library.resolve()))
    score=lib.halo2_spawn_influence_score
    score.argtypes=[c.POINTER(Vector),c.POINTER(Influence),c.c_uint];score.restype=c.c_float
    candidate=lib.halo2_spawn_candidate_score
    candidate.argtypes=[c.c_float,c.c_float,c.POINTER(c.c_uint32)];candidate.restype=c.c_float
    seed_for=lib.halo2_spawn_private_seed
    seed_for.argtypes=[c.c_uint32]*3+[c.c_int16];seed_for.restype=c.c_uint32
    choose=lib.halo2_choose_spawn
    choose.argtypes=[c.POINTER(Vector),c.c_uint,c.POINTER(Influence),c.c_uint,c.c_float,
                    c.POINTER(c.c_uint32),c.POINTER(c.c_uint8),c.POINTER(c.c_float)]
    choose.restype=c.c_int
    kernels=0; candidates=0
    for version,rows in report['kernels'].items():
        for row in rows:
            point=Vector(*row['point']); data=influences(row['influences'])
            actual=score(c.byref(point),data,len(data))
            if bits(actual)!=row['bits']: raise AssertionError((version,kernels,'score',bits(actual),row['bits']))
            kernels+=1
    maps={x['map']:x for x in report['maps']}
    for row in report['rows']:
        state=row['input_state']; source=maps[row['map']]
        if digest(ROOT/source['path'])!=source['sha256']: raise ValueError('Source map differs')
        points=row.get('fixture_overrides',{}).get('starts',source['start_positions'])
        starts=(Vector*len(points))(*(Vector(*x) for x in points))
        data=influences(row['influences'])
        seed=seed_for(int(state['spawning_player_datum'],16),int(state['options_10'],16),
                      int(state['options_14'],16),state['spawn_count'])
        if seed!=int(row['private_initial_seed'],16): raise AssertionError(('private seed',row['case']))
        initial=seed if row['deterministic'] else int(row['global_initial_seed'],16)
        running=c.c_uint32(initial)
        for expected,point in zip(row['scores'],starts):
            base=score(c.byref(point),data,len(data))
            actual=candidate(base,state['maximum_random_bias'],c.byref(running))
            target=expected['private_seed'] if row['deterministic'] else expected['global_spawn_seed']
            if bits(actual)!=expected['bits'] or running.value!=int(target,16):
                raise AssertionError((row['map'],row['case'],expected['index'],'candidate',bits(actual),expected['bits']))
            candidates+=1
        running=c.c_uint32(initial); allowed=(c.c_uint8*len(points))(*(int(i not in row['blocked_indices']) for i in range(len(points))))
        selected_score=c.c_float()
        selected=choose(starts,len(points),data,len(data),state['maximum_random_bias'],c.byref(running),allowed,c.byref(selected_score))
        final=row['final_seed'] if row['deterministic'] else row['global_final_seed']
        if selected!=row['selected'] or running.value!=int(final,16):
            raise AssertionError((row['map'],row['case'],'choice',selected,row['selected']))
        if selected>=0 and bits(selected_score.value)!=row['selected_score_bits']:
            raise AssertionError((row['map'],row['case'],'selected score'))
    args.output.write_text(json.dumps(dict(passed=True,kernel_cases=kernels,chooser_cases=len(report['rows']),
        candidates=candidates,native_library_sha256=digest(args.library),original_report_sha256=digest(args.report),
        tool_sha256=digest(Path(__file__)),scope='Source scoring, private/global RNG, candidate order, strict winner and supplied collision acceptance. Influence gather/world services remain separate.'),indent=2)+'\n')
    if args.export_map_fixtures:
        if ROOT/'analysis' not in args.export_map_fixtures.resolve().parents:
            raise ValueError('Extracted authored map fixtures must remain in ignored analysis')
        lines=['# Original bounded empty-world initial spawn comparisons; raw authored data remains local.']
        for row in report['rows']:
            if row['case']!='empty_initial': continue
            s=row['input_state']; source=maps[row['map']]
            values=s['spawn_settings']+[v for influence in row['influences'] for v in influence]
            lines.append(' '.join(map(str,[source['path'],s['options_10'],s['options_14'],row['selected'],
                row['influence_count']]+[bits(x) for x in values])))
        args.export_map_fixtures.write_text('\n'.join(lines)+'\n')
    print(f'Matched {kernels} score cases, {len(report["rows"])} choosers and {candidates} candidate states')
if __name__=='__main__': main()
