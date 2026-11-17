"""Private exact296220 source overlay and authenticated continuation labels."""
import importlib.util,re
from pathlib import Path
def sibling(name):
    spec=importlib.util.spec_from_file_location('_codec_scale_'+name,Path(__file__).with_name(name+'.py'))
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);return module
inputs=sibling('native_model_marker_lookup_replacements')
BODY=re.compile(r'static void run_296220\(WorldState &s\) \{.*?\n\}',re.S)
INCLUDE='#include "physics/runtime/native_tu15_codec_scale_source_owner.hpp"\n'
ANCHOR='#include "physics/runtime/physics_world_runtime_internal.hpp"\n'
SITES=(0x296225,0x296228,0x29622e,0x296234,0x296239,0x29623c)
LABELS={pc:'native_codec_scale_resume_'+format(pc,'x')+': 'for pc in SITES}
PREFIX='\n    switch(halo2::native_codec_scale_owner_detail::bridge(s)){case 0u:break;case 1u:return;'+''.join('case 0x'+format(pc,'x')+'u:goto '+LABELS[pc][:-2]+';'for pc in SITES)+'default:throw std::logic_error("Unsupported native codec scale continuation");}'
UNITS=('src/rendering/models/source_model_render_tu15_generated.cpp','src/gameplay/objects/source_sandbox_special_tu15_generated.cpp')
# Fresh full-owner contexts are independently qualified by native-codec-scale-full-owner-audit-20261110.
CONTEXTS={('a3c15470d8dbf7c4ac2ed125a9e2b8a8d4985eb2cba66fbab062ab6900912ddb', '57359239fb680e03adea12fc0ce9e509fad6bef6ce05f9c03a9a37e7012d16db', 'f928af6956a3965aef767189095b9732744dcbdb209a345a3aec57baa618e0d1', '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575'): 0, ('b29389a475b626f4a74666c2788a945885b01aa6d9b828a8952258a645f79412', '57359239fb680e03adea12fc0ce9e509fad6bef6ce05f9c03a9a37e7012d16db', 'c112d857c78bf307a089c1297c9b000870c293149fd78edd9852012887dd439d', '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575'): 0, ('a3c15470d8dbf7c4ac2ed125a9e2b8a8d4985eb2cba66fbab062ab6900912ddb', '0fddb23fd6f15344aa915c1c50b845847dd34460782d49467474e9e1cfed5fed', '961134517848136bf248393b533a030ae2607735d71fec8f570e0332e5ce4bc6', '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575'): 1, ('b29389a475b626f4a74666c2788a945885b01aa6d9b828a8952258a645f79412', '0fddb23fd6f15344aa915c1c50b845847dd34460782d49467474e9e1cfed5fed', 'dffd3beb5abbf920720b8ab5d8cb462f41bd953860b2eb02c0d072a28a65f2da', '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575'): 1, ('64189eca3c5ec6f3b0b80f4062822d7db15b3a4631a854bd656b5b111f1e4d83', '57359239fb680e03adea12fc0ce9e509fad6bef6ce05f9c03a9a37e7012d16db', '4af1a5ed426380ed605d661066155bd625fd9f3a1add625cedf00fdc17f15604', '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575'): 0, ('b29389a475b626f4a74666c2788a945885b01aa6d9b828a8952258a645f79412', '57359239fb680e03adea12fc0ce9e509fad6bef6ce05f9c03a9a37e7012d16db', '4af1a5ed426380ed605d661066155bd625fd9f3a1add625cedf00fdc17f15604', '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575'): 0, ('64189eca3c5ec6f3b0b80f4062822d7db15b3a4631a854bd656b5b111f1e4d83', '0fddb23fd6f15344aa915c1c50b845847dd34460782d49467474e9e1cfed5fed', 'd46a1635c618577534bfdd091f45ecea819b4886662ca1309400154271d5a987', '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575'): 1, ('b29389a475b626f4a74666c2788a945885b01aa6d9b828a8952258a645f79412', '0fddb23fd6f15344aa915c1c50b845847dd34460782d49467474e9e1cfed5fed', 'd46a1635c618577534bfdd091f45ecea819b4886662ca1309400154271d5a987', '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575'): 1}
def canonical_body(body):
    at=body.index('{')+1
    if not body[at:].startswith(PREFIX):return body
    result=body[:at]+body[at+len(PREFIX):]
    for pc,label in LABELS.items():
        if result.count(label)!=1:raise ValueError('Incomplete codec scale continuation labels')
        raw='if(s.services.instruction){s.services.instruction(0x'+format(pc,'x')+'u);}'
        literal='s.instruction_literal(0x'+format(pc,'x')+'u);'
        if sum(result.count(label+callback)for callback in(raw,literal))!=1:raise ValueError('Relocated codec scale continuation label')
        result=result.replace(label,'',1)
    return result
def authenticate(source):
    bodies=list(BODY.finditer(source))
    if len(bodies)!=1:raise ValueError('Missing or ambiguous complete codec scale owner')
    body=canonical_body(bodies[0][0])
    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)
    dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)
    state=re.search(r'struct LiveState : physics_world_runtime_detail::WorldState \{.*?\n\};\nusing WorldState=LiveState;',source,re.S)
    if not resolver or not dispatcher or not state:raise ValueError('Incomplete codec scale producer context')
    context=tuple(map(inputs.digest,(body,resolver[0],dispatcher[0],state[0])))
    if context not in CONTEXTS:raise ValueError('Complete codec scale body/resolver/dispatcher/class drift')
    installed=bodies[0][0]!=body
    if source.count(INCLUDE)!=(1 if installed else 0):raise ValueError('Codec scale include/prefix ownership drift')
    return CONTEXTS[context],bodies[0],body
def replace_codec_scale(source,*,defer_sandbox=False):
    if not BODY.search(source):
        if PREFIX in source or INCLUDE in source:raise ValueError('Codec scale overlay lacks its owner')
        return source
    if defer_sandbox:
        # The complete owning wrapper restores its authenticated required-owner
        # boundary after common emission. Do not bind its raw dispatcher.
        helper=sibling('native_sandbox_dispatch_preservation')
        try:dispatcher=helper.dispatcher(source)[0]
        except ValueError:dispatcher=''
        if inputs.digest(dispatcher)==helper.RAW_PIN:
            authenticate(helper.preserve(source));return source
    owner,match,body=authenticate(source)
    if match[0]!=body:return source
    for pc,label in LABELS.items():
        raw='if(s.services.instruction){s.services.instruction(0x'+format(pc,'x')+'u);}'
        literal='s.instruction_literal(0x'+format(pc,'x')+'u);'
        candidates=[needle for needle in(raw,literal)if body.count(needle)==1]
        if len(candidates)!=1:raise ValueError('Codec scale continuation site drift')
        body=body.replace(candidates[0],label+candidates[0],1)
    at=body.index('{')+1;body=body[:at]+PREFIX+body[at:]
    result=source[:match.start()]+body+source[match.end():]
    if result.count(ANCHOR)!=1:raise ValueError('Codec scale owning include anchor drift')
    at=result.index(ANCHOR)+len(ANCHOR)
    return result[:at]+INCLUDE+result[at:]
def remove_codec_scale(source):
    _,match,body=authenticate(source)
    if match[0]==body:return source
    return (source[:match.start()]+body+source[match.end():]).replace(INCLUDE,'',1)
def installed_entries(source):
    _,match,body=authenticate(source)
    if match[0]==body or match[0].count(PREFIX)!=1:raise ValueError('Codec scale native overlay missing')
    return ['296220']

