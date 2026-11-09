#!/usr/bin/env python3
"""Run the unchanged full 322D20 numeric/default physical matrix on the candidate."""
import argparse,hashlib,inspect,json,sys
from pathlib import Path
import validate_native_contact_point_create_tu15 as parent
ROOT=Path(__file__).resolve().parents[1]
CANDIDATE='c14eb547e5ddfb253b59151019f0f1cfbe299532f6474612da18d0ceadca19bc'
def main():
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--source-root',type=Path,required=True);parser.add_argument('--candidate',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);a=parser.parse_args()
 assert parent.sha(a.candidate)==CANDIDATE
 text=inspect.getsource(parent.main)
 copy='''  project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
  for unit in UNITS:shutil.copyfile(ROOT/'src/physics'/unit,project/unit)
  shutil.copyfile(ROOT/'src/physics/native_tu15_joint_constraint_numeric_internal.hpp',project/'native_tu15_joint_constraint_numeric_internal.hpp')'''
 replacement='''  project=scratch/'project';project.mkdir();shutil.copytree(COHERENT/'include',project/'include')
  for unit in UNITS:
   f=CANDIDATE if unit==UNITS[0] else COHERENT/'src/physics'/unit
   shutil.copyfile(f,project/unit)
  shutil.copyfile(COHERENT/'src/physics/native_tu15_joint_constraint_numeric_internal.hpp',project/'native_tu15_joint_constraint_numeric_internal.hpp')'''
 assert text.count(copy)==1;text=text.replace(copy,replacement)
 old="source_hashes={n:sha(ROOT/n)for n in ('include/halo2/native_tu15_contact_point_create.hpp',*[f'src/physics/{u}'for u in UNITS])}"
 new="source_hashes={n:sha(CANDIDATE if n.endswith('/'+UNITS[0]) else COHERENT/n)for n in ('include/halo2/native_tu15_contact_point_create.hpp',*[f'src/physics/{u}'for u in UNITS])},coherent_manifest_sha256='0c20914f37d6160ab3a7d7e8d7581d948cccddf6c3a4c277276a6404687e8308',unchanged_original_and_default_numeric_fixture=True"
 assert text.count(old)==1;text=text.replace(old,new)
 sys.argv=[sys.argv[0],'--output',str(a.output.resolve())]
 env=dict(parent.__dict__,COHERENT=a.source_root.resolve(),CANDIDATE=a.candidate.resolve());exec(text,env);env['main']()
 assert parent.sha(a.candidate)==CANDIDATE
if __name__=='__main__':main()
