"""Authenticate complete16C000 query owners and the shared public DATA view."""
import hashlib,importlib.util,re
from pathlib import Path

def _sibling(filename):
    spec=importlib.util.spec_from_file_location('camera_publication_'+Path(filename).stem,Path(__file__).with_name(filename))
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);return module
require_selected_identity=_sibling('native_selected_operand_binding.py').require_selected_identity
_halfword=_sibling('native_halfword_bit_query_replacements.py')
BODY_PINS={'7568d9b56c4f415710b83c34d67f00f849f604bafeb5a78e0c016909c294b17a','a3dcc61bf80a16d252b77f9dc767571955a7eb0bc668671ba6664c909f4ee49f',
           '378022e00a98cf0ef5ba4ebeda6f980f57f0883cf708d963613fbaa942369952'}
RESOLVER_PINS={'1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4','5eab69b03c4be4556290b5ba3e7765897c12d0064ea5b20267317b365ea4a657'}
DISPATCH_PINS={'5901d2c9c67421f4dabe988f833de38ce4a22dd4cb0778fbf19eb0fd0cb55ce0','1cf61f52187c986d02510a40d28a466d95589b66af5521ade12ae9640085e28b'}
FRESH_LITERAL_CONTEXTS={('b366b77691cb314f6c6d0f8f20e71c1bda7114225c8d7b1f4043ee80e12354b8','974fddd715271a3ccdc93c266782b8f84d13f514bd11653b34af22a1f8be267a')}
# Exact identity-literal projection of canonical AI a3dcc61b..., independently
# byte-equal to the actual fresh generator output. No general normalization.
LITERAL_BODY_PIN='378022e00a98cf0ef5ba4ebeda6f980f57f0883cf708d963613fbaa942369952'
PREPARED_OUTPUT_PINS={'f39679ebf845d21c757d0706adc6372fe700dc62e918c3c3c73250df4ea00c01',
                     '1d98af93c049d0b6def70cb0401365ff543ac102584f14ec4c9ca055184df458'}
FRESH_CASE='case 0xfa5d0u:run_fa5d0(s);return;'
FRESH_PRIMARY_DISPATCH_PIN='aba69140bdfbfe48111ef3727b7bb22a4891a8daa06770aabcb75183386d5849'
BODY_CONTEXTS={
 ('7568d9b56c4f415710b83c34d67f00f849f604bafeb5a78e0c016909c294b17a','1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4','5901d2c9c67421f4dabe988f833de38ce4a22dd4cb0778fbf19eb0fd0cb55ce0'),
 ('7568d9b56c4f415710b83c34d67f00f849f604bafeb5a78e0c016909c294b17a','1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4',FRESH_PRIMARY_DISPATCH_PIN),
 ('a3dcc61bf80a16d252b77f9dc767571955a7eb0bc668671ba6664c909f4ee49f','5eab69b03c4be4556290b5ba3e7765897c12d0064ea5b20267317b365ea4a657','1cf61f52187c986d02510a40d28a466d95589b66af5521ade12ae9640085e28b'),
 (LITERAL_BODY_PIN,'b366b77691cb314f6c6d0f8f20e71c1bda7114225c8d7b1f4043ee80e12354b8','974fddd715271a3ccdc93c266782b8f84d13f514bd11653b34af22a1f8be267a')}
PREFIX='\n    if(halo2::native_camera_instance_query_detail::bridge(s,[](auto&state,std::uint32_t target,std::uint32_t site){dispatch(state,target,site,false);}))return;'
INCLUDE='#include "physics/runtime/native_tu15_camera_instance_query_frame.hpp"\n'
BODY=re.compile(r'static void run_16c000\(WorldState &s\) \{.*?\n\}',re.S)
def digest(text):return hashlib.sha256(text.encode()).hexdigest()
def canonical_body(body):
    opening=body.index('{')+1
    return body[:opening]+body[opening+len(PREFIX):]if body[opening:].startswith(PREFIX)else body

