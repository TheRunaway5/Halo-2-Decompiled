"""Authenticate complete16B9A0 publication owners and the shared public DATA view."""
import hashlib,importlib.util,re
from pathlib import Path

def _sibling(filename):
    spec=importlib.util.spec_from_file_location('camera_publication_'+Path(filename).stem,Path(__file__).with_name(filename))
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);return module
require_selected_identity=_sibling('native_selected_operand_binding.py').require_selected_identity
_halfword=_sibling('native_halfword_bit_query_replacements.py')
BODY_PINS={'331e4a98f6e6b5b8566bcb03acd3400e50f94d03a6a93394d9b9b77e42bf2d50'}
RESOLVER_PINS={'1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4'}
DISPATCH_PINS={'5901d2c9c67421f4dabe988f833de38ce4a22dd4cb0778fbf19eb0fd0cb55ce0'}
FRESH_LITERAL_CONTEXTS=set()
LITERAL_BODY_PIN='331e4a98f6e6b5b8566bcb03acd3400e50f94d03a6a93394d9b9b77e42bf2d50'
PREPARED_OUTPUT_PIN='36da4663a5457b40ee6b924e716f3b1db8ea80372ee7f4351b700a8a159e7403'
FRESH_CASE='case 0xfa5d0u:run_fa5d0(s);return;'
FRESH_PRIMARY_DISPATCH_PIN='aba69140bdfbfe48111ef3727b7bb22a4891a8daa06770aabcb75183386d5849'
PREFIX='\n    if(halo2::native_camera_collision_publication_detail::bridge(s))return;'
INCLUDE='#include "physics/runtime/native_tu15_camera_collision_publication.hpp"\n'
BODY=re.compile(r'static void run_16b9a0\(WorldState &s\) \{.*?\n\}',re.S)
def digest(text):return hashlib.sha256(text.encode()).hexdigest()
def canonical_body(body):
    opening=body.index('{')+1
    return body[:opening]+body[opening+len(PREFIX):]if body[opening:].startswith(PREFIX)else body

def authenticate(source):
    found=list(BODY.finditer(source))
    if len(found)!=1:raise ValueError('Expected one complete16B9A0 publication owner')
    match=found[0];body=canonical_body(match[0])
    if digest(body)not in BODY_PINS:raise ValueError('Complete16B9A0 publication body drift')
    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)
    if not resolver:raise ValueError('Complete16B9A0 selected resolver missing')
    require_selected_identity(source,body,RESOLVER_PINS)
    dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)
    if not dispatcher:raise ValueError('Missing complete16B9A0 owning dispatcher')
    fresh_literal=((digest(resolver[0]),digest(dispatcher[0]))in FRESH_LITERAL_CONTEXTS and
                   digest(body)==LITERAL_BODY_PIN and 'selected('not in body)
    if digest(resolver[0])not in RESOLVER_PINS and not fresh_literal:raise ValueError('Complete16B9A0 selected resolver drift')
    if digest(dispatcher[0])not in DISPATCH_PINS and not fresh_literal:
        extra=re.search(r'static void run_fa5d0\(WorldState &s\) \{.*?\n\}',source,re.S)
        if not(digest(dispatcher[0])==FRESH_PRIMARY_DISPATCH_PIN and dispatcher[0].count(FRESH_CASE)==1 and
               digest(dispatcher[0].replace(FRESH_CASE,'',1))in DISPATCH_PINS and extra and
               digest(_halfword.canonical_body(extra[0]))==_halfword.PIN):
            raise ValueError('Complete16B9A0 owning dispatcher drift')
    return match,body

def replace_camera_collision_publication(source):
    if not BODY.search(source):return source
    match,body=authenticate(source)
    if match[0]==body:
        opening=body.index('{')+1;bound=body[:opening]+PREFIX+body[opening:]
        source=source[:match.start()]+bound+source[match.end():]
    if source.count(INCLUDE)>1:raise ValueError('Duplicate16B9A0 native include')
    if INCLUDE not in source:source=INCLUDE+source
    return source

PUBLIC_PIN='7a50b4a077d4fd51949c1b6b890953b6e1a386978e15e9d0ad482c65dac9998d'
PUBLIC_INCLUDE='#include "halo2/native_tu15_camera_collision_publication_value.hpp"\n'
PUBLIC_BODY='void native_tu15_publish_camera_collision(SourcePhysicsContactMemory&m,std::uint32_t out,\n    std::uint32_t transform,std::uint32_t collision,NativeTu15PhysicsFrameFp&fp,const SourcePhysicsContactRecordObserver&o){\n#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)\n    Environment environment(fp);\n    auto read=[&](std::uint32_t address,unsigned width=4u){return word(m,address,width);};\n    auto write=[&](std::uint32_t pc,std::uint32_t address,std::uint32_t value,unsigned width){store(m,o,pc,address,value,width);};\n    native_camera_collision_publication_detail::Values view{read,write};\n    native_camera_collision_publication_detail::Progress data;data.output=out;data.transform=transform;data.collision=collision;\n    native_camera_collision_publication_detail::publish(view,data);\n#else\n    (void)m;(void)out;(void)transform;(void)collision;(void)fp;(void)o;throw std::runtime_error("Native camera collision backend unavailable");\n#endif\n}'

def public_body(source):return _sibling('audit_native_execution_ownership.py').unique_function(source,'native_tu15_publish_camera_collision')
def replace_public(source):
    current=public_body(source)
    if current!=PUBLIC_BODY and digest(current)!=PUBLIC_PIN:raise ValueError('Complete public16B9A0 publication body drift')
    source=source.replace(current,PUBLIC_BODY,1)
    if source.count(PUBLIC_INCLUDE)>1:raise ValueError('Duplicate public camera publication include')
    if PUBLIC_INCLUDE not in source:source=PUBLIC_INCLUDE+source
    return source
