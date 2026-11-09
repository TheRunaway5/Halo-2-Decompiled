#!/usr/bin/env python3
"""Unchanged TU1.5 support-carrier184EC0 and its real numerical dependencies.

Physical originals execute1D8B90/144090/145560/1D8CD0/323A10 unchanged. Native
inputs are ordinary before banks only. Every retained external byte, external
read/store, result AL and numeric environment is compared. Callee-private
temporary frames are typed native values, not CPU postimages; they are outside
the persistent-bank claim and cannot alias any declared public input/output.
Run through tools/run_test.py; compiler products and raw fixtures are removed.
"""
import argparse,copy,ctypes,hashlib,inspect,json,os,random,shutil,struct,subprocess,sys
from pathlib import Path
from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
import validate_native_island_agent_phase_tu15 as physical
import validate_native_sphere_overlap_tu15 as sphere
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body

SOURCE=ROOT
B=0x1000000
MANAGER,COMPONENT,ROWS,BODY,MOTION,TABLE,POINT,PREVIOUS,OLD_FRAME,FRAME,VELOCITY,DELTA,BASIS,ZERO,IDENTITY=[B+n for n in(0x100,0x200,0x400,0x600,0x800,0xb00,0xc00,0xc40,0xd00,0xe00,0xf00,0xf40,0x1000,0x1100,0x1140)]
TEMP=physical.SP-0x1000
BACKEND='unchanged-tu15-support-carrier-v1'
OWNERS=(0x184ec0,0x1d8b90,0x2e4650,0x2e44d0,0x144090,0x145560,0x1d8cd0,0x323a10)
def sha(p):
 with Path(p).open('rb')as stream:return hashlib.file_digest(stream,'sha256').hexdigest()
def save(p,value):p.write_text(json.dumps(value,indent=2)+'\n')
def metadata(image):
 code=_Code(image,'.text');regs={n:i for i,n in enumerate(('eax','ecx','edx','ebx','esp','ebp','esi','edi'))};sites={};counts={};pins={}
 for owner in OWNERS:
  body=complete_body(code,owner);counts[hex(owner)]=len(body.code);listing=[]
  for pc in sorted(body.code):
   i=code.at(pc);listing.append(f'{pc:x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n');ops=[]
   for op in i.operands:
    if op.type!=X86_OP_MEM or i.mnemonic=='lea':continue
    assert not op.mem.segment or i.reg_name(op.mem.segment)in('es','ds')
    access=CS_AC_WRITE if i.mnemonic in('fst','fstp')else op.access
    if i.mnemonic in('cmp','test'):access|=CS_AC_READ
    ops.append(dict(base=regs.get(i.reg_name(op.mem.base),-1),index=regs.get(i.reg_name(op.mem.index),-1),scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(access&CS_AC_READ),write=bool(access&CS_AC_WRITE)))
   sites[pc]=dict(instruction=pc,operands=ops)
  pins[hex(owner)]=hashlib.sha256(''.join(listing).encode()).hexdigest()
 assert counts['0x184ec0']==94 and counts['0x1d8cd0']==51,counts
 return dict(sites=list(sites.values()),body_sites=counts,listing_sha256=pins,regions=[],boundaries=[])
