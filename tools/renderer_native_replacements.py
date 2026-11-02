"""Keep generated renderer callers bound to semantic native kernels."""
import re

RENDER_OWNERSHIP_SITES = {
    0xbcfb0: 'bcfb0 bcfb3 bcfb6 bcfb8 bcfbd bcfc0 bcfc2 bcfc8 bcfcb bcfcf bcfd2 bcfd5 bcfd7',
    0xbac70: 'bac70 bac76 bac79 bac7f bac82 bac86 bac88 bac8e bac94 bac95 bac98 bac9b bac9f baca2 baca5 baca7 bacaa bacb0 bacb2 bacb5 bacbb bacbf bacc0',
    0x3e9b0: '3e9b0 3e9b1 3e9b3 3e9b6 3e9b8 3e9b9 3e9bf 3e9c2 3e9c4 3e9c6 3e9cb 3e9cd 3e9cf 3e9d1 3e9d6 3e9dc 3e9df 3e9e4 3e9e7 3e9eb 3e9ed 3e9f0 3e9f2 3e9f5 3e9fb 3e9ff 3ea05 3ea07 3ea08 3ea0a 3ea0b 3ea0c 3ea0d 3ea0f 3ea10 3ea11 3ea13 3ea14',
    0x158290: '158290 158293 158296 15829b 15829e 1582a4 1582a9 1582ab 1582b0 1582b7 1582b8 1582bb 1582bd 1582bf 1582c0 1582c6 1582c8 1582cd 1582ce 1582d4 1582d6 1582dc 1582de 1582e3 1582e5 1582e7 1582ec 1582ed 1582ef',
}


def _check_retired_body(source, body, expected_sites, replacement):
    if body == replacement:
        return  # The shared generator and a family generator may both bind it.
    sites = tuple(int(value, 16) for value in
                  re.findall(r's\.services\.instruction\(0x([0-9a-f]+)u\)', body))
    if sites != expected_sites:
        raise ValueError('Original renderer instruction sequence changed before native replacement')
    remainder = source.replace(body, '', 1)
    for label in re.findall(r'^(L[0-9a-f]+):', body, re.M):
        if re.search(r'\bgoto\s+' + label + r'\s*;', remainder):
            raise ValueError('Retiring the renderer body would leave an external label reference')


def replace_lens_query_slot_body(source):
    include = '#include "rendering/effects/native_tu15_lens_query_slot_bridge.hpp"\n'
    if include not in source:
        source = include + source
    pattern = r'static void run_20e30\(WorldState &s\) \{.*?\n\}'
    replacement = ('static void run_20e30(WorldState &s) {\n'
                   '    native_tu15_lens_query_slot_bridge(s);\n'
                   '    return;\n'
                   'throw std::runtime_error("Original solver fell outside authenticated CFG");\n}')
    # The production partitioner authenticates bodies with this canonical
    # unreachable trailer. Retain it when the body becomes a native bridge.
    expected_sites = (0x20e30, 0x20e31, 0x20e33, 0x20e35, 0x20e38, 0x20e3d,
                      0x20e3f, 0x20e42, 0x20e48, 0x20e4b, 0x20e4d, 0x20e54,
                      0x20e59, 0x20e60, 0x20e62, 0x20e65, 0x20e66, 0x20e68,
                      0x20e6a, 0x20e6c, 0x20e6e, 0x20e71, 0x20e74, 0x20e77,
                      0x20e7a, 0x20e7d, 0x20e80, 0x20e83, 0x20e86, 0x20e8b,
                      0x20e8e, 0x20e8f)
    for match in re.finditer(pattern, source, flags=re.S):
        _check_retired_body(source, match.group(), expected_sites, replacement)
    source, count = re.subn(pattern, lambda _: replacement, source, flags=re.S)
    if count != 1:
        raise ValueError('Expected exactly one original 20E30 body to retire')
    return source


def replace_model_view_slot_reset(source):
    include = '#include "rendering/models/native_tu15_model_view_slots_bridge.hpp"\n'
    if include not in source:
        source = include + source
    pattern = (r'if\(s.services.instruction\)\{s.services.instruction\(0x22836u\);\}.*?'
               r'(?=if\(s.services.instruction\)\{s.services.instruction\(0x2286eu\);\})')
    replacement = '    native_tu15_model_view_slots_bridge(s);\n'
    expected_sites = (0x22836, 0x2283b, 0x2283e, 0x22840, 0x22843, 0x22846,
                      0x22849, 0x2284b, 0x2284e, 0x22853, 0x22853, 0x22855, 0x22858,
                      0x2285a, 0x2285d, 0x22860, 0x22866, 0x22869, 0x2286c)
    for match in re.finditer(pattern, source, flags=re.S):
        _check_retired_body(source, match.group(), expected_sites, replacement)
    source, count = re.subn(pattern, lambda _: replacement,
                            source, flags=re.S)
    if count != 1:
        raise ValueError('Expected exactly one original 22830 slot reset to retire')
    return source


def replace_render_ownership_bodies(source, entries):
    owned = set(entries) & RENDER_OWNERSHIP_SITES.keys()
    if not owned:
        return source
    include = '#include "rendering/models/native_tu15_render_object_ownership_bridge.hpp"\n'
    if include not in source:
        source = include + source
    for entry in sorted(owned):
        pattern = rf'static void run_{entry:x}\(WorldState &s\) \{{.*?\n\}}'
        replacement = (f'static void run_{entry:x}(WorldState &s) {{\n'
                       f'    native_tu15_render_object_ownership_bridge(s,0x{entry:x}u);\n'
                       '    return;\n'
                       'throw std::runtime_error("Original solver fell outside authenticated CFG");\n}')
        expected = tuple(int(site, 16) for site in RENDER_OWNERSHIP_SITES[entry].split())
        for match in re.finditer(pattern, source, flags=re.S):
            _check_retired_body(source, match.group(), expected, replacement)
        source, count = re.subn(pattern, lambda _: replacement, source, flags=re.S)
        if count != 1:
            raise ValueError(f'Expected exactly one TU1.5 render ownership body {entry:x}')
    return source


def replace_render_root_alias_body(source):
    """Retire BBE70's TU1.5 path while preserving the MapPack11 body."""
    include = '#include "rendering/models/native_tu15_render_object_ownership_bridge.hpp"\n'
    if 'case 0x4e78d0u:return 0x4ecbd0u;' not in source:
        raise ValueError('Render root alias has no authenticated TU1.5 object pool mapping')
    pattern = r'static void run_bbe70\((\w+) &s\) \{.*?\n\}'
    matches = list(re.finditer(pattern, source, flags=re.S))
    if len(matches) != 1:
        raise ValueError('Expected exactly one legacy render root alias')
    body = matches[0].group()
    prefix = ('\n    if(s.image==SourcePhysicsContactImage::TitleUpdate15) {\n'
              '        native_tu15_render_object_ownership_bridge(s,0xbcfb0u);return;\n'
              '    }')
    start = body.index('{') + 1
    if body[start:].startswith(prefix):
        return source
    normalized = re.sub(r'world_observation_pc\((0x[0-9a-f]+u),0x[0-9a-f]+u,s\)', r'\1', body)
    expected = tuple(int(site, 16) - 0x1140 for site in RENDER_OWNERSHIP_SITES[0xbcfb0].split())
    _check_retired_body(source.replace(body, normalized, 1), normalized, expected, '')
    source = source.replace(body, body[:start] + prefix + body[start:], 1)
    if include not in source:
        source = include + source
    return source
