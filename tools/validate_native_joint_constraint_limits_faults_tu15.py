#!/usr/bin/env python3
"""Supplement328A00 with correctly selected repeated-row fault/observer seeds."""
import json
from pathlib import Path
import validate_native_joint_constraint_limits_tu15 as kernel
def main():
 original=kernel.cases
 def cases(image):
  rows=original(image)
  for row in rows:
   if row['name'].startswith('limits_fault_seed_'):
    row['name']='fault_seed_limits_'+row['name'][len('limits_fault_seed_'):]
  return rows
 kernel.cases=cases;code=kernel.main()
 path=Path(kernel.base.sys.argv[kernel.base.sys.argv.index('--output')+1])/'report.json';r=json.loads(path.read_text());r['scope']=__doc__;r['source_hashes'][str(Path(__file__).resolve().relative_to(kernel.ROOT))]=kernel.base.sha(Path(__file__));kernel.base.save(path,r);return code
if __name__=='__main__':raise SystemExit(main())
