"""Private exact2961F0 source overlay and authenticated continuation labels."""
import importlib.util,re
from pathlib import Path
def sibling(name):
    spec=importlib.util.spec_from_file_location('_codec_translation_'+name,Path(__file__).with_name(name+'.py'))
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);return module
packed=sibling('native_packed_rotation_replacements')
inputs=packed.inputs
BODY=re.compile(r'static void run_2961f0\(WorldState &s\) \{.*?\n\}',re.S)
INCLUDE='#include "physics/runtime/native_tu15_codec_translation_source_owner.hpp"\n'
ANCHOR=packed.INCLUDE
INTERPOLATION_INCLUDE='#include "physics/runtime/native_tu15_codec_rotation_interpolation_source_owner.hpp"\n'
def include_position(source,installed):
    if source.count(ANCHOR)!=1:raise ValueError('Codec translation owning include anchor drift')
    at=source.index(ANCHOR)+len(ANCHOR)
    if INTERPOLATION_INCLUDE in source:
        if source.count(INTERPOLATION_INCLUDE)!=1 or not source.startswith(INTERPOLATION_INCLUDE,at):
            raise ValueError('Codec translation known interpolation sibling placement drift')
        at+=len(INTERPOLATION_INCLUDE)
    if installed and source.index(INCLUDE)!=at:
        raise ValueError('Codec translation include placement drift')
    return at
SITES=(0x2961f5,0x2961fd,0x296200,0x296209,0x29620d,0x296212,0x296215,0x296218,0x29621b,0x29621e)
LABELS={pc:'native_codec_translation_resume_'+format(pc,'x')+': 'for pc in SITES}
PREFIX='\n    switch(halo2::native_codec_translation_owner_detail::bridge(s)){case 0u:break;case 1u:return;'+''.join('case 0x'+format(pc,'x')+'u:goto '+LABELS[pc][:-2]+';'for pc in SITES)+'default:throw std::logic_error("Unsupported native codec translation continuation");}'
CONTEXTS={('0ccee0c159968ca18af5c82d307ad6eb9912c9b66e7bf4e2991049c92004d27d', '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4', '5901d2c9c67421f4dabe988f833de38ce4a22dd4cb0778fbf19eb0fd0cb55ce0', '086a7e9e0eaa7ac687afb8b8094118d1211250698a0911a0beac4447b116ec46'): 0, ('0f67c5e711c83cdf31934fd02203288928be3464b5e95b19d8e8571d2a314184', '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4', '37477844625e78f7f747e917b6bc039f03390bfaa5419a9843557f0af9e17052', '086a7e9e0eaa7ac687afb8b8094118d1211250698a0911a0beac4447b116ec46'): 0, ('0ccee0c159968ca18af5c82d307ad6eb9912c9b66e7bf4e2991049c92004d27d', '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4', 'aba69140bdfbfe48111ef3727b7bb22a4891a8daa06770aabcb75183386d5849', '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575'): 0, ('0f67c5e711c83cdf31934fd02203288928be3464b5e95b19d8e8571d2a314184', '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4', '8aee54bced297db8c7b162c84479071889c2bfc03472376f8a6cba0b7f68b851', '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575'): 0, ('a114b042529b301e48726c554d0d45d53d775d8b7eac6f6e9fa15f4592252d55', '5eab69b03c4be4556290b5ba3e7765897c12d0064ea5b20267317b365ea4a657', '1cf61f52187c986d02510a40d28a466d95589b66af5521ade12ae9640085e28b', '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575'): 1, ('0f67c5e711c83cdf31934fd02203288928be3464b5e95b19d8e8571d2a314184', '5eab69b03c4be4556290b5ba3e7765897c12d0064ea5b20267317b365ea4a657', '06f9ce13109229ec52f698071f0abb7f403a23f29cd2efaf9e27828d4057423e', '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575'): 1, ('0ccee0c159968ca18af5c82d307ad6eb9912c9b66e7bf4e2991049c92004d27d', 'b366b77691cb314f6c6d0f8f20e71c1bda7114225c8d7b1f4043ee80e12354b8', '974fddd715271a3ccdc93c266782b8f84d13f514bd11653b34af22a1f8be267a', '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575'): 1, ('0f67c5e711c83cdf31934fd02203288928be3464b5e95b19d8e8571d2a314184', 'b366b77691cb314f6c6d0f8f20e71c1bda7114225c8d7b1f4043ee80e12354b8', '974fddd715271a3ccdc93c266782b8f84d13f514bd11653b34af22a1f8be267a', '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575'): 1, ('a114b042529b301e48726c554d0d45d53d775d8b7eac6f6e9fa15f4592252d55', '57359239fb680e03adea12fc0ce9e509fad6bef6ce05f9c03a9a37e7012d16db', 'f928af6956a3965aef767189095b9732744dcbdb209a345a3aec57baa618e0d1', '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575'): 2, ('0f67c5e711c83cdf31934fd02203288928be3464b5e95b19d8e8571d2a314184', '57359239fb680e03adea12fc0ce9e509fad6bef6ce05f9c03a9a37e7012d16db', 'c112d857c78bf307a089c1297c9b000870c293149fd78edd9852012887dd439d', '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575'): 2, ('0ccee0c159968ca18af5c82d307ad6eb9912c9b66e7bf4e2991049c92004d27d', '57359239fb680e03adea12fc0ce9e509fad6bef6ce05f9c03a9a37e7012d16db', '4af1a5ed426380ed605d661066155bd625fd9f3a1add625cedf00fdc17f15604', '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575'): 2, ('0f67c5e711c83cdf31934fd02203288928be3464b5e95b19d8e8571d2a314184', '57359239fb680e03adea12fc0ce9e509fad6bef6ce05f9c03a9a37e7012d16db', '4af1a5ed426380ed605d661066155bd625fd9f3a1add625cedf00fdc17f15604', '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575'): 2}
def canonical_body(body):
    at=body.index('{')+1
    if not body[at:].startswith(PREFIX):return body
    result=body[:at]+body[at+len(PREFIX):]
    for pc,label in LABELS.items():
        if result.count(label)!=1:raise ValueError('Incomplete codec translation continuation labels')
        raw='if(s.services.instruction){s.services.instruction(0x'+format(pc,'x')+'u);}'
        literal='s.instruction_literal(0x'+format(pc,'x')+'u);'
        if sum(result.count(label+callback)for callback in(raw,literal))!=1:raise ValueError('Relocated codec translation continuation label')
        result=result.replace(label,'',1)
    return result
