#!/usr/bin/env python3
"""Export actual SMG patch emitter/creation inputs for unchanged native kernels."""
import argparse,json,struct,subprocess
from pathlib import Path
from oracle_smg_particle_owner_1_1 import ROOT,OUT,FROZEN,sha,save,encoded
from oracle_smg_firing_1_1 import Cache,CACHE_PIN
num=lambda s:int(s,16)
def raw(v,k,n):
 b=bytes.fromhex(v[k])if v[k]is not None else None
 assert b is not None and len(b)==n,(k,len(b)if b else None)
 return b
def word(b,at,n=4):return int.from_bytes(b[at:at+n],'little')
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source',type=Path,default=OUT/'report.json');p.add_argument('--output',type=Path,default=OUT/'native-fixtures');p.add_argument('--creation-native',type=Path,default=ROOT/'build/particle_creation_tests');p.add_argument('--emitter-native',type=Path,default=ROOT/'build/particle_emitter_tests');a=p.parse_args();report=json.loads(a.source.read_text());cache=Cache(ROOT/'assets/disc/maps/lockout.map');assert sha(cache.path)==CACHE_PIN if hasattr(cache,'path')else sha(ROOT/'assets/disc/maps/lockout.map')==CACHE_PIN
 def identity(c):
  sys=raw(c['before'],'system_hex',84);owner,event=struct.unpack_from('<II',sys,16);index=word(sys,14,2);root=cache.tags[owner]['offset'];system=cache.block(cache.block(root+0x14,0x38)[event]+0x30,0x38)[index]
  emitters=cache.block(system+0x30,0xb8);at=num(c['definition_address'])-cache.magic;assert at in emitters
  assert bytes(cache.data[at:at+0xb8]).hex()==c['definition_hex'];return owner,event,index,emitters.index(at)
 creation=[c for c in report['calls']if c['entry']=='00249a20'];emitters=[c for c in report['calls']if c['entry']=='00249540'];inputs=bytearray(struct.pack('<I',len(creation)));expected=bytearray()
 for c in creation:
  alloc=next(q for q in c['checkpoints']if q['instruction']=='00249a75');prefix=next(q for q in c['checkpoints']if q['instruction']=='00249efa');b=c['before'];o=prefix['state'];s=[num(v)for v in c['stack_words']]
  inputs+=struct.pack('<6I',*identity(c),num(alloc['actual_allocated_datum']),num(alloc['actual_particle_address']))
  inputs+=raw(alloc,'actual_particle40_hex',64)+raw(b,'emitter_hex',76)+raw(b,'cache_hex',84)+raw(b,'system_hex',84)+raw(b,'location_hex',52)
  assert b['effect_hex']is not None;inputs+=struct.pack('<I',1)+raw(b,'effect_hex',400)+raw(b,'clock_hex',12)+raw(b,'camera_hex',36)
  inputs+=struct.pack('<5I',s[3],s[4],s[5],s[2],word(raw(b,'seed8_hex',8),4))
  expected+=raw(prefix,'particle40_hex',64)+raw(o,'cache_hex',84)+raw(o,'emitter_hex',76)+raw(o,'seed8_hex',8)[4:8]
 save(a.output/'creation.inputs.bin',inputs);save(a.output/'creation.original.bin',expected)
 ei=bytearray(struct.pack('<I',len(emitters)));ee=bytearray();births=0
 for c in emitters:
  b=c['before'];s=[num(v)for v in c['stack_words']];sys=raw(b,'system_hex',84);nested=[q for q in creation if c['order']<q['order']<c['return_order']];births+=len(nested)
  ei+=struct.pack('<8I',*identity(c),word(sys,12,2),s[0],s[5],bool(c['frame_hex']))
  ei+=bytes.fromhex(c['frame_hex'])if c['frame_hex']else bytes(52)
  ei+=raw(b,'emitter_hex',76)+raw(b,'cache_hex',84)[:68]+struct.pack('<I',len(nested));ee+=struct.pack('<I',len(nested))
  for q in nested:
   sq=[num(v)for v in q['stack_words']];after=raw(q['after'],'emitter_hex',76)
   ei+=struct.pack('<II',word(after,4),word(after,2,2));ee+=struct.pack('<4I',sq[3],sq[4],sq[5],sq[2])+raw(q['before'],'emitter_hex',76)
  ee+=raw(c['after'],'emitter_hex',76)
 save(a.output/'emitter.inputs.bin',ei);save(a.output/'emitter.original.bin',ee)
 results=[]
 for label,exe in(('creation',a.creation_native),('emitter',a.emitter_native)):
  cmd=[str(exe.resolve()),str(ROOT/'assets/disc/maps/lockout.map'),str((a.output/(label+'.inputs.bin')).resolve()),str((a.output/(label+'.original.bin')).resolve())];r=subprocess.run(cmd,capture_output=True,text=True);save(a.output/(label+'.log'),(r.stdout+r.stderr).encode());results.append(dict(kind=label,command=cmd,native_binary_sha256=sha(exe),exit_code=r.returncode,stdout=r.stdout,stderr=r.stderr))
 r=dict(schema='source-smg-particle-native-fixtures-v1',exporter_sha256=sha(__file__),original_report_sha256=sha(a.source),map_sha256=CACHE_PIN,creations=len(creation),emitter_updates=len(emitters),ordered_birth_requests=births,files={p.name:sha(p)for p in sorted(a.output.glob('*.bin'))},native_results=results,
  expected_data='Literal original actual allocated-slot, emitter/cache/system/location/effect/time/camera/request/RNG and successful-prefix output only; no native-produced expected geometry. Native emitter comparison retains only measured lower birth-head/count boundary; creation consumes actual original allocated slot.')
 save(a.output/'report.json',encoded(r));print(json.dumps(r));assert all(q['exit_code']==0 for q in results)
if __name__=='__main__':main()
