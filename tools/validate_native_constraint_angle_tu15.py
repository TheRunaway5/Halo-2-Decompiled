#!/usr/bin/env python3
"""Exact37C6B0 authored x87 angle, extended result and partial-FP read faults."""
import inspect,json
from pathlib import Path
import validate_native_dirty_island_objects_composed_tu15 as support
import validate_native_contact_pressure_tu15 as base
from oracle_smg_firing_1_1 import _Code
from generate_physics_live_support_tu15 import complete_body
OWNERS=(0x37c6b0,)
BACKEND='unchanged-tu15-native-constraint-angle-v1'
ROOT=Path(__file__).resolve().parents[1]
def metadata(image):
 support.OWNERS=OWNERS;support.BOUNDARIES=()
 text=inspect.getsource(support.metadata).replace('assert counts==[276,38,20];','assert counts==[48];');environment=dict(support.__dict__);exec(text,environment);return environment['metadata'](image)
def cases(image):
 rows=[];pairs=[(0,0),(0x80000000,0),(0,0x80000000),(0x80000000,0x80000000),(0x3f800000,0),(0xbf800000,0),(0,0x3f800000),(0,0xbf800000),(0x3f000000,0x3f800000),(0x3f800000,0x3f000000),(0x3f800000,0x3f800000),(0xbf800000,0x3f000000),(0x3f000000,0xbf800000),(0xbf800000,0xbf000000),(1,0x3f800000),(0x80000001,1),(0x7f7fffff,0x00800000),(0x00800000,0x7f7fffff),(0x7f800000,0x3f800000),(0x3f800000,0xff800000),(0x7f800000,0x7f800000),(0x7fc12345,0x3f800000),(0x3f800000,0xffc54321),(0x7f812345,0x3f800000),(0x3f800000,0xff854321),(0x7f812345,0xff854321)]
 constants=[dict(address=p,before=image.read(p,n).hex())for p,n in((0x4485cc,16),(0x41c4cc,4),(0x46a0e8,4))]
 for pair in pairs:
  for precision in(0,2,3):
   for rounding in range(4):
    n=len(rows);rows.append(dict(name=f'angle_{pair[0]:x}_{pair[1]:x}_{precision}_{rounding}',entry=0x37c6b0,receiver=0,arguments=list(pair),banks=constants,before_fp=[0x7f|(precision<<8)|(rounding<<10),((n%8)<<11)|0x4500|(n%32),0x1f80|((n%4)<<13)|(0x40 if n&1 else 0)],stop_pc=base.physical.STOP,boundary=None))
 for index in(8*12,9*12,11*12,12*12):c=dict(rows[index]);c['name']='fault_seed_'+c['name'];rows.append(c)
 return rows
