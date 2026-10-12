"""Read the owned modified kernel at its first instruction, before initialization.

The debugger uses an execution breakpoint, without writing guest memory. Only
section metadata, loader-initial INIT bytes and named timer/interrupt records
are retained. No EEPROM, key, scheduler snapshot or title state is retained.
"""
import hashlib,json,os,struct
from pathlib import Path
import gdb
out=Path(os.environ['HALO2_KERNEL_DATA_OUTPUT']);out.mkdir(parents=True,exist_ok=False)
read=lambda p,n:bytes(gdb.selected_inferior().read_memory(p,n))
word=lambda p:struct.unpack('<I',read(p,4))[0]
base=0x80010000
assert int(gdb.parse_and_eval('$eip'))==0x8005504c
assert read(base,2)==b'MZ'
pe=base+word(base+0x3c);assert read(pe,4)==b'PE\0\0'
coff=read(pe+4,20);optional_bytes=struct.unpack_from('<H',coff,16)[0]
optional=read(pe+24,optional_bytes)
assert base+struct.unpack_from('<I',optional,16)[0]==0x8005504c
sections=[]
for i in range(struct.unpack_from('<H',coff,2)[0]):
 raw=read(pe+24+optional_bytes+i*40,40)
 name=raw[:8].split(b'\0')[0].decode();size,rva,raw_size,raw_offset=struct.unpack_from('<IIII',raw,8)
 flags=struct.unpack_from('<I',raw,36)[0]
 row=dict(name=name,address=base+rva,bytes=size,raw_bytes=raw_size,raw_offset=raw_offset,flags=flags)
 if name=='INIT':
  assert flags&0x20 and size<1024*1024
  code=read(base+rva,size);(out/'kernel-init.bin').write_bytes(code)
  row['sha256']=hashlib.sha256(code).hexdigest()
 sections.append(row)
records=[]
for name,address,size in [('pending-interrupts',0x80035a1c,4),('current-irql',0x80035c00,4),
 ('interrupt-policy',0x80035c08,4),('system-time',0x8003a880,12),('interrupt-time',0x8003a890,12)]:
 data=read(address,size);(out/(name+'.bin')).write_bytes(data)
 records.append(dict(name=name,address=address,bytes=size,sha256=hashlib.sha256(data).hexdigest(),hex=data.hex()))
metadata=read(base,64)+read(pe,24+optional_bytes+len(sections)*40)
(out/'entry-metadata.bin').write_bytes(metadata)
report=dict(schema=1,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
 reference='owned modified Complex4627 in xemu 0.8.136',phase='before kernel entry instruction',
 kernel_base=base,entry=0x8005504c,sections=sections,records=records,
 guest_memory_writes=[],code_patches=[],title_executed=False,physical_xbox_proof=False)
(out/'report.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
print(json.dumps(report))
