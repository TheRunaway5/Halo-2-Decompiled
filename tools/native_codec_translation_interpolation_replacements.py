"""Private exact2962f0 source overlay and authenticated continuation labels."""
import importlib.util,re
from pathlib import Path
def sibling(name):
    spec=importlib.util.spec_from_file_location('_codec_translation_interpolation_'+name,Path(__file__).with_name(name+'.py'))
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);return module
inputs=sibling('native_model_marker_lookup_replacements')
BODY=re.compile(r'static void run_2962f0\(WorldState &s\) \{.*?\n\}',re.S)
INCLUDE='#include "physics/runtime/native_tu15_codec_translation_interpolation_source_owner.hpp"\n'
ANCHOR='namespace halo2 {\nnamespace {\nusing namespace physics_world_runtime_detail;\n'
INCLUDE_ANCHOR='#include "halo2/source_projectile_effect_observer.hpp"\n'
SITES=(2712310, 2712313, 2712316, 2712323, 2712331, 2712341, 2712349, 2712355, 2712358, 2712366, 2712372, 2712375, 2712381, 2712387, 2712395, 2712400, 2712403, 2712411, 2712416, 2712434, 2712437, 2712443, 2712448, 2712456, 2712464, 2712481, 2712489, 2712497, 2712514)
LABELS={pc:'native_codec_translation_interpolation_resume_'+format(pc,'x')+': 'for pc in SITES}
OLD_PREFIX='\n    switch(halo2::native_codec_translation_interpolation_owner_detail::bridge(s)){case 0u:break;case 1u:return;'+''.join('case 0x'+format(pc,'x')+'u:goto '+LABELS[pc][:-2]+';'for pc in SITES)+'default:throw std::logic_error("Unsupported native codec translation interpolation continuation");}'
RAW_PREFIX=OLD_PREFIX.replace('::bridge(s)', '::bridge<false>(s)')
PREPARED_PREFIX=OLD_PREFIX.replace('::bridge(s)', '::bridge<true>(s)')
PREFIX=RAW_PREFIX

UNITS=('src/physics/physics_live_support_tu15_generated.cpp', 'src/rendering/models/source_model_render_tu15_generated.cpp')
# Exact current/fresh tuples measured by genuine full-owner generation and8whole-CPP syntax.
CONTEXTS={('36ed28493a2e2710e798c4dab16101377808bea2274443b1b502c54cccc6df24', '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4', '5901d2c9c67421f4dabe988f833de38ce4a22dd4cb0778fbf19eb0fd0cb55ce0', '086a7e9e0eaa7ac687afb8b8094118d1211250698a0911a0beac4447b116ec46'): 0, ('980ae798cab81272832690bd151e09fecb9d4b54c1de57e6666d1ccd1f097992', '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4', '37477844625e78f7f747e917b6bc039f03390bfaa5419a9843557f0af9e17052', '086a7e9e0eaa7ac687afb8b8094118d1211250698a0911a0beac4447b116ec46'): 0, ('c3dafa7ac8e43e84d8626adf70ff858ef3359738a26185ce8c9320186ada9317', '57359239fb680e03adea12fc0ce9e509fad6bef6ce05f9c03a9a37e7012d16db', 'f928af6956a3965aef767189095b9732744dcbdb209a345a3aec57baa618e0d1', '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575'): 1, ('980ae798cab81272832690bd151e09fecb9d4b54c1de57e6666d1ccd1f097992', '57359239fb680e03adea12fc0ce9e509fad6bef6ce05f9c03a9a37e7012d16db', 'c112d857c78bf307a089c1297c9b000870c293149fd78edd9852012887dd439d', '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575'): 1, ('36ed28493a2e2710e798c4dab16101377808bea2274443b1b502c54cccc6df24', '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4', 'aba69140bdfbfe48111ef3727b7bb22a4891a8daa06770aabcb75183386d5849', '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575'): 0, ('980ae798cab81272832690bd151e09fecb9d4b54c1de57e6666d1ccd1f097992', '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4', '8aee54bced297db8c7b162c84479071889c2bfc03472376f8a6cba0b7f68b851', '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575'): 0, ('36ed28493a2e2710e798c4dab16101377808bea2274443b1b502c54cccc6df24', '57359239fb680e03adea12fc0ce9e509fad6bef6ce05f9c03a9a37e7012d16db', '4af1a5ed426380ed605d661066155bd625fd9f3a1add625cedf00fdc17f15604', '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575'): 1, ('980ae798cab81272832690bd151e09fecb9d4b54c1de57e6666d1ccd1f097992', '57359239fb680e03adea12fc0ce9e509fad6bef6ce05f9c03a9a37e7012d16db', '4af1a5ed426380ed605d661066155bd625fd9f3a1add625cedf00fdc17f15604', '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575'): 1}
ORDERED={}
def include_position(source):
    if source.count(ANCHOR)!=1 or source.count(INCLUDE_ANCHOR)!=1:
        raise ValueError('Codec translation interpolation owning include anchor drift')
    at=source.index(INCLUDE_ANCHOR)+len(INCLUDE_ANCHOR)
    if at>source.index(ANCHOR):raise ValueError('Codec translation interpolation include anchor is not global')
    return at
def ordered_body(body):
    for old,new in ORDERED.items():
        if body.count(old)!=1 or new in body:raise ValueError('Exact interpolation ordered expression drift')
        body=body.replace(old,new,1)
    return body