SCOPE=r'''class TestAngleEnvironment {
 alignas(16)std::array<std::byte,108>host_{};NativeTu15PhysicsFrameFp&fp_;unsigned host_mxcsr_;
public:explicit TestAngleEnvironment(NativeTu15PhysicsFrameFp&fp):fp_(fp),host_mxcsr_(_mm_getcsr()){
  asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
  std::array<std::byte,28>env{};std::memcpy(env.data(),host_.data(),28);const std::uint16_t empty=0xffff;
  std::memcpy(env.data(),&fp.control_word,2);std::memcpy(env.data()+4,&fp.status,2);std::memcpy(env.data()+8,&empty,2);
  asm volatile("fldenv %0"::"m"(env):"memory");_mm_setcsr(fp.mxcsr);
 }
 ~TestAngleEnvironment(){asm volatile("fnstcw %0":"=m"(fp_.control_word));asm volatile("fnstsw %0":"=a"(fp_.status));fp_.mxcsr=_mm_getcsr();asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(host_mxcsr_);}
};
'''
def main():
 base.physical.observer_source=support.parent.observer_source;base.physical.physical=support.parent.physical;base.OWNERS=OWNERS;base.BACKEND=BACKEND;base.metadata=metadata;base.cases=cases;text=inspect.getsource(base.main)
 begin=text.index('  def boundary(c,actual):');end=text.index('  physical.boundary=boundary',begin);text=text[:begin]+"  def boundary(c,actual):\n   assert actual['actual_stop']==physical.STOP;return None\n"+text[end:]
 text=text.replace("source=ROOT/'src/physics/native_tu15_contact_pressure.cpp'","source=ROOT/'src/physics/native_tu15_constraint_angle.cpp'")
 injected='''driver=(ROOT/'tests/native_tu15_contact_pressure_tests.cpp').read_text();driver=driver.replace('#include "halo2/native_tu15_contact_pressure.hpp"','#include "halo2/native_tu15_constraint_angle.hpp"\\n#include <cstring>').replace('#include "halo2/native_tu15_contact_pressure_binding.hpp"','').replace('unchanged-tu15-native-contact-pressure-v1',BACKEND).replace('J::array({220,2,2,2})','J::array({48})').replace('auto services=native_tu15_contact_pressure_native_services();','').replace('NativeTu15ContactPressureStats stats;','NativeTu15ConstraintAngleStats stats;std::array<std::byte,10>extended{};').replace('native_tu15_prune_contact_pressure(m,fp,services,observer,&stats);','{TestAngleEnvironment environment(fp);native_tu15_constraint_angle_active(m,c.at("arguments")[0],c.at("arguments")[1],0,&stats);alignas(16)std::array<std::byte,512>result{};asm volatile("fxsave %0":"=m"(result)::"memory");std::copy_n(result.begin()+32u,10u,extended.begin());}');begin=driver.index('   services.select_candidate=');end=driver.index('   const auto cw=',begin);driver=driver[:begin]+driver[end:];driver=driver.replace('std::uint16_t control()',test_scope+'std::uint16_t control()').replace('   check(failed==fault_case','   if(!failed)check(hex(extended)==c.at("extended_result").get<std::string>(),name+" original extended angle differs");\\n   check(failed==fault_case');(project/'driver.cpp').write_text(driver);variants={}'''
 text=text.replace("shutil.copyfile(ROOT/'tests/native_tu15_contact_pressure_tests.cpp',project/'driver.cpp');variants={}",injected)
 begin=text.index('  for name,needle,replacement in [');end=text.index("\n  shutil.copyfile(ROOT/'src/physics/native_tu15_activation_services.cpp'",begin)
 text=text[:begin]+'''  for name,needle,replacement in [('cubic','word(memory,0x4485d4u)','word(memory,0x4485d8u)'),('complement','order==0u||order==0x4100u','order==0x0100u||order==0x4000u'),('first_sign','if(first_order==0x0100u||first_order==0x0400u)','if(first_order==0u)')]:
   assert needle in text;variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
'''+text[end:]
 text=text.replace("shutil.copyfile(ROOT/'src/physics/native_tu15_activation_services.cpp',project/'types.cpp')","(project/'types.cpp').write_text('')")
 text=text.replace("ROOT/'include/halo2/native_tu15_contact_pressure.hpp',ROOT/'include/halo2/native_tu15_contact_pressure_binding.hpp',ROOT/'src/physics/native_tu15_activation_services.cpp'","ROOT/'include/halo2/native_tu15_constraint_angle.hpp'")
 environment=dict(base.__dict__,__file__=__file__,test_scope=SCOPE);exec(text,environment);code=environment['main']();path=Path(base.sys.argv[base.sys.argv.index('--output')+1])/'report.json';report=json.loads(path.read_text());sites=set(complete_body(_Code(base.Xbe(base.NEW_DEFAULT),'.text'),0x37c6b0).code);report.update(physically_executed_parent_sites=len(sites&support.parent.EXECUTED),unexecuted_parent_sites=sorted(sites-support.parent.EXECUTED),preserved_x87_depth_physically_tested=0,scope=__doc__,remaining=['Nonempty caller x87 live-value preservation and capacity composition','Native31D930 caller and329100/3294E0/328A00 closure','Actual31D930 source input and live route; no parent-only substitution']);base.save(path,report);return code
if __name__=='__main__':raise SystemExit(main())
