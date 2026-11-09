"""Authenticate complete TU1.5 object-node controller and animation owners.

The similarly named MapPack State owner is deliberately outside this spelling.
"""
import hashlib
import importlib.util
from pathlib import Path
import re

def _sibling(filename):
    # Specialization is also imported by absolute file path. Keep the owning
    # helper graph file-relative without changing its caller's sys.path.
    spec=importlib.util.spec_from_file_location('object_node_'+Path(filename).stem,Path(__file__).with_name(filename))
    helper=importlib.util.module_from_spec(spec);spec.loader.exec_module(helper)
    return helper

require_selected_identity=_sibling('native_selected_operand_binding.py').require_selected_identity
replace_node_descriptor_lookup=_sibling('native_node_descriptor_lookup_replacements.py').replace_node_descriptor_lookup
_halfword=_sibling('native_halfword_bit_query_replacements.py')
halfword_body,HALFWORD_PIN=_halfword.canonical_body,_halfword.PIN

PARENT='bf0c0'
ANIMATION=('bfb90','b8780','b87b0')
BODY_PINS={
    'bf0c0':{'8bf592452daafe279643d1d0a060a3cc4daaed26add47339ffc0aa6ac702b950','982bfb2a2268f72084feb43f12b64b3f04672acba3e5a685bd07e8eb36411fd8','f1e9e24c477b1f63ed15a54a12adea28aa384f5b1c1968787b091bfd1f44c2ed'},
    'bfb90':{'566e4cb12c806b179051d6db181c2938d691490ab271f8930b918243aa50bba1','ecf665528de0172c857f795cd994829a3a7dc084b620cedcea421bfd48c8dfca'},
    'b8780':{'a9ebd40c6156f79b7d0347658c21346f31c8267d381475364827dc9468fc9085','1edeebd0cc5ab2f0d1a6807ec28441b4ceb418f39456a923560fa4bbdf4a08f0'},
    'b87b0':{'e17db6b696190912c4580a562d4ed5709062587f887ef9e137c0615b5bfd3feb','29b44b40363f4d97b4ebe4ba78c4fa5c9397e10a0c4752a04d30759db26df582'},
}
RESOLVER_PINS={'1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4','5eab69b03c4be4556290b5ba3e7765897c12d0064ea5b20267317b365ea4a657'}
# Exact fresh49-reference/691-owner common-generator context, before the AI
# module wrapper. Its four bodies are literal; retain its complete otherwise
# unused resolver identity rather than accepting arbitrary identity tables.
FRESH_AI_LITERAL_RESOLVER_PIN='b366b77691cb314f6c6d0f8f20e71c1bda7114225c8d7b1f4043ee80e12354b8'
FRESH_AI_LITERAL_DISPATCH_PIN='974fddd715271a3ccdc93c266782b8f84d13f514bd11653b34af22a1f8be267a'
SPECIALIZED_DISPATCH_PINS={
    '37477844625e78f7f747e917b6bc039f03390bfaa5419a9843557f0af9e17052':'primary',
    '06f9ce13109229ec52f698071f0abb7f403a23f29cd2efaf9e27828d4057423e':'frame',
}
ACTUAL_OWNER_ADAPTER=('if(halo2::native_allocator_owner_detail::dispatch(s,target,'
    '[](auto &v,std::uint32_t owner,std::uint32_t call_site,bool is_tail){dispatch(v,owner,call_site,is_tail);}))return;')
DISPATCH_PINS={'5901d2c9c67421f4dabe988f833de38ce4a22dd4cb0778fbf19eb0fd0cb55ce0','1cf61f52187c986d02510a40d28a466d95589b66af5521ade12ae9640085e28b',
    '974fddd715271a3ccdc93c266782b8f84d13f514bd11653b34af22a1f8be267a'}
FRESH_CASE='case 0xfa5d0u:run_fa5d0(s);return;'
PREFIXES={name:('\n    if(halo2::native_object_node_rebuild_detail::bridge(s,'if name==PARENT else f'\n    if(halo2::native_object_animation_detail::bridge(s,0x{name}u,')+
    '[](auto &v,std::uint32_t target,std::uint32_t site){dispatch(v,target,site,false);}))return;'for name in BODY_PINS}
INCLUDES=('#include "physics/runtime/native_tu15_object_animation_delivery_abi.hpp"\n','#include "physics/runtime/native_tu15_object_node_rebuild_abi.hpp"\n')
PREDICTION_COPY_ACTUAL_OWNER_IMAGES={0xadcd0:(27,'514e40a41f0f14fb43d196e57373d3444e54db20f032faa895eb6621c1494a2c')}

def canonical_body(name,value):
    start=value.index('{')+1;prefix=PREFIXES[name]
    return value[:start]+value[start+len(prefix):]if value[start:].startswith(prefix)else value

