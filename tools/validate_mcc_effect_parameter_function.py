from pathlib import Path
import argparse,gzip,json,struct,subprocess,tempfile
p=argparse.ArgumentParser();kind=p.add_mutually_exclusive_group(required=True)
kind.add_argument('--probe',type=Path);kind.add_argument('--definition-probe',type=Path)
p.add_argument('--maps',type=Path);p.add_argument('--vectors',type=Path,required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args()
r=json.loads(gzip.decompress(a.vectors.read_bytes()));assert len(r['controls'])==3498
if a.probe:
 rows=[]
 for c in r['controls']:
  b=bytes.fromhex(c['curve_hex']);header=list(struct.unpack_from('<2I',b,4));coefficients=list(struct.unpack_from('<4I',b,0x34))+list(struct.unpack_from('<4I',b,0x64))
  speaker=c['speaker']if c['speaker']<0x80000000 else c['speaker']-0x100000000
  rows.append([c['cw'],c['input'],c['range_input'],c['period_bits'],*header,b[1],speaker,*coefficients,*c['context_bits'],*r['speaker_geometry_words']])
 run=subprocess.run([str(a.probe)],input=''.join(' '.join(map(str,row))+'\n'for row in rows),capture_output=True,text=True,check=True)
 assert not run.stderr,run.stderr
 actual=run.stdout.splitlines();assert len(actual)==3498
 finite=lambda word:(word&0x7fffffff)<0x7f800000
 accepted=0;rejected=0
 for c,out in zip(r['controls'],actual):
  expected=[c['fraction_bits'],c['value_before_parameter_bounds_bits']]
  if all(finite(v)for v in expected):assert out!='rejected'and list(map(int,out.split()))==expected;accepted+=1
  else:assert out=='rejected';rejected+=1
 flags=[bytes.fromhex(c['curve_hex'])[1]for c in r['controls']if'all_flags'in c['label']]
 assert accepted==3468 and rejected==30 and len(flags)==1536 and len(set(flags))==256
 report={'passed':True,'original_controls':3498,'finite_exact_pairs':accepted,'temporary_pre_bound_nonfinite_rejections':rejected,'flag_bytes':256,'install_ack':False,'initializer45_ready':False}
else:
 assert a.maps and all((a.maps/n).is_file()for n in ['mainmenu.map','00a_introduction.map','lockout.map'])
 with tempfile.TemporaryDirectory(prefix='halo2-effect-definitions-')as temporary:
  s=Path(temporary);source=s/'reference.json';output=s/'report.json';source.write_text(json.dumps(r))
  subprocess.run([str(a.definition_probe),str(a.maps),str(source),str(output)],check=True)
  report=json.loads(output.read_text());assert report['passed']and report['guards']==42 and report['retained_proposals_and_controls']==141 and not report['Ready']
a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
