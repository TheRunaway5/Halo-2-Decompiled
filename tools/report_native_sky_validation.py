#!/usr/bin/env python3
"""Render bounded native sky evidence; this does not run or change the renderer."""
import argparse
from collections import Counter
import csv
import hashlib
import io
import json
import lzma
import os
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUTPUT_ROOT = ROOT / 'analysis/sky-exact-20261025/full-native-validation'
OUTPUT_ROOTS = (OUTPUT_ROOT, OUTPUT_ROOT.with_name('full-native-validation-v79'),
                OUTPUT_ROOT.with_name('integrated-source-validation'))
LIMIT = 16 * 1024 * 1024
OUTPUT_LIMIT = 1024 * 1024
OUTPUT_NAMES = ('README.md', 'all-map-matrix.csv', 'acceptance.json')
COVERAGE_KEYS = ('counterpart_case_coverage', 'input_comparison_coverage',
                 'pixel_comparison_coverage', 'authored_absence_comparison_coverage')


def fingerprint(data):
    return hashlib.sha256(data).hexdigest()


def read_report(path):
    if path.suffix not in ('.json', '.xz'):
        raise ValueError('Report must be .json or .xz')
    if path.stat().st_size > LIMIT:
        raise ValueError('Encoded report exceeds the bounded 16 MiB input limit')
    with (lzma.open(path, 'rb') if path.suffix == '.xz' else path.open('rb')) as stream:
        raw = stream.read(LIMIT + 1)
    if len(raw) > LIMIT:
        raise ValueError('Decoded report exceeds the bounded 16 MiB input limit')
    return json.loads(raw), raw


def count(value):
    if type(value) is not int or value < 0:
        raise ValueError('Report count must be a nonnegative integer')
    return value


def full_coverage(value):
    return (isinstance(value, dict) and value.get('complete') is True and
            value.get('expected') == value.get('executed') and
            type(value.get('expected')) is int and value['expected'] >= 0 and
            type(value.get('executed')) is int and
            value.get('missing') == [] and value.get('unexpected', []) == [])


def has_reference(case):
    return bool(case.get('original_asset_reference')) and case['original_asset_reference'] != 'unavailable'


def pair_key(entry, bsp, sky):
    name = entry.get('authority_map', Path(entry['path']).stem)
    if entry['source'] == 'prepared-mcc-custom':
        name = 'custom:' + Path(entry['prepared_context']).name + ':' + Path(entry['path']).stem
    return name, bsp, sky


def matrix_scope(entry, cases):
    source = entry['source']
    if source.startswith('original-'):
        return 'original Xbox assets / native renderer'
    scopes = sorted({case.get('comparison_scope', '') for _, case in cases} - {''})
    if scopes:
        return '; '.join(scopes)
    if not cases:
        return 'resource entry / no sky'
    return 'prepared MCC / original Xbox asset counterpart'


