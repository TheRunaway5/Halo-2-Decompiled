#!/usr/bin/env python3
"""Compare native309940 collision-tree operations with unchanged English TU1.5.

Authored MOPP programs and typed quantized bounds are input DATA. Private
alignment holes are not numerical inputs. Key arrays, ordered persistent
reads/stores, complete FP and faults are compared independently.
"""
import inspect,json,struct
from pathlib import Path
import validate_native_dirty_island_objects_composed_tu15 as support
import validate_native_contact_pressure_tu15 as base
from oracle_smg_firing_1_1 import _Code
from generate_physics_live_support_tu15 import complete_body
ROOT=Path(__file__).resolve().parents[1]
BACKEND='unchanged-tu15-native-mopp-aabb-traversal-v1'
OWNERS=(0x309940,)
VARS,CACHE,CODE,KEYS,DATA=0x1002000,0x1000100,0x1001000,0x1000300,0x1000400
def expanded_source():
 projection=(ROOT/'include/halo2/native_tu15_mopp_aabb_projection_kernel.hpp').read_text().removeprefix('#pragma once\n')
 core=(ROOT/'include/halo2/native_tu15_mopp_aabb_traversal_kernel.hpp').read_text().removeprefix('#pragma once\n')
 core=core.replace('#include "halo2/native_tu15_mopp_aabb_projection_kernel.hpp"\n',projection)
 return(ROOT/'src/physics/native_tu15_mopp_aabb_traversal.cpp').read_text().replace('#include "halo2/native_tu15_mopp_aabb_traversal_kernel.hpp"\n',core)
def metadata(image):
 support.OWNERS=OWNERS;support.BOUNDARIES=()
 text=inspect.getsource(support.metadata).replace('assert counts==[276,38,20];','assert counts==[464];')
 text=text.replace('assert not op.mem.segment;',"assert not op.mem.segment or ins.reg_name(op.mem.segment)=='es';")
 env=dict(support.__dict__);exec(text,env);return env['metadata'](image)
