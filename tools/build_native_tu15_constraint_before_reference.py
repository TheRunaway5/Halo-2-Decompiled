#!/usr/bin/env python3
"""Authenticate typed frame BEFORE banks and declare an offline original ABI.

No guest ABI was captured at22D460. Its ordinary component argument and the
recorded frame FP are the only semantic entry inputs. Original instructions
execute only in the independent physical oracle, never in a native kernel.
"""
import bisect,hashlib,json,lzma,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from replay_live_phantom_hardware_tu15 import reference_text_gaps

def sha(path):
 with Path(path).open('rb')as stream:return hashlib.file_digest(stream,'sha256').hexdigest()
def save(path,value):Path(path).write_text(json.dumps(value,indent=2,sort_keys=True)+'\n')

class BeforeInput:
 def __init__(self,archive,archive_sha,destination):
  archive=archive.resolve();assert sha(archive)==archive_sha
  self.path=destination;destination.mkdir()
  names=subprocess.check_output(['tar','--zstd','-tf',str(archive)],text=True).splitlines()
  assert all(not Path(n).is_absolute()and '..'not in Path(n).parts for n in names)
  types=subprocess.check_output(['tar','--zstd','-tvf',str(archive)],text=True).splitlines()
  assert all(n[0]in '-d'for n in types)
  subprocess.run(['tar','--zstd','-xf',str(archive),'-C',str(destination)],check=True)
  self.data=json.loads((destination/'capture-native-before.json').read_text())
  assert self.data['reference']=='English TU1.5 only'and not self.data['guest_abi_fabricated']
  assert not self.data['captured_after_consumed']and not self.data['physics_result_supplied']
  assert self.data['preserved_x87_depth']==0
  self.regions=sorted(self.data['regions'],key=lambda r:r['address'])
  self.addresses=[r['address']for r in self.regions]
  manifest=json.loads(lzma.decompress(archive.with_name('all-file-hashes.json.xz').read_bytes()))
  for row in self.regions:
   name=row['before_file'];assert not Path(name).is_absolute()and '..'not in Path(name).parts
   proof=manifest.get('./'+name,manifest.get(name));assert proof is not None
   assert (destination/name).stat().st_size==row['size']and sha(destination/name)==proof['sha256']
  self.receipt=dict(archive=str(archive),archive_sha256=archive_sha,metadata_sha256=sha(destination/'capture-native-before.json'),
   verified_bank_files=len(self.regions),before_bytes=sum(r['size']for r in self.regions),captured_guest_ABI=False)
 def read(self,address,size):
  result=bytearray()
  while size:
   index=bisect.bisect_right(self.addresses,address)-1;assert index>=0
   row=self.regions[index];offset=address-row['address'];assert 0<=offset<row['size'],hex(address)
   count=min(size,row['size']-offset)
   with(self.path/row['before_file']).open('rb')as stream:stream.seek(offset);part=stream.read(count)
   assert len(part)==count;result.extend(part);address+=count;size-=count
  return bytes(result)
 def word(self,address,width=4):return int.from_bytes(self.read(address,width),'little')

def build_constraint_reference(source,destination):
 destination.mkdir();assert sha(NEW_DEFAULT)==NEW_PIN
 _,gaps,authentication=reference_text_gaps(Xbe(NEW_DEFAULT),source.path,source.regions,destination)
 sections=[];assembly=[]
 for n,row in enumerate(sorted([*source.regions,*gaps],key=lambda r:r['address'])):
  address,size=row['address'],row['size'];assert not any(address<=p<address+size for p in[0x70000000,0x71000000,0x72000000])
  file=row.get('reference_file',source.path/row.get('before_file',''))
  assembly.append(f'.section .bank{n},"awx"\n.incbin "{file}"')
  sections.append(f'.bank{n} 0x{address:x}:{{*(.bank{n})}}')
 assembly.append('''.code32
.section .bridge,"ax"
.globl _start
_start:
 mov $123,%eax
 mov $1,%ebx
 mov $descriptor,%ecx
 mov $16,%edx
 int $0x80
 test %eax,%eax
 js failed
 mov $7,%eax
 mov %ax,%fs
 fninit
 mov $0x7200f000,%esp
 xor %eax,%eax
 xor %ecx,%ecx
 xor %edx,%edx
 xor %ebx,%ebx
 xor %ebp,%ebp
 xor %esi,%esi
 xor %edi,%edi
 cld
 int3
failed:
 mov $1,%eax
 mov $125,%ebx
 int $0x80
.balign 4
descriptor:.long 0,0x71000000,0xfffff,0x51
.section .private_fs,"aw",@nobits
.skip 4096
.section .private_arena,"aw",@nobits
.skip 65536
''')
 sections.extend(['.bridge 0x70000000:{*(.bridge)}','.private_fs 0x71000000:{*(.private_fs)}','.private_arena 0x72000000:{*(.private_arena)}'])
 (destination/'reference.s').write_text('\n'.join(assembly));(destination/'link.ld').write_text('ENTRY(_start)\nSECTIONS {\n'+'\n'.join(sections)+'\n}\n')
 commands=[['as','--32',str(destination/'reference.s'),'-o',str(destination/'reference.o')],['ld','-melf_i386','-T',str(destination/'link.ld'),str(destination/'reference.o'),'-o',str(destination/'reference')]]
 for command in commands:subprocess.run(command,check=True,capture_output=True)
 receipt=dict(source.receipt,elf=str(destination/'reference'),original_sha256=NEW_PIN,code_authentication=authentication,
  private_stack=[0x72000000,0x72010000],entry_sp=0x7200f000,private_fs=0x71000000,
  entry='Declared original22D460 stack argument component datum; no original caller captured',commands=commands)
 save(destination/'reference.json',receipt);return receipt
