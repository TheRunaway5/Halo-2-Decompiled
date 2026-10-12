#!/usr/bin/env python3
"""Replay captured customization1BF50 arguments with literal mainmenu metadata.

Original ordinary shader LOD selection executes unchanged. Actual1BF50 entry
captures supply metric, bias, global view bank and modifier; template thresholds
are copied from the original map. This proves selection for these inputs, not
that native collectors produce those metrics or that every later draw is equal.
"""
import hashlib,json,struct,subprocess
from pathlib import Path
from audit_rendering import Cache
from oracle_multiplayer_hud_positions import Retail,ROOT,PIN
from unicorn.x86_const import UC_X86_REG_EAX

base=ROOT/'analysis/frontend/customization'
capture=base/'original-preview-shader.json'
captured=json.loads(capture.read_text())
c=Cache(ROOT/'assets/disc/maps/mainmenu.map');r=Retail()
r.g.mem_map(0x80000000,0x4000000)
loaded=set()
def load(datum,group):
 t,p=c.tag(datum,group)
 if datum not in loaded:
  r.g.mem_write(t['address'],bytes(c.data[p:p+t['size']]))
  r.g.mem_write(r.table+(datum&65535)*16,struct.pack('<4I',int.from_bytes(group.encode(),'big'),datum,t['address'],t['size']))
  loaded.add(datum)
 return t,p
result=r.alloc(bytes(4));rows=[];lines=[]
for ordinal,record in enumerate(captured['rows']):
 if record['pc']!=0x1bf50:continue
 stack=bytes.fromhex(record['stack_hex']);ret,datum,metric=struct.unpack_from('<3I',stack)
 t,root=load(datum,'shad');pp=c.block(root+0x20,124);assert len(pp)==1
 _,stem=load(c.u32(pp[0]),'stem');tp=c.block(stem+0x58,40);assert len(tp)==1
 thresholds=[c.u32(lod+6) for lod in c.block(tp[0],10)]
 bias=record['registers']['eax'];assert bias==c.u16(root+0x3c)
 bank=bytes.fromhex(record['lod_multipliers_hex']);globals_=bytes.fromhex(record['globals_4ba010'])
 r.g.mem_write(0x485b28,bank);r.g.mem_write(0x4ba010,globals_)
 r.run(0x1bf50,argv=[datum,metric,result],registers=[(UC_X86_REG_EAX,bias)])
 expected=[r.r(result),r.g.reg_read(UC_X86_REG_EAX)&255]
 view=struct.unpack('<7I',bank);modifier=globals_[9]
 row=[1,bias,modifier,metric,*view,len(thresholds),*thresholds]
 lines.append(' '.join(f'{v:x}' for v in [len(row),*row]))
 rows.append(dict(capture_ordinal=ordinal,return_address=f'{ret:08x}',shader=f'{datum:08x}',name=t['name'],bias=bias,metric_bits=f'{metric:08x}',modifier=modifier,view_bits=[f'{v:08x}'for v in view],threshold_bits=[f'{v:08x}'for v in thresholds],original_choice=expected))
binary=base/'preview-shader-lod-kernel'
process=subprocess.run([str(binary),'--probe'],input='\n'.join(lines)+'\n',text=True,capture_output=True,check=True)
outputs=[[int(v,16)for v in line.split()] for line in process.stdout.splitlines()]
assert len(outputs)==len(rows)==32
for row,output in zip(rows,outputs):row['native_choice']=output;row['different_words']=sum(a!=b for a,b in zip(row['original_choice'],output))
pass_selectors=[dict(capture_ordinal=i,shader=f"{record['registers']['ebp']:08x}",layer=record['registers']['edi'],implementation=record['registers']['eax']) for i,record in enumerate(captured['rows']) if record['pc']==0x4cc54]
report=dict(original_xbe_sha256=PIN,mainmenu_sha256=hashlib.sha256(c.data).hexdigest(),capture_sha256=hashlib.sha256(capture.read_bytes()).hexdigest(),native_kernel_binary_sha256=hashlib.sha256(binary.read_bytes()).hexdigest(),native_sources={path:hashlib.sha256((ROOT/path).read_bytes()).hexdigest() for path in ['src/source_shader_lod.cpp','src/source_geometry_lod.cpp','tests/source_shader_lod_tests.cpp']},native_build_command='c++ -std=c++20 -O2 -fno-fast-math -ffp-contract=off -ffunction-sections -fdata-sections -Wl,--gc-sections -Iinclude tests/source_shader_lod_tests.cpp src/source_shader_lod.cpp src/source_geometry_lod.cpp -o analysis/frontend/customization/preview-shader-lod-kernel',method=__doc__.splitlines()[2:],rows=rows,actual_4cc54_pass_selectors=pass_selectors,cases=len(rows),different_words=sum(row['different_words']for row in rows))
(base/'preview-shader-lod-choice-audit.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({k:v for k,v in report.items() if k!='rows'},indent=2))
for datum in sorted({row['shader'] for row in rows}):print(datum,[(row['original_choice'],row['metric_bits'])for row in rows if row['shader']==datum])
