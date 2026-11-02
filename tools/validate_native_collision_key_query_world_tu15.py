#!/usr/bin/env python3
"""Compile the qualified canonical181-CPP model plus the six new query owners.

Keep this finite closure explicit: the earlier immutable model checkpoint
contains its qualified dependencies, rather than every unrelated Root target.
"""
import inspect,json,sys
from pathlib import Path
import validate_native_world_island_merge_stage_tu15 as parent
ROOT=Path(__file__).resolve().parents[1]
def main():
 extras=[]
 while '--extra-source' in sys.argv:
  index=sys.argv.index('--extra-source');relative=sys.argv[index+1];del sys.argv[index:index+2]
  assert relative.startswith('src/physics/')and relative.endswith('.cpp')and '..'not in Path(relative).parts
  extras.append(relative)
 base_report=ROOT/'analysis/physics/native-canonical-root-world-gcc-v2-20261102/report.json'
 baseline=json.loads(base_report.read_text());assert baseline['passed']and baseline['compiled_cpp_count']==181
 new_sources=json.loads((ROOT/'analysis/physics/native-collision-key-query-root-cmake-20261102/report.json').read_text())['new_sources']
 assert len(new_sources)==6
 source=inspect.getsource(parent.main)
 needle='  files=list(dict.fromkeys(files));assert len(files)==len(set(files));'
 assert source.count(needle)==1
 source=source.replace(needle,'  files='+repr(baseline['compiled_cpp_closure']+list(new_sources)+extras)+';'+needle.strip())
 marker="  targets=' '.join"
 assert source.count(marker)==1
 extra='''  for helper in re.findall(r'#include "(helpers/[^"\\n]+)"',(project/'tests/native_tu15_world_residuals_tests.cpp').read_text()):
   relative='tests/'+helper;path=a.stage/'staged'/relative
   if not path.is_file():path=a.source_root/relative
   target=project/relative;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(path,target);pins[relative]=sha(path)
'''
 source=source.replace(marker,extra+marker)
 source=source.replace("('merge_prefix','merge-prefix ","('pair_wake_prefix','pair-wake-prefix \\\"'+str(reference)+'\\\"'),('merge_prefix','merge-prefix ")
 source=source.replace('default|factory_prefix|merge_prefix','default|factory_prefix|merge_prefix|pair_wake_prefix')
 environment=dict(parent.__dict__,__file__=__file__);exec(source,environment);result=environment['main']()
 out=Path(sys.argv[sys.argv.index('--output')+1])/'report.json';report=json.loads(out.read_text());report.update(scope=__doc__,canonical_base_report=str(base_report.relative_to(ROOT)),canonical_base_report_sha256=parent.sha(base_report),new_query_sources=new_sources,remaining=['Actual2F36F0/304350 native service and live admission','Current shared Root Host/light changes beyond this immutable187-CPP source cohort']);parent.save(out,report);return result
if __name__=='__main__':raise SystemExit(main())
