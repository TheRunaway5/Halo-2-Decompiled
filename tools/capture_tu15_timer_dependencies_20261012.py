"""Read the six actual direct dependencies of the owned timer149/150 bodies.

All records are code windows, not assumed function extents or execution proof.
No guest data/register/code writes or title execution. Modified BIOS reference.
"""
import hashlib,json,os,runpy,struct
from pathlib import Path
import gdb
ROOT=Path(__file__).resolve().parents[1]
runpy.run_path(str(ROOT/'tools/capture_tu15_timer_exports_20261012.py'))
out=Path(os.environ['HALO2_KERNEL_DATA_OUTPUT']);report=json.loads((out/'report.json').read_text())
read=lambda p,n:bytes(gdb.selected_inferior().read_memory(p,n))
records=[]
for p,n in ((0x80014348,256),(0x80018ba0,512),(0x8001a3c3,1024),(0x8001c033,2048),(0x8001c4cf,4096),(0x8002dab0,512)):
    # All addresses are actual CALL targets authenticated by the retained CFG.
    assert 0x800102c0<=p<p+n<=0x800102c0+153136
    b=read(p,n);name=f'{p:08x}-window.bin';(out/name).write_bytes(b)
    records.append(dict(address=p,bytes=n,file=name,sha256=hashlib.sha256(b).hexdigest(),window_only=True,function_executed=False))
report['timer_exports_tool_sha256']=report['tool_sha256']
report['tool_sha256']=hashlib.sha256(Path(__file__).read_bytes()).hexdigest();report['scope']=__doc__;report['timer_dependencies']=records
(out/'report.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
print(json.dumps(dict(dependencies=records,guest_writes=[])))
