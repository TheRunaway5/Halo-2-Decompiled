"""Authenticate complete16B810 predicate owners and the shared public DATA view."""
import hashlib,importlib.util,re
from pathlib import Path

def _sibling(filename):
    spec=importlib.util.spec_from_file_location('camera_admission_'+Path(filename).stem,Path(__file__).with_name(filename))
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);return module
require_selected_identity=_sibling('native_selected_operand_binding.py').require_selected_identity
_halfword=_sibling('native_halfword_bit_query_replacements.py')
BODY_PINS={'a57d91655c1931c2645e51ce0a614f580af902de7a975e9d84a7439b339f072c',
 '40c1ac3763ef1fc805242e3c5e0c94e0e4000e1f45c5a4627e12a44b3d42eba4'}
RESOLVER_PINS={'1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4',
 '5eab69b03c4be4556290b5ba3e7765897c12d0064ea5b20267317b365ea4a657',
 'cc3118fc8d131fc3d8beb8e314827a9412e84288f99b72509be63ff966688b61'}
DISPATCH_PINS={'5901d2c9c67421f4dabe988f833de38ce4a22dd4cb0778fbf19eb0fd0cb55ce0',
 '1cf61f52187c986d02510a40d28a466d95589b66af5521ade12ae9640085e28b',
 '2ddcb4ca932da7abcdbbbf8050bead73a3a18e4a4153021913f0555281b8aa20',
 '974fddd715271a3ccdc93c266782b8f84d13f514bd11653b34af22a1f8be267a'}
FRESH_LITERAL_CONTEXTS={
 # Exact unused resolver + complete dispatcher pairs emitted before the
 # owning AI/Death wrappers. Only the complete authenticated literal body
 # may use these; no selected operand can consume either table.
 ('b366b77691cb314f6c6d0f8f20e71c1bda7114225c8d7b1f4043ee80e12354b8','974fddd715271a3ccdc93c266782b8f84d13f514bd11653b34af22a1f8be267a'),
 ('1e70ace48573a64c67d51a9c7cde9f01c560e73795f8b089a918001d6bb1fbd6','1940da2e3890172390375a282ec9b1827d5a001fc02f5b85d0367efb9666ac45')}
LITERAL_BODY_PIN='a57d91655c1931c2645e51ce0a614f580af902de7a975e9d84a7439b339f072c'
# Output-only attribution of the actually hardware-compared prepared body.
PREPARED_OUTPUT_PIN='438959c33bcdf6655a680b854c1cc2d797eafc2f157fe4a8681a63403fc0de8a'
FRESH_CASE='case 0xfa5d0u:run_fa5d0(s);return;'
FRESH_PRIMARY_DISPATCH_PIN='aba69140bdfbfe48111ef3727b7bb22a4891a8daa06770aabcb75183386d5849'
PREFIX='\n    if(halo2::native_camera_instance_admission_detail::bridge(s))return;'
INCLUDE='#include "physics/runtime/native_tu15_camera_instance_admission.hpp"\n'
BODY=re.compile(r'static void run_16b810\(WorldState &s\) \{.*?\n\}',re.S)
def digest(text):return hashlib.sha256(text.encode()).hexdigest()
def canonical_body(body):
    opening=body.index('{')+1
    return body[:opening]+body[opening+len(PREFIX):]if body[opening:].startswith(PREFIX)else body