def cases(image):
 rows=[]
 def add(name,flags=0,disabled=0,slot=0,count=1,scale=1.,changes=(),aliases=(),fp=None,missing=False,seed=False):
  rng=random.Random(len(rows));banks={B:bytearray(rng.randbytes(0x2000)),0x52be5c:bytearray(struct.pack('<I',MANAGER)),0x46a0d0:bytearray(image.read(0x46a0d0,4)),0x474d04:bytearray(struct.pack('<I',ZERO)),0x474d30:bytearray(struct.pack('<I',IDENTITY))}
  def put(p,n,width=4):
   lo=next(lo for lo,data in banks.items()if lo<=p and p+width<=lo+len(data));banks[lo][p-lo:p-lo+width]=(n&((1<<(width*8))-1)).to_bytes(width,'little')
  def vec(p,values):
   for n,value in enumerate(values):put(p+n*4,struct.unpack('<I',struct.pack('<f',value))[0]if isinstance(value,float)else value)
  def frame(p,scale=1.,position=(3.,-4.,5.)):
   vec(p,(scale,0.,1.,0.,-1.,0.,0.,0.,0.,1.,*position))
  put(MANAGER+0x44,COMPONENT);put(COMPONENT+4,flags);put(COMPONENT+0x18,slot,1);put(COMPONENT+0x19,0,1);put(COMPONENT+0x74,count);put(COMPONENT+0x70,ROWS)
  for n in range(3):put(ROWS+n*96+0x40,BODY)
  put(BODY+0x3c,MOTION);put(BODY+0x40,disabled,1);put(MOTION,TABLE);put(TABLE+0x58,0x323a10)
  vec(MOTION+0x40,(1.25,-2.5,3.75,0.));vec(MOTION+0x50,(.5,-1.25,2.,0.));vec(MOTION+0x70,(2.,3.,-4.,0.))
  for n,values in enumerate(((0.,1.,0.,0.),(-1.,0.,0.,0.),(0.,0.,1.,0.),(7.,8.,-9.,1.))):vec(MOTION+0x80+n*16,values)
  for n,values in enumerate(((1.,0.,0.,0.),(0.,0.,-1.,0.),(0.,1.,0.,0.),(2.,-3.,4.,1.))):vec(COMPONENT+0x30+n*16,values)
  for n in(1,2):
   other_body=B+0x1200+(n-1)*0x100;other_motion=B+0x1500+(n-1)*0x100
   put(ROWS+n*96+0x40,other_body);put(other_body+0x3c,other_motion);put(other_body+0x40,disabled,1)
   banks[B][other_motion-B:other_motion-B+0xc0]=banks[B][MOTION-B:MOTION-B+0xc0]
   vec(other_motion+0x40,(float(n+2),-2.5,3.75,0.));vec(other_motion+0xb0,(float(n+9),8.,-9.,1.))
  vec(POINT,(3.,-5.,7.));vec(PREVIOUS,(-.25,1.5,-2.));vec(ZERO,(0.,0.,0.));frame(OLD_FRAME,scale);frame(IDENTITY,1.,(0.,0.,0.))
  for p,n,width in changes:put(p,n,width)
  args=[POINT,OLD_FRAME,FRAME,VELOCITY,DELTA,BASIS]
  for index,p in aliases:args[index]=p
  rows.append(dict(name=('fault_seed_'if seed else'')+name,entry=0x184ec0,receiver=0xffffffff if missing else 0,entry_registers={'ebx':PREVIOUS},arguments=args,previous_velocity=PREVIOUS,
      banks=[dict(address=p,before=data.hex())for p,data in sorted(banks.items())],before_fp=fp or[0x27f,0x453f,0x1f80],stop_pc=physical.STOP,boundary=None))
 for flags in(0,0x40):
  for disabled in(0,1):
   for rounding in range(4):
    for mode in(0,0x40,0x8000,0x8040):add(f'fp_{flags}_{disabled}_{rounding}_{mode}',flags,disabled,fp=[0x27f|(rounding<<10),0x453f,0x1f80|(rounding<<13)|mode])
 for slot in(0,1,2,127,128,255):
  for count in(0,1,3,0x80000000):add(f'slot_{slot}_{count:x}',slot=slot,count=count)
 add('component_none',missing=True)
 for value in(0.,-0.,2.,-2.,0x7f800000,0x7fc12345,0x7f812345,0x80000001):
  add('scale_'+str(value),scale=value)
  add('motion_payload_'+str(value),changes=((MOTION+0x54,struct.unpack('<I',struct.pack('<f',value))[0]if isinstance(value,float)else value,4),))
 for index,p in((3,PREVIOUS),(4,PREVIOUS),(5,OLD_FRAME),(2,OLD_FRAME),(3,POINT),(4,VELOCITY),(5,FRAME)):
  add(f'alias_{index}_{p:x}',aliases=((index,p),))
 for flags in(0,0x40):
  for disabled in(0,1):add(f'valid_{flags}_{disabled}',flags,disabled,seed=True)
 for depth in range(1,8):
  for flags in(0,0x40):
   for disabled in(0,1):
    add(f'preserved_x87_{depth}_{flags}_{disabled}',flags,disabled,seed=flags==0x40 and disabled==0);rows[-1]['x87_depth']=depth
 add('unknown_motion_owner',changes=((TABLE+0x58,0xdeadbad0,4),));rows[-1].update(expected_native_refusal=True,stop_pc=0xdeadbad0)
 return rows

