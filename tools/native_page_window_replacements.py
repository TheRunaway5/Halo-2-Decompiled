"""Authenticate the complete13FE00 page-window policy before native binding."""
import hashlib
import re

BODY_PIN='c2b51e3cce51c2ca58ce1d90a8e9dcd9d79ede0eaf475212228bdae3a4cb0aad'
SPECIALIZED_BODY_PIN='d172314b06fb8e0d43b3dc6caec2ca0333fa28b6bd705cbb2ee9c90f39a6f9c7'
INCLUDE='#include "physics/runtime/native_tu15_page_window_abi.hpp"\n'
PREFIX='    if(halo2::native_page_window_detail::run(s))return;\n'


def body(source):
    return re.search(r'static void run_13fe00\(WorldState &s\) \{.*?\n\}',source,re.S)


def canonical_body(value):
    start=value.index('{\n')+2
    return value[:start]+value[start+len(PREFIX):]if value[start:].startswith(PREFIX)else value


def installed_entries(source):
    current=body(source)
    if not current or current[0]==canonical_body(current[0]):return[]
    if hashlib.sha256(canonical_body(current[0]).encode()).hexdigest()not in(BODY_PIN,SPECIALIZED_BODY_PIN):
        raise ValueError('Bound13FE00 page-window body drift')
    return['13fe00']


def install_native_page_window(source):
    current=body(source)
    if not current:return source
    original=canonical_body(current[0])
    if hashlib.sha256(original.encode()).hexdigest()!=BODY_PIN:raise ValueError('Complete13FE00 page-window body drift')
    if current[0]==original:
        start=original.index('{\n')+2;source=source[:current.start()]+original[:start]+PREFIX+original[start:]+source[current.end():]
    if INCLUDE not in source:
        anchor='#include "physics/runtime/physics_world_runtime_internal.hpp"\n'
        if source.count(anchor)!=1:raise ValueError('Missing13FE00 include seam')
        source=source.replace(anchor,anchor+INCLUDE,1)
    return source
