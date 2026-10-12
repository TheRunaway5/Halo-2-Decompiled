#!/usr/bin/env python3
"""Unchanged retail132D60 sphere-cluster construction over retained Lockout BSP.

Explicit containing cluster, sphere center and radius vary. Original portal
geometry, traversal order, visited epoch and all132D60/14A5B0/14A370/sort
instructions execute without interception. No light admission/selection claim.
"""
import argparse,hashlib,json,struct
from pathlib import Path
from unicorn import x86_const as R
from oracle_effect_light_snapshot import Snapshot,u

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--snapshot',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 s=Snapshot(a.snapshot);sha=hashlib.sha256(s.ram).hexdigest();assert sha=='68271994d841fcc777be6b2b9271c311cb784c00521020aa8b20e9694c45ff0e'
 bsp=u(s.read(0x4e0348,4));count=u(s.read(bsp+0x9c,4));clusters=u(s.read(bsp+0xa0,4));rows=[]
 for cluster in range(-1,count):
  bounds=struct.unpack('<6f',s.read(clusters+max(cluster,0)*0xb0+0x54,24));position=[(bounds[i]+bounds[i+1])*.5 for i in (0,2,4)]
  for radius in (0.,.01,.371,1.,2.,7.293,32.):
   g=s.guest();g.mem_map(0x70000000,0x40000);ctx=0x70001000;bank=0x70005000;stack=0x7003e000;stop=0x7003ff00
   g.mem_write(ctx,struct.pack('<I',bank));g.mem_write(ctx+8,struct.pack('<h',cluster));g.mem_write(ctx+0x2a68,struct.pack('<4f',*position,radius))
   g.mem_write(stack,struct.pack('<I',stop));g.reg_write(R.UC_X86_REG_ESP,stack);g.reg_write(R.UC_X86_REG_EAX,ctx);g.reg_write(R.UC_X86_REG_FPCW,0x023f);g.reg_write(R.UC_X86_REG_MXCSR,0x1fa0)
   g.emu_start(0x132d60,stop,count=5000000);assert g.reg_read(R.UC_X86_REG_EIP)==stop
   n=struct.unpack('<h',g.mem_read(bank+0xa6c,2))[0];assert 0<=n<=512
   created=[struct.unpack('<h',g.mem_read(bank+0xa6e+i*0x1a,2))[0]for i in range(n)]
   sorted_indices=list(struct.unpack('<'+'I'*n,g.mem_read(bank+6000,n*4)))
   rows.append(dict(input=[cluster&0xffffffff,*struct.unpack('<4I',struct.pack('<4f',*position,radius))],output=[n,*[v&0xffffffff for v in created],*sorted_indices]))
 (a.output/'report.json').write_text(json.dumps(dict(boundary=__doc__,snapshot_sha256=sha,entry='00132d60',intercepted_instructions=0,cases=rows,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest()),indent=2)+'\n')
 (a.output/'fixture.tsv').write_text('# Unchanged132D60 over actual Lockout BSP. Explicit containing cluster/sphere.\n'+'\n'.join(' '.join(f'{v:x}'for v in row['input'])+' | '+' '.join(f'{v:x}'for v in row['output'])for row in rows)+'\n');print('PASS',len(rows),'whole original sphere-cluster calls')
if __name__=='__main__':main()
