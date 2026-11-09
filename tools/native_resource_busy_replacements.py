"""Authenticate complete .text/D3D busy-query bodies before native binding."""
import hashlib
import importlib.util
from pathlib import Path
import re

PINS={
    '12fcb0':('7be8374150d17cb592e03074ccdcff4797511a24359bde3f0923ec49c28bf535',
        'ba8cb04b484daf673cec4318338a92c60941fda29c4a1bc46294e480ffe0333e',
        '70937fa42338b49117d717cfa93e231ab79060e50cd6c99dd4a02f09864eb918'),
    '4057f0':('ed41100ceb15f8ada64661a57e88be9e6db9c948b3e47d63d0c54edcabe32e4c',
        '4d688da37c5c4aa63f16e350d80eb6b353a34ab969ac705429fcf2babff358db')}
RESOLVER_PINS=('cc3118fc8d131fc3d8beb8e314827a9412e84288f99b72509be63ff966688b61',
    '7b1e3e35e2876b19b074a87b5235526135a7effaaa31c63b2e86450091613d75')
INCLUDE='#include "physics/runtime/native_tu15_resource_busy_abi.hpp"\n'


def prefix(entry):
    if entry=='4057f0':return '    if(halo2::native_resource_busy_detail::query(s))return;\n'
    return '''    if(halo2::native_resource_busy_detail::callback(s,
        [](auto &v,std::uint32_t owner,std::uint32_t site,bool tail){dispatch(v,owner,site,tail);}))return;
'''


def canonical_body(value,entry):
    start=value.index('{\n')+2;binding=prefix(entry)
    return value[:start]+value[start+len(binding):]if value[start:].startswith(binding)else value


def authenticate(source,value,entry,specialized=False):
    candidate=value
    if specialized:candidate=re.sub(r's\.instruction_literal\((0x[0-9a-f]+u)\);',r'if(s.services.instruction){s.services.instruction(\1);}',candidate)
    if hashlib.sha256(candidate.encode()).hexdigest()not in PINS[entry]:raise ValueError('Complete native busy body drift '+entry)
    spec=importlib.util.spec_from_file_location('resource_busy_selected_binding',Path(__file__).with_name('native_selected_operand_binding.py'))
    helper=importlib.util.module_from_spec(spec);spec.loader.exec_module(helper);helper.require_selected_identity(source,candidate,RESOLVER_PINS)


def installed_entries(source):
    installed=[]
    for entry in PINS:
        match=re.search(r'static void run_'+entry+r'\(WorldState &s\) \{.*?\n\}',source,re.S)
        if match and canonical_body(match[0],entry)!=match[0]:
            authenticate(source,canonical_body(match[0],entry),entry,True);installed.append(entry)
    return sorted(installed)


def install_native_resource_busy(source):
    installed=False
    for entry in PINS:
        match=re.search(r'static void run_'+entry+r'\(WorldState &s\) \{.*?\n\}',source,re.S)
        if not match:continue
        original=canonical_body(match[0],entry);authenticate(source,original,entry)
        if original==match[0]:
            start=original.index('{\n')+2;source=source[:match.start()]+original[:start]+prefix(entry)+original[start:]+source[match.end():]
        installed=True
    if installed and INCLUDE not in source:
        anchor='#include "physics/runtime/physics_world_runtime_internal.hpp"\n'
        if source.count(anchor)!=1:raise ValueError('Missing resource busy include seam')
        source=source.replace(anchor,anchor+INCLUDE,1)
    return source
