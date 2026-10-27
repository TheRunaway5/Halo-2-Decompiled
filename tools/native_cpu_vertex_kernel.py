"""Build-time fixed C++ lowering for source vertex position/skin prefixes.

Program decoding happens here. Runtime code only matches the complete original
word identity and calls an ordinary compiled C++ function. The functions retain
the original CPU reference's explicit float operation order and register writes.
"""
import hashlib
import struct


def _program(row):
    raw = bytes.fromhex(row['vertex_program_hex'])
    if len(raw) < 20 or raw[:2] != b'\x78\x20':
        raise ValueError('Native CPU vertex source has no original header')
    count = struct.unpack_from('<H', raw, 2)[0]
    if not 1 <= count <= 136 or len(raw) != 4 + count * 16:
        raise ValueError('Native CPU vertex source has an invalid complete extent')
    words = [list(struct.unpack_from('<4I', raw, 4 + i * 16)) for i in range(count)]
    if words != row['vertex_instructions']:
        raise ValueError('Native CPU vertex source words differ from original bytes')
    for i, t in enumerate(words):
        if bool(t[3] & 1) != (i + 1 == count):
            raise ValueError('Native CPU vertex source final marker changed')
    return raw, words


def position_prefix(words):
    """Exact offline form of the source-owned pStencil projection boundary."""
    for i, t in enumerate(words):
        mac = (t[1] >> 21) & 15
        if (mac in (6, 7) and ((t[2] >> 11) & 3) == 3 and
                ((t[1] >> 13) & 255) == 0 and not t[3] & 2 and
                (t[3] & 0xf800) == 0x8800 and ((t[3] >> 3) & 15) == 0 and not t[3] & 4):
            result = [list(row) for row in words[:i+1]]
            for row in result:
                row[3] &= ~1
            result[i][1] = (t[1] & 0x1fffff) | (1 << 21)
            result[i][3] = 0xf800 | (9 << 3) | 1
            return result
    return None


