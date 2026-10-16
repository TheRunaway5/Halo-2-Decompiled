#!/usr/bin/env python3
"""Record untouched TU1.5 quaternion-to-matrix arithmetic and x87 state."""
import argparse,hashlib,json,struct,subprocess,sys
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--directory',required=True,type=Path);p.add_argument('--fixture',required=True,type=Path);a=p.parse_args()
 assert hashlib.sha256(NEW_DEFAULT.read_bytes()).hexdigest()==NEW_PIN
 x=Xbe(NEW_DEFAULT);c=_Code(x,'.text');body=complete_body(c,0x37c5d0)
 assert not body.tables and not any(c.at(q).mnemonic=='call' for q in body.code)
 a.directory.mkdir(parents=True,exist_ok=False);a.directory=a.directory.resolve();text=next(s for s in x.sections if s.name=='.text');(a.directory/'text.bin').write_bytes(x.section_bytes(text))
 cases=[];vectors=((0,0,0,1),(.1,.2,.3,.9),(.3,.2,.1,.9),(.25,.125,.5,.75),(-.1,.3,-.2,.9),(.70710677,0,0,.70710677),(0,.70710677,0,.70710677),(.1234567,-.2345678,.3456789,.9012345))
 for index,quaternion in enumerate(vectors):
  root=a.directory/f'case{index}';root.mkdir();stack=0x60001000;gp=[0x12345678,0x2002000,0x22446688,0x11223344,stack,0x55667788,0x99aabbcc,0xddeeff00]
  initial=dict(gp=gp,xmm=[[0]*4 for _ in range(8)],eflags=2,fs_base=0,x87_values=[],x87_status=0x100,x87_control_word=0x37f,mxcsr=0x1f80)
  memory=bytearray(0x3000);struct.pack_into('<4f',memory,0x1000,*quaternion);memory[0x2000:0x2040]=bytes([0x5a])*64
  scratch=bytearray(0x2000);struct.pack_into('<2I',scratch,0x1000,0x60300000,0x2001000)
  buffers=((text.va,None,'../text.bin'),(0x46a0d0,x.read(0x46a0d0,4),'constant.bin'),(0x2000000,memory,'data.bin'),(0x60000000,scratch,'stack.bin'))
  regions=[]
  for address,data,name in buffers:
   if data is not None:(root/name).write_bytes(data)
   regions.append(dict(address=address,size=len(data) if data is not None else len(x.section_bytes(text)),before_file=name))
  record=dict(entry=0x37c5d0,reference='English TU1.5 only',boundary='Declared quaternion callback inputs; original hardware result recording only',before=initial,after=initial,regions=regions,**{'return':0x60300000})
  (root/'capture.json').write_text(json.dumps(record,indent=2)+'\n')
  out=a.directory/f'original{index}';r=subprocess.run([sys.executable,str(Path(__file__).with_name('replay_live_phantom_hardware_tu15.py')),'--capture',str(root),'--output',str(out),'--return-breakpoints'],capture_output=True,text=True)
  assert r.returncode in (0,1),(r.stdout,r.stderr)
  report=json.loads((out/'report.json').read_text());actual=(out/'output.bin').read_bytes();offset=276;small=[]
  for region in sorted(regions,key=lambda r:r['address']):
   final=actual[offset:offset+region['size']];offset+=region['size']
   if region['address']!=text.va:small.append(dict(address=region['address'],before=(root/region['before_file']).read_bytes().hex(),after=final.hex()))
  assert offset==len(actual)
  cases.append(dict(quaternion=quaternion,entry=0x37c5d0,input=initial,regions=small,original=report['original_abi'],eflags_mask=report['eflags_comparison_mask']))
  print(index,hex(report['original_abi']['x87_status']),flush=True)
 fixture=dict(reference='English TU1.5 only',original_sha256=NEW_PIN,entry=0x37c5d0,complete_sites=len(body.code),boundary='Eight declared quaternion callback inputs, untouched original i386 execution; host IF excluded, raw flags retained',cases=cases)
 a.fixture.parent.mkdir(parents=True,exist_ok=True);a.fixture.write_text(json.dumps(fixture,indent=2)+'\n')
 print('PASS original hardware quaternion cases',len(cases))
if __name__=='__main__':main()