def guard_source():
 return r'''
#if defined(HALO2_CARRIER_HANDOFF)
 for(unsigned kind=0;kind<28;++kind){
  Memory m;m.banks.emplace(0x200f000u,std::vector<std::byte>(28));
  const auto put=[&](unsigned offset,std::uint32_t value){std::array<std::byte,4>b{};for(unsigned k=0;k<4;++k)b[k]=std::byte(value>>(k*8u));m.write(0x200f000u+offset,b);};
  put(0,0x1fa192u);for(unsigned n=0;n<6;++n)put(4u+n*4u,0x1001000u+n*64u);
  SourcePhysicsContactRuntimeAbi abi{};abi.gp[0]=0xaabbccddu;abi.gp[1]=0;abi.gp[3]=0x1000c40u;abi.gp[4]=0x200f000u;
  abi.eflags=2;abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x27fu,0x1f80u};
  SourcePhysicsContactRuntimeCall call{0x1fa18du,0x184ec0u,false};
  bool first{},second{};NativeTu15SupportCarrierHandoffPolicy policy{0x2000000u,0x2010000u,0x4000000u,256u,true,&first,&second};
  if(kind==0)call.target=0x184ec1u;
  if(kind==1)call.instruction++;
  if(kind==2)call.tail=true;
  if(kind==3)put(0,0x1fa193u);
  if(kind==4)policy.stack_end=0x200f018u;
  if(kind==5)policy.stack_first=0x200f001u;
  if(kind==6)policy.observers_read_only=false;
  if(kind==7)first=true;
  if(kind==8)second=true;
  if(kind>=9&&kind<=15)abi.x87_depth=kind-1;
  if(kind==16)abi.active_mmx=SourcePhysicsMmxAlias{};
  if(kind==17)abi.floating_point.reset();
  if(kind==18)abi.floating_point->mxcsr=0;
  if(kind==19)abi.floating_point->mxcsr|=0x10000u;
  if(kind==20)abi.eflags|=0x400u;
  if(kind==21)policy.data_first++;
  if(kind==22)policy.data_bytes=255u;
  if(kind==23)policy.data_first=policy.stack_first-16u;
  if(kind==24)policy.data_first=abi.gp[4]-128u;
  if(kind==25)put(4,0x4000040u);
  if(kind==26)abi.floating_point->x87_control_word=0;
  if(kind==27)abi.x87_status=0x80;
  const auto gp=abi.gp;const auto cw=abi.floating_point?abi.floating_point->x87_control_word:0;
  const auto mx=abi.floating_point?abi.floating_point->mxcsr:0;
  m.log_reads=true;bool refused{},handled{};NativeTu15SupportCarrierHandoffActivity activity;
  try{handled=native_tu15_handoff_support_carrier(m,call,abi,policy,{},&activity);}catch(const std::runtime_error&){refused=true;}
  check(kind==0?(!handled&&!refused&&!activity.attempts):(refused&&activity.refusals==1&&activity.carriers.calls==0),"Caller/FP/arena guard differs");
  check(m.reads.empty()&&abi.gp==gp&&(!abi.floating_point||(abi.floating_point->x87_control_word==cw&&abi.floating_point->mxcsr==mx)),"Refused carrier entered DATA or changed ABI/FP");
 }
#endif
'''

