#!/usr/bin/env python3
"""Select authenticated image PCs for physics-world trace and store observations.

The paired-image runtime retains canonical entry IDs and required-owner call
sites. Only observer PCs change. All generated arithmetic, memory addresses,
caller frames, branches and independent precision fixes remain byte-identical.
"""
import argparse
import hashlib
import json
import re
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT, Xbe, _Code, _trace, PIN
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN, full_tokens

BASELINE_SHA256='5fd09c3ef84366002472c4c1fe170c847c86af4072e4765c78ee3b2ded263840'
BEGIN='// BEGIN authenticated physics world observation addresses\n'
END='// END authenticated physics world observation addresses\n'
HELPER=BEGIN+'''// Preserve the existing canonical-observer API. Live TU1.5 composition opts
// into selected-image observations without changing any public caller ABI.
struct ObservedWorldState : physics_world_runtime_detail::WorldState {
    using physics_world_runtime_detail::WorldState::WorldState;
    bool selected_observation_pcs{};
};
std::uint32_t world_observation_pc(std::uint32_t disc,std::uint32_t tu15,
    const ObservedWorldState &state) {
    return state.selected_observation_pcs && state.image==SourcePhysicsContactImage::TitleUpdate15?tu15:disc;
}
'''+END
WRAPPER_BEGIN='// BEGIN physics world observation API wrappers\n'
WRAPPER_END='// END physics world observation API wrappers\n'
WRAPPERS=WRAPPER_BEGIN+'''void source_run_physics_world_runtime(M &m,std::uint32_t entry,
    SourcePhysicsContactRuntimeAbi &abi,SourcePhysicsContactImage image,FP fp,
    const SourcePhysicsContactRuntimeServices &services,const O &observe) {
    source_run_physics_world_runtime_impl(m,entry,abi,image,fp,services,observe,false);
}
void source_run_physics_world_runtime_selected_observations(M &m,std::uint32_t entry,
    SourcePhysicsContactRuntimeAbi &abi,SourcePhysicsContactImage image,FP fp,
    const SourcePhysicsContactRuntimeServices &services,const O &observe) {
    source_run_physics_world_runtime_impl(m,entry,abi,image,fp,services,observe,true);
}
'''+WRAPPER_END
OBSERVATION=re.compile(r'(s\.services\.instruction\(|(?:w|vw)\(s\.memory,s\.observe,|s\.push\()0x([0-9a-f]+)u')

BODY_PINS={
    'bbe70':'80d96bc8881a68838cac82a56b87434543bf8df1869ddcfe8eb30bd3952951a4',
    '309d40':'e883718d5683a19011a39a503a33c34f461ff4d3e260fe9edc09f6a1978d50d4',
    '30a370':'107ca7b0a5252b655e224368eaeb9d4f29f130fc76cdd241b1a78c3acc51b918',
    '30ae40':'59aecf5b620e94486db8c8d2f6c2e0ae3ba4f9f7af7e3ec4da2b75d0246076a2',
    '326820':'5fd3807e832daa37c6c883e25ae100102e201e0968a79524dd02318cda74a4ef',
}
OWNER_PINS={
    r'std::uint32_t selected\(.*?\}\}':'20de37b96fc56e19b3ec67f5a2e7aedebe84b001436654380330b4704430f0f2',
    r'std::uint32_t canonical_branch\(.*?\}\}':'e5779c8ed411532bc15334539a7814df2a584d81946b32d413bb5346d166e5c6',
    r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}':'c70c675fd3a1b25f4ebc895e487cfb699548752eab54d0e25566e120f6ea848a',
}
RENDER_HEADER='#include "rendering/models/native_tu15_render_object_ownership_bridge.hpp"\n'
SIMPLEX_HEADER='#include "physics/runtime/native_tu15_simplex_helpers_handoff_internal.hpp"\n'
CONTACT_HEADER='#include "physics/runtime/native_tu15_contact_features_handoff_internal.hpp"\n'
SCHEMA_HEADER='#include "physics/runtime/source_constraint_schema_emitter_native_tu15_internal.hpp"\n'
MODERN_HEADER='#include "physics/runtime/physics_world_runtime_internal.hpp"\n'
LEGACY_HEADER='#include "physics_world_runtime_internal.hpp"\n'
RENDER_PREFIX=('\n    if(s.image==SourcePhysicsContactImage::TitleUpdate15) {\n'
               '        native_tu15_render_object_ownership_bridge(s,0xbcfb0u);return;\n'
               '    }')
