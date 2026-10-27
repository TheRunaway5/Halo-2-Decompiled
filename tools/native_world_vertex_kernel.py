"""Ahead-of-time lowering of admitted world vertex words to native GLSL.

Only the build tools call this module. The result contains fixed arithmetic,
register accesses and declaration bindings; it reads no instruction bank and
decodes no opcode, operand, destination mask or vertex declaration at runtime.
The world wrapper supplies vpConstant, vpProduct and the ordinary draw inputs.
Arithmetic follows the existing strict world backend; this does not establish
physical NV2A reciprocal/RSQ precision beyond the separately measured kernels.
"""


def _uint(value, maximum=0xffffffff):
    if type(value) is not int or not 0 <= value <= maximum:
        raise ValueError('vertex kernel requires literal unsigned words')
    return value


def _declaration(value):
    if value is None:
        return [255] * 16
    if not isinstance(value, (list, tuple)):
        raise ValueError('vertex declaration must contain four words or sixteen bytes')
    if len(value) == 4:
        return [(_uint(value[i // 4]) >> (8 * (i % 4))) & 255 for i in range(16)]
    if len(value) == 16:
        return [_uint(x, 255) for x in value]
    raise ValueError('vertex declaration must contain four words or sixteen bytes')


def lower_vertex_program(program_words, declaration=None, extended_vertex_bank=False):
    """Return a direct runVertexProgram function and its static read metadata.

    program_words contains the decoded four-word rows, excluding the 0x2078
    header. The caller authenticates the complete original byte identity and
    associates it with the exact packed declaration and extended-bank flag.
    """
    if type(extended_vertex_bank) is not bool:
        raise ValueError('extended vertex bank must be an explicit boolean')
    if not isinstance(program_words, (list, tuple)) or not 1 <= len(program_words) <= 136:
        raise ValueError('vertex kernel instruction count is outside NV2A storage')
    words = []
    for row in program_words:
        if not isinstance(row, (list, tuple)) or len(row) != 4:
            raise ValueError('vertex kernel instruction must contain four words')
        words.append(tuple(_uint(x) for x in row))
    semantics = _declaration(declaration)
    if semantics[0] == 255 and any(x != 255 for x in semantics):
        raise ValueError('legacy vertex declaration must be the complete absent bank')
    lines = ['    // Static world input and register initialization.']
    if extended_vertex_bank:
        lines.append('    vpAddress=0;')
    lines.extend(f'    vpTemp[{i}]=vec4(0.0);' for i in range(13))
    lines.extend(f'    vpOut[{i}]=vec4(0.0,0.0,0.0,1.0);' for i in range(16))
    lines.extend(f'    vpInput[{i}]=material.vertexAttributes[{i}];' for i in range(16))
    bindings = {0: 'vec4(position,1.0)', 64: 'vec4(position,1.0)',
                3: 'vec4(texcoord,0.0,1.0)', 4: 'vec4(normal,1.0)',
                5: 'vec4(tangent,1.0)', 6: 'vec4(binormal,1.0)',
                8: 'vec4(incident,1.0)', 9: 'vec4(lightmapCoord,0.0,1.0)',
                14: 'vec4(color,1.0)', 19: 'vec4(prtCluster,0.0,0.0,1.0)',
                20: 'prtWeights'}
    if semantics[0] == 255:
        for reg, semantic in enumerate((0, 3, 4, 5, 6, 8, 9)):
            lines.append(f'    vpInput[{reg}]={bindings[semantic]};')
        lines.extend(['    if(material.flags.z==10u && material.sourceCameraControl.w==1u)',
                      '        vpInput[1]=vec4(normal,1.0);'])
        if extended_vertex_bank:
            lines.extend(['    if(vertexBank.range.z!=0u) {',
                          '        vpInput[5]=vec4(prtCluster,0.0,0.0,1.0);',
                          '        vpInput[6]=prtWeights;', '    }'])
        lines.append('    if(material.flags.z==4u)vpInput[6]=vec4(color,1.0);')
    else:
        for reg, raw in enumerate(semantics):
            semantic = raw & 127 if reg == 0 else raw
            if semantic == 255:
                continue
            if semantic in (19, 20) and not extended_vertex_bank:
                raise ValueError('PRT declaration requires the explicit extended vertex bank')
            if semantic == 68:
                lines.extend([f'    precise vec3 nativeSquared{reg}=normal*normal;',
                              f'    precise float nativeXY{reg}=nativeSquared{reg}.x+nativeSquared{reg}.y;',
                              f'    precise float nativeLength{reg}=nativeXY{reg}+nativeSquared{reg}.z;',
                              f'    float nativeInverse{reg}=inversesqrt(abs(nativeLength{reg}));',
                              f'    vpInput[{reg}]=vec4(normal.x==0.0?0.0:normal.x*nativeInverse{reg},',
                              f'        normal.y==0.0?0.0:normal.y*nativeInverse{reg},',
                              f'        normal.z==0.0?0.0:normal.z*nativeInverse{reg},1.0);'])
            elif semantic in bindings:
                lines.append(f'    vpInput[{reg}]={bindings[semantic]};')
            else:
                raise ValueError(f'vertex declaration semantic {semantic} has no world input owner')
    lines.append('    if(material.flags.z==12u)vpInput[11]=vec4(color,sourceColorAlpha);')
    inputs, constants, operations = set(), set(), set()
    relative = False
    default_w = 15

    def operand(t, which):
        nonlocal relative
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
            if reg > 12:
                raise ValueError('vertex temporary read exceeds the original register file')
            value = 'vpOut[0]' if reg == 12 else f'vpTemp[{reg}]'
        elif mux == 2:
            reg = (t[1] >> 9) & 15
            inputs.add(reg)
            value = f'vpInput[{reg}]'
        elif mux == 3:
            reg = (t[1] >> 13) & 255
            if reg >= 192:
                raise ValueError('vertex constant read exceeds the original register file')
            if t[3] & 2:
                if not extended_vertex_bank:
                    raise ValueError('relative vertex constant requires the explicit extended bank')
                relative = True
                constants.update(range(192))
                index = f'(int({reg})+vpAddress)'
                value = f'(({index}>=0 && {index}<192)?vpConstant(uint({index})):vec4(0.0))'
            else:
                constants.add(reg)
                value = f'vpConstant({reg}u)'
        else:
            raise ValueError('active vertex operand has no register-file owner')
        return f'({"-1.0" if neg else "1.0"}*{value}.{"".join("xyzw"[x] for x in swizzle)})'

    def write(destination, value, mask):
        if mask:
            axes = ''.join('xyzw'[i] for i in range(4) if mask & (8 >> i))
            lines.append(f'    {destination}.{axes}={value}.{axes};')

    def temporary(reg):
        if reg > 12:
            raise ValueError('vertex temporary write exceeds the original register file')
        return 'vpOut[0]' if reg == 12 else f'vpTemp[{reg}]'

    for i, t in enumerate(words):
        if bool(t[3] & 1) != (i + 1 == len(words)):
            raise ValueError('vertex final instruction marker disagrees with its extent')
        mac, ilu = (t[1] >> 21) & 15, (t[1] >> 25) & 7
        if mac > 13:
            raise ValueError('vertex MAC opcode is not implemented by the world backend')
        reg, mm, im, om = (t[3] >> 20) & 15, (t[3] >> 24) & 15, (t[3] >> 16) & 15, (t[3] >> 12) & 15
        output = (t[3] >> 3) & 255
        if om and (not t[3] & 2048 or output > 15):
            raise ValueError('vertex constant/output destination is not supported')
        if om and output in range(9, 13) and om & 1:
            default_w &= ~(1 << (output - 9))
        if mac == 13 and not extended_vertex_bank:
            raise ValueError('ARL requires the explicit extended vertex bank')
        operations.add((mac, ilu))
        used = {0} if mac else set()
        if mac == 2 or 4 <= mac <= 12:
            used.add(1)
        if ilu or mac in (3, 4):
            used.add(2)
        lines.append(f'    // Original instruction {i}: MAC {mac}, ILU {ilu}.')
        # All operands snapshot the old register state before either paired
        # arithmetic result or any destination write can become observable.
        for which, name in enumerate('abc'):
            expr = operand(t, which) if which in used else 'vec4(0.0)'
            lines.append(f'    vec4 native_{name}{i}={expr};')
        a, b, c, m, l = (f'native_{x}{i}' for x in 'abcml')
        lines.append(f'    precise vec4 {m}=vec4(0.0),{l}=vec4(0.0);')
        arithmetic = {1: a, 2: f'vpProduct({a},{b})', 3: f'{a}+{c}',
                      4: f'vpProduct({a},{b})+{c}', 8: f'vec4(1.0,vpProduct({a},{b}).y,{a}.z,{b}.w)',
                      9: f'min({a},{b})', 10: f'max({a},{b})',
                      11: f'vec4(lessThan({a},{b}))', 12: f'vec4(greaterThanEqual({a},{b}))'}
        if mac in arithmetic:
            lines.append(f'    {m}={arithmetic[mac]};')
        elif mac in (5, 6, 7):
            lines.extend([f'    vec4 native_product{i}=vpProduct({a},{b});',
                          f'    precise float native_dot{i}=(native_product{i}.x+native_product{i}.y)+native_product{i}.z;'])
            if mac == 6:
                lines.append(f'    native_dot{i}+={b}.w;')
            if mac == 7:
                lines.append(f'    native_dot{i}+=native_product{i}.w;')
            lines.append(f'    {m}=vec4(native_dot{i});')
        scalar = {1: c, 2: f'vec4(1.0/{c}.x)', 4: f'vec4(inversesqrt(abs({c}.x)))',
                  5: f'vec4(exp2(floor({c}.x)),fract({c}.x),exp2({c}.x),1.0)',
                  7: f'vec4(1.0,max({c}.x,0.0),{c}.x>0.0?pow(max({c}.y,0.0),clamp({c}.w,-128.0,128.0)):0.0,1.0)'}
        if ilu in scalar:
            lines.append(f'    {l}={scalar[ilu]};')
        elif ilu == 3:
            q = f'native_reciprocal{i}'
            lines.extend([f'    float {q}=1.0/{c}.x;',
                          f'    if({q}>0.0 || floatBitsToUint({q})==0u)',
                          f'        {q}=clamp({q},5.421010862427522e-20,1.8446744073709552e19);',
                          f'    else if(!isnan({q}))',
                          f'        {q}=clamp({q},-1.8446744073709552e19,-5.421010862427522e-20);',
                          f'    {l}=vec4({q});'])
        elif ilu == 6:
            lines.extend([f'    float native_log{i}=log2(abs({c}.x));',
                          f'    {l}=vec4(floor(native_log{i}),abs({c}.x)/exp2(floor(native_log{i})),native_log{i},1.0);'])
        if mac == 13:
            lines.append(f'    vpAddress=int(floor({a}.x));')
        if mac not in (0, 13) and mm and not (ilu and reg == 1):
            write(temporary(reg), m, mm)
        if ilu and im:
            write(temporary(1 if mac else reg), l, im)
        if om:
            write(f'vpOut[{output}]', l if t[3] & 4 else m, om)
    statements = '\n'.join(lines) + '\n'
    glsl = 'void runVertexProgram() {\n' + statements + '}\n'
    return {'helpers': '', 'statements': statements, 'glsl': glsl,
            'instruction_count': len(words), 'consumed_inputs': sorted(inputs),
            'consumed_constants': sorted(constants), 'default_texture_w_mask': default_w,
            'relative_constants': relative, 'operations': sorted(operations),
            'declaration_bytes': semantics, 'extended_vertex_bank': extended_vertex_bank}
