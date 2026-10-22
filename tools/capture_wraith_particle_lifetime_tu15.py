#!/usr/bin/env python3
"""Build and run the actual Wraith effect lifetime through the new source API.

The read-only coherent v4 world receives the recorded particle API/dispatch,
actual sound-state constructor/reset and native camera/portal publication
adapters. Native allocation/stop, particle frames and the real object/effect
tick own cleanup. The MP mortar charge callback and declared diagnostic camera
remain explicit inputs; actual charge trigger/render production is unverified.
"""
import argparse,json,shutil,subprocess
from pathlib import Path
from audit_rendering import file_sha
ROOT=Path(__file__).resolve().parents[1]
METHOD='''    void advance_effect_particles(float seconds) {
        if(!session_configured || !word(bank,0x52be48u) || !std::isfinite(seconds) || seconds<0)
            throw FormatError("TU1.5 particle frame requires a configured source world and finite frame delta");
        const std::array args{std::bit_cast<std::uint32_t>(seconds)};
        call_actual(0x179df0u,args);
    }
'''
DISPATCH='''        if(source_effect_particle_frame_tu15_owns_entry(c.target)) {
            source_run_effect_particle_frame_tu15(m,c.target,a,fp,services(),o);return;
        }
'''

def replace_once(source,old,new):
    assert source.count(old)==1,old;return source.replace(old,new)

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);ap.add_argument('--particle-build',type=Path,required=True);ap.add_argument('--draw-build',type=Path);mode=ap.add_mutually_exclusive_group();mode.add_argument('--interleaved',action='store_true');mode.add_argument('--emitting',action='store_true');mode.add_argument('--visible-stop',action='store_true');mode.add_argument('--draw',action='store_true');mode.add_argument('--draw-stop',action='store_true');a=ap.parse_args();out=a.output.resolve();assert not out.exists();out.mkdir(parents=True);base=ROOT/'analysis/weapons-tu15-goal-20261013';v4=ROOT/'.local/sandbox-tu15-source-20261014-v4'
    pbuild=a.particle_build.resolve();pb=json.loads((pbuild/'build.json').read_text());assert file_sha(pbuild/'particle.o')==pb['objects_sha256']['particle.o']
    pin=json.loads((base/'charge-effect-source-fixed-v2/build.json').read_text())
    for p,h in {**pin['linked_libraries_sha256'],**pin['override_objects'],**pin['override_source_files']}.items():assert file_sha(Path(p))==h,p
    provenance=json.loads((base/'wraith-source-provenance-v1.json').read_text());cp=Path(provenance['source_checkpoint']);assert file_sha(cp)==provenance['source_checkpoint_sha256'];checkpoint=json.loads(cp.read_text())
    names=('src/physics/physics_live_world_tu15.cpp','include/halo2/physics_live_world_tu15.hpp')
    for name in names:assert file_sha(v4/name)==checkpoint['source_sha256'][name],name
    source=out/'source';(source/'include/halo2').mkdir(parents=True)
    world=(v4/names[0]).read_text();world=replace_once(world,'#include "halo2/source_combat_action_tu15.hpp"','#include "halo2/source_combat_action_tu15.hpp"\n#include "halo2/source_effect_particle_frame_tu15.hpp"')
    world=replace_once(world,'        initialize_sound_storage();\n','        initialize_sound_storage();\n        call_actual(0x22c440u); // Literal44B56C per-frame sound state.\n')
    world=replace_once(world,'        call_actual(0x157eb0u); // Literal44B3C4 original director new-map reset.\n','        call_actual(0x157eb0u); // Literal44B3C4 original director new-map reset.\n        call_actual(0x22c480u); // Literal44B574 per-frame sound state reset.\n')
    needle='''        if(source_physics_live_support_tu15_owns_entry(c.target)) {
            source_run_physics_live_support_tu15(m,c.target,a,fp,services(),o);return;
        }
''';world=replace_once(world,needle,needle+DISPATCH);world=replace_once(world,'    void step_objects() {',METHOD+'    void step_objects() {')
    wrapper='void PhysicsLiveWorldTu15::step_objects(){Impl::PublicCall active(*impl_);impl_->step_objects();}'
    world=replace_once(world,wrapper,wrapper+'\nvoid PhysicsLiveWorldTu15::advance_effect_particles(float seconds){Impl::PublicCall active(*impl_);impl_->advance_effect_particles(seconds);}')
    (source/'world.cpp').write_text(world)
    current=(ROOT/names[0]).read_text();start=current.index('    void publish_render_visibility(');end=current.index('    SourceModelBuildWorld model_build_world(',start);visibility_method=current[start:end]
    world=replace_once(world,'    SourceModelBuildWorld model_build_world(',visibility_method+'    SourceModelBuildWorld model_build_world(')
    world=replace_once(world,'        put(bank,0x4c6854u,frame);put(bank,0x494338u,cursor);','        put(bank,0x4c6854u,frame);put(bank,0x494338u,cursor);\n        std::ranges::fill(bank.mutable_view(0x4d1ee0u,64),std::byte{});')
    world=replace_once(world,'void PhysicsLiveWorldTu15::begin_model_frame(){Impl::PublicCall active(*impl_);impl_->begin_model_frame();}','void PhysicsLiveWorldTu15::begin_model_frame(){Impl::PublicCall active(*impl_);impl_->begin_model_frame();}\nvoid PhysicsLiveWorldTu15::publish_render_visibility(const SourcePortalVisibilitySet& visibility){Impl::PublicCall active(*impl_);impl_->publish_render_visibility(visibility);}')
    start=current.index('void PhysicsLiveWorldTu15::publish_native_observer_position(');end=current.index('std::vector<SourceFirstPersonModelTu15>',start)
    world=replace_once(world,wrapper,current[start:end]+wrapper)
    draw_object=[]
    if a.draw_build:
        draw=a.draw_build.resolve();draw_report=json.loads((draw/'build.json').read_text());assert file_sha(draw/'particle.o')==draw_report['objects_sha256']['particle.o'];assert draw_report['frame_build_sha256']==file_sha(pbuild/'build.json')
        draw_object=[str(draw/'particle.o')]
        world=replace_once(world,'#include "halo2/source_effect_particle_frame_tu15.hpp"','#include "halo2/source_effect_particle_frame_tu15.hpp"\n#include "halo2/source_effect_particle_draw_tu15.hpp"')
        world=replace_once(world,DISPATCH,DISPATCH+DISPATCH.replace('particle_frame','particle_draw'))
        world=replace_once(world,'        call_actual(0x381370u);','        call_actual(0x381370u);\n        call_actual(0x380fa0u); // Literal46D990 portal/object empty bounds.\n')
        shutil.copyfile(ROOT/'include/halo2/source_effect_particle_draw_tu15.hpp',source/'include/halo2/source_effect_particle_draw_tu15.hpp')
    assert not (a.draw or a.draw_stop) or draw_object
    (source/'world.cpp').write_text(world)
    header=(v4/names[1]).read_text();header=replace_once(header,'    void step_objects();','    void step_objects();\n    void advance_effect_particles(float frame_seconds);');header=replace_once(header,'    void begin_model_frame();','    void begin_model_frame();\n    void publish_render_visibility(const SourcePortalVisibilitySet&);\n    void publish_native_observer_position(unsigned view,Vec3 position);');(source/'include/halo2/physics_live_world_tu15.hpp').write_text(header)
    shutil.copyfile(ROOT/'include/halo2/source_effect_particle_frame_tu15.hpp',source/'include/halo2/source_effect_particle_frame_tu15.hpp');shutil.copyfile(ROOT/'tools/capture_wraith_particle_lifetime_tu15.cpp',source/'capture.cpp')
    (source/'task-api.txt').write_text(METHOD+DISPATCH+visibility_method+current[start:end])
    common=['c++','-std=c++20','-O1','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(source/'include'), '-I'+str(source/'src'),'-I'+str(v4/'include'),'-I'+str(v4/'src'),'-isystem','/usr/include/SDL2']
    commands=[common+['-c',str(source/'world.cpp'),'-o',str(out/'world.o')],common+['-c',str(source/'capture.cpp'),'-o',str(out/'capture.o')]]
    link=list(pin['link']);index=next(i for i,p in enumerate(link) if p.endswith('/capture.o'));link[index:index+1]=[str(out/'capture.o'),str(out/'world.o'),str(pbuild/'particle.o'),*draw_object];link[link.index('-o')+1]=str(out/'capture');commands.append(link)
    with (out/'build.log').open('w') as log:
        for command in commands:subprocess.run(command,cwd=ROOT/'build-sandbox-tu15-20261014-v4',stdout=log,stderr=subprocess.STDOUT,check=True)
    xbe=Path.home()/'.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe';assert file_sha(xbe)=='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd';map_=ROOT/'assets/disc/maps/coagulation.map';assert file_sha(map_)=='1de37727dbb9c51aa08e03bfea86c0641ad67250dba28c5dcb3ba3dadaaf93fa'
    command=[str(out/'capture'),str(xbe),str(map_),str(out)]+(['interleaved'] if a.interleaved else ['emitting'] if a.emitting else ['visible-stop'] if a.visible_stop else ['draw'] if a.draw else ['draw-stop'] if a.draw_stop else [])
    with (out/'native.log').open('w') as log:result=subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,timeout=180)
    metadata=dict(scope=__doc__,tool_sha256=file_sha(Path(__file__)),compile=commands[:-1],link=link,command=command,native_returncode=result.returncode,binary_sha256=file_sha(out/'capture'),source_checkpoint_sha256=file_sha(cp),base_source_sha256={n:checkpoint['source_sha256'][n] for n in names},source_files_sha256={str(p.relative_to(out)):file_sha(p) for p in source.rglob('*') if p.is_file()},particle_build_sha256=file_sha(pbuild/'build.json'),draw_build_sha256=file_sha(a.draw_build.resolve()/'build.json') if a.draw_build else None,linked_libraries_sha256=pin['linked_libraries_sha256'],override_objects=pin['override_objects'],lifetime_banks_sha256={str(p.relative_to(out)):file_sha(p) for directory in out.glob('particle-frame*') for p in directory.glob('*.bin')},active_banks_sha256={str(p.relative_to(out)):file_sha(p) for directory in [out/'visibility-consumer',out/'transparent-init',out/'initial-particle-frame',out/'view-reset',out/'view-admission',out/'world-draw',*out.glob('active-frame*')] if directory.is_dir() for p in directory.glob('*.bin')})
    for directory in (out/'stop',out):
        if not directory.exists():continue
        m=dict(metadata,files_sha256={p.name:file_sha(p) for p in directory.iterdir() if p.is_file() and p.name!='build.json'})
        if (directory/'native.json').exists():m['native_result']=json.loads((directory/'native.json').read_text())
        (directory/'build.json').write_text(json.dumps(m,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(returncode=result.returncode,lifetime=json.loads((out/'particle-lifetime.json').read_text()) if (out/'particle-lifetime.json').exists() else None)));return result.returncode
if __name__=='__main__':raise SystemExit(main())
