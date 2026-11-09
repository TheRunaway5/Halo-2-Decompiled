"""Authenticate complete24DEC0 provider bodies and their actual source owners."""
import hashlib
import importlib.util
from pathlib import Path
import re

def _sibling(filename):
    spec=importlib.util.spec_from_file_location('instanced_child_'+Path(filename).stem,Path(__file__).with_name(filename))
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);return module
require_selected_identity=_sibling('native_selected_operand_binding.py').require_selected_identity
_halfword=_sibling('native_halfword_bit_query_replacements.py')
BODY_PINS={'e345ae00d8492e74338070b76ebacdfd661495f5a63513b92075f21b621fc4a9',
    '8d79a9820ea86fcf5863eb58afccebd9f425e86fa5871fa2043b643802dc2db2'}
# Output-only witness after the authenticated source installer. This does not
# admit a separately supplied specialized dispatcher to the source installer.
PREPARED_BODY_PIN='423a1570f18313b9d932d3233efbe7cb01bfe279b6b2b59445dd0f2ab1f4ee66'
RESOLVER_PINS={'1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4',
    '5eab69b03c4be4556290b5ba3e7765897c12d0064ea5b20267317b365ea4a657',
    'b366b77691cb314f6c6d0f8f20e71c1bda7114225c8d7b1f4043ee80e12354b8'}
DISPATCH_PINS={'5901d2c9c67421f4dabe988f833de38ce4a22dd4cb0778fbf19eb0fd0cb55ce0',
    '1cf61f52187c986d02510a40d28a466d95589b66af5521ade12ae9640085e28b',
    '974fddd715271a3ccdc93c266782b8f84d13f514bd11653b34af22a1f8be267a'}
FRESH_CASE='case 0xfa5d0u:run_fa5d0(s);return;'
FRESH_PRIMARY_DISPATCH_PIN='aba69140bdfbfe48111ef3727b7bb22a4891a8daa06770aabcb75183386d5849'
PREFIX='\n    if(halo2::native_instanced_ray_child_binding_detail::bridge(s))return;'
INCLUDE='#include "physics/runtime/native_tu15_instanced_ray_child.hpp"\n'
BODY=re.compile(r'static void run_24dec0\(WorldState &s\) \{.*?\n\}',re.S)
def digest(text):return hashlib.sha256(text.encode()).hexdigest()
def canonical_body(body):
    opening=body.index('{')+1
    return body[:opening]+body[opening+len(PREFIX):]if body[opening:].startswith(PREFIX)else body
def authenticate(source):
    found=list(BODY.finditer(source))
    if len(found)!=1:raise ValueError('Expected one complete24DEC0 provider owner')
    match=found[0];body=canonical_body(match[0])
    if digest(body)not in BODY_PINS:raise ValueError('Complete24DEC0 provider body drift')
    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)
    if not resolver or digest(resolver[0])not in RESOLVER_PINS:raise ValueError('Complete24DEC0 selected resolver drift')
    require_selected_identity(source,body,RESOLVER_PINS)
    dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)
    if not dispatcher:raise ValueError('Missing complete24DEC0 owning dispatcher')
    if digest(dispatcher[0])not in DISPATCH_PINS:
        # Only the already-qualified fresh FA5D0 case may distinguish the
        # known complete Primary dispatcher. Its whole body stays pinned.
        extra=re.search(r'static void run_fa5d0\(WorldState &s\) \{.*?\n\}',source,re.S)
        if not(digest(dispatcher[0])==FRESH_PRIMARY_DISPATCH_PIN and dispatcher[0].count(FRESH_CASE)==1 and digest(dispatcher[0].replace(FRESH_CASE,'',1))in DISPATCH_PINS and
               extra and digest(_halfword.canonical_body(extra[0]))==_halfword.PIN):
            raise ValueError('Complete24DEC0 owning dispatcher drift')
    return match,body
def replace_instanced_ray_child(source):
    if not BODY.search(source):return source
    match,body=authenticate(source)
    if match[0]==body:
        opening=body.index('{')+1;bound=body[:opening]+PREFIX+body[opening:]
        source=source[:match.start()]+bound+source[match.end():]
    if INCLUDE not in source:source=INCLUDE+source
    return source

