#!/usr/bin/env python3
"""World model preserving four production archives and transitive links."""
from pathlib import Path
import importlib.util
import inspect

ROOT=Path(__file__).resolve().parents[1]
SPEC=importlib.util.spec_from_file_location('native_producer_flat_model',ROOT/'.local/validate-native-producer-world-model-v1482.py')
parent=importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(parent)


def main():
    source=inspect.getsource(parent.main)
    old="for library in ('halo2_native_object_velocity','halo2_native_pair_consumer','halo2_native_simplex_helpers'):"
    new="for library in ('halo2_native_object_velocity','halo2_native_pair_consumer','halo2_native_contact_features','halo2_native_simplex_helpers'):"
    assert source.count(old)==1
    source=source.replace(old,new)
    first=source.index("  targets=' '.join(")
    last=source.index('  tests=[',first)
    builder=r'''  production_cmake=cmake
  groups={}
  for library in ('halo2_native_object_velocity','halo2_native_pair_consumer','halo2_native_contact_features','halo2_native_simplex_helpers'):
   parts=re.findall(r'(?:add_library|target_sources)\('+library+r'\s+(?:STATIC|PRIVATE)\s+([^)]*)\)',production_cmake,re.S)
   members=[p for part in parts for p in re.findall(r'src/[^\s)]+\.cpp',part)]
   assert len(members)==len(set(members)),('Duplicate source in archive',library)
   groups[library]=members
  all_members=[p for members in groups.values()for p in members]
  assert len(all_members)==len(set(all_members)), 'Preexisting archive source overlap'
  assert set(all_members)<=set(files), 'Archive source missing from model closure'
  executable=[p for p in files if p not in set(all_members)]
  assert len(executable)+len(all_members)==len(files)
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(nativeWorldArchives LANGUAGES CXX)\ninclude(CTest)\nfind_package(Threads REQUIRED)\n'
  for library,members in groups.items():
   cmake+='add_library('+library+' STATIC '+' '.join('"'+str(project/p)+'"'for p in members)+')\n'
   cmake+='target_include_directories('+library+' PUBLIC "'+str(project/'include')+'")\ntarget_compile_features('+library+' PUBLIC cxx_std_20)\ntarget_compile_options('+library+' PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
  dependency='target_link_libraries(halo2_native_pair_consumer PUBLIC halo2_native_contact_features halo2_native_simplex_helpers)'
  threads='target_link_libraries(halo2_native_pair_consumer PUBLIC Threads::Threads)'
  assert production_cmake.count(dependency)==production_cmake.count(threads)==1
  cmake+=dependency+'\n'+threads+'\n'
  cmake+='add_executable(native_tu15_world_residuals_tests '+' '.join('"'+str(project/p)+'"'for p in executable)+')\ntarget_include_directories(native_tu15_world_residuals_tests PRIVATE "'+str(project/'include')+'")\ntarget_compile_features(native_tu15_world_residuals_tests PRIVATE cxx_std_20)\ntarget_compile_options(native_tu15_world_residuals_tests PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\ntarget_link_libraries(native_tu15_world_residuals_tests PRIVATE halo2_native_object_velocity halo2_native_pair_consumer Threads::Threads)\n'
  report.update(production_archives=groups,archive_member_count=len(all_members),executable_cpp_count=len(executable),direct_executable_libraries=['halo2_native_object_velocity','halo2_native_pair_consumer','Threads::Threads'],exact_pair_consumer_public_dependency=dependency,preexisting_archive_overlaps=[],each_cpp_compiled_once=True,production_cmake_sha256=sha(a.stage/'staged/cmake/PhysicsLiveWorldTu15.cmake'))
'''
    source=source[:first]+builder+source[last:]
    marker="  (project/'CMakeLists.txt').write_text(cmake)"
    assert source.count(marker)==1
    source=source.replace(marker,marker+"\n  (a.output/'generated-CMakeLists.txt').write_text(cmake)")
    env=dict(parent.__dict__,__file__=__file__)
    exec(source,env)
    return env['main']()


if __name__=='__main__':raise SystemExit(main())