def authenticate(source):
    found=list(BODY.finditer(source))
    if len(found)!=1:raise ValueError('Expected one complete16C000 publication owner')
    match=found[0];body=canonical_body(match[0])
    if digest(body)not in BODY_PINS:raise ValueError('Complete16C000 query body drift')
    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)
    if not resolver:raise ValueError('Complete16C000 selected resolver missing')
    require_selected_identity(source,body,RESOLVER_PINS)
    dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)
    if not dispatcher:raise ValueError('Missing complete16C000 owning dispatcher')
    fresh_literal=((digest(resolver[0]),digest(dispatcher[0]))in FRESH_LITERAL_CONTEXTS and
                   digest(body)==LITERAL_BODY_PIN and 'selected('not in body)
    if digest(resolver[0])not in RESOLVER_PINS and not fresh_literal:raise ValueError('Complete16C000 selected resolver drift')
    if digest(dispatcher[0])not in DISPATCH_PINS and not fresh_literal:
        extra=re.search(r'static void run_fa5d0\(WorldState &s\) \{.*?\n\}',source,re.S)
        if not(digest(dispatcher[0])==FRESH_PRIMARY_DISPATCH_PIN and dispatcher[0].count(FRESH_CASE)==1 and
               digest(dispatcher[0].replace(FRESH_CASE,'',1))in DISPATCH_PINS and extra and
               digest(_halfword.canonical_body(extra[0]))==_halfword.PIN):
            raise ValueError('Complete16C000 owning dispatcher drift')
    if (digest(body),digest(resolver[0]),digest(dispatcher[0]))not in BODY_CONTEXTS:
        raise ValueError('Complete16C000 body differs from its exact owning context')
    return match,body

def replace_camera_instance_query(source):
    if not BODY.search(source):return source
    match,body=authenticate(source)
    # The production parent must call the authenticated current dependency
    # owners. Retained references remain selected only by their own controls.
    dependencies=((0x16b810,'native_camera_instance_admission_replacements.py','replace_camera_instance_admission'),
                  (0x186e30,'native_camera_instance_bitmap_replacements.py','replace_camera_instance_bitmap'),
                  (0x120fa0,'native_segment_sphere_replacements.py','replace_segment_sphere'),
                  (0x1e68e0,'native_bsp_segment_setup_replacements.py','replace_bsp_segment_setup'))
    for entry,filename,name in dependencies:
        pattern=rf'static void run_{entry:x}\(WorldState &s\) \{{.*?\n\}}'
        if len(re.findall(pattern,source,re.S))!=1:
            raise ValueError('Native16C000 requires exactly one complete local '+hex(entry)+' owner')
        if getattr(_sibling(filename),name)(source)!=source:
            raise ValueError('Native16C000 requires its authenticated '+name+' owner')
    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)[0]
    publication=re.findall(r'static void run_16b9a0\(WorldState &s\) \{.*?\n\}',source,re.S)
    # The exact Primary resolver owns publication locally. Both authenticated
    # AI contexts reach the separate owner through their real dispatcher.
    expected_publication=int(digest(resolver)=='1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4')
    if len(publication)!=expected_publication:
        raise ValueError('Native16C000 query ownership differs from its exact context')
    if expected_publication:
        module=_sibling('native_camera_collision_publication_replacements.py')
        if module.replace_camera_collision_publication(source)!=source:
            raise ValueError('Native16C000 requires its authenticated publication owner')
    if match[0]==body:
        opening=body.index('{')+1;bound=body[:opening]+PREFIX+body[opening:]
        source=source[:match.start()]+bound+source[match.end():]
    if source.count(INCLUDE)>1:raise ValueError('Duplicate16C000 native include')
    if INCLUDE not in source:source=INCLUDE+source
    return source

PUBLIC_PIN='81d3d55e0aa7c7211e23a3560692f539bbba914d985a44cba04abb1923e2a6d5'
PUBLIC_INCLUDE='#include "halo2/native_tu15_camera_instance_query_value.hpp"\n'
PUBLIC_BODY='''bool native_tu15_query_camera_instance(SourcePhysicsContactMemory&memory,
 const NativeTu15CameraInstanceQuery&q,NativeTu15CameraInstanceQueryScratch scratch,
 NativeTu15PhysicsFrameFp&fp,unsigned depth,const SourcePhysicsContactRecordObserver&observe,
 NativeTu15CameraInstanceQueryStats*stats){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
 Memory m(memory,observe);
 const NativeTu15CameraInstanceQuery input{q.instance,q.mask,q.bsp_filter,q.origin,q.displacement,q.result};
 native_camera_instance_query_value_detail::View<Memory,Environment> view{memory,m,input,scratch,fp,depth,observe,stats};
 native_camera_instance_query_detail::Progress<__m128> data;return native_camera_instance_query_detail::query(view,data);
#else
 if(stats)++stats->calls;
 (void)memory;(void)q;(void)scratch;(void)fp;(void)depth;(void)observe;throw std::runtime_error("Native camera instance query backend unavailable");
#endif
}'''


def public_body(source):
    return _sibling('audit_native_execution_ownership.py').unique_function(source,'native_tu15_query_camera_instance')


def replace_public(source):
    current=public_body(source)
    if current!=PUBLIC_BODY and digest(current)!=PUBLIC_PIN:
        raise ValueError('Complete public16C000 query body drift')
    source=source.replace(current,PUBLIC_BODY,1)
    if source.count(PUBLIC_INCLUDE)>1:
        raise ValueError('Duplicate public camera query include')
    if PUBLIC_INCLUDE not in source:
        source=PUBLIC_INCLUDE+source
    return source

