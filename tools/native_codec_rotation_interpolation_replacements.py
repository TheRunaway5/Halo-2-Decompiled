"""Private exact296240 source overlay and authenticated continuation labels."""
import importlib.util,re
from pathlib import Path
def sibling(name):
    spec=importlib.util.spec_from_file_location('_codec_rotation_interpolation_'+name,Path(__file__).with_name(name+'.py'))
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);return module
inputs=sibling('native_model_marker_lookup_replacements')
packed=sibling('native_packed_rotation_replacements')
BODY=re.compile(r'static void run_296240\(WorldState &s\) \{.*?\n\}',re.S)
INCLUDE='#include "physics/runtime/native_tu15_codec_rotation_interpolation_source_owner.hpp"\n'
ANCHOR='#include "physics/runtime/native_tu15_packed_rotation_source_owner.hpp"\n'
SITES=(0x296248,0x29624b,0x296252,0x296258,0x296260,0x29626a,0x29626d,0x296273,0x296283,0x296287,0x29628b,0x29628f,0x296293,0x296299,0x29629c,0x29629f,0x2962c9,0x2962cc,0x2962d2)
LABELS={pc:'native_codec_rotation_interpolation_resume_'+format(pc,'x')+': 'for pc in SITES}
OLD_PREFIX='\n    switch(halo2::native_codec_rotation_interpolation_owner_detail::bridge(s)){case 0u:break;case 1u:return;'+''.join('case 0x'+format(pc,'x')+'u:goto '+LABELS[pc][:-2]+';'for pc in SITES)+'default:throw std::logic_error("Unsupported native codec rotation interpolation continuation");}'
RAW_PREFIX=OLD_PREFIX.replace('::bridge(s)', '::bridge<false>(s)')
PREPARED_PREFIX=OLD_PREFIX.replace('::bridge(s)', '::bridge<true>(s)')
PREFIX=RAW_PREFIX

UNITS=('src/physics/physics_live_support_tu15_generated.cpp', 'src/rendering/models/source_model_render_tu15_generated.cpp')
# Current/fresh complete tuples come from genuine eight-context full generation.
CONTEXTS={('da886c672464b2f714114b44c34bbd69186680fe470bf2d0de846ee7d5527d37', '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4', '5901d2c9c67421f4dabe988f833de38ce4a22dd4cb0778fbf19eb0fd0cb55ce0', '086a7e9e0eaa7ac687afb8b8094118d1211250698a0911a0beac4447b116ec46'): 0, ('6d42fe6adc77a60e72f8ea7a0974bc0099cea5f7ec55b974c1be130f138bf82a', '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4', '37477844625e78f7f747e917b6bc039f03390bfaa5419a9843557f0af9e17052', '086a7e9e0eaa7ac687afb8b8094118d1211250698a0911a0beac4447b116ec46'): 0, ('c05d27413620e331733629ab4f8872de3c6db16fe6f745b0e95e1686ae06df34', '57359239fb680e03adea12fc0ce9e509fad6bef6ce05f9c03a9a37e7012d16db', 'f928af6956a3965aef767189095b9732744dcbdb209a345a3aec57baa618e0d1', '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575'): 1, ('6d42fe6adc77a60e72f8ea7a0974bc0099cea5f7ec55b974c1be130f138bf82a', '57359239fb680e03adea12fc0ce9e509fad6bef6ce05f9c03a9a37e7012d16db', 'c112d857c78bf307a089c1297c9b000870c293149fd78edd9852012887dd439d', '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575'): 1, ('da886c672464b2f714114b44c34bbd69186680fe470bf2d0de846ee7d5527d37', '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4', 'aba69140bdfbfe48111ef3727b7bb22a4891a8daa06770aabcb75183386d5849', '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575'): 0, ('6d42fe6adc77a60e72f8ea7a0974bc0099cea5f7ec55b974c1be130f138bf82a', '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4', '8aee54bced297db8c7b162c84479071889c2bfc03472376f8a6cba0b7f68b851', '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575'): 0, ('da886c672464b2f714114b44c34bbd69186680fe470bf2d0de846ee7d5527d37', '57359239fb680e03adea12fc0ce9e509fad6bef6ce05f9c03a9a37e7012d16db', '4af1a5ed426380ed605d661066155bd625fd9f3a1add625cedf00fdc17f15604', '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575'): 1, ('6d42fe6adc77a60e72f8ea7a0974bc0099cea5f7ec55b974c1be130f138bf82a', '57359239fb680e03adea12fc0ce9e509fad6bef6ce05f9c03a9a37e7012d16db', '4af1a5ed426380ed605d661066155bd625fd9f3a1add625cedf00fdc17f15604', '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575'): 1}
ORDERED={
    '_mm_sub_ps(s.xmm[2],s.xmm[1])':'halo2::native_codec_rotation_interpolation_detail::ordered_subtract(s.image,s.xmm[2],s.xmm[1])',
    '_mm_mul_ps(s.xmm[2],s.xmm[3])':'halo2::native_codec_rotation_interpolation_detail::ordered_multiply(s.image,s.xmm[2],s.xmm[3])',
    '_mm_add_ps(s.xmm[2],s.xmm[1])':'halo2::native_codec_rotation_interpolation_detail::ordered_add(s.image,s.xmm[2],s.xmm[1])',
    '_mm_mul_ps(s.xmm[0],s.xmm[2])':'halo2::native_codec_rotation_interpolation_detail::ordered_multiply(s.image,s.xmm[0],s.xmm[2])',
    '_mm_add_ps(s.xmm[0],s.xmm[7])':'halo2::native_codec_rotation_interpolation_detail::ordered_add(s.image,s.xmm[0],s.xmm[7])',
    '_mm_add_ps(s.xmm[0],s.xmm[6])':'halo2::native_codec_rotation_interpolation_detail::ordered_add(s.image,s.xmm[0],s.xmm[6])',
    '_mm_mul_ps(s.xmm[2],s.xmm[0])':'halo2::native_codec_rotation_interpolation_detail::ordered_multiply(s.image,s.xmm[2],s.xmm[0])'}
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
        if result.count(label)!=1:raise ValueError('Incomplete codec rotation interpolation continuation labels')
        raw='if(s.services.instruction){s.services.instruction(0x'+format(pc,'x')+'u);}'
        literal='s.instruction_literal(0x'+format(pc,'x')+'u);'
        if sum(result.count(label+callback)for callback in(raw,literal))!=1:raise ValueError('Relocated codec rotation interpolation continuation label')
        result=result.replace(label,'',1)
    for old,new in ORDERED.items():
        if result.count(new)!=1 or old in result:raise ValueError('Incomplete ordered interpolation fallback')
        result=result.replace(new,old,1)
    literal='s.instruction_literal('in result
    if not allow_retag and prefix!=(PREPARED_PREFIX if literal else RAW_PREFIX):raise ValueError('Interpolation source-format tag mismatch')
    return result
