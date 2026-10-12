#!/usr/bin/env python3
"""Export scalar native API fixtures from unchanged original camera outputs."""
import argparse,hashlib,json,struct
from pathlib import Path

def word(raw,at):return f'{struct.unpack_from("<I",raw,at)[0]:08x}'
def state(v):
 raw=bytes.fromhex(v['observer_hex']);p=bytes.fromhex(v['packet_hex'])
 return [str(raw[0xb5]),word(p,0),word(raw,8),str(raw[0x98]),
  *[word(raw,n)for n in (0x130,0x30,0xac,0x1a4,0x1d8,0x344)],
  *[word(raw,n)for n in (0x20c,0x240,0x274,0x2a8,0x2dc,0x310)],word(raw,0xf0)]
def packet(v):
 p=bytes.fromhex(v['packet_hex'])
 return [word(p,0),word(p,0x28),word(p,0x88),str(p[0x90]),word(p,0xa4)]
def export(source,destination):
 r=json.loads(source.read_text());a=r['authored'];c=a['immutable_constants'];init=r['stages'][6]['after']['runtime_fov_globals']
 assert r['stages'][6]['name']=='source_observer_constructor'
 constants=[init[n]for n in ('005563a4','00565fe4','00565fe8','005563a8','005563ac')]
 constants.extend(c[n]for n in ('0046570c','00465708','00465030','0047885c','004653b8','004701c8',
  '0046502c','00465044','00465060','00465068','0046506c','00465070','00465078','0046507c','00465080','00465084','00465218'))
 caps=bytes.fromhex(a['acceleration_caps']);constants.append(word(caps,16))
 double=f'{struct.unpack("<Q",bytes.fromhex(a["x87_reciprocal_numerator"]))[0]:016x}'
 rows=[['C',*constants,double],['I',*state(r['stages'][6]['after'])]]
 for stage in r['stages'][7:]:
  mag=next(v['return_xmm0_low']for v in stage['calls']if v['entry']=='000c9be0')
  rows.append(['L',stage['name'],a['unit_fov'],mag,stage['return_xmm0_low']])
 for f in r['frames']:
  output=f['packet']['after'];call=next(v for v in f['packet']['calls']if v['entry']=='000c9be0')
  fov=next(v['return_xmm0_low']for v in f['packet']['calls']if v['entry']=='00188b60')
  rows.append(['F',str(f['frame']),str(f['selector']),a['unit_fov'],call['return_xmm0_low'],fov,
   f['motion']['supplied_boundary']['host_frame_dt_bits'],f['motion']['after']['dt'],output['persistent_fov'],*packet(output)])
  for phase in ('registration','adoption','motion','render_packet'):
   rows.append(['S',str(f['frame']),phase,*state(f[phase]['after']),*packet(f[phase]['after'])])
 text='\n'.join(' '.join(row)for row in rows)+'\n'
 destination.write_text(text)
 print(json.dumps(dict(rows=len(rows),source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),fixture_sha256=hashlib.sha256(text.encode()).hexdigest())))
def main():
 p=argparse.ArgumentParser();p.add_argument('source',type=Path);p.add_argument('destination',type=Path);a=p.parse_args();export(a.source,a.destination)
if __name__=='__main__':main()
