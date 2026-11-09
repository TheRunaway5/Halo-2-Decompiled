"""Exact owner authentication for async ABI and shared public DATA delegation."""
import hashlib
from pathlib import Path
from audit_native_execution_ownership import unique_function
ROOT=Path(__file__).resolve().parents[1]
ABI_REFERENCE_PIN='c8e83a44304c3cb0f51ccde489b3c37e15d3b0fc7cda7137e7869ab93cc81e0e'
VALUE_REFERENCE_PIN='dbef142c4fb8a0c7ec059fb2896f4902e26a9785ce850ac3770244cfa7c6a6ff'
PUBLIC_PINS={'source_async_before':'68a478758347744c6c1105ca4dda7f94552337f0884a506a91a6dd96766a5961',
    'source_async_insert':'40c9d2b5d885f163eadcd545379cd4dcd376753cdd797bf4b3d5e6ab5ba02333'}
VALIDATION_PINS={'valid':'524afb39bf75037c6e68695d7116ea29252c974e8afb9a4c7bfa0a36e71e4eb2',
    'chain':'6115278ac3487ec21da93ed4119dd7eada7933abdf328b5ad4d499ad1bb8e816',
    'unlinked':'52217479238e6728ca05bc48d24b63627d3a32f137ebfc39819d625fbf45dabc'}
PUBLIC_INCLUDE='#include "halo2/native_tu15_async_queue_value.hpp"\n'
PUBLIC_BODIES={
    'source_async_before':'''bool source_async_before(const SourceAsyncNode &incoming,const SourceAsyncNode &other,
    SourceAsyncInsertState &state){
    return native_async_queue_value_detail::before(incoming,other,state);
}''',
    'source_async_insert':'''void source_async_insert(SourceAsyncQueue &queue,unsigned node){
    unlinked(queue,node);
    native_async_queue_value_detail::insert(queue,node);
}'''}
ABI_BODY='''#pragma once
#include "physics/runtime/native_tu15_matrix_leaves.hpp"
#include "platform/native_tu15_async_queue_abi.hpp"
#include <utility>

namespace halo2::native_async_order_detail {
inline bool reference(){return native_async_queue_abi_detail::reference();}
template<class State>bool admitted(State&s){return native_async_queue_abi_detail::admitted(s);}
template<class State>bool compare(State&s,std::uint32_t site_base){
    return native_async_queue_abi_detail::compare(s,site_base);
}
template<class State,class Compare>bool insert(State&s,std::uint32_t site_base,Compare&&compare){
    return native_async_queue_abi_detail::insert(s,site_base,std::forward<Compare>(compare));
}
} // namespace halo2::native_async_order_detail
'''
def digest(text):return hashlib.sha256(text.encode()).hexdigest()
def reference_public():
    text=(ROOT/'tests/reference/native_tu15_async_queue_value_reference.hpp').read_text()
    if digest(text)!=VALUE_REFERENCE_PIN:raise ValueError('Preserved async public reference changed')
    before=unique_function(text,'before').replace('before(','source_async_before(',1)
    insertion=unique_function(text,'insert').replace('insert(','source_async_insert(',1).replace('before(','source_async_before(')
    result={'source_async_before':before,'source_async_insert':insertion}
    if any(digest(body)!=PUBLIC_PINS[name]for name,body in result.items()):raise ValueError('Complete public async source reference changed')
    return result
def replace_public(source):
    for name,pin in VALIDATION_PINS.items():
        if digest(unique_function(source,name))!=pin:raise ValueError('Async actual queue validation owner changed '+name)
    for name in PUBLIC_PINS:
        current=unique_function(source,name)
        if current!=PUBLIC_BODIES[name]and digest(current)!=PUBLIC_PINS[name]:raise ValueError('Complete public async owner changed '+name)
        source=source.replace(current,PUBLIC_BODIES[name],1)
    if PUBLIC_INCLUDE not in source:source=PUBLIC_INCLUDE+source
    return source
def canonical_public(source):
    originals=reference_public()
    for name,body in PUBLIC_BODIES.items():
        current=unique_function(source,name)
        if current==body:source=source.replace(current,originals[name],1)
        elif digest(current)!=PUBLIC_PINS[name]:raise ValueError('Complete async public owner drift '+name)
    if source.startswith(PUBLIC_INCLUDE):source=source[len(PUBLIC_INCLUDE):]
    return source
def replace_abi(source):
    if source==ABI_BODY:return source
    if digest(source)!=ABI_REFERENCE_PIN:raise ValueError('Complete original async ABI helper changed')
    return ABI_BODY
