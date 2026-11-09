"""Authenticate complete1E3150 node resolution and its real lifecycle owner."""
import hashlib,re
import importlib.util
from pathlib import Path

def _sibling(filename):
    spec=importlib.util.spec_from_file_location('node_descriptor_'+Path(filename).stem,Path(__file__).with_name(filename))
    helper=importlib.util.module_from_spec(spec);spec.loader.exec_module(helper)
    return helper

require_selected_identity=_sibling('native_selected_operand_binding.py').require_selected_identity
_lifecycle=_sibling('native_node_resource_lifecycle_replacements.py')
replace_node_resource_lifecycle,RESOLVER_PINS,CHILD_INCLUDE=_lifecycle.replace_node_resource_lifecycle,_lifecycle.RESOLVER_PINS,_lifecycle.INCLUDE

BODY_PINS=(
    '2cf839a496fd2c54ea06fa5b8f6ff3e035971e0ae8e8e50e74272aa294f2b63e',
    '0cf5646330629c959c1c77c31474736d34dfc12b67d3bee78cdb30b0ddca7ba4',
)
PREFIX='\n    if(halo2::native_node_descriptor_lookup_detail::bridge(s,[](auto&state,std::uint32_t target,std::uint32_t site,bool tail){dispatch(state,target,site,tail);}))return;'
INCLUDE='#include "physics/runtime/native_tu15_node_descriptor_lookup.hpp"\n'
def canonical_body(body):
    opening=body.index('{')+1
    return body[:opening]+body[opening+len(PREFIX):]if body[opening:].startswith(PREFIX)else body
def replace_node_descriptor_lookup(source):
    found=list(re.finditer(r'static void run_1e3150\(WorldState &s\) \{.*?\n\}',source,re.S))
    if not found:return source
    if len(found)!=1:raise ValueError('Expected one complete1E3150 node descriptor owner')
    current=found[0][0];original=canonical_body(current)
    if hashlib.sha256(original.encode()).hexdigest()not in BODY_PINS:raise ValueError('Complete1E3150 node descriptor body drift')
    require_selected_identity(source,original,RESOLVER_PINS)
    if not re.search(r'static void run_1e5c80\(WorldState &s\) \{',source):raise ValueError('Native node descriptor requires its complete lifecycle owner')
    if replace_node_resource_lifecycle(source)!=source:raise ValueError('Native node descriptor requires its authenticated native lifecycle owner')
    if current==original:
        opening=current.index('{')+1;changed=current[:opening]+PREFIX+current[opening:]
        source=source[:found[0].start()]+changed+source[found[0].end():]
    if INCLUDE not in source:
        if source.count(CHILD_INCLUDE)!=1:raise ValueError('Expected one qualified lifecycle include')
        source=source.replace(CHILD_INCLUDE,CHILD_INCLUDE+INCLUDE,1)
    return source
