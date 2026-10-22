#!/usr/bin/env python3
"""Select authenticated image PCs for physics-world trace and store observations.

The paired-image runtime retains canonical entry IDs and required-owner call
sites. Only observer PCs change. All generated arithmetic, memory addresses,
caller frames, branches and independent precision fixes remain byte-identical.
"""
import argparse
import hashlib
import json
import re
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT, Xbe, _Code, _trace, PIN
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN, full_tokens

BASELINE_SHA256='5fd09c3ef84366002472c4c1fe170c847c86af4072e4765c78ee3b2ded263840'
BEGIN='// BEGIN authenticated physics world observation addresses\n'
END='// END authenticated physics world observation addresses\n'
HELPER=BEGIN+'''// Preserve the existing canonical-observer API. Live TU1.5 composition opts
// into selected-image observations without changing any public caller ABI.
struct ObservedWorldState : physics_world_runtime_detail::WorldState {
    using physics_world_runtime_detail::WorldState::WorldState;
    bool selected_observation_pcs{};
};
std::uint32_t world_observation_pc(std::uint32_t disc,std::uint32_t tu15,
    const ObservedWorldState &state) {
    return state.selected_observation_pcs && state.image==SourcePhysicsContactImage::TitleUpdate15?tu15:disc;
}
'''+END
WRAPPER_BEGIN='// BEGIN physics world observation API wrappers\n'
WRAPPER_END='// END physics world observation API wrappers\n'
WRAPPERS=WRAPPER_BEGIN+'''void source_run_physics_world_runtime(M &m,std::uint32_t entry,
    SourcePhysicsContactRuntimeAbi &abi,SourcePhysicsContactImage image,FP fp,
    const SourcePhysicsContactRuntimeServices &services,const O &observe) {
    source_run_physics_world_runtime_impl(m,entry,abi,image,fp,services,observe,false);
}
void source_run_physics_world_runtime_selected_observations(M &m,std::uint32_t entry,
    SourcePhysicsContactRuntimeAbi &abi,SourcePhysicsContactImage image,FP fp,
    const SourcePhysicsContactRuntimeServices &services,const O &observe) {
    source_run_physics_world_runtime_impl(m,entry,abi,image,fp,services,observe,true);
}
'''+WRAPPER_END
OBSERVATION=re.compile(r'(s\.services\.instruction\(|(?:w|vw)\(s\.memory,s\.observe,|s\.push\()0x([0-9a-f]+)u')

def sha(path):
    with path.open('rb') as source:return hashlib.file_digest(source,'sha256').hexdigest()

def canonical(source):
    source=re.sub(re.escape(BEGIN)+r'.*?'+re.escape(END),'',source,flags=re.S)
    source=re.sub(re.escape(WRAPPER_BEGIN)+r'.*?'+re.escape(WRAPPER_END),'',source,flags=re.S)
    source=source.replace('static void source_run_physics_world_runtime_impl(', 'void source_run_physics_world_runtime(')
    source=source.replace('const O &observe,bool selected_observation_pcs)', 'const O &observe)')
    source=source.replace('s.load(abi);s.selected_observation_pcs=selected_observation_pcs;', 's.load(abi);')
    source=source.replace('ObservedWorldState','WorldState')
    return re.sub(r'world_observation_pc\(0x([0-9a-f]+)u,0x[0-9a-f]+u,s\)',r'0x\1u',source)

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source',type=Path,default=ROOT/'src/physics/runtime/physics_world_runtime_generated.cpp')
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--manifest',type=Path,required=True)
    args=parser.parse_args();source=canonical(args.source.read_text())
    assert hashlib.sha256(source.encode()).hexdigest()==BASELINE_SHA256,'Physics source changed; retain and re-audit its independent edits'
    old_path=ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'
    assert sha(old_path)==PIN and sha(NEW_DEFAULT)==NEW_PIN
    old,new=Xbe(old_path),Xbe(NEW_DEFAULT);oc,nc=_Code(old,'.text'),_Code(new,'.text')
    selected=re.search(r'std::uint32_t selected\(.*?\n',source).group()
    addresses={int(p,16):int(q,16) for p,q in re.findall(r'case 0x([0-9a-f]+)u:return 0x([0-9a-f]+)u;',selected)}
    bodies={int(p,16):body for p,body in re.findall(r'(?:static )?void run_([0-9a-f]+)\(WorldState &s\) \{\n(.*?)\n\}',source,re.S)}
    declared={int(p,16) for p in re.findall(r'static void run_([0-9a-f]+)\(WorldState &\);',source)}
    assert set(bodies)==declared and len(bodies)==477
    targets={addresses[p] for p in bodies};mapping={}
    for entry,body in bodies.items():
        sites=sorted({int(p,16) for p in re.findall(r's\.services\.instruction\(0x([0-9a-f]+)u\)',body)})
        assert sites==sorted(_trace(oc,entry,set(bodies)).code),hex(entry)
        actual=sorted(_trace(nc,addresses[entry],targets).code)
        assert len(sites)==len(actual) and full_tokens([oc.at(p) for p in sites],old)==full_tokens([nc.at(p) for p in actual],new),hex(entry)
        for p,q in zip(sites,actual):
            assert p not in mapping or mapping[p]==q
            mapping[p]=q
    count=0
    def replace(match):
        nonlocal count
        p=int(match[2],16);q=mapping[p];count+=1
        return match[1]+f'world_observation_pc(0x{p:x}u,0x{q:x}u,s)'
    output=OBSERVATION.sub(replace,source)
    output=re.sub(r'\bWorldState\b','ObservedWorldState',output)
    anchor='using namespace physics_world_runtime_detail;\n'
    assert output.count(anchor)==1
    output=output.replace(anchor,anchor+HELPER)
    output=output.replace('void source_run_physics_world_runtime(', 'static void source_run_physics_world_runtime_impl(')
    output=output.replace('const O &observe) {', 'const O &observe,bool selected_observation_pcs) {')
    output=output.replace('s.load(abi);', 's.load(abi);s.selected_observation_pcs=selected_observation_pcs;')
    output=output.replace('} // namespace halo2\n',WRAPPERS+'} // namespace halo2\n')
    assert canonical(output)==source
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(output)
    report=dict(scope=__doc__,source_sha256=sha(args.source),canonical_baseline_sha256=BASELINE_SHA256,
        generated_cpp_sha256=sha(args.output),tool_sha256=sha(Path(__file__)),original_map_pack_sha256=PIN,
        original_tu15_sha256=NEW_PIN,whole_routines_authenticated=len(bodies),static_instruction_sites=len(mapping),
        observations_selected=count,arithmetic_memory_and_owner_call_ABI_byte_identical=True,
        canonical_observation_API_preserved=True,selected_image_observation_API_explicit=True,
        instruction_map={f'{p:08x}':q for p,q in sorted(mapping.items())},full_weapon_parity=False)
    args.manifest.parent.mkdir(parents=True,exist_ok=True);args.manifest.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps({k:v for k,v in report.items() if k not in ('instruction_map','scope')}))

if __name__=='__main__':main()