def driver_source():
 text=(ROOT/'tests/native_tu15_sphere_overlap_tests.cpp').read_text()
 text=text.replace('#include "halo2/native_tu15_convex_overlap_query.hpp"','#include "halo2/native_tu15_support_carrier_handoff.hpp"')
 a=text.index(' for(unsigned depth=1;');b=text.index(' for(const auto&c:fixture.at("cases"))',a);text=text[:a]+text[b:]
 text=text.replace(' for(const auto&c:fixture.at("cases"))',guard_source()+' for(const auto&c:fixture.at("cases"))')
 a=text.index('  NativeTu15PhysicsFrameFp fp{c.at');b=text.index('  const auto cw=',a)
 text=text[:a]+'''  NativeTu15PhysicsFrameFp fp{c.at("before_fp")[0],c.at("before_fp")[1],c.at("before_fp")[2]};
  m.banks.emplace(0x4000000u,std::vector<std::byte>(256));
  const auto&a=c.at("arguments");NativeTu15SupportCarrierRequest request{c.at("receiver"),c.at("previous_velocity"),a[0],a[1],a[2],a[3],a[4],a[5]};
  bool result{},owner_refused{};
'''+text[b:]
 a=text.index('#if defined(HALO2_SPHERE_QUERY_BINDING)',text.index('  try{'));b=text.index('  }catch(const Fault&)',a)
 text=text[:a]+'''#if defined(HALO2_CARRIER_HANDOFF)
   m.banks.emplace(0x200f000u,std::vector<std::byte>(28));const auto put=[&](unsigned offset,std::uint32_t value){std::array<std::byte,4>b{};for(unsigned k=0;k<4;++k)b[k]=std::byte(value>>(k*8u));m.write(0x200f000u+offset,b);};
   put(0,0x1fa192u);for(unsigned n=0;n<6;++n)put(4u+n*4u,a[n]);
   SourcePhysicsContactRuntimeAbi abi{};abi.gp[0]=0xa5a5a5a5u;abi.gp[1]=request.component_datum;abi.gp[3]=request.previous_velocity;abi.gp[4]=0x200f000u;abi.eflags=2;abi.x87_status=fp.status;abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};
   abi.x87_depth=c.value("x87_depth",0u);const auto original_x87=bytes(c.at("incoming_x87"));
   for(unsigned n=0;n<abi.x87_depth;++n)std::memcpy(&abi.x87[n],original_x87.data()+n*16u,10);
   const auto preserved_payload=[&]{for(unsigned n=0;n<abi.x87_depth;++n)check(std::memcmp(&abi.x87[n],original_x87.data()+n*16u,10)==0,name+" production preserved x87 payload differs");};
   NativeTu15SupportCarrierHandoffPolicy policy{0x2000000u,0x2010000u,0x4000000u,256u,true};
   try{check(native_tu15_handoff_support_carrier(m,{0x1fa18du,0x184ec0u,false},abi,policy,observe),name+" production caller was not owned");}
   catch(...){fp={abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr};preserved_payload();throw;}
   fp={abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr};result=bool(abi.gp[0]&255u);
   check(abi.gp[4]==0x200f01cu&&abi.gp[3]==request.previous_velocity,name+" production RET24/nonvolatile differs");
   preserved_payload();
#else
   result=native_tu15_update_support_carrier(m,request,{0x4000000u,0x4000040u,0x4000080u,0x40000c0u,0x40000d0u},fp,observe);
#endif
'''+text[b:]
 text=text.replace('}catch(const Fault&){failed=true;}catch(const ObserverFault&){observer_failed=true;}',
     '}catch(const Fault&){failed=true;}catch(const ObserverFault&){observer_failed=true;}catch(const std::runtime_error&e){if(c.value("expected_native_refusal",false)&&std::string(e.what()).find("qualified motion point owner")!=std::string::npos)owner_refused=true;else throw;}')
 text=text.replace('check(clock==c.at("timestamps").size(),name+" clock consumption differs");','check(owner_refused==c.value("expected_native_refusal",false),name+" unknown motion guard differs");if(!fault_case&&!c.contains("observer_fault")&&!owner_refused)check(result==bool(c.at("boundary_gp")[0].get<std::uint32_t>()&255u),name+" result AL differs");')
 text=text.replace('const auto fault=c.value("fault",J{});','check(c.at("incoming_x87")==c.at("outgoing_x87")&&c.at("incoming_x87_tags")==c.at("outgoing_x87_tags"),name+" original x87 preservation differs");const auto fault=c.value("fault",J{});')
 text=text.replace('#include <fstream>','#include <fstream>\n#include <cstring>')
 marker='std::uint16_t control()'
 assert text.count(marker)==1
 text=text.replace(marker,r'''struct CallerX87 {
 alignas(16)std::array<std::byte,512>host{},target{};
 bool active{};
 explicit CallerX87(const J&c):active(c.value("x87_depth",0u)!=0){
  if(!active)return;asm volatile("fxsave64 %0":"=m"(host)::"memory");target=host;
  const auto cw=c.at("before_fp")[0].get<std::uint16_t>(),sw=c.at("before_fp")[1].get<std::uint16_t>();
  const auto tags=c.at("incoming_x87_tags").get<std::uint8_t>();const auto values=bytes(c.at("incoming_x87"));
  std::memcpy(target.data(),&cw,2);std::memcpy(target.data()+2,&sw,2);target[4]=std::byte(tags);std::copy(values.begin(),values.end(),target.begin()+32);
  restore(target);
 }
 static void restore(const std::array<std::byte,512>&value){asm volatile("fxrstor64 %0"::"m"(value):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)","xmm0","xmm1","xmm2","xmm3","xmm4","xmm5","xmm6","xmm7","xmm8","xmm9","xmm10","xmm11","xmm12","xmm13","xmm14","xmm15");}
 void verify(const std::string&name)const{
  if(!active)return;alignas(16)std::array<std::byte,512>actual{};asm volatile("fxsave64 %0":"=m"(actual)::"memory");
  check(actual[4]==target[4],name+" native hardware x87 tags differ");
  for(unsigned n=0;n<8;++n)check(std::memcmp(actual.data()+32+n*16,target.data()+32+n*16,10)==0,name+" native hardware x87 payload differs");
 }
 ~CallerX87(){if(active)restore(host);}
};
'''+marker)
 text=text.replace('  const auto cw=control(),sw=status();','  CallerX87 occupied(c);const auto cw=control(),sw=status();')
 text=text.replace('check(control()==cw&&status()==sw&&_mm_getcsr()==mx,name+" host FP differs");','check(control()==cw&&status()==sw&&_mm_getcsr()==mx,name+" host FP differs");occupied.verify(name);')
 text=text.replace('if(log_reads)reads.push_back','if(log_reads&&!(p>=0x4000000u&&std::uint64_t(p)+b.size()<=0x4000100ull))reads.push_back')
 text=text.replace('if(log_writes)raw_stores.push_back','if(log_writes&&!(p>=0x4000000u&&std::uint64_t(p)+b.size()<=0x4000100ull))raw_stores.push_back')
 text=text.replace('stores.push_back(e);','if(e.address>=0x4000000u&&std::uint64_t(e.address)+e.written.size()<=0x4000100ull)return;stores.push_back(e);')
 text=text.replace('p>=0x4000000u&&std::uint64_t(p)+b.size()<=0x4000100ull','((p>=0x4000000u&&std::uint64_t(p)+b.size()<=0x4000100ull)||(p>=0x200f000u&&std::uint64_t(p)+b.size()<=0x200f01cull))')
 for value in(0x4000100,0x40000d0,0x40000c0,0x4000080,0x4000040,0x4000000):text=text.replace(hex(value),hex(TEMP+value-0x4000000))
 return text.replace('unchanged-tu15-sphere-overlap-v1',BACKEND)

