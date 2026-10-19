#!/usr/bin/env python3
"""Run the real campaign model-child/coupled-world test in managed scratch."""
import hashlib,json,os,subprocess,sys
from pathlib import Path
scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
assert len(sys.argv)==5
runtime,xbe,map_path,report_path=map(Path,sys.argv[1:]);output=scratch/'source-world'
result=subprocess.run([str(runtime),str(xbe),str(map_path),str(output)])
source=output/'report.json';record={'passed':False,'exit_code':result.returncode,'runtime_sha256':hashlib.file_digest(runtime.open('rb'),'sha256').hexdigest(),'scratch':str(scratch),'large_artifacts_retained':False}
if source.exists():
 assert source.stat().st_size<=1024*1024
 record['source_report']=json.loads(source.read_text());record['passed']=result.returncode==0 and record['source_report'].get('passed')is True
report_path.parent.mkdir(parents=True,exist_ok=True);report_path.write_text(json.dumps(record,indent=2)+'\n')
raise SystemExit(result.returncode or (0 if record['passed']else 2))
