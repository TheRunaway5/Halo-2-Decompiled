#!/usr/bin/env python3
"""Reproduce selected native cutscene modules from the pinned TU1.5 bytes.

This proves reproducible source recovery for the named complete CPU bodies,
not full engine coverage, behavior equivalence, final pixels or an installed
runtime. Run each selection through tag_match_memory_budget.py. Results never
replace production source or prior evidence.
"""
import argparse
import gc
import hashlib
import importlib
import json
import re
import sys
from pathlib import Path

MODULES = {
    'cinematic': ('generate_campaign_cinematic_tu15', 'source_campaign_cinematic_tu15_generated.cpp'),
    'completion': ('generate_campaign_completion_tu15', 'source_campaign_completion_tu15_generated.cpp'),
    'activation': ('generate_campaign_activation_tu15', 'source_campaign_activation_tu15_generated.cpp'),
    'audio-frame': ('generate_campaign_audio_tu15', 'source_campaign_audio_tu15_generated.cpp'),
    'sound-dsp-image': ('generate_sound_dsp_image_tu15', 'source_sound_dsp_image_tu15_generated.cpp'),
    'camera': ('generate_campaign_camera_tu15', 'source_campaign_camera_tu15_generated.cpp'),
    'screen': ('generate_campaign_screen_tu15', 'source_campaign_screen_tu15_generated.cpp'),
    'emotion': ('generate_unit_cinematic_tu15', 'source_unit_cinematic_tu15_generated.cpp'),
    'speech': ('generate_unit_speech_tu15', 'source_unit_speech_tu15_generated.cpp'),
    'object-sound': ('generate_object_sound_tu15', 'source_object_sound_tu15_generated.cpp'),
    'shadow-control': ('generate_cinematic_shadow_control_tu15', 'source_cinematic_shadow_control_tu15_generated.cpp'),
}


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def instruction_text(source):
    """Keep every emitted instruction line, including labels and FP state changes."""
    result = {}
    for line in source.splitlines():
        marker = re.search(r'if\(s\.services\.instruction\)\{s\.services\.instruction\(0x([0-9a-f]+)u\);\}', line)
        if marker:
            site = int(marker.group(1), 16)
            if site in result:
                raise ValueError(f'Duplicate emitted source instruction {site:08x}')
            result[site] = line
    return result


