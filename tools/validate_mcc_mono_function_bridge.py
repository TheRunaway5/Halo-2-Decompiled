from pathlib import Path
import argparse,gzip,json,struct,subprocess,tempfile
p=argparse.ArgumentParser()
p.add_argument('--kind',choices=['raw','bridge','boundary'],required=True)
for name in ['probe','maps','functions','builder','boundary','report']:p.add_argument('--'+name,type=Path,required=True)
a=p.parse_args();source=json.loads(gzip.decompress(a.functions.read_bytes()))
if a.kind=='raw':
 rows=[]
 for c in source['controls']:
  b=bytes.fromhex(c['curve_hex']);coefficients=list(struct.unpack_from('<4I',b,0x34))+list(struct.unpack_from('<4I',b,0x64))
  speaker=c['speaker']if c['speaker']<0x80000000 else c['speaker']-0x100000000
  rows.append([c['cw'],c['input'],c['range_input'],c['period_bits'],*struct.unpack_from('<2I',b,4),b[1],speaker,*coefficients,*c['context_bits'],*source['speaker_geometry_words']])
 run=subprocess.run([str(a.probe)],input=''.join(' '.join(map(str,row))+'\n'for row in rows),capture_output=True,text=True,check=True)
 assert not run.stderr and len(rows)==3498
 actual=[list(map(int,line.split()))for line in run.stdout.splitlines()]
 assert actual==[[c['fraction_bits'],c['value_before_parameter_bounds_bits']]for c in source['controls']]
 report=dict(passed=True,original_function_results_exact=3498,nonfinite_original_results_retained=30,initializer45_ready=False)
else:
 with tempfile.TemporaryDirectory(prefix='halo2-mono-function-')as temporary:
  s=Path(temporary);output=s/'report.json'
  if a.kind=='bridge':
   (s/'functions.json').write_text(json.dumps(source));(s/'builder.json').write_bytes(gzip.decompress(a.builder.read_bytes()))
   command=[str(a.probe),str(a.maps),str(s/'functions.json'),str(s/'builder.json'),str(output)]
  else:
   (s/'boundary.json').write_bytes(gzip.decompress(a.boundary.read_bytes()))
   command=[str(a.probe),str(a.maps),str(s/'boundary.json'),str(output)]
  subprocess.run(command,check=True);report=json.loads(output.read_text());assert report['passed'] and not report['initializer45_ready']
  if a.kind=='bridge':assert report['complete_original_builder_converter_matches']==3480 and report['NaN_converter_paths_unresolved']==18 and report['actual_MCC_calls']==126
  else:assert report['original_builder_converter_boundary_matches']==465 and report['constant_infinity_source_clamps']==42
a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
