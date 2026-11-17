#!/usr/bin/env python3
"""Install the native 44E80/2E79A0/2E7C40 projection-query bridges in a generated unit.

The bridges sit at the entry of 44E80 and of the two sole-caller wrappers
(call sites 4520E and 456AB); the translated bodies stay below them as the
HALO2_REFERENCE_PROJECTION_QUERY oracle. Idempotent; usable on an existing
generated file:  python3 tools/native_projection_query_replacements.py FILE
"""
import sys
from pathlib import Path

INCLUDE = '#include "rendering/shadows/native_tu15_projection_query.hpp"\n'
CHILD = '[](auto&v,std::uint32_t target,std::uint32_t site,bool tail){dispatch(v,target,site,tail);}'
BRIDGES = {
    0x44e80: f'\n    if(halo2::native_tu15_projection_query::bridge_44e80(s,{CHILD},[](auto&v){{run_44e80(v);}}))return;',
    0x2e79a0: f'\n    if(halo2::native_tu15_projection_query::bridge_2e79a0(s,{CHILD},[](auto&v){{run_2e79a0(v);}}))return;',
    0x2e7c40: f'\n    if(halo2::native_tu15_projection_query::bridge_2e7c40(s,{CHILD},[](auto&v){{run_2e7c40(v);}}))return;',
}


def replace_projection_query(source):
    installed = False
    for entry, prefix in BRIDGES.items():
        opening = f'static void run_{entry:x}(WorldState &s) {{'
        count = source.count(opening)
        if count == 0:
            continue
        if count != 1:
            raise ValueError(f'Expected one complete {entry:X} owner')
        at = source.index(opening) + len(opening)
        if source.count(prefix) > 1:
            raise ValueError(f'Duplicate {entry:X} projection bridge')
        if not source.startswith(prefix, at):
            if prefix in source:
                raise ValueError(f'{entry:X} projection bridge moved from its entry')
            source = source[:at] + prefix + source[at:]
        installed = True
    if not installed:
        return source
    if source.count(INCLUDE) > 1:
        raise ValueError('Duplicate projection-query include')
    return source if INCLUDE in source else INCLUDE + source


if __name__ == '__main__':
    for name in sys.argv[1:]:
        path = Path(name)
        text = path.read_text()
        updated = replace_projection_query(text)
        if updated != text:
            path.write_text(updated)
            print('installed', name)
        else:
            print('unchanged', name)