SCHEMA_PREFIX='\nif(try_native_constraint_schema_emitter_tu15(s))return;'
# Exact authored call tails survive observation-PC selection: return operands
# and required-owner call sites retain their canonical ABI domain.
TAIL_PATCHES=(
    ('309d40','selected(0x309feeu,s.image));run_309900(s);','selected(0x309feeu,s.image));if(!handoff_native_simplex_helpers(s,0x30d3f0u,0x30dad9u))run_309900(s);',SIMPLEX_HEADER),
    ('30a370','selected(0x30a8cbu,s.image));run_309900(s);','selected(0x30a8cbu,s.image));if(!handoff_native_simplex_helpers(s,0x30d3f0u,0x30e3b6u))run_309900(s);',SIMPLEX_HEADER),
    ('30ae40','dispatch(s,target,0x30ae7du,false);','if(!handoff_native_contact_features(s,target,0x30e96du))dispatch(s,target,0x30ae7du,false);',CONTACT_HEADER),
    ('30ae40','dispatch(s,target,0x30ae95u,false);','if(!handoff_native_contact_features(s,target,0x30e985u))dispatch(s,target,0x30ae95u,false);',CONTACT_HEADER),
    ('30ae40','selected(0x30afe1u,s.image));run_30eb30(s);','selected(0x30afe1u,s.image));if(!handoff_native_simplex_helpers(s,0x312620u,0x30eaccu))run_30eb30(s);',SIMPLEX_HEADER),
    ('30ae40','selected(0x30b09au,s.image));run_309900(s);','selected(0x30b09au,s.image));if(!handoff_native_simplex_helpers(s,0x30d3f0u,0x30eb85u))run_309900(s);',SIMPLEX_HEADER),
    ('30ae40','selected(0x30b1f7u,s.image));run_309900(s);','selected(0x30b1f7u,s.image));if(!handoff_native_simplex_helpers(s,0x30d3f0u,0x30ece2u))run_309900(s);',SIMPLEX_HEADER),
)

def body_match(source,entry,state='WorldState'):
    matches=list(re.finditer(r'static void run_'+entry+r'\('+state+r' &s\) \{.*?\n\}',source,re.S))
    if len(matches)!=1:raise ValueError('Expected one complete world overlay body '+entry)
    return matches[0]

