#!/usr/bin/env python3
"""Prove the native TU1.5 object tick children (BE850/BE610/C0F60/10DD80/C0200/1D5790/1E15D0/C0910).

Run through tools/run_test.py. Extracts the eight unmodified translated
bodies from the checked-in live-support unit (pinned) and the verification
references the production binder generates, compiles
tests/native_tu15_object_subtree_tests.cpp against them, and compares every
seeded case (ordered store events with preimages, child call log, bank,
registers, flags, XMM, MXCSR, cursor; faults by exception, cursor and store prefix)
in Release, census and verify modes and an ASan/UBSan build. Requires
complete reachable branch coverage. Checks the production binding (install,
idempotence, refusal of changed bodies, missing children and changed routes,
references without an unrouted child). Then screens deliberate defects: each
mutant is an out-of-tree copy of the native header placed first on the
include path and must be rejected (default mode, then verify mode, whose
replay also orders every read). Source hashes are rechecked after the
screen.
"""
import argparse
import concurrent.futures
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]
UNIT = ROOT / 'src/physics/physics_live_support_tu15_generated.cpp'
HEADER = ROOT / 'src/physics/runtime/native_tu15_object_subtree.hpp'
SYSTEMS = ROOT / 'src/physics/runtime/native_tu15_object_subtree_systems.hpp'
MACHINE = ROOT / 'src/physics/runtime/native_tu15_object_tick.hpp'
MACHINE_SYSTEMS = ROOT / 'src/physics/runtime/native_tu15_object_tick_systems.hpp'
TEST = ROOT / 'tests/native_tu15_object_subtree_tests.cpp'
HELPER = ROOT / 'tools/native_object_subtree_replacements.py'
OWNERS = ('be850', 'be610', 'c0f60', '10dd80', 'c0200', '1d5790', '1e15d0', 'c0910')
UNREACHABLE = set()
# (name, file, anchor, replacement). The logic lives in SYSTEMS and the ABI
# in the HEADER adapter; each mutant is an out-of-tree copy of its file.
MUTANTS = [
    # BE850
    ('move-frame', HEADER, 'gp[esp]-=0x20u;this->push(0xbe85cu,gp[ebp]);', 'gp[esp]-=0x1cu;this->push(0xbe85cu,gp[ebp]);'),
    ('move-owner-flags', HEADER, 'case cluster:this->compare32(v,logic::none);break;', 'case cluster:this->test32(v);break;'),
    ('move-shift', HEADER, 'gp[eax]=1u<<(gp[ecx]&31u);', 'gp[eax]=1u<<(gp[ecx]&15u);'),
    ('move-machine-type', SYSTEMS, 'return (type&31u)==7u;', 'return (type&15u)==7u;'),
    ('move-required-site', HEADER, 'this->call_routed(required,0xbe892u,0xbe897u,0xbf0c0u);', 'this->call_routed(required,0xbe941u,0xbe897u,0xbf0c0u);'),
    # The node count plus 0x1F is always positive (SAR equals SHR there) and
    # the byte count is a multiple of four (the REP STOSB count is always
    # zero), so those forms have no defect; the screen mutates the live count
    # arithmetic.
    ('move-clear-round', SYSTEMS, 'return std::uint32_t(signed_value(nodes+0x1fu)>>5)<<2;', 'return std::uint32_t(signed_value(nodes+0x1eu)>>5)<<2;'),
    ('move-clear-direction', HEADER, 'gp[edi]+=down?0xfffffffcu:4u;', 'gp[edi]+=4u;'),
    ('move-clear-dwords', HEADER, 'gp[edx]=bytes;gp[ecx]=bytes>>2;', 'gp[edx]=bytes;gp[ecx]=bytes>>3;'),
    ('move-clear-arg', HEADER, 'gp[eax]=gp[esp]+0x14u;this->push(0xbe90au,gp[eax]);', 'gp[eax]=gp[esp]+0x10u;this->push(0xbe90au,gp[eax]);'),
    ('move-handle-reload', HEADER, 'gp[edi]=this->read(0xbe911u,gp[esp]+0x34u,4);', 'gp[edi]=this->read(0xbe911u,gp[esp]+0x30u,4);'),
    ('move-parent-bit', SYSTEMS, 'if(memory.load8(parent_flags,flags)&update_flags::parent) {', 'if(memory.load8(parent_flags,flags)&update_flags::disabled) {'),
    ('move-moved-clear', SYSTEMS, 'memory.store8(parent_clear,flags,value&~update_flags::parent);',
     'memory.store8(parent_clear,flags,value&~update_flags::previous_parent);'),
    ('move-attach-args', HEADER, 'this->push(0xbe956u,1);', 'this->push(0xbe956u,0);'),
    ('move-sibling', SYSTEMS, 'm.child=memory.load32(next_sibling,at(child,offsetof(ObjectData,next_sibling)));',
     'm.child=memory.load32(next_sibling,at(child,offsetof(ObjectData,first_child)));'),
    ('move-exit-flags', HEADER, 'this->flags.plus(gp[esp],0x20u,4);gp[esp]+=0x20u;', 'gp[esp]+=0x20u;'),
    ('move-handback', HEADER, 'm.handle=gp[edi];esi_value=gp[esi];m.header=gp[ebp];}', 'm.handle=gp[edi];esi_value=gp[esi];}'),
    ('move-order', SYSTEMS,
     '    if(memory.load32(cluster,at(m.object,offsetof(ObjectData,cluster)))!=none)frame.move_cluster(m);\n'
     '    if(!is_machine(memory.load8(header_type,at(m.header,offsetof(ObjectHeader,type)))))frame.rebuild_node_matrices(m);\n',
     '    const bool machine=is_machine(memory.load8(header_type,at(m.header,offsetof(ObjectHeader,type))));\n'
     '    if(memory.load32(cluster,at(m.object,offsetof(ObjectData,cluster)))!=none)frame.move_cluster(m);\n'
     '    if(!machine)frame.rebuild_node_matrices(m);\n'),
    # BE610
    ('cluster-record', SYSTEMS, 'const auto offset=handle_index(reference)*std::uint32_t(sizeof(ClusterRecord));',
     'const auto offset=handle_index(reference)*0x90u;'),
    ('cluster-index-mask', HEADER, 'this->push(0xbe653u,gp[ebp]);gp[eax]&=0xffffu;', 'this->push(0xbe653u,gp[ebp]);'),
    ('cluster-result-slot', HEADER, 'this->store(0xbe66bu,this->gp[esp]+0x13u,1,result);', 'this->store(0xbe66bu,this->gp[esp]+0x12u,1,result);'),
    ('cluster-clear-bit', SYSTEMS, 'inline constexpr std::uint32_t pending=0x00010000u;', 'inline constexpr std::uint32_t pending=0x00020000u;'),
    ('cluster-count-sign', SYSTEMS, 'offsetof(ClusterRecord,phantom_count))))>0)', 'offsetof(ClusterRecord,phantom_count))))>=0)'),
    ('cluster-static-bit', SYSTEMS, 'inline constexpr std::uint32_t fixed=0x00000100u;', 'inline constexpr std::uint32_t fixed=0x00000080u;'),
    ('cluster-flagged', HEADER, 'if(without_extent){gp[ebx]=0;gp[eax]=0;this->test32(0);}', 'if(without_extent){gp[ebx]=0;gp[eax]=0;}'),
    ('cluster-collide-arg', HEADER, 'gp[eax]=gp[esp]+0x78u;', 'gp[eax]=gp[esp]+0x74u;'),
    ('cluster-skip-eax', HEADER, 'gp[eax]=c.handle;\n        this->push(0xbe74cu,0);', 'gp[eax]=gp[ebx];\n        this->push(0xbe74cu,0);'),
    ('cluster-second-edi', HEADER, 'gp[edi]=gp[ecx];c.handle=gp[esi];c.record=gp[ebp];', 'gp[edi]=gp[eax];c.handle=gp[esi];c.record=gp[ebp];'),
    ('cluster-global', SYSTEMS, 'inline constexpr std::uint32_t phantom_default=0x474d04u;', 'inline constexpr std::uint32_t phantom_default=0x474d00u;'),
    ('cluster-mode', SYSTEMS, 'if(game_mode!=4u &&', 'if(game_mode!=5u &&'),
    ('cluster-release-bit', SYSTEMS, 'offsetof(ObjectData,update_flags)))>>11)&1u))', 'offsetof(ObjectData,update_flags)))>>10)&1u))'),
    ('cluster-parent-bit', SYSTEMS, 'c.in_cluster=memory.load8(parent_flags,at(parent_data,offsetof(ObjectData,cluster_flags)))&object_cluster_flags::in_cluster;',
     'c.in_cluster=memory.load8(parent_flags,at(parent_data,offsetof(ObjectData,cluster_flags)))&2u;'),
    ('cluster-ret', HEADER, 'gp[esp]+=0x58u;\n        this->ret(last,4);', 'gp[esp]+=0x58u;\n        this->ret(last,0);'),
    ('cluster-handback', HEADER, 'c.handle=gp[ebx];c.object=gp[edi];c.record=gp[ebp];', 'c.handle=gp[ebx];c.object=gp[edi];'),
    ('cluster-extent-local', HEADER, 'gp[ebx]=gp[esp]+0x38u;this->store(0xbe6cau', 'gp[ebx]=gp[esp]+0x34u;this->store(0xbe6cau'),
    # C0F60
    ('attach-deleted', SYSTEMS, '&native_object_update::header_flags::pending_delete)return;', '&native_object_update::header_flags::held)return;'),
    ('attach-birth-arg', HEADER, 'gp[edi]=r.object;gp[ecx]=gp[esi];', 'gp[edi]=r.object;gp[ecx]=gp[edi];'),
    ('attach-node-count', SYSTEMS, 'offsetof(ObjectData,node_attachments_size))))>>3;', 'offsetof(ObjectData,node_attachments_size))))>>2;'),
    ('attach-node-stride', SYSTEMS, 'r.entry+=std::uint32_t(sizeof(NodeLightAttachment));', 'r.entry+=4u;'),
    ('attach-node-blocked', SYSTEMS, '!=none && !r.blocked) {', '!=none) {'),
    ('attach-placement-order', HEADER, 'this->push(0xc1031u,gp[eax]);this->push(0xc1032u,gp[ecx]);', 'this->push(0xc1031u,gp[ecx]);this->push(0xc1032u,gp[eax]);'),
    ('attach-child-flag', HEADER, 'set_low_byte(gp[edx],(gp[ebx]&0xffu)==0?1u:0u);', 'set_low_byte(gp[edx],(gp[ebx]&0xffu)!=0?1u:0u);'),
    ('attach-child-slot', HEADER, 'gp[edx]=this->read(0xc104bu,gp[esp]+0x1cu,4);', 'gp[edx]=this->read(0xc104bu,gp[esp]+0x18u,4);'),
    ('attach-sibling-slot', HEADER, 'this->store(0xc1069u,this->gp[esp]+0x14u,4,sibling);', 'this->store(0xc1069u,this->gp[esp]+0x18u,4,sibling);'),
    ('attach-ret', HEADER, 'this->ret(0xc1076u,0x14u);', 'this->ret(0xc1076u,0x10u);'),
    ('attach-node-handback', HEADER, 'void take_node(Kept &r)const{const auto &gp=this->gp;r.entry=gp[esi]-4u;',
     'void take_node(Kept &r)const{const auto &gp=this->gp;'),
    ('attach-node-mirror', HEADER, 'else {gp[esi]+=8u;gp[ebp]=this->flags.increment(gp[ebp],4,true);}', 'else {gp[esi]+=8u;}'),
    ('attach-ready-order', SYSTEMS,
     '        const bool ready=frame.ready();\n'
     '        const auto header=element(rows,handle_index(r.handle),sizeof(ObjectHeader));\n'
     '        r.object=memory.load32(object,at(header,offsetof(ObjectHeader,object)));\n',
     '        const auto header=element(rows,handle_index(r.handle),sizeof(ObjectHeader));\n'
     '        r.object=memory.load32(object,at(header,offsetof(ObjectHeader,object)));\n'
     '        const bool ready=frame.ready();\n'),
    # 10DD80
    ('types-callback', SYSTEMS, 'memory.load32(move,at(entry,offsetof(ObjectTypeDefinition,move)))',
     'memory.load32(move,at(entry,offsetof(ObjectTypeDefinition,update)))'),
    ('types-arg', HEADER, 'this->push(0x10ddc0u,gp[edi]);', 'this->push(0x10ddc0u,gp[esi]);'),
    ('types-flags', HEADER, 'this->test32(callback);this->push(0x10ddc0u,gp[edi]);', 'this->push(0x10ddc0u,gp[edi]);'),
    ('types-sign', SYSTEMS, 'const auto object_type=widen8(memory.load8(type,at(data,offsetof(ObjectData,type))));',
     'const auto object_type=memory.load8(type,at(data,offsetof(ObjectData,type)));'),
    ('types-handback', HEADER, 'w.handle=gp[edi];w.slot=gp[esi];', 'w.handle=gp[edi];'),
    ('types-mirror', HEADER, 'case chain_next:gp[ecx]=v;gp[esi]+=4u;gp[eax]=gp[esi];break;', 'case chain_next:gp[ecx]=v;gp[esi]+=4u;break;'),
    # C0200
    ('nodes-second', SYSTEMS, 'at(model_data,offsetof(ModelDefinition,second_node_tag))', 'at(model_data,offsetof(ModelDefinition,second_node_group))'),
    ('nodes-slot', HEADER, 'gp[ecx]=this->read(0xc025au,gp[esp]+4u,4);', 'gp[ecx]=this->read(0xc025au,gp[esp],4);'),
    # 1E15D0 / 1D5790
    ('timer-compare', SYSTEMS, 'if(signed_value(widen8(value))<signed_value(widen8(end)))return Exit::running;', 'if(value<end)return Exit::running;'),
    ('timer-down', SYSTEMS, 'if(signed_value(widen8(value))>0)return Exit::running;', 'if(value)return Exit::running;'),
    ('timer-held', SYSTEMS, 'memory.store8(held_up,flags_address,state|countdown_flags::held);',
     'memory.store8(held_up,flags_address,state|countdown_flags::counting_up);'),
    ('timer-flags', HEADER, 'this->flags.minus(gp[eax]&0xffu,gp[ebx]&0xffu,1);', 'this->flags.minus(gp[ebx]&0xffu,gp[eax]&0xffu,1);'),
    ('timers-reset-order', SYSTEMS,
     '        memory.store8(reset_limit,at(t.timer,offsetof(CountdownTimer,limit)),0);\n'
     '        memory.store8(reset_counter,at(t.timer,offsetof(CountdownTimer,counter)),0);\n',
     '        memory.store8(reset_counter,at(t.timer,offsetof(CountdownTimer,counter)),0);\n'
     '        memory.store8(reset_limit,at(t.timer,offsetof(CountdownTimer,limit)),0);\n'),
    ('timers-second', SYSTEMS, 't.timer=at(t.block,offsetof(TimerBlock,second_timer));', 't.timer=at(t.block,offsetof(TimerBlock,first_timer));'),
    ('timers-or', SYSTEMS, 'return std::uint8_t(t.first|second);', 'return std::uint8_t(t.first);'),
    ('timers-handback', HEADER, 't.block=gp[esi];t.timer=gp[ecx];set_low_byte(gp[ebx],gp[eax]);', 't.block=gp[esi];set_low_byte(gp[ebx],gp[eax]);'),
    # C0910
    ('lights-rows', SYSTEMS, 'std::uint32_t((std::uint64_t(0xaaaaaaabu)*size)>>32)>>3;', 'std::uint32_t((std::uint64_t(0xaaaaaaabu)*size)>>32)>>2;'),
    ('lights-copy', SYSTEMS, 'memory.load32(copy_green,at(source,offsetof(ColourRow,green)))', 'memory.load32(copy_green,at(source,offsetof(ColourRow,blue)))'),
    ('lights-copy-order', SYSTEMS,
     '        const auto green=memory.load32(copy_green,at(source,offsetof(ColourRow,green)));\n'
     '        memory.store32(store_green,at(r.lit_row,offsetof(ColourRow,green)),green);\n'
     '        const auto blue=memory.load32(copy_blue,at(source,offsetof(ColourRow,blue)));\n',
     '        const auto green=memory.load32(copy_green,at(source,offsetof(ColourRow,green)));\n'
     '        const auto blue=memory.load32(copy_blue,at(source,offsetof(ColourRow,blue)));\n'
     '        memory.store32(store_green,at(r.lit_row,offsetof(ColourRow,green)),green);\n'),
    ('lights-weight-slot', HEADER, 'weigh(function,0xc09e1u,0x20u,0xc09ecu)', 'weigh(function,0xc09e1u,0x24u,0xc09ecu)'),
    ('lights-zero-weight', HEADER, 'this->store(0xc09f8u,this->gp[esp]+0x20u,4,0);', 'this->store(0xc09f8u,this->gp[esp]+0x20u,4,0x80000000u);'),
    ('lights-accumulate-args', HEADER, 'gp[esi]=gp[edi]+0x14u;', 'gp[esi]=gp[edi]+0x10u;'),
    ('lights-entry-stride', SYSTEMS, 'const auto offset=frame.contribution_offset()+std::uint32_t(sizeof(LightEntry));',
     'const auto offset=frame.contribution_offset()+0x24u;'),
    ('lights-mul-order', SYSTEMS, 'sse::multiply(green_bits,weight)', 'sse::multiply(weight,green_bits)'),
    ('lights-limit-lanes', HEADER, 'case limit:x[2]=scalar(v);break;', 'case limit:x[2]=_mm_set1_ps(_mm_cvtss_f32(scalar(v)));break;'),
    ('lights-clamp-operands', SYSTEMS, 'if(sse::above(memory.load32(value,address),limit_bits))return limit_bits;',
     'const auto channel=memory.load32(value,address);\n    if(sse::above(limit_bits,channel))return limit_bits;'),
    ('lights-clamp-unordered', SYSTEMS, 'const bool green_negative=sse::above(0u,memory.load32(green_test,green_address));',
     'const bool green_negative=!sse::above(memory.load32(green_test,green_address),0u);'),
    ('lights-store-order', SYSTEMS,
     '        const bool blue_negative=sse::above(0u,memory.load32(blue_test,blue_address));\n'
     '        memory.store32(green_store,green_address,green_result);\n',
     '        memory.store32(green_store,green_address,green_result);\n'
     '        const bool blue_negative=sse::above(0u,memory.load32(blue_test,blue_address));\n'),
    ('lights-result', HEADER, 'this->store(0xc0989u,gp[esp]+0x13u,1,1);', 'this->store(0xc0989u,gp[esp]+0x13u,1,0);'),
    ('lights-exit-flags', HEADER, 'this->flags.plus(gp[esp],0x24u,4);gp[esp]+=0x24u;', 'gp[esp]+=0x24u;'),
    ('lights-handback', HEADER, 'this->call(0xc0a0fu,0xc0a14u,0x134730u,accumulate_child);\n        r.light_row=gp[ebp];r.lit_row=gp[ebx];',
     'this->call(0xc0a0fu,0xc0a14u,0x134730u,accumulate_child);\n        r.light_row=gp[ebp];'),
]


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def helper():
    spec = importlib.util.spec_from_file_location('native_object_subtree_replacements', HELPER)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def compile_harness(scratch, name, includes, sanitizers=False):
    binary = scratch / name
    command = ['c++', '-std=c++20', '-O2', '-fno-fast-math', '-ffp-contract=off',
               *[f'-I{path}' for path in includes], f'-I{ROOT / "include"}', f'-I{ROOT / "src"}',
               f'-I{scratch}', str(TEST), '-o', str(binary)]
    if sanitizers:
        command[1:1] = ['-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer']
    subprocess.run(command, check=True, cwd=scratch)
    return binary


