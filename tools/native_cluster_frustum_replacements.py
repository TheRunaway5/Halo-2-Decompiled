"""Authenticate complete167EF0 and its public range-aggregation owner."""
import hashlib,re
BODY_PIN='dd41d7611ab271a7e94ffa248c759b2917facd041be177cc61de9b43607023ce'
RESOLVER_PIN='892cf508106b0c182d59e4dba45804e434a1833003cfb3c322f9a8875c54d7fb'
DISPATCH_PIN='c0e112fb90d7ea32e73e6b3b37d333b6a375456791c5f2eaf9a08906d613f6a9'
PREFIX='\n    if(halo2::native_cluster_frustum_abi_detail::bridge_with(s,[](auto&v,auto&){run_184690(v);}))return;'
INCLUDE='#include "rendering/native_tu15_cluster_frustum_abi.hpp"\n'
BODY=re.compile(r'static void run_167ef0\(WorldState &s\) \{.*?\n\}',re.S)
def digest(text):return hashlib.sha256(text.encode()).hexdigest()
def canonical_body(body):
    start=body.index('{')+1
    return body[:start]+body[start+len(PREFIX):]if body[start:].startswith(PREFIX)else body
def authenticate(source):
    found=list(BODY.finditer(source))
    if len(found)!=1:raise ValueError('Expected one complete167EF0 owner')
    match=found[0];body=canonical_body(match[0])
    if digest(body)!=BODY_PIN:raise ValueError('Complete167EF0 cluster frustum body drift')
    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)
    if not resolver or digest(resolver[0])!=RESOLVER_PIN:raise ValueError('Complete167EF0 selected resolver drift')
    dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)
    if not dispatcher or digest(dispatcher[0])!=DISPATCH_PIN:raise ValueError('Complete167EF0 dispatcher drift')
    from native_sphere_frustum_replacements import replace_sphere_frustum
    replace_sphere_frustum(source) # authenticates the actual complete child
    return match,body
def replace_cluster_frustum(source):
    if not BODY.search(source):return source
    match,body=authenticate(source)
    if match[0]==body:
        start=body.index('{')+1;source=source[:match.start()]+body[:start]+PREFIX+body[start:]+source[match.end():]
    if source.count(INCLUDE)>1:raise ValueError('Duplicate cluster frustum include')
    if INCLUDE not in source:source=INCLUDE+source
    return source

PUBLIC_PIN='62dbbf88419512d01f617fca00e35175d98e7880c1b70eb2e1a011d7b739e491'
PUBLIC_INCLUDE='#include "halo2/native_tu15_cluster_frustum_value.hpp"\n'
PUBLIC_BODY='''SourceClusterFrustumResult source_cluster_frustum_visibility(
    const SourceClusterFrustumSet& set,std::size_t section_index,Vec3 center,float radius) {
    return native_cluster_frustum_value_detail::visible(set,section_index,center,radius);
}'''
def replace_world_visibility(source):
    from audit_native_execution_ownership import unique_function
    from native_sphere_frustum_replacements import PUBLIC_BODY as sphere_body
    if unique_function(source,'source_sphere_frustum_visibility')!=sphere_body:raise ValueError('Independent qualified sphere facade drift')
    current=unique_function(source,'source_cluster_frustum_visibility')
    if current!=PUBLIC_BODY:
        if digest(current)!=PUBLIC_PIN:raise ValueError('Complete public cluster visibility body drift')
        source=source.replace(current,PUBLIC_BODY,1)
    if source.count(PUBLIC_INCLUDE)>1:raise ValueError('Duplicate cluster frustum value include')
    if PUBLIC_INCLUDE not in source:source=source.replace('#include "halo2/source_visibility.hpp"\n','#include "halo2/source_visibility.hpp"\n'+PUBLIC_INCLUDE,1)
    if PUBLIC_INCLUDE not in source:raise ValueError('Missing source visibility include anchor')
    return source
