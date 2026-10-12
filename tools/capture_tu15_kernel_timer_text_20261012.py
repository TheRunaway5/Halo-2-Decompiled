"""Read only the owned kernel's complete .text for timer dependency recovery.

No mutable kernel scheduler/clock state, keys, title state, guest writes or
instruction execution is captured. Modified Complex4627/xemu reference only.
"""
import hashlib,json,os,runpy,struct
from pathlib import Path
import gdb
ROOT=Path(__file__).resolve().parents[1]
runpy.run_path(str(ROOT/'tools/capture_xbox_kernel_hardware_20261012.py'))
out=Path(os.environ['HALO2_KERNEL_DATA_OUTPUT']);report=json.loads((out/'report.json').read_text())
read=lambda p,n:bytes(gdb.selected_inferior().read_memory(p,n))
word=lambda p:struct.unpack('<I',read(p,4))[0]
base=report['kernel_base'];pe=base+word(base+0x3c);coff=read(pe+4,20)
optional_bytes=struct.unpack_from('<H',coff,16)[0];found=[]
for i in range(struct.unpack_from('<H',coff,2)[0]):
    raw=read(pe+24+optional_bytes+i*40,40);name=raw[:8].split(b'\0')[0].decode()
    if name!='.text':continue
    size,rva=struct.unpack_from('<II',raw,8);flags=struct.unpack_from('<I',raw,36)[0]
    assert flags&0x20 and not flags&0x80000000 and size<=1024*1024
    code=read(base+rva,size);(out/'kernel-text.bin').write_bytes(code)
    found.append(dict(address=base+rva,bytes=size,flags=flags,name=name,sha256=hashlib.sha256(code).hexdigest()))
assert len(found)==1
report['hardware_capture_tool_sha256']=report['tool_sha256'];report['tool_sha256']=hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
report['scope']=__doc__;report['code_section']=found[0];report['mutable_kernel_state_captured']=False
(out/'report.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
print(json.dumps(dict(code_section=found[0],guest_writes=[])))
