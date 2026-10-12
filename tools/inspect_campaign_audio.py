#!/usr/bin/env python3
from pathlib import Path
import csv, hashlib, struct, json
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
root=Path(__file__).resolve().parents[1]
(root/'analysis/campaign').mkdir(parents=True,exist_ok=True)
image=(root/'assets/disc/default.xbe').read_bytes()
expected='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
assert hashlib.sha256(image).hexdigest()==expected
base=struct.unpack_from('<I',image,0x104)[0]
count,table=struct.unpack_from('<II',image,0x11c)
sections=[struct.unpack_from('<5I',image,table-base+i*56)[1:] for i in range(count)]
def read(address,n):
 for start,size,offset,raw in sections:
  if start<=address<start+size:
   assert address+n<=start+raw
   return image[offset+address-start:offset+address-start+n]
 raise KeyError(hex(address))
def string(address):
 result=bytearray()
 for i in range(256):
  value=read(address+i,1)
  if value==b'\0':return result.decode('ascii')
  result.extend(value)
 raise ValueError(f'Unterminated retail string at {address:08x}')
with (root/'tools/reference/halo2-decompiled/config/functions.csv').open() as f:
 records={int(r['va'],16):r for r in csv.DictReader(f)}
cs=Cs(CS_ARCH_X86,CS_MODE_32)
def dump(a):
 r=records[a];n=int(r['size'])
 (root/f'analysis/campaign/audio-research-{a:x}.txt').write_text(f'XBE SHA256 {expected}\nRetail {a:08x}, {n} bytes, manifest status {r["status"]}\n'+''.join(f'{i.address:08x} {i.bytes.hex():<20} {i.mnemonic} {i.op_str}\n' for i in cs.disasm(read(a,n),a)))
classes=[]
for index in range(54):
 ptr=struct.unpack('<I',read(0x470090+index*4,4))[0]
 classes.append({'index':index,'pointer':f'{ptr:08x}','name':string(ptr)})
functions=[]
for op in [0x239,0x23b,0x2ee,0x2ef,0x2f0,0x267,0x268,0x372,0x386]:
 ptr=struct.unpack('<I',read(0x4744e0+op*4,4))[0]
 rtype,flags,evaluator,parse,argc=struct.unpack('<HHIIH',read(ptr,14))
 types=list(struct.unpack('<'+'H'*argc,read(ptr+14,2*argc)))
 functions.append(dict(opcode=f'{op:03x}',definition=f'{ptr:08x}',return_type=rtype,flags=flags,evaluator=f'{evaluator:08x}',parse=f'{parse:08x}',argument_types=types))
 if evaluator in records:dump(evaluator)
for a in [0x2197f0,0x2195f0,0x221900,0x221980,0x18d170,0x18d090,0x13bf00,0x13bff0,0x1251e0,0x127010,0x188ac0,0x22c041,0x22c075,0x125b40,0x1269f0,0xc18e0,0x17ca10,0x232371,0x23223d,0x18bb80,0x18ae80,0x18a7b0,0x21a250,0x21b0e0,0x21fa80,0x221850,0x2218a0,0x2218e0,0x2218f0,0x123d40,0x123e20,0x123f60,0x2151f0,0x2152e0,0x199520,0x199540,0x1264c0,0x1889d0,0x188d60,0x21aeb0,0x21cc60,0x21ce60]:
 if a in records:dump(a)
module_rows=[]
for i in range(68):
 values=struct.unpack('<9I',read(0x440df0+i*36,36))
 if 0x221850 in values:
  module_rows.append(dict(index=i,address=f'{0x440df0+i*36:08x}',values=[f'{v:08x}' for v in values]))
restore_callbacks=struct.unpack('<30I',read(0x46e320,120))
result={'xbe_sha256':expected,'sound_class_table':'00470090','classes':classes,'functions':functions,'sound_class_lifecycle':module_rows,'restore_callbacks':[dict(index=i,address=f'{0x46e320+i*4:08x}',callback=f'{v:08x}',phase='save_before' if i<4 else 'save_after' if i==4 else 'restore_before' if i<10 else 'restore_after') for i,v in enumerate(restore_callbacks)]}
p=root/'analysis/campaign/audio-research-retail.json';p.write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