def body(source,name):
    found=list(re.finditer(r'static void run_'+name+r'\(WorldState &s\) \{.*?\n\}',source,re.S))
    if len(found)>1:raise ValueError('Ambiguous complete object node owner '+name)
    return found[0]if found else None

def authenticate(source):
    found={name:body(source,name)for name in BODY_PINS}
    if not any(found.values()):return{}
    if not all(found.values()):raise ValueError('Complete object node family missing before binding')
    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)
    dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)
    if not dispatcher:raise ValueError('Missing complete object node child dispatcher')
    pin=hashlib.sha256(dispatcher[0].encode()).hexdigest()
    resolver_pin=hashlib.sha256(resolver[0].encode()).hexdigest()if resolver else None
    fresh_ai_literal=(resolver_pin==FRESH_AI_LITERAL_RESOLVER_PIN and pin==FRESH_AI_LITERAL_DISPATCH_PIN and
        not any('selected('in canonical_body(name,match[0])for name,match in found.items()))
    if resolver_pin not in RESOLVER_PINS and not fresh_ai_literal:raise ValueError('Complete object node selected resolver drift')
    if pin not in DISPATCH_PINS:
        extra=body(source,'fa5d0')
        if not(dispatcher[0].count(FRESH_CASE)==1 and hashlib.sha256(dispatcher[0].replace(FRESH_CASE,'',1).encode()).hexdigest()in DISPATCH_PINS and
               extra and hashlib.sha256(halfword_body(extra[0]).encode()).hexdigest()==HALFWORD_PIN):
            raise ValueError('Complete object node child dispatcher drift')
    for name,match in found.items():
        original=canonical_body(name,match[0])
        if hashlib.sha256(original.encode()).hexdigest()not in BODY_PINS[name]:raise ValueError('Complete object node body drift '+name)
        require_selected_identity(source,original,RESOLVER_PINS)
    if replace_node_descriptor_lookup(source)!=source:raise ValueError('Object animation requires authenticated qualified node descriptor lookup')
    return found

def install_native_object_nodes(source):
    found=authenticate(source)
    if not found:return source
    for name in (*ANIMATION,PARENT):
        match=body(source,name);original=canonical_body(name,match[0])
        if original==match[0]:
            start=original.index('{')+1;changed=original[:start]+PREFIXES[name]+original[start:]
            source=source[:match.start()]+changed+source[match.end():]
    for include in reversed(INCLUDES):
        if include not in source:source=include+source
    return source

def installed_entries(source):
    # A repeated specialization keeps the exact native prefixes. Authenticate
    # the two complete emitted dispatchers, then restore only their proven
    # cursor/identity-call spelling before the same original full-body pins.
    dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)
    variant=SPECIALIZED_DISPATCH_PINS.get(hashlib.sha256(dispatcher[0].encode()).hexdigest())if dispatcher else None
    if not variant and dispatcher and dispatcher[0].count(FRESH_CASE)==1:
        base=hashlib.sha256(dispatcher[0].replace(FRESH_CASE,'',1).encode()).hexdigest()
        extra=body(source,'fa5d0')
        value=halfword_body(extra[0])if extra else''
        value=re.sub(r's\.instruction_literal\((0x[0-9a-f]+u)\);',r'if(s.services.instruction){s.services.instruction(\1);}',value)
        if base in SPECIALIZED_DISPATCH_PINS and SPECIALIZED_DISPATCH_PINS[base]=='primary'and hashlib.sha256(value.encode()).hexdigest()==HALFWORD_PIN:variant='primary-fresh'
    if variant:
        original_dispatch=dispatcher[0]
        if variant.startswith('primary'):
            if original_dispatch.count(ACTUAL_OWNER_ADAPTER)!=1:raise ValueError('Complete specialized object owner adapter drift')
            restored=original_dispatch.replace(ACTUAL_OWNER_ADAPTER,'',1)
            expected='5901d2c9c67421f4dabe988f833de38ce4a22dd4cb0778fbf19eb0fd0cb55ce0'if variant=='primary'else'aba69140bdfbfe48111ef3727b7bb22a4891a8daa06770aabcb75183386d5849'
        else:
            restored=re.sub(r'if\(target==(0x[0-9a-f]+u)\)',r'if(target==selected(\1,s.image))',original_dispatch)
            expected='1cf61f52187c986d02510a40d28a466d95589b66af5521ade12ae9640085e28b'
        if hashlib.sha256(restored.encode()).hexdigest()!=expected:raise ValueError('Complete specialized object dispatcher normalization drift')
        source=source.replace(original_dispatch,restored,1)
    source=re.sub(r's\.instruction_literal\((0x[0-9a-f]+u)\);',r'if(s.services.instruction){s.services.instruction(\1);}',source)
    found=authenticate(source)
    return [name for name,match in found.items()if canonical_body(name,match[0])!=match[0]]
