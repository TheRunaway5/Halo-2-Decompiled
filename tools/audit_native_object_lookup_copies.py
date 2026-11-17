#!/usr/bin/env python3
"""Authenticate complete Scene, Sandbox and ParticleDraw object-lookup owners."""
import argparse, hashlib, json, os, re, subprocess, sys
from pathlib import Path
from specialize_native_operands import specialize

ROOT = Path(__file__).resolve().parents[1]
OWNERS = {
    'scene_device_constructor': ('src/rendering/renderer/source_scene_device_constructor_tu15_generated.cpp', 'tools/generate_scene_device_constructor_tu15.py'),
    'sandbox_special': ('src/gameplay/objects/source_sandbox_special_tu15_generated.cpp', 'tools/generate_sandbox_special_tu15.py'),
    'effect_particle_draw': ('src/rendering/effects/source_effect_particle_draw_tu15_generated.cpp', 'tools/generate_effect_particle_draw_tu15.py'),
    'ai_frame': ('src/campaign/ai/source_ai_frame_tu15_generated.cpp', 'tools/generate_ai_frame_tu15.py'),
}
GENERATION_PROVENANCE = {}
BODY_PIN = '8fc06927ea71a7373c2f01b38037dfd22c5211a7e11976f9d386000de9ed9dfb'

def digest(value):
    return hashlib.sha256(value.encode()).hexdigest()

def parts(source, owner):
    patterns = {
        'body': r'^static void run_bcdf0\(WorldState &s\) \{.*?^\}',
        'resolver': r'^std::uint32_t selected\([^\n]+',
        'dispatcher': r'^static void dispatch\([^\n]+',
        'state': r'^struct LiveState : physics_world_runtime_detail::WorldState \{.*?^\};\nusing WorldState=LiveState;',
        'sign_extend': r'^std::uint32_t sign_extend\([^\n]+\{.*?^\}',
        'public_entry': r'^void source_run_' + owner + r'_tu15\(.*?^\}',
        'owns_entry': r'^bool source_' + owner + r'_tu15_owns_entry\([^\n]+',
    }
    result = {}
    for key, pattern in patterns.items():
        found = re.findall(pattern, source, re.M | re.S)
        if len(found) != 1:
            raise ValueError('Missing/ambiguous complete ' + owner + ' ' + key)
        result[key] = found[0]
    return result

def project(body, resolver):
    body = re.sub(r's\.instruction_literal\((0x[0-9a-f]+u)\);', r'if(s.services.instruction){s.services.instruction(\1);}', body)
    used = set(re.findall(r'selected\((0x[0-9a-f]+u),s\.image\)', body))
    resolved = dict(re.findall(r'case (0x[0-9a-f]+u):return (0x[0-9a-f]+u);', resolver))
    for value in used:
        if resolved.get(value) != value:
            raise ValueError('Nonidentity actual TU1.5 lookup operand ' + value)
    projected = re.sub(r'selected\((0x[0-9a-f]+u),s\.image\)', r'\1', body)
    if digest(projected) != BODY_PIN:
        raise ValueError('Complete lookup projection differs from the qualified transport owner')
    return projected, sorted(used)

