"""Read alpha/environment parameter declarations and complete pass metadata.

Existing H2EK bytes supply authoring names, not automatic native behavior.
Compiled original/MCC metadata and immutable original identities are captured
separately. Shader instructions are skipped/hashed; only explicit named
constant declarations are retained. Run through run_test.py.
"""
import argparse
import json
import lzma
import os
from pathlib import Path

from audit_h2ek_shader_roles import Reader, sha, need
from audit_h2ek_shader_bindings import BindingReader, psh_bindings
from mcc_env_illum_projection import native_graph


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--authority', type=Path, required=True)
    parser.add_argument('--h2ek-root', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    owner = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    raw = args.authority.read_bytes()
    data = json.loads(lzma.decompress(raw))
    need(data['template'] == r'shaders\shader_templates\opaque\tex_bump_env_alpha_test' and
         len(data['materials']) == 88 and not data['errors'] and len(data['stems']) == 1, 'authority census differs')
    tags = (args.h2ek_root / 'tags').resolve(strict=True)
    template = Reader(tags / (data['template'].replace('\\', '/') + '.shader_template'), 'stem').template()
    source = next(iter(data['stems'].values()))['graph']['definitions'][0]['arrays']
    names = {name for group, name in source[2]['typed_references']}
    variants = []
    for name, meta in data['native_libraries'].items():
        path = Path(meta['path'])
        native_raw = path.read_bytes()
        need(sha(native_raw) == meta['sha256'], 'original native library changed')
        for record in json.loads(native_raw)['shader_tags']:
            if record['group'] != 'stem' or record['name'] != data['template']:
                continue
            graph = native_graph(path.parent, record)
            names.update(pass_name for group, pass_name in graph[2]['typed_references'])
            variants.append(dict(map=name, payload=record['payload'], sha256=sha(json.dumps(graph, sort_keys=True).encode())))
    need(len(variants) == 38 and len({row['sha256'] for row in variants}) == 1, 'original variant graphs differ')
    editable, textual, missing = {}, {}, []
    for name in sorted(names):
        path = tags / (name.replace('\\', '/') + '.shader_pass')
        if path.is_file():
            editable[name] = BindingReader(path, 'spas').shader_pass()
        else:
            missing.append(name)
        prefix = name.replace('\\', '_') + '_'
        for path in sorted((tags / 'rasterizer/pixel_shaders_dx9').glob(prefix + '*.psh')):
            suffix = path.stem[len(prefix):]
            if suffix.isdecimal():
                textual[name + ':' + suffix] = psh_bindings(path)
    report = dict(scope=__doc__, passed=True, template=template, editable=editable, textual=textual,
        missing_editable_passes=missing, native_variants=variants, authority_sha256=sha(raw),
        helper_sha256=sha(Path(__file__).read_bytes()), shader_programs_imported=False,
        native_role_mapping_admitted=False, owned_scratch=str(owner), retained_large_artifacts=[])
    raw = json.dumps(report, indent=2) + '\n'
    need(len(raw.encode()) <= 1024 * 1024 and not args.report.resolve().is_relative_to(owner), 'report budget/owner differs')
    args.report.write_text(raw)
    print(json.dumps(dict(passed=True, editable_passes=len(editable), textual_implementations=len(textual), missing=missing)))


if __name__ == '__main__':
    main()
