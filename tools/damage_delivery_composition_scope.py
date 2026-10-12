"""Extend original TU1.1 capture through already linked native lower modules.

No native outcomes or instructions are executed by the capture. These emitted
site markers only delimit the scope; unchanged original XBE instructions supply
all CPU and memory outcomes. The full composed native replay verifies the scope
against its actual dispatch and compares every executed site and store.
"""
import copy,re
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code,PIN
from generate_physics_world_bootstrap_runtime_20261011 import sha

MODULES=(
    'src/unit_weapon_routing.cpp',
    'src/unit_object_runtime_generated.cpp',
    'src/physics_vehicle_motion_generated.cpp',
)

def extend(paired):
    paired=copy.deepcopy(paired);records=[]
    path=ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'
    assert sha(path)==PIN
    code=_Code(Xbe(path),'.text')
    for name in MODULES:
        path=ROOT/name
        sites=sorted({int(p,16) for p in re.findall(r'services\.instruction\(0x([0-9a-f]+)u\)',path.read_text())})
        assert sites,name
        for p in sites:
            assert code.at(p).address==p
            # Only TU1.1 is captured in this composition extension.
            paired['instruction_map'].setdefault(f'{p:08x}',p)
        records.append(dict(path=name,sha256=sha(path),instruction_sites=len(sites)))
    paired['native_composition_scope']=records
    return paired
