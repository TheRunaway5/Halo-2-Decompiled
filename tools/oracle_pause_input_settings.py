#!/usr/bin/env python3
"""Reproducible original Vista input layout/preset evidence."""
import sys,struct,json
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from oracle_vista_lobby_countdown import *
import argparse
parser=argparse.ArgumentParser(description="Execute unchanged Vista45EE72 input tables; requires Unicorn")
parser.add_argument('--exe',type=Path,default=Path('Halo 2 Vista installer/halo2.exe'))
parser.add_argument('--output',type=Path,required=True)
args=parser.parse_args()
image=args.exe.read_bytes()
g=make_machine(image);P=OBJECT
calls={}
trail=[]
def hook(m,pc,n,_):
 trail.append(hex(pc));trail[:]=trail[-15:]
 if pc==STOP:m.emu_stop()
 elif pc==0x437e51:ret_input(m,0)
 elif pc==0x4381fd:ret_input(m,0)
 elif pc==0x40fc03:ret_input(m,0)
g.hook_add(UC_HOOK_CODE,hook)
rows=[]
for stick,button,key in [(s,b,4)for s in range(4)for b in range(4)]+[(0,0,k)for k in range(3)]:
 g.mem_write(P,bytes(0x1680));sp=STACK+0x8000
 g.mem_write(sp,struct.pack('<6I',STOP,P,stick,button,key,1));g.reg_write(UC_X86_REG_ESP,sp)
 try:g.emu_start(0x45ee72,STOP+1,count=500000)
 except Exception as e:raise RuntimeError(f"Original input execution failed at {g.reg_read(UC_X86_REG_EIP):x}: {trail}") from e
 assert g.reg_read(UC_X86_REG_EIP)==STOP
 raw=bytes(g.mem_read(P,0x1680));actions=[]
 for i in range(57):
  offset=20+i*100;count=struct.unpack_from('<I',raw,offset)[0];assert count<=8
  actions.append([list(struct.unpack_from('<3I',raw,offset+4+j*12))for j in range(count)])
 rows.append(dict(stick=stick,button=button,keyboard=key,actions=actions))
args.output.write_text(json.dumps(rows,indent=2))
print('rows',len(rows))