def lower_cpu_vertex_program(words, name):
    lines = [f'inline void {name}(VertexProgramState& state) {{',
             '    std::array<ShaderVector,13> temporary{};',
             '    int address=0;',
             '    for(auto& value:state.outputs)value={0,0,0,1};']

    def temp(reg):
        if reg > 12:
            raise ValueError('Native CPU temporary escapes source register file')
        return 'state.outputs[0]' if reg == 12 else f'temporary[{reg}]'

    def operand(t, which):
        if which == 0:
            mux, reg, neg = (t[2] >> 26) & 3, t[2] >> 28, (t[1] >> 8) & 1
            swizzle = [(t[1] >> shift) & 3 for shift in (6, 4, 2, 0)]
        elif which == 1:
            mux, reg, neg = (t[2] >> 11) & 3, (t[2] >> 13) & 15, (t[2] >> 25) & 1
            swizzle = [(t[2] >> shift) & 3 for shift in (23, 21, 19, 17)]
        else:
            mux, reg, neg = (t[3] >> 28) & 3, (t[2] & 3) * 4 + (t[3] >> 30), (t[2] >> 10) & 1
            swizzle = [(t[2] >> shift) & 3 for shift in (8, 6, 4, 2)]
        if mux == 1:
            value = temp(reg)
        elif mux == 2:
            value = f'state.attributes[{(t[1] >> 9) & 15}]'
        elif mux == 3:
            reg = (t[1] >> 13) & 255
            value = f'native_constant(state,{reg}+address)' if t[3] & 2 else f'native_constant(state,{reg})'
        else:
            raise ValueError('Native CPU active operand has no source owner')
        return '{' + ','.join(f'({"-1.f" if neg else "1.f"})*{value}[{axis}]' for axis in swizzle) + '}'

    def writes(target, value, mask):
        for axis in range(4):
            if mask & (8 >> axis):
                lines.append(f'    {target}[{axis}]={value}[{axis}];')

    for i, t in enumerate(words):
        mac, ilu = (t[1] >> 21) & 15, (t[1] >> 25) & 7
        if mac > 13:
            raise ValueError('Native CPU MAC opcode has no source reference')
        used = {0} if mac else set()
        if mac == 2 or 4 <= mac <= 12:
            used.add(1)
        if ilu or mac in (3, 4):
            used.add(2)
        for which, letter in enumerate('abc'):
            lines.append(f'    const ShaderVector {letter}{i}=' + (operand(t, which) if which in used else '{}') + ';')
        a, b, c, m, l = [f'{letter}{i}' for letter in 'abcml']
        lines.append(f'    ShaderVector {m}{{}},{l}{{}};')
        if mac in (5, 6, 7):
            lines.append(f'    float dot{i}=(native_product({a}[0],{b}[0])+native_product({a}[1],{b}[1]))+native_product({a}[2],{b}[2]);')
            if mac == 6:
                lines.append(f'    dot{i}+={b}[3];')
            if mac == 7:
                lines.append(f'    dot{i}+=native_product({a}[3],{b}[3]);')
            lines.append(f'    {m}.fill(dot{i});')
        elif mac == 8:
            lines.append(f'    {m}={{1,native_product({a}[1],{b}[1]),{a}[2],{b}[3]}};')
        elif mac not in (0, 13):
            for axis in range(4):
                aa, bb, cc = (f'{v}[{axis}]' for v in (a, b, c))
                expressions = {1: aa, 2: f'native_product({aa},{bb})', 3: f'{aa}+{cc}',
                    4: f'native_product({aa},{bb})+{cc}', 9: f'std::min({aa},{bb})',
                    10: f'std::max({aa},{bb})', 11: f'{aa}<{bb}?1.f:0.f', 12: f'{aa}>={bb}?1.f:0.f'}
                lines.append(f'    {m}[{axis}]={expressions[mac]};')
        if ilu == 1:
            lines.append(f'    {l}={c};')
        elif ilu in (2, 3, 4):
            value = f'1.f/std::sqrt(std::abs({c}[0]))' if ilu == 4 else f'1.f/{c}[0]'
            lines.append(f'    float scalar{i}={value};')
            if ilu == 3:
                lines.append(f'    scalar{i}=std::copysign(std::clamp(std::abs(scalar{i}),0x1p-64f,0x1p64f),scalar{i});')
            lines.append(f'    {l}.fill(scalar{i});')
        elif ilu == 5:
            lines.append(f'    {l}={{std::exp2(std::floor({c}[0])),{c}[0]-std::floor({c}[0]),std::exp2({c}[0]),1}};')
        elif ilu == 6:
            lines += [f'    const float scalar{i}=std::log2(std::abs({c}[0]));',
                      f'    {l}={{std::floor(scalar{i}),std::abs({c}[0])/std::exp2(std::floor(scalar{i})),scalar{i},1}};']
        elif ilu == 7:
            lines.append(f'    {l}={{1,std::max({c}[0],0.f),{c}[0]>0?std::pow(std::max({c}[1],0.f),std::clamp({c}[3],-128.f,128.f)):0.f,1}};')
        if mac == 13:
            lines += [f'    const double loaded{i}=std::floor(double({a}[0]));',
                      f'    if(!std::isfinite(loaded{i}) || loaded{i}<std::numeric_limits<int>::min() || loaded{i}>std::numeric_limits<int>::max())',
                      '        throw FormatError("Native position address outside source register range");',
                      f'    address=int(loaded{i});']
        reg, mm, im, om = (t[3] >> 20) & 15, (t[3] >> 24) & 15, (t[3] >> 16) & 15, (t[3] >> 12) & 15
        if mac not in (0, 13) and mm and not (ilu and reg == 1):
            writes(temp(reg), m, mm)
        if ilu and im:
            writes(temp(1 if mac else reg), l, im)
        if om:
            output = (t[3] >> 3) & 255
            if not t[3] & 2048 or output > 15:
                raise ValueError('Native CPU source writes an unsupported constant/output')
            writes(f'state.outputs[{output}]', l if t[3] & 4 else m, om)
    lines += ['}', '']
    return '\n'.join(lines)


def build_position_header(registry):
    source = ['#pragma once', '#include "halo2/vertex_program.hpp"', '#include "halo2/native_world_kernel.hpp"',
              '#include <algorithm>', '#include <cmath>', '#include <limits>',
              'namespace halo2::generated_native_position_kernels {',
              'inline float native_product(float a,float b){return a==0 || b==0?0.f:a*b;}',
              'inline const ShaderVector& native_constant(const VertexProgramState& state,int index){',
              '    if(index<0 || unsigned(index)>=state.constants.size())throw FormatError("Native position constant outside source register bank");',
              '    return state.constants[unsigned(index)];', '}']
    entries, seen = [], set()
    for row in registry.get('cpu_vertex_programs', []):
        raw, original = _program(row)
        if raw in seen:
            raise ValueError('Native CPU registry duplicates a complete source program')
        seen.add(raw)
        prefix = position_prefix(original)
        if prefix is None:
            continue
        key = hashlib.sha256(raw).hexdigest()
        name = 'position_' + key
        source.append(lower_cpu_vertex_program(prefix, name))
        source.append(f'inline constexpr std::array<VertexInstruction,{len(original)}> words_{key}{{{{')
        source.extend('    {' + ','.join(str(v)+'u' for v in words) + '},' for words in original)
        source.append('}};')
        entries.append((key, name, len(prefix)))
    source.append(f'inline constexpr std::array<NativeCpuPositionKernelEntry,{len(entries)}> entries{{{{')
    source.extend(f'    {{words_{key},&{name}}},' for key, name, _ in entries)
    source += ['}};', '} // namespace halo2::generated_native_position_kernels', '']
    return '\n'.join(source), {'source_programs': len(seen), 'position_kernels': len(entries),
                               'prefix_instruction_count': sum(count for _, _, count in entries),
                               'runtime_instruction_interpreter': False}
