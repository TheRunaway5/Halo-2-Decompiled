#!/usr/bin/env python3
"""Lower pinned146100 SH polynomials to native scalar SSA, without CPU state.

The production output has native float/extended locals, structured degree
exits and typed square-matrix calls. Virtual registers, flags, stack-machine
execution and instruction callbacks exist only in the authenticated input.
"""
import argparse
import hashlib
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parent.parent
PIN = 'aaf6838f73e44fd434a12f52006dc855dfee0d5adac0a1172e2d50efc788cabf'
SOURCE = ROOT / 'src/rendering/models/source_model_render_tu15_generated.cpp'
OUTPUT = ROOT / 'src/rendering/models/native_tu15_prt_rotation_polynomial.hpp'


def arguments(text):
    values, depth, start = [], 0, 0
    for index, char in enumerate(text):
        if char == '(':
            depth += 1
        elif char == ')':
            depth -= 1
        elif char == ',' and not depth:
            values.append(text[start:index]); start = index + 1
    values.append(text[start:])
    if depth:
        raise ValueError('Unbalanced pinned polynomial expression')
    return values


def call(text, name):
    prefix = name + '('
    if not text.startswith(prefix) or not text.endswith(')'):
        raise ValueError('Unsupported pinned polynomial call: ' + text)
    return arguments(text[len(prefix):-1])


