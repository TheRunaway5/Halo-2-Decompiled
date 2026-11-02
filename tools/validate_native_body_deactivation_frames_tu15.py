#!/usr/bin/env python3
"""Native sleep notification through the existing native component-frame kernel.

Requalifies the actual1D8B90/2E4650/2E44D0 composition, not a frame afterimage.
All generated banks/ELFs/builds stay in run_test.py scratch.
"""
import inspect
from pathlib import Path
import validate_native_body_deactivation_motion_tu15 as previous
from validate_native_body_deactivation_motion_tu15 import once


def cases():
    rows = previous.cases()
    image = previous.parent.Xbe(previous.parent.NEW_DEFAULT)
    for ordinal, row in enumerate(rows):
        banks = {bank['address']: bytearray.fromhex(bank['before']) for bank in row['banks']}
        raw = banks[previous.parent.BASE]
        for component in range(4):
            base = previous.parent.COMPONENTS + component * 160 + 0x30 - previous.parent.BASE
            for column in range(4):
                for lane in range(4):
                    value = 0x3f800000 if column == lane and column < 3 else 0
                    if column == 3 and lane < 3:
                        value = (0x3f000000, 0xbf800000, 0x40000000)[lane]
                    raw[base+column*16+lane*4:base+column*16+lane*4+4] = value.to_bytes(4, 'little')
        banks[0x46a0d0] = bytearray(image.read(0x46a0d0, 4))
        row['banks'] = [dict(address=p, before=data.hex()) for p, data in sorted(banks.items())]
    return rows


def main():
    source = inspect.getsource(previous.main)
    source = once(source, "namespace['OWNERS'] = (0x2815f0, 0x2e3250, 0x1d7260, 0x1d8b20)",
                  "namespace['OWNERS'] = (0x2815f0, 0x2e3250, 0x1d7260, 0x1d8b20, 0x1d8b90, 0x2e4650, 0x2e44d0)")
    source = once(source, "'assert counts == [63, 46, 75, 31]'", "'assert counts == [63, 46, 75, 31, 55, 20, 40]'")
    source = once(source, "'boundaries=[0x149da0, 0x1d8b90]'", "'boundaries=[0x149da0]'")
    insertion = '''source = source.replace('unchanged-tu15-native-body-deactivation-motion-v2','unchanged-tu15-native-body-deactivation-frames-v3')
    source = source.replace('tests/native_tu15_body_deactivation_motion_tests.cpp','tests/native_tu15_body_deactivation_frame_tests.cpp')
    source = source.replace('include/halo2/native_tu15_body_deactivation_motion_binding.hpp','include/halo2/native_tu15_body_deactivation_frame_binding.hpp')
    source = source.replace('"array_growth", "component_body_motion_snapshot")','"array_growth", "component_body_motion_snapshot", "body_constraint_update", "geometry")')
    source = source.replace('component_body_motion_snapshot.cpp)','component_body_motion_snapshot.cpp body_constraint_update.cpp geometry.cpp data_array_allocate.cpp)')
    source = source.replace('        variants = {}','        shutil.copyfile(ROOT / "src/core/native_tu15_data_array_allocate.cpp",project / "data_array_allocate.cpp")\\n        variants = {}')
    source = source.replace('files = [ROOT /','files = [ROOT / "tools/validate_native_body_deactivation_motion_tu15.py",ROOT / "src/core/native_tu15_data_array_allocate.cpp",ROOT / "include/halo2/native_tu15_data_array_allocate.hpp",ROOT /',1)
    source = source.replace('"Native returned1D8B90 component-frame branch", ','')
    exec(source, namespace)'''
    source = once(source, '    exec(source, namespace)', '    ' + insertion)
    namespace = dict(previous.__dict__)
    namespace.update(__file__=__file__, cases=cases)
    exec(source, namespace)
    return namespace['main']()


if __name__ == '__main__':
    raise SystemExit(main())