def cases(image):
 rows=[]
 def make(name,program,upper=(255,255,255),lower=(0,0,0),fields=(0,0,0,0x12340000,0,0x12345678),cache_upper=(0x1000000,0x1000000,0x1000000),cache_lower=(0,0,0)):
  data=bytearray((n*17+11)&255 for n in range(0x3000))
  def put(p,v):struct.pack_into('<I',data,p-0x1000000,v&0xffffffff)
  data[CODE-0x1000000:CODE-0x1000000+len(program)]=bytes(program)
  for k,v in enumerate(upper):put(VARS+k*4,v)
  for k,v in enumerate(lower):put(VARS+16+k*4,v)
  for k,v in enumerate(fields):put(VARS+32+k*4,v)
  for k in range(3):put(CACHE+0x10+k*4,cache_upper[k]);put(CACHE+0x20+k*4,cache_lower[k])
  put(CACHE,KEYS);put(KEYS,DATA);put(KEYS+4,0);put(KEYS+8,0x80000020)
  n=len(rows);rows.append(dict(name=name,entry=0x309940,receiver=CACHE,arguments=[VARS,CODE],variables=VARS,code=CODE,banks=[dict(address=0x1000000,before=data.hex())],before_fp=[(0x7f,0x27f,0x37f)[n%3]|((n%4)<<10),((n%8)<<11)|0x453f,0x1f80|((n%4)<<13)],stop_pc=base.physical.STOP,boundary=None))
 make('return',[0])
 for op in range(1,5):
  for shift in(0,15,16,17,31,32,0xffffffff):
   make(f'scale_{op}_{shift:x}',[op,12,13,14,0x30],fields=(0xfffffffd,0x12345678,0x80000000,0xfffffff0,shift,0),cache_upper=(0x7fffffff,0x80000000,0xffffffff),cache_lower=(0x80000000,0x7fffffff,0))
  # Observe transformed bounds through a subsequent real clipping decision,
  # rather than accepting dead arithmetic followed by an unconditional leaf.
  for axis in range(3):
   for extent in(8,16,32,64,128,255):
    make(f'scale_then_slab_{op}_{axis}_{extent}',[op,1,2,3,0x26+axis,192,255,0x30],cache_upper=(extent<<16,)*3)
 for op in range(5,8):make(f'jump_{op}',[op]+[0]*(op-5)+[2,255,255,0x30])
 for op,size in((9,1),(10,2),(11,4)):
  for base_key in(0,1,0x7fffffff,0xffffffff):make(f'key_base_{op}_{base_key:x}',[op]+[255]*size+[0x4f],fields=(0,0,0,base_key,0,0))
 for op in (0x60,0x64,0x68):
  size=1 if op<0x64 else 2 if op<0x68 else 4
  make(f'property_{op:x}',[op]+[255]*size+[9,7,0x30])
 for op in range(0x10,0x1d):
  for upper,lower in(((255,255,255),(0,0,0)),((0,0,0),(255,255,255)),((64,64,64),(192,192,192)),((192,192,192),(64,64,64)),((0xffffffff,0x80000000,0x7fffffff),(0x80000000,0x7fffffff,0xffffffff))):
   for threshold in(0,64,128,255):make(f'split_{op:x}_{len(rows)}',[op,threshold,192,1,0x30,0x31],upper,lower)
 for op in range(0x20,0x23):
  for upper in(0,127,128,129,255,0x7fffffff,0x80000000,0xffffffff):
   for lower in(0,128,255):make(f'split_single_{op:x}_{upper:x}_{lower}',[op,128,1,0x30,0x31],(upper,)*3,(lower,)*3)
 for op in range(0x23,0x26):
  for upper in(0,64,192,255,0x7fffffff,0x80000000,0xffffffff):
   for lower in(0,64,192,255):make(f'split_offsets_{op:x}_{upper:x}_{lower}',[op,64,192,0,0,0,1,0x30,0x31],(upper,)*3,(lower,)*3)
 for op in range(0x26,0x29):
  for upper in(0,64,192,255,0x7fffffff,0x80000000,0xffffffff):
   for lower in(0,64,192,255):make(f'slab_{op:x}_{upper:x}_{lower}',[op,64,192,0x30],(upper,)*3,(lower,)*3)
 for op in range(0x29,0x2c):
  for upper in(0,64,192,255,0x7fffffff,0x80000000,0xffffffff):
   for lower in(0,64,192,255):make(f'wide_slab_{op:x}_{upper:x}_{lower}',[op,0,0,64,0,0,192,0x30],cache_upper=(upper,)*3,cache_lower=(lower,)*3)
 for op in range(0x30,0x54):make(f'leaf_{op:x}',[op]+([0xaa]*(op-0x4f)if op>=0x50 else []))
 for left in([9,17,0x30],[10,255,255,0x30],[11,255,255,255,255,0x30],[1,1,2,3,0x30],[0x60,12,9,17,0x30]):
  make('recursive_COW_'+str(len(rows)),[0x10,64,192,len(left),*left,0x30])
 for name,program in(('scale',[1,1,2,3,0x30]),('split',[0x10,64,192,1,0x30,0x31]),('slab',[0x29,0,0,64,0,0,192,0x30]),('leaf',[0x53,0x12,0x34,0x56,0x78])):
  make('fault_seed_'+name,program)
 return rows