class Polynomial:
    def __init__(self):
        self.address = ['input.matrix', 'input.coefficients', None, 'input.saved_bx',
                        'input.stack', 'input.saved_bp', 'input.output', 'input.saved_di']
        self.scalar = [None] * 8
        self.extended = []
        self.c1 = 'input.x87_c1'
        self.lines = []
        self.ordinal = 0
        self.order = None
        self.gates = 0

    def value(self, kind, expression):
        name = f'{kind}_{self.ordinal}'
        self.ordinal += 1
        self.lines.append(f'    const auto {name} = {expression};')
        return name

    def refs(self, text):
        def address(match):
            value = self.address[int(match[1])]
            if value is None:
                raise ValueError('Polynomial reads an unowned incoming address')
            return value
        text = re.sub(r'selected\((0x[0-9a-f]+u),s.image\)', r'\1', text)
        text = re.sub(r's\.reg\((\d),4,0\)', address, text)
        text = re.sub(r's\.gp\[(\d)\]', address, text)
        text = re.sub(r's\.xmm\[(\d)\]', lambda m: self.scalar[int(m[1])] or
                      (_ for _ in ()).throw(ValueError('Unowned incoming SIMD lane')), text)
        text = re.sub(r's\.fp\.st\((\d)\)', lambda m: self.extended[int(m[1])], text)
        return text

    def expression(self, text):
        text = self.refs(text)
        if text.startswith('r(s.memory,'):
            owner, address, width = call(text, 'r')
            if owner != 's.memory' or width != '4':
                raise ValueError('Unsupported polynomial read')
            return f'frame.word({address})'
        if text.startswith('_mm_set_ss('):
            inner, = call(text, '_mm_set_ss')
            inner, = call(inner, 'f')
            owner, address, width = call(inner, 'r')
            if owner != 's.memory' or width != '4':
                raise ValueError('Unsupported polynomial scalar')
            return f'frame.scalar({address})'
        for original, native in (('_mm_mul_ss', 'multiply'), ('_mm_add_ss', 'add'), ('_mm_sub_ss', 'subtract')):
            if text.startswith(original + '('):
                left, right = call(text, original)
                # SSE memory-source operands become a native scalar read.
                if right.startswith('_mm_set_ss('):
                    right = self.expression(right)
                return f'{native}({left},{right})'
        if text.startswith('_mm_xor_ps('):
            left, right = call(text, '_mm_xor_ps')
            if left != right:
                raise ValueError('Nonzero SIMD bitwise operation in polynomial')
            return '0.0f'
        for operation, symbol in (('minus', '-'), ('plus', '+'), ('product', '*')):
            prefix = 's.flags.' + operation
            if text.startswith(prefix + '('):
                operands = call(text, prefix)
                if operation != 'product' and operands.pop() != '4':
                    raise ValueError('Unsupported polynomial integer width')
                left, right = operands
                return f'std::uint32_t(({left}){symbol}({right}))'
        return text

    def finish(self):
        if self.extended:
            raise ValueError('Polynomial leaves unowned extended values')
        if self.address[1] != '0u':
            raise ValueError('Polynomial native child has a nonzero remaining dimension')
        if any(value is None for value in self.scalar):
            raise ValueError('Polynomial leaves an incoming scalar lane unbound')
        self.lines.append('    return Continuation{' + ','.join(
            [self.address[6], self.address[2], *self.scalar, f'std::uint16_t({self.c1})']) + '};')

    def instruction(self, pc, operation):
        if operation.startswith('s.set('):
            destination, value, width, shift = call(operation[:-1], 's.set')
            if width != '4' or shift != '0':
                raise ValueError('Polynomial has partial integer transport')
            index = int(destination)
            if value == 's.pop()':
                # Common nonvolatile restore is owned by the adapter.
                return
            self.address[index] = self.value('address', self.expression(value))
            return
        if operation.startswith('s.xmm['):
            match = re.fullmatch(r's\.xmm\[(\d)\]=(.*);', operation)
            self.scalar[int(match[1])] = self.value('coefficient', self.expression(match[2]))
            return
        if operation.startswith('s.push('):
            first, *after = operation.split(';')
            site, value = call(first, 's.push')
            self.address[4] = self.value('address', f'({self.address[4]})-4u')
            self.lines.append(f'    frame.publish({site},{self.address[4]},{self.refs(value)});')
            if ''.join(after):
                if ''.join(after) != 'run_145d50(s)':
                    raise ValueError('Polynomial has an unconverted child')
                self.square()
            return
        if operation.startswith('w('):
            first = operation.split(';')[0]
            memory, observe, site, address, width, value = call(first, 'w')
            if memory != 's.memory' or observe != 's.observe' or width != '4':
                raise ValueError('Unsupported polynomial store')
            if value.startswith('u('):
                value, = call(value, 'u')
                value, = call(value, '_mm_cvtss_f32')
                native = f'std::bit_cast<std::uint32_t>({self.refs(value)})'
            elif value.startswith('native_single_spill('):
                value, status = call(value, 'native_single_spill')
                if status != 's.fp.status':
                    raise ValueError('Polynomial spill lacks its source condition')
                rounded = self.value('rounded', f'prt_basis_detail::round_single({self.refs(value)})')
                self.c1 = rounded + '.status&0x200u'
                native = rounded + '.bits'
            else:
                native = self.refs(value)
            self.lines.append(f'    frame.publish({site},{self.refs(address)},{native});')
            if operation.endswith('s.fp.pop();'):
                self.extended.pop(0)
            return
        if operation.startswith('s.fp.push('):
            value = operation[len('s.fp.push('):operation.index(');s.fp.status')]
            match = re.fullmatch(r'static_cast<long double>\(std::bit_cast<double>\(native_read64\(s.memory,(.*)\)\)\)', value)
            if not match:
                raise ValueError('Polynomial has an unowned extended input: ' + value)
            self.extended.insert(0, self.value('extended', f'frame.extended_double({self.refs(match[1])})'))
            self.c1 = '0u'
            return
        if operation.startswith('std::swap(s.fp.st('):
            self.extended[0], self.extended[1] = self.extended[1], self.extended[0]
            self.c1 = '0u'
            return
        if operation.startswith('s.fp.st('):
            match = re.fullmatch(r's\.fp\.st\((\d)\)=(.*?);(?:s.fp.pop\(\);)?', operation)
            if not match:
                raise ValueError('Unsupported extended polynomial expression: ' + operation)
            value = self.refs(match[2])
            value = value.replace('root(', 'physics_solver_detail::root(').replace('mul(', 'physics_step_detail::mul(')
            value = re.sub(r'ext\(f\(r\(s.memory,([^,]+),4\)\)\)', r'frame.extended_single(\1)', value)
            self.extended[int(match[1])] = self.value('extended', value)
            if operation.endswith('s.fp.pop();'):
                self.extended.pop(0)
            return
        if operation.startswith('s.flags.minus('):
            self.order, limit, width = call(operation[:-1], 's.flags.minus')
            if width != '4' or limit not in ('selected(0x2u,s.image)', 'selected(0x3u,s.image)', '0x2u', '0x3u'):
                raise ValueError('Unowned polynomial order gate')
            self.order = self.refs(self.order)
            return
        if operation == 'if(s.flags.cf||s.flags.zf)goto L148b07;':
            self.gates += 1
            self.lines.append(f'    if({self.order}<={self.gates+1}u) {{')
            self.finish()
            self.lines.append('    }')
            return
        if operation == 's.pop();s.gp[4]+=4u;return;':
            self.finish()
            return
        raise ValueError(f'Unsupported authenticated polynomial statement at {pc:x}: {operation}')

    def square(self):
        name = self.value('degree', 'frame.square(' + ','.join(
            [self.address[4], self.address[3], self.address[0], self.address[5], self.address[6], self.address[7], self.scalar[1]]) + ')')
        self.address[4] = self.value('address', self.address[4] + '+12u')
        self.address[0], self.address[1], self.address[2] = name + '.output', '0u', name + '.source'
        self.address[5], self.address[6], self.address[7] = name + '.saved_bp', name + '.saved_si', name + '.saved_di'
        self.scalar[0], self.scalar[1] = name + '.first', name + '.second'