def strip_native_overlays(source):
    """Reverse only exact independent edits before checking the old baseline."""
    for pattern,pin in OWNER_PINS.items():
        match=re.search(pattern,source,re.S)
        if not match or hashlib.sha256(match[0].encode()).hexdigest()!=pin:
            raise ValueError('Complete physics-world overlay resolver/dispatcher drift')
    state={'renderer':False,'schema':False,'tails':[],'headers':[],'modern_header':MODERN_HEADER in source}
    for entry,pin in BODY_PINS.items():
        match=body_match(source,entry);current=match[0];original=current
        for key,prefix,owner in (('renderer',RENDER_PREFIX,'bbe70'),('schema',SCHEMA_PREFIX,'326820')):
            if entry==owner:
                start=original.index('{')+1
                if original[start:].startswith(prefix):
                    original=original[:start]+original[start+len(prefix):];state[key]=True
        for index,(owner,old,new,header)in enumerate(TAIL_PATCHES):
            if owner==entry and new in original:
                if original.count(new)!=1:raise ValueError('Duplicated physics-world native handoff')
                original=original.replace(new,old,1);state['tails'].append(index)
        if hashlib.sha256(original.encode()).hexdigest()!=pin:
            raise ValueError('Complete physics-world independent overlay body drift '+entry)
        source=source[:match.start()]+original+source[match.end():]
    for header in(RENDER_HEADER,SIMPLEX_HEADER,CONTACT_HEADER,SCHEMA_HEADER):
        if source.count(header)>1:raise ValueError('Duplicate physics-world overlay include')
        if header in source:state['headers'].append(header);source=source.replace(header,'',1)
    for enabled,header in ((state['renderer'],RENDER_HEADER),(state['schema'],SCHEMA_HEADER)):
        if enabled and header not in state['headers']:raise ValueError('Native world overlay lacks its owning header')
    for index in state['tails']:
        if TAIL_PATCHES[index][3]not in state['headers']:raise ValueError('Native world handoff lacks its owning header')
    if source.count(MODERN_HEADER)+source.count(LEGACY_HEADER)!=1:raise ValueError('Unknown world runtime header placement')
    return source.replace(MODERN_HEADER,LEGACY_HEADER),state

def restore_native_overlays(source,state):
    """Preserve the exact installed native routes after choosing observations."""
    for index in state['tails']:
        entry,old,new,_=TAIL_PATCHES[index];match=body_match(source,entry,'ObservedWorldState')
        if match[0].count(old)!=1:raise ValueError('World native handoff restoration site drift')
        source=source[:match.start()]+match[0].replace(old,new,1)+source[match.end():]
    if state['schema']:
        match=body_match(source,'326820','ObservedWorldState');start=match[0].index('{')+1
        source=source[:match.start()]+match[0][:start]+SCHEMA_PREFIX+match[0][start:]+source[match.end():]
    if state['modern_header']:source=source.replace(LEGACY_HEADER,MODERN_HEADER,1)
    anchor=MODERN_HEADER if state['modern_header']else LEGACY_HEADER
    headers=''.join(header for header in(SIMPLEX_HEADER,CONTACT_HEADER,SCHEMA_HEADER)if header in state['headers'])
    source=source.replace(anchor,anchor+headers,1)
    if state['renderer']:
        from renderer_native_replacements import replace_render_root_alias_body
        source=replace_render_root_alias_body(source)
    elif RENDER_HEADER in state['headers']:source=RENDER_HEADER+source
    return source

def sha(path):
    with path.open('rb') as source:return hashlib.file_digest(source,'sha256').hexdigest()

def legacy_quaternion_layout(source):
    """Keep the already repaired C1 body while matching its original placement.

    The original one-body repair inserted its helper after the 'static '
    token. The current generator emits that identical helper in its preamble.
    Both declarations live in the anonymous namespace; only exact layout is
    normalized, with the entire repaired body and helper separately pinned.
    """
    marker='static void run_378ab0(WorldState &s) {'
    if marker not in source:return source,False
    from fix_map_quaternion_spills_tu15 import HELPER as spill_helper
    if hashlib.sha256(spill_helper.encode()).hexdigest()!='d96ac98746041f60c97b8b203d4fd7277486d13e8cf084eb54c8b5f021f2c9b8':
        raise ValueError('Independent quaternion spill helper changed')
    if source.count(spill_helper)!=1:raise ValueError('Expected one exact quaternion spill helper')
    match=body_match(source,'378ab0');body=match[0][len('static '):]
    if hashlib.sha256(body.encode()).hexdigest()!='e3c8167a4d7bf02a80c6f5bf895ea9fa5ff81b3039bee546caeccc2af5ca9cfa':
        raise ValueError('Complete repaired quaternion body changed')
    source=source.replace(spill_helper,'',1)
    match=body_match(source,'378ab0')
    return source[:match.start()]+'static '+spill_helper+body+source[match.end():],True

