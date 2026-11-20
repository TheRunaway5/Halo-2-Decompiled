#!/usr/bin/env python3
"""Prove the native TU1.5 1D7260 body motion snapshot (a havok component's
snapshot of one body's position and velocities): the logic in
native_tu15_world_leaves_systems.hpp (body_motion_snapshot::refresh) through
its boundary adapter in native_tu15_world_leaves.hpp (kernel_1d7260, with the
translated child 1D8B20 through the unit's route).

Run through tools/run_test.py. Extracts the unmodified canonical translated
body (run_1d29a0; pinned by tools/native_body_motion_snapshot_replacements.py),
its selected() operand cases and the binder's verification reference, and
compiles tests/native_tu15_body_motion_snapshot_tests.cpp against them.
Compares every seeded case (ordered store events with preimages, bank,
registers, flags, XMM, x87, MXCSR, cursor; faults by exception, cursor and
bank) in Release, census and verify (record/replay with every child boundary)
modes and an ASan/UBSan build, and requires every branch. Checks the
production binding (install, idempotence, refusal of a changed body, operand
or a missing child). Then screens deliberate defects in the logic and the
adapter: each mutant is an out-of-tree copy of one header placed first on the
include path and must be rejected in release or verify mode. Source hashes
are rechecked after the screen.
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
TEST = ROOT / 'tests/native_tu15_body_motion_snapshot_tests.cpp'
HELPER = ROOT / 'tools/native_body_motion_snapshot_replacements.py'
MUTANTS = [
    # logic
    (SYSTEMS, 'snapshot-slots-offset', 'inline constexpr std::uint32_t component_body_slots=0x70u;', 'inline constexpr std::uint32_t component_body_slots=0x74u;'),
    (SYSTEMS, 'snapshot-slot-stride', 'const auto slot=element(memory.load32(slots,at(component,component_body_slots)),index,sizeof(BodySlot));',
     'const auto slot=element(memory.load32(slots,at(component,component_body_slots)),index,0x50u);'),
    (SYSTEMS, 'snapshot-slot-located-late', 'memory.store32(slot_position_x,at(slot,', 'memory.store32(slot_position_x,at(slot_now,'),
    (SYSTEMS, 'snapshot-body-early', 'const auto body=memory.load32(body_read,at(slot_now,', 'const auto body=memory.load32(body_read,at(slot,'),
    (SYSTEMS, 'snapshot-body-field', 'const auto body=memory.load32(body_read,at(slot_now,offsetof(BodySlot,body)));',
     'const auto body=memory.load32(body_read,at(slot_now,offsetof(BodySlot,body)+4u));'),
    (SYSTEMS, 'snapshot-motionless', 'const bool moving=memory.load8(motionless,at(body,body_motionless))==0u;',
     'const bool moving=memory.load8(motionless,at(body,body_motionless))!=0u;'),
    (SYSTEMS, 'snapshot-motionless-width', 'const bool moving=memory.load8(motionless,', 'const bool moving=memory.load32(motionless,'),
    (SYSTEMS, 'snapshot-zero-late', '    const auto zero=memory.load32(zero_vector,globals::zero_vector);\n',
     '    const auto zero=moving?0u:memory.load32(zero_vector,globals::zero_vector);\n'),
    (SYSTEMS, 'snapshot-linear-offset', 'motion_linear_velocity=0x40u,', 'motion_linear_velocity=0x44u,'),
    (SYSTEMS, 'snapshot-angular-offset', 'motion_angular_velocity=0x50u;', 'motion_angular_velocity=0x54u;'),
    (SYSTEMS, 'snapshot-motion-offset', 'inline constexpr std::uint32_t body_motion=0x3cu,', 'inline constexpr std::uint32_t body_motion=0x38u,'),
    (SYSTEMS, 'snapshot-linear-reload-order',
     'const auto y=memory.load32(linear_y,staged(velocity,1));\n        linear_x_kept=memory.load32(linear_x_reload,staged(local.linear_velocity,0));',
     'linear_x_kept=memory.load32(linear_x_reload,staged(local.linear_velocity,0));\n        const auto y=memory.load32(linear_y,staged(velocity,1));'),
    (SYSTEMS, 'snapshot-linear-x-unstaged', 'linear_x_kept=memory.load32(linear_x_reload,staged(local.linear_velocity,0));',
     'linear_x_kept=memory.load32(linear_x_reload,staged(velocity,0));'),
    (SYSTEMS, 'snapshot-angular-stage-order',
     'angular[0]=memory.load32(angular_x_reload,staged(local.angular_velocity,0));\n        memory.store32(angular_z_staged,staged(local.angular_velocity,2),z);',
     'memory.store32(angular_z_staged,staged(local.angular_velocity,2),z);\n        angular[0]=memory.load32(angular_x_reload,staged(local.angular_velocity,0));'),
    (SYSTEMS, 'snapshot-zero-store-order',
     'memory.store32(zero_linear_y_staged,staged(local.linear_velocity,1),y);\n        memory.store32(zero_linear_z_staged,staged(local.linear_velocity,2),z);',
     'memory.store32(zero_linear_z_staged,staged(local.linear_velocity,2),z);\n        memory.store32(zero_linear_y_staged,staged(local.linear_velocity,1),y);'),
    (SYSTEMS, 'snapshot-zero-angular-lane', 'angular[1]=memory.load32(zero_angular_y,staged(zero,1));', 'angular[1]=memory.load32(zero_angular_y,staged(zero,2));'),
    (SYSTEMS, 'snapshot-position-lane', 'memory.load32(position_y,staged(local.position,1))', 'memory.load32(position_y,staged(local.position,2))'),
    (SYSTEMS, 'snapshot-linear-y-lane',
     'memory.load32(linear_y_reload,staged(local.linear_velocity,1))', 'memory.load32(linear_y_reload,staged(local.linear_velocity,2))'),
    (SYSTEMS, 'snapshot-angular-slot', 'memory.store32(slot_angular_y,at(slot,offsetof(BodySlot,angular_velocity[1])),angular[1]);',
     'memory.store32(slot_angular_y,at(slot,offsetof(BodySlot,angular_velocity[1])),angular[0]);'),
    # adapter
    (HEADER, 'snapshot-save-order', 'case component_argument:port.push(0x1d7263u,reg(ebx));port.push(0x1d7264u,reg(ebp));break;',
     'case component_argument:port.push(0x1d7263u,reg(ebp));port.push(0x1d7264u,reg(ebx));break;'),
    (HEADER, 'snapshot-return', 'snapshot_position_return=0x1d7281u;', 'snapshot_position_return=0x1d7280u;'),
    (HEADER, 'snapshot-push-esi', 'port.push(0x1d726cu,reg(esi));', 'port.push(0x1d726cu,reg(edi));'),
    (HEADER, 'snapshot-slot-ebx', 'reg(ebx)=flags.plus(reg(ebx),reg(esi),4);', 'reg(ebx)=flags.plus(reg(ebx),reg(eax),4);'),
    (HEADER, 'snapshot-esi-shift', 'reg(esi)=flags.shift(reg(esi),5u,4);', 'reg(esi)=flags.shift(reg(esi),4u,4);'),
    (HEADER, 'snapshot-call-flags', 'reg(ebx)=flags.plus(reg(ebx),reg(esi),4);', 'reg(ebx)=reg(ebx)+reg(esi);'),
    (HEADER, 'snapshot-exit-eax', 'case slots_again:case motion_read:case motion_again:case zero_linear_z:case angular_x_reload:case zero_angular_x:reg(eax)=value;break;',
     'case slots_again:case motion_read:case motion_again:case zero_linear_z:case angular_x_reload:reg(eax)=value;break;'),
    (HEADER, 'snapshot-exit-ecx', 'case angular_y_reload:case zero_angular_y:reg(ecx)=value;break;', 'case zero_angular_y:reg(ecx)=value;break;'),
    (HEADER, 'snapshot-exit-edx', 'case zero_vector:case angular_z_reload:case zero_angular_z:reg(edx)=value;break;',
     'case zero_vector:case zero_angular_z:reg(edx)=value;break;'),
    (HEADER, 'snapshot-xmm0', 'case angular_z:port.set_xmm(0,scalar(std::bit_cast<float>(value)));break;', 'case angular_z:break;'),
    (HEADER, 'snapshot-pop-order', 'case slot_angular_x:reg(edi)=port.pop(0x1d7341u);break;\n        case slot_angular_y:reg(esi)=port.pop(0x1d7344u);break;',
     'case slot_angular_x:reg(esi)=port.pop(0x1d7341u);break;\n        case slot_angular_y:reg(edi)=port.pop(0x1d7344u);break;'),
    (HEADER, 'snapshot-exit-flags', 'gp[esp]=port.flags.plus(gp[esp],0x24u,4);  // ADD ESP,24', 'gp[esp]=gp[esp]+0x24u;  // ADD ESP,24'),
    (HEADER, 'snapshot-ret', 'port.ret(0x1d7350u,4u);port.publish(0x1d7350u);', 'port.ret(0x1d7350u,8u);port.publish(0x1d7350u);'),
    (HEADER, 'snapshot-frame', 'SnapshotAbi(Port<State> &p,Children &c,std::uint32_t entry):port(p),children(c),t(entry-0x34u) {',
     'SnapshotAbi(Port<State> &p,Children &c,std::uint32_t entry):port(p),children(c),t(entry-0x30u) {'),
    (HEADER, 'snapshot-store-pc', 'case zero_linear_y_staged:return 0x1d72ccu;case zero_linear_z_staged:return 0x1d72d0u;',
     'case zero_linear_y_staged:return 0x1d72d0u;case zero_linear_z_staged:return 0x1d72ccu;'),
    # remap admission (the phantom/world composition)
    (HEADER, 'snapshot-remap-delta', 'inline constexpr Function f_1d7260[]{{0x1d7260u,0x1d7350u,0x48c0u,0x48c0u}};',
     'inline constexpr Function f_1d7260[]{{0x1d7260u,0x1d7350u,0x48b0u,0x48c0u}};'),
    (HEADER, 'snapshot-remap-store-pc', '        return pc-function(pc).published_offset;',
     '        return pc;'),
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
        raise ValueError('Canonical 1D7260 body or operands drifted from the pins')
    (directory / 'snapshot_bodies.inc').write_text(
        f'static void run_{module.NAME}(ObservedWorldState &);\n' + module.body(text, module.NAME)[0] + '\n')
    (directory / 'snapshot_reference.inc').write_text(module.reference(module.body(text, module.NAME)))
    operands = module.OPERANDS
    (directory / 'snapshot_selected.inc').write_text(''.join(f'case 0x{a:x}u:return 0x{b:x}u;\n' for a, b in sorted(operands.items())))


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
    module = load(HELPER, 'native_body_motion_snapshot_replacements')
    generate(module, text, scratch)
    # Production binding: installs, idempotent, refuses a changed body, a
    # changed operand mapping or a missing child.
    output, installed = module.install(text, text)
    if installed != [module.ENTRY] or module.prefix() not in output or module.REFERENCE not in output:
        raise ValueError(f'1D7260 binding installed {installed}')
    if module.install(text, output)[1]:
        raise ValueError('1D7260 binding is not idempotent')
    found = module.body(text, module.NAME)
    changed = text.replace(found[0], found[0].replace('0x24u', '0x28u', 1), 1)
    if changed == text or module.install(changed, changed)[1]:
        raise ValueError('1D7260 binding accepted a changed body')
    operand = sorted(module.OPERANDS)[0]
    remapped = text.replace(f'case 0x{operand:x}u:return 0x{module.OPERANDS[operand]:x}u;',
                            f'case 0x{operand:x}u:return 0x{module.OPERANDS[operand] + 4:x}u;', 1)
    if remapped == text or module.install(remapped, remapped)[1]:
        raise ValueError('1D7260 binding accepted a changed operand mapping')
    child = next(iter(module.CHILDREN))
    orphaned = text.replace(f'static void run_{child}(ObservedWorldState &s) {{', f'static void run_{child}_moved(ObservedWorldState &s) {{', 1)
    if orphaned == text or module.install(orphaned, orphaned)[1]:
        raise ValueError('1D7260 binding accepted a missing child')
    report = dict(passed=False, unit=str(UNIT.relative_to(ROOT)), pin=module.PIN)
    release = compile_harness(scratch, 'snapshot-release', [])
    result = run(release, args.cases)
    census = run(release, max(args.cases // 4, 1000), 'census')
    verify = run(release, max(args.cases // 4, 1000), 'verify')
    # The phantom/world composition's canonical-PC remap (release, census, verify).
    report['remap'] = remap_sites.run_modes(run, release, max(args.cases // 4, 1000))
    header = HEADER.read_text()
    # The bits after E8D80's in coverage word 2.
    offset = 64 + sum(len(enum_names(header, name)) - 1 for name in ('ClosestPointBranch', 'SweptPairBranch', 'ChildShapeBranch',
                                                                     'AabbQueryBranch', 'ChildAgentBranch', 'FlagUpdateBranch'))
    branches = [n for n in enum_names(header, 'SnapshotBranch') if n != 'snapshot_branch_end']
    coverage = (result['coverage'] | result['coverage_high'] << 64 | result['coverage_top'] << 128) | \
        (verify['coverage'] | verify['coverage_high'] << 64 | verify['coverage_top'] << 128)
    hit = {name for index, name in enumerate(branches) if coverage >> (offset + index) & 1}
    missing = sorted(set(branches) - hit)
    report.update(release=result, census=census, verify=verify, branches=branches, covered=sorted(hit), missing=missing)
    if missing:
        raise ValueError(f'Uncovered 1D7260 branches: {missing}')
    if census['compared'] == 0 or verify['compared'] == 0 or result['child_calls'] == 0:
        raise ValueError('Census, verify or the children never ran')
    if not args.skip_sanitizers:
        sanitized = compile_harness(scratch, 'snapshot-sanitized', [], sanitizers=True)
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
        if runs.returncode == 0:
            # The composition's remap (admission, cursor and store PCs).
            runs = subprocess.run([str(binary), str(max(args.mutant_cases // 3, 200)), 'remap'], capture_output=True, text=True)
            mode = 'remap'
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
    print(f"PASS {result['compared']} native 1D7260 comparisons ({result['ordered_stores']} ordered stores, "
          f"{result['faults']} faults), census {census['compared']}, verify {verify['compared']}, "
          f"{len(mutants)} mutants rejected")
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