def generate(scratch, owner):
    unit, generator = OWNERS[owner]
    if owner == 'ai_frame':
        from validate_native_camera_instance_bitmap_generation_tu15 import generate_ai
        previous = Path.cwd()
        try:
            # Original hardware-step manifests contain repository-relative
            # paths; CTest starts in its private build directory.
            os.chdir(ROOT)
            command, proof = generate_ai(scratch)
        finally:
            os.chdir(previous)
        GENERATION_PROVENANCE[owner] = proof
        return [(ROOT / unit).read_text(), (scratch / 'ai.cpp').read_text()], command
    target = scratch / (owner + '.cpp')
    command = [sys.executable, '-B', str(ROOT / generator), '--output', str(target), '--directory', str(scratch / (owner + '-generation'))]
    done = subprocess.run(command, capture_output=True, text=True)
    if done.returncode:
        raise ValueError('Genuine ' + owner + ' generation failed: ' + done.stderr[-7000:])
    return [(ROOT / unit).read_text(), target.read_text()], command

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--candidate', action='store_true')
    parser.add_argument('--require-binding', action='store_true')
    args = parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Run through tools/run_test.py')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    args.output.mkdir(parents=True, exist_ok=False)
    names = [name for pair in OWNERS.values() for name in pair]
    names += ['tools/audit_native_object_lookup_copies.py', 'tools/generate_physics_live_support_tu15.py', 'tools/specialize_native_operands.py']
    if args.candidate:
        names += ['tools/native_object_lookup_copies_replacements.py', 'src/gameplay/objects/native_tu15_object_lookup_copies_source_owner.hpp', 'tools/native_object_body_replacements.py']
    pins = {name: hashlib.sha256((ROOT / name).read_bytes()).hexdigest() for name in names}
    report = dict(passed=False, production_bound=False, owned_scratch=str(scratch), source_before=pins, retained_large_artifacts=[])
    try:
        binding = None
        if args.candidate:
            import native_object_lookup_copies_replacements as binding
        contexts = []
        for owner in OWNERS:
            actual, command = generate(scratch, owner)
            if args.require_binding:
                for value in actual:
                    if binding.replace_object_lookup_copies(value) != value:
                        raise ValueError('Current/fresh lookup copy default missing: ' + owner)
                    binding.installed_entries(value)
            values = [binding.remove_object_lookup_copies(value) if binding else value for value in actual]
            values += [specialize(value)[0] for value in values]
            labels = ['current', 'fresh', 'current-prepared', 'fresh-prepared']
            mixed_context_refusals = 0
            if binding and owner == 'ai_frame':
                # Known pieces from distinct real contexts must not form a
                # new admitted resolver/dispatcher/public-entry combination.
                for source in values:
                    original_parts = parts(source, owner)
                    for alternative in values:
                        alternate_parts = parts(alternative, owner)
                        for key in ('resolver', 'dispatcher', 'public_entry'):
                            if original_parts[key] == alternate_parts[key]:
                                continue
                            changed = source.replace(original_parts[key], alternate_parts[key], 1)
                            candidate_parts = parts(changed, owner)
                            candidate_tuple = tuple(digest(value) for value in (
                                binding.original_body(candidate_parts['body']),
                                candidate_parts['resolver'], candidate_parts['dispatcher'],
                                candidate_parts['public_entry']))
                            if candidate_tuple in binding.AI_CONTEXTS:
                                continue
                            try:
                                binding.authenticate(changed)
                            except ValueError:
                                mixed_context_refusals += 1
                            else:
                                raise ValueError('Mixed real AI source context accepted')
                if not mixed_context_refusals:
                    raise ValueError('Missing genuine AI mixed-context controls')
            if binding:
                proposals = []
                for value in values:
                    proposed = binding.replace_object_lookup_copies(value)
                    if proposed == value or binding.replace_object_lookup_copies(proposed) != proposed or binding.remove_object_lookup_copies(proposed) != value:
                        raise ValueError('Exact lookup copy overlay inverse/idempotence drift: ' + owner)
                    proposals.append(proposed)
                values += proposals
                labels += [label + '-candidate' for label in labels]
            for index, value in enumerate(values):
                context = parts(value, owner)
                body = binding.original_body(context['body']) if binding else context['body']
                projection, operands = project(body, context['resolver'])
                refusals = {}
                if binding:
                    for key, part in context.items():
                        at = part.index('{') + 1
                        changed = value.replace(part, part[:at] + '/* context drift */' + part[at:], 1)
                        try:
                            binding.authenticate(changed)
                        except ValueError as error:
                            refusals[key] = str(error)
                        else:
                            raise ValueError('Lookup copy context drift accepted: ' + owner + ' ' + key)
                    if binding.entry_prefix(context['body']) is not None:
                        for pc in binding.FRONTIERS:
                            label = f'NativeObjectBody{pc:x}: '
                            entry = 's.instruction_literal(0xbcdf0u);' if 's.instruction_literal(0xbcdf0u);' in context['body'] else 'if(s.services.instruction){s.services.instruction(0xbcdf0u);}'
                            changed = value.replace(label, '', 1).replace(entry, label + entry, 1)
                            try:
                                binding.authenticate(changed)
                            except ValueError as error:
                                refusals[f'relocated_{pc:x}'] = str(error)
                            else:
                                raise ValueError('Lookup copy relocated continuation accepted: ' + owner)
                target = scratch / f'{owner}-context-{index}.cpp'
                target.write_text(value)
                checks = []
                for compiler in ('g++', 'clang++'):
                    syntax = [compiler, '-std=c++20', '-Wno-ignored-attributes', '-fsyntax-only', '-I' + str(ROOT / 'include'), '-I' + str(ROOT / 'src'), str(target)]
                    done = subprocess.run(syntax, capture_output=True, text=True)
                    if done.returncode:
                        raise ValueError('Complete lookup owner syntax failed: ' + (done.stdout + done.stderr)[-7000:])
                    checks.append(dict(compiler=compiler, command=syntax, passed=True))
                contexts.append(dict(owner=owner, variant=labels[index], whole_source_sha256=digest(value), complete_context_sha256={key:digest(body if key == 'body' else part) for key,part in context.items()}, identity_projection_sha256=digest(projection), exact_identity_operands=operands, whole_cpp_syntax=checks, strict_refusals=refusals, genuine_generation_command=command, mixed_real_context_refusals=mixed_context_refusals))
        report.update(passed=True, contexts=contexts, production_bound=args.require_binding, full_parent_closure=False, genuine_reference_provenance=GENERATION_PROVENANCE)
    except BaseException as error:
        report['error'] = repr(error)
        raise
    finally:
        report['source_after'] = {name:hashlib.sha256((ROOT / name).read_bytes()).hexdigest() for name in names}
        if report['source_after'] != pins:
            report.update(passed=False, source_drift=True)
        (args.output / 'report.json').write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')
    return 0

if __name__ == '__main__':
    raise SystemExit(main())
