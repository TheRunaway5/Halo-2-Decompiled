#!/usr/bin/env python3
"""Compare the 1772C0 native prefix against its pinned translated boundary."""

import argparse
import hashlib
import json
import os
import re
from pathlib import Path
import subprocess

import native_particle_draw_entry_replacements as binding


ROOT = Path(__file__).resolve().parents[1]
UNIT = ROOT / 'src/rendering/effects/source_effect_particle_draw_tu15_generated.cpp'
HEADER = ROOT / 'src/rendering/effects/native_tu15_particle_draw_entry.hpp'
TEST = ROOT / 'tests/native_tu15_particle_draw_entry_tests.cpp'
POSITION_TEST = ROOT / 'tests/native_tu15_particle_draw_position_tests.cpp'
POSITION = ROOT / 'src/rendering/effects/native_tu15_particle_draw_position.hpp'
VISIBILITY = ROOT / 'src/rendering/effects/native_tu15_particle_draw_visibility.hpp'
VISIBILITY_TEST = ROOT / 'tests/native_tu15_particle_draw_visibility_tests.cpp'
VIEW = ROOT / 'src/rendering/effects/native_tu15_particle_draw_view.hpp'
VIEW_TEST = ROOT / 'tests/native_tu15_particle_draw_view_tests.cpp'
DISTANCE = ROOT / 'src/rendering/effects/native_tu15_particle_draw_distance.hpp'
DISTANCE_TEST = ROOT / 'tests/native_tu15_particle_draw_distance_tests.cpp'
ROUTING = ROOT / 'src/rendering/effects/native_tu15_particle_draw_routing.hpp'
ROUTING_TEST = ROOT / 'tests/native_tu15_particle_draw_routing_tests.cpp'
SUBTREE = ROOT / 'src/rendering/effects/native_tu15_particle_draw_subtree.hpp'
SUBTREE_TEST = ROOT / 'tests/native_tu15_particle_draw_subtree_tests.cpp'
MATRIX = ROOT / "src/rendering/effects/native_tu15_particle_draw_matrix.hpp"
MATRIX_TEST = ROOT / "tests/native_tu15_particle_draw_matrix_tests.cpp"
SUBMISSION = ROOT / "src/rendering/effects/native_tu15_particle_draw_submission.hpp"
SUBMISSION_TEST = ROOT / "tests/native_tu15_particle_draw_submission_tests.cpp"
MUTANTS = (
    ('signed-tag', 'static_cast<std::int16_t>(gp[edx])', 'static_cast<std::uint16_t>(gp[edx])'),
    ('element-array', 'gp[eax] + 0x44u, 4', 'gp[eax] + 0x40u, 4'),
    ('virtual-slot', 'gp[edx] + 0x28u, 4', 'gp[edx] + 0x24u, 4'),
    ('protected-draw-bit', 'gp[edx] & 0xffu, 5u, 1', 'gp[edx] & 0xffu, 4u, 1'),
    ('virtual-child-fault-state', '                this->gp = state.gp;\n', ''),
    ('missing-accessor-route', 'this->reached(missing_accessor);\n            continuation = descendants;',
     'this->reached(missing_accessor);\n            continuation = draw;'),
)


def hashes():
    files = (UNIT, HEADER, TEST, POSITION, POSITION_TEST, Path(binding.__file__), Path(__file__),
             ROOT / 'tests/native_tu15_frame_misc_harness.hpp')
    files += (VISIBILITY, VISIBILITY_TEST, VIEW, VIEW_TEST, DISTANCE, DISTANCE_TEST, ROUTING, ROUTING_TEST, SUBTREE, SUBTREE_TEST, MATRIX, MATRIX_TEST, SUBMISSION, SUBMISSION_TEST)
    return {str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest() for path in files}