def generate(source):
    match = re.search(r'static void run_146100\(WorldState &s\) \{.*?\n\}', source, re.S)
    if not match:
        raise ValueError('Missing canonical SH rotation owner')
    body = match[0]
    native_prefix = ('\n    if(!prt_helpers_detail::reference()) {\n'
                     '        prt_rotation_detail::bridge(s);return;\n'
                     '    }')
    body = body.replace(native_prefix, '', 1)
    from model_prt_native_replacements import authenticate_body
    authenticate_body(body, 0x146100, source)
    polynomial = Polynomial()
    for line in body.splitlines()[1:-2]:
        match = re.fullmatch(r'(?:L[0-9a-f]+: )?if\(s.services.instruction\)\{s.services.instruction\(0x([0-9a-f]+)u\);\} (.*)', line)
        if not match:
            raise ValueError('Unauthenticated SH rotation statement: ' + line)
        pc, operation = int(match[1], 16), match[2]
        if pc in (0x148b07, 0x148b08, 0x148b09, 0x148b0b, 0x148b0c):
            continue  # Common published-stack/ABI restoration belongs to Frame's adapter.
        polynomial.instruction(pc, operation)
    if polynomial.gates != 2:
        raise ValueError('Native SH rotation lost a degree boundary')
    return ('#pragma once\n// Generated native scalar SH rotation polynomial.\n'
            f'// Canonical146100 body SHA-256: {PIN}\n'
            '// Regenerate with tools/generate_native_tu15_prt_rotation.py.\n'
            '#include "rendering/models/native_tu15_prt_rotation_types.hpp"\n\n'
            'namespace halo2::prt_rotation_detail {\n'
            'inline Continuation rotate_polynomial(Frame& frame, const Caller& input) {\n' +
            '\n'.join(polynomial.lines) + '\n}\n}\n')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, default=SOURCE)
    parser.add_argument('--output', type=Path, default=OUTPUT)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    output = generate(args.source.read_text())
    if args.check:
        if args.output.read_text() != output:
            raise ValueError('Native SH rotation polynomial is stale')
    else:
        args.output.write_text(output)
    print('Native SH rotation polynomial: ' + hashlib.sha256(output.encode()).hexdigest())


if __name__ == '__main__':
    main()
