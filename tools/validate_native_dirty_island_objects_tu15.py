#!/usr/bin/env python3
"""Whole no-service returns and real first-child prefixes for native282C10."""
import inspect,struct,json
from pathlib import Path
import validate_native_contact_pressure_tu15 as parent
from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
from oracle_smg_firing_1_1 import _Code
from generate_physics_live_support_tu15 import complete_body
BACKEND='unchanged-tu15-native-dirty-island-objects-v1'
W,A,I,B,P,C,R,O,H,G=0x1000100,0x1000200,0x1000300,0x1001000,0x1006000,0x1008000,0x1009000,0x100c000,0x100d000,0x100c800
OWNERS=(0x282c10,)
BOUNDARIES=(0x1cc310,0x1496f0,0x1d7ca0,0x16e8a0,0xba570,0x1d9510,0xba8c0)
def metadata(image):
 code=_Code(image,'.text');body=complete_body(code,0x282c10);registers={n:i for i,n in enumerate(('eax','ecx','edx','ebx','esp','ebp','esi','edi'))};sites=[];listing=[]
 for pc in sorted(body.code):
  ins=code.at(pc);listing.append(f'{pc:x} {ins.bytes.hex()} {ins.mnemonic} {ins.op_str}\n');ops=[]
  for op in ins.operands:
   if op.type!=X86_OP_MEM or ins.mnemonic=='lea':continue
   assert not op.mem.segment;base,index=ins.reg_name(op.mem.base),ins.reg_name(op.mem.index)
   read=bool(op.access&CS_AC_READ)or ins.mnemonic=='test';write=bool(op.access&CS_AC_WRITE)
   ops.append(dict(base=registers.get(base,-1),index=registers.get(index,-1),scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=read,write=write))
  sites.append(dict(instruction=pc,operands=ops))
 assert len(body.code)==276
 return dict(sites=sites,body_sites=[276],listing_sha256={'0x282c10':parent.hashlib.sha256(''.join(listing).encode()).hexdigest()},boundaries=list(BOUNDARIES),regions=[])
