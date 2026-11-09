#!/usr/bin/env python3
"""Selected World model with exact accepted-contact production archive ownership."""
from pathlib import Path
import importlib.util,inspect
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('world_models',ROOT/'.local/validate-native-producer-world-model-v1482.py')
parent=importlib.util.module_from_spec(spec);spec.loader.exec_module(parent)

def main():
    text=inspect.getsource(parent.main)
    groups=('halo2_native_object_velocity','halo2_native_pair_consumer','halo2_native_contact_features',
        'halo2_native_array_reserve','halo2_native_partition_metadata','halo2_native_accepted_contact',
        'halo2_native_camera_projection')
    old=r"""  for library in ('halo2_native_object_velocity','halo2_native_pair_consumer','halo2_native_simplex_helpers'):
   start=cmake.index('add_library('+library+' STATIC');end=cmake.index(')',start);files+=re.findall(r'src/[^\s)]+\.cpp',cmake[start:end])
  for added in re.findall(r'target_sources\(halo2_native_(?:object_velocity|pair_consumer)\s+PRIVATE\s+([^)]*)\)',cmake,re.S):
   files+=re.findall(r'src/[^\s)]+\.cpp',added)
"""
    assert text.count(old)==1
    pre='  archive_names='+repr(groups)+'\n'+r"""  production=cmake;archives={}
  for library in archive_names:
   parts=re.findall(r'(?:add_library|target_sources)\('+library+r'\s+(?:STATIC|PRIVATE)\s+([^)]*)\)',production,re.S)
   members=[p for part in parts for p in re.findall(r'src/[^\s)]+\.cpp',part)]
   assert members and len(members)==len(set(members)),(library,members)
   archives[library]=members;files+=members
  members=[p for group in archives.values()for p in group]
  assert len(members)==len(set(members)), 'Production archive overlaps'
"""
    text=text.replace(old,pre)
    helper_marker="  for path in (a.stage/'staged').rglob('*'):"
    assert text.count(helper_marker)==1
    text=text.replace(helper_marker,'''  for helper_name in ('native_world_island_merge_prefix_tu15.hpp','native_contact_process_binding_model_tu15.hpp','native_world_factory_scratch_model_tu15.hpp'):
   rel='tests/helpers/'+helper_name
   shutil.copyfile(a.source_root/rel,project/rel);pins[rel]=sha(a.source_root/rel)
'''+helper_marker)
    start=text.index("  targets=' '.join(");end=text.index('  tests=[',start)
    builder=r"""  executable=[p for p in files if p not in set(members)]
  assert len(executable)+len(members)==len(files)
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(nativeAcceptedWorld LANGUAGES CXX)\ninclude(CTest)\nfind_package(Threads REQUIRED)\n'
  for library,cpps in archives.items():
   cmake+='add_library('+library+' STATIC '+' '.join('"'+str(project/p)+'"'for p in cpps)+')\n'
   cmake+='target_include_directories('+library+' PUBLIC "'+str(project/'include')+'")\ntarget_compile_features('+library+' PUBLIC cxx_std_20)\ntarget_compile_options('+library+' PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
  public=[]
  for lib,links in re.findall(r'target_link_libraries\((halo2_native_[a-z_]+) PUBLIC ([^)]+)\)',production):
   if lib not in archives:continue
   assert all(t in archives or t=='Threads::Threads' for t in links.split()),(lib,links)
   public.append('target_link_libraries('+lib+' PUBLIC '+links+')')
  for exact in ['target_link_libraries(halo2_native_accepted_contact PUBLIC halo2_native_object_velocity halo2_native_pair_consumer halo2_native_array_reserve halo2_native_partition_metadata)',
     'target_link_libraries(halo2_native_object_velocity PUBLIC halo2_native_partition_metadata)',
     'target_link_libraries(halo2_native_pair_consumer PUBLIC halo2_native_partition_metadata)']:
   assert public.count(exact)==1,exact
  cmake+='\n'.join(public)+'\n'
  cmake+='add_executable(native_tu15_world_residuals_tests '+' '.join('"'+str(project/p)+'"'for p in executable)+')\ntarget_include_directories(native_tu15_world_residuals_tests PRIVATE "'+str(project/'include')+'")\ntarget_compile_features(native_tu15_world_residuals_tests PRIVATE cxx_std_20)\ntarget_compile_options(native_tu15_world_residuals_tests PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\ntarget_link_libraries(native_tu15_world_residuals_tests PRIVATE halo2_native_accepted_contact)\n'
  report.update(production_archives=archives,archive_member_count=len(members),executable_cpp_count=len(executable),direct_executable_libraries=['halo2_native_accepted_contact'],public_edges=public,no_overlaps=True,each_cpp_compiled_once=True,production_cmake_sha256=sha(a.stage/'staged/cmake/PhysicsLiveWorldTu15.cmake'))
"""
    text=text[:start]+builder+text[end:]
    marker="  (project/'CMakeLists.txt').write_text(cmake)"
    assert text.count(marker)==1
    text=text.replace(marker,marker+"\n  (a.output/'generated-CMakeLists.txt').write_text(cmake)")
    env=dict(parent.__dict__,__file__=__file__);exec(text,env);return env['main']()

if __name__=='__main__':raise SystemExit(main())
