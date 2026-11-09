"""Exact typed promotion delegation; original CPU and public owners stay pinned."""
import hashlib
from pathlib import Path
from audit_native_execution_ownership import unique_function
from native_async_queue_data_replacements import replace_public as queue_binding

ROOT=Path(__file__).resolve().parents[1]
ABI_REFERENCE_PIN='eac293b0b04aed62d2cdbff0e0dbf8d823f01fb094c6453065f392030fc46608'
PUBLIC_PIN='57ae5430f2993ea59bfd6f8186e4d57d8b323cf485c8d3b6c2adf54e9b8b6e06'
VALUE_REFERENCE_PIN='14db78ad5f8148a53015eaa78b1b0c2a5ec3dd92b291d6256575e737d43fb03b'
UNLINK_PIN='0e07e6efae1bd690ef13905ba14d804ab87ceaf417d682d3b90f2ee7ba6e0ca8'
ABI_BODY='''#pragma once
#include "platform/native_tu15_async_promotion_abi.hpp"
#include <utility>

namespace halo2::native_async_promote_detail {
template<class State,class Insert>bool promote(State&state,Insert&&insert){
    return native_async_promotion_abi_detail::promote(state,std::forward<Insert>(insert));
}
} // namespace halo2::native_async_promote_detail
'''
INCLUDE='#include "halo2/native_tu15_async_promotion_value.hpp"\n'
PUBLIC_BODY='''bool source_async_reprioritize(SourceAsyncQueue &queue,std::uint32_t job,std::int32_t priority){
    valid(job&255u);
    return native_async_promotion_value_detail::reprioritize(queue,job,priority,[&]{chain(queue,queue.work_head);});
}'''
ORIGINAL_PUBLIC='''bool source_async_reprioritize(SourceAsyncQueue &queue,std::uint32_t job,std::int32_t priority){
    const auto node=job&255u;valid(node);
    if(queue.nodes[node].job!=job)return false;
    //120CE0 unlinks without clearing its existing job identity.
    source_async_unlink(queue,node);queue.nodes[node].job=job;
    queue.nodes[node].priority=priority;source_async_insert(queue,node);return true;
}'''
def digest(text):return hashlib.sha256(text.encode()).hexdigest()
def reference_public():
    source=(ROOT/'tests/reference/native_tu15_async_promotion_value_reference.hpp').read_text()
    if digest(source)!=VALUE_REFERENCE_PIN:raise ValueError('Preserved public promotion reference changed')
    body=unique_function(source,'reprioritize').replace('reprioritize(','source_async_reprioritize(',1).replace('async_queue_value_reference::insert(queue,node);','source_async_insert(queue,node);')
    if digest(body)!=PUBLIC_PIN:raise ValueError('Complete public promotion source reference changed')
    return body
def replace_abi(source):
    if source==ABI_BODY:return source
    if digest(source)!=ABI_REFERENCE_PIN:raise ValueError('Complete original promotion ABI helper changed')
    return ABI_BODY
def replace_public(source):
    if digest(ORIGINAL_PUBLIC)!=PUBLIC_PIN:raise ValueError('Pinned promotion source control changed')
    if digest(unique_function(source,'source_async_unlink'))!=UNLINK_PIN:raise ValueError('Original public unlink owner changed')
    if queue_binding(source)!=source:raise ValueError('Qualified shared queue DATA must be installed before promotion')
    current=unique_function(source,'source_async_reprioritize')
    if current!=PUBLIC_BODY and digest(current)!=PUBLIC_PIN:raise ValueError('Complete public reprioritize owner changed')
    source=source.replace(current,PUBLIC_BODY,1)
    if INCLUDE not in source:source=INCLUDE+source
    return source
def canonical_public(source):
    current=unique_function(source,'source_async_reprioritize')
    if current==PUBLIC_BODY:source=source.replace(current,ORIGINAL_PUBLIC,1)
    elif digest(current)!=PUBLIC_PIN:raise ValueError('Complete public reprioritize owner changed')
    if source.startswith(INCLUDE):source=source[len(INCLUDE):]
    return source
