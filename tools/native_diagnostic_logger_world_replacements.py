"""Exact World required-owner route for the complete actual2E16D0 leaf."""
import hashlib
import importlib.util
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
_spec=importlib.util.spec_from_file_location('_diagnostic_logger_world_functions',Path(__file__).with_name('native_model_marker_lookup_replacements.py'))
_shared=importlib.util.module_from_spec(_spec);_spec.loader.exec_module(_shared)
function=_shared.function
def digest(text):return hashlib.sha256(text.encode()).hexdigest()
METHOD='required_entry_original'
METHOD_PIN='62a00aafd5df86418d93e827f940fa1040b2f77a8886f56065f7c109bb6ce0f1'
METHOD_WITH_D3950_PIN='35595a0ff81f79e48f7cd6ca6be87691d52906e46b93f89fa4a745c9a0e388ae'
D3950_ROUTE=('        if(owns_d3950_node_progress_call(image,c)) {\n'
             '            source_run_ai_frame_tu15(m,c.target,a,fp,services(),o);return;\n'
             '        }\n')
INCLUDE='#include "runtime/native_tu15_diagnostic_logger_owner.hpp"\n'
ANCHOR='        if(&m!=&bank)throw std::runtime_error("TU1.5 live world bank owner differs");'
PREFIX='\n        if(c.target==0x2e16d0u && native_diagnostic_logger_owner_detail::run(m,c,a,services(),o))return;'
HELPERS={
    'include/halo2/native_tu15_diagnostic_logger_kernel.hpp':'21454edcfe5024e698c059e72ff4354b36fba3e36eb0723b8b244c4adc8ce982',
    'src/runtime/native_tu15_diagnostic_logger.hpp':'6bb8fde793bca59f61ec88f99c3542d2f8b6d344bd3f56fe0817faf31e35862e',
    'src/runtime/native_tu15_diagnostic_logger_owner.hpp':'e4d54a16bde360b713a35eaf43bb3f7b96bf56cb73cce226484483de00429971',
}
def canonical_method(body):
    if body.count(PREFIX)>1:raise ValueError('Duplicate diagnostic getter route')
    if PREFIX not in body:return body
    if body.count(ANCHOR+PREFIX)!=1:raise ValueError('Diagnostic getter is not after the unchanged bank gate')
    return body.replace(PREFIX,'',1)
def authenticate(source):
    for name,pin in HELPERS.items():
        if hashlib.sha256((ROOT/name).read_bytes()).hexdigest()!=pin:raise ValueError('Complete diagnostic getter helper drift '+name)
    if source.count('#include "halo2/physics_live_world_tu15.hpp"')!=1 or source.count('struct PhysicsLiveWorldTu15::Impl {')!=1:
        raise ValueError('Diagnostic getter actual World owner drift')
    body=function(source,METHOD);original=canonical_method(body)
    pin=digest(original)
    if pin==METHOD_WITH_D3950_PIN and original.count(D3950_ROUTE)==1:
        # The independently qualified D3950 route added exactly this sibling
        # after the existing World selectors. Its inverse recovers the full
        # previously authenticated method, including the logger's bank gate.
        pin=digest(original.replace(D3950_ROUTE,'',1))
    if pin!=METHOD_PIN or original.count(ANCHOR)!=1:raise ValueError('Complete diagnostic getter World method drift')
    mask=_shared.NON_CODE.sub(lambda m:' '*len(m[0]),source)
    owner=mask.index('struct PhysicsLiveWorldTu15::Impl {')+len('struct PhysicsLiveWorldTu15::Impl {')
    start=source.index(body)
    if start<owner or 1+mask[owner:start].count('{')-mask[owner:start].count('}')!=1:
        raise ValueError('Diagnostic getter method is not the real direct World member')
    return body,original
def replace_diagnostic_logger_world(source):
    body,original=authenticate(source)
    if body==original:source=source.replace(body,body.replace(ANCHOR,ANCHOR+PREFIX,1),1)
    if source.count(INCLUDE)>1:raise ValueError('Duplicate diagnostic getter World include')
    return source if INCLUDE in source else INCLUDE+source