def authenticate(source):
    bodies=list(BODY.finditer(source))
    if len(bodies)!=1:raise ValueError('Missing or ambiguous complete codec translation owner')
    body=canonical_body(bodies[0][0])
    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)
    dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)
    state=re.search(r'struct LiveState : physics_world_runtime_detail::WorldState \{.*?\n\};\nusing WorldState=LiveState;',source,re.S)
    if not resolver or not dispatcher or not state:raise ValueError('Incomplete codec translation producer context')
    context=tuple(map(inputs.digest,(body,resolver[0],dispatcher[0],state[0])))
    if context not in CONTEXTS:raise ValueError('Complete codec translation body/resolver/dispatcher/class drift')
    # This is the independently authenticated existing producer cohort, not a
    # runtime child. Reuse its whole fresh-FA/helper and prepared-shim checks.
    if packed.installed_entries(source)!=['296180']:raise ValueError('Authenticated codec producer cohort changed')
    installed=bodies[0][0]!=body
    if source.count(INCLUDE)!=(1 if installed else 0):raise ValueError('Codec translation include/prefix ownership drift')
    include_position(source,installed)
    return CONTEXTS[context],bodies[0],body
def replace_codec_translation(source):
    if not BODY.search(source):
        if PREFIX in source or INCLUDE in source:raise ValueError('Codec translation overlay lacks its owner')
        return source
    owner,match,body=authenticate(source)
    if match[0]!=body:return source
    for pc,label in LABELS.items():
        raw='if(s.services.instruction){s.services.instruction(0x'+format(pc,'x')+'u);}'
        literal='s.instruction_literal(0x'+format(pc,'x')+'u);'
        candidates=[needle for needle in(raw,literal)if body.count(needle)==1]
        if len(candidates)!=1:raise ValueError('Codec translation continuation site drift')
        body=body.replace(candidates[0],label+candidates[0],1)
    at=body.index('{')+1;body=body[:at]+PREFIX+body[at:]
    result=source[:match.start()]+body+source[match.end():]
    at=include_position(result,False)
    return result[:at]+INCLUDE+result[at:]
def remove_codec_translation(source):
    _,match,body=authenticate(source)
    if match[0]==body:return source
    return (source[:match.start()]+body+source[match.end():]).replace(INCLUDE,'',1)
def installed_entries(source):
    _,match,body=authenticate(source)
    if match[0]==body or match[0].count(PREFIX)!=1:raise ValueError('Codec translation native overlay missing')
    return ['2961f0']