def observer_source():
 text=sphere.observer_source()
 text=text.replace('unsigned count=0;','user_fpregs_struct incoming{};request(PTRACE_GETFPREGS,child.pid,nullptr,&incoming);unsigned count=0;')
 marker='{ "visited",visited}';assert text.count(marker)==1
 text=text.replace(marker,marker+',{"incoming_x87",hex(std::vector<std::byte>(reinterpret_cast<const std::byte*>(incoming.st_space),reinterpret_cast<const std::byte*>(incoming.st_space)+128))},{"outgoing_x87",hex(std::vector<std::byte>(reinterpret_cast<const std::byte*>(fp.st_space),reinterpret_cast<const std::byte*>(fp.st_space)+128))},{"incoming_x87_tags",incoming.ftw},{"outgoing_x87_tags",fp.ftw}')
 marker='if(pc==stop||'
 assert text.count(marker)==1
 text=text.replace(marker,'if((pc==stop&&!m.value("stop_before_rep_write",false))||')
 marker='    std::uint32_t estimate_input{};'
 assert text.count(marker)==1
 text=text.replace(marker,'''    if(pc==stop&&m.value("stop_before_rep_write",false))break;
'''+marker)
 marker='    if(pc==0x1e1042){user_fpregs_struct fp{};'
 assert text.count(marker)==1
 text=text.replace(marker,'''    if(m.value("stop_after_store",0u)&&writes.size()==m.at("stop_after_store").get<unsigned>()){
      request(PTRACE_GETREGS,child.pid,nullptr,&regs);break;
    }
'''+marker)
 marker='check(std::uint32_t(regs.rip)==stop||'
 assert text.count(marker)==1
 return text.replace(marker,'check((m.value("stop_after_store",0u)&&writes.size()==m.at("stop_after_store").get<unsigned>())||std::uint32_t(regs.rip)==stop||')

