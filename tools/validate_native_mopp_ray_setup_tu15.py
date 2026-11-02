#!/usr/bin/env python3
"""Native30B890 plus2E9750 setup to genuine30AA70 entry, returns/FP/ordered DATA and faults."""
import inspect,json,struct
from pathlib import Path
import validate_native_dirty_island_objects_composed_tu15 as support
import validate_native_contact_pressure_tu15 as base
from oracle_smg_firing_1_1 import _Code
from generate_physics_live_support_tu15 import complete_body
OWNERS=(0x30b890,0x2e9750,0x2e96a0)
BACKEND='unchanged-tu15-native-mopp-ray-setup-v1'
ROOT=Path(__file__).resolve().parents[1]
def metadata(image):
 support.OWNERS=OWNERS;support.BOUNDARIES=(0x30aa70,)
 text=inspect.getsource(support.metadata).replace('assert counts==[276,38,20];','assert counts==[66,24,3];')
 text=text.replace("read=bool(op.access&CS_AC_READ)or ins.mnemonic=='test',write=bool(op.access&CS_AC_WRITE)","read=False if ins.mnemonic in('fst','fstp')else bool(op.access&CS_AC_READ)or ins.mnemonic=='test',write=bool(op.access&CS_AC_WRITE)or ins.mnemonic in('fst','fstp')")
 environment=dict(support.__dict__);exec(text,environment);return environment['metadata'](image)
def cases(image):
 rows=[]
 for entry in(0x30b890,0x2e9750):
  for pattern in range(8):
   for rounding in range(4):
    data=bytearray((n*37+11)&255 for n in range(0x3000));shape=0x1000100;cache=0x1000300 if entry==0x30b890 else base.physical.SP-0x80;collection=0x1000500;code=0x1000800;ray=0x1001000;context=0x1001200;collector=0x1001400
    def put(at,value):struct.pack_into('<I',data,at-0x1000000,value&0xffffffff)
    vectors=((0x3f800000,0xbe800000,0x3e000000,0),(0x7fc12345,0x3f800000,0,0),(0x7f812345,0,0,0),(0,0,0,0),(0x7f800000,0xff800000,0x3f800000,0),(1,0x80000001,0x00800000,0),(0x7f7fffff,0xff7fffff,0x3f800000,0),(0x80000000,0,0xbf800000,0))
    for lane,value in enumerate(vectors[pattern]):put(code+lane*4,value);put(ray+lane*4,vectors[(pattern+1)%8][lane]);put(ray+16+lane*4,vectors[(pattern+2)%8][lane])
    put(code+12,(0x3f800000,0x40000000,0x3f000000,0,0x7fc12345,0x7f812345,0x7f800000,1)[pattern]);put(collector+4,(0x3f800000,0x3f000000,0,0x7fc12345)[pattern%4]);put(ray+32,0xaabbccdd);put(ray+36,0);put(shape+12,collection);put(shape+16,code)
    banks=[dict(address=0x1000000,before=data.hex()),dict(address=0x46a0d0,before=image.read(0x46a0d0,4).hex()),dict(address=0x46a1bc,before=image.read(0x46a1bc,4).hex())]
    if entry==0x2e9750:banks.append(dict(address=cache,before=bytes((n*11+pattern)&255 for n in range(0x68)).hex()))
    n=len(rows);rows.append(dict(name=f'mopp_setup_{n}_{entry:x}_{pattern}',entry=entry,receiver=cache if entry==0x30b890 else shape,cache=cache,arguments=[collection,code,ray,context,collector]if entry==0x30b890 else[ray,context,collector],banks=banks,before_fp=[(0x7f,0x27f,0x37f)[n%3]|(rounding<<10),((n%8)<<11)|0x4500|(n%32),0x1f80|(rounding<<13)],stop_pc=base.physical.STOP,boundary=None))
 for seed in(rows[0],rows[8],rows[32]):c=dict(seed);c['name']='fault_seed_'+seed['name'];rows.append(c)
 return rows
