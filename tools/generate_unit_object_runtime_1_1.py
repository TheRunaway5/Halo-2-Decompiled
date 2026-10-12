#!/usr/bin/env python3
"""Compile the complete TU1.1 object/Unit birth callers and storage owners.

The linked runtime shares the real retained ABI with the existing animation
graph. No allocation, resource, pose, physics or world result is supplied.
"""
import argparse
import json
from pathlib import Path
import generate_unit_animation_selection_runtime_1_1 as compiler
from generate_unit_weapon_routing_1_1 import ENTRIES as WEAPON_ENTRIES

ENTRIES = (
    0xb8820, 0xbbcb0, 0xb8a30, 0x10ca30, 0x10cbd0, 0x10cc20,
    0xc0890, 0xbd170, 0xbd270, 0xbd1f0, 0x16d2c0, 0x16d4c0,
    0x18fce0, 0x18fd70, 0x18fe60, 0x18ff20, 0x18ff90,
    0x190030, 0x1900c0, 0x326970,
    0xddac0, 0xddb40, 0xdfaf0, 0xc52c0, 0x1ce8c0, 0x1ce9b0,
    0x1ceac0, 0x1cea90, 0x1d0a60, 0x1d0590, 0x1d06c0,
    0x1cacf0, 0xe7e30, 0x110a20, 0x10f380, 0x1100e0,
    0x110800, 0x115030, 0x16f510, 0x116680, 0x110bf0,
    0x114f00, 0x114f70, 0x148360, 0x148510, 0x148740,
    0x22ecb0, 0x1c7290, 0x1c7050, 0x1c6f50, 0x1c72e0,
    0xbbe70, 0xc0d10, 0x1eff60, 0x1e0aa0, 0xe6f60, 0xe6dc0, 0xe6fb0, 0x1e8e60, 0x1f3da0,
    0xbb430, 0x16ec00, 0xbb480, 0xbb580, 0xbf130, 0x16ed00, 0x16f550, 0x16f760,
    0xbb5e0, 0xbb6e0, 0x16f0e0, 0x17cc40, 0x1c8e10, 0x1c70d0, 0xd7160, 0xbf7d0, 0xbba30,
    0x3dd00, 0xbb2c0, 0xbdf10, 0xbdf80, 0xbede0, 0x16f3c0,
    0xc0490, 0xc0b80, 0xc0d30, 0xc0d80, 0xc0dc0, 0xb7670, 0xb9ac0,
)
ANIMATION_ENTRIES = compiler.ENTRIES
compiler.ENTRIES = tuple(sorted(set(ENTRIES)))
compiler.RUNTIME_HEADER = 'halo2/unit_object_runtime.hpp'
compiler.RUNTIME_FUNCTION = 'source_run_unit_object_graph_1_1'
compiler.CONFIGURATION_TOOL = __file__
compiler.EXTRA_TABLES = {0x114ee4: 3, 0x1e8f2c: 6, 0x1e8f44: 6}

if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--manifest', type=Path, required=True)
    args, _ = parser.parse_known_args()
    compiler.main()
    path = args.output.with_name('unit_object_runtime.cpp')
    path.write_text('''// Generated link ownership; complete bodies live in their existing modules.
#include "halo2/unit_object_runtime.hpp"
#include "halo2/unit_weapon_routing.hpp"
#include <algorithm>
#include <stdexcept>
namespace halo2 {
namespace {
constexpr std::uint32_t construction_entries[]={'''+','.join(hex(e)+'u' for e in sorted(set(ENTRIES)))+'''};
constexpr std::uint32_t animation_entries[]={'''+','.join(hex(e)+'u' for e in sorted(set(ANIMATION_ENTRIES)))+'''};
constexpr std::uint32_t weapon_entries[]={'''+','.join(hex(e)+'u' for e in sorted(set(WEAPON_ENTRIES)))+'''};
}
void source_run_unit_object_runtime_1_1(SourcePhysicsContactMemory &memory,
    std::uint32_t entry, SourceUnitAnimationRuntimeAbi &abi,
    SourcePhysicsFactoryFloatingPoint fp,
    const SourceUnitAnimationRuntimeServices &services,
    const SourcePhysicsContactRecordObserver &observe) {
    auto linked=services;
    auto run_weapons=[&](auto &m,auto target,auto &state,const auto &o) {
        // These complete weapon bodies contain no MMX operations. Retain
        // the live MMX state across their external animation/pose callbacks.
        auto mmx=state.mmx;
        SourcePhysicsContactRuntimeServices weapons;
        weapons.instruction=services.instruction;
        weapons.required_owner=[&](auto &bank,const auto &call,auto &base,const auto &observer) {
            SourceUnitAnimationRuntimeAbi nested;
            static_cast<SourcePhysicsContactRuntimeAbi &>(nested)=base;nested.mmx=mmx;
            try {linked.required_owner(bank,call,nested,observer);}
            catch(...) {base=static_cast<const SourcePhysicsContactRuntimeAbi &>(nested);mmx=nested.mmx;throw;}
            base=static_cast<const SourcePhysicsContactRuntimeAbi &>(nested);mmx=nested.mmx;
        };
        try {source_run_unit_weapon_routing_1_1(m,target,state,state.floating_point.value_or(fp),weapons,o);}
        catch(...) {state.mmx=mmx;throw;}
        state.mmx=mmx;
    };
    linked.required_owner=[&](auto &m,const auto &call,auto &state,const auto &o) {
        const auto controls=state.floating_point.value_or(fp);
        if(std::binary_search(std::begin(construction_entries),std::end(construction_entries),call.target))
            source_run_unit_object_graph_1_1(m,call.target,state,controls,linked,o);
        else if(std::binary_search(std::begin(animation_entries),std::end(animation_entries),call.target))
            source_run_unit_animation_selection_1_1(m,call.target,state,controls,linked,o);
        else if(std::binary_search(std::begin(weapon_entries),std::end(weapon_entries),call.target))
            run_weapons(m,call.target,state,o);
        else {
            if(!services.required_owner)throw std::runtime_error("Required original Unit construction callee unavailable");
            services.required_owner(m,call,state,o);
        }
    };
    const auto controls=abi.floating_point.value_or(fp);
    if(std::binary_search(std::begin(construction_entries),std::end(construction_entries),entry))
        source_run_unit_object_graph_1_1(memory,entry,abi,controls,linked,observe);
    else if(std::binary_search(std::begin(animation_entries),std::end(animation_entries),entry))
        source_run_unit_animation_selection_1_1(memory,entry,abi,controls,linked,observe);
    else if(std::binary_search(std::begin(weapon_entries),std::end(weapon_entries),entry))
        run_weapons(memory,entry,abi,observe);
    else throw std::runtime_error("Required original Unit construction entry unavailable");
}
} // namespace halo2
''')
    report = json.loads(args.manifest.read_text())
    report.update(linked_cpp_sha256=compiler.sha(path),
                  animation_ownership_generator_sha256=compiler.sha(compiler.__file__),
                  weapon_ownership_generator_sha256=compiler.sha(Path(__file__).with_name('generate_unit_weapon_routing_1_1.py')),
                  linked_animation_entries=len(set(ANIMATION_ENTRIES)),
                  linked_weapon_entries=len(set(WEAPON_ENTRIES)))
    args.manifest.write_text(json.dumps(report, indent=2, sort_keys=True)+'\n')
    print(json.dumps(dict(final_manifest_sha256=compiler.sha(args.manifest),
                         linked_cpp_sha256=compiler.sha(path))))
