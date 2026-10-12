#!/usr/bin/env python3
"""Unchanged scalar37 dispatch on declared projectile parent records.

Object/header/render-context records are explicit ABI inputs, not proof of
original projectile birth or parent publication. No instruction patches or
supplied lower results. Compare the free type5 branch against an attached
type5 control to distinguish it from the shooter's Unit camouflage.
"""
import hashlib,json
from pathlib import Path
from oracle_camouflage_render_context import Context,HudCache,ROOT,bits

def main():
    cache=HudCache(ROOT/'assets/disc/maps/lockout.map')
    inputs=[dict(kind='free_projectile',datum=0x8aaa0000,
        objects=[dict(type=5,parent=0xffffffff,primary_bits=bits(.625))]),
        dict(kind='attached_projectile_control',datum=0x8aaa0000,
        objects=[dict(type=5,parent=0x8aaa0001),dict(type=0,parent=0xffffffff,primary_bits=bits(.625))])]
    versions={}
    for name in ('retail','tu11','u5'):
        source=Context(name,cache)
        results=[source.render(row)for row in inputs]
        assert results[0]['scalar_bits']==0 and results[0]['primary_read_slots']==[]
        assert results[1]['scalar_bits']==bits(.625) and results[1]['primary_read_slots']==[1]
        versions[name]=dict(image_sha256=source.spec['pin'],owners=source.d,results=results)
    dependencies={}
    import sys
    for module in list(sys.modules.values()):
        name=getattr(module,'__file__',None)
        if name:
            path=Path(name).resolve()
            if path.is_relative_to(ROOT/'tools')and path.suffix=='.py':
                dependencies[str(path.relative_to(ROOT))]=hashlib.sha256(path.read_bytes()).hexdigest()
    report=dict(scope=__doc__,inputs=inputs,versions=versions,dependency_sha256=dependencies,
        instruction_patches=[],supplied_lower_results=[])
    path=ROOT/'analysis/multiplayer/integration/free-projectile-camouflage-20261011.json'
    path.parent.mkdir(parents=True,exist_ok=True);path.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print('6 original scalar37 cases exact across retail/TU1.1/TU1.5; free projectiles select zero without a Unit read')
if __name__=='__main__':main()
