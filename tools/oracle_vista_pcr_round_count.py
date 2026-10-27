#!/usr/bin/env python3
"""Offline original PC setter controls for PCR's common round-count field."""
import hashlib,json,struct
from pathlib import Path
from oracle_vista_pcr_publication import Original,VARIANT,OBJ,ROOT,SHA,VALUES,u32

def main():
 image=(ROOT/'Halo 2 Vista installer/halo2.exe').read_bytes()
 assert hashlib.sha256(image).hexdigest()==SHA
 source=Original(image);u=source.u;source.call(u32(u,0x864d88),(OBJ,OBJ+4,OBJ+8))
 descriptor=[u32(u,OBJ+i*4)for i in range(3)]
 prior=ROOT/'tests/fixtures/vista_pcr_publication_original.tsv'
 rows=prior.read_text().splitlines()
 assert SHA in rows[0]
 assert descriptor==list(map(int,next(row for row in rows if row.startswith('Q 0 ')).split()[2:]))
 reference=ROOT/'tests/fixtures/source_game_variant_original.tsv'
 original_rounds={int(parts[3]):bytes.fromhex(parts[4])for row in reference.read_text().splitlines()
   if (parts:=row.split()) and parts[:3]==['S','0','0']}
 cases=[]
 for teams in (False,True):
  for value in VALUES:
   source.variant()
   if teams:
    source.call(0x63a8c2,(VARIANT,91,1));source.call(0x63a8c2,(VARIANT,10,8))
   before=bytes(u.mem_read(VARIANT,0x130))
   source.call(0x63a8c2,(VARIANT,0,value))
   after=bytes(u.mem_read(VARIANT,0x130));got=source.call(0x63aa54,(VARIANT,0))
   assert after[76:80]==original_rounds[value][76:80]
   assert after[:76]==before[:76] and after[80:]==before[80:]
   if got&0x80000000:got-=0x100000000
   cases.append(dict(teams=teams,value=value,before=before.hex(),after=after.hex(),read=got))
 fixture=dict(schema='H2_PC_PCR_ROUND_COUNT_ORIGINAL_V1',source_sha256=SHA,
  descriptor=descriptor,cases=cases,unique_instructions=len(source.executed),instruction_visits=source.visits,
  prior_descriptor_sha256=hashlib.sha256(prior.read_bytes()).hexdigest(),
  original_variant_reference_sha256=hashlib.sha256(reference.read_bytes()).hexdigest(),
  boundary='Original PC setter/getter and descriptor unchanged; Xbox descriptor identity checked. Offline oracle only, no runtime or onboarding image dependency.')
 target=ROOT/'tests/fixtures/vista_pcr_round_count_original.json'
 assert not target.exists(),'Preserve existing reference evidence'
 target.write_text(json.dumps(fixture,indent=2)+'\n')
 print(json.dumps(dict(cases=len(cases),descriptor=descriptor,fixture_sha256=hashlib.sha256(target.read_bytes()).hexdigest())))

if __name__=='__main__':main()
