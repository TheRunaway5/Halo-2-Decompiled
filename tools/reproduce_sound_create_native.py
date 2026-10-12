#!/usr/bin/env python3
"""Independent source/native repeats with captured commands and no sharedbuild."""
import argparse,concurrent.futures,json,os,subprocess
from pathlib import Path
from oracle_mp_title_update_display import ROOT,sha,save,encoded

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--directory',type=Path,required=True);a=p.parse_args();d=a.directory.resolve();python=str(ROOT/'.tools/python/bin/python')
 jobs=[('paired-repeat',[python,str(ROOT/'tools/research_sound_create_u5.py'),'--output',str(d/'paired-repeat')]),
 ('source-repeat',[python,str(ROOT/'tools/oracle_sound_create_native.py'),'--output',str(d/'repeated'),'--replay',str(d/'measured/report.json')]),
 ('export-repeat',[python,str(ROOT/'tools/export_sound_create_fixtures.py'),'--report',str(d/'measured/report.json'),'--providers',str(d/'measured/providers.json'),'--output',str(d/'export-repeat')]),
 ('native',[python,str(ROOT/'tools/validate_sound_create_native.py'),'--directory',str(d)])]
 def run(job):
  label,argv=job;r=subprocess.run(argv,cwd=ROOT,capture_output=True,text=True)
  save(d/'reproduction-logs'/(label+'.stdout'),r.stdout.encode());save(d/'reproduction-logs'/(label+'.stderr'),r.stderr.encode())
  assert r.returncode==0,(label,r.stdout,r.stderr)
  return dict(label=label,argv=argv,cwd=str(ROOT),exit_code=r.returncode,stdout_sha256=sha(d/'reproduction-logs'/(label+'.stdout')),stderr_sha256=sha(d/'reproduction-logs'/(label+'.stderr')))
 with concurrent.futures.ThreadPoolExecutor(max_workers=4)as pool:results=list(pool.map(run,jobs))
 results.append(run(('native-repeat',[python,str(ROOT/'tools/validate_sound_create_native.py'),'--directory',str(d/'native-repeat-run'),'--fixture-directory',str(d)])))
 save(d/'reproduction.json',encoded(dict(schema='sound-create-independent-reproduction-v1',tool_sha256=sha(__file__),commands=results,parallel_independent_jobs=True,source_inputs_replayed_only_from_measured_originals=True,whole_archive_or_launcher_freshness_not_claimed=True)))
 print(json.dumps(dict(reproduction_sha256=sha(d/'reproduction.json'),commands=len(results))))
if __name__=='__main__':main()
