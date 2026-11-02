#!/usr/bin/env python3
"""Compose native2815F0/1D7260/1D8B20 with unchanged physical TU1.5.

Only1D8B90's distinct component-frame branch and actual allocation results
remain true child boundaries. The original readonly zero vector is pinned.
All generated inputs/ELFs/compiler trees use run_test.py scratch.
"""
import inspect
import struct
from pathlib import Path
import validate_native_body_deactivation_listener_tu15 as parent


def once(text, old, new):
    assert text.count(old) == 1, old[:100]
    return text.replace(old, new)


def cases():
    image = parent.Xbe(parent.NEW_DEFAULT)
    zero_pointer = struct.unpack('<I', image.read(0x474d04, 4))[0]
    assert zero_pointer == 0x44abec and image.read(zero_pointer, 12) == bytes(12)
    rows = parent.cases()
    for ordinal, row in enumerate(rows):
        banks = {bank['address']: bytearray.fromhex(bank['before']) for bank in row['banks']}
        banks[parent.BASE].extend(bytes(0x10000 - len(banks[parent.BASE])))

        def put(address, value, width=4):
            first = next(p for p, data in banks.items() if p <= address and address + width <= p + len(data))
            banks[first][address-first:address-first+width] = (value & ((1 << (width * 8))-1)).to_bytes(width, 'little')

        banks[0x474d04] = bytearray(image.read(0x474d04, 4))
        banks[zero_pointer] = bytearray(image.read(zero_pointer, 12))
        for component in range(4):
            entries = parent.BASE + 0x5000 + component * 0x400
            put(parent.COMPONENTS + component * 160 + 0x70, entries)
            for index in range(4):
                body = parent.BASE + 0x7000 + component * 0x800 + index * 0x100
                motion = parent.BASE + 0xa000 + component * 0x800 + index * 0x100
                put(entries + index * 96 + 0x40, body)
                put(body + 0x3c, motion)
                put(body + 0x40, (0, 1, 0x80)[(ordinal + index) % 3], 1)
                # Noncanonical values exercise MOVSS/word copies as bits,
                # without supplying a physics result or evaluating arithmetic.
                for offset in (0x40, 0x44, 0x48, 0x50, 0x54, 0x58, 0xb0, 0xb4, 0xb8):
                    value = (0x3f800001 + ordinal * 131 + index * 17 + offset) & 0xffffffff
                    if ordinal % 5 == 0:
                        value = (0x7f812345, 0xffc54321, 0x80000000)[index % 3]
                    put(motion + offset, value)
        row['banks'] = [dict(address=p, before=data.hex()) for p, data in sorted(banks.items())]
    return rows


def main():
    namespace = dict(parent.__dict__)
    namespace['__file__'] = __file__
    namespace['OWNERS'] = (0x2815f0, 0x2e3250, 0x1d7260, 0x1d8b20)
    meta = inspect.getsource(parent.metadata)
    meta = once(meta, 'assert counts == [63, 46]', 'assert counts == [63, 46, 75, 31]')
    meta = once(meta, 'boundaries=[0x149da0, 0x1d7260]', 'boundaries=[0x149da0, 0x1d8b90]')
    exec(meta, namespace)
    namespace['cases'] = cases
    source = inspect.getsource(parent.main)
    source = source.replace('native-body-deactivation-listener', 'native-body-deactivation-motion')
    source = source.replace('unchanged-tu15-native-body-deactivation-listener-v1', 'unchanged-tu15-native-body-deactivation-motion-v2')
    # The previous replace changes the backend stem first.
    source = source.replace('unchanged-tu15-native-body-deactivation-motion-v1', 'unchanged-tu15-native-body-deactivation-motion-v2')
    source = source.replace('tests/native_tu15_body_deactivation_listener_tests.cpp', 'tests/native_tu15_body_deactivation_motion_tests.cpp')
    source = once(source,
        'assert owner == 0x1d7260\n            return dict(owner=owner, component=arguments[1], index=gp[0])',
        'assert owner == 0x1d8b90\n            return dict(owner=owner, component=gp[1], index=gp[0])')
    source = source.replace('("activation_payload_filter", "activation_services", "array_growth")',
                            '("activation_payload_filter", "activation_services", "array_growth", "component_body_motion_snapshot")')
    source = once(source,
        'ROOT / "include/halo2/native_tu15_body_deactivation_binding.hpp",',
        'ROOT / "include/halo2/native_tu15_body_deactivation_motion_binding.hpp",\n             ROOT / "tools/validate_native_body_deactivation_listener_tu15.py",')
    source = once(source,
        'activation_payload_filter.cpp activation_services.cpp array_growth.cpp)',
        'activation_payload_filter.cpp activation_services.cpp array_growth.cpp component_body_motion_snapshot.cpp)')
    source = once(source,
        '("wrong_queue", \'data+insertion*4u\', \'data+insertion*8u\')):',
        '("wrong_queue", \'data+insertion*4u\', \'data+insertion*8u\'),\n            ("wrong_motion_index", \'services.publish_component_body_motion(component,index,fp)\', \'services.publish_component_body_motion(component,index+1u,fp)\')):')
    # The independent observer stops at the first occurrence of a source PC.
    # Repeated iterations still compare every final byte/read/store; only
    # first-occurrence committed observer-fault prefixes are declared here.
    source = once(source,
        '            for n, event in enumerate(seed["writes"]):\n                row = copy.deepcopy(seed)',
        '            observer_sites = set()\n            for n, event in enumerate(seed["writes"]):\n                if event["instruction"] in observer_sites:\n                    continue\n                observer_sites.add(event["instruction"])\n                row = copy.deepcopy(seed)')
    source = once(source,
        'remaining=["Native returned1D7260 component motion closure", "Full allocation return composition",',
        'remaining=["Native returned1D8B90 component-frame branch", "Full allocation return composition", "Repeated-site observer-fault occurrences beyond first",')
    exec(source, namespace)
    return namespace['main']()


if __name__ == '__main__':
    raise SystemExit(main())
