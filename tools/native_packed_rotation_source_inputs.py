"""Exact complete source owners for the private packed-rotation continuation proof."""
import hashlib,importlib.util,re
from pathlib import Path
def sibling(name):
    spec=importlib.util.spec_from_file_location('_packed_rotation_inputs_'+name,Path(__file__).with_name(name+'.py'))
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);return module
function=sibling('native_model_marker_lookup_replacements').function
PREFIX='\n    if(halo2::native_packed_rotation_owner_detail::bridge(s))return;'
INCLUDE='#include "physics/runtime/native_tu15_packed_rotation_source_owner.hpp"\n'
UNITS=('src/physics/physics_live_support_tu15_generated.cpp','src/campaign/ai/source_ai_frame_tu15_generated.cpp','src/rendering/models/source_model_render_tu15_generated.cpp')
CONTEXTS=(
('de153a78bb522a3785bb331d7009686050e542b33dc98785fec94f027e1b85d5','1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4','5901d2c9c67421f4dabe988f833de38ce4a22dd4cb0778fbf19eb0fd0cb55ce0','086a7e9e0eaa7ac687afb8b8094118d1211250698a0911a0beac4447b116ec46'),
('72828156a606211cefd50d955ae75d8d6fe328faace58b865a88cf1ffe1b4009','5eab69b03c4be4556290b5ba3e7765897c12d0064ea5b20267317b365ea4a657','1cf61f52187c986d02510a40d28a466d95589b66af5521ade12ae9640085e28b','457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575'),
('72828156a606211cefd50d955ae75d8d6fe328faace58b865a88cf1ffe1b4009','57359239fb680e03adea12fc0ce9e509fad6bef6ce05f9c03a9a37e7012d16db','f928af6956a3965aef767189095b9732744dcbdb209a345a3aec57baa618e0d1','457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575'))
# Genuine full generation and exact target/fallback-map comparison retained in
# native-packed-rotation-fresh-owner-audit-20261110. No partial resolver match.
EXTRA_CONTEXTS={
('de153a78bb522a3785bb331d7009686050e542b33dc98785fec94f027e1b85d5','1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4','aba69140bdfbfe48111ef3727b7bb22a4891a8daa06770aabcb75183386d5849','457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575'):0,
('de153a78bb522a3785bb331d7009686050e542b33dc98785fec94f027e1b85d5','b366b77691cb314f6c6d0f8f20e71c1bda7114225c8d7b1f4043ee80e12354b8','974fddd715271a3ccdc93c266782b8f84d13f514bd11653b34af22a1f8be267a','457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575'):1,
('de153a78bb522a3785bb331d7009686050e542b33dc98785fec94f027e1b85d5','57359239fb680e03adea12fc0ce9e509fad6bef6ce05f9c03a9a37e7012d16db','4af1a5ed426380ed605d661066155bd625fd9f3a1add625cedf00fdc17f15604','457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575'):2,
}
HELPERS={'native_read64':'5365e17a8ead608ed6628a60f2943303f8e10222a5162aadc6561e6e0f00bdc0','native_convert_mmx_dwords':'45a70982e8df998a32a98c01a9776ca93ec11d388df97db0a7a82d82948b8ae0','source_estimate':'bb50fdcd1d22250d871032ff227a462ef42c30981de20b9f23137bb1c60efd89','native_estimate_vector':'a5e2ad3c7a078be7decea72984e8efc82b03f966b46aea97f1cc89e4c48d6aa4'}
def digest(text):return hashlib.sha256(text.encode()).hexdigest()
def canonical_body(body):
    opening=body.index('{')+1
    return body[:opening]+body[opening+len(PREFIX):]if body[opening:].startswith(PREFIX)else body
def authenticate(source):
    if source.count('static void run_296180(WorldState &s) {')!=1:raise ValueError('Missing or ambiguous complete packed rotation owner')
    current=function(source,'run_296180');body=canonical_body(current)
    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)
    dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)
    state=re.search(r'struct LiveState : physics_world_runtime_detail::WorldState \{.*?\n\};\nusing WorldState=LiveState;',source,re.S)
    if not resolver or not dispatcher or not state:raise ValueError('Incomplete packed rotation owning context')
    context=tuple(map(digest,(body,resolver[0],dispatcher[0],state[0])))
    if context in CONTEXTS:index=CONTEXTS.index(context)
    elif context in EXTRA_CONTEXTS:index=EXTRA_CONTEXTS[context]
    else:raise ValueError('Complete packed rotation source context drift '+repr(context))
    if index==0 and context[2]=='aba69140bdfbfe48111ef3727b7bb22a4891a8daa06770aabcb75183386d5849':
        case='case 0xfa5d0u:run_fa5d0(s);return;';halfword=sibling('native_halfword_bit_query_replacements')
        if dispatcher[0].count(case)!=1 or digest(dispatcher[0].replace(case,'',1))!=CONTEXTS[0][2] or digest(halfword.canonical_body(function(source,'run_fa5d0')))!=halfword.PIN:raise ValueError('Exact fresh packed rotation FA dispatcher/body drift')
    if (current!=body and source.count(INCLUDE)!=1)or(current==body and INCLUDE in source):raise ValueError('Packed rotation prefix/include ownership drift')
    if 'case 0x296180u:return 0x296180u;'not in resolver[0]:raise ValueError('Packed rotation actual identity changed')
    mapping={int(a,16):int(b,16)for a,b in re.findall(r'case 0x([0-9a-f]+)u:return 0x([0-9a-f]+)u;',resolver[0])}
    for operand in re.findall(r'selected\(0x([0-9a-f]+)u,s.image\)',body):
        if mapping.get(int(operand,16))!=int(operand,16):raise ValueError('Packed rotation has nonidentity selected data')
    helpers=[]
    for helper,pin in HELPERS.items():
        value=function(source,helper)
        if digest(value)!=pin:raise ValueError('Complete packed rotation helper drift '+helper)
        helpers.append(value)
    return index,dict(current=current,body=body,resolver=resolver[0],dispatcher=dispatcher[0],state=state[0],helpers=helpers)
def prepare(root,scratch):
    rows=[];pieces=[]
    for index,name in enumerate(UNITS):
        source=(root/name).read_text();owner,parts=authenticate(source)
        if owner!=index:raise ValueError('Packed rotation file has a different owning context '+name)
        pieces.append('namespace packed_rotation_source'+str(index)+' {using namespace halo2;using namespace halo2::physics_world_runtime_detail;\n'+parts['state']+'\n'+parts['resolver']+'\n'+'\n'.join(parts['helpers'])+'\n'+parts['body']+'\n}\n')
        rows.append(dict(source=name,body_sha256=digest(parts['body']),resolver_sha256=digest(parts['resolver']),dispatcher_sha256=digest(parts['dispatcher']),state_sha256=digest(parts['state'])))
    (scratch/'packed_rotation_sources.inc').write_text('\n'.join(pieces))
    return rows