def cases(image):
 rows=[]
 def make(name,*,pending=1,busy=0,active=1,inactive=0,dirty=0,bodies=0,hidden=0,kind=1,mode=0,component=0xffffffff,flags=0):
  banks={0x1000000:bytearray(0x20000),0x52be44:bytearray(4),0x52be48:bytearray(4),0x52be5c:bytearray(4),0x4ecbd0:bytearray(4),0x4f3218:bytearray(4),0x56dfcc:bytearray(4),0x48b870:bytearray(8)}
  def put(at,value,width=4):
   lo=next(lo for lo,b in banks.items()if lo<=at and at+width<=lo+len(b));banks[lo][at-lo:at-lo+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little')
  put(0x56dfcc,pending,1);put(0x48b873,busy,1);put(0x52be48,W);put(W+8,A);put(W+12,active);put(W+0x14,A+16);put(W+0x18,inactive)
  put(0x52be44,0x101f000);put(0x101f000,10);put(0x52be5c,C);put(C+0x44,R);put(0x4ecbd0,O);put(O+0x44,H);put(0x4f3218,G);put(G+0xc,mode,1)
  for cohort in range(2):
   island=I+cohort*0x100;put(A+cohort*16,island);put(island+0xae,dirty,1);put(island+0x40,bodies);put(island+0x3c,B)
  for i in range(max(1,min(bodies,128))):
   body=0x1003000+i*0x50;put(B+i*4,body);put(body+0x40,hidden,1);put(body+0x34,1);put(body+0x30,P+i*8);put(P+i*8,0x2001);put(P+i*8+4,i%4)
  for i in range(4):
   datum=0x12340000+i;obj=0x100e000+i*0x200;put(R+i*160+8,datum);put(H+i*12+3,kind,1);put(H+i*12+8,obj);put(obj+0xb4,component);put(obj+0xc0,flags,1);put(obj+0xd4,1)
  n=len(rows);rows.append(dict(name=name,entry=0x282c10,receiver=0,arguments=[],banks=[dict(address=p,before=b.hex())for p,b in sorted(banks.items())],before_fp=[0x27f|((n%4)<<10),((n%8)<<11)|0x453f,0x1f80|((n%4)<<13)],stop_pc=parent.physical.STOP,boundary=None))
 make('pending0',pending=0)
 for busy in(1,2,255):make(f'busy{busy}',busy=busy)
 for active,inactive in((0,0),(1,0),(0,1),(1,1),(0xffffffff,0x80000000)):
  for dirty in(0,1,255):
   for bodies in(0,1,4,0xffffffff):make(f'nochild_{active:x}_{inactive:x}_{dirty}_{bodies:x}',active=active,inactive=inactive,dirty=dirty,bodies=bodies,hidden=1)
 for mode in(0,4):
  for flags in(0,1,64,65):
   for component in(0xffffffff,0x12340002):make(f'candidate_{mode}_{flags}_{component:x}',dirty=1,bodies=4,mode=mode,flags=flags,component=component)
 for kind in(0,7,1,33,255):make(f'kind{kind}',dirty=1,bodies=8,kind=kind)
 make('fault_seed_simple',dirty=1,bodies=4,hidden=1);make('fault_seed_positive',dirty=1,bodies=4,component=0x12340002)
 return rows
ORIGINAL_OBSERVER=parent.physical.observer_source
ORIGINAL_PHYSICAL=parent.physical.physical
EXECUTED=set()
def observer_source():
 text=ORIGINAL_OBSERVER();text=text.replace('user_regs_struct regs{};unsigned count=0;','J executed=J::array();user_regs_struct regs{};unsigned count=0;')
 needle='const auto site=sites.find(pc);';assert text.count(needle)==1;text=text.replace(needle,'executed.push_back(pc);'+needle)
 needle='{"banks",banks},{"actual_stop"';assert text.count(needle)==1;text=text.replace(needle,'{"executed_sites",executed},{"banks",banks},{"actual_stop"');return text
def physical(*args,**kwargs):
 rows=ORIGINAL_PHYSICAL(*args,**kwargs)
 for row in rows:EXECUTED.update(row['executed_sites'])
 return rows
def main():
 parent.physical.observer_source=observer_source;parent.physical.physical=physical;parent.OWNERS=OWNERS;parent.BACKEND=BACKEND;parent.metadata=metadata;parent.cases=cases;text=inspect.getsource(parent.main)
 begin=text.index('  def boundary(c,actual):');end=text.index('  physical.boundary=boundary',begin)
 text=text[:begin]+'''  def boundary(c,actual):
   owner=actual['actual_stop']
   if owner==physical.STOP:return None
   assert owner in boundary_owners,hex(owner)
   gp,args=actual['boundary_gp'],actual['boundary_stack'];result=dict(owner=owner)
   if owner==0x1cc310:result.update(datum=args[1],flags=args[2])
   elif owner==0x1d7ca0:result.update(component=gp[6])
   elif owner==0x16e8a0:result.update(pool=gp[2],datum=gp[0])
   elif owner==0x1d9510:result.update(component=gp[1])
   elif owner in (0xba570,0xba8c0):result.update(datum=args[1])
   return result
'''+text[end:]
 text=text.replace("source=ROOT/'src/physics/native_tu15_contact_pressure.cpp'","source=ROOT/'src/physics/native_tu15_dirty_island_objects.cpp'")
 text=text.replace("shutil.copyfile(ROOT/'tests/native_tu15_contact_pressure_tests.cpp',project/'driver.cpp');variants={}","driver=(ROOT/'tests/native_tu15_contact_pressure_tests.cpp').read_text();"+'''driver=driver.replace('#include "halo2/native_tu15_contact_pressure.hpp"','#include "halo2/native_tu15_dirty_island_objects.hpp"').replace('#include "halo2/native_tu15_contact_pressure_binding.hpp"','').replace('unchanged-tu15-native-contact-pressure-v1',BACKEND).replace('J::array({220,2,2,2})','J::array({276})').replace('auto services=native_tu15_contact_pressure_native_services();','NativeTu15DirtyIslandObjectServices services;').replace('NativeTu15ContactPressureStats stats;','NativeTu15DirtyIslandObjectStats stats;').replace('native_tu15_prune_contact_pressure(m,fp,services,observer,&stats);','native_tu15_process_dirty_island_objects(m,fp,0,services,observer,&stats);');begin=driver.index('   services.select_candidate=');end=driver.index('   const auto cw=',begin);driver=driver[:begin]+native_boundaries+driver[end:];(project/'driver.cpp').write_text(driver);variants={}''')
 begin=text.index("  for name,needle,replacement in [");end=text.index('\n  (project/',begin)
 text=text[:begin]+'''  for name,needle,replacement in [('busy_guard','if(m.word(0x48b873u,1))','if(!m.word(0x48b873u,1))'),('body_stride','cursor+=4u','cursor+=8u'),('dirty_clear','island+0xaeu,0,1','island+0xaeu,1,1')]:
   assert needle in text;variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
  (project/'types.cpp').write_text('')'''+text[end:]
 # Avoid an infinite intentionally-wrong dirty flag mutant; corrupt its pending
 # clear instead while preserving a finite source control flow.
 text=text.replace("('dirty_clear','island+0xaeu,0,1','island+0xaeu,1,1')","('pending_clear','0x56dfccu,0,1','0x56dfccu,1,1')")
 text=text.replace("ROOT/'include/halo2/native_tu15_contact_pressure.hpp',ROOT/'include/halo2/native_tu15_contact_pressure_binding.hpp'","ROOT/'include/halo2/native_tu15_dirty_island_objects.hpp'")
 env=dict(parent.__dict__,boundary_owners=BOUNDARIES,native_boundaries=NATIVE_BOUNDARIES,__file__=__file__);exec(text,env);code=env['main']();output=Path(parent.sys.argv[parent.sys.argv.index('--output')+1]);path=output/'report.json';report=json.loads(path.read_text());all_sites=set(complete_body(_Code(parent.Xbe(parent.NEW_DEFAULT),'.text'),0x282c10).code);report.update(physically_executed_parent_sites=len(all_sites&EXECUTED),unexecuted_parent_sites=sorted(all_sites-EXECUTED),whole_selected_child_closure=False,source_TEST_memory_access_authenticated=True);report['source_hashes']['tools/validate_native_contact_pressure_tu15.py']=parent.sha(parent.ROOT/'tools/validate_native_contact_pressure_tu15.py');parent.save(path,report);return code
NATIVE_BOUNDARIES='''   const auto check_argument=[&](const char*name,std::uint32_t value){check(c.at("boundary").at(name)==value,name+std::string(" typed dirty-island argument differs"));};
   services.set_component_flags=[&](auto datum,auto flags,auto&,auto){check_argument("datum",datum);check_argument("flags",flags);boundary(0x1cc310u);};
   services.collect_memory=[&](auto&,auto){boundary(0x1496f0u);};
   services.destroy_component=[&](auto component,auto&,auto){check_argument("component",component);boundary(0x1d7ca0u);};
   services.free_component=[&](auto pool,auto datum,auto&,auto){check_argument("pool",pool);check_argument("datum",datum);boundary(0x16e8a0u);};
   services.delete_object=[&](auto datum,auto&,auto){check_argument("datum",datum);boundary(0xba570u);};
   services.remove_component=[&](auto component,auto&,auto){check_argument("component",component);boundary(0x1d9510u);};
   services.defer_object=[&](auto datum,auto&,auto){check_argument("datum",datum);boundary(0xba8c0u);};
'''
if __name__=='__main__':raise SystemExit(main())
