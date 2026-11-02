#!/usr/bin/env python3
"""Physical30CD00 creation/reduction through the sole shared native services."""
import inspect,re
from pathlib import Path
import validate_native_contact_publication_creation_tu15 as parent

def bind_driver(driver):
    driver='#include "halo2/native_tu15_contact_publication_binding.hpp"\n'+driver
    begin=driver.index('      NativeTu15ContactPublicationServices publication;')
    end=driver.index('      try{const auto args=',begin)
    return driver[:begin]+'''      NativeTu15ContactPublicationNativeServices providers;
      providers.creation.allocation=[&](auto&){return growth;};
      providers.creation.observe_listener=[&](const auto&view,auto listener,auto target,auto event){std::array<std::byte,2>id{};std::array<std::byte,32>fields{};view.read(event,id);view.read(event+4u,fields);listener_calls.push_back(J{{"owner",target.identity},{"receiver",listener},{"id",hex(id)},{"fields",hex(fields)}});};
      providers.remove_point=[&](auto&view,auto manager,auto id,auto&state){NativeTu15ContactPointRemovalServices removal;removal.collector_removed=[&](auto island,auto target,auto,auto info,auto&){if(target.identity!=0x320100u)throw Required{};native_tu15_remove_island_constraint_info(view,{island},{info});};native_tu15_remove_contact_point(view,manager,id,{PRIVATE+0x300u,PRIVATE+0x320u,PRIVATE+0x340u},state,removal);};
      NativeTu15PairDeletionNativeScope native_scope;NativeTu15ContactPublicationBindingActivity bound_activity;
      auto publication=native_tu15_make_contact_publication_services(memory,{{PRIVATE+0x200u},{PRIVATE+0x300u,PRIVATE+0x320u,PRIVATE+0x340u}},0,providers,native_scope,&bound_activity);
'''+driver[end:]

def main():
    root=parent.ROOT
    pending=[root/'include/halo2/native_tu15_contact_publication_binding.hpp',
        *(root/'src/physics'/name for name in parent.UNITS),Path(__file__),
        root/'tools/validate_native_contact_publication_creation_tu15.py',
        root/'tools/validate_native_contact_point_create_tu15.py']
    pinned={}
    while pending:
        path=pending.pop();name=str(path.relative_to(root))
        if name in pinned:continue
        pinned[name]=parent.component.sha(path)
        for include in re.findall(r'^#include "([^"]+)"',path.read_text(),re.M):
            child=root/'include'/include if include.startswith('halo2/')else path.parent/include
            if child.is_file():pending.append(child)
    source=inspect.getsource(parent.main)
    marker=' env=dict(component.__dict__,__file__=__file__,UNITS=UNITS,cases=cases);'
    assert source.count(marker)==1
    inject='''
 marker="  (project/'driver.cpp').write_text(driver);source="
 assert text.count(marker)==1
 text=text.replace(marker,"  driver=bind_driver(driver)\\n"+marker)
 text=text.replace(' try:\\n  assert sha(NEW_DEFAULT)'," try:\\n  (a.output/'input-source-pins.json').write_text(json.dumps(pinned,indent=2)+'\\\\n')\\n  assert sha(NEW_DEFAULT)")
 needle=';report.update(passed=True,profiles='
 assert text.count(needle)==1
 text=text.replace(needle,";assert all(sha(ROOT/name)==value for name,value in pinned.items()),'Publication binding source inputs changed';report.update(input_source_pins=pinned,source_inputs_unchanged=True,passed=True,profiles=")
'''
    replacement=' env=dict(component.__dict__,__file__=__file__,UNITS=UNITS,cases=cases,bind_driver=bind_driver,pinned=pinned);'
    source=source.replace(marker,inject+replacement)
    environment=dict(parent.__dict__,__file__=__file__,bind_driver=bind_driver,pinned=pinned)
    exec(source,environment)
    return environment['main']()

if __name__=='__main__':main()