def authenticate(source):
    bodies=list(BODY.finditer(source))
    if len(bodies)!=1:raise ValueError('Missing or ambiguous complete codec rotation interpolation owner')
    body=canonical_body(bodies[0][0])
    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)
    dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)
    state=re.search(r'struct LiveState : physics_world_runtime_detail::WorldState \{.*?\n\};\nusing WorldState=LiveState;',source,re.S)
    if not resolver or not dispatcher or not state:raise ValueError('Incomplete codec rotation interpolation producer context')
    context=tuple(map(inputs.digest,(body,resolver[0],dispatcher[0],state[0])))
    if context not in CONTEXTS:raise ValueError('Complete codec rotation interpolation body/resolver/dispatcher/class drift')
    for helper,pin in (('source_estimate','bb50fdcd1d22250d871032ff227a462ef42c30981de20b9f23137bb1c60efd89'),('native_estimate_vector','a5e2ad3c7a078be7decea72984e8efc82b03f966b46aea97f1cc89e4c48d6aa4')):
        if inputs.digest(inputs.function(source,helper))!=pin:raise ValueError('Complete interpolation estimate helper drift')
    if packed.installed_entries(source)!=['296180']:raise ValueError('Authenticated interpolation producer/FA cohort changed')
    installed=bodies[0][0]!=body
    if source.count(INCLUDE)!=(1 if installed else 0):raise ValueError('Codec rotation interpolation include/prefix ownership drift')
    for prefix in(RAW_PREFIX,PREPARED_PREFIX,OLD_PREFIX):
        if source.count(prefix)!=int(prefix in bodies[0][0]):raise ValueError('Foreign interpolation prefix outside its owner')
    return CONTEXTS[context],bodies[0],body
def replace_codec_rotation_interpolation(source):
    if not BODY.search(source):
        if any(prefix in source for prefix in(RAW_PREFIX,PREPARED_PREFIX,OLD_PREFIX))or INCLUDE in source:raise ValueError('Codec rotation interpolation overlay lacks its owner')
        return source
    owner,match,body=authenticate(source)
    if match[0]!=body:return source
    body=ordered_body(body)
    for pc,label in LABELS.items():
        raw='if(s.services.instruction){s.services.instruction(0x'+format(pc,'x')+'u);}'
        literal='s.instruction_literal(0x'+format(pc,'x')+'u);'
        candidates=[needle for needle in(raw,literal)if body.count(needle)==1]
        if len(candidates)!=1:raise ValueError('Codec rotation interpolation continuation site drift')
        body=body.replace(candidates[0],label+candidates[0],1)
    at=body.index('{')+1;prefix=PREPARED_PREFIX if 's.instruction_literal('in body else RAW_PREFIX;body=body[:at]+prefix+body[at:]
    result=source[:match.start()]+body+source[match.end():]
    if result.count(ANCHOR)!=1:raise ValueError('Codec rotation interpolation owning include anchor drift')
    at=result.index(ANCHOR)+len(ANCHOR)
    return result[:at]+INCLUDE+result[at:]
def remove_codec_rotation_interpolation(source):
    _,match,body=authenticate(source)
    if match[0]==body:return source
    return (source[:match.start()]+body+source[match.end():]).replace(INCLUDE,'',1)
def installed_entries(source):
    _,match,body=authenticate(source)
    if match[0]==body or sum(match[0].count(prefix)for prefix in(RAW_PREFIX,PREPARED_PREFIX))!=1:raise ValueError('Codec rotation interpolation native overlay missing')
    return ['296240']


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
