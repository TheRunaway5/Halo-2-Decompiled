#!/usr/bin/env python3
"""Qualify a private replacement of the sole native authored curve module."""
import inspect,json,sys
from pathlib import Path
import validate_native_function_curves_tu15 as base
if __name__=='__main__':
 n=sys.argv.index('--candidate');candidate=Path(sys.argv[n+1]).resolve();del sys.argv[n:n+2]
 output=Path(sys.argv[sys.argv.index('--output')+1])
 source=inspect.getsource(base.main)
 needle="text=(ROOT/'src/core/native_tu15_function_curves.cpp').read_text()"
 assert source.count(needle)==1
 source=source.replace(needle,'text=Path('+repr(str(candidate))+').read_text()')
 namespace=dict(base.__dict__);namespace['__file__']=__file__;exec(source,namespace)
 try:status=namespace['main']()
 finally:
  p=output/'report.json'
  if p.exists():
   r=json.loads(p.read_text());r['frozen_baseline_cpp_sha256']=r.get('source_hashes',{}).get('src/core/native_tu15_function_curves.cpp');r['private_candidate']=str(candidate);r['private_candidate_sha256']=base.sha(candidate);r.get('source_hashes',{})['src/core/native_tu15_function_curves.cpp']=base.sha(candidate);base.save(p,r)
 sys.exit(status)