def authenticate(source):
    found=list(BODY.finditer(source))
    if len(found)!=1:raise ValueError('Expected one complete16B810 predicate owner')
    match=found[0];body=canonical_body(match[0])
    if digest(body)not in BODY_PINS:raise ValueError('Complete16B810 predicate body drift')
    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)
    if not resolver:raise ValueError('Complete16B810 selected resolver missing')
    require_selected_identity(source,body,RESOLVER_PINS)
    dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)
    if not dispatcher:raise ValueError('Missing complete16B810 owning dispatcher')
    fresh_literal=((digest(resolver[0]),digest(dispatcher[0]))in FRESH_LITERAL_CONTEXTS and
                   digest(body)==LITERAL_BODY_PIN and 'selected('not in body)
    if digest(resolver[0])not in RESOLVER_PINS and not fresh_literal:raise ValueError('Complete16B810 selected resolver drift')
    if digest(dispatcher[0])not in DISPATCH_PINS and not fresh_literal:
        extra=re.search(r'static void run_fa5d0\(WorldState &s\) \{.*?\n\}',source,re.S)
        if not(digest(dispatcher[0])==FRESH_PRIMARY_DISPATCH_PIN and dispatcher[0].count(FRESH_CASE)==1 and
               digest(dispatcher[0].replace(FRESH_CASE,'',1))in DISPATCH_PINS and extra and
               digest(_halfword.canonical_body(extra[0]))==_halfword.PIN):
            raise ValueError('Complete16B810 owning dispatcher drift')
    return match,body

def replace_camera_instance_admission(source):
    if not BODY.search(source):return source
    match,body=authenticate(source)
    if match[0]==body:
        opening=body.index('{')+1;bound=body[:opening]+PREFIX+body[opening:]
        source=source[:match.start()]+bound+source[match.end():]
    if source.count(INCLUDE)>1:raise ValueError('Duplicate16B810 native include')
    if INCLUDE not in source:source=INCLUDE+source
    return source

PUBLIC_PIN='5fc617d5c9b334a9843976f3009095fcb229c3e7c2593237fdcd776b922dac22'
PUBLIC_INCLUDE='#include "halo2/native_tu15_camera_instance_admission_value.hpp"\n'
PUBLIC_BODY='bool native_tu15_admit_camera_query_instance(const SourcePhysicsContactMemory&memory,\n std::uint32_t instance,std::uint32_t definition,std::uint32_t mask){\n const SourcePhysicsContactRecordObserver none;Memory m(const_cast<SourcePhysicsContactMemory&>(memory),none);\n native_camera_instance_admission_detail::Values view{m,instance,definition,mask};\n native_camera_instance_admission_detail::Progress data;\n return native_camera_instance_admission_detail::admit(view,data);\n}'
def public_body(source):return _sibling('audit_native_execution_ownership.py').unique_function(source,'native_tu15_admit_camera_query_instance')
def replace_public(source):
    current=public_body(source)
    if current!=PUBLIC_BODY and digest(current)!=PUBLIC_PIN:raise ValueError('Complete public16B810 predicate body drift')
    source=source.replace(current,PUBLIC_BODY,1)
    if source.count(PUBLIC_INCLUDE)>1:raise ValueError('Duplicate public camera admission include')
    if PUBLIC_INCLUDE not in source:source=PUBLIC_INCLUDE+source
    return source
ORIGINAL_PUBLIC_BODY='bool native_tu15_admit_camera_query_instance(const SourcePhysicsContactMemory&memory,\n std::uint32_t instance,std::uint32_t definition,std::uint32_t mask){\n const SourcePhysicsContactRecordObserver none;Memory m(const_cast<SourcePhysicsContactMemory&>(memory),none);\n const auto count=m.word(definition+0x98u);\n if(std::int32_t(count)<=0)return false;\n const auto rows=m.word(definition+0x9cu);\n if(m.word(rows+count*8u-4u,1)&0x10u)return false;\n if(!(mask&0x40000u))return true;\n const auto scenario=m.word(0x4ecc18u),clusters=m.word(scenario+0xc4u);\n if(std::int32_t(clusters)<=0)return true;\n auto table=m.word(scenario+0xc8u);if(!table)return true;\n if(std::int32_t(instance)<0)return false;\n if(std::int32_t(instance)>=std::int32_t(m.word(table+0x40u)))return false;\n table=m.word(table+0x44u);return m.word(table+instance*4u,2)!=0xffffu;\n}'
