"""Extract named authoring bindings without retaining shader instructions.

H2EK supplies both editable pass data and generated textual .psh files. Only
explicit parameter/constant declarations are retained. Original combiner stage
numbers are used only before the first conditional directive; this reader does
not flatten branches, execute programs or qualify native behavior.
"""
import argparse
import json
import os
from pathlib import Path
import re

from audit_h2ek_shader_roles import Reader, need, sha, word


def combiner_bindings(raw):
    stage, prefix = -1, True
    bindings = []
    for line_number, line in enumerate(raw.decode('ascii').splitlines(), 1):
        text = line.split('//', 1)[0].strip()
        if text.startswith('#'):
            prefix = False
        if text == '---':
            stage += 1
        match = re.fullmatch(r'C([01])(a?)\s*=\s*\$([A-Za-z_][A-Za-z_0-9]*)', text)
        if match:
            bindings.append(dict(line=line_number, constant_bank=int(match[1]),
                component='alpha' if match[2] else 'rgb', parameter=match[3],
                prefix_combiner_index=stage if prefix else None))
        elif re.match(r'C[01]a?\s*=', text) and '$' in text:
            raise ValueError('Unparsed parameter assignment at line ' + str(line_number))
    return bindings


class BindingReader(Reader):
    def data(self, data, at, code=False):
        raw = self.raw[self.at:self.at + word(data, at)] if code else None
        result = super().data(data, at, code)
        if code:
            result['named_parameter_bindings'] = combiner_bindings(raw)
        return result


def psh_bindings(path):
    raw = path.read_bytes()
    need(len(raw) <= 1024 * 1024, 'text shader metadata exceeds budget')
    declarations = []
    for line_number, line in enumerate(raw.decode('ascii').splitlines(), 1):
        if not line.startswith('#const '):
            continue
        match = re.fullmatch(r'#const C(\d+)\.([rgba]+)\s*=\s*\$([A-Za-z_][A-Za-z_0-9]*)', line.strip())
        need(match is not None, 'unknown named shader constant declaration: ' + line.strip())
        declarations.append(dict(line=line_number, register=int(match[1]),
                                 component=match[2], parameter=match[3]))
    need(path.read_bytes() == raw, 'shader metadata input changed')
    return dict(path=str(path), bytes=len(raw), sha256=sha(raw), declarations=declarations)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--h2ek-root', type=Path, required=True)
    parser.add_argument('--authored-roles', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    owner = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    previous = json.loads(args.authored_roles.read_text())
    need(previous['passed'], 'editable roles did not decode')
    tags = (args.h2ek_root / 'tags').resolve(strict=True)
    names = set(previous['passes']) | {r'shaders\shader_passes\texture_mapping\tiling_specular_zwrite'}
    editable, textual = {}, {}
    for name in sorted(names):
        path = tags / (name.replace('\\', '/') + '.shader_pass')
        if path.is_file():
            decoded = BindingReader(path, 'spas').shader_pass()
            need(decoded['sha256'] == previous['passes'][name]['sha256'], 'editable pass changed')
            editable[name] = decoded
        prefix = name.replace('\\', '_') + '_'
        candidates = sorted((tags / 'rasterizer/pixel_shaders_dx9').glob(prefix + '*.psh'))
        for candidate in candidates:
            suffix = candidate.stem[len(prefix):]
            # A distinct pass such as *_fast_0 shares this filename prefix.
            if not suffix.isdecimal():
                continue
            textual[name + ':' + suffix] = psh_bindings(candidate)
    report = dict(scope=__doc__, passed=True, editable=editable, textual=textual,
        helper_sha256=sha(Path(__file__).read_bytes()), authored_roles_sha256=sha(args.authored_roles.read_bytes()),
        shader_programs_imported=False, native_role_mapping_admitted=False,
        owned_scratch=str(owner), retained_large_artifacts=[])
    raw = json.dumps(report, indent=2) + '\n'
    need(len(raw.encode()) <= 1024 * 1024 and not args.report.resolve().is_relative_to(owner), 'report budget/owner differs')
    args.report.write_text(raw)
    print(json.dumps(dict(passed=True, editable_passes=len(editable), textual_implementations=len(textual),
        named_textual_bindings=sum(len(item['declarations']) for item in textual.values()))))


if __name__ == '__main__':
    main()
