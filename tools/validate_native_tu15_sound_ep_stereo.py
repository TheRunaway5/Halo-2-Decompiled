from pathlib import Path
import argparse,gzip,json,subprocess
p=argparse.ArgumentParser();p.add_argument('--kind',choices=['numeric','loader'],required=True)
for name in ['probe','vectors','report']:p.add_argument('--'+name,type=Path,required=True)
a=p.parse_args();fixture=json.loads(gzip.decompress(a.vectors.read_bytes()));rows=fixture[a.kind]
assert len(rows)==(512 if a.kind=='numeric'else 144)
text=''.join(' '.join(map(str,[r['entry_status'],r['scale_enabled'],r['scale'],*[v for c in r['inputs']for v in c]]))+'\n'for r in rows)
command=[str(a.probe)]+(['--pack']if a.kind=='loader'else[])
r=subprocess.run(command,input=text,text=True,capture_output=True,check=True);assert not r.stderr
lines=r.stdout.splitlines();assert len(lines)==len(rows)
for row,line in zip(rows,lines):
 values=line.split();assert list(map(int,values[:526]))==row['original_words'],row['label']
 if a.kind=='loader':assert len(values)==527 and values[526]==row['original_fifo_hex'],row['label']
 else:assert len(values)==526
report=dict(passed=True,original_cases=len(rows),exact_numeric_words=526*len(rows),original_FIFO_byte_matches=len(rows)if a.kind=='loader'else 0,entry_CCR_values=256 if a.kind=='numeric'else sorted(set(row['entry_status']for row in rows)),native_install_status_clock_or_initializer45_ready=False)
a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