def main():
 base.physical.observer_source=support.parent.observer_source
 base.physical.physical=support.parent.physical;base.OWNERS=OWNERS;base.BACKEND=BACKEND;base.metadata=metadata;base.cases=cases
 text=inspect.getsource(base.main)
 begin=text.index('  def boundary(c,actual):');end=text.index('  physical.boundary=boundary',begin)
 text=text[:begin]+'''  def boundary(c,actual):
   assert actual['actual_stop']==physical.STOP,hex(actual['actual_stop']);return None
'''+text[end:]
 # The entire14-word Source packet is read-only input. Project exactly the
 # twelve initialized semantic words; compare all persistent banks unchanged.
 text=text.replace("    for e in seed[category]:\n", "    for e in seed[category]:\n     if VARS<=e['address']<VARS+56:continue\n")
 text=text.replace("source=ROOT/'src/physics/native_tu15_contact_pressure.cpp'","source=ROOT/'src/physics/native_tu15_mopp_aabb_traversal.cpp'")
 # Compile the shared projection core into each owned positive/mutant source.
 # This keeps a changed numerical header under the same genuine oracle rather
 # than allowing copied mutants to use an unmodified external implementation.
 text=text.replace("text=source.read_text();", "text=expanded_source();")
 injected='''driver=(ROOT/'tests/native_tu15_contact_pressure_tests.cpp').read_text()
  driver=driver.replace('#include "halo2/native_tu15_contact_pressure.hpp"','#include "halo2/native_tu15_mopp_aabb_traversal.hpp"').replace('#include "halo2/native_tu15_contact_pressure_binding.hpp"','').replace('unchanged-tu15-native-contact-pressure-v1',BACKEND).replace('J::array({220,2,2,2})','J::array({464})').replace('auto services=native_tu15_contact_pressure_native_services();','NativeTu15MoppAabbTraversalServices services;').replace('NativeTu15ContactPressureStats stats;','NativeTu15MoppAabbTraversalStats stats;').replace('native_tu15_prune_contact_pressure(m,fp,services,observer,&stats);',CALLS)
  begin=driver.index('   services.select_candidate=');end=driver.index('   const auto cw=',begin);driver=driver[:begin]+driver[end:]
  driver=driver.replace('auto m=before;m.log_reads=read_pass;',SETUP)
  driver=driver.replace('for(const auto&r:c.at("reads"))expected.push_back', 'for(const auto&r:c.at("reads"))if(r.at("address").get<std::uint32_t>()<c.at("variables").get<std::uint32_t>()||r.at("address").get<std::uint32_t>()>=c.at("variables").get<std::uint32_t>()+56u)expected.push_back')
  (project/'driver.cpp').write_text(driver);variants={}'''
 text=text.replace("shutil.copyfile(ROOT/'tests/native_tu15_contact_pressure_tests.cpp',project/'driver.cpp');variants={}",injected)
 begin=text.index('  for name,needle,replacement in [');end=text.index("\n  shutil.copyfile(ROOT/'src/physics/native_tu15_activation_services.cpp'",begin)
 text=text[:begin]+'''  for name,needle,replacement in [('leaf_base','d_.key+=d_.base','d_.key+=d_.base+1u'),('scale_shift','d_.shift=16u-d_.quantization','d_.shift=15u-d_.quantization'),('split_inclusive','signed_word(d_.lower)>=signed_word(d_.low)','signed_word(d_.lower)>signed_word(d_.low)'),('projection_sign','d.lower-=d.upper;d.lower-=upper(1);','d.lower+=d.upper;d.lower-=upper(1);')]:
   assert needle in text;variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
'''+text[end:]
 text=text.replace("shutil.copyfile(ROOT/'src/physics/native_tu15_activation_services.cpp',project/'types.cpp')","shutil.copyfile(ROOT/'src/physics/native_tu15_array_growth.cpp',project/'types.cpp')")
 text=text.replace("ROOT/'include/halo2/native_tu15_contact_pressure.hpp',ROOT/'include/halo2/native_tu15_contact_pressure_binding.hpp',ROOT/'src/physics/native_tu15_activation_services.cpp'","ROOT/'include/halo2/native_tu15_mopp_aabb_traversal.hpp',ROOT/'include/halo2/native_tu15_mopp_aabb_setup.hpp',ROOT/'include/halo2/native_tu15_mopp_aabb_projection_kernel.hpp',ROOT/'include/halo2/native_tu15_mopp_aabb_traversal_kernel.hpp',ROOT/'src/physics/native_tu15_array_growth.cpp'")
 calls='native_tu15_traverse_mopp_aabb(m,c.at("receiver"),c.at("code"),variables,fp,0,services,observer,&stats);'
 setup='''auto m=before;m.fault=J{};m.write_fault=J{};NativeTu15MoppAabbVariables variables;const auto vp=c.at("variables").get<std::uint32_t>();const auto word=[&](std::uint32_t p){std::array<std::byte,4>b{};m.read(p,b);std::uint32_t value{};for(unsigned k=0;k<4;++k)value|=std::to_integer<unsigned>(b[k])<<(k*8u);return value;};for(unsigned k=0;k<3;++k){variables.upper[k]=word(vp+k*4u);variables.lower[k]=word(vp+16u+k*4u);}for(unsigned k=0;k<6;++k)variables.translation_and_properties[k]=word(vp+32u+k*4u);m.fault=before.fault;m.write_fault=before.write_fault;m.log_reads=read_pass;'''
 env=dict(base.__dict__,__file__=__file__,CALLS=calls,SETUP=setup,VARS=VARS,expanded_source=expanded_source);exec(text,env);result=env['main']()
 path=Path(base.sys.argv[base.sys.argv.index('--output')+1])/'report.json';r=json.loads(path.read_text());code=_Code(base.Xbe(base.NEW_DEFAULT),'.text');sites=set(complete_body(code,0x309940).code);r.update(scope=__doc__,physically_executed_sites=len(sites&support.parent.EXECUTED),unexecuted_sites=sorted(sites-support.parent.EXECUTED),production_native_only_closure=False,remaining=['Positive array-growth composition and real allocator ownership','Actual caller/native setup composition and visible native key-query delivery','Malformed-program diagnostics, private input-address faults, recursion capacity and wider lifetime domains']);base.save(path,r);return result
if __name__=='__main__':raise SystemExit(main())
