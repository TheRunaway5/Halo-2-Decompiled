#!/usr/bin/env python3
"""Same body helper and sole activation builder versus genuine original parents."""
import inspect,sys,re,json
from pathlib import Path
import validate_native_contact_body_update_binding_tu15 as binding
from build_native_tu15_constraint_before_reference import sha,save
ROOT=Path(__file__).resolve().parents[1]
def main():
 i=sys.argv.index('--stage');stage=Path(sys.argv[i+1]).resolve()/'staged';del sys.argv[i:i+2]
 i=sys.argv.index('--source-root');base=Path(sys.argv[i+1]).resolve();del sys.argv[i:i+2]
 output=Path(sys.argv[sys.argv.index('--output')+1]).resolve()
 text=inspect.getsource(binding.main)
 old='native.activate_body=[&](auto body,auto&current,auto){NativeTu15IslandActivationServices activation;native_tu15_activate_physics_body(memory,body,current,activation,observe);};'
 new='native.activate_body=[&](auto body,auto&current,auto depth){NativeTu15ObjectActivationNativeServices native;native_tu15_activate_body_with_native_services(memory,body,current,depth,{0x200e020u,0x200e024u,0x200e040u,0x200e080u,0x800u},native,[](auto owner,auto,auto,unsigned,auto&)->std::uint32_t{throw std::runtime_error("Unconverted shared native activation owner="+std::to_string(owner));},observe);};'
 assert text.count(old)==1;text=text.replace(old,new)
 text=text.replace("changes=[('#include", "changes=[('#include")
 marker="changes=[('#include \"halo2/native_tu15_contact_body_update.hpp\"','#include \"halo2/native_tu15_contact_body_update_binding.hpp\"'),(old,new)]"
 assert marker in text;text=text.replace(marker,marker.replace('#include \\"','x') if False else "changes=[('#include \"halo2/native_tu15_contact_body_update.hpp\"','#include \"halo2/native_tu15_contact_body_update_binding.hpp\"\\n#include \"halo2/native_tu15_object_activation_residuals.hpp\"'),(old,new)]")
 cmake=(stage/'cmake/PhysicsLiveWorldTu15.cmake').read_text();start=cmake.index('add_library(halo2_native_object_velocity STATIC');end=cmake.index(')',start);units=re.findall(r'src/[^\s)]+\.cpp',cmake[start:end])
 for block in re.findall(r'target_sources\(halo2_native_object_velocity\s+PRIVATE\s+([^)]*)\)',cmake,re.S):units+=re.findall(r'src/[^\s)]+\.cpp',block)
 units=json.loads((ROOT/'analysis/physics/native-world-contact-body-model-gcc-20261102-v2/report.json').read_text())['compiled_cpp_closure'];units=[u for u in units if not u.startswith('tests/')];units=list(dict.fromkeys(units));pins={}
 for relative in units:
  p=stage/relative
  if not p.exists():p=base/relative
  pins[relative]=sha(p)
 setup='''
  shutil.copytree(shared_base/'include',project/'include',dirs_exist_ok=True)
  for header in (shared_stage/'include').rglob('*.hpp'):
   target=project/'include'/header.relative_to(shared_stage/'include');target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(header,target)
  for private in (shared_base/'src/physics').rglob('*'):
   if private.suffix in ('.hpp','.inc'):shutil.copyfile(private,project/private.name)
  extra_units=[]
  for relative in shared_units:
   source=shared_stage/relative
   if not source.exists():source=shared_base/relative
   target=project/source.name;shutil.copyfile(source,target);extra_units.append(source.name)
  build_text=(project/'CMakeLists.txt').read_text();begin=build_text.index('add_executable(cleanup ');end=build_text.index(')',begin);present=set(build_text[begin:end].split())
  additions=[name for name in extra_units if name not in present]
  build_text=build_text[:end]+' '+' '.join(additions)+build_text[end:];(project/'CMakeLists.txt').write_text(build_text)
'''
 # The binding transforms its parent, which then transforms the physical driver.
 anchor="text=text.replace(anchor,' extra+='"
 at=text.index(' environment=dict(parent.__dict__)')
 injection=" text=text.replace(' environment=dict(child.__dict__)',"+repr(" source=source.replace('results=[];commands=[]',"+repr('results=[];commands=[]'+setup)+")\n environment=dict(child.__dict__)")+")\n"
 text=text[:at]+injection+text[at:]
 # The injected helper names must survive both nested generated environments.
 text=text.replace('environment=dict(parent.__dict__)','environment=dict(parent.__dict__,shared_stage=shared_stage,shared_base=shared_base,shared_units=shared_units)')
 # Core parent forwards its globals through child.__dict__; install data there.
 binding.parent.child.shared_stage=stage;binding.parent.child.shared_base=base;binding.parent.child.shared_units=units
 env=dict(binding.__dict__,shared_stage=stage,shared_base=base,shared_units=units);exec(text,env);code=env['main']()
 report=json.loads((output/'report.json').read_text());report.update(shared_native_activation=True,staged_shared_hashes={str(p.relative_to(stage)):sha(p)for p in stage.rglob('*')if p.is_file()},additional_compiled_source_hashes=pins,explicit_unknown_owner_refusal=True);save(output/'report.json',report);return code
if __name__=='__main__':raise SystemExit(main())