def observation_baseline(source):
    from native_polygon_features_replacements import remove_polygon_features,replace_polygon_features,PREFIX as FEATURES_PREFIX,INCLUDE as FEATURES_INCLUDE
    if FEATURES_PREFIX in source or FEATURES_INCLUDE in source:
        if replace_polygon_features(source)!=source:
            raise ValueError('Incomplete native polygon-feature observation owner')
        source=remove_polygon_features(source)
    # Reverse the typed support owner while its exact observation type and
    # complete selected dispatcher remain available for authentication.
    from native_polygon_support_replacements import remove_polygon_support,replace_polygon_support,PREFIX as POLYGON_PREFIX,INCLUDE as POLYGON_INCLUDE
    if POLYGON_PREFIX in source or POLYGON_INCLUDE in source:
        if replace_polygon_support(source)!=source:
            raise ValueError('Incomplete native polygon-support observation owner')
        source=remove_polygon_support(source)
    # MOPP's exact body/shift/include reversal must see ObservedWorldState.
    from native_mopp_aabb_replacements import remove_mopp_aabb,replace_mopp_aabb,PREFIX as MOPP_PREFIX
    if MOPP_PREFIX in source and replace_mopp_aabb(source)!=source:
        raise ValueError('Incomplete native MOPP observation owner')
    source=remove_mopp_aabb(source)
    from native_weapon_effect_datum_bridge_tu15 import remove_native_effect_datum
    source=remove_native_effect_datum(source)
    source=re.sub(re.escape(BEGIN)+r'.*?'+re.escape(END),'',source,flags=re.S)
    source=re.sub(re.escape(WRAPPER_BEGIN)+r'.*?'+re.escape(WRAPPER_END),'',source,flags=re.S)
    source=source.replace('static void source_run_physics_world_runtime_impl(', 'void source_run_physics_world_runtime(')
    source=source.replace('const O &observe,bool selected_observation_pcs)', 'const O &observe)')
    source=source.replace('s.load(abi);s.selected_observation_pcs=selected_observation_pcs;', 's.load(abi);')
    source=source.replace('ObservedWorldState','WorldState')
    source=re.sub(r'world_observation_pc\(0x([0-9a-f]+)u,0x[0-9a-f]+u,s\)',r'0x\1u',source)
    source,quaternion_relocated=legacy_quaternion_layout(source)
    source,state=strip_native_overlays(source)
    state['quaternion_helper_layout_normalized']=quaternion_relocated
    if hashlib.sha256(source.encode()).hexdigest()!=BASELINE_SHA256:
        raise ValueError('Physics source changed; retain and re-audit its independent edits')
    return source,state

