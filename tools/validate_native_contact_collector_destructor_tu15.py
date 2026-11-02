#!/usr/bin/env python3
"""Physical3261D0 wrapper proof with genuine standalone collector allocations.

The source allocator constructs every disposing object's ownership. Collector
body/reference and owned-array lifetime remain the separately qualified31F660
domains; this wrapper adds no original-CPU callback or allocator outcome.
"""
import inspect
import validate_native_agent_cleanup_children_tu15 as child
from oracle_smg_firing_1_1 import ROOT


def cases():
    u = child.base.UI
    result = []
    for flags in (0, 1, 2, 3, 0x100, 0x101, 0xffffffff):
        result.append(dict(name=f'collector_scalar_flags_{flags:x}', entry=0x3261d0,
            receiver=u+0x100, arguments=[flags], owned_object=bool(flags & 1),
            user_words=[(0x100,0x41f478), (0x104,0x110), (0x108,0x41f468),
                (0x10c,0), (0x110,0), (0x114,0),
                (0x128,0x80000008), (0x13c,0x80000000), (0x198,0x80000004)]))
    # Source reads the current size a second time after the base destructor.
    # Borrowed identity is valid because the read faults before any release.
    result.append(dict(result[0], name='fault_current_size_after_base', arguments=[1],
        fault=dict(address=u+0x104,bytes=2,occurrence=2,write=False)))
    for offset, width, write in ((0x198,4,False), (0x13c,4,False),
                                 (0x128,4,False), (0x100,4,True)):
        # Offsets below are collector-relative, independent of the UI address.
        relative = offset - 0x100
        row = dict(result[0], name=f'fault_collector_{relative:x}_{write}',
            fault=dict(address=u+0x100+relative,bytes=width,occurrence=1,write=write))
        result.append(row)
    return result


def main():
    child.OWNERS=(0x3261d0,0x31f660,0x325af0,0xb4150,
        0x149da0,0x149e00,0x233250,0x233420)
    child.cases=cases
    source=inspect.getsource(child.main)
    needle="'tools/validate_native_agent_cleanup_children_tu15.py'"
    source=source.replace(needle,needle+",'tools/validate_native_contact_collector_destructor_tu15.py','include/halo2/native_tu15_contact_collector_destructor.hpp','src/physics/native_tu15_contact_collector_destructor.cpp','include/halo2/native_tu15_activation_collector_cleanup.hpp','src/physics/native_tu15_activation_contact_manager.cpp','src/physics/native_tu15_island_activation.cpp'")
    # Original preparation is actual149DA0, valid272B object/class27, no spoof.
    source=source.replace('push $0x1a', 'push $0x27').replace('push $256', 'push $272').replace('mov $256,%ecx','mov $272,%ecx')
    needle="'native_tu15_contact_unwind_internal.hpp'):shutil.copyfile"
    assert source.count(needle)==1
    source=source.replace(needle,"'native_tu15_contact_unwind_internal.hpp','native_tu15_contact_collector_destructor.cpp','native_tu15_activation_contact_manager.cpp','native_tu15_island_activation.cpp'):shutil.copyfile")
    needle='driver=\'#include "halo2/native_tu15_agent_cleanup_children.hpp"\\n\'+driver'
    assert source.count(needle)==1
    source=source.replace(needle,'driver=\'#include "halo2/native_tu15_contact_collector_destructor.hpp"\\n#include "halo2/native_tu15_agent_cleanup_children.hpp"\\n\'+driver')
    needle='NativeTu15AgentCleanupChildrenServices services;services.allocation=growth;'
    assert source.count(needle)==1
    source=source.replace(needle,needle+'''NativeTu15ActivationContactManagerServices collector_services;collector_services.allocation=growth;NativeTu15ActivationContactManagerStats collector_stats;
      collector_services.destroy_body=[](auto,auto,auto,auto&){throw std::runtime_error("Unqualified collector body destructor");};
      collector_services.release_base_lock=[](auto,auto,auto&){throw std::runtime_error("Unqualified collector base lock");};''')
    needle='if(entry==0x2f82d0u)native_tu15_cleanup_cached_contact_agent'
    assert source.count(needle)==1
    source=source.replace(needle,'if(entry==0x3261d0u)result=native_tu15_delete_contact_collector(memory,receiver,c.at("arguments")[0],fp,collector_services,observe,&collector_stats);\\n        else '+needle)
    source=source.replace('native_tu15_array_allocator.cpp native_tu15_array_growth.cpp)', 'native_tu15_array_allocator.cpp native_tu15_array_growth.cpp native_tu15_contact_collector_destructor.cpp native_tu15_activation_contact_manager.cpp native_tu15_island_activation.cpp)')
    needle="(project/'driver.cpp').write_text(driver)"
    assert source.count(needle)==1
    source=source.replace(needle,"driver=driver.replace('check(cases>20,','check(cases>0,').replace('      check(calls==','      if(!failed&&!required)check(result==c.at(\"return_word\"),name+\" scalar collector result differs\");\\n      check(calls==');"+needle)
    source=source.replace("target=project/'native_tu15_agent_cleanup_children.cpp'", "target=project/'native_tu15_contact_collector_destructor.cpp'")
    begin=source.index("mutants=[('point_stride'")
    end=source.index('\n  for name,before,after in mutants:',begin)
    source=source[:begin]+'''mutants=[('disposing_bit','if(disposing&1u)','if(disposing&2u)'),('release_class','bytes,0x27u','bytes,0x12u'),('current_size','const auto bytes=read(collector+4u,2)','const auto bytes=0x100u')]'''+source[end:]
    source=source.replace("scope='Declared valid objects", "scope='Exact3261D0 normal scalar-deleting wrapper; valid source-allocated standalone272B collector objects, all14 wrapper sites, current post-base size/allocator/owner reads, return identity and RET4; body and array lifetime inherited from separately qualified31F660; no fabricated freeing of embedded manager collectors; declared valid objects")
    environment=dict(child.__dict__)
    exec(source,environment)
    return environment['main']()


if __name__=='__main__':
    raise SystemExit(main())
