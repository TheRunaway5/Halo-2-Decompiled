#!/usr/bin/env python3
"""Read-only source-qualified BCDF0 owner leads, never default acceptance."""
import argparse,hashlib,json,os,re
from pathlib import Path
from generated_routine_ownership import function_bodies,literal_source_paths,qualified_dispatch_entries
from perf_4k60.census_common import callback_image_scope
ROOT=Path(__file__).resolve().parents[1]
def digest(data):return hashlib.sha256(data).hexdigest()
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--root',type=Path,default=ROOT);p.add_argument('--report',type=Path,required=True);a=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    rows=[]
    for path in literal_source_paths(a.root):
        text=path.read_text();source=str(path.relative_to(a.root));qualified=qualified_dispatch_entries(text,'tu15')
        for signature,body in function_bodies(text):
            route=qualified.get(signature.symbol);literal=bool(re.search(r'instruction(?:_literal)?\s*\(0xbcdf0u\)',body))
            if not(route and route['entry']==0xbcdf0)and not literal:continue
            rows.append(dict(source=source,source_sha256=digest(text.encode()),symbol=signature.symbol,state_type=signature.state_type,
                body_sha256=digest(body.encode()),body_lines=len(body.splitlines()),image_scope=callback_image_scope(text),
                dispatcher=route,exact_literal_entry_observation=literal,
                direct_calls=sorted(set(re.findall(r'\brun_([0-9a-f]+)\(s\)',body)))))
    report=dict(passed=True,scope='Static owner leads from exact simple dispatchers or explicit entry callbacks; no native/default/runtime acceptance',
        owners=rows,owners_found=len(rows),source_after_unchanged=all(digest((a.root/row['source']).read_bytes())==row['source_sha256']for row in rows),
        tool_sha256=digest(Path(__file__).read_bytes()),owned_scratch=os.environ['HALO2_TEST_SCRATCH'],retained_large_artifacts=[])
    if not report['source_after_unchanged']:raise ValueError('Owning source changed during audit')
    payload=(json.dumps(report,indent=2,sort_keys=True)+'\n').encode()
    if len(payload)>1048576:raise ValueError('Owner audit exceeds1MiB')
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_bytes(payload);print(json.dumps(dict(owners=len(rows),sources=[row['source']for row in rows])))
if __name__=='__main__':main()
