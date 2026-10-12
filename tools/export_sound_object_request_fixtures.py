#!/usr/bin/env python3
"""Export literal original sound-owner memory/packets; no native expectations."""
import argparse, hashlib, json, struct
from pathlib import Path
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def save(p,b):
 p.parent.mkdir(parents=True,exist_ok=True)
 if p.exists():assert p.read_bytes()==b,('Existing fixture differs',str(p))
 else:p.write_bytes(b)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',required=True,type=Path)
 p.add_argument('--providers',required=True,type=Path);p.add_argument('--output',required=True,type=Path);a=p.parse_args()
 r=json.loads(a.report.read_text());c=json.loads(a.providers.read_text())
 constants={q['name']:struct.unpack('<I',bytes.fromhex(q['raw4_hex']))[0]for q in c['constants']}
 lines=['# Literal original18DDE0 read-only memory and ordered location stores.']
 lines.append('CONSTANTS '+' '.join(f'{constants[n]:08x}'for n in('one_bits','minus_one_bits','z_scale_bits','xy_scale_bits'))+' '+c['zero_vector']['raw12_hex'])
 for n,row in enumerate(r['cases']):
  lines.append(f"CASE {n} {row['phase']} {row['owner']} {row['sound']} {row['fpcw']} {row['result_al']}")
  lines.append('ROOTS '+' '.join(row['roots'][k]for k in('object_pool','physics_pool','sound_system','render_bsp')))
  for local in row['locals']:lines.append(f"LOCAL {local['view']} {local['player']} {local['unit']} {local['mode']}")
  definition=r['definitions'][row['sound']]
  lines.append(f"DEFINITION {row['sound']} {definition['sound20_hex']} {definition['class92_hex']}")
  for region in row['memory']:lines.append(f"MEM {region['address']} {region['raw_hex']}")
  lines.append('MARKER '+row['marker40_hex']);lines.append('BEFORE '+row['location_before_hex']);lines.append('AFTER '+row['location_after_hex'])
  for store in row['stores']:
   before=int.from_bytes(bytes.fromhex(store['before_hex']),'little');after=int.from_bytes(bytes.fromhex(store['after_hex']),'little')
   lines.append(f"STORE {store['instruction']} {store['offset']} {store['size']} {before:08x} {after:08x}")
  for q in row['point_queries']:
   assert 'output8_hex'in q
   lines.append(f"POINT {q['point12_hex']} {q['output8_hex']}")
  lines.append('ENDCASE')
 for row in r['compression_controls']:lines.append(f"COMPRESS {row['input12_hex']} {row['source_fpcw']} {row['result']}")
 save(a.output,('\n'.join(lines)+'\n').encode())
 metadata=dict(schema='literal-sound-object-request-fixtures-v1',tool_sha256=sha(__file__),
  source_report=dict(path=str(a.report),sha256=sha(a.report)),providers=dict(path=str(a.providers),sha256=sha(a.providers)),
  fixture_sha256=sha(a.output),cases=len(r['cases']),stores=sum(len(q['stores'])for q in r['cases']),compression=len(r['compression_controls']),
  scope='Expected bytes and instruction stores are exported only from original execution, never from native implementation. Service backing/local associations reflect the inherited declared original fixture; they do not close a live source producer.')
 save(a.output.with_suffix('.json'),(json.dumps(metadata,indent=2,sort_keys=True)+'\n').encode())
 print(json.dumps(metadata))
if __name__=='__main__':main()