def prepare(scratch):
    from specialize_native_operands import specialize

    source = UNIT.read_text()
    world = binding.world_helper()
    if not binding.admitted(source, world):
        raise ValueError('Particle draw entry canonical body drift')
    canonical = world.body(source, binding.ENTRY)[0]
    bound, receipt = specialize(source)
    if (bound.count(binding.PREFIX) != 1 or binding.ENTRY not in receipt.get('native_spatial_owners', [])
            or binding.install(source, bound) != (bound, [])):
        raise ValueError('Particle draw entry binding must install exactly once')
    changed = source.replace(canonical, canonical.replace('{\n', '{\n// changed body\n', 1), 1)
    if binding.install(changed, changed) != (changed, []):
        raise ValueError('Particle draw entry admitted a changed body')
    (scratch / 'body.inc').write_text(binding.reference(canonical, 'retained_particle_draw_entry', replay=False))
    (scratch / 'reference.inc').write_text(binding.reference(canonical))
    (scratch / 'position_refs.inc').write_text(binding.position_references(canonical))
    (scratch / 'visibility_reference.inc').write_text(
        binding.visibility_reference(canonical, 'retained_visibility', replay=False)
        + binding.visibility_reference(canonical, 'replay_visibility'))
    (scratch / 'view_refs.inc').write_text(binding.view_references(canonical))
    spill, = re.findall(
        r'^std::uint32_t native_single_spill\(long double x,std::uint16_t &status\) \{.*?^\}',
        source, re.M | re.S)
    (scratch / 'distance_refs.inc').write_text(
        spill + '\n' + binding.distance_reference(canonical) +
        binding.distance_reference(canonical, 'retained_distance', replay=False))
    (scratch / 'routing_refs.inc').write_text(
        binding.routing_references(canonical) + binding.routing_references(canonical, replay=False))
    (scratch / 'subtree_refs.inc').write_text(
        binding.subtree_reference(canonical) + binding.subtree_reference(canonical, 'retained_subtree', replay=False))
    (scratch / 'matrix_refs.inc').write_text(binding.matrix_references(canonical))
    (scratch / 'submission_refs.inc').write_text(
        binding.submission_references(canonical) + binding.submission_references(canonical, replay=False))
    prepared = scratch / UNIT.name
    prepared.write_text(bound)
    return prepared


def flags(compiler):
    return [compiler, '-std=c++20', '-O2', '-fno-fast-math', '-ffp-contract=off', '-frounding-math']


def compile_harness(compiler, scratch, name, overlay=None, sanitize=False, test=TEST):
    binary = scratch / name
    command = flags(compiler)
    if sanitize:
        command.extend(('-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer'))
    if overlay:
        command.append(f'-I{overlay}')
    command.extend((f'-I{ROOT / "src"}', f'-I{ROOT / "include"}', f'-I{scratch}', str(test), '-o', str(binary)))
    subprocess.run(command, cwd=scratch, check=True)
    return binary


def execute(binary, cases, mode):
    return subprocess.run([str(binary), str(cases), mode], capture_output=True, text=True)


def cohort(binary, cases):
    results = {}
    for mode in ('', 'census', 'verify'):
        result = execute(binary, cases, mode)
        if result.returncode:
            raise RuntimeError(f'{binary.name} {mode}: {result.stderr.strip()}')
        results[mode or 'literal'] = json.loads(result.stdout)
    return results


