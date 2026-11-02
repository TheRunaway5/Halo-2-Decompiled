"""Check zero-factor tint independence in the authenticated authored expression.

This is bounded symbolic dependency analysis for finite inputs. No HLSL is
compiled, loaded into the engine or submitted to a GPU. The nonzero-factor
control must retain the tint dependency. Rendered/native equivalence remains
a separate gate. Run through run_test.py; retain only the small receipt.
"""
import argparse
import ast
from dataclasses import dataclass
import hashlib
import json
import os
from pathlib import Path
import re


@dataclass(frozen=True)
class Value:
    constant: int | None = None
    tint: bool = False


def need(ok, why):
    if not ok:
        raise ValueError('Authored midrange expression: ' + why)


def analyze(lines, factor):
    state = {'c2.a': Value(factor), 'c3.rgb': Value(tint=True)}
    statements = 0

    def read(name):
        if name in state:
            return state[name]
        if '.' in name:
            parent, component = name.split('.')
            if parent in state:
                return state[parent]
            if component in 'rgb' and parent + '.rgb' in state:
                return state[parent + '.rgb']
        return Value()

    def expression(node):
        if isinstance(node, ast.Constant):
            need(type(node.value) is int, 'unsupported literal')
            return Value(node.value)
        if isinstance(node, ast.Name):
            return read(node.id)
        if isinstance(node, ast.Attribute) and isinstance(node.value, ast.Name):
            return read(node.value.id + '.' + node.attr)
        if isinstance(node, ast.BinOp):
            left, right = expression(node.left), expression(node.right)
            if isinstance(node.op, ast.Mult):
                if left.constant == 0 or right.constant == 0:
                    return Value(0)
                constant = left.constant * right.constant if left.constant is not None and right.constant is not None else None
            elif isinstance(node.op, (ast.Add, ast.Sub)):
                constant = (left.constant + right.constant if isinstance(node.op, ast.Add) else left.constant - right.constant
                            ) if left.constant is not None and right.constant is not None else None
            else:
                raise ValueError('Unsupported authored arithmetic operation')
            return Value(constant, left.tint or right.tint)
        if isinstance(node, ast.Call) and isinstance(node.func, ast.Name):
            name = node.func.id
            need(name in ('saturate', 'round', 'dot', 'lerp') and not node.keywords, 'unsupported authored function')
            values = [expression(arg) for arg in node.args]
            need(len(values) == {'saturate': 1, 'round': 1, 'dot': 2, 'lerp': 3}[name], 'function arity differs')
            if name in ('saturate', 'round') and values[0].constant == 0:
                return Value(0)
            if name == 'lerp' and values[0] == values[1] and values[0].constant is not None:
                return values[0]
            return Value(tint=any(value.tint for value in values))
        raise ValueError('Unsupported authored syntax')

    for line in lines:
        text = line.split('//', 1)[0].strip()
        if not text or re.fullmatch(r'float[134]?\s+[A-Za-z_][A-Za-z_0-9]*;', text):
            continue
        match = re.fullmatch(r'([A-Za-z_][A-Za-z_0-9]*(?:\.[rgba]+)?)\s*=\s*(.+);', text)
        need(match is not None, 'unowned statement in arithmetic interval')
        state[match[1]] = expression(ast.parse(match[2], mode='eval').body)
        statements += 1
    return dict(statements=statements, tint_reaches_color=state['r0.rgb'].tint,
                tint_reaches_alpha=state['r0.a'].tint, intermediate_tint_weight_zero=state['r1.a'].constant == 0)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--bindings', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    owner = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    metadata = json.loads(args.bindings.read_text())
    key = r'shaders\shader_passes\environment_mapping\default_environment_map_tangent_space:0'
    item = metadata['textual'][key]
    named = {row['parameter']: (row['register'], row['component']) for row in item['declarations']}
    need(named['env_midrange_tint_factor'] == (2, 'a') and named['env_midrange_tint_color'] == (3, 'rgb'),
         'explicit named register declarations differ')
    path = Path(item['path'])
    raw = path.read_bytes()
    need(hashlib.sha256(raw).hexdigest() == item['sha256'], 'complete authored input changed')
    lines = raw.decode('ascii').splitlines()
    start = next(index for index, line in enumerate(lines) if line.strip().startswith('// exp.components='))
    end = next(index for index, line in enumerate(lines) if line.strip() == '//fog zvalue')
    need(start < end, 'arithmetic interval is not bounded')
    uncommented = '\n'.join(line.split('//', 1)[0] for line in lines)
    need(len(re.findall(r'\bc2\b', uncommented)) == 3 and len(re.findall(r'\bc3\b', uncommented)) == 2 and
         len(re.findall(r'\bc2\.a\b', uncommented)) == 2 and len(re.findall(r'\bc3\.rgb\b', uncommented)) == 1,
         'named constants have an additional unowned use')
    zero, active = analyze(lines[start:end], 0), analyze(lines[start:end], 1)
    need(zero['intermediate_tint_weight_zero'] and not zero['tint_reaches_color'] and not zero['tint_reaches_alpha'],
         'zero-factor color remains active')
    need(active['tint_reaches_color'] and not active['tint_reaches_alpha'], 'nonzero-factor dependency control is ineffective')
    need(path.read_bytes() == raw, 'authored input changed during analysis')
    report = dict(scope=__doc__, passed=True, input_path=str(path), input_sha256=item['sha256'],
        helper_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        bindings_sha256=hashlib.sha256(args.bindings.read_bytes()).hexdigest(),
        arithmetic_line_interval=[start + 1, end], zero_factor=zero, nonzero_factor_control=active,
        finite_inputs_only=True, native_projection_admitted=False,
        owned_scratch=str(owner), retained_large_artifacts=[])
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(passed=True, zero_factor=zero, nonzero_factor_control=active)))


if __name__ == '__main__':
    main()