def run(binary, cases, mode=''):
    command = [str(binary), str(cases)] + ([mode] if mode else [])
    return json.loads(subprocess.run(command, check=True, capture_output=True, text=True).stdout)


def branch_names(text, group):
    block = re.sub(r'//[^\n]*', '', re.search(r'enum ' + group + r' : unsigned \{(.*?)\};', text, re.S)[1])
    return [name for name in re.sub(r'\s', '', block).split(',') if name and not name.endswith('_count')]


def check_binding(module, text):
    output, installed = module.install(text, text)
    if installed != list(OWNERS) or any(module.prefix(name) not in output for name in OWNERS):
        raise ValueError(f'Object subtree binding did not install: {installed}')
    if module.install(text, output)[1]:
        raise ValueError('Object subtree binding is not idempotent')
    if output.count(module.REFERENCES_BEGIN) != 1 or \
            output.index(module.REFERENCES_BEGIN) > output.index('static void run_be850(WorldState &s) {'):
        raise ValueError('Object subtree references are not in the unit prefix')
    original = module.body(text, 'be610')[0]
    changed = text.replace(original, original.replace('0x474d04u', '0x474d08u', 1), 1)
    if changed == text or 'be610' in module.install(changed, changed)[1]:
        raise ValueError('Object subtree binding accepted a changed BE610 body')
    child = module.body(text, 'c3750')[0]
    childless = text.replace(child, '', 1)
    if 'c0f60' in module.install(childless, childless)[1]:
        raise ValueError('Object subtree binding accepted C0F60 without its C3750 body')
    route = module.ROUTES['be850'][0]
    rerouted = text.replace(route, route.replace('s.required(', 'dispatch(s,0xbf0c0u,0xbe892u,false);(void)(', 1), 1)
    if 'be850' in module.install(rerouted, rerouted)[1]:
        raise ValueError('Object subtree binding accepted BE850 with a changed BF0C0 route')
    # C0F60's C100F leaves through the light callers' required boundary; the
    # old direct C4D30 call must not bind.
    route = module.ROUTES['c0f60'][0]
    direct = text.replace(route, 's.push(0xc100fu,0xc1014u);run_c4d30(s);', 1)
    if direct == text or 'c0f60' in module.install(direct, direct)[1]:
        raise ValueError('Object subtree binding accepted C0F60 with a direct C4D30 call')
    if f'v.required(r(v.memory,v.gp[4],4)-5u,0xc4d30u,false)' not in module.prefix('c0f60'):
        raise ValueError('C0F60 binding does not route C4D30 through the required owner')
    return output


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--cases', type=int, default=14000)
    parser.add_argument('--mutant-cases', type=int, default=3500)
    parser.add_argument('--jobs', type=int, default=4)
    parser.add_argument('--skip-sanitizers', action='store_true')
    parser.add_argument('--skip-mutants', action='store_true')
    args = parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Run through tools/run_test.py')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    sources = [UNIT, HEADER, SYSTEMS, MACHINE, MACHINE_SYSTEMS, TEST, HELPER, Path(__file__).resolve()]
    before = {str(path.relative_to(ROOT)): digest(path) for path in sources}
    text = UNIT.read_text()
    module = helper()
    if module.admitted(text) != list(OWNERS):
        raise ValueError(f'Canonical object subtree bodies drifted from their pins: {module.admitted(text)}')
    for name in OWNERS:
        found = module.body(text, name)
        (scratch / f'subtree_{name}.inc').write_text(found[0] + '\n')
        (scratch / f'subtree_{name}_reference.inc').write_text(module.reference(found, name))
    check_binding(module, text)
    report = dict(passed=False, unit=str(UNIT.relative_to(ROOT)), pins=module.PINS)
    release = compile_harness(scratch, 'subtree-release', [])
    result = run(release, args.cases)
    census = run(release, max(args.cases // 4, 1000), 'census')
    verify = run(release, max(args.cases // 4, 1000), 'verify')
    header_text = HEADER.read_text()
    names, hit = [], set()
    for group, key in (('Move', 'move_coverage'), ('Attach', 'attach_coverage')):
        group_names = branch_names(header_text, group)
        coverage = result[key] | verify[key]
        names += group_names
        hit |= {name for index, name in enumerate(group_names) if coverage >> index & 1}
    missing = sorted(set(names) - hit - UNREACHABLE)
    report.update(release=result, census=census, verify=verify, branches=names, covered=sorted(hit),
                  unreachable=sorted(UNREACHABLE), missing=missing)
    if missing:
        raise ValueError(f'Uncovered native object subtree branches: {missing}')
    if census['move_coverage'] == 0 or census['attach_coverage'] == 0:
        raise ValueError('Census mode never admitted the native closures')
    if not args.skip_sanitizers:
        sanitized = compile_harness(scratch, 'subtree-sanitized', [], sanitizers=True)
        report['sanitized'] = run(sanitized, max(args.cases // 5, 1000))
        report['sanitized_verify'] = run(sanitized, 500, 'verify')
        sanitized.unlink()

    texts = {path: path.read_text() for path in (HEADER, SYSTEMS)}

    def screen(mutant):
        name, target, old, new = mutant
        text = texts[target]
        if text.count(old) != 1:
            return dict(name=name, rejected=False, error='mutation anchor missing')
        directory = scratch / 'mutants' / name / 'physics/runtime'
        directory.mkdir(parents=True)
        (directory / target.name).write_text(text.replace(old, new, 1))
        try:
            binary = compile_harness(scratch, f'mutant-{name}', [scratch / 'mutants' / name])
        except subprocess.CalledProcessError:
            return dict(name=name, rejected=False, error='mutant failed to compile')
        # The default mode compares stores, registers and the bank; the verify
        # mode also orders every read against the stores.
        runs = subprocess.run([str(binary), str(args.mutant_cases)], capture_output=True, text=True)
        if runs.returncode == 0:
            runs = subprocess.run([str(binary), str(max(args.mutant_cases // 2, 500)), 'verify'],
                                  capture_output=True, text=True)
        binary.unlink()
        return dict(name=name, rejected=runs.returncode != 0, diagnostic=runs.stderr.strip()[-200:])
    mutants = []
    if not args.skip_mutants:
        with concurrent.futures.ThreadPoolExecutor(args.jobs) as pool:
            mutants = list(pool.map(screen, MUTANTS))
    report['mutants'] = mutants
    after = {str(path.relative_to(ROOT)): digest(path) for path in sources}
    report['source_sha256'] = after
    if after != before:
        raise ValueError('Sources changed during the mutation screen')
    survivors = [m['name'] for m in mutants if not m['rejected']]
    report['passed'] = not survivors
    report['survivors'] = survivors
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')
    if survivors:
        raise ValueError(f'Mutants survived: {survivors}')
    print(f"PASS {result['compared']} native object subtree comparisons ({result['ordered_stores']} ordered stores, "
          f"{result['children']} genuine children, {result['faults']} faults), census {census['compared']}, "
          f"verify {verify['compared']}, {len(mutants)} mutants rejected")
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
