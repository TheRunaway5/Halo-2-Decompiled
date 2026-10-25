#!/usr/bin/env python3
"""Install shared native lookup routes while preserving existing literal bodies.

Each route keeps its module's observer ABI and older-image behavior. Conversion
alone does not qualify the native replacement.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re

INCLUDE='''// BEGIN native damage model lookup include
#include "gameplay/objects/source_damage_model_native_tu15_internal.hpp"
// END native damage model lookup include
'''
ROUTE='''// BEGIN native TU1.5 damage model lookup
if(s.image==SourcePhysicsContactImage::TitleUpdate15) {
    damage_model_lookup_native_tu15(s,[&](std::uint32_t disc,std::uint32_t actual){
        if(s.services.instruction)s.services.instruction(damage_observation_pc(disc,actual,s));
    });
    return;
}
// END native TU1.5 damage model lookup
'''
GETTER='''// BEGIN native damage model lookup activity
SourceDamageModelNativeActivity source_damage_model_native_activity() {
    using namespace physics_world_runtime_detail;
    return {damage_model_native_calls.load(std::memory_order_relaxed),
        damage_model_null_object.load(std::memory_order_relaxed),
        damage_model_null_model.load(std::memory_order_relaxed),
        damage_model_no_information.load(std::memory_order_relaxed),
        damage_model_resolved.load(std::memory_order_relaxed)};
}
// END native damage model lookup activity
'''
ACTUAL_INCLUDE='''// BEGIN native actual-address model lookup include
#include "gameplay/objects/source_damage_model_native_tu15_internal.hpp"
// END native actual-address model lookup include
'''
ACTUAL_ROUTE='''// BEGIN native actual-address model lookup
if(s.image==SourcePhysicsContactImage::TitleUpdate15) {
    damage_model_lookup_native_tu15(s,[&](std::uint32_t,std::uint32_t actual){
        if(s.services.instruction)s.services.instruction(actual);
    });
    return;
}
// END native actual-address model lookup
'''
PAIRED_INCLUDE='''// BEGIN native paired canonical model lookup include
#include "gameplay/objects/source_damage_model_native_tu15_internal.hpp"
// END native paired canonical model lookup include
'''
PAIRED_ROUTE='''// BEGIN native paired canonical model lookup
if(s.image==SourcePhysicsContactImage::TitleUpdate15) {
    damage_model_lookup_native_tu15(s,[&](std::uint32_t canonical,std::uint32_t){
        if(s.services.instruction)s.services.instruction(canonical);
    });
    return;
}
// END native paired canonical model lookup
'''


def strip_native_model_lookup(source):
    for part in (INCLUDE,ROUTE,GETTER):
        if source.count(part)>1:raise ValueError('Duplicate native damage-model hook')
        source=source.replace(part,'')
    if re.search(r'// (?:BEGIN|END) native (?:TU1\.5 damage model lookup|damage model lookup)',source):
        raise ValueError('Changed native damage-model hook; retain and re-audit')
    return source


def install_native_model_lookup(source):
    source=strip_native_model_lookup(source)
    anchor='static void run_d7160(ObservedDamageState &s) {\n'
    if source.count(anchor)!=1:raise ValueError('Model-damage lookup owner changed')
    start=source.index(anchor);end=source.index('\n}',start)+2
    digest=hashlib.sha256(source[start:end].encode()).hexdigest()
    if digest!='609c93d237583a8b9d155ab0bf9195e36c47ca6fab35191ca92e385b66431519':
        raise ValueError('Original complete model-damage body changed')
    source=source.replace(anchor,anchor+ROUTE)
    include='#include "halo2/source_damage_delivery_runtime.hpp"\n'
    if source.count(include)!=1 or source.count('} // namespace halo2\n')!=1:
        raise ValueError('Damage module layout changed')
    # Preserve the neighboring material owner's include/getter placement when
    # independently regenerating this route in an already composed module.
    include_anchor='// END native material damage include\n'
    if include_anchor not in source:include_anchor=include
    source=source.replace(include_anchor,include_anchor+INCLUDE)
    getter_anchor='// BEGIN native material damage activity\n'
    if getter_anchor not in source:getter_anchor='} // namespace halo2\n'
    return source.replace(getter_anchor,GETTER+getter_anchor)


def strip_actual_model_lookup(source):
    for part in (ACTUAL_INCLUDE,ACTUAL_ROUTE):
        if source.count(part)>1:raise ValueError('Duplicate actual-address model lookup hook')
        source=source.replace(part,'')
    if '// BEGIN native actual-address model lookup' in source:
        raise ValueError('Changed actual-address model lookup hook; retain and re-audit')
    return source


def install_actual_model_lookup(source):
    source=strip_actual_model_lookup(source)
    anchor='static void run_d80a0(WorldState &s) {\n'
    if source.count(anchor)!=1:raise ValueError('Actual-address model lookup owner changed')
    start=source.index(anchor);end=source.index('\n}',start)+2
    if hashlib.sha256(source[start:end].encode()).hexdigest()!='66117f6f72f3727c9f601106e32b3b8f7690569c0c802bf2310e5a2766d08e9c':
        raise ValueError('Actual-address complete model lookup body changed')
    source=source.replace(anchor,anchor+ACTUAL_ROUTE)
    include='#include "physics/runtime/physics_world_runtime_internal.hpp"\n'
    if source.count(include)!=1:raise ValueError('Actual-address model lookup include changed')
    return source.replace(include,include+ACTUAL_INCLUDE)


def strip_paired_model_lookup(source):
    for part in (PAIRED_INCLUDE,PAIRED_ROUTE):
        if source.count(part)>1:raise ValueError('Duplicate paired model lookup hook')
        source=source.replace(part,'')
    if '// BEGIN native paired canonical model lookup' in source:
        raise ValueError('Changed paired model lookup hook; retain and re-audit')
    return source


def install_paired_model_lookup(source):
    source=strip_paired_model_lookup(source)
    anchor='static void run_d7160(WorldState &s) {\n'
    if source.count(anchor)!=1:raise ValueError('Paired canonical model lookup owner changed')
    start=source.index(anchor);end=source.index('\n}',start)+2
    if hashlib.sha256(source[start:end].encode()).hexdigest()!='4d544b8197986ab76191f84a546a162bc229fdeca68e3dc96be6e180bb801fdf':
        raise ValueError('Complete paired model lookup body changed')
    source=source.replace(anchor,anchor+PAIRED_ROUTE)
    include='#include "physics/runtime/physics_world_runtime_internal.hpp"\n'
    if source.count(include)!=1:raise ValueError('Paired model lookup include changed')
    return source.replace(include,include+PAIRED_INCLUDE)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--manifest',type=Path,required=True)
    parser.add_argument('--actual-support',action='store_true')
    parser.add_argument('--paired-canonical',action='store_true')
    args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Use tools/run_test.py')
    from oracle_smg_firing_1_1 import Xbe,_Code,_trace
    from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
    def sha(path):
        with path.open('rb')as stream:return hashlib.file_digest(stream,'sha256').hexdigest()
    if sha(NEW_DEFAULT)!=NEW_PIN:raise ValueError('Original TU1.5 changed')
    code=_Code(Xbe(NEW_DEFAULT),'.text');body=_trace(code,0xd80a0,{0xd80a0})
    if len(body.code)!=24 or body.tables:raise ValueError('Complete original lookup changed')
    if args.actual_support and args.paired_canonical:parser.error('Select one native route kind')
    install=install_paired_model_lookup if args.paired_canonical else install_actual_model_lookup if args.actual_support else install_native_model_lookup
    strip=strip_paired_model_lookup if args.paired_canonical else strip_actual_model_lookup if args.actual_support else strip_native_model_lookup
    before=args.source.read_text();output=install(before)
    if strip(output)!=strip(before):
        raise ValueError('Native conversion changed an unrelated original body')
    root=Path(__file__).resolve().parents[1]
    kernel=root/'src/gameplay/objects/source_damage_model_native_tu15_internal.hpp'
    actual={int(pc,16)for pc in re.findall(r'site\(0x[0-9a-f]+u,0x([0-9a-f]+)u\)',kernel.read_text())}
    if actual!=set(body.code):raise ValueError('Native lookup observation sites differ from the original owner')
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(output)
    report=dict(passed=True,scope='Native routing and unchanged existing literal bodies only; original execution qualification remains separate',
        original_sha256=NEW_PIN,entry='d80a0',complete_original_instructions=24,
        source_before_sha256=hashlib.sha256(before.encode()).hexdigest(),generated_cpp_sha256=sha(args.output),
        literal_source_sha256=hashlib.sha256(strip(before).encode()).hexdigest(),
        unchanged_all_existing_literal_bodies=True,actual_support=args.actual_support,
        paired_canonical=args.paired_canonical,original_execution_qualified=False,
        retained_literal_bodies=len(re.findall(r'(?:static )?void run_[0-9a-f]+\([^;]+?\) \{\n',strip(before))),
        kernel_sha256=sha(kernel),installer_sha256=sha(Path(__file__)),
        activity_header_sha256=sha(root/'include/halo2/source_damage_model_native_tu15.hpp'),
        original_instructions=[dict(pc=hex(pc),bytes=bytes(code.at(pc).bytes).hex())for pc in sorted(body.code)])
    args.manifest.parent.mkdir(parents=True,exist_ok=True);args.manifest.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k]for k in ('passed','entry','complete_original_instructions','generated_cpp_sha256')}))


if __name__=='__main__':main()
