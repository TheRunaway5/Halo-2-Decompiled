#!/usr/bin/env python3
"""Prove the native TU1.5 2F36F0 child agent update: the logic in
native_tu15_world_leaves_systems.hpp (child_agent_update::update) through its
boundary adapter in native_tu15_world_leaves.hpp (kernel_2f36f0, with 2E4550
and 30BAD0 inline).

Run through tools/run_test.py. Extracts the unmodified canonical translated
body (run_2efc30; pinned by tools/native_child_agents_replacements.py) and
the translated 2E4550 and 30BAD0 its reference runs, their selected() operand cases
and the binder's verification reference, and compiles
tests/native_tu15_child_agents_tests.cpp against them. Compares every
seeded case (ordered store events with preimages, bank, registers, flags,
XMM, x87, MXCSR, cursor; faults by exception, cursor and bank) in Release,
census and verify (record/replay with every child boundary) modes and an
ASan/UBSan build, and requires every branch. Checks the production binding
(install, idempotence, refusal of a changed body, operand or dispatch site).
Then screens deliberate defects in the logic and the adapter: each mutant is an
out-of-tree copy of one header placed first on the include path and must be
rejected in release or verify mode. Source hashes are rechecked after the
screen.
"""
import argparse
import concurrent.futures
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]
UNIT = ROOT / 'src/physics/runtime/physics_world_runtime_generated.cpp'
HEADER = ROOT / 'src/physics/runtime/native_tu15_world_leaves.hpp'
SYSTEMS = ROOT / 'src/physics/runtime/native_tu15_world_leaves_systems.hpp'
TEST = ROOT / 'tests/native_tu15_child_agents_tests.cpp'
HELPER = ROOT / 'tools/native_child_agents_replacements.py'
MUTANTS = [
    # logic
    (SYSTEMS, 'agents-cache-flag', 'const bool cache=memory.load8(cache_enabled,globals::aabb_cache)!=0u;',
     'const bool cache=memory.load8(cache_enabled,globals::aabb_cache)==0u;'),
    (SYSTEMS, 'agents-slack-lane', 'const auto slack=splat(vector({memory.load32(slack_reload,local.slack),0u,0u,0u}),0);',
     'const auto slack=splat(vector({memory.load32(slack_reload,local.slack),0u,0u,0u}),1);'),
    (SYSTEMS, 'agents-moved-lanes', 'test.moved=std::uint32_t(_mm_movemask_ps(test.beyond))&7u;',
     'test.moved=std::uint32_t(_mm_movemask_ps(test.beyond))&15u;'),
    (SYSTEMS, 'agents-corner-order', 'auto test=face_test(slack,cached_max,maximum,mask);', 'auto test=face_test(slack,maximum,cached_max,mask);'),
    (SYSTEMS, 'agents-half-multiply', 'const auto half=x87::multiply(x87::load(tolerance_value),memory.load_float(half_factor,globals::half));',
     'const auto half=x87::add(x87::load(tolerance_value),memory.load_float(half_factor,globals::half));'),
    (SYSTEMS, 'agents-half-rounding', 'const auto rounded=x87::load(memory.load_float(half_reload,local.half_tolerance));',
     'const auto rounded=half;memory.load_float(half_reload,local.half_tolerance);'),
    (SYSTEMS, 'agents-cache-store-order',
     'memory.store_vector(cache_minimum_store,at(agent,offsetof(Agent,cached.min)),minimum);\n    memory.store_vector(cache_maximum_store,at(agent,offsetof(Agent,cached.max)),maximum);',
     'memory.store_vector(cache_minimum_store,at(agent,offsetof(Agent,cached.max)),maximum);\n    memory.store_vector(cache_maximum_store,at(agent,offsetof(Agent,cached.min)),minimum);'),
    (SYSTEMS, 'agents-inline-keys', 'inline constexpr std::uint32_t inline_keys=0x80u;', 'inline constexpr std::uint32_t inline_keys=0x7fu;'),
    (SYSTEMS, 'agents-matching-choice', 'const auto keys=in_query_order?match_in_query_order(memory,frame,local,agent):merge_sorted(memory,frame,local,agent);',
     'const auto keys=!in_query_order?match_in_query_order(memory,frame,local,agent):merge_sorted(memory,frame,local,agent);'),
    (SYSTEMS, 'agents-expected-step', 'if(next)expected+=key_size;', 'if(next)expected+=entry_size;'),
    (SYSTEMS, 'agents-search-step', 'while(!(found=key==memory.load32(search_key,position)) && (position+=key_size)!=keys_end){}',
     'while(!(found=key==memory.load32(search_key,position)) && (position+=entry_size)!=keys_end){}'),
    (SYSTEMS, 'agents-found-next', 'expected=position+key_size;', 'expected=position;'),
    (SYSTEMS, 'agents-removal-check', 'if(!(std::int32_t(index)<std::int32_t(memory.load32(removal_size_reload,size_address))))return false;',
     'if(!(std::int32_t(index)<=std::int32_t(memory.load32(removal_size_reload,size_address))))return false;'),
    (SYSTEMS, 'agents-removal-bound', 'if(!(std::int32_t(index)<std::int32_t(bound)))return true;', 'if(!(std::int32_t(index)<std::int32_t(bound)+1))return true;'),
    (SYSTEMS, 'agents-removal-step', 'entry-=entry_size;end-=entry_size;  // the next entry', 'end-=entry_size;  // the next entry'),
    (SYSTEMS, 'agents-insert-index', 'const auto index=std::uint32_t(std::int32_t(cursor-keys)>>2);',
     'const auto index=std::uint32_t(std::int32_t(cursor-keys)>>3);'),
    (SYSTEMS, 'agents-grown-capacity', 'return std::int32_t(needed)<std::int32_t(capacity*2u)?capacity*2u:needed;',
     'return std::int32_t(needed)<std::int32_t(capacity*2u)?needed:capacity*2u;'),
    (SYSTEMS, 'agents-insert-room', 'if(std::int32_t(capacity)<std::int32_t(grown))frame.reserve(children,grown_capacity(capacity,grown));',
     'if(std::int32_t(capacity)<=std::int32_t(grown))frame.reserve(children,grown_capacity(capacity,grown));'),
    (SYSTEMS, 'agents-move-count', 'memory.store32(move_count_store,local.move_count,size-index);',
     'memory.store32(move_count_store,local.move_count,size-index+1u);'),
    (SYSTEMS, 'agents-new-agent-field', 'memory.store32(new_agent_store,at(entry,offsetof(ChildEntry,agent)),created);',
     'memory.store32(new_agent_store,at(entry,offsetof(ChildEntry,not_used_4)),created);'),
    (SYSTEMS, 'agents-merge-unsigned', '} else if(wanted>held) {', '} else if(std::int32_t(wanted)>std::int32_t(held)) {'),
    (SYSTEMS, 'agents-kept-word', 'memory.store32(kept_word1_store,at(out,4),memory.load32(kept_word1,at(entry,4)));',
     'memory.store32(kept_word1_store,at(out,4),memory.load32(kept_word1,at(entry,8)));'),
    (SYSTEMS, 'agents-release-null', 'if(child)frame.cleanup_gone(', 'frame.cleanup_gone('),
    (SYSTEMS, 'agents-tail-stride', 'key+=key_size;out+=entry_size;\n        if(key==last)break;', 'key+=key_size;out+=key_size;\n        if(key==last)break;'),
    (SYSTEMS, 'agents-install-owned', 'memory.store32(install_capacity_store,at(array,offsetof(HavokArray,capacity_and_flags)),size);',
     'memory.store32(install_capacity_store,at(array,offsetof(HavokArray,capacity_and_flags)),size|local_buffers::storage_not_owned);'),
    (SYSTEMS, 'agents-install-bytes', 'owner,old,(held_capacity&contact_array::capacity_mask)*entry_size,',
     'owner,old,(held_capacity&contact_array::capacity_mask)*key_size,'),
    (SYSTEMS, 'agents-copy-count', 'for(auto left=size;left;--left,source+=entry_size,target+=entry_size) {',
     'for(auto left=size-1u;left;--left,source+=entry_size,target+=entry_size) {'),
    (SYSTEMS, 'agents-keys-bytes', 'owner,keys,(capacity&contact_array::capacity_mask)*key_size,', 'owner,keys,(capacity&contact_array::capacity_mask)*entry_size,'),
    (SYSTEMS, 'agents-comparator-width', 'memory.store8(comparator_store,local.comparator,0u);', 'memory.store32(comparator_store,local.comparator,0u);'),
    (SYSTEMS, 'agents-sort-high', 'key_count-1u,comparator);  // 2F33B0', 'key_count,comparator);  // 2F33B0'),
    (SYSTEMS, 'agents-child-parent', 'memory.store32(child_parent_store,at(local.child_body,offsetof(CdBody,parent)),body_b);',
     'memory.store32(child_parent_store,at(local.child_body,offsetof(CdBody,parent)),body_a);'),
    (SYSTEMS, 'agents-install-room', 'if(std::int32_t(held_capacity&contact_array::capacity_mask)<std::int32_t(size)) {',
     'if(std::int32_t(held_capacity&contact_array::capacity_mask)<=std::int32_t(size)) {'),
    # adapter
    (HEADER, 'agents-save-order', 'case body_a_argument:port.push(0x2f3718u,reg(esi));port.push(0x2f3719u,reg(edi));break;',
     'case body_a_argument:port.push(0x2f3718u,reg(edi));port.push(0x2f3719u,reg(esi));break;'),
    (HEADER, 'agents-handler', 'inline constexpr std::uint32_t child_agents_handler=0x37e656u;', 'inline constexpr std::uint32_t child_agents_handler=0x37e657u;'),
    (HEADER, 'agents-unwind-zero', 'port.store(0x2f3808u,t+0x3dcu,4,reg(eax));', 'port.store(0x2f3808u,t+0x3dcu,1,reg(eax));'),
    (HEADER, 'agents-unwind-one', 'port.store(0x2f3b49u,t+0x3dcu,1,1u);', 'port.store(0x2f3b49u,t+0x3dcu,4,1u);'),
    (HEADER, 'agents-enabled-sites', 'site[3]{child_agents_site::enabled,child_agents_site::merge_enabled,child_agents_site::tail_enabled};',
     'site[3]{child_agents_site::enabled,child_agents_site::tail_enabled,child_agents_site::merge_enabled};'),
    (HEADER, 'agents-null-return', 'next[3]{0x2f3a97u,0x2f3c36u,0x2f3ce7u};', 'next[3]{0x2f3a97u,0x2f3c37u,0x2f3ce7u};'),
    (HEADER, 'agents-filter-push', 'argument(0x2f3a6bu,reg(ebx));argument(0x2f3a6cu,reg(eax));',
     'argument(0x2f3a6bu,reg(eax));argument(0x2f3a6cu,reg(ebx));'),
    (HEADER, 'agents-cache-al', 'reg(eax)=(reg(eax)&0xffffff00u)|(kept?1u:0u);flags.logic(reg(eax)&0xffu,1);',
     'reg(eax)=(reg(eax)&0xffffff00u)|(kept?0u:1u);flags.logic(reg(eax)&0xffu,1);'),
    (HEADER, 'agents-reserve-push', 'port.push(0x2f39b8u,0xcu);port.push(0x2f39bau,reg(eax));port.push(0x2f39bbu,reg(esi));',
     'port.push(0x2f39b8u,0xcu);port.push(0x2f39bau,reg(esi));port.push(0x2f39bbu,reg(eax));'),
    (HEADER, 'agents-exit-pops', 'gp[edi]=port.pop(0x2f3dc9u);gp[esi]=port.pop(0x2f3dcau);', 'gp[esi]=port.pop(0x2f3dc9u);gp[edi]=port.pop(0x2f3dcau);'),
    (HEADER, 'agents-ret', 'port.ret(0x2f3dd6u,0x10u);', 'port.ret(0x2f3dd6u,0xcu);'),
    (HEADER, 'agents-kept-esi', '        case kept_word2_store:reg(esi)=flags.plus(reg(esi),4u,4);break;\n', ''),
    (HEADER, 'agents-minimum-xmm', '        case aabb_minimum:port.set_xmm(4,value);break;\n', ''),
    (HEADER, 'agents-x87-pop', 'case half_store:case margin_store:x87.pop();break;', 'case half_store:x87.pop();break;'),
    (HEADER, 'agents-tail-compare', 'reg(esi)=flags.plus(reg(esi),4u,4);reg(ebx)=flags.plus(reg(ebx),0xcu,4);flags.minus(reg(esi),reg(eax),4);',
     'reg(esi)=flags.plus(reg(esi),4u,4);reg(ebx)=flags.plus(reg(ebx),0xcu,4);flags.minus(reg(eax),reg(esi),4);'),
    (HEADER, 'agents-merge-compare', 'case merge_entry_key:flags.minus(reg(eax),value,4);break;  // CMP EAX,ECX',
     'case merge_entry_key:flags.minus(value,reg(eax),4);break;  // CMP EAX,ECX'),
    (HEADER, 'agents-gone-edi', 'if(gone_pending){gone_pending=false;reg(edi)=flags.plus(reg(edi),0xcu,4);}', 'if(gone_pending){gone_pending=false;}'),
    (HEADER, 'agents-result-slot', 'argument(0x2f3a6bu,reg(ebx));argument(0x2f3a6cu,reg(eax));reg(eax)=t+0x1fu;',
     'argument(0x2f3a6bu,reg(ebx));argument(0x2f3a6cu,reg(eax));reg(eax)=t+0x1eu;'),
    (HEADER, 'agents-keys-end-register', 'flags.minus(reg(edi),reg(ecx),4);reg(eax)=reg(edx)+reg(eax)*4u;',
     'flags.minus(reg(edi),reg(ecx),4);reg(eax)=reg(edx)+reg(eax)*3u;'),
]

