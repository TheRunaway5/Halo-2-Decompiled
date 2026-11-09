#!/usr/bin/env python3
"""Exact V3 native callback numeric/SourcePC complete-return mutation controls."""
import hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
import validate_native_accepted_body_contact_scalar_controls_v3_tu15 as scalar
ROOT=Path(__file__).resolve().parents[1]
def main():
 original_observer=scalar.observer_source;source=original_observer()
 line=' if(mutation.empty())for(const auto&control:meta.at("declared_scalar_controls"))'
 begin=source.index(line);end=source.index('\n',begin);source=source[:begin]+source[end:];scalar.observer_source=lambda:source
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']);cache=scratch/'mutation-object-cache';cache.mkdir();actual_run=subprocess.run;compiled={};commands=[];pin=hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
 def run(command,*args,**kwargs):
  if isinstance(command,list)and command and command[0]in('g++','clang++')and'-o'in command and any(str(p).endswith('/driver.cpp')for p in command):
   # One owned coherent source/header tree; mutation variants change one CPP.
   # Preserve all actual compile flags, source paths, unit order and link flags.
   flags=[];sources=[];i=1
   while i<len(command):
    item=str(command[i])
    if item=='-o':i+=2;continue
    if item.endswith('.cpp'):sources.append(item)
    else:flags.append(item)
    i+=1
   compile_flags=[f for f in flags if not f.startswith('-Wl,')];objects=[]
   for path in sources:
    key=(command[0],path,hashlib.sha256(Path(path).read_bytes()).hexdigest(),tuple(compile_flags))
    if key not in compiled:
     obj=cache/(hashlib.sha256(repr(key).encode()).hexdigest()+'.o');cmd=[command[0],*compile_flags,'-c',path,'-o',str(obj)];r=actual_run(cmd,capture_output=True,text=True);commands.append(dict(command=cmd,exit_code=r.returncode))
     if r.returncode:return r
     compiled[key]=str(obj)
    objects.append(compiled[key])
   cmd=[command[0],*flags,*objects,'-o',str(command[command.index('-o')+1])];r=actual_run(cmd,*args,**kwargs);commands.append(dict(command=cmd,exit_code=r.returncode));return r
  return actual_run(command,*args,**kwargs)
 subprocess.run=run
 try:scalar.main()
 finally:
  subprocess.run=actual_run;p=Path(sys.argv[sys.argv.index('--output')+1]);report=p/'report.json'
  if report.is_file():
   value=json.loads(report.read_text());value.update(scope=__doc__,declared_scalar_cases_executed=False,actual_normal_profiles_each=4,owned_compile_cache=True,compile_and_link_commands=commands,mutation_validator_sha256=pin,mutation_validator_unchanged=hashlib.sha256(Path(__file__).read_bytes()).hexdigest()==pin);report.write_text(json.dumps(value,indent=2,sort_keys=True)+'\n');target=p/'source/tools'/Path(__file__).name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(__file__,target)
if __name__=='__main__':main()