def main():
 original_observer=support.parent.observer_source;owner_observer=lambda:original_observer().replace('{"boundary_packet",packet}','{"boundary_packet",packet},{"mopp_variables",(std::uint32_t(regs.rip)==0x30aa70u?hex(memory(child.pid,stack.at(1).get<std::uint32_t>(),32)):std::string{})},{"mopp_bounds",(std::uint32_t(regs.rip)==0x30aa70u?hex(memory(child.pid,stack.at(3).get<std::uint32_t>(),32)):std::string{})}');base.physical.observer_source=owner_observer;base.physical.physical=support.parent.physical;base.OWNERS=OWNERS;base.BACKEND=BACKEND;base.metadata=metadata;base.cases=cases;text=inspect.getsource(base.main)
 begin=text.index('  def boundary(c,actual):');end=text.index('  physical.boundary=boundary',begin);text=text[:begin]+"  def boundary(c,actual):\n   if actual['actual_stop']==physical.STOP:return None\n   assert actual['actual_stop']==0x30aa70\n   return dict(owner=0x30aa70,cache=actual['boundary_gp'][1],code=actual['boundary_stack'][2],variables=actual['mopp_variables'],bounds=actual['mopp_bounds'])\n"+text[end:]
 text=text.replace("source=ROOT/'src/physics/native_tu15_contact_pressure.cpp'","source=ROOT/'src/physics/native_tu15_mopp_ray_setup.cpp'")
 injected='''driver=(ROOT/'tests/native_tu15_contact_pressure_tests.cpp').read_text();driver=driver.replace('#include "halo2/native_tu15_contact_pressure.hpp"','#include "halo2/native_tu15_mopp_ray_setup.hpp"').replace('#include "halo2/native_tu15_contact_pressure_binding.hpp"','').replace('unchanged-tu15-native-contact-pressure-v1',BACKEND).replace('J::array({220,2,2,2})','J::array({66,24,3})').replace('auto services=native_tu15_contact_pressure_native_services();','NativeTu15MoppRaySetupServices services;').replace('NativeTu15ContactPressureStats stats;','std::uint32_t result{};').replace('native_tu15_prune_contact_pressure(m,fp,services,observer,&stats);',CALLS);begin=driver.index('   services.select_candidate=');end=driver.index('   const auto cw=',begin);driver=driver[:begin]+NATIVE_SERVICES+driver[end:];driver=driver.replace('   check(failed==fault_case',RETURN_CHECK+'   check(failed==fault_case');(project/'driver.cpp').write_text(driver);variants={}'''
 text=text.replace("shutil.copyfile(ROOT/'tests/native_tu15_contact_pressure_tests.cpp',project/'driver.cpp');variants={}",injected)
 begin=text.index('  for name,needle,replacement in [');end=text.index("\n  shutil.copyfile(ROOT/'src/physics/native_tu15_activation_services.cpp'",begin)
 text=text[:begin]+'''  for name,needle,replacement in [('inverse_scale','fdivs %0','fmuls %0'),('start_normalization','multiply(packed_scale,start_delta)','multiply(packed_scale,end_delta)'),('fraction_field','cache+0x54u,fraction','cache+0x54u,0u')]:
   assert needle in text;variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
'''+text[end:]
 
 text=text.replace("ROOT/'include/halo2/native_tu15_contact_pressure.hpp',ROOT/'include/halo2/native_tu15_contact_pressure_binding.hpp',ROOT/'src/physics/native_tu15_activation_services.cpp'","ROOT/'include/halo2/native_tu15_mopp_ray_setup.hpp'")
 calls='''if(c.at("entry")==0x30b890){const auto a=c.at("arguments");native_tu15_prepare_mopp_ray(m,c.at("cache"),a[0],a[1],a[2],a[3],a[4],fp,0,services,observer);}else{const auto a=c.at("arguments");native_tu15_cast_instanced_ray_shape(m,c.at("receiver"),a[0],a[1],a[2],c.at("cache"),fp,0,services,observer);}'''
 check=''
 native_services='''   services.traverse=[&](auto cache,auto code,const auto&variables,const auto&bounds,auto&,auto depth){const auto&b=c.at("boundary");check(cache==b.at("cache")&&code==b.at("code")&&depth==0,name+" MOPP boundary formals differ");std::array<std::uint32_t,8>v,r;std::copy(variables.translation.begin(),variables.translation.end(),v.begin());std::copy(variables.fields10_1c.begin(),variables.fields10_1c.end(),v.begin()+4);std::copy(bounds.start.begin(),bounds.start.end(),r.begin());std::copy(bounds.end.begin(),bounds.end.end(),r.begin()+4);check(hex(std::as_bytes(std::span(v)))==b.at("variables").get<std::string>()&&hex(std::as_bytes(std::span(r)))==b.at("bounds").get<std::string>(),name+" Native computedMOPP variables/bounds differ");boundary(0x30aa70u);};
'''
 text=text.replace("shutil.copyfile(ROOT/'src/physics/native_tu15_activation_services.cpp',project/'types.cpp')","(project/'types.cpp').write_text('');[shutil.copyfile(ROOT/'src/physics'/name,project/name)for name in('native_tu15_joint_constraint_numeric_internal.hpp','native_tu15_instanced_ray_lifetime.cpp','native_tu15_island_activation.cpp')]")
 text=text.replace("+'.cpp types.cpp)","+'.cpp types.cpp native_tu15_instanced_ray_lifetime.cpp native_tu15_island_activation.cpp)")
 environment=dict(base.__dict__,__file__=__file__,CALLS=calls,RETURN_CHECK=check,NATIVE_SERVICES=native_services);exec(text,environment);code=environment['main']();path=Path(base.sys.argv[base.sys.argv.index('--output')+1])/'report.json';report=json.loads(path.read_text());codebook=_Code(base.Xbe(base.NEW_DEFAULT),'.text');sites=set().union(*(set(complete_body(codebook,owner).code)for owner in OWNERS));report.update(physically_executed_sites=len(sites&support.parent.EXECUTED),unexecuted_sites=sorted(sites-support.parent.EXECUTED),prefetch_policy='OriginalPREFETCHT0 unchanged/nonfaulting; no semantic memory read invented. Surrounding architectural pool stride/base reads remain strict.',mopp_boundary_contract='Originalvariables/bounds arecomparisonoutputs only; nativecomputes allfields fromcurrent DATA, no suppliedchildresult',scope=__doc__,remaining=['Actual16C620 query parent/BSP/object closure and real input/currentFP composition','Production native routing/typed service composition and visible camera/strictOriginal']);base.save(path,report);return code
if __name__=='__main__':raise SystemExit(main())