def summarize(report):
    if report.get('schema') != 'HALO2_ALL_MAP_SKY_GPU_V1' or not isinstance(report.get('maps'), list):
        raise ValueError('Unsupported all-map sky report schema')
    rows, contexts, unpaired = [], [], []
    derived = Counter()
    flags = dict(used_native_programs=True, zero_instruction_uploads=True,
                 repeat_exact=True, queue_independent=True, image_visibility=True,
                 input_pairs_exact=True, pixel_pairs_exact=True, absence_pairs_exact=True,
                 case_statuses_valid=True, complete_direction_sets=True)
    for entry in report['maps']:
        cases = [(bsp['bsp'], case) for bsp in entry.get('bsps', []) for case in bsp.get('skies', [])]
        rendered = [case for _, case in cases if case.get('directions')]
        absent = [case for _, case in cases if case.get('status') == 'authored-NONE-or-model-less']
        row = dict(map=entry.get('authority_map', Path(entry['path']).stem), source=entry['source'],
                   scope=matrix_scope(entry, cases), status=entry.get('status', 'unknown'),
                   input_path=entry['path'], bsps=len(entry.get('bsps', [])), sky_contexts=len(cases),
                   rendered_sky_contexts=len(rendered), authored_absences=len(absent), directions=0,
                   native_aot_material_bindings=0, vertex_instruction_words=0,
                   native_programs='N/A', repeat_exact='N/A', queue_independent='N/A',
                   paired_inputs=0, differing_inputs=0, paired_pixel_directions=0,
                   differing_pixel_directions=0, paired_authored_absences=0)
        programs_ok = repeats_ok = queue_ok = True
        for bsp, case in cases:
            contexts.append((entry, bsp, case))
            directions = case.get('directions', [])
            if entry['source'].startswith('prepared-') and case.get('comparison_scope'):
                unpaired.append(dict(map=pair_key(entry, bsp, case['ordinal'])[0], bsp=bsp,
                    sky=case['ordinal'], scope=case['comparison_scope'], directions=len(directions),
                    native_control_status=case.get('status', 'unknown')))
            if case.get('status') not in ('native-controls-passed', 'authored-NONE-or-model-less'):
                flags['case_statuses_valid'] = False
            if directions:
                derived['rendered_sky_contexts'] += 1
                flags['complete_direction_sets'] &= (len(directions) == 6 and
                    sorted(d.get('index', -1) for d in directions) == list(range(6)))
                row['directions'] += len(directions)
                used = [m for m in case.get('materials', []) if m.get('used_for_geometry') is True]
                native = count(case.get('native_aot_materials', 0))
                uploaded = count(case.get('uploaded_vertex_instruction_words', 0))
                row['native_aot_material_bindings'] += native
                row['vertex_instruction_words'] += uploaded
                admitted = (case.get('used_material_programs_admitted') is True and bool(used) and
                    native == len(used) and all(m.get('forward_pass') is True and
                    m.get('source_layer') == 15 and m.get('texture_program_supported') is True and
                    count(m.get('vertex_program_bytes', 0)) > 0 for m in used))
                programs_ok &= admitted
                flags['used_native_programs'] &= admitted
                flags['zero_instruction_uploads'] &= ('uploaded_vertex_instruction_words' in case and uploaded == 0)
                repeated = case.get('repeat_exact') is True and all(d.get('repeat_exact') is True for d in directions)
                independent = (case.get('dedicated_sky_independent_of_transparent_queue') is True and
                    all(d.get('dedicated_sky_independent_of_transparent_queue') is True and
                        d.get('queue_toggle', {}).get('byte_exact') is True for d in directions))
                repeats_ok &= repeated
                queue_ok &= independent
                flags['repeat_exact'] &= repeated
                flags['queue_independent'] &= independent
                flags['image_visibility'] &= sum(count(d.get('changed_pixels', 0)) for d in directions) > 0
            if has_reference(case) and directions:
                row['paired_inputs'] += 1
                exact = all(case.get(field) is True for field in
                    ('original_asset_definition_exact', 'original_asset_materials_exact', 'original_asset_posed_geometry_exact'))
                row['differing_inputs'] += not exact
                flags['input_pairs_exact'] &= exact
            if has_reference(case) and case.get('status') == 'authored-NONE-or-model-less':
                row['paired_authored_absences'] += 1
                flags['absence_pairs_exact'] &= case.get('original_asset_absence_exact') is True
            for direction in directions:
                queued = direction.get('original_assets_native_renderer_queued')
                independent = direction.get('original_assets_native_renderer_independent')
                if queued is not None or independent is not None:
                    row['paired_pixel_directions'] += 1
                    exact = (isinstance(queued, dict) and isinstance(independent, dict) and
                             queued.get('byte_exact') is True and independent.get('byte_exact') is True)
                    row['differing_pixel_directions'] += not exact
                    flags['pixel_pairs_exact'] &= exact
        if rendered:
            row.update(native_programs='PASS' if programs_ok else 'FAIL',
                       repeat_exact='PASS' if repeats_ok else 'FAIL',
                       queue_independent='PASS' if queue_ok else 'FAIL')
        for field in ('directions', 'native_aot_material_bindings', 'vertex_instruction_words', 'paired_inputs',
                      'differing_inputs', 'paired_pixel_directions', 'differing_pixel_directions', 'paired_authored_absences'):
            derived[field] += row[field]
        if rendered:
            derived['sky_bearing_map_entries'] += 1
        derived['authored_absences'] += len(absent)
        rows.append(row)
    derived['map_entries'] = len(rows)
    derived['bsp_sky_contexts'] = len(contexts)
    derived['unique_input_paths'] = len({row['input_path'] for row in rows})
    coverage = {key: report.get(key, {}) for key in COVERAGE_KEYS}
    inventories = report.get('prepared_map_inventories', [])
    inventory_ok = bool(inventories) and all(v.get('complete') is True and
        v.get('declared_maps') == v.get('actual_maps') == v.get('expected') == v.get('executed') and
        v.get('executed') == len({m['path'] for m in report['maps'] if
            m.get('source') == v.get('source') and m.get('prepared_context') == v.get('prepared_root')}) and
        v.get('missing') == [] and v.get('unlisted') == [] for v in inventories)
    probes = report.get('native_kernel_negative_probes', [])
    negative_ok = bool(probes) and all(p.get('rejected') is True and
        p.get('expected_exception') == p.get('observed_exception') for p in probes)
    required_mutations = {'missing-vertex-program', 'disabled-forward-program-with-source-owner',
                          'default-material-with-source-geometry'}
    negative_ok &= required_mutations <= {p.get('mutation') for p in probes}
    actual_unpaired = {(v['map'], v['bsp'], v['sky']) for v in unpaired}
    declared_unpaired = {(v['map'], v['bsp'], v['sky']) for v in report.get('unpaired_authored_cases', [])}
    consistency = dict(
        direction_total=derived['directions'] == report.get('cases'),
        sky_map_total=derived['sky_bearing_map_entries'] == report.get('sky_maps'),
        input_total=derived['paired_inputs'] == report.get('original_asset_native_input_pairs'),
        pixel_total=derived['paired_pixel_directions'] == report.get('original_asset_native_pixel_pairs'),
        input_difference_total=derived['differing_inputs'] == report.get('original_asset_native_input_pairs_with_differences'),
        pixel_difference_total=derived['differing_pixel_directions'] == report.get('original_asset_native_pixel_pairs_with_differences'),
        absence_total=derived['paired_authored_absences'] == coverage['authored_absence_comparison_coverage'].get('executed'),
        input_coverage_total=derived['paired_inputs'] == coverage['input_comparison_coverage'].get('executed'),
        pixel_coverage_total=derived['paired_pixel_directions'] == coverage['pixel_comparison_coverage'].get('executed'),
        counterpart_coverage_total=derived['paired_inputs'] + derived['paired_authored_absences'] ==
            coverage['counterpart_case_coverage'].get('executed'),
        unpaired_cases=actual_unpaired == declared_unpaired)
    acceptance_checks = dict(flags, **{'consistent_' + key: value for key, value in consistency.items()},
        source_report_passed=report.get('passed') is True,
        no_report_errors=type(report.get('errors')) is int and report['errors'] == 0,
        native_controls_passed=report.get('native_controls_passed') is True,
        original_asset_inputs_exact=report.get('original_asset_native_inputs_exact') is True,
        original_asset_pixels_exact=report.get('original_asset_native_pixels_exact') is True,
        unfiltered=report.get('filtered') is False and report.get('map_filter') == [],
        prepared_inventory_complete=report.get('prepared_map_inventory_complete') is True and inventory_ok,
        counterpart_coverage_complete=all(full_coverage(value) for value in coverage.values()),
        no_missing_original_cases=report.get('original_cases_without_prepared_map') == [],
        no_unexpected_imported_cases=report.get('unexpected_imported_visible_cases') == [],
        negative_probes_executed=report.get('native_kernel_negative_probes_executed') is True,
        negative_probes_passed=report.get('native_kernel_negative_probes_passed') is True and negative_ok,
        rendered_cases_present=derived['rendered_sky_contexts'] > 0)
    return dict(schema='H2_NATIVE_SKY_VALIDATION_SUMMARY_V1', native_sky_scope_accepted=all(acceptance_checks.values()),
        counts=dict(derived), sources=dict(sorted(Counter(row['source'] for row in rows).items())),
        scope=report.get('scope', ''), pixel_comparison_scope=report.get('pixel_comparison_scope', ''),
        acceptance_checks=acceptance_checks, coverage=coverage, prepared_inventories=inventories,
        negative_controls=dict(executed=len(probes), rejected=sum(p.get('rejected') is True for p in probes),
            mutations=[p.get('mutation', '') for p in probes]), unpaired_authored_cases=unpaired,
        original_reference_priority=report.get('original_reference_priority', ''),
        superseded_original_references=report.get('superseded_original_references', []),
        physical_nv2a=dict(accepted=False, original_framebuffer_parity_reported=report.get('original_framebuffer_parity'),
            physical_xbox_precision_reported=report.get('physical_xbox_precision'),
            scope='This native-renderer report does not establish physical NV2A framebuffer or precision parity.')), rows


