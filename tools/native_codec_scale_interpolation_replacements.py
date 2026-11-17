"""Private exact296620 source overlay and authenticated continuation labels."""
import importlib.util,re
from pathlib import Path
def sibling(name):
    spec=importlib.util.spec_from_file_location('_codec_scale_interpolation_'+name,Path(__file__).with_name(name+'.py'))
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);return module
inputs=sibling('native_model_marker_lookup_replacements')
BODY=re.compile(r'static void run_296620\(WorldState &s\) \{.*?\n\}',re.S)
INCLUDE='#include "physics/runtime/native_tu15_codec_scale_interpolation_source_owner.hpp"\n'
ANCHOR='namespace halo2 {\nnamespace {\nusing namespace physics_world_runtime_detail;\n'
SITES=(0x296629,0x29662c,0x296633,0x296636,0x29663e,0x296641,0x296647,0x29664b,0x29664e,0x296654,0x296659,0x29665c,0x296661,0x29666d,0x296676)
LABELS={pc:'native_codec_scale_interpolation_resume_'+format(pc,'x')+': 'for pc in SITES}
OLD_PREFIX='\n    switch(halo2::native_codec_scale_interpolation_owner_detail::bridge(s)){case 0u:break;case 1u:return;'+''.join('case 0x'+format(pc,'x')+'u:goto '+LABELS[pc][:-2]+';'for pc in SITES)+'default:throw std::logic_error("Unsupported native codec scale interpolation continuation");}'
RAW_PREFIX=OLD_PREFIX.replace('::bridge(s)', '::bridge<false>(s)')
PREPARED_PREFIX=OLD_PREFIX.replace('::bridge(s)', '::bridge<true>(s)')
PREFIX=RAW_PREFIX

UNITS=('src/rendering/models/source_model_render_tu15_generated.cpp',)
# Complete contexts from genuine ModelRender current/fresh raw/prepared sources.
CONTEXTS={
    ('d5d4a860ce014588a725c310772d33afdb3115f83faaa8e6b87f5d6f761415fd', '57359239fb680e03adea12fc0ce9e509fad6bef6ce05f9c03a9a37e7012d16db', 'f928af6956a3965aef767189095b9732744dcbdb209a345a3aec57baa618e0d1', '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575'): 0,
    ('16f18851eeaeb4909fb26386ad09f13823e4df034ccb255c3d851b9b206c0734', '57359239fb680e03adea12fc0ce9e509fad6bef6ce05f9c03a9a37e7012d16db', 'c112d857c78bf307a089c1297c9b000870c293149fd78edd9852012887dd439d', '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575'): 0,
    ('92e7ada884070a8bec9a8fd6bb582510b3f70a147ef2f21ee7f039c089ea40f8', '57359239fb680e03adea12fc0ce9e509fad6bef6ce05f9c03a9a37e7012d16db', '4af1a5ed426380ed605d661066155bd625fd9f3a1add625cedf00fdc17f15604', '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575'): 0,
    ('16f18851eeaeb4909fb26386ad09f13823e4df034ccb255c3d851b9b206c0734', '57359239fb680e03adea12fc0ce9e509fad6bef6ce05f9c03a9a37e7012d16db', '4af1a5ed426380ed605d661066155bd625fd9f3a1add625cedf00fdc17f15604', '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575'): 0,
}
ORDERED={}
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
        if result.count(label)!=1:raise ValueError('Incomplete codec scale interpolation continuation labels')
        raw='if(s.services.instruction){s.services.instruction(0x'+format(pc,'x')+'u);}'
        literal='s.instruction_literal(0x'+format(pc,'x')+'u);'
        if sum(result.count(label+callback)for callback in(raw,literal))!=1:raise ValueError('Relocated codec scale interpolation continuation label')
        result=result.replace(label,'',1)
    for old,new in ORDERED.items():
        if result.count(new)!=1 or old in result:raise ValueError('Incomplete ordered interpolation fallback')
        result=result.replace(new,old,1)
    literal='s.instruction_literal('in result
    if not allow_retag and prefix!=(PREPARED_PREFIX if literal else RAW_PREFIX):raise ValueError('Interpolation source-format tag mismatch')
    return result
def authenticate(source):
    bodies=list(BODY.finditer(source))
    if len(bodies)!=1:raise ValueError('Missing or ambiguous complete codec scale interpolation owner')
    body=canonical_body(bodies[0][0])
    resolver=re.findall(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)
    dispatcher=re.findall(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)
    state=re.findall(r'struct LiveState : physics_world_runtime_detail::WorldState \{.*?\n\};\nusing WorldState=LiveState;',source,re.S)
    if not (len(resolver)==len(dispatcher)==len(state)==1):raise ValueError('Incomplete codec scale interpolation producer context')
    context=tuple(map(inputs.digest,(body,resolver[0],dispatcher[0],state[0])))
    if context not in CONTEXTS:raise ValueError('Complete codec scale interpolation body/resolver/dispatcher/class drift')
    installed=bodies[0][0]!=body
    if source.count(INCLUDE)!=(1 if installed else 0):raise ValueError('Codec scale interpolation include/prefix ownership drift')
    if installed and source.count(INCLUDE+ANCHOR)!=1:raise ValueError('Codec scale interpolation include placement drift')
    if any(source.count(label)!=int(label in bodies[0][0]) for label in LABELS.values()):raise ValueError('Foreign scale interpolation continuation label outside its owner')
    for prefix in(RAW_PREFIX,PREPARED_PREFIX,OLD_PREFIX):
        if source.count(prefix)!=int(prefix in bodies[0][0]):raise ValueError('Foreign interpolation prefix outside its owner')
    return CONTEXTS[context],bodies[0],body
def replace_codec_scale_interpolation(source):
    if not BODY.search(source):
        if any(prefix in source for prefix in(RAW_PREFIX,PREPARED_PREFIX,OLD_PREFIX))or INCLUDE in source:raise ValueError('Codec scale interpolation overlay lacks its owner')
        return source
    owner,match,body=authenticate(source)
    if match[0]!=body:return source
    body=ordered_body(body)
    for pc,label in LABELS.items():
        raw='if(s.services.instruction){s.services.instruction(0x'+format(pc,'x')+'u);}'
        literal='s.instruction_literal(0x'+format(pc,'x')+'u);'
        candidates=[needle for needle in(raw,literal)if body.count(needle)==1]
        if len(candidates)!=1:raise ValueError('Codec scale interpolation continuation site drift')
        body=body.replace(candidates[0],label+candidates[0],1)
    at=body.index('{')+1;prefix=PREPARED_PREFIX if 's.instruction_literal('in body else RAW_PREFIX;body=body[:at]+prefix+body[at:]
    result=source[:match.start()]+body+source[match.end():]
    if result.count(ANCHOR)!=1:raise ValueError('Codec scale interpolation owning include anchor drift')
    # Append to the complete global preamble. Older owners keep every exact
    # prepend/after-anchor include relation when independently reconstructed.
    at=result.index(ANCHOR)
    return result[:at]+INCLUDE+result[at:]
def remove_codec_scale_interpolation(source):
    _,match,body=authenticate(source)
    if match[0]==body:return source
    return (source[:match.start()]+body+source[match.end():]).replace(INCLUDE,'',1)
def installed_entries(source):
    _,match,body=authenticate(source)
    if match[0]==body or sum(match[0].count(prefix)for prefix in(RAW_PREFIX,PREPARED_PREFIX))!=1:raise ValueError('Codec scale interpolation native overlay missing')
    return ['296620']


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