PUBLIC_INCLUDE='#include "halo2/native_tu15_instanced_ray_child_value.hpp"\n'
PUBLIC_BODY='''std::uint32_t native_tu15_resolve_instanced_ray_child(SourcePhysicsContactMemory&memory,
    std::uint32_t collection,std::uint32_t key,std::uint32_t child,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15InstancedRayCallbackStats*stats){
    if(stats)++stats->provider_calls;if(!child){if(stats)++stats->provider_null;return 0;}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    try{joint_constraint_numeric::Memory words(memory,observe);
        auto narrow_store=[&](std::uint32_t pc,std::uint32_t address,std::uint32_t value,unsigned width){
            narrow(memory,observe,pc,address,value,width);
        };
        native_instanced_ray_child_value_detail::View view{words,narrow_store,collection,key,child};
        native_instanced_ray_child_detail::Progress data;return native_instanced_ray_child_detail::resolve(view,data);
    }catch(...){if(stats)++stats->refusals;throw;}
#else
    (void)memory;(void)collection;(void)key;(void)observe;throw std::runtime_error("Native instanced child backend unavailable");
#endif
}'''
def public_function(source,name):
    from audit_native_execution_ownership import unique_function
    return unique_function(source,name)
def replace_public(source):
    for name,pin in PUBLIC_PRESERVATION_PINS.items():
        if digest(public_function(source,name))!=pin:raise ValueError('Independent instanced callback/collector/helper drift '+name)
    current=public_function(source,'native_tu15_resolve_instanced_ray_child')
    if current!=PUBLIC_BODY and digest(current)!=PUBLIC_BODY_PIN:raise ValueError('Complete public instanced-child provider drift')
    source=source.replace(current,PUBLIC_BODY,1)
    if PUBLIC_INCLUDE not in source:source=PUBLIC_INCLUDE+source
    return source
def canonical_public(source):
    if digest(ORIGINAL_PUBLIC_BODY)!=PUBLIC_BODY_PIN:raise ValueError('Preserved original public child provider drift')
    current=public_function(source,'native_tu15_resolve_instanced_ray_child')
    if current==PUBLIC_BODY:source=source.replace(current,ORIGINAL_PUBLIC_BODY,1)
    elif digest(current)!=PUBLIC_BODY_PIN:raise ValueError('Complete public instanced-child provider drift')
    if source.startswith(PUBLIC_INCLUDE):source=source[len(PUBLIC_INCLUDE):]
    return source

PUBLIC_BODY_PIN='4113c3cf909e5ffdb6fe2fec2abf31d3245cfde1798e0c5c82f8ad87bf2669fd'
PUBLIC_REFERENCE_PIN='89cdb784e6c03b3b6721b52e65dfc9493d0d9c654be3e47cfac4592c205f97b1'
PUBLIC_PRESERVATION_PINS={'native_tu15_forward_instanced_child_ray': '70e344cb410597a2657baf0df703a994b7efe59117208e2471612c1d7687311e', 'collect': 'a5b285ced42a4dae47688e5f26909d7dc558fe8ff39f07026ccc09ba5077a847', 'native_tu15_collect_instanced_scene_ray': '4f3ff461e4a1b66502c909654a2f9ec072cf6f754ccd4755e16d6a8a0820e1bb', 'native_tu15_collect_instanced_scene_ray_key': 'a80df013f7139b7b6e30e3a0b58447d88c8bd0c40a15235c9e56279a40215ddf', 'native_tu15_forward_instanced_child_ray_key': '476b8316e87b424a0e54c0ddcafc13e3bccf78d730dba45bfa63fe8772a52a8b', 'narrow': '2bb162c75c218ff2024abac401f51aa822ffc92dd81c193ffbc28ff8c6910f6e'}
ORIGINAL_PUBLIC_BODY='std::uint32_t native_tu15_resolve_instanced_ray_child(SourcePhysicsContactMemory&memory,\n    std::uint32_t collection,std::uint32_t key,std::uint32_t child,\n    const SourcePhysicsContactRecordObserver&observe,NativeTu15InstancedRayCallbackStats*stats){\n    if(stats)++stats->provider_calls;if(!child){if(stats)++stats->provider_null;return 0;}\n#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)\n    try{joint_constraint_numeric::Memory m(memory,observe);const auto scenario=m.word(collection+12u);\n        m.word(0x24decbu,child+12u,scenario);narrow(memory,observe,0x24ded2u,child+6u,1u,2u);\n        m.word(0x24ded8u,child+8u,0u);m.word(0x24dedfu,child,0x46625cu);\n        m.word(0x24dee5u,child+16u,key);return child;\n    }catch(...){if(stats)++stats->refusals;throw;}\n#else\n    (void)memory;(void)collection;(void)key;(void)observe;throw std::runtime_error("Native instanced child backend unavailable");\n#endif\n}'