def relative_link(path, directory):
    return Path(os.path.relpath(path, directory)).as_posix()


def component_evidence(report_path):
    candidates = (report_path.parent / 'evidence/native-world-fragment-gpu.json',
                  ROOT / 'analysis/sky-exact-20261025/native-fragment/numerical.json')
    for path in candidates:
        if path.is_file():
            data, raw = read_report(path)
            if data.get('schema') != 'H2_NATIVE_WORLD_FRAGMENT_GPU_V1':
                raise ValueError('Unexpected separate fragment numerical report schema')
            fields = ('passed', 'native_fragment_modules', 'probe_cases', 'rgba_channels', 'byte_exact_channels',
                      'maximum_byte_error', 'maximum_allowed_unorm_byte_error', 'device', 'scope',
                      'physical_xbox_precision', 'original_framebuffer_parity')
            return dict(path=str(path.resolve()), sha256=fingerprint(raw), **{field: data.get(field) for field in fields})
    return None


def markdown(summary, source, directory):
    c = summary['counts']
    accepted = 'PASS' if summary['native_sky_scope_accepted'] else 'FAIL'
    lines = ['# Native sky validation', '', f'**{accepted} within the reported native sky scope.**', '',
        f"[Source report]({relative_link(source, directory)}) · [All-map matrix](all-map-matrix.csv) · [Acceptance checks](acceptance.json)", '',
        f"The report enumerates **{c['map_entries']} map/source entries**, including **{c['sky_bearing_map_entries']} sky-bearing entries**. "
        f"It records {c['rendered_sky_contexts']} rendered BSP/sky contexts, {c['authored_absences']} authored absent contexts, "
        f"and **{c['directions']} direction cases**. Map/source entries and BSP/sky contexts are separate counts.", '',
        '| Check | Observed result |', '| --- | --- |',
        f"| Original/prepared input comparisons | {c['paired_inputs']} pairs; {c['differing_inputs']} differing |",
        f"| Paired native-renderer pixel directions | {c['paired_pixel_directions']} pairs; {c['differing_pixel_directions']} differing |",
        f"| Native material selections | {c['native_aot_material_bindings']} admitted bindings across rendered contexts |",
        f"| Uploaded vertex instruction words | {c['vertex_instruction_words']} |",
        f"| Program/fallback negative controls | {summary['negative_controls']['rejected']}/{summary['negative_controls']['executed']} rejected |"]
    for key, label in (('counterpart_case_coverage', 'Counterpart cases'), ('input_comparison_coverage', 'Input coverage'),
                       ('pixel_comparison_coverage', 'Pixel coverage'), ('authored_absence_comparison_coverage', 'Authored absence coverage')):
        value = summary['coverage'][key]
        lines.append(f"| {label} | {value.get('executed', 'unknown')}/{value.get('expected', 'unknown')}; complete={value.get('complete', False)} |")
    lines += ['', 'Prepared inventories:', '']
    for value in summary['prepared_inventories']:
        lines.append(f"- `{value.get('source', 'unknown')}`: {value.get('executed', 'unknown')}/{value.get('expected', 'unknown')} entries; "
                     f"{len(value.get('missing', []))} missing, {len(value.get('unlisted', []))} unlisted.")
    lines += ['', 'Authored inputs without an original Xbox map counterpart:', '',
              '| Map | BSP / sky | Source scope | Native controls |', '| --- | --- | --- | --- |']
    for value in summary['unpaired_authored_cases']:
        lines.append(f"| `{value['map']}` | {value['bsp']} / {value['sky']} | {value['scope']} | {value['native_control_status']} |")
    if not summary['unpaired_authored_cases']:
        lines.append('| None | — | — | — |')
    lines += ['', 'The source-versus-prepared pixel comparisons use the same native renderer. '
              'PC and custom authored cases retain the explicit unpaired scope above. '
              '**Physical Xbox/NV2A framebuffer and precision parity remain unverified.** '
              'This isolated background sweep does not establish full-scene parity.', '']
    component = summary.get('separate_fragment_numerical_component')
    if component:
        path = Path(component['path'])
        state = 'PASS' if component.get('passed') is True else 'FAIL / not accepted'
        lines += [f"A [separate production fragment numerical component]({relative_link(path, directory)}) reports **{state}** and covers "
            f"{component['native_fragment_modules']} modules and {component['rgba_channels']} RGBA channels: "
            f"{component['byte_exact_channels']} channels are byte-exact; maximum error is {component['maximum_byte_error']} UNORM byte "
            f"against its separate CPU numerical reference and a declared bound of {component['maximum_allowed_unorm_byte_error']}. "
            'This is an arithmetic component result, not a byte-exact claim for every kernel or physical NV2A behavior.', '']
    failed = [key for key, passed in summary['acceptance_checks'].items() if not passed]
    if failed:
        lines += ['Failed acceptance checks:', ''] + ['- `' + key + '`' for key in failed] + ['']
    lines += [f"Source report SHA-256: `{summary['source_report']['file_sha256']}`.", '']
    return '\n'.join(lines)


