#!/usr/bin/env python3
"""Check both native lift handoffs in their physical TU1.5 parent."""
import inspect

import validate_native_lift_velocity_parent_tu15 as base

PARENT = base.ROOT / 'analysis/physics/native-phantom-body-velocity-parent-stage-v2-20261103/staged'


def driver():
    source = '#include "halo2/native_campaign_object_marker_request_tu15.hpp"\n' + base.driver()
    source = base.replace(source, '  std::map<std::uint32_t,std::byte> data;',
                          '  std::map<std::uint32_t,std::byte> data;\n  std::set<std::uint32_t> *marker_writes{};')
    source = base.replace(source, 'at->second=b;++p;',
                          'at->second=b;if(marker_writes)marker_writes->insert(p);++p;')
    source = base.replace(source, '    halo2::NativeLiftScalarConsumerScopeTu15 scope', r'''
    std::size_t marker_index{};
    policy.request_marker=[&](auto &current,auto object,auto direct,auto marker,auto capacity,auto destination,auto &numeric){
      const auto &requests=row.at("marker_deliveries");
      check(&current==&memory && marker_index<requests.size(),label+" marker owner or count differs");
      const auto &expected=requests.at(marker_index++);
      check(expected.at("object")==object && expected.at("direct")==direct && expected.at("marker")==marker &&
            expected.at("capacity")==capacity && expected.at("destination")==destination,label+" marker inputs differ");
      std::set<std::uint32_t> touched;
      memory.marker_writes=&touched;
      const auto count=halo2::native_campaign_object_marker_request_tu15(current,object,direct,marker,capacity,destination,numeric);
      memory.marker_writes=nullptr;
      check(expected.at("count")==count,label+" marker AX count differs");
      std::set<std::uint32_t> defined;
      for(const auto &field:expected.at("defined_bytes")){
        const auto at=destination+field[0].get<std::uint32_t>();
        defined.insert(at);
        check(std::to_integer<unsigned>(memory.data.at(at))==field[1],label+" defined marker output differs");
      }
      check(touched==defined,label+" marker wrote an undefined byte or missed a field");
      return count;
    };
    halo2::NativeLiftScalarConsumerScopeTu15 scope''')
    return base.replace(source, '    const auto& expected_stores=row.at("semantic_writes");',
                        '    check(marker_index==row.at("marker_deliveries").size(),label+" missing native marker requests");\n'
                        '    const auto& expected_stores=row.at("semantic_writes");')


def main():
    source = inspect.getsource(base.main)
    source = base.replace(source, 'lift-velocity-parent.json', 'lift-marker-velocity-parent.json')
    source = base.replace(source, 'generate_native_lift_velocity_parent_fixture_tu15.py',
                          'generate_native_lift_marker_velocity_fixture_tu15.py')
    env = dict(base.__dict__, PARENT=PARENT, driver=driver, __doc__=__doc__, __file__=__file__)
    exec(source, env)
    env['main']()


if __name__ == '__main__':
    main()
