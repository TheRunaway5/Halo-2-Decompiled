"""Authenticate complete140310 DATA/ABI bindings in both TU1.5 owners."""
import hashlib
import importlib.util
from pathlib import Path
import re

BODY_PINS=('28fffe9549d5ea4baa6b1f3119b2b9581ca815f07ba6a3e74dfd315e57b3bf67',
           '34626a93e6efc51930ee5940ea1411bc5fa839b2520859e89d9552f68c36c96b',
           'd043342e1662c7a542588f30ba755f2f1b00ac7c8741c86a07b3599bfd6401d0')
SPECIALIZED_PINS=('2657d956c23f0c6491482387ccf7130f024b2fcb37bea4743f1b5a2cf5100b7e',
                  'a5ddcce0833bb4fd387d2d526347f6efec2f9f96abc0a578a3197a42077d938c')
RESOLVER_PINS=('7b1e3e35e2876b19b074a87b5235526135a7effaaa31c63b2e86450091613d75',)
INCLUDE='#include "physics/runtime/native_tu15_page_retire_abi.hpp"\n'
PREFIX='''    if(halo2::native_page_retire_detail::run(s,
        [](auto &v,std::uint32_t owner,std::uint32_t site,bool tail){dispatch(v,owner,site,tail);}))return;
'''


def body(source):
    return re.search(r'static void run_140310\(WorldState &s\) \{.*?\n\}',source,re.S)


def canonical_body(value):
    start=value.index('{\n')+2
    return value[:start]+value[start+len(PREFIX):]if value[start:].startswith(PREFIX)else value


def selected_identity(source,value):
    if 'selected('not in value:return
    spec=importlib.util.spec_from_file_location('page_retire_selected_binding',Path(__file__).with_name('native_selected_operand_binding.py'))
    helper=importlib.util.module_from_spec(spec);spec.loader.exec_module(helper)
    helper.require_selected_identity(source,value,RESOLVER_PINS)


def installed_entries(source):
    current=body(source)
    if not current or current[0]==canonical_body(current[0]):return[]
    original=canonical_body(current[0])
    if hashlib.sha256(original.encode()).hexdigest()not in(*BODY_PINS,*SPECIALIZED_PINS):raise ValueError('Bound140310 page-retire body drift')
    selected_identity(source,original);return['140310']


def install_native_page_retire(source):
    current=body(source)
    if not current:return source
    original=canonical_body(current[0])
    if hashlib.sha256(original.encode()).hexdigest()not in BODY_PINS:raise ValueError('Complete140310 page-retire body drift')
    selected_identity(source,original)
    if current[0]==original:
        start=original.index('{\n')+2;source=source[:current.start()]+original[:start]+PREFIX+original[start:]+source[current.end():]
    if INCLUDE not in source:
        anchor='#include "physics/runtime/physics_world_runtime_internal.hpp"\n'
        if source.count(anchor)!=1:raise ValueError('Missing140310 include seam')
        source=source.replace(anchor,anchor+INCLUDE,1)
    return source
