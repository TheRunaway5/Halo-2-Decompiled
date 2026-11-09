#!/usr/bin/env python3
"""Private exact-current-source reuse at two additional Host constructor sites."""
from pathlib import Path
import difflib
import hashlib
import json
import shutil

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT/'analysis/physics/native-look-additional-host-stage-20261102'
LIVE = 'src/physics/physics_live_world_tu15.cpp'
STATS = 'include/halo2/native_tu15_physics_frame_residuals.hpp'
HOST = 'tests/physics_multiplayer_acceptance.cpp'


def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()


def replace(s, a, b, n=1):
    assert s.count(a) == n, (a[:100], s.count(a), n)
    return s.replace(a, b)


def main():
    OUT.mkdir(exist_ok=False)
    stage, base = OUT/'staged', OUT/'base'
    files = [LIVE, STATS, HOST,
        'include/halo2/native_tu15_look_initialization.hpp',
        'include/halo2/native_tu15_look_initialization_binding.hpp',
        'src/physics/native_tu15_look_initialization.cpp']
    for name in files:
        for folder in (base, stage):
            p = folder/name
            p.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(ROOT/name, p)
    before = {name:sha(base/name) for name in files}
    assert all(sha(ROOT/name) == pin for name, pin in before.items())
    parent = ROOT/'analysis/physics/native-look-initialization-world-stage-v2-20261102/source-pins.json'
    prior = json.loads(parent.read_text())
    for name in files[3:]:
        assert before[name] == prior['staged_hashes'][name], name
    text = (stage/LIVE).read_text()
    text = replace(text, '    NativeTu15LookInitializationBindingActivity native_look_initialization;',
        '    NativeTu15LookInitializationBindingActivity native_look_initialization;\n'
        '    NativeTu15LookInitializationBindingActivity native_remote_look_birth_initialization, native_input_collection_look_initialization;')
    remote_old = '            auto a=frame({});a.gp[0]=0;a.gp[1]=unit;run_actual(0x18a0c0u,a);\n'
    remote = '''            auto a=frame({});
            source_execution_untracked(bank,0x18a0c0u);Execution execution(*this);
            SourcePhysicsContactRecordObserver observed;
            if(physics_store_observer)observed=[this,collector=physics_store_observer](const auto&store){
                last_instruction=store.instruction;collector(store);
            };
            native_tu15_initialize_bound_look_controller(bank,0u,unit,
                {a.floating_point->x87_control_word,a.x87_status,a.floating_point->mxcsr,a.x87_depth},
                !physics_store_observer||physics_store_observer_read_only,observed,&native_remote_look_birth_initialization);
'''
    text = replace(text, remote_old, remote)
    input_old = '''        auto a=impl_->frame({});a.gp[0]=view;a.gp[1]=unit;
        impl_->run_actual(0x18a0c0u,a);
'''
    collection = '''        auto a=impl_->frame({});
        source_execution_untracked(bank,0x18a0c0u);Impl::Execution execution(*impl_);
        SourcePhysicsContactRecordObserver observed;
        if(impl_->physics_store_observer)observed=[this,collector=impl_->physics_store_observer](const auto&store){
            impl_->last_instruction=store.instruction;collector(store);
        };
        native_tu15_initialize_bound_look_controller(bank,view,unit,
            {a.floating_point->x87_control_word,a.x87_status,a.floating_point->mxcsr,a.x87_depth},
            !impl_->physics_store_observer||impl_->physics_store_observer_read_only,observed,&impl_->native_input_collection_look_initialization);
'''
    text = replace(text, input_old, collection)
    paths = dict(attempts='attempts', calls='completed', refusals='refusals',
        cleared_records='initialization.cleared_records', unit_directions='initialization.unit_directions',
        unchanged='initialization.unchanged')
    for name, field in paths.items():
        text = replace(text, f'    result.native_look_initialization_{name}=look_initialization.{field};',
            f'    result.native_look_initialization_{name}=look_initialization.{field}+'
            f'impl_->native_remote_look_birth_initialization.{field}+'
            f'impl_->native_input_collection_look_initialization.{field};')
    fields = {}
    for site, activity in (('remote_birth','native_remote_look_birth_initialization'),
                           ('input_collection','native_input_collection_look_initialization')):
        for name, field in list(paths.items())[:3]:
            fields[f'native_look_initialization_{site}_{name}'] = f'{activity}.{field}'
    anchor = '    result.world_integrations=impl_->native_physics_integrations.world_integrations;'
    text = replace(text, anchor, ''.join(f'    result.{name}=impl_->{field};\n' for name, field in fields.items())+anchor)
    assert 'run_actual(0x18a0c0u' not in text
    (stage/LIVE).write_text(text)
    text = (stage/STATS).read_text()
    anchor = '  std::uint64_t native_look_initialization_attempts{}'
    pos = text.index(anchor)
    text = text[:pos]+''.join(f'  std::uint64_t {name}{{}};\n' for name in fields)+text[pos:]
    (stage/STATS).write_text(text)
    text = (stage/HOST).read_text()
    anchor = '        {"native_look_initialization_unchanged",native_frames.native_look_initialization_unchanged},'
    text = replace(text, anchor, anchor+''.join(f'\n        {{"{name}",native_frames.{name}}},' for name in fields), 2)
    anchor = '  if(std::getenv("HALO2_REQUIRE_NATIVE_LOOK_INITIALIZATION")) {'
    gates = ''
    for site, suffix in (('remote_birth','REMOTE_BIRTH'),('input_collection','INPUT_COLLECTION')):
        prefix = f'native_look_initialization_{site}_'
        gates += f'''  if(std::getenv("HALO2_REQUIRE_NATIVE_LOOK_{suffix}")) {{
    const bool native_site=native_frames.{prefix}calls>0&&
      native_frames.{prefix}attempts==native_frames.{prefix}calls&&native_frames.{prefix}refusals==0;
    passed&=native_site;report["{prefix}positive_gate"]=native_site;
  }}
'''
    text = replace(text, anchor, gates+anchor)
    (stage/HOST).write_text(text)
    (OUT/'remote-block.cpp.txt').write_text(remote)
    (OUT/'input-block.cpp.txt').write_text(collection)
    patch = ''.join(''.join(difflib.unified_diff((base/n).read_text().splitlines(True),
        (stage/n).read_text().splitlines(True), fromfile='a/'+n, tofile='b/'+n)) for n in files[:3])
    (OUT/'delta.patch').write_text(patch)
    r = dict(qualified=False, root_reviewed=False, production_applied=False, live_qualified=False,
        source_sha256={n:sha(stage/n) for n in files}, base_hashes=before, fields=fields,
        prior_constructor_manifest=str(parent.relative_to(ROOT)), prior_constructor_manifest_sha256=sha(parent),
        selected_sites=['prepare_player_look_births remote fallback, actual index0/unit',
                        'collect_native_input new-unit fallback, actual view/unit'],
        scope='Current canonical source overlay only, preserving foreign caller behavior. Sole qualified native92-site constructor/FP-field helper reused. No active1481 source changes, generated constructor/scheduler closure or live claim.',
        no_old_constructor_dispatch_in_selected_host_cpp=True,
        remaining=['Exact both-site Host/FP/observer/restore/counter/gate proof',
                   'Independent source review and final current-source context/integration',
                   'Genuine positive per-site live coverage; frozen1481 cannot prove these later canonical entries'])
    (OUT/'source-pins.json').write_text(json.dumps(r, indent=2)+'\n')
    print(sha(OUT/'source-pins.json'))


if __name__ == '__main__':
    main()
