#!/usr/bin/env python3
"""Offline original camera read-closure loader; no physics backend or GP capture."""
import hashlib,struct,subprocess
from pathlib import Path
from screen_native_constraint_effect_camera_reference_tu15 import RAM,RAM_SHA,TEXT_SHA
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from validate_native_contact_agent_dispatch_tu15 import sha,save
class TitleMemory:
 def __init__(self):
  assert sha(RAM)==RAM_SHA;self.ram=RAM.read_bytes();self.directory=0xf000
  assert hashlib.sha256(self.read(0x12000,3611948)).hexdigest()==TEXT_SHA
 def physical_word(self,address):
  assert 0<=address<=len(self.ram)-4;return struct.unpack_from('<I',self.ram,address)[0]
 def translate(self,address):
  pde=self.physical_word(self.directory+(address>>22)*4);assert pde&1
  if pde&0x80:return (pde&0xffc00000)+(address&0x3fffff)
  pte=self.physical_word((pde&0xfffff000)+((address>>12)&0x3ff)*4);assert pte&1
  return (pte&0xfffff000)+(address&0xfff)
 def read(self,address,size):
  result=bytearray()
  while size:
   physical=self.translate(address);count=min(size,4096-(address&4095));assert physical+count<=len(self.ram)
   result.extend(self.ram[physical:physical+count]);address+=count;size-=count
  return bytes(result)
 def word(self,address):return int.from_bytes(self.read(address,4),'little')
def build_camera_reference(destination):
 destination.mkdir();m=TitleMemory();assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT)
 text=m.read(0x12000,3611948);assert text==image.read(0x12000,len(text))
 players=m.word(0x4f54f0);slots=[m.word(players+0xc+n*4)for n in range(4)];active=[n for n,v in enumerate(slots)if v!=0xffffffff];assert active
 pages={0x46a000,0x474000,0x4f5000,0x4f6000}
 for address in [players+0xc+n*4 for n in range(4)]:pages.add(address&~4095)
 regions=[];text_file=destination/'text.bin';text_file.write_bytes(text)
 assembly=['.section .original_text,"ax",@progbits',f'.incbin "{text_file}"'];linker=['ENTRY(_start)','SECTIONS { . = 0x12000; .original_text : { *(.original_text) }']
 for n,address in enumerate(sorted(pages)):
  data=m.read(address,4096);file=destination/f'input-{address:08x}.bin';file.write_bytes(data)
  assembly.extend([f'.section .input_{n},"aw",@progbits',f'.incbin "{file}"']);linker.append(f'. = 0x{address:x}; .input_{n} : {{ *(.input_{n}) }}')
  regions.append(dict(address=address,size=len(data),sha256=hashlib.sha256(data).hexdigest()))
 assembly.extend(['.section .arena,"aw",@nobits','.skip 65536','.section .bridge,"ax",@progbits','.globl _start','_start:','int3','mov $1,%eax','xor %ebx,%ebx','int $0x80'])
 linker.extend(['. = 0x60000000; .arena : { *(.arena) }','. = 0x70000000; .bridge : { *(.bridge) }','}'])
 source=destination/'reference.s';script=destination/'link.ld';source.write_text('\n'.join(assembly)+'\n');script.write_text('\n'.join(linker)+'\n')
 obj=destination/'reference.o';elf=destination/'reference';commands=[['as','--32',str(source),'-o',str(obj)],['ld','-melf_i386','-T',str(script),str(obj),'-o',str(elf)]]
 for command in commands:subprocess.run(command,check=True)
 metadata=dict(elf=str(elf),original_sha256=NEW_PIN,original_text_sha256=TEXT_SHA,reference_ram=dict(path=str(RAM),sha256=RAM_SHA),
  page_directory_physical=m.directory,regions=regions,players_pointer=players,local_player_slots=slots,active_slots=active,
  original_scene='retained original TU1.5 RAM; different phase from adjacent GDB clock/object snapshots',
  current_MCC_live_input=False,captured_caller_abi=False,supplied_original_outcomes=False,commands=commands)
 save(destination/'metadata.json',metadata);return metadata
