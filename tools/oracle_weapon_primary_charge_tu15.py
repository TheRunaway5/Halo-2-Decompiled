#!/usr/bin/env python3
"""Whole unchanged1046C0/107140 over declared primary-trigger and clock data."""
import sys,json,struct,hashlib
from pathlib import Path
from oracle_weapon_heat_shader_tu15 import Original
from oracle_weapon_barrel_timing_tu15 import WEAPON,DEF,pack
from weapon_reference_versions import images

def bits(x):return struct.unpack('<I',struct.pack('<f',x))[0]
def main():
 label,image,_=next(row for row in images()if row[0]=='TU1.5');original=Original(label,image);g=original.g
 trigger,clock=0x1010000,0x1011000
 g.mem_write(DEF+0x2c8,pack([1,trigger]));g.mem_write(0x51df54,pack([clock]))
 rows=[]
 for mode in (0,1,2,3,4,127,128,255):
  for ticks in (-32768,-30,-1,0,1,5,15,29,30,31,32767):
   for charging in (0.,.5,1.,-1.,float('inf'),float('nan')):
    for step in (0.,1/30,1/60):
     g.mem_write(WEAPON+0x20c,bytes([mode,0])+struct.pack('<h',ticks)+bytes(8))
     g.mem_write(trigger+0x1c,pack([bits(charging)]));g.mem_write(clock+4,pack([bits(step)]))
     value,enabled=original.call(0,0,0x0f000574)
     rows.append(dict(mode=mode,ticks=ticks,charging=bits(charging),step=bits(step),value=value,enabled=enabled))
 fixture=Path('tests/fixtures/weapon_primary_charge_tu15_original.json');fixture.write_text(json.dumps(rows,separators=(',',':'))+'\n')
 out=Path('analysis/weapon-effects-20261013/live-tu15/weapon-primary-charge-oracle.json')
 out.write_text(json.dumps(dict(scope=__doc__,cases=len(rows),original_sha256=hashlib.sha256(image.data).hexdigest(),supplied_services=[],instruction_patches=[],executed_instructions=original.instructions,fixture_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest()),indent=2)+'\n')
 print('Whole original primary charge',len(rows),'cases')
if __name__=='__main__':main()
