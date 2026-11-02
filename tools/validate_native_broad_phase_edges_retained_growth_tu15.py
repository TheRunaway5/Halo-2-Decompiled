#!/usr/bin/env python3
"""Extend the frozen sweep proof to nonempty borrowed/owned pair-array growth."""
from pathlib import Path
import hashlib,json,sys,copy
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 parent=ROOT/'analysis/physics/native-broad-phase-edges-qualified-20261102';pins=json.loads((parent/'source-pins.json').read_text());template=parent/'source/tools/validate_native_broad_phase_edges_tu15.py';assert sha(template)==pins['source_sha256']['tools/validate_native_broad_phase_edges_tu15.py'];text=template.read_text()
 needle='    namespace=dict(array.__dict__);exec(text,namespace)'
 # This is ordinary declared BEFORE setup. It runs the genuine allocator and
 # fills its real output allocation before either original/native sweep starts.
 setup='''        if 'prepare_array'in case:
            r=case['prepare_array'];prepare=f'mov ${SP-0x100},%esp\\n push $0x12\\n push ${r["bytes"]}\\n mov $0x486080,%ecx\\n call 0x149da0\\n mov %eax,{r["header"]}\\n mov %eax,%edi\\n mov ${r["initial_data"]},%esi\\n mov ${r["bytes"]},%ecx\\n rep movsb\\n'
'''
 replacement='    text=text.replace("        assembly.append(f\'\'\'",'+repr(setup)+'+"        assembly.append(f\'\'\'")\n'+needle
 assert text.count(needle)==1;text=text.replace(needle,replacement)
 namespace=dict(__name__='retained_growth_proof',__file__=str(template));exec(compile(text,str(template),'exec'),namespace);base_cases=namespace['cases']
 def cases():
  rows=base_cases();extra=[]
  for phase,seedname,header,data,stride in [('created','axis0_upper_right_cap0',namespace['CREATED'],namespace['CREATE_DATA'],8),('removed','axis0_upper_left_cap0',namespace['REMOVED'],namespace['REMOVE_DATA'],8),('special','x_special_upper_right_cap0',namespace['SPECIAL']+4,namespace['SPECIAL_DATA'],2)]:
   for count in (1,4,7):
    for owned in (False,True):
     c=copy.deepcopy(next(x for x in rows if x['name']==seedname));c['name']=f'retained_{phase}_{count}_'+('owned'if owned else'borrowed');w=dict(c['user_words']);w[header+4-namespace['UI']]=count;w[header+8-namespace['UI']]=count if owned else count|0x80000000;c['user_words']=sorted(w.items())
     if owned:c['prepare_array']=dict(header=header,bytes=count*stride,initial_data=data)
     extra.append(c)
  return rows+extra
 namespace['cases']=cases;result=namespace['main']();out=Path(sys.argv[sys.argv.index('--output')+1]);report=json.loads((out/'report.json').read_text());report.update(retained_growth_additional_full_cases=18,real_owned_before_allocations=9,validator_sha256=sha(Path(__file__)),frozen_parent_validator_sha256=sha(template));(out/'report.json').write_text(json.dumps(report,indent=2)+'\n');return result
if __name__=='__main__':raise SystemExit(main())