def physical_with_x87():
 text=inspect.getsource(physical.physical).replace('def physical(','def carrier_physical(',1)
 marker="struct.pack_into('<H',env,4,sw)"
 assert text.count(marker)==1
 text=text.replace(marker,"struct.pack_into('<H',env,4,(sw&~0x3800)|((((sw>>11)+c.get('x87_depth',0))&7)<<11))")
 marker="  assembly.append(f'''.code32"
 assert text.count(marker)==1
 text=text.replace(marker,"""  depth=c.get('x87_depth',0);values=(1.25,-2.5,3.75,-4.125,5.5,-6.75,7.875)
  payload=b''.join(ctypes.string_at(ctypes.byref(ctypes.c_longdouble(v)),10)for v in values[:depth]);(owned/'x87-input.bin').write_bytes(payload)
  entry_setup+='\\n'+'\\n'.join(f' fldt x87_input+{n*10}'for n in range(depth))
"""+marker)
 marker="environment: .incbin \"{owned/'environment.bin'}\""
 assert text.count(marker)==1
 text=text.replace(marker,marker+"\nx87_input: .incbin \"{owned/'x87-input.bin'}\"")
 env=dict(physical.__dict__,ctypes=ctypes);exec(compile(text,__file__,'exec'),env);return env['carrier_physical']

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--no-prefixes',action='store_true');a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False)
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,scope=__doc__,owned_scratch=str(scratch),retained_large_artifacts=[],original_outcomes_supplied=False)
 try:
  source_pins={str(path):sha(path)for path in[
      Path(__file__),ROOT/'tests/native_tu15_sphere_overlap_tests.cpp',
      ROOT/'tools/validate_native_island_agent_phase_tu15.py',ROOT/'tools/validate_native_sphere_overlap_tu15.py',
      ROOT/'tools/observe_native_constraint_step_tu15.cpp',ROOT/'tools/generate_physics_live_support_tu15.py',
      *(ROOT/'include/halo2'/name for name in('native_tu15_support_carrier.hpp','native_tu15_support_carrier_handoff.hpp',
        'native_tu15_body_constraint_update.hpp','native_tu15_camera_projection.hpp','native_tu15_contact_body_update.hpp',
        'native_tu15_geometry.hpp','native_tu15_data_array_allocate.hpp','native_tu15_activation_services.hpp',
        'native_tu15_retained_store_observer.hpp','native_tu15_physics_frame.hpp','physics_contact_runtime.hpp',
        'physics_contact_record.hpp'))]}
  (a.output/'qualifier-source.py.txt').write_bytes(Path(__file__).read_bytes())
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);meta=metadata(image);physical.observer_source=observer_source;physical.boundary=lambda c,d:None
  original=physical_with_x87();rows=original(image,meta,cases(image),scratch);assert all(c['boundary_gp'][4]==physical.SP+28 for c in rows if not c.get('expected_native_refusal')),'Actual184EC0 must RET24'
  prefixes=[];code=_Code(image,'.text')
  if not a.no_prefixes:
   for seed in(c for c in rows if c['name'].startswith('fault_seed_')):
    for category in('reads','writes'):
     seen=set();occurrences={}
     for e in seed[category]:
      key=e['address'],e['bytes']if category=='reads'else len(e['written'])//2;occurrences[key]=occurrences.get(key,0)+1
      if e['instruction']in seen:continue
      seen.add(e['instruction']);c=copy.deepcopy(seed);c.update(name=seed['name']+'_'+category+'_'+hex(e['instruction']),stop_pc=e['instruction'],fault=dict(address=key[0],bytes=key[1],occurrence=occurrences[key],access='read'if category=='reads'else'write'));prefixes.append(c)
    seen=set()
    for n,e in enumerate(seed['writes']):
     if e['instruction']in seen:continue
     seen.add(e['instruction']);c=copy.deepcopy(seed);c.update(name=seed['name']+'_observer_'+str(n),stop_pc=e['instruction']+code.at(e['instruction']).size,observer_fault=n+1);prefixes.append(c)
   for case in prefixes:
    rep_write=case.get('fault',{}).get('access')=='write'and code.at(case['stop_pc']).mnemonic.startswith('rep movs')
    if rep_write:case['declared_rep_prewrite_boundary']=True
    rows+=original(image,dict(meta,stop_after_store=case.get('observer_fault',0),stop_before_rep_write=rep_write),[case],scratch)
  fixture=scratch/'original.json';save(fixture,dict(image_sha256=NEW_PIN,required_backend=BACKEND,cases=rows))
  project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
  for name in('native_tu15_support_carrier.hpp','native_tu15_support_carrier_handoff.hpp'):shutil.copyfile(SOURCE/'include/halo2'/name,project/'include/halo2'/name)
  paths={
   'support.cpp':SOURCE/'src/physics/native_tu15_support_carrier.cpp',
   'frames.cpp':ROOT/'src/physics/native_tu15_body_constraint_update.cpp',
   'geometry.cpp':ROOT/'src/physics/native_tu15_geometry.cpp',
   'allocator.cpp':ROOT/'src/core/native_tu15_data_array_allocate.cpp',
   'inverse.cpp':ROOT/'src/rendering/world/native_tu15_camera_projection.cpp',
   'point.cpp':ROOT/'src/physics/native_tu15_contact_body_update.cpp',
   'handoff.cpp':SOURCE/'src/physics/native_tu15_support_carrier_handoff.cpp'}
  pins={str(path):sha(path)for path in paths.values()}
  for name,path in paths.items():shutil.copyfile(path,project/name)
  driver=driver_source();(project/'driver.cpp').write_text(driver);(a.output/'qualified-driver.cpp.txt').write_text(driver);source=paths['support.cpp'].read_text()
  mutants={
   'wrong_delta':('subss %1,%0','addss %1,%0'),
   'wrong_default_slot':('component+0x18u,1','component+0x19u,1'),
   'missing_identity_basis':('copy(m,o,0x184fc0u,identity+4u,r.basis,9);','(void)identity;'),
   'wrong_product_up':('up=row(m,first+0x1cu)','up=row(m,first+0x10u)'),
   'wrong_point_result':('read(m,scratch.velocity+n*4u)','read(m,scratch.velocity+(n==1u?2u:n)*4u)')}
  for name,(old,new)in mutants.items():
   assert source.count(old)==1,(name,source.count(old));(project/(name+'.cpp')).write_text(source.replace(old,new))
  (project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nassert r.returncode!=0 and "differ"in r.stderr,r.stderr\nprint(r.stderr.strip())\n')
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(support_carrier LANGUAGES CXX)\ninclude(CTest)\n'
  for name,units in(('deps','frames.cpp geometry.cpp allocator.cpp inverse.cpp point.cpp handoff.cpp'),('driver_direct','driver.cpp'),('driver_handoff','driver.cpp')):
   cmake+=f'add_library({name} OBJECT {units})\ntarget_include_directories({name} PRIVATE include)\ntarget_compile_features({name} PRIVATE cxx_std_20)\ntarget_compile_options({name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math -ffunction-sections -fdata-sections)\n'
  cmake+='target_compile_definitions(driver_handoff PRIVATE HALO2_CARRIER_HANDOFF=1)\n'
  for name in('native','handoff',*mutants):
   unit='support.cpp'if name in('native','handoff')else name+'.cpp';driver='driver_handoff'if name=='handoff'else'driver_direct';cmake+=f'add_executable(carrier_{name} $<TARGET_OBJECTS:deps> $<TARGET_OBJECTS:{driver}> {unit})\ntarget_include_directories(carrier_{name} PRIVATE include)\ntarget_compile_features(carrier_{name} PRIVATE cxx_std_20)\ntarget_compile_options(carrier_{name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
   cmake+=f'target_link_options(carrier_{name} PRIVATE -Wl,--gc-sections)\n'
   prefix=''if name in('native','handoff')else f'"{sys.executable}" "{project}/negative.py" '
   cmake+=f'add_test(NAME carrier_{name} COMMAND {prefix}$<TARGET_FILE:carrier_{name}> "{fixture}")\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in('g++','clang++'):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','2','--test-jobs','1',*sum((['--target','carrier_'+name]for name in('native','handoff',*mutants)),[]),'--filter','^carrier_']
   result=subprocess.run(command,capture_output=True,text=True);(a.output/(compiler+'-runner.log')).write_text(result.stdout+result.stderr);results.append(dict(compiler=compiler,exit_code=result.returncode));assert result.returncode==0,result.stdout[-2000:]+result.stderr[-2000:]
  pins.update(source_pins);assert all(sha(path)==pin for path,pin in pins.items());visited={pc for c in rows for pc in c['visited']}
  report.update(passed=True,original_sha256=NEW_PIN,cases_per_compiler=len(rows),fault_prefixes=sum('fault'in c for c in rows),observer_fault_prefixes=sum('observer_fault'in c for c in rows),
      listing_sha256=meta['listing_sha256'],body_coverage={n:dict(covered=sum(pc in visited for pc in complete_body(code,int(n,16)).code),sites=count)for n,count in meta['body_sites'].items()},
      negative_controls=list(mutants),compiler_results=results,source_hashes=pins,
      direct_and_actual_handoff_compared=True,caller_fp_arena_refusal_controls_per_compiler=28,
      preserved_x87_depths=sorted({c.get('x87_depth',0)for c in rows}),
      occupied_x87_profiles_per_compiler=sum(c.get('x87_depth',0)>0 for c in rows),
      native_hardware_x87_payload_and_abi_preservation=True,
      rep_prewrite_operand_boundaries=sum(c.get('declared_rep_prewrite_boundary',False)for c in rows),
      rep_prewrite_scope='For an injected write rejection of REP MOVSD, unchanged original execution stops at that instruction after its ordinary source-read operand is recorded. This is a typed pre-write operand boundary, not a claim to single-step inside an x86 instruction.',
      private_temporary_scope='Only explicit callee-private typed frame/point values are excluded from persistent-bank/operand comparison. Every declared external bank is compared unchanged in full; original private CPU/stack/volatile postimages are not supplied to native.',full_live_world_qualification=False)
 except BaseException as error:report['error']=str(error);raise
 finally:save(a.output/'report.json',report)
 return 0
if __name__=='__main__':raise SystemExit(main())