def checked_output_directory(report_path, output):
    output = output.resolve()
    if not any(output.is_relative_to(root.resolve()) for root in OUTPUT_ROOTS):
        raise ValueError('Output must stay under an approved full-native-validation analysis directory')
    for name in OUTPUT_NAMES:
        if (output / name).is_symlink():
            raise ValueError('A report output must not replace or follow a symlink')
        if (output / name).resolve() == report_path.resolve():
            raise ValueError('A generated report must not overwrite its input')
    return output


def render(report_path, output):
    output = checked_output_directory(report_path, output)
    report, raw = read_report(report_path)
    summary, rows = summarize(report)
    summary['source_report'] = dict(path=str(report_path.resolve()), file_sha256=fingerprint(report_path.read_bytes()),
        decoded_sha256=fingerprint(raw), schema=report['schema'])
    summary['generator'] = dict(path='tools/report_native_sky_validation.py',
                              sha256=fingerprint(Path(__file__).read_bytes()))
    summary['separate_fragment_numerical_component'] = component_evidence(report_path)
    table = io.StringIO(newline='')
    writer = csv.DictWriter(table, fieldnames=list(rows[0]) if rows else ['map', 'source', 'scope', 'status'])
    writer.writeheader()
    writer.writerows(rows)
    contents = {'README.md': markdown(summary, report_path.resolve(), output),
                'all-map-matrix.csv': table.getvalue(), 'acceptance.json': json.dumps(summary, indent=2) + '\n'}
    encoded = {name: value.encode('utf-8') for name, value in contents.items()}
    if any(len(value) > OUTPUT_LIMIT for value in encoded.values()):
        raise ValueError('A generated report exceeds the bounded 1 MiB output limit')
    output.mkdir(parents=True, exist_ok=True)
    for name in OUTPUT_NAMES:
        (output / name).write_bytes(encoded[name])
    return summary


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    try:
        summary = render(args.report.resolve(strict=True), args.output)
    except (OSError, ValueError, KeyError, TypeError, lzma.LZMAError) as error:
        parser.exit(2, f'{error}\n')
    print(json.dumps(dict(native_sky_scope_accepted=summary['native_sky_scope_accepted'],
                          counts=summary['counts'], output=str(args.output.resolve()))))
    return 0 if summary['native_sky_scope_accepted'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
