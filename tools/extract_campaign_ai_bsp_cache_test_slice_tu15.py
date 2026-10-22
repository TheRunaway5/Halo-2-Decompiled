#!/usr/bin/env python3
"""Compile exact BSP-cache, camera, geometry and behavior-iterator test bodies.

This extracts whole bodies without rewriting arithmetic, memory operations or
calls. Production continues to use its existing PhysicsLiveSupport and sound
owners. The focused test avoids compiling their unrelated, very large closure.
Only generated build-directory output is allowed.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re

ENTRIES = {
    'src/physics/physics_live_support_tu15_generated.cpp': (0x203c00, 0x252f30, 0x253070, 0x1578a0),
    'src/audio/source_sound_listener_selection_tu15_generated.cpp': (0x2531e0,),
    'src/rendering/renderer/source_scene_device_constructor_tu15_generated.cpp': (0x218fe0, 0x219030, 0x278dd0),
}


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--source', type=Path, required=True)
    ap.add_argument('--output', type=Path, required=True)
    ap.add_argument('--build-root', type=Path, required=True)
    args = ap.parse_args()
    source, output, build = args.source.resolve(), args.output.resolve(), args.build_root.resolve()
    assert build != source and output.is_relative_to(build)
    scratch = os.environ.get('HALO2_TEST_SCRATCH')
    if scratch:
        assert build.is_relative_to(Path(scratch).resolve())
    bodies = []; helpers = []; address_helpers = []
    for name, entries in ENTRIES.items():
        raw = (source/name).read_bytes(); text = raw.decode()
        marker = 'std::uint32_t sign_extend(std::uint32_t x,unsigned n) {'
        begin = text.index(marker); end = text.index('\n}',begin)+2
        helpers.append(text[begin:end])
        if name == 'src/rendering/renderer/source_scene_device_constructor_tu15_generated.cpp':
            # These generated bodies retain their original image-address
            # resolver. Copy it exactly, rather than replacing it with an
            # identity mapping or altering any body operands.
            marker = 'std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage image)'
            begin = text.index(marker);end = text.index('\n',begin)
            address_helpers.append(text[begin:end])
        for entry in entries:
            marker = f'static void run_{entry:x}(WorldState &s) {{'
            assert text.count(marker) == 1
            begin = text.index(marker); end = text.index('\n}', begin)+2
            body = text[begin:end]
            assert 'Original solver fell outside authenticated CFG' in body
            bodies.append((entry, body))
    entries = {entry for entry, _ in bodies}
    for _, body in bodies:
        assert {int(p,16) for p in re.findall(r'run_([0-9a-f]+)\(s\)',body)} <= entries
        assert {int(p,16) for p in re.findall(r'const auto target=0x([0-9a-f]+)u',body)} <= entries
    header = '''// Test-only exact production-body slice; never linked into the game.
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "halo2/source_runtime_memory.hpp"
namespace halo2 {
namespace {
using namespace physics_world_runtime_detail;
'''
    assert len(set(helpers)) == 1
    assert len(address_helpers) == 1
    # Exercise the existing native forward-table provider together with final
    # startup storage. This catches duplicate ownership that isolated data
    # tests cannot detect. Both the byte encoder and mount call are exact
    # production source, not replacement table bytes in a fixture.
    world = (source/'src/platform/native_tu15_world_memory.cpp').read_text()
    marker = 'template<std::size_t N>auto encode(const std::array<std::uint32_t,N>& words) {'
    begin = world.index(marker);end = world.index('\n}',begin)+2
    forward_encoder = world[begin:end]
    marker = 'memory.mount(0x44ac8cu,encode(std::array{'
    begin = world.index(marker);end = world.index(');',begin)+2
    forward_publication = world[begin:end]
    declarations = helpers[0]+'\n'+address_helpers[0]+'\n'+'\n'.join(f'static void run_{entry:x}(WorldState &);' for entry,_ in bodies)
    declarations += '\n'+forward_encoder+'\n'
    dispatch = '''
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail) {
    if(target==0x2531e0u){run_2531e0(s);return;}
    s.required(site,target,tail);
}
'''
    footer = '''
} // namespace
void source_run_campaign_ai_bsp_cache_test_slice_tu15(M &memory,
    SourcePhysicsContactRuntimeAbi &abi,FP fp,
    const SourcePhysicsContactRuntimeServices &services,const O &observe) {
    FpScope scope(fp);
    WorldState s{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};s.load(abi);
    try {run_203c00(s);} catch(...) {abi=s.save();throw;}
    abi=s.save();
}
} // namespace halo2
'''
    footer=footer.replace('} // namespace halo2','''void source_run_campaign_camera_effect_request_test_slice_tu15(M &memory,
    SourcePhysicsContactRuntimeAbi &abi,FP fp,
    const SourcePhysicsContactRuntimeServices &services,const O &observe) {
    FpScope scope(fp);
    WorldState s{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};s.load(abi);
    try {run_1578a0(s);} catch(...) {abi=s.save();throw;}
    abi=s.save();
}
} // namespace halo2''')
    footer=footer.replace('} // namespace halo2','''void mount_campaign_existing_projection_test_slice_tu15(SourceRuntimeMemory &memory) {
'''+forward_publication+'''
}
} // namespace halo2''')
    footer=footer.replace('} // namespace halo2','''void source_run_campaign_geometry_permutation_test_slice_tu15(M &memory,std::uint32_t entry,
    SourcePhysicsContactRuntimeAbi &abi,FP fp,
    const SourcePhysicsContactRuntimeServices &services,const O &observe) {
    FpScope scope(fp);
    WorldState s{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};s.load(abi);
    try {
        if(entry==0x218fe0u)run_218fe0(s);
        else if(entry==0x219030u)run_219030(s);
        else throw std::invalid_argument("Unknown original geometry permutation entry");
    } catch(...) {abi=s.save();throw;}
    abi=s.save();
}
} // namespace halo2''')
    footer=footer.replace('} // namespace halo2','''void source_run_campaign_behavior_iterator_test_slice_tu15(M &memory,
    SourcePhysicsContactRuntimeAbi &abi,FP fp,
    const SourcePhysicsContactRuntimeServices &services,const O &observe) {
    FpScope scope(fp);
    WorldState s{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};s.load(abi);
    try {run_278dd0(s);} catch(...) {abi=s.save();throw;}
    abi=s.save();
}
} // namespace halo2''')
    output.parent.mkdir(parents=True,exist_ok=True)
    output.write_text(header+declarations+dispatch+'\n'.join(body for _,body in bodies)+footer)
    print(json.dumps(dict(exact_production_bodies=[f'{entry:08x}' for entry,_ in bodies],
        body_sha256={f'{entry:08x}':hashlib.sha256(body.encode()).hexdigest() for entry,body in bodies},
        exact_address_mapper_sha256=hashlib.sha256(address_helpers[0].encode()).hexdigest(),
        exact_forward_provider_sha256=hashlib.sha256(forward_publication.encode()).hexdigest(),
        exact_forward_encoder_sha256=hashlib.sha256(forward_encoder.encode()).hexdigest(),
        output_sha256=hashlib.sha256(output.read_bytes()).hexdigest(),production_sources_modified=False)))


if __name__ == '__main__': main()
