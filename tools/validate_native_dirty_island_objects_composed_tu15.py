#!/usr/bin/env python3
"""Native282C10 plus genuine1CC310/BA8C0 native component-state helpers."""
import inspect,json
from pathlib import Path
import validate_native_dirty_island_objects_tu15 as parent
from capstone.x86_const import X86_OP_MEM
from capstone import CS_AC_READ,CS_AC_WRITE
from oracle_smg_firing_1_1 import _Code
from generate_physics_live_support_tu15 import complete_body
OWNERS=(0x282c10,0x1cc310,0xba8c0)
BOUNDARIES=(0x1496f0,0x1d7ca0,0x16e8a0,0xba570,0x1d9510,0x1d8470,0x1d73d0,0x1d97f0)
def metadata(image):
 code=_Code(image,'.text');registers={n:i for i,n in enumerate(('eax','ecx','edx','ebx','esp','ebp','esi','edi'))};sites=[];counts=[];pins={}
 for entry in OWNERS:
  body=complete_body(code,entry);counts.append(len(body.code));listing=[]
  for pc in sorted(body.code):
   ins=code.at(pc);listing.append(f'{pc:x} {ins.bytes.hex()} {ins.mnemonic} {ins.op_str}\n');ops=[]
   for op in ins.operands:
    if op.type!=X86_OP_MEM or ins.mnemonic=='lea':continue
    assert not op.mem.segment;base,index=ins.reg_name(op.mem.base),ins.reg_name(op.mem.index)
    ops.append(dict(base=registers.get(base,-1),index=registers.get(index,-1),scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(op.access&CS_AC_READ)or ins.mnemonic=='test',write=bool(op.access&CS_AC_WRITE)))
   sites.append(dict(instruction=pc,operands=ops))
  pins[hex(entry)]=parent.parent.hashlib.sha256(''.join(listing).encode()).hexdigest()
 assert counts==[276,38,20];return dict(sites=sites,body_sites=counts,listing_sha256=pins,boundaries=list(BOUNDARIES),regions=[])
def main():
 parent.metadata=metadata;parent.OWNERS=OWNERS;parent.BOUNDARIES=BOUNDARIES
 case_text=inspect.getsource(parent.cases).replace('component=0xffffffff,flags=0):','component=0xffffffff,flags=0,property_kind=0x2001):').replace('put(P+i*8,0x2001)','put(P+i*8,property_kind)').replace('put(H+i*12+3,kind,1)','put(H+i*12+3,kind[i%len(kind)]if isinstance(kind,tuple)else kind,1)').replace(' return rows'," make('mixed_native',dirty=1,bodies=8,mode=4,kind=(0,1,7,1));make('property_miss',dirty=1,bodies=4,mode=4,property_kind=0x2000);make('fault_seed_mixed',dirty=1,bodies=8,mode=4,kind=(0,1,7,1))\n return rows")
 environment=dict(parent.__dict__);exec(case_text,environment);parent.cases=environment['cases']
 parent.NATIVE_BOUNDARIES='''   const auto check_argument=[&](const char*name,std::uint32_t value){check(c.at("boundary").at(name)==value,name+std::string(" typed native component argument differs"));};
   NativeTu15ComponentStateServices component_services;
   component_services.remove=[&](auto component,auto&,auto){check_argument("component",component);boundary(0x1d9510u);};
   component_services.reset=[&](auto component,auto&,auto){check_argument("component",component);boundary(0x1d8470u);};
   component_services.refresh=[&](auto datum,auto&,auto){check_argument("datum",datum);boundary(0x1d73d0u);};
   component_services.activate=[&](auto component,auto&,auto){check_argument("component",component);boundary(0x1d97f0u);};
   services.set_component_flags=[&](auto datum,auto flags,auto&fp,auto depth){native_tu15_set_component_state_flags(m,datum,flags,fp,depth,component_services,observer);};
   services.defer_object=[&](auto datum,auto&fp,auto depth){native_tu15_defer_component_object(m,datum,fp,depth,component_services,observer);};
   services.collect_memory=[&](auto&,auto){boundary(0x1496f0u);};
   services.destroy_component=[&](auto component,auto&,auto){check_argument("component",component);boundary(0x1d7ca0u);};
   services.free_component=[&](auto pool,auto datum,auto&,auto){check_argument("pool",pool);check_argument("datum",datum);boundary(0x16e8a0u);};
   services.delete_object=[&](auto datum,auto&,auto){check_argument("datum",datum);boundary(0xba570u);};
   services.remove_component=component_services.remove;
'''
 text=inspect.getsource(parent.main)
 text=text.replace('elif owner==0x1d9510:','elif owner in(0x1d9510,0x1d97f0):').replace('   return result\n','   if owner==0x1d8470:result.update(component=args[1])\n   if owner==0x1d73d0:result.update(datum=gp[0])\n   return result\n')
 at=text.index(' env=dict(parent.__dict__,boundary_owners=')
 injected="driver=driver.replace('J::array({276})','J::array({276,38,20})').replace("+repr('#include "halo2/native_tu15_dirty_island_objects.hpp"')+","+repr('#include "halo2/native_tu15_dirty_island_objects.hpp"\n#include "halo2/native_tu15_component_state.hpp"')+");(project/'driver.cpp').write_text(driver)"
 extra=" text=text.replace("+repr("  (project/'types.cpp').write_text('')")+","+repr("  shutil.copyfile(ROOT/'src/physics/native_tu15_component_state.cpp',project/'component.cpp');(project/'types.cpp').write_text('')")+")\n"
 extra+=" text=text.replace("+repr("+'.cpp types.cpp)")+","+repr("+'.cpp types.cpp component.cpp)")+")\n"
 extra+=" text=text.replace("+repr("(project/'driver.cpp').write_text(driver)")+","+repr(injected)+")\n"
 filter_code="rmw={site['instruction']for site in meta['sites']if any(op['read']and op['write']for op in site['operands'])};excluded_write_faults=[c['name']for c in prefix if c.get('fault',{}).get('access')=='write'and c['stop_pc']in rmw];prefix=[c for c in prefix if c['name']not in excluded_write_faults];rows+=physical.physical(image,meta,prefix,scratch)"
 extra+=" text=text.replace("+repr('rows+=physical.physical(image,meta,prefix,scratch)')+","+repr(filter_code)+")\n"
 extra+=" text=text.replace("+repr('report.update(passed=')+","+repr("report.update(excluded_preinstruction_RMW_write_fault_cases=excluded_write_faults,passed=")+")\n"

 text=text[:at]+extra+text[at:];env=dict(parent.__dict__);exec(text,env);code=env['main']()
 path=Path(parent.parent.sys.argv[parent.parent.sys.argv.index('--output')+1])/'report.json';report=json.loads(path.read_text());report['source_hashes'].update({str(p.relative_to(parent.parent.ROOT)):parent.parent.sha(p)for p in(parent.parent.ROOT/'include/halo2/native_tu15_component_state.hpp',parent.parent.ROOT/'src/physics/native_tu15_component_state.cpp',Path(__file__))});report.update(native_component_state_helpers=True,scope='Whole public282C10 source+native1CC310/BA8C0 helpers; full no-component mixed selected/deferred publications versus unchanged original, deeper selected owner prefixes explicit; no supplied child outcomes or GP/afterimages.');parent.parent.save(path,report);return code
if __name__=='__main__':raise SystemExit(main())
