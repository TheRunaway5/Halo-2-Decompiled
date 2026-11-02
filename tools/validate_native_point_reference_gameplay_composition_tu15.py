#!/usr/bin/env python3
"""Qualified staged point reference callback in complete gameplay row removal.

Declared component inputs mirror the genuine fast reference count5, without
claiming a captured gameplay caller or supplying any child outcome.
"""
import argparse,hashlib,inspect,json,os,sys
from pathlib import Path
import validate_native_gameplay_contact_removal_tu15 as parent

def main():
 parser=argparse.ArgumentParser(add_help=False);parser.add_argument('--stage',type=Path,required=True);a,rest=parser.parse_known_args();sys.argv=[sys.argv[0],*rest]
 stage=a.stage.resolve()/'staged';header=stage/'include/halo2/native_tu15_contact_point_binding.hpp';observer=stage/'include/halo2/native_tu15_constraint_reference_input_observer.hpp';raw=header.read_text()
 start=raw.index('gameplay.release_constraint_reference=');end=raw.index('\n        };',start)+len('\n        };');callback=raw[start:end]
 callback=callback.replace('gameplay.release_constraint_reference','services.release_constraint_reference').replace('&result.reference','&binding.reference')
 assert callback.count('auto&state){')==1
 callback=callback.replace('auto&state){','auto&state){calls.push_back(J{{"owner",0x22e1b0u},{"instance",instance},{"datum",datum}});',1)
 original_cases=parent.cases
 def cases():
  rows=[r for r in original_cases()if not r.get('unbound_reference',False)]
  address=parent.CONSTRAINTS-parent.U+160+8
  for row in rows:row['user_words']=[(p,5 if p==address else value)for p,value in row['user_words']]
  return rows
 parent.cases=cases;source=inspect.getsource(parent.main)
 needle='''services.release_constraint_reference=[&](auto instance,auto datum,auto&state){calls.push_back(J{{"owner",0x22e1b0u},{"instance",instance},{"datum",datum}});if(c.value("unbound_reference",false))throw Required{};native_tu15_release_constraint_reference(memory,instance,datum,state,0,{},observe);};'''
 assert source.count(needle)==1;source=source.replace(needle,'NativeTu15ContactPointBindingActivity binding;const unsigned incoming_x87_depth=0;'+callback)
 needle="  project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')"
 assert source.count(needle)==1
 source=source.replace(needle,needle+'\n  shutil.copyfile(STAGED_HEADER,project/\'include/halo2/native_tu15_contact_point_binding.hpp\');shutil.copyfile(STAGED_OBSERVER,project/\'include/halo2/native_tu15_constraint_reference_input_observer.hpp\')')
 source=source.replace('driver=\'#include "halo2/native_tu15_gameplay_contact_removal.hpp"', 'driver=\'#include "halo2/native_tu15_contact_point_binding.hpp"\\n#include "halo2/native_tu15_gameplay_contact_removal.hpp"')
 environment=dict(parent.__dict__,STAGED_HEADER=header,STAGED_OBSERVER=observer)
 try:exec(source,environment);return environment['main']()
 finally:
  p=Path(sys.argv[sys.argv.index('--output')+1])/'report.json'
  if p.exists():
   report=json.loads(p.read_text());report.update(staged_builder_sha256=hashlib.sha256(header.read_bytes()).hexdigest(),staged_observer_sha256=hashlib.sha256(observer.read_bytes()).hexdigest(),qualified_callback_sha256=hashlib.sha256(callback.encode()).hexdigest(),declared_reference_count=5,actual_captured_gameplay_caller_claimed=False,scope='Exact staged production point native reference callback, adapted only activity variable name plus diagnostic call ledger, in full109 declared gameplay component cases. Complete persistent DATA+ordered reads/stores/preimages+FP; native5→4 reference release, row clearing/copy and genuine part-child prefixes. No child outcome/GP afterimage or actual gameplay caller claimed.')
   p.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
if __name__=='__main__':raise SystemExit(main())
