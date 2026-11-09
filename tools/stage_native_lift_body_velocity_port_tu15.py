#!/usr/bin/env python3
"""Stage the reviewed native lift handoffs over current source; never edit ROOT."""
import argparse
import difflib
import hashlib
import json
import os
from pathlib import Path

import validate_native_phantom_body_velocity_port_tu15 as core

ROOT = core.ROOT
PARENT = core.PARENT
PARENT_FILES = ('include/halo2/native_lift_scalar_consumer_tu15.hpp',
    'include/halo2/native_tu15_object_activation_residuals.hpp',
    'src/physics/native_tu15_object_activation_residuals.cpp',
    'include/halo2/native_tu15_physics_frame_residuals.hpp',
    'src/physics/physics_phantom_force_tu15_generated.cpp')


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def stage(source, output):
    output.mkdir(parents=True, exist_ok=False)
    source.mkdir(parents=True, exist_ok=False)
    pins = core.prepare(source, output)
    manifest = json.loads((PARENT.parent / 'source-pins.json').read_text())
    diffs = [(output / 'candidate.diff').read_text()]
    for name in PARENT_FILES:
        before = (ROOT / name).read_text()
        after = (PARENT / name).read_text()
        assert sha(PARENT / name) == manifest['source_sha256'][name]
        removed = [line[2:] for line in difflib.ndiff(before.splitlines(), after.splitlines()) if line.startswith('- ')]
        if name.endswith('physics_phantom_force_tu15_generated.cpp'):
            assert len(removed) == 4 and all(any(site in line for site in ('0x1da9a6u', '0x1db370u', '0x1db3a4u', '0x1db64fu')) for line in removed)
        else:
            assert not removed, 'Unreviewed current-source drift: ' + name
        path = source / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(after)
        pins[name] = sha(ROOT / name)
        diffs.extend(difflib.unified_diff(before.splitlines(keepends=True), after.splitlines(keepends=True),
                                        fromfile='current/' + name, tofile='candidate/' + name))
    name = 'src/physics/native_tu15_contact_point_create.cpp'
    before = (ROOT / name).read_text()
    after = core.once(before, '#include "halo2/native_tu15_contact_point_create.hpp"',
        '#include "halo2/native_tu15_contact_point_create.hpp"\n#include "halo2/native_tu15_retained_counter_update.hpp"')
    after = core.once(after, 'class EventMemory final:public SourcePhysicsContactMemory {',
        'class EventMemory final:public SourcePhysicsContactMemory,public NativeTu15RetainedCounterStorage {')
    anchor = '    void read(std::uint32_t p,std::span<std::byte>b)const override{check(p,b.size());storage_.read(p,b);}'
    after = core.once(after, anchor, '''    NativeTu15RetainedCounterUpdate adjust_counter(std::uint32_t p,unsigned width,std::uint32_t delta)override{
        check(p,width);
        auto*storage=dynamic_cast<NativeTu15RetainedCounterStorage*>(&storage_);
        if(!storage)throw std::runtime_error("Native contact event counter requires checked outer storage");
        return storage->adjust_counter(p,width,delta);
    }
''' + anchor)
    path = source / name
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(after)
    pins[name] = sha(ROOT / name)
    diffs.extend(difflib.unified_diff(before.splitlines(keepends=True), after.splitlines(keepends=True),
                                    fromfile='current/' + name, tofile='candidate/' + name))
    # Existing test storage owners must explicitly admit checked counters.
    # They never grant a production read/write fallback to unknown views.
    for name in ('tests/native_tu15_world_residuals_tests.cpp', 'tests/native_tu15_shape_bounds_tests.cpp'):
        before = (ROOT / name).read_text()
        after = '#include "halo2/native_tu15_retained_counter_update.hpp"\n' + before
        after = core.once(after, 'struct Memory:SourcePhysicsContactMemory{',
                          'struct Memory:SourcePhysicsContactMemory,NativeTu15RetainedCounterStorage{')
        marker = '  void put(' if 'world_residuals' in name else '};\nstruct Host {'
        if 'world_residuals' in name:
            body = '''  NativeTu15RetainedCounterUpdate adjust_counter(std::uint32_t p,unsigned width,std::uint32_t delta)override{
    check(width==2u||width==4u,"Unqualified model counter width");const auto target=at(p,width);
    std::array<std::byte,4>before{};read(p,std::span(before).first(width));
    const auto result=native_tu15_adjust_counter_value(before,width,delta);
    std::copy_n(result.written.begin(),width,banks.at(target->first).begin()+p-target->first);return result;
  }
'''
        else:
            body = ''' NativeTu15RetainedCounterUpdate adjust_counter(std::uint32_t p,unsigned width,std::uint32_t delta)override{
  check(width==2u||width==4u,"Unqualified shape-model counter width");
  if(phase=="before_write"&&writes++==ordinal)throw Prefix{};const auto target=at(p,width);
  std::array<std::byte,4>before{};read(p,std::span(before).first(width));const auto result=native_tu15_adjust_counter_value(before,width,delta);
  std::copy_n(result.written.begin(),width,banks.at(target->first).begin()+p-target->first);
  if(raw_logging)raw.push_back({0,p,{result.before.begin(),result.before.begin()+width},{result.written.begin(),result.written.begin()+width}});return result;
 }
'''
        at = after.index(marker)
        after = after[:at] + body + after[at:]
        target = source / name
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(after)
        pins[name] = sha(ROOT / name)
        diffs.extend(difflib.unified_diff(before.splitlines(keepends=True), after.splitlines(keepends=True),
                                        fromfile='current/' + name, tofile='candidate/' + name))
    # A code generator owns the native boundary substitutions as well as the
    # ordinary lowered instructions. The template is the exact qualified text.
    force = (source / 'src/physics/physics_phantom_force_tu15_generated.cpp').read_text()
    helpers = force[force.index('bool native_lift_marker_call('):force.index('std::uint32_t integer_spill_observed(')]
    assert "'''" not in helpers
    template = '''"""Native TU1.5 force handoff text and authenticated source-site substitutions."""
HANDOFFS = r\x27\x27\x27''' + helpers + '''\x27\x27\x27
def once(text,old,new):
    assert text.count(old)==1,old
    return text.replace(old,new)
def apply_native_lift_handoffs(text):
    text=once(text,'#include "halo2/physics_velocity_observation.hpp"',
        '#include "halo2/physics_velocity_observation.hpp"\\n#include "halo2/native_lift_scalar_consumer_tu15.hpp"\\n#include "halo2/native_tu15_physics_frame.hpp"\\n#include "halo2/native_tu15_retained_store_observer.hpp"')
    text=once(text,'using namespace physics_vehicle_motion_math_detail;\\n','using namespace physics_vehicle_motion_math_detail;\\n'+HANDOFFS)
    for site in (0x1dabbe,0x1daf99,0x1db2d0,0x1db4db):
        old=f's.push(0x{site:x}u,0x{site+5:x}u);run_1da530(s);'
        text=once(text,old,f'native_lift_scalar_call(s,0x{site:x}u,0x{site+5:x}u);')
    for site in (0x1da9a6,0x1db3a4):
        old=f'{{const auto target=0xbad60u;s.push(0x{site:x}u,0x{site+5:x}u);s.required(0x{site:x}u,target,false);}}'
        text=once(text,old,'if(!native_lift_marker_call(s))'+old)
    for site,owner in ((0x1db370,0x1d92c0),(0x1db64f,0x1d9100)):
        old=f's.push(0x{site:x}u,0x{site+5:x}u);run_{owner:x}(s);'
        text=once(text,old,f'if(!native_lift_body_velocity_call(s,0x{site:x}u)){{'+old+'}')
    return text
'''
    name = 'tools/native_lift_handoff_template_tu15.py'
    target = source / name
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(template)
    diffs.extend(difflib.unified_diff([], template.splitlines(keepends=True),fromfile='current/'+name,tofile='candidate/'+name))
    name = 'tools/generate_phantom_force_tu15_20261011.py'
    before = (ROOT / name).read_text()
    after = core.once(before, 'import argparse,csv,json,struct',
                      'import argparse,csv,json,struct\nfrom native_lift_handoff_template_tu15 import apply_native_lift_handoffs')
    after = core.once(after, ' a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(text)',
                      ' text=apply_native_lift_handoffs(text)\n a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(text)')
    (source / name).write_text(after)
    pins[name] = sha(ROOT / name)
    diffs.extend(difflib.unified_diff(before.splitlines(keepends=True), after.splitlines(keepends=True),fromfile='current/'+name,tofile='candidate/'+name))
    name = 'src/physics/physics_live_world_tu15.cpp'
    before = (ROOT / name).read_text()
    after = before
    after = core.once(after, '#include "halo2/native_lift_scalar_consumer_tu15.hpp"',
                      '#include "halo2/native_lift_scalar_consumer_tu15.hpp"\n#include "halo2/native_campaign_object_marker_request_tu15.hpp"')
    after = core.once(after, '    NativeLiftScalarActivityTu15 native_lift_activity;',
                      '    NativeLiftScalarActivityTu15 native_lift_activity;\n    NativeTu15ObjectActivationBindingActivity native_lift_body_wake_activity;')
    old = '            !o&&(!physics_store_observer||physics_store_observer_read_only),&native_lift_activity});'
    parent = (PARENT / name).read_text()
    start = parent.index('            !o&&(!physics_store_observer||physics_store_observer_read_only),&native_lift_activity,')
    stop = parent.index('        NativeVehicleRateConsumerScopeTu15', start)
    after = core.once(after, old + '\n', parent[start:stop])
    anchor = '    result.native_lift_observation_refusals=impl_->native_lift_activity.observation_refusals;'
    start = parent.index(anchor)
    stop = parent.index('    result.native_vehicle_rate_acceleration_calls', start)
    after = core.once(after, anchor + '\n', parent[start:stop])
    target = source / name
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(after)
    pins[name] = sha(ROOT / name)
    diffs.extend(difflib.unified_diff(before.splitlines(keepends=True), after.splitlines(keepends=True),
                                    fromfile='current/' + name, tofile='candidate/' + name))
    assert all(sha(ROOT / name) == value for name, value in pins.items()), 'Current source changed during staging'
    (output / 'candidate.diff').write_text(''.join(diffs))
    candidates = {str(path.relative_to(source)): sha(path) for path in source.rglob('*') if path.is_file()}
    report = dict(passed=True, source_only=True, live_qualified=False, root_modified=False,
        current_source_sha256=pins, candidate_source_sha256=candidates,
        staging_tool_sha256=sha(Path(__file__)),
        original_authority_sha256='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd',
        excluded_from_patch=['src/runtime/source_runtime_write_protection.inc'],
        generator_scope='Native scalar, marker and velocity source-site substitutions are owned by tools/native_lift_handoff_template_tu15.py; exact generator reproduction must pass before integration.',
        scope='Native body velocity callee and typed marker/velocity handoffs under the generated1DA750 parent; this is not a full semantic parent conversion.',
        retained_large_artifacts=[])
    (output / 'source-pins.json').write_text(json.dumps(report, indent=2) + '\n')
    return report


def main():
    assert os.environ.get('HALO2_TEST_SCRATCH')
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    report = stage(args.source.resolve(), args.output.resolve())
    print(json.dumps({'passed': report['passed'], 'files': len(report['candidate_source_sha256']),
                      'source': str(args.source), 'output': str(args.output)}))


if __name__ == '__main__':
    main()