REMAP = Path(__file__).resolve().with_name('native_world_leaves_remap_sites.py')


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def load(path, name):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def generate(module, text, directory):
    """Write the harness inputs (bodies, reference, selected() cases)."""
    if not module.admitted(text):
        raise ValueError('Canonical 2F36F0 body, operands or children drifted from the pins')
    names = module.INLINE + (module.NAME,)
    (directory / 'agents_bodies.inc').write_text(
        ''.join(f'static void run_{name}(ObservedWorldState &);\n' for name in names) +
        ''.join(module.ordered(module.body(text, name)[0]) + '\n' for name in names))
    (directory / 'agents_reference.inc').write_text(module.reference(module.body(text, module.NAME)))
    # 30BAD0's operand (the null agent) as the collection agent binder pins it.
    operands = {**module.OPERANDS, **module.load('native_collection_agent_replacements').NULL_AGENT[2]}
    (directory / 'agents_selected.inc').write_text(''.join(f'case 0x{a:x}u:return 0x{b:x}u;\n' for a, b in sorted(operands.items())))


def compile_harness(scratch, name, includes, sanitizers=False):
    binary = scratch / name
    command = ['c++', '-std=c++20', '-O2', '-fno-fast-math', '-ffp-contract=off', '-msse2',
               *[f'-I{path}' for path in includes], f'-I{ROOT / "include"}', f'-I{ROOT / "src"}',
               f'-I{scratch}', str(TEST), '-o', str(binary)]
    if sanitizers:
        command[1:1] = ['-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer']
    subprocess.run(command, check=True, cwd=scratch)
    return binary