def canonical(source):
    return observation_baseline(source)[0]

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source',type=Path,default=ROOT/'src/physics/runtime/physics_world_runtime_generated.cpp')
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--manifest',type=Path,required=True)
    args=parser.parse_args();source,overlays=observation_baseline(args.source.read_text())
    assert hashlib.sha256(source.encode()).hexdigest()==BASELINE_SHA256,'Physics source changed; retain and re-audit its independent edits'
    old_path=ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'
    assert sha(old_path)==PIN and sha(NEW_DEFAULT)==NEW_PIN
    old,new=Xbe(old_path),Xbe(NEW_DEFAULT);oc,nc=_Code(old,'.text'),_Code(new,'.text')
    selected=re.search(r'std::uint32_t selected\(.*?\n',source).group()
    addresses={int(p,16):int(q,16) for p,q in re.findall(r'case 0x([0-9a-f]+)u:return 0x([0-9a-f]+)u;',selected)}
    bodies={int(p,16):body for p,body in re.findall(r'(?:static )?void run_([0-9a-f]+)\(WorldState &s\) \{\n(.*?)\n\}',source,re.S)}
    declared={int(p,16) for p in re.findall(r'static void run_([0-9a-f]+)\(WorldState &\);',source)}
    assert set(bodies)==declared and len(bodies)==477
    targets={addresses[p] for p in bodies};mapping={}
    for entry,body in bodies.items():
        sites=sorted({int(p,16) for p in re.findall(r's\.services\.instruction\(0x([0-9a-f]+)u\)',body)})
        assert sites==sorted(_trace(oc,entry,set(bodies)).code),hex(entry)
        actual=sorted(_trace(nc,addresses[entry],targets).code)
        assert len(sites)==len(actual) and full_tokens([oc.at(p) for p in sites],old)==full_tokens([nc.at(p) for p in actual],new),hex(entry)
        for p,q in zip(sites,actual):
            assert p not in mapping or mapping[p]==q
            mapping[p]=q
    count=0
    def replace(match):
        nonlocal count
        p=int(match[2],16);q=mapping[p];count+=1
        return match[1]+f'world_observation_pc(0x{p:x}u,0x{q:x}u,s)'
    output=OBSERVATION.sub(replace,source)
    output=re.sub(r'\bWorldState\b','ObservedWorldState',output)
    anchor='using namespace physics_world_runtime_detail;\n'
    assert output.count(anchor)==1
    output=output.replace(anchor,anchor+HELPER)
    output=output.replace('void source_run_physics_world_runtime(', 'static void source_run_physics_world_runtime_impl(')
    output=output.replace('const O &observe) {', 'const O &observe,bool selected_observation_pcs) {')
    output=output.replace('s.load(abi);', 's.load(abi);s.selected_observation_pcs=selected_observation_pcs;')
    output=output.replace('} // namespace halo2\n',WRAPPERS+'} // namespace halo2\n')
    output=restore_native_overlays(output,overlays)
    # Native bridge headers use the repository's current physical layout.
    # This exact include relocation is reversed by baseline authentication.
    output=output.replace(LEGACY_HEADER,MODERN_HEADER,1)
    from native_weapon_effect_datum_bridge_tu15 import install_native_effect_datum
    output=install_native_effect_datum(output,mode='observed')
    from native_mopp_aabb_replacements import replace_mopp_aabb
    output=replace_mopp_aabb(output)
    from native_polygon_support_replacements import replace_polygon_support
    output=replace_polygon_support(output)
    from native_polygon_features_replacements import replace_polygon_features
    output=replace_polygon_features(output)
    assert canonical(output)==source
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(output)
    report=dict(scope=__doc__,source_sha256=sha(args.source),canonical_baseline_sha256=BASELINE_SHA256,
        generated_cpp_sha256=sha(args.output),tool_sha256=sha(Path(__file__)),original_map_pack_sha256=PIN,
        original_tu15_sha256=NEW_PIN,whole_routines_authenticated=len(bodies),static_instruction_sites=len(mapping),
        observations_selected=count,original_fallback_arithmetic_and_owner_call_ABI_byte_identical=False,
        original_legacy_baseline_byte_identical_after_exact_overlay_reversal=True,
        original_owner_call_ABI_operands_preserved=True,
        qualified_fallback_primitive_overlays=['MOPP ordered shifts','TU1.5 polygon ordered shifts/SSE/whole-vector store'],
        native_datum_allocator=dict(canonical_entry=0x16d2c0,tu15_entry=0x16e6a0,whole_raw_ABI=True,canonical_observation_API_preserved=True),
        native_polygon_support=dict(canonical_entry=0x1f1d20,tu15_entry=0x1f6610,source_prefix_preserved=True),
        native_polygon_features=dict(canonical_entry=0x1f1da0,tu15_entry=0x1f6690,source_prefix_preserved=True),
        canonical_observation_API_preserved=True,selected_image_observation_API_explicit=True,
        preserved_native_overlays=overlays,unchanged_whole_legacy_baseline_authenticated=True,
        instruction_map={f'{p:08x}':q for p,q in sorted(mapping.items())},full_weapon_parity=False)
    args.manifest.parent.mkdir(parents=True,exist_ok=True);args.manifest.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps({k:v for k,v in report.items() if k not in ('instruction_map','scope')}))

if __name__=='__main__':main()
