"""Authenticate complete186E30 predicate owners and the shared public DATA view."""
import hashlib,importlib.util,re
from pathlib import Path

def _sibling(filename):
    spec=importlib.util.spec_from_file_location('camera_bitmap_'+Path(filename).stem,Path(__file__).with_name(filename))
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);return module
require_selected_identity=_sibling('native_selected_operand_binding.py').require_selected_identity
_halfword=_sibling('native_halfword_bit_query_replacements.py')
BODY_PINS={'d0e7e34d40ae429094f96c5e0980c245f62c7d8f8063a455d0bb637270bccba5',
 'cdba5b83223c9c29152de9e81ba3366f04e91908a0fb7643d520faa375e0712d'}
RESOLVER_PINS={'1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4',
 '5eab69b03c4be4556290b5ba3e7765897c12d0064ea5b20267317b365ea4a657'}
DISPATCH_PINS={'5901d2c9c67421f4dabe988f833de38ce4a22dd4cb0778fbf19eb0fd0cb55ce0',
 '1cf61f52187c986d02510a40d28a466d95589b66af5521ade12ae9640085e28b',
 '974fddd715271a3ccdc93c266782b8f84d13f514bd11653b34af22a1f8be267a'}
FRESH_LITERAL_CONTEXTS={
 # Exact unused resolver + complete dispatcher pairs emitted before the
 # owning AI wrapper. Only the complete authenticated literal body
 # may use these; no selected operand can consume either table.
 ('b366b77691cb314f6c6d0f8f20e71c1bda7114225c8d7b1f4043ee80e12354b8','974fddd715271a3ccdc93c266782b8f84d13f514bd11653b34af22a1f8be267a')}
LITERAL_BODY_PIN='d0e7e34d40ae429094f96c5e0980c245f62c7d8f8063a455d0bb637270bccba5'
# Output-only attribution of the actually hardware-compared prepared body.
PREPARED_OUTPUT_PIN='7cbae6e57eceaa3350c655903a3b223f222e69a1afe21da0cb6d6fe975bd5335'
FRESH_CASE='case 0xfa5d0u:run_fa5d0(s);return;'
FRESH_PRIMARY_DISPATCH_PIN='aba69140bdfbfe48111ef3727b7bb22a4891a8daa06770aabcb75183386d5849'
PREFIX='\n    if(halo2::native_camera_instance_bitmap_detail::bridge(s))return;'
INCLUDE='#include "physics/runtime/native_tu15_camera_instance_bitmap.hpp"\n'
BODY=re.compile(r'static void run_186e30\(WorldState &s\) \{.*?\n\}',re.S)
SHIFT_REWRITES={
    f's.flags.shift(s.reg({register},4,0),{count},4)':
    f'halo2::native_camera_instance_bitmap_detail::ordered_shift(s,s.reg({register},4,0),{count})'
    for register,values in ((0,('0x5u','selected(0x5u,s.image)')),(2,('0xau','selected(0xau,s.image)')))
    for count in values}
def digest(text):return hashlib.sha256(text.encode()).hexdigest()
def canonical_body(body):
    opening=body.index('{')+1
    body=body[:opening]+body[opening+len(PREFIX):]if body[opening:].startswith(PREFIX)else body
    for old,new in SHIFT_REWRITES.items():body=body.replace(new,old)
    return body

def authenticate(source):
    found=list(BODY.finditer(source))
    if len(found)!=1:raise ValueError('Expected one complete186E30 predicate owner')
    match=found[0];body=canonical_body(match[0])
    if digest(body)not in BODY_PINS:raise ValueError('Complete186E30 predicate body drift')
    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)
    if not resolver:raise ValueError('Complete186E30 selected resolver missing')
    require_selected_identity(source,body,RESOLVER_PINS)
    dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)
    if not dispatcher or 'run_186e30(s);'not in dispatcher[0]:raise ValueError('Missing complete186E30 owning dispatcher')
    fresh_literal=((digest(resolver[0]),digest(dispatcher[0]))in FRESH_LITERAL_CONTEXTS and
                   digest(body)==LITERAL_BODY_PIN and 'selected('not in body)
    if digest(resolver[0])not in RESOLVER_PINS and not fresh_literal:raise ValueError('Complete186E30 selected resolver drift')
    if digest(dispatcher[0])not in DISPATCH_PINS and not fresh_literal:
        extra=re.search(r'static void run_fa5d0\(WorldState &s\) \{.*?\n\}',source,re.S)
        if not(digest(dispatcher[0])==FRESH_PRIMARY_DISPATCH_PIN and dispatcher[0].count(FRESH_CASE)==1 and
               digest(dispatcher[0].replace(FRESH_CASE,'',1))in DISPATCH_PINS and extra and
               digest(_halfword.canonical_body(extra[0]))==_halfword.PIN):
            raise ValueError('Complete186E30 owning dispatcher drift')
    return match,body

def replace_camera_instance_bitmap(source):
    if not BODY.search(source):return source
    match,body=authenticate(source)
    bound=match[0];opening=bound.index('{')+1
    if not bound[opening:].startswith(PREFIX):bound=bound[:opening]+PREFIX+bound[opening:]
    for old,new in SHIFT_REWRITES.items():bound=bound.replace(old,new)
    source=source[:match.start()]+bound+source[match.end():]
    if source.count(INCLUDE)>1:raise ValueError('Duplicate186E30 native include')
    if INCLUDE not in source:source=INCLUDE+source
    return source

PUBLIC_PIN='59d5b0ad8a05da66e326d91d01b65cb95ac9155559f71de052725ca4cc45782f'
PUBLIC_INCLUDE='#include "halo2/native_tu15_camera_instance_bitmap_kernel.hpp"\n'
PUBLIC_BODY='std::uint32_t native_tu15_camera_instance_collision_bitmap(const SourcePhysicsContactMemory&memory,std::uint32_t instance){\n const SourcePhysicsContactRecordObserver none;Memory m(const_cast<SourcePhysicsContactMemory&>(memory),none);\n native_camera_instance_bitmap_detail::Values view{m,instance};native_camera_instance_bitmap_detail::Progress data;\n return native_camera_instance_bitmap_detail::locate(view,data);\n}'
def public_body(source):return _sibling('audit_native_execution_ownership.py').unique_function(source,'native_tu15_camera_instance_collision_bitmap')
def replace_public(source):
    current=public_body(source)
    if current!=PUBLIC_BODY and digest(current)!=PUBLIC_PIN:raise ValueError('Complete public186E30 bitmap body drift')
    source=source.replace(current,PUBLIC_BODY,1)
    if source.count(PUBLIC_INCLUDE)>1:raise ValueError('Duplicate public camera bitmap include')
    if PUBLIC_INCLUDE not in source:source=PUBLIC_INCLUDE+source
    return source
ORIGINAL_PUBLIC_BODY='std::uint32_t native_tu15_camera_instance_collision_bitmap(const SourcePhysicsContactMemory&memory,std::uint32_t instance){\n const SourcePhysicsContactRecordObserver none;Memory m(const_cast<SourcePhysicsContactMemory&>(memory),none);\n const auto index=sign16(m.word(0x474c24u,2)),base=m.word(0x4f9b50u);\n return instance==0xffffffffu?base+(index<<5u)+1u:base+(index<<10u)+instance+0x201u;\n}'