def run(binary, cases, mode=''):
    command = [str(binary), str(cases)] + ([mode] if mode else [])
    return json.loads(subprocess.run(command, check=True, capture_output=True, text=True).stdout)


def enum_names(text, name):
    block = re.search(r'enum ' + name + r' : unsigned \{(.*?)\};', text, re.S)[1]
    return [entry.split('=')[0] for entry in re.sub(r'\s|//[^\n]*', '', block).split(',') if entry]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--cases', type=int, default=20000)
    parser.add_argument('--mutant-cases', type=int, default=3000)
    parser.add_argument('--jobs', type=int, default=4)
    parser.add_argument('--skip-sanitizers', action='store_true')
    parser.add_argument('--skip-mutants', action='store_true')
    args = parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Run through tools/run_test.py')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    spec = importlib.util.spec_from_file_location('native_world_leaves_remap_sites', REMAP)
    remap_sites = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(remap_sites)
    remap_sites.write(scratch, 'world_sites')
    sources = [UNIT, HEADER, SYSTEMS, TEST, HELPER, Path(__file__).resolve()] + remap_sites.SOURCES
    before = {str(path.relative_to(ROOT)): digest(path) for path in sources}
    text = UNIT.read_text()
    module = load(HELPER, 'native_child_agents_replacements')
    generate(module, text, scratch)
    # Production binding: installs, idempotent, refuses a changed body, a
    # changed operand mapping or a changed dispatch site.
    output, installed = module.install(text, text)
    if installed != [module.ENTRY] or module.prefix() not in output or module.REFERENCE not in output:
        raise ValueError(f'2F36F0 binding installed {installed}')
    if module.install(text, output)[1]:
        raise ValueError('2F36F0 binding is not idempotent')
    found = module.body(text, module.NAME)
    changed = text.replace(found[0], found[0].replace('0x7fffffffu', '0x7ffffffeu', 1), 1)
    if changed == text or module.install(changed, changed)[1]:
        raise ValueError('2F36F0 binding accepted a changed body')
    operand = sorted(module.OPERANDS)[0]
    remapped = text.replace(f'case 0x{operand:x}u:return 0x{module.OPERANDS[operand]:x}u;',
                            f'case 0x{operand:x}u:return 0x{module.OPERANDS[operand] + 4:x}u;', 1)
    if remapped == text or module.install(remapped, remapped)[1]:
        raise ValueError('2F36F0 binding accepted a changed operand mapping')
    site = module.SITES[0]
    moved = text.replace(found[0], found[0].replace(f'0x{site:x}u,false)', f'0x{site + 1:x}u,false)', 1), 1)
    if moved == text or module.install(moved, moved)[1]:
        raise ValueError('2F36F0 binding accepted a changed dispatch site')
    report = dict(passed=False, unit=str(UNIT.relative_to(ROOT)), pin=module.PIN)
    release = compile_harness(scratch, 'agents-release', [])
    result = run(release, args.cases)
    census = run(release, max(args.cases // 4, 1000), 'census')
    verify = run(release, max(args.cases // 4, 1000), 'verify')
    # The phantom/world composition's canonical-PC remap (release, census, verify).
    report['remap'] = remap_sites.run_modes(run, release, max(args.cases // 4, 1000))
    header = HEADER.read_text()
    # The bits after the earlier word-1 owners' in coverage word 1.
    offset = 64 + sum(len(enum_names(header, name)) - 1 for name in ('ClosestPointBranch', 'SweptPairBranch', 'ChildShapeBranch', 'AabbQueryBranch'))
    branches = [n for n in enum_names(header, 'ChildAgentBranch') if n != 'child_agent_branch_end']
    coverage = (result['coverage'] | result['coverage_high'] << 64) | (verify['coverage'] | verify['coverage_high'] << 64)
    hit = {name for index, name in enumerate(branches) if coverage >> (offset + index) & 1}
    missing = sorted(set(branches) - hit)
    report.update(release=result, census=census, verify=verify, branches=branches, covered=sorted(hit), missing=missing)
    if missing:
        raise ValueError(f'Uncovered 2F36F0 branches: {missing}')
    if census['compared'] == 0 or verify['compared'] == 0 or result['child_calls'] == 0:
        raise ValueError('Census, verify or the children never ran')
    if not args.skip_sanitizers:
        sanitized = compile_harness(scratch, 'agents-sanitized', [], sanitizers=True)
        report['sanitized'] = run(sanitized, max(args.cases // 5, 1000))
        report['sanitized_verify'] = run(sanitized, 1000, 'verify')
        sanitized.unlink()
    texts = {path: path.read_text() for path in (HEADER, SYSTEMS)}

    def screen(mutant):
        path, name, old, new = mutant
        if texts[path].count(old) != 1:
            return dict(name=name, rejected=False, error='mutation anchor missing')
        directory = scratch / 'mutants' / name / 'physics/runtime'
        directory.mkdir(parents=True)
        (directory / path.name).write_text(texts[path].replace(old, new, 1))
        try:
            binary = compile_harness(scratch, f'mutant-{name}', [scratch / 'mutants' / name])
        except subprocess.CalledProcessError:
            return dict(name=name, rejected=False, error='mutant failed to compile')
        # Release compares stores, registers and the cursor; verify also
        # replays every read (order, address, width) against the translated
        # body.
        runs = subprocess.run([str(binary), str(args.mutant_cases)], capture_output=True, text=True)
        mode = 'release'
        if runs.returncode == 0:
            runs = subprocess.run([str(binary), str(max(args.mutant_cases // 3, 200)), 'verify'], capture_output=True, text=True)
            mode = 'verify'
        binary.unlink()
        return dict(name=name, rejected=runs.returncode != 0, mode=mode, diagnostic=runs.stderr.strip()[-200:])
    mutants = []
    if not args.skip_mutants:
        with concurrent.futures.ThreadPoolExecutor(args.jobs) as pool:
            mutants = list(pool.map(screen, MUTANTS))
    report['mutants'] = mutants
    release.unlink()
    after = {str(path.relative_to(ROOT)): digest(path) for path in sources}
    report['source_sha256'] = after
    if after != before:
        raise ValueError('Sources changed during the mutation screen')
    survivors = [m['name'] for m in mutants if not m['rejected']]
    report['passed'] = not survivors
    report['survivors'] = survivors
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')
    if survivors:
        raise ValueError(f'Mutants survived: {survivors}')
    print(f"PASS {result['compared']} native 2F36F0 comparisons ({result['ordered_stores']} ordered stores, "
          f"{result['faults']} faults), census {census['compared']}, verify {verify['compared']}, "
          f"{len(mutants)} mutants rejected")
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