def normalize_identity_dispatch(source, entries):
    """Prove the two exact generator forms before comparing instruction text.

    Older TU1.5-only output routes literals through a checked identity lookup
    and an if-chain. Current output uses the same literals and a switch. No
    arithmetic, state access, instruction label or other text is normalized.
    """
    marker = 'std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage image)'
    line = next(line for line in source.splitlines() if line.startswith(marker))
    identity = (marker + '{if(image==SourcePhysicsContactImage::MapPack11)return p;switch(p){')
    suffix = 'default:throw std::runtime_error("Unbound original contact executable address");}}'
    if not line.startswith(identity) or not line.endswith(suffix):
        raise ValueError('Unrecognized executable-address lookup; leave the difference unresolved')
    cases = line[len(identity):-len(suffix)]
    pairs = re.findall(r'case (0x[0-9a-f]+u):return (0x[0-9a-f]+u);', cases)
    if ''.join('case ' + a + ':return ' + b + ';' for a, b in pairs) != cases or any(a != b for a, b in pairs):
        raise ValueError('Executable-address lookup is not an explicit identity map')
    allowed = {a for a, _ in pairs}
    used = set(re.findall(r'selected\((0x[0-9a-f]+u),s.image\)', source))
    if not used <= allowed:
        raise ValueError('A selected literal is absent from the authenticated identity map')
    source = re.sub(r'selected\((0x[0-9a-f]+u),s.image\)', r'\1', source)
    prefix = 'static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail){'
    ordered = sorted(entries.values())
    old = prefix + ''.join(f'if(target==0x{p:x}u){{run_{p:x}(s);return;}}' for p in ordered) + 's.required(site,target,tail);}'
    new = prefix + 'switch(target){' + ''.join(f'case 0x{p:x}u:run_{p:x}(s);return;' for p in ordered) + 'default:s.required(site,target,tail);return;}}'
    dispatch = next(line for line in source.splitlines() if line.startswith(prefix))
    if dispatch not in (old, new):
        raise ValueError('Unrecognized native dispatch; leave the difference unresolved')
    return source.replace(dispatch, new, 1)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--module', choices=MODULES, action='append', required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--sources', type=Path,
                        help='Explicit source directory for an independent candidate or failing control')
    args = parser.parse_args()
    if args.output.exists():
        raise ValueError('Preserve prior provenance evidence; use a fresh directory')
    root = Path(__file__).resolve().parents[1]
    args.output.mkdir(parents=True)
    rows = []
    for name in dict.fromkeys(args.module):
        generator_name, source_name = MODULES[name]
        module = importlib.import_module(generator_name)
        directory = args.output / name
        directory.mkdir()
        generated = directory / source_name
        saved_argv = sys.argv
        try:
            sys.argv = [str(root / 'tools' / (generator_name + '.py')),
                        '--output', str(generated), '--directory', str(directory / 'evidence')]
            module.main()
        finally:
            sys.argv = saved_argv
        bodies = json.loads((directory / 'evidence' / 'bodies.json').read_text())
        generation = json.loads((directory / 'evidence' / 'generation.json').read_text())
        source = (args.sources if args.sources else root / 'src') / source_name
        source_bytes, generated_bytes = source.read_bytes(), generated.read_bytes()
        literal_source = normalize_identity_dispatch(source_bytes.decode(), bodies['entries'])
        literal_generated = normalize_identity_dispatch(generated_bytes.decode(), bodies['entries'])
        source_lines = instruction_text(literal_source)
        generated_lines = instruction_text(literal_generated)
        instructions = generation['lowered_instructions']
        if set(instructions) != set(bodies['instruction_map']):
            raise ValueError('Lowered instruction coverage differs from the recovered bodies')
        if set(generated_lines) != {int(site, 16) for site in instructions}:
            raise ValueError('Emitted instruction lines differ from the recovered source bytes')
        changed = sorted(site for site in source_lines.keys() & generated_lines.keys()
                         if source_lines[site] != generated_lines[site])
        recovered_bodies = {}
        for entry, sites in bodies['body_sites'].items():
            retained = [(pc, bytes.fromhex(instructions[f'{pc:08x}']['source_bytes'])) for pc in sites]
            recovered_bodies[entry] = dict(
                instructions=len(retained), original_bytes=sum(len(data) for _, data in retained),
                address_and_instruction_bytes_sha256=hashlib.sha256(b''.join(
                    pc.to_bytes(4, 'little') + len(data).to_bytes(1, 'little') + data
                    for pc, data in retained)).hexdigest())
        row = dict(module=name, original_sha256=bodies['original_sha256'],
                   entries=bodies['entries'], instructions=len(bodies['instruction_map']),
                   recovered_bodies=recovered_bodies,
                   original_bytes=sum(len(bytes.fromhex(i['source_bytes'])) for i in instructions.values()),
                   external_owners=generation['external_owners'],
                   external_owners_verified_by_this_audit=False,
                   generator_sha256=digest(root / 'tools' / (generator_name + '.py')),
                   production_source=str(source.resolve()), production_sha256=hashlib.sha256(source_bytes).hexdigest(),
                   reproduced_sha256=hashlib.sha256(generated_bytes).hexdigest(),
                   emitted_instruction_comparison=dict(
                       identical=len(source_lines.keys() & generated_lines.keys()) - len(changed),
                       changed=[f'{site:08x}' for site in changed],
                       missing=[f'{site:08x}' for site in sorted(generated_lines.keys() - source_lines.keys())],
                       unexpected=[f'{site:08x}' for site in sorted(source_lines.keys() - generated_lines.keys())],
                       includes_all_helper_text=False),
                   byte_identical_generated_source=source_bytes == generated_bytes,
                   identity_lookup_and_dispatch_verified=True,
                   identical_after_verified_identity_dispatch=literal_source == literal_generated)
        rows.append(row)
        report = dict(scope=__doc__, modules=rows,
                      selected_module_recovery_passed=all(r['identical_after_verified_identity_dispatch'] for r in rows),
                      complete_cutscene_engine_verified=False)
        (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
        print(json.dumps(row), flush=True)
        del bodies, generation, instructions, source_bytes, generated_bytes
        gc.collect()
    if not all(r['identical_after_verified_identity_dispatch'] for r in rows):
        raise SystemExit('Current source differs from its reproduced original-byte lowering')


if __name__ == '__main__':
    main()
