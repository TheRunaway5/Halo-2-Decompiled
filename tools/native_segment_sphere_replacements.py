"""Authenticate complete120FA0 predicate owners and the shared public DATA view."""
import hashlib,importlib.util,re
from pathlib import Path

def _sibling(filename):
    spec=importlib.util.spec_from_file_location('camera_publication_'+Path(filename).stem,Path(__file__).with_name(filename))
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);return module
require_selected_identity=_sibling('native_selected_operand_binding.py').require_selected_identity
_halfword=_sibling('native_halfword_bit_query_replacements.py')
BODY_PINS={'5517375891c88549590d4e8944f573d9b9bc489f7e423138d2005af8647921ab', '3ca5bd1ec276a7fadc69de67049f83304b507451672b41a6ef2362fdabc8f524'}
RESOLVER_PINS={'1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4','5eab69b03c4be4556290b5ba3e7765897c12d0064ea5b20267317b365ea4a657'}
DISPATCH_PINS={'5901d2c9c67421f4dabe988f833de38ce4a22dd4cb0778fbf19eb0fd0cb55ce0','1cf61f52187c986d02510a40d28a466d95589b66af5521ade12ae9640085e28b'}
FRESH_LITERAL_CONTEXTS={('b366b77691cb314f6c6d0f8f20e71c1bda7114225c8d7b1f4043ee80e12354b8','974fddd715271a3ccdc93c266782b8f84d13f514bd11653b34af22a1f8be267a')}
LITERAL_BODY_PIN='5517375891c88549590d4e8944f573d9b9bc489f7e423138d2005af8647921ab'
PREPARED_OUTPUT_PIN='dc44859db2ae0e2dd5742d7504872a9ed7c17951a0fc07aef07830219c177590'
FRESH_CASE='case 0xfa5d0u:run_fa5d0(s);return;'
FRESH_PRIMARY_DISPATCH_PIN='aba69140bdfbfe48111ef3727b7bb22a4891a8daa06770aabcb75183386d5849'
PREFIX='\n    if(halo2::native_segment_sphere_detail::bridge(s))return;'
INCLUDE='#include "physics/runtime/native_tu15_segment_sphere.hpp"\n'
BODY=re.compile(r'static void run_120fa0\(WorldState &s\) \{.*?\n\}',re.S)
def digest(text):return hashlib.sha256(text.encode()).hexdigest()
def canonical_body(body):
    opening=body.index('{')+1
    return body[:opening]+body[opening+len(PREFIX):]if body[opening:].startswith(PREFIX)else body

def authenticate(source):
    found=list(BODY.finditer(source))
    if len(found)!=1:raise ValueError('Expected one complete120FA0 predicate owner')
    match=found[0];body=canonical_body(match[0])
    if digest(body)not in BODY_PINS:raise ValueError('Complete120FA0 predicate body drift')
    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)
    if not resolver:raise ValueError('Complete120FA0 selected resolver missing')
    dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)
    if not dispatcher:raise ValueError('Missing complete120FA0 owning dispatcher')
    fresh_literal=((digest(resolver[0]),digest(dispatcher[0]))in FRESH_LITERAL_CONTEXTS and
                   digest(body)==LITERAL_BODY_PIN and 'selected('not in body)
    if not fresh_literal:require_selected_identity(source,body,RESOLVER_PINS)
    if digest(resolver[0])not in RESOLVER_PINS and not fresh_literal:raise ValueError('Complete120FA0 selected resolver drift')
    if digest(dispatcher[0])not in DISPATCH_PINS and not fresh_literal:
        extra=re.search(r'static void run_fa5d0\(WorldState &s\) \{.*?\n\}',source,re.S)
        if not(digest(dispatcher[0])==FRESH_PRIMARY_DISPATCH_PIN and dispatcher[0].count(FRESH_CASE)==1 and
               digest(dispatcher[0].replace(FRESH_CASE,'',1))in DISPATCH_PINS and extra and
               digest(_halfword.canonical_body(extra[0]))==_halfword.PIN):
            raise ValueError('Complete120FA0 owning dispatcher drift')
    return match,body

def replace_segment_sphere(source):
    if not BODY.search(source):return source
    match,body=authenticate(source)
    if match[0]==body:
        opening=body.index('{')+1;bound=body[:opening]+PREFIX+body[opening:]
        source=source[:match.start()]+bound+source[match.end():]
    if source.count(INCLUDE)>1:raise ValueError('Duplicate120FA0 native include')
    if INCLUDE not in source:source=INCLUDE+source
    return source

PUBLIC_PIN='f5b26047126fbfae4f61ac1f4044a75a48f488439fd203ba7931ff78e2eb73bd'
PUBLIC_INCLUDE='#include "halo2/native_tu15_segment_sphere_value.hpp"\n'
PUBLIC_BODY='bool native_tu15_segment_near_sphere(SourcePhysicsContactMemory&memory,\n    std::uint32_t first,std::uint32_t center,std::uint32_t direction,std::uint32_t radius_bits,NativeTu15PhysicsFrameFp&fp){\n#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)\n    SseEnvironment environment(fp);const SourcePhysicsContactRecordObserver empty;Memory m(memory,empty);\n    auto read=[&](std::uint32_t address){return m.word(address);};\n    native_segment_sphere_detail::Values view{read,first,center,direction,radius_bits};\n    native_segment_sphere_detail::Progress data;return native_segment_sphere_detail::intersects(view,data);\n#else\n    (void)memory;(void)first;(void)center;(void)direction;(void)radius_bits;(void)fp;\n    throw std::runtime_error("Exact native camera sphere query backend unavailable");\n#endif\n}'

def public_body(source):return _sibling('audit_native_execution_ownership.py').unique_function(source,'native_tu15_segment_near_sphere')
def replace_public(source):
    current=public_body(source)
    if current!=PUBLIC_BODY and digest(current)!=PUBLIC_PIN:raise ValueError('Complete public120FA0 sphere body drift')
    source=source.replace(current,PUBLIC_BODY,1)
    if source.count(PUBLIC_INCLUDE)>1:raise ValueError('Duplicate public segment sphere include')
    if PUBLIC_INCLUDE not in source:source=PUBLIC_INCLUDE+source
    return source
