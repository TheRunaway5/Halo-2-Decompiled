"""Strict complete packed-rotation source and public policy installation."""
import importlib.util,re
from pathlib import Path
def sibling(name):
    spec=importlib.util.spec_from_file_location('_packed_rotation_binding_'+name,Path(__file__).with_name(name+'.py'))
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);return module
inputs=sibling('native_packed_rotation_source_inputs')
PREFIX=inputs.PREFIX
INCLUDE=inputs.INCLUDE
canonical_body=inputs.canonical_body
authenticate=inputs.authenticate
BODY=re.compile(r'static void run_296180\(WorldState &s\) \{.*?\n\}',re.S)
PUBLIC_PIN='9dcc5919a56879a106b79fbfa92da195e935803661b223e991af6306ffa92e7c'
PUBLIC_NEW_PIN='9dcbef09d5a9ec24abf8ba2ad035631e10b5a1643914a5fd65fe3d54c8bf5a13'
MODEL_ANCHOR='#include "rendering/models/native_tu15_model_view_slots_bridge.hpp"\n'
PREPARED_CONTEXTS={
('0f6d8adf752ad0e9506e6e1f7b56e3a899ec904d88591d56134d97b280cb69e3','1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4','37477844625e78f7f747e917b6bc039f03390bfaa5419a9843557f0af9e17052','086a7e9e0eaa7ac687afb8b8094118d1211250698a0911a0beac4447b116ec46'):0,
('0f6d8adf752ad0e9506e6e1f7b56e3a899ec904d88591d56134d97b280cb69e3','1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4','8aee54bced297db8c7b162c84479071889c2bfc03472376f8a6cba0b7f68b851','457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575'):0,
('0f6d8adf752ad0e9506e6e1f7b56e3a899ec904d88591d56134d97b280cb69e3','5eab69b03c4be4556290b5ba3e7765897c12d0064ea5b20267317b365ea4a657','06f9ce13109229ec52f698071f0abb7f403a23f29cd2efaf9e27828d4057423e','457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575'):1,
('0f6d8adf752ad0e9506e6e1f7b56e3a899ec904d88591d56134d97b280cb69e3','b366b77691cb314f6c6d0f8f20e71c1bda7114225c8d7b1f4043ee80e12354b8','974fddd715271a3ccdc93c266782b8f84d13f514bd11653b34af22a1f8be267a','457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575'):1,
('0f6d8adf752ad0e9506e6e1f7b56e3a899ec904d88591d56134d97b280cb69e3','57359239fb680e03adea12fc0ce9e509fad6bef6ce05f9c03a9a37e7012d16db','c112d857c78bf307a089c1297c9b000870c293149fd78edd9852012887dd439d','457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575'):2,
('0f6d8adf752ad0e9506e6e1f7b56e3a899ec904d88591d56134d97b280cb69e3','57359239fb680e03adea12fc0ce9e509fad6bef6ce05f9c03a9a37e7012d16db','4af1a5ed426380ed605d661066155bd625fd9f3a1add625cedf00fdc17f15604','457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575'):2,
}
def replace_packed_rotation(source):
    if not re.search(r'static void run_296180\(WorldState &s\) \{',source):
        if PREFIX in source or INCLUDE in source:raise ValueError('Packed rotation overlay has no complete owner')
        return source
    owner,parts=authenticate(source);body=parts['current']
    if body==parts['body']:
        opening=body.index('{')+1;source=source.replace(body,body[:opening]+PREFIX+body[opening:],1)
        if owner==2 and MODEL_ANCHOR in source:
            if source.count(MODEL_ANCHOR)!=1:raise ValueError('Model packed rotation include anchor drift')
            at=source.index(MODEL_ANCHOR)+len(MODEL_ANCHOR);source=source[:at]+INCLUDE+source[at:]
        else:source=INCLUDE+source
    if source.count(INCLUDE)!=1:raise ValueError('Packed rotation include ownership changed')
    return source
def remove_packed_rotation(source):
    _,parts=authenticate(source)
    if parts['current']==parts['body']:return source
    return source.replace(parts['current'],parts['body'],1).replace(INCLUDE,'',1)
def installed_entries(source):
    if len(BODY.findall(source))!=1:raise ValueError('Missing or ambiguous prepared packed rotation owner')
    current=inputs.function(source,'run_296180')
    if current.count(PREFIX)!=1 or source.count(INCLUDE)!=1:raise ValueError('Packed rotation default is missing')
    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)
    dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)
    state=re.search(r'struct LiveState : physics_world_runtime_detail::WorldState \{.*?\n\};\nusing WorldState=LiveState;',source,re.S)
    if not resolver or not dispatcher or not state:raise ValueError('Prepared packed rotation context missing')
    context=tuple(map(inputs.digest,(current,resolver[0],dispatcher[0],state[0])))
    if context not in PREPARED_CONTEXTS:authenticate(source)
    else:
        for helper,pin in inputs.HELPERS.items():
            if inputs.digest(inputs.function(source,helper))!=pin:raise ValueError('Prepared packed rotation helper drift '+helper)
        if PREPARED_CONTEXTS[context]==0:
            halfword=sibling('native_halfword_bit_query_replacements');fa=halfword.canonical_body(inputs.function(source,'run_fa5d0'))
            fa=re.sub(r's\.instruction_literal\((0x[0-9a-f]+u)\);',r'if(s.services.instruction){s.services.instruction(\1);}',fa)
            if inputs.digest(fa)!=halfword.PIN:raise ValueError('Prepared packed rotation FA body drift')
    return ['296180']
def replace_value(source):
    if inputs.digest(source)==PUBLIC_NEW_PIN:return source
    value=sibling('validate_native_packed_rotation_value_tu15')
    if inputs.digest(source)!=PUBLIC_PIN:raise ValueError('Complete frontend grip owner drift')
    result=value.candidate(source)
    if inputs.digest(result)!=PUBLIC_NEW_PIN:raise ValueError('Qualified frontend grip candidate drift')
    return result
