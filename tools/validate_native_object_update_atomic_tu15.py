#!/usr/bin/env python3
"""Physical BE4A0/B37A0 composition with B3858 atomic rejection restored."""
import inspect
from pathlib import Path

import validate_native_object_update_prediction_tu15 as graph
from validate_native_object_atomic_byte_tu15 import STAGE as BYTE_STAGE

STAGE = graph.ROOT / 'analysis/physics/native-object-update-stage-v5-rmw-20261103/staged'


def driver():
    text = '#include "halo2/native_tu15_byte_update.hpp"\n' + graph.driver()
    marker = 'struct Memory:SourcePhysicsContactMemory {'
    assert text.count(marker) == 1
    text = text.replace(marker, 'struct Memory:SourcePhysicsContactMemory,NativeTu15ByteUpdateStorage {')
    marker = '  void write(std::uint32_t p,std::span<const std::byte>in)override'
    assert text.count(marker) == 1
    text = text.replace(marker, '''  NativeTu15ByteUpdate update_byte(std::uint32_t p,std::uint8_t and_mask,std::uint8_t or_mask)override{
    const auto i=at(p,1);
    if(!fault.is_null()&&fault.value("write",false)&&p==fault.at("address")&&fault.at("bytes")==1u&&
       ++fault_count==fault.at("occurrence"))throw Fault{};
    std::array<std::byte,1>logical{};read(p,logical);
    return native_tu15_update_byte_value(banks.at(i->first)[p-i->first],and_mask,or_mask);
  }
''' + marker)
    return text


def main():
    text = inspect.getsource(graph.main)
    marker = "files=[*STAGE.rglob('*.cpp'),*STAGE.rglob('*.hpp'),"
    assert text.count(marker) == 1
    text = text.replace(marker, marker + "BYTE_STAGE/'include/halo2/native_tu15_byte_update.hpp',BYTE_STAGE/'src/physics/native_tu15_byte_update.cpp',ROOT/'tools/validate_native_object_update_prediction_tu15.py',ROOT/'tools/validate_native_object_atomic_byte_tu15.py',")
    text = text.replace("for h in STAGE.rglob('*.hpp'):", "for h in [*STAGE.rglob('*.hpp'),BYTE_STAGE/'include/halo2/native_tu15_byte_update.hpp']:")
    text = text.replace('native_tu15_object_prediction.cpp native_tu15_object_publication.cpp', 'native_tu15_object_prediction.cpp native_tu15_byte_update.cpp native_tu15_object_publication.cpp')
    start = text.index("     if kind=='writes' and any(e['instruction']==event['instruction'] for e in sample['reads']):")
    end = text.index('     seen.add((event[', start)
    text = text[:start] + text[end:]
    text = text.replace("  fs=scratch/'faults';", "  assert sum(r['name']=='fault_writes_b3858' for r in faults)==1\n  fs=scratch/'faults';")
    text = text.replace('excluded_atomic_rmw_preinstruction_write_faults=excluded_atomic_write_faults,', 'atomic_b3858_preinstruction_write_fault_tested=True,')
    text = text.replace("'atomicRMWpreinstructionwriteFaultService',", '')
    ns = dict(graph.__dict__, STAGE=STAGE, BYTE_STAGE=BYTE_STAGE, driver=driver,
              __file__=__file__, __doc__=__doc__)
    exec(text, ns)
    return ns['main']()


if __name__ == '__main__':
    main()
