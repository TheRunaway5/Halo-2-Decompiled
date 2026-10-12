"""Read the owned kernel301 routine and its complete immutable mapping tables.

This is a read-only modified Complex4627/xemu resource capture, not execution
or physical-Xbox proof. No title is selected and no guest state is written.
"""
import hashlib,json,os,runpy,struct
from pathlib import Path
import gdb
ROOT=Path(__file__).resolve().parents[1]
runpy.run_path(str(ROOT/'tools/capture_tu15_status_export_20261012.py'))
out=Path(os.environ['HALO2_KERNEL_DATA_OUTPUT'])
read=lambda p,n:bytes(gdb.selected_inferior().read_memory(p,n))
sha=lambda b:hashlib.sha256(b).hexdigest()
report=json.loads((out/'report.json').read_text())
assert report['status301']['address']==0x80023653
window=(out/'RtlNtStatusToDosError-window.bin').read_bytes()
assert sha(window)=='e8ee8966beaa3eed42d2f2e75f033ccad293449b5c79452695076607406ae655'
# Exact constants encoded by the retained complete 0xD0-byte routine.
# 103 entries {ULONG start, USHORT count, USHORT width}; each next start is
# read by the loop before its bounded index test. Retain that final read too.
function=window[:0xd0];assert function[-3:]==b'\xc2\x04\x00'
ranges=read(0x80011f88,103*8+4)
rows=[struct.unpack_from('<IHH',ranges,i*8) for i in range(103)]
assert rows[-1]==(0,0,0),'Actual final record must be retained, including its zero width'
assert all(n>0 and w in (1,2) for start,n,w in rows[:-1]),str(rows)
assert all(rows[i][0]<rows[i+1][0] for i in range(101)),str(rows)
words=sum(n*w for start,n,w in rows)
assert words*2<=0x80011f88-0x80011988,str(words)
values=read(0x80011988,words*2)
records=[]
for name,address,data in [('function',0x80023653,function),('ranges',0x80011f88,ranges),('values',0x80011988,values)]:
    (out/(name+'.bin')).write_bytes(data)
    records.append(dict(name=name,address=address,bytes=len(data),sha256=sha(data)))
report['export_probe_tool_sha256']=report['tool_sha256']
report['tool_sha256']=sha(Path(__file__).read_bytes())
report['scope']=__doc__;report['status301_tables']=dict(records=records,ranges=103,value_words=words,function_executed=False)
(out/'report.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
print(json.dumps(report['status301_tables']))
