#!/usr/bin/env python3
"""Qualify independent attached-object camouflage without copying unrelated work."""
import hashlib,json,os,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 owner=Path(os.environ['HALO2_TEST_SCRATCH']);source=owner/'source';out=ROOT/'analysis/combat-presentation-audit-20261022/repair-attachment-material';out.mkdir(exist_ok=True);changes=[]
 def write(relative,s,phase):
  p=source/relative;before=sha(p);p.write_text(s);changes.append(dict(path=relative,before=before,after=sha(p),phase=phase));(out/'source-refresh.json').write_text(json.dumps(changes,indent=2)+'\n')
 def edit(relative,pairs,phase='candidate'):
  s=(source/relative).read_text()
  for a,b in pairs:
   assert s.count(a)==1,(relative,a,s.count(a));s=s.replace(a,b)
  write(relative,s,phase)
 if 'std::optional<float> camouflage;' not in (source/'include/halo2/renderer.hpp').read_text():
  edit('include/halo2/renderer.hpp',[('    std::span<const SourcePrtMaterialBank> prt_banks;','    std::span<const SourcePrtMaterialBank> prt_banks;\n    std::optional<float> camouflage;')],'red-interface-only')
 write('tests/camouflage_material_gpu_tests.cpp',(ROOT/'tests/camouflage_material_gpu_tests.cpp').read_text(),'red-test')
 needle='        set_tests_properties(camouflage_material_owner_isolation PROPERTIES SKIP_RETURN_CODE 77)'
 if 'add_test(NAME camouflage_object_owner_isolation' not in (source/'CMakeLists.txt').read_text():edit('CMakeLists.txt',[(needle,needle+'''\n        add_test(NAME camouflage_object_owner_isolation COMMAND camouflage_material_gpu_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps ${CMAKE_CURRENT_BINARY_DIR}/camouflage-object-test)
        set_tests_properties(camouflage_object_owner_isolation PROPERTIES SKIP_RETURN_CODE 77
            ENVIRONMENT "HALO2_TEST_CAMOUFLAGE_OBJECT_ALIASES=1")''')],'red-test')
 from validate_weapon_runtime_tu15 import private_display
 display_root=owner/'attachment-material-display';display_root.mkdir(exist_ok=True)
 with private_display(display_root) as display:
  env=dict(os.environ,DISPLAY=display,SDL_VIDEODRIVER='x11',SDL_AUDIODRIVER='dummy',XDG_CACHE_HOME=str(owner/'graphics-cache'))
  def worker(label,targets,filter_):
   cmd=[sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),'--filter',filter_,'--jobs','8','--test-jobs','1','--output',str(out/label)]
   for target in targets:cmd+=['--target',target]
   with (out/(label+'-command.log')).open('w') as log:r=subprocess.run(cmd,env=env,stdout=log,stderr=subprocess.STDOUT)
   print(json.dumps(dict(phase=label,exit_code=r.returncode)),flush=True);return r.returncode
  red=worker('red',['camouflage_material_gpu_tests'],'^camouflage_object_owner_isolation$')
  if red==0:raise RuntimeError('Missing per-object camouflage was not detected by the rendered regression')
  log=(out/'red/tests.log').read_text()
  if 'Authored camouflage scalar produced no measured GPU response' not in log:raise RuntimeError('Red did not fail on the intended material symptom')
  # The source snapshot predates concurrent camouflage/illumination work in
  # root. Refresh only these declared fields and selected-pass operations.
  for rel,needle in [('include/halo2/source_dynamic_object.hpp','    std::uint16_t control_flags{};'),('include/halo2/source_dynamic_object_scene.hpp','    std::vector<SourcePrtMaterialBank> prt;')]:
   edit(rel,[(needle,needle+'\n    float camouflage{};')])
  rel='include/halo2/world_material.hpp';needle='void bind_object_camouflage(Material &,float,std::uint16_t x87_control_word=0x023f);'
  declaration=(ROOT/rel).read_text();a=declaration.index('void bind_object_camouflage_constants(');b=declaration.index(';',a)+1
  edit(rel,[(needle,needle+'\n'+declaration[a:b])])
  rel='src/rendering/world/world_material.cpp';s=(source/rel).read_text();current=(ROOT/rel).read_text();a=s.index('void bind_object_camouflage(');b=s.index('Texture Cache::load_scope_default_bitmap()',a);c=current.index('void bind_object_camouflage(');d=current.index('Texture Cache::load_scope_default_bitmap()',c);write(rel,s[:a]+current[c:d]+s[b:],'candidate')
  rel='src/rendering/renderer/renderer.cpp';current=(ROOT/rel).read_text();a=current.index('            if(owner_blend)*owner_blend=');b=current.index('            if(owner_input && pass!=fog_material_passes.end()) {',a);binding=current[a:b]
  edit(rel,[('#include "halo2/world_material_uniform.hpp"','#include "halo2/world_material_uniform.hpp"\n#include "halo2/world_material.hpp"'),
   ('const ObjectColors *owner_colors=nullptr,const ShaderInput *owner_input=nullptr) {','const ObjectColors *owner_colors=nullptr,const ShaderInput *owner_input=nullptr,\n                                             const float *owner_camouflage=nullptr,std::uint32_t *owner_blend=nullptr) {'),
   ('            if(owner_input && pass!=fog_material_passes.end()) {',binding+'            if(owner_input && pass!=fog_material_passes.end()) {'),
   ('        std::vector<MaterialUniform> object_uniforms;','        std::vector<MaterialUniform> object_uniforms;\n        std::vector<std::uint32_t> object_blend_colors;'),
   ('std::span<const SourcePrtMaterialBank> prt_banks=std::span<const SourcePrtMaterialBank>{}) {','std::span<const SourcePrtMaterialBank> prt_banks=std::span<const SourcePrtMaterialBank>{},const float *camouflage=nullptr) {'),
   ('                object_uniforms.push_back(view_material_uniform(view,material,lighting,colors,input));','                std::uint32_t blend{};\n                object_uniforms.push_back(view_material_uniform(view,material,lighting,colors,input,camouflage,&blend));\n                object_blend_colors.push_back(blend);'),
   ('object.shader_input?&object.shader_input:nullptr,object.prt_banks);','object.shader_input?&object.shader_input:nullptr,object.prt_banks,object.camouflage?&*object.camouflage:nullptr);'),
   ('auto material_pipeline=[&](unsigned m) {','auto material_pipeline=[&](unsigned m,const Draw *draw=nullptr) {'),
   ('            const auto color=material_blend_colors[m];','            const auto color=draw && draw->object_alias!=std::numeric_limits<std::size_t>::max()?\n                object_blend_colors.at(object_aliases.at(draw->object_alias).at({m,draw->lightmap})):material_blend_colors[m];')])
  s=(source/rel).read_text();s=s.replace('material_pipeline(material);','material_pipeline(material,&draw);').replace('material_pipeline(draw.material);','material_pipeline(draw.material,&draw);');write(rel,s,'candidate-draw-raster-consumer')
  edit('src/gameplay/objects/source_dynamic_object.cpp',[('            count>16 ||','            !std::isfinite(o.camouflage) || o.camouflage<0 || o.camouflage>1 ||\n            count>16 ||')])
  edit('src/gameplay/objects/source_dynamic_object_scene.cpp',[('geometry.owner_banks(cache,nodes,sh,selected.lod)});','geometry.owner_banks(cache,nodes,sh,selected.lod),o.camouflage});')])
  edit('src/network/multiplayer_network.cpp',[
   ('put_float(p,o.sphere.radius);put(p,o.runtime_flags,4);put(p,o.control_flags,2);','put_float(p,o.sphere.radius);put(p,o.runtime_flags,4);put(p,o.control_flags,2);put_float(p,o.camouflage);'),
   ('o.sphere.center={r.real(),r.real(),r.real()};o.sphere.radius=r.real();o.runtime_flags=r.get(4);o.control_flags=r.get(2);','o.sphere.center={r.real(),r.real(),r.real()};o.sphere.radius=r.real();o.runtime_flags=r.get(4);o.control_flags=r.get(2);o.camouflage=r.real();')])
  rel='src/gameplay/multiplayer/multiplayer.cpp';current=(ROOT/rel).read_text();a=current.index('    const auto raw_body=',current.index('SourceModelResourceWorld visibility;'));b=current.index('    for(const auto &object:source_object_world_->live_objects())',a)
  edit(rel,[('#include "halo2/raw_datum_pool.hpp"','#include "halo2/raw_datum_pool.hpp"\n#include "halo2/source_camouflage_context.hpp"'),
   ('    visibility.banks.read=[&bank](auto p,auto n){return bank.view(p,n);};','    visibility.banks.read=[&bank](auto p,auto n){return bank.view(p,n);};\n'+current[a:b].rstrip()),
   ('      dynamic.parent=object.parent;dynamic.type=object.type;','      dynamic.parent=object.parent;dynamic.type=object.type;\n      dynamic.camouflage=source_object_camouflage(object.object,camouflage);')])
  rel='src/gameplay/multiplayer/multiplayer_runtime.cpp';edit(rel,[('std::optional{draw.lighting}:std::nullopt,draw.colors,{},draw.prt});','std::optional{draw.lighting}:std::nullopt,draw.colors,{},draw.prt,draw.camouflage});')])
  baseline=owner/'audit-originals/multiplayer_runtime.cpp';s=baseline.read_text();s=s.replace('std::optional{draw.lighting}:std::nullopt,draw.colors,{},draw.prt});','std::optional{draw.lighting}:std::nullopt,draw.colors,{},draw.prt,draw.camouflage});');baseline.write_text(s)
  for rel in ['tests/camouflage_material_tests.cpp','tests/camouflage_state_material_tests.cpp']:
   write(rel,(ROOT/rel).read_text(),'candidate-original-regression')
  result=worker('green',['camouflage_material_gpu_tests','camouflage_material_tests','camouflage_state_material_tests','combat_presentation_audit'],'^camouflage_object_owner_isolation$|^original_camouflage_material_animation$|^original_camouflage_state_material$')
  if result:return result
  from mcc_content import directory
  map_=directory(ROOT/'assets/imported/mcc-056309e12f444a3a8992c94f6fe32f0b')/'h2_maps_win64_dx11/coagulation.map'
  env['HALO2_COMBAT_AUDIT_CACHE']=str(owner/'graphics-cache');env['HALO2_COMBAT_AUDIT_REFERENCE_DIR']=str(out/'reference')
  report=out/'needler.json'
  with (out/'needler-command.log').open('w') as log:r=subprocess.run([sys.executable,str(ROOT/'tools/run_combat_paired_20261022.py'),str(owner/'build/combat_presentation_audit'),'fire',str(map_),str(report),'needler'],env=env,stdout=log,stderr=subprocess.STDOUT)
  return r.returncode
if __name__=='__main__':raise SystemExit(main())
