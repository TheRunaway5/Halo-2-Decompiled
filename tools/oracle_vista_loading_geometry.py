#!/usr/bin/env python3
"""Expanded unchanged-PE normal loading geometry corpus.

Uses the retained original command oracle, including source CRT pow and x87
trigonometry, actual constants caches, declaration63, viewport and draw bodies.
Platform/device service results are explicit fixtures. This does not prove
shader/raster precision, pixels, actual sweep progress or loader lifetime.
"""
from pathlib import Path
import hashlib
import importlib.util
import json
import random
import sys

ROOT=Path(__file__).resolve().parents[1]
SOURCE=ROOT/'analysis/frontend/vista-session-flow-audit/native-loading-presentation-original'
sys.path.insert(0,str(SOURCE))
spec=importlib.util.spec_from_file_location('original_loading_commands',SOURCE/'commands.py')
commands=importlib.util.module_from_spec(spec)
spec.loader.exec_module(commands)


def main():
    e=commands.PE(commands.EXE,commands.EXE_SHA)
    original=json.loads((SOURCE/'commands.json').read_text())
    ticks=[0,1,2,16,32,33,34,65,66,67,499,500,999,1000,1499,1500,1501,
        1999,2000,2001,2499,2500,2501,5499,5500,5501,6000,6001,6999,7000,
        7001,7499,7500,7501,14999,15000,15001,22499,22500,22501,29999,
        30000,30001,59999,60000,60001,0x7FFFFFFF,0x80000000,0xFFFFFFFF]
    ticks+=random.Random(0x66A512).sample(range(120000),100)
    cases=[]
    for width,height in [(640,480),(1280,720),(1280,800),(3440,1440),(3840,1080)]:
        for tick in ticks:
            row=commands.Original(e).run(elapsed=tick,width=width,height=height,progress=0.375)
            compact=[]
            for draw in row['draws']:
                compact.append(dict(kind=draw['kind'],shader=draw['shader'],
                    texture_handles=draw['texture_handles'],
                    states={k:v for k,v in draw['states'].items() if k in (19,20)},
                    constants=draw['constants'],vertices=[dict(words=v['words']) for v in draw['vertices']]))
            row['draws']=compact
            for key in ('events','spinner_args','fp_after','raw_eax','deadline','fade_gain','writes_to_sweep_progress'):
                row.pop(key,None)
            cases.append(row)
    report=dict(schema=original['schema'],source_sha256=commands.EXE_SHA,
        generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        command_oracle_sha256=hashlib.sha256((SOURCE/'commands.py').read_bytes()).hexdigest(),
        vertex_declaration=original['vertex_declaration'],boundary=__doc__,cases=cases)
    out=ROOT/'tests/fixtures/vista_loading_geometry_original.json'
    out.write_text(json.dumps(report,separators=(',',':'))+'\n')
    evidence=SOURCE/'geometry-original.json'
    evidence.write_text(json.dumps(dict(schema='H2V_LOADING_GEOMETRY_ORIGINAL_V1',
        source_sha256=commands.EXE_SHA,fixture=str(out.relative_to(ROOT)),
        fixture_sha256=hashlib.sha256(out.read_bytes()).hexdigest(),
        generator_sha256=report['generator_sha256'],command_oracle_sha256=report['command_oracle_sha256'],
        cases=len(cases),instructions=sum(c['instruction_visits'] for c in cases),
        draws=sum(len(c['draws']) for c in cases),boundary=__doc__),indent=2)+'\n')
    print(evidence.read_text())


if __name__=='__main__':main()