def canonical_body(body,*,allow_retag=False):
    at=body.index('{')+1
    prefixes=[prefix for prefix in(RAW_PREFIX,PREPARED_PREFIX)if body[at:].startswith(prefix)]
    if not prefixes:
        if OLD_PREFIX in body or any(label in body for label in LABELS.values()):raise ValueError('Untagged or partial interpolation overlay')
        return body
    if len(prefixes)!=1 or body.count(prefixes[0])!=1:raise ValueError('Ambiguous tagged interpolation owner')
    prefix=prefixes[0];result=body[:at]+body[at+len(prefix):]
    for pc,label in LABELS.items():
        if result.count(label)!=1:raise ValueError('Incomplete codec translation interpolation continuation labels')
        raw='if(s.services.instruction){s.services.instruction(0x'+format(pc,'x')+'u);}'
        literal='s.instruction_literal(0x'+format(pc,'x')+'u);'
        if sum(result.count(label+callback)for callback in(raw,literal))!=1:raise ValueError('Relocated codec translation interpolation continuation label')
        result=result.replace(label,'',1)
    for old,new in ORDERED.items():
        if result.count(new)!=1 or old in result:raise ValueError('Incomplete ordered interpolation fallback')
        result=result.replace(new,old,1)
    literal='s.instruction_literal('in result
    if not allow_retag and prefix!=(PREPARED_PREFIX if literal else RAW_PREFIX):raise ValueError('Interpolation source-format tag mismatch')
    return result
def authenticate(source):
    bodies=list(BODY.finditer(source))
    if len(bodies)!=1:raise ValueError('Missing or ambiguous complete codec translation interpolation owner')
    body=canonical_body(bodies[0][0])
    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)
    dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)
    state=re.search(r'struct LiveState : physics_world_runtime_detail::WorldState \{.*?\n\};\nusing WorldState=LiveState;',source,re.S)
    if not resolver or not dispatcher or not state:raise ValueError('Incomplete codec translation interpolation producer context')
    context=tuple(map(inputs.digest,(body,resolver[0],dispatcher[0],state[0])))
    if context not in CONTEXTS:raise ValueError('Complete codec translation interpolation body/resolver/dispatcher/class drift')
    installed=bodies[0][0]!=body
    if source.count(INCLUDE)!=(1 if installed else 0):raise ValueError('Codec translation interpolation include/prefix ownership drift')
    at=include_position(source)
    if installed and not source.startswith(INCLUDE,at):raise ValueError('Codec translation interpolation include placement drift')
    for prefix in(RAW_PREFIX,PREPARED_PREFIX,OLD_PREFIX):
        if source.count(prefix)!=int(prefix in bodies[0][0]):raise ValueError('Foreign interpolation prefix outside its owner')
    return CONTEXTS[context],bodies[0],body
def replace_codec_translation_interpolation(source):
    if not BODY.search(source):
        if any(prefix in source for prefix in(RAW_PREFIX,PREPARED_PREFIX,OLD_PREFIX))or INCLUDE in source:raise ValueError('Codec translation interpolation overlay lacks its owner')
        return source
    owner,match,body=authenticate(source)
    if match[0]!=body:return source
    body=ordered_body(body)
    for pc,label in LABELS.items():
        raw='if(s.services.instruction){s.services.instruction(0x'+format(pc,'x')+'u);}'
        literal='s.instruction_literal(0x'+format(pc,'x')+'u);'
        candidates=[needle for needle in(raw,literal)if body.count(needle)==1]
        if len(candidates)!=1:raise ValueError('Codec translation interpolation continuation site drift')
        body=body.replace(candidates[0],label+candidates[0],1)
    at=body.index('{')+1;prefix=PREPARED_PREFIX if 's.instruction_literal('in body else RAW_PREFIX;body=body[:at]+prefix+body[at:]
    result=source[:match.start()]+body+source[match.end():]
    # Keep leading and trailing generated-owner include relations independent.
    at=include_position(result)
    return result[:at]+INCLUDE+result[at:]
def remove_codec_translation_interpolation(source):
    _,match,body=authenticate(source)
    if match[0]==body:return source
    return (source[:match.start()]+body+source[match.end():]).replace(INCLUDE,'',1)
def installed_entries(source):
    _,match,body=authenticate(source)
    if match[0]==body or sum(match[0].count(prefix)for prefix in(RAW_PREFIX,PREPARED_PREFIX))!=1:raise ValueError('Codec translation interpolation native overlay missing')
    return ['2962f0']


def rebind_prepared(source):
    matches=list(BODY.finditer(source))
    if len(matches)!=1:raise ValueError('Missing complete interpolation retag owner')
    match=matches[0];body=canonical_body(match[0],allow_retag=True)
    if body==match[0]or 's.instruction_literal('not in body:raise ValueError('Interpolation retag input is not a prepared overlay')
    at=match[0].index('{')+1
    prefix=next((value for value in(RAW_PREFIX,PREPARED_PREFIX)if match[0][at:].startswith(value)),None)
    if prefix is None:raise ValueError('Missing tagged interpolation retag prefix')
    result=source[:match.start()]+match[0][:at]+PREPARED_PREFIX+match[0][at+len(prefix):]+source[match.end():]
    authenticate(result)
    return result