def verify_mutants(compiler, scratch):
    rejected = []
    source = HEADER.read_text()
    for name, old, new in MUTANTS:
        if source.count(old) != 1:
            raise ValueError(f'Particle draw mutant anchor drift: {name}')
        overlay = scratch / 'mutants' / name
        header = overlay / HEADER.relative_to(ROOT / 'src')
        header.parent.mkdir(parents=True)
        header.write_text(source.replace(old, new, 1))
        binary = compile_harness(compiler, scratch, name, overlay=overlay)
        result = execute(binary, 1000, '')
        if result.returncode != 1 or 'case ' not in result.stderr:
            raise ValueError(f'Particle draw entry did not reject {name}: {result.stderr.strip()}')
        rejected.append({'name': name, 'diagnostic': result.stderr.strip()[-600:]})
        binary.unlink()
    return rejected


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', default='c++')
    parser.add_argument('--cases', type=int, default=8000)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Run through tools/run_test.py')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    source_hashes = hashes()
    prepared = prepare(scratch)
    subprocess.run(flags(args.compiler) + ['-fsyntax-only', f'-I{UNIT.parent}', f'-I{ROOT / "src"}',
                   f'-I{ROOT / "include"}', str(prepared)], cwd=scratch, check=True)
    results = cohort(compile_harness(args.compiler, scratch, 'release'), args.cases)
    sanitized = cohort(compile_harness(args.compiler, scratch, 'sanitized', sanitize=True), 2000)
    position = cohort(compile_harness(args.compiler, scratch, 'position', test=POSITION_TEST), args.cases)
    position_sanitized = cohort(compile_harness(args.compiler, scratch, 'position-sanitized', sanitize=True,
                                                test=POSITION_TEST), 2000)
    visibility = cohort(compile_harness(args.compiler, scratch, 'visibility', test=VISIBILITY_TEST), args.cases)
    visibility_sanitized = cohort(compile_harness(args.compiler, scratch, 'visibility-sanitized', sanitize=True,
                                                  test=VISIBILITY_TEST), 1500)
    view = cohort(compile_harness(args.compiler, scratch, 'view', test=VIEW_TEST), args.cases)
    view_sanitized = cohort(compile_harness(args.compiler, scratch, 'view-sanitized', sanitize=True,
                                            test=VIEW_TEST), 1500)
    distance = cohort(compile_harness(args.compiler, scratch, 'distance', test=DISTANCE_TEST), args.cases)
    distance_sanitized = cohort(compile_harness(args.compiler, scratch, 'distance-sanitized', sanitize=True,
                                            test=DISTANCE_TEST), 1500)
    routing = cohort(compile_harness(args.compiler, scratch, 'routing', test=ROUTING_TEST), args.cases)
    routing_sanitized = cohort(compile_harness(args.compiler, scratch, 'routing-sanitized', sanitize=True,
                                            test=ROUTING_TEST), 1500)
    subtree = cohort(compile_harness(args.compiler, scratch, 'subtree', test=SUBTREE_TEST), args.cases)
    subtree_sanitized = cohort(compile_harness(args.compiler, scratch, 'subtree-sanitized', sanitize=True,
                                               test=SUBTREE_TEST), 1500)
    matrix = cohort(compile_harness(args.compiler, scratch, 'matrix', test=MATRIX_TEST), args.cases)
    matrix_sanitized = cohort(compile_harness(args.compiler, scratch, 'matrix-sanitized', sanitize=True,
                                                test=MATRIX_TEST), 1500)
    submission = cohort(compile_harness(args.compiler, scratch, 'submission', test=SUBMISSION_TEST), args.cases)
    submission_sanitized = cohort(compile_harness(args.compiler, scratch, 'submission-sanitized', sanitize=True,
                                                test=SUBMISSION_TEST), 1500)
    mutations = verify_mutants(args.compiler, scratch)
    if hashes() != source_hashes:
        raise ValueError('Particle draw entry proof sources changed')
    report = dict(passed=True, pin=binding.PIN, source_sha256=source_hashes, results=results,
                  position=position, position_sanitized=position_sanitized,
                  visibility=visibility, visibility_sanitized=visibility_sanitized,
                  view=view, view_sanitized=view_sanitized,
                  distance=distance, distance_sanitized=distance_sanitized,
                  routing=routing, routing_sanitized=routing_sanitized,
                  subtree=subtree, subtree_sanitized=subtree_sanitized,
                  matrix=matrix, matrix_sanitized=matrix_sanitized,
                  submission=submission, submission_sanitized=submission_sanitized,
                  sanitized=sanitized, rejected_mutants=mutations, prepared_syntax_passed=True,
                  owned_scratch=str(scratch))
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(f'PASS 1772C0 particle draw: {sum(row["compared"] for row in results.values())} comparisons, '
          f'{len(mutations)} rejected mutants')


if __name__ == '__main__':
    main()
