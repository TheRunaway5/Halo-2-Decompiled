"""Preserve the native request observer around the unchanged damage voice CALL."""

INCLUDE = '''// BEGIN native damage voice request include
#include "halo2/source_damage_voice_request_tu15.hpp"
// END native damage voice request include
'''
HOOK = '''
// BEGIN native damage voice request publication
source_damage_voice_request_tu15(s.memory,
    s.image==SourcePhysicsContactImage::TitleUpdate15?0xcc93du:0xcb7bdu,
    s.gp[3],s.gp[5],s.gp[6]);
// END native damage voice request publication
'''


def strip_native_damage_voice_request(source):
    for part in (INCLUDE, HOOK):
        if source.count(part) > 1:
            raise ValueError("Duplicate native damage voice observer")
        source = source.replace(part, "")
    if "// BEGIN native damage voice request" in source:
        raise ValueError("Changed native damage voice observer; retain and re-audit")
    return source


def install_native_damage_voice_request(source):
    source = strip_native_damage_voice_request(source)
    start = source.index("Lcb77f: ")
    end = source.index("Lcb7c5: ", start)
    block = source[start:end]
    # This observer follows the actual return-address push. The source gate,
    # CALL frame, callee, original PC observations and stores remain intact.
    call = ";run_115fa0(s);"
    if block.count(call) != 1:
        raise ValueError("Original CB7BD damage voice caller changed")
    block = block.replace(call, ";" + HOOK + "run_115fa0(s);")
    source = source[:start] + block + source[end:]
    include = '#include "halo2/source_damage_delivery_runtime.hpp"\n'
    if source.count(include) != 1:
        raise ValueError("Damage delivery header owner changed")
    return source.replace(include, include + INCLUDE)
