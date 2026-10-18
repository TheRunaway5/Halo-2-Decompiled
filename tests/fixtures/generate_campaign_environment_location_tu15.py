"""Unchanged original main-location prefix on an authenticated native input bank.

No original code, query return, leaf, cluster or location result is supplied.
The submitting position and previous per-view location are caller controls.
This stops before14DE90 and makes no whole-main, live-script or GPU claim.
"""
from pathlib import Path
import argparse,hashlib,json,struct,sys,time,traceback
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools'))
from oracle_smg_firing_1_1 import Xbe
NEW_PIN="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"
from differential_physics_native_frame_tu15_20261012 import regions
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn import x86_const as R
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--xbe',type=Path,required=True)
parser.add_argument('--native-input',type=Path,required=True)
parser.add_argument('--output',type=Path,required=True)
arguments=parser.parse_args()
if arguments.output.exists():raise ValueError('Preserve existing original fixture evidence')
arguments.output.parent.mkdir(parents=True,exist_ok=True)
NEW_DEFAULT=arguments.xbe;SOURCE=arguments.native_input
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
r={'status':'RUNNING','scope':__doc__,'started':time.time(),'code_patches':[],'supplied_query_returns':[],'native_outputs_used_as_inputs':False,'cases':[]}
try:
 assert sha(NEW_DEFAULT)==NEW_PIN=='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
 assert sha(SOURCE)=='14457cb4fb18292e5f0c2fd007e9a4f36590ae97d210f3212c1cba32e9d4cb49'
 available=int(next(x.split()[1]for x in Path('/proc/meminfo').read_text().splitlines()if x.startswith('MemAvailable:')))//1024;assert available>=8192
 x=Xbe(NEW_DEFAULT);g=Uc(UC_ARCH_X86,UC_MODE_32);mapped=set()
 def mount(p,data):
  pending=[page for page in range(p&~4095,(p+len(data)+4095)&~4095,4096)if page not in mapped];start=0
  while start<len(pending):
   end=start+1
   while end<len(pending)and pending[end]==pending[end-1]+4096:end+=1
   g.mem_map(pending[start],(end-start)*4096);start=end
  mapped.update(pending);g.mem_write(p,data)
 for s in x.sections:mount(s.va,x.section_bytes(s))
 banks=list(regions(SOURCE))
 for p,data in banks:mount(p,data)
 text=next(s for s in x.sections if s.name=='.text');assert bytes(g.mem_read(text.va,len(x.section_bytes(text))))==x.section_bytes(text)
 def word(p):return struct.unpack('<I',g.mem_read(p,4))[0]
 def put(p,v):g.mem_write(p,struct.pack('<I',v&0xffffffff))
 bsp=word(0x4ecc18);leaves,clusters=word(bsp+0x2c),word(bsp+0x9c)
 r.update(xbe_sha256=NEW_PIN,native_input_sha256=sha(SOURCE),native_input=str(SOURCE),native_input_bytes=sum(len(b)for _,b in banks),memory_available_mib=available,bsp={'leaves':leaves,'clusters':clusters},original_segments=[{'start':a,'end_exclusive':z,'sha256':hashlib.sha256(x.read(a,z-a)).hexdigest()}for a,z in [(0x2b770,0x2b816),(0x2b700,0x2b761),(0x14cdb0,0x14ce2b)]])
 pc_trace=[]
 def code(cpu,p,n,_):
  assert bytes(cpu.mem_read(p,n))==x.read(p,n),hex(p);pc_trace.append(p)
 g.hook_add(UC_HOOK_CODE,code)
 stack,camera=0x600ef000,0x600f2000
 def run(name,position,previous_leaf=-1,previous_cluster=-1):
  put(0x4c6874,previous_cluster);put(0x4c6884,previous_leaf)
  g.mem_write(camera,bytes(0x74));put(camera+4,0);g.mem_write(camera+0xc,struct.pack('<3f',*position))
  put(stack,0x60300000);put(stack+4,camera)
  for register in [R.UC_X86_REG_EAX,R.UC_X86_REG_ECX,R.UC_X86_REG_EDX,R.UC_X86_REG_EBX,R.UC_X86_REG_EBP,R.UC_X86_REG_ESI,R.UC_X86_REG_EDI]:g.reg_write(register,0)
  g.reg_write(R.UC_X86_REG_ESP,stack);g.reg_write(R.UC_X86_REG_EFLAGS,2);g.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
  pc_trace.clear();g.emu_start(0x2b770,0x2b816,count=1000000);assert g.reg_read(R.UC_X86_REG_EIP)==0x2b816
  esp=g.reg_read(R.UC_X86_REG_ESP)
  signed=lambda v:struct.unpack('<i',struct.pack('<I',v))[0]
  row={'name':name,'position':list(position),'previous_leaf':previous_leaf,'previous_cluster':previous_cluster,
      'leaf':signed(word(esp+0x1c)),'cluster':signed(word(esp+0x18)),
      'location_found':g.mem_read(esp+0x20,1)[0]==0,
      'persistent_leaf':signed(word(0x4c6884)),'persistent_cluster':signed(word(0x4c6874)),
      'original_instructions':len(pc_trace),'unchanged_14cdb0_executed':0x14cdb0 in pc_trace}
  assert row['unchanged_14cdb0_executed'];r['cases'].append(row);return row
 fleet=run('fleet',(349.011566162109375,340.078338623046875,52.2694091796875))
 destruction=run('destruction',(353.634918212890625,294.381256103515625,24.961688995361328125))
 outside=None
 for n,point in enumerate([(1e7,1e7,1e7),(-1e7,-1e7,-1e7),(0,0,0),(1e7,0,0),(-1e7,0,0)]):
  row=run('outside-search-'+str(n),point)
  if not row['location_found']:outside=point;break
 assert outside is not None,'No authenticated failed-query control was reached'
 for name,leaf,cluster in [('valid-previous',fleet['leaf'],fleet['cluster']),('mismatched-but-valid-pair',fleet['leaf'],(fleet['cluster']+1)%clusters),('negative-leaf',-1,fleet['cluster']),('leaf-end',leaves,fleet['cluster']),('negative-cluster',fleet['leaf'],-1),('cluster-end',fleet['leaf'],clusters),('both-invalid',-1,-1)]:
  row=run(name,outside,leaf,cluster);assert not row['location_found']
  assert row['persistent_leaf']==leaf and row['persistent_cluster']==cluster,'Failed caller repaired persistent history'
  if name in ['valid-previous','mismatched-but-valid-pair']:assert row['leaf']==leaf and row['cluster']==cluster
  else:assert row['leaf']==row['cluster']==-1
 assert fleet['cluster']==destruction['cluster']==27
 assert bytes(g.mem_read(text.va,len(x.section_bytes(text))))==x.section_bytes(text)
 r['status']='PASS_UNCHANGED_ORIGINAL_MAIN_LOCATION_QUERY_AND_FALLBACK'
except Exception as e:r.update(status='FAIL_RETAINED',error=str(e),traceback=traceback.format_exc());raise
finally:r['finished']=time.time();arguments.output.write_text(json.dumps(r,indent=2)+'\n');print(r['status'])
