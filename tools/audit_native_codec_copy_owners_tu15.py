#!/usr/bin/env python3
"""Authenticate the original first codec triple and its genuine producer contexts."""
import argparse
import hashlib
import json
import os
import re
import subprocess
import sys
from pathlib import Path

import native_codec_copy_replacements as binding
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from specialize_native_operands import specialize
from validate_native_codec_row_copy_tu15 import PINS as PAIR_PINS, UNITS
from validate_native_codec_scale_row_tu15 import PINS as SCALE_PINS
from validate_native_model_marker_lookup_generation_tu15 import CHILD_WRAPPER

ROOT = Path(__file__).resolve().parents[1]
ENTRIES = ((0x2963D0, 0x296410), (0x2963D0, 0x296410, 0x2CEC60))
CONTEXT_PATTERNS = (
    r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',
    r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',
    r'struct LiveState : physics_world_runtime_detail::WorldState \{.*?\n\};\nusing WorldState=LiveState;',
)
ANCHOR = '#include "halo2/source_presented_model_pose_tu15.hpp"\n'
CALL = r'if\(s\.services\.instruction\)\{s\.services\.instruction\((0x[0-9a-f]+u)\);\}'
DISPATCH_PINS = {
    ('primary', 'current', 'raw'): '5901d2c9c67421f4dabe988f833de38ce4a22dd4cb0778fbf19eb0fd0cb55ce0',
    ('primary', 'current', 'prepared'): '37477844625e78f7f747e917b6bc039f03390bfaa5419a9843557f0af9e17052',
    ('primary', 'fresh', 'raw'): 'aba69140bdfbfe48111ef3727b7bb22a4891a8daa06770aabcb75183386d5849',
    ('primary', 'fresh', 'prepared'): '8aee54bced297db8c7b162c84479071889c2bfc03472376f8a6cba0b7f68b851',
    ('model', 'current', 'raw'): 'f928af6956a3965aef767189095b9732744dcbdb209a345a3aec57baa618e0d1',
    ('model', 'current', 'prepared'): 'c112d857c78bf307a089c1297c9b000870c293149fd78edd9852012887dd439d',
    ('model', 'fresh', 'raw'): '4af1a5ed426380ed605d661066155bd625fd9f3a1add625cedf00fdc17f15604',
    ('model', 'fresh', 'prepared'): '4af1a5ed426380ed605d661066155bd625fd9f3a1add625cedf00fdc17f15604',
}


def digest(value):
    return hashlib.sha256(value.encode() if isinstance(value, str) else value).hexdigest()


def body_pattern(entry):
    return rf'static void run_{entry:x}\(WorldState &s\) \{{.*?\n\}}'


def parts(text, entries):
    matches = [re.findall(pattern, text, re.S) for pattern in (*map(body_pattern, entries), *CONTEXT_PATTERNS)]
    if any(len(values) != 1 for values in matches):
        raise ValueError('Codec copy complete owner/context cardinality drift')
    return tuple(values[0] for values in matches)


def identity_projection(body, resolver):
    mapping = {int(a, 16): int(b, 16) for a, b in re.findall(r'case 0x([0-9a-f]+)u:return 0x([0-9a-f]+)u;', resolver)}

    def identity(match):
        selected = int(match[1][:-1], 16)
        if mapping.get(selected) != selected:
            raise ValueError(f'Codec copy operand is not TU1.5 identity: {selected:#x}')
        return match[1]

    return re.sub(r'selected\((0x[0-9a-f]+u),s\.image\)', identity, body)


