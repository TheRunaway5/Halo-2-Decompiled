"""Read owned kernel149/150 code windows without invoking or modifying them.

Modified Complex4627/xemu evidence only. Function extent/CFG and dependencies
remain to be recovered; these windows do not prove execution or physical Xbox.
"""
import hashlib,json,os,runpy,struct
from pathlib import Path
import gdb
ROOT=Path(__file__).resolve().parents[1]
runpy.run_path(str(ROOT/'tools/capture_xbox_kernel_hardware_20261012.py'))
out=Path(os.environ['HALO2_KERNEL_DATA_OUTPUT'])
read=lambda p,n:bytes(gdb.selected_inferior().read_memory(p,n))
word=lambda p:struct.unpack('<I',read(p,4))[0]
base=0x80010000;pe=base+word(base+0x3c);coff=read(pe+4,20)
optional_bytes=struct.unpack_from('<H',coff,16)[0];optional=read(pe+24,optional_bytes)
exports=read(base+struct.unpack_from('<I',optional,96)[0],40)
ordinal_base,count,_,table_rva=struct.unpack_from('<IIII',exports,16)
records=[]
for ordinal,name,limit in ((149,'KeSetTimer',512),(150,'KeSetTimerEx',2048)):
    assert ordinal_base<=ordinal<ordinal_base+count
    address=base+word(base+table_rva+(ordinal-ordinal_base)*4);section=None
    for i in range(struct.unpack_from('<H',coff,2)[0]):
        raw=read(pe+24+optional_bytes+i*40,40)
        size,rva=struct.unpack_from('<II',raw,8);flags=struct.unpack_from('<I',raw,36)[0]
        if base+rva<=address<base+rva+size:
            assert flags&0x20
            section=dict(name=raw[:8].split(b'\0')[0].decode(),address=base+rva,size=size,flags=flags);break
    assert section is not None
    extent=min(limit,section['address']+section['size']-address)
    code=read(address,extent);(out/(name+'-window.bin')).write_bytes(code)
    records.append(dict(ordinal=ordinal,name=name,address=address,bytes=extent,section=section,
        sha256=hashlib.sha256(code).hexdigest(),window_only=True,function_executed=False))
report=json.loads((out/'report.json').read_text());report['hardware_capture_tool_sha256']=report['tool_sha256']
report['tool_sha256']=hashlib.sha256(Path(__file__).read_bytes()).hexdigest();report['scope']=__doc__
report['timer_exports']=records
(out/'report.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
print(json.dumps(dict(timer_exports=records,guest_writes=[],physical_xbox_proof=False)))