def raw_callbacks(body):
    return re.sub(r's\.instruction_literal\((0x[0-9a-f]+u)\);', r'if(s.services.instruction){s.services.instruction(\1);}', body)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--xbe', type=Path, default=NEW_DEFAULT)
    args = parser.parse_args()
    if 'HALO2_TEST_SCRATCH' not in os.environ:
        parser.error('Use tools/run_test.py')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    args.output.mkdir(parents=True, exist_ok=False)
    before = {unit: digest((ROOT / unit).read_bytes()) for unit in UNITS}
    tool_hashes = {str(path.relative_to(ROOT)): digest(path.read_bytes()) for path in (ROOT / 'tools').rglob('*.py')}
    used = set()
    report = dict(passed=False, private_unbound=True, owned_scratch=str(scratch), retained_large_artifacts=[], unit_before=before, owners=[])
    try:
        if digest(args.xbe.read_bytes()) != NEW_PIN:
            raise ValueError('Original TU1.5 XBE changed')
        code = _Code(Xbe(args.xbe), '.text')
        sites = {entry: sorted(_trace(code, entry, {entry}).code) for entry in ENTRIES[1]}
        if list(map(len, sites.values())) != [18, 17, 10]:
            raise ValueError('Original codec copy entry sites changed')
        report['original_sites'] = {hex(entry): [dict(pc=hex(pc), bytes=bytes(code.at(pc).bytes).hex()) for pc in values] for entry, values in sites.items()}
        for index, (owner, name, script, arg) in enumerate(zip(
            ('primary', 'model'), UNITS,
            ('generate_physics_live_support_tu15.py', 'generate_model_render_tu15.py'),
            ('--manifest', '--directory'), strict=True,
        )):
            entries = ENTRIES[index]
            current = (ROOT / name).read_text()
            baseline = parts(binding.remove_codec_copy(current), entries)
            if tuple(map(digest, baseline[:2] + baseline[-3:])) != PAIR_PINS[index] or (
                index == 1 and tuple(map(digest, baseline[2:])) != SCALE_PINS[0]
            ):
                raise ValueError('Authenticated original codec copy body/context changed')
            projections = tuple(identity_projection(body, baseline[-3]) for body in baseline[:-3])
            fresh_path = scratch / (owner + '-fresh.cpp')
            imports = scratch / (owner + '-imports.json')
            command = [sys.executable, '-B', '-c', CHILD_WRAPPER, str(imports), str(ROOT / 'tools' / script),
                       '--output', str(fresh_path), arg, str(scratch / (owner + '-generation'))]
            done = subprocess.run(command, capture_output=True, text=True, timeout=300)
            if done.returncode:
                raise ValueError('Genuine codec copy generation failed: ' + (done.stdout + done.stderr)[-7000:])
            used.update(str(Path(value).relative_to(ROOT)) for value in json.loads(imports.read_text()) if Path(value).is_relative_to(ROOT / 'tools'))
            fresh = fresh_path.read_text()
            fresh_parts = parts(binding.remove_codec_copy(fresh), entries)
            if fresh_parts[:-3] != projections or fresh_parts[-3] != baseline[-3]:
                raise ValueError('Fresh complete codec body/resolver differs from identity projection')
            records = []
            for stage, source in (('current', current), ('fresh', fresh)):
                prepared, _ = specialize(source)
                for form, text in (('raw', source), ('prepared', prepared)):
                    values = parts(binding.remove_codec_copy(text), entries)
                    if text.count(ANCHOR) != 1 or text.index(ANCHOR) > text.index('namespace halo2 {'):
                        raise ValueError('Private codec copy include anchor is ambiguous')
                    if values[-3] != baseline[-3]:
                        raise ValueError('Codec copy resolver changed in preparation')
                    if digest(values[-2]) != DISPATCH_PINS[(owner, stage, form)]:
                        raise ValueError('Codec copy complete dispatcher changed in preparation')
                    state_pin = PAIR_PINS[1][-1] if stage == 'fresh' else PAIR_PINS[index][-1]
                    if digest(values[-1]) != state_pin:
                        raise ValueError('Codec copy complete State wrapper changed in preparation')
                    for entry, body, projection in zip(entries, values[:-3], projections, strict=True):
                        raw = raw_callbacks(body)
                        if identity_projection(raw, values[-3]) != projection:
                            raise ValueError(f'Codec copy complete {entry:x} projection changed')
                        if [int(pc[:-1], 16) for pc in re.findall(CALL, raw)] != sites[entry]:
                            raise ValueError(f'Codec copy {entry:x} original instruction order changed')
                        if ('s.instruction_literal(' in body) != (form == 'prepared'):
                            raise ValueError('Codec copy format/literal mismatch')
                        clauses = (f'case 0x{entry:x}u:run_{entry:x}(s);return;',
                                   f'if(target==selected(0x{entry:x}u,s.image)){{run_{entry:x}(s);return;}}',
                                   f'if(target==0x{entry:x}u){{run_{entry:x}(s);return;}}')
                        if sum(values[-2].count(clause) for clause in clauses) != 1:
                            raise ValueError(f'Codec copy actual dispatch target changed {entry:x}')
                    path = scratch / f'{owner}-{stage}-{form}.cpp'
                    path.write_text(text)
                    records.append(dict(stage=stage, form=form, context=list(map(digest, values)), source_sha256=digest(text)))
            report['owners'].append(dict(owner=owner, source=name, entries=list(map(hex, entries)), variants=records, generator=script))
        report['passed'] = True
    except BaseException as error:
        report['error'] = repr(error)
        raise
    finally:
        used.update(str(Path(module.__file__).resolve().relative_to(ROOT)) for module in tuple(sys.modules.values())
                    if getattr(module, '__file__', None) and Path(module.__file__).resolve().is_relative_to(ROOT / 'tools'))
        report['tool_before'] = {name: tool_hashes[name] for name in sorted(used)}
        report['tool_after'] = {name: digest((ROOT / name).read_bytes()) for name in report['tool_before']}
        report['unit_after'] = {unit: digest((ROOT / unit).read_bytes()) for unit in UNITS}
        if report['tool_before'] != report['tool_after'] or before != report['unit_after']:
            report.update(passed=False, source_drift=True)
        (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
