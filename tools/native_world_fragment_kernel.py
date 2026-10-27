"""Ahead-of-time lowering of authored world fragment programs to fixed GLSL.

Only the offline compiler reads instruction/control words. Generated kernels
contain ordinary texture lookups and arithmetic; animated color constants stay
in the existing material uniform ABI.
"""
import re
from native_combiner_contract import validate_combiner_controls


def _u(value):
    return f'{value}u'


def _input(operand, alpha=False):
    operand &= 255
    reg = f'registers[{operand & 15}]'
    value = (f'vec3({reg}.a)' if operand & 16 else f'vec3({reg}.b)') if alpha else (
        f'{reg}.aaa' if operand & 16 else f'{reg}.rgb')
    return [f'max({value},vec3(0))', f'(vec3(1)-clamp({value},0.0,1.0))',
            f'(2.0*max({value},vec3(0))-vec3(1))',
            f'(vec3(1)-2.0*max({value},vec3(0)))',
            f'(max({value},vec3(0))-vec3(0.5))',
            f'(vec3(0.5)-max({value},vec3(0)))', value,
            f'(-{value})'][(operand >> 5) & 7]


def _output(value, operation):
    mode = (operation >> 15) & 7
    value = {1: f'({value}-vec3(0.5))', 2: f'({value}*2.0)',
             3: f'(({value}-vec3(0.5))*2.0)', 4: f'({value}*4.0)',
             6: f'({value}*0.5)'}.get(mode, value)
    return f'clamp({value},-1.0,1.0)'


def _commit(lines, operation, alpha, prefix):
    for destination, value, blue in [((operation >> 4) & 15, prefix+'ab', operation & 0x80000),
                                     (operation & 15, prefix+'cd', operation & 0x40000),
                                     ((operation >> 8) & 15, prefix+'sum', False)]:
        if destination:
            lines.append(f'registers[{destination}].'+('a' if alpha else 'rgb')+
                         f'={value}'+('.x;' if alpha else ';'))
            if blue and not alpha:
                lines.append(f'registers[{destination}].a={value}.b;')


def _combiner(words, color_expression=None, preserve_source_operations=False):
    # Keep packed animated colors dynamic. Control-word decoding happens here.
    lines = ['vec4 registers[16];',
             'vec4 argb(uint c){return vec4((c>>16u)&255u,(c>>8u)&255u,c&255u,c>>24u)/255.0;}',
             'vec4 runCombiners(vec4 textures[4],vec4 v0,vec4 v1){']
    lines += [f'registers[{i}]=vec4(0);' for i in range(16)]
    lines += ['#ifdef H2_COMBINER_FOG', 'registers[3]=H2_COMBINER_FOG();', '#endif',
              'registers[4]=v0;registers[5]=v1;']
    lines += [f'registers[{8+i}]=textures[{i}];' for i in range(4)]
    lines += ['registers[12].a='+('textures[0].a;' if words[54] & 31 else '1.0;')]

    def color(index):
        return color_expression(index) if color_expression else f'argb(material.program[{index//4}][{index%4}])'

    # combine() in combiner.glsl computes AB and CD before their SUM, then
    # applies each output mapping and saturation.  Fog requires those source
    # operation boundaries: contracting the generated multiply/add changes an
    # actual source-owned fog frame. GLSL precise propagates to these arithmetic
    # dependencies before SPIR-V optimization; no runtime program is involved.
    precision = 'precise ' if preserve_source_operations else ''
    for stage in range(min(words[53] & 255, 8)):
        lines += ['{', f'registers[1]={color(10+(stage if words[53]&0x1000 else 0))};',
                  f'registers[2]={color(18+(stage if words[53]&0x10000 else 0))};']
        operations = []
        for alpha, inputs, operation, prefix in [
                (False, words[34+stage], words[45+stage], 'r'),
                (True, words[stage], words[26+stage], 'a')]:
            a,b,c,d = [_input(inputs >> shift, alpha) for shift in (24,16,8,0)]
            ab = f'vec3(dot({a},{b}))' if not alpha and operation & 0x2000 else f'({a}*{b})'
            cd = f'vec3(dot({c},{d}))' if not alpha and operation & 0x1000 else f'({c}*{d})'
            lines += [f'{precision}vec3 {prefix}rawab={ab},{prefix}rawcd={cd};']
            mux = 'registers[12].a>=0.5' if words[53] & 0x100 else '(uint(registers[12].a*255.0)&1u)!=0u'
            total = f'(({mux})?{prefix}rawcd:{prefix}rawab)' if operation & 0x4000 else f'({prefix}rawab+{prefix}rawcd)'
            lines += [f'{precision}vec3 {prefix}ab={_output(prefix+"rawab",operation)},'
                      f'{prefix}cd={_output(prefix+"rawcd",operation)},'
                      f'{prefix}sum={_output(total,operation)};']
            operations.append((operation, alpha, prefix))
        # Both channels snapshot their inputs before either channel commits.
        for operation, alpha, prefix in operations:
            _commit(lines, operation, alpha, prefix)
        lines.append('}')
    abcd, efg = words[8:10]
    if abcd == 0 and efg == 0:
        lines.append('return clamp(registers[12],0.0,1.0);}')
    else:
        lines += [f'registers[1]={color(43)};registers[2]={color(44)};']
        left = '(vec3(1)-registers[5].rgb)' if efg & 64 else 'registers[5].rgb'
        right = '(vec3(1)-registers[12].rgb)' if efg & 32 else 'registers[12].rgb'
        total = f'({left}+{right})'
        if efg & 128: total = f'clamp({total},0.0,1.0)'
        lines += [f'registers[14]=vec4({total},0);',
                  f'registers[15]=vec4({_input(efg>>24)}*{_input(efg>>16)},0);']
        a,b,c,d = [_input(abcd >> shift) for shift in (24,16,8,0)]
        lines += [f'return clamp(vec4({a}*{b}+(vec3(1)-{a})*{c}+{d},'
                  f'{_input(efg>>8,True)}.x),0.0,1.0);}}']
    return '\n'.join(lines)+'\n'


def _textures(words):
    modes = [(words[54] >> (5*i)) & 31 for i in range(4)]
    allowed = {0,1,2,3,4,5,6,9,11,12,14,15,16,17}
    if any(x not in allowed for x in modes):
        raise ValueError(f'Unsupported authored texture modes: {modes}')
    valid = [False]*4
    lines = ['float textureSignedByte(float v){float b=v*255.0;return (b>=128.0?b-256.0:b)/127.0;}',
             'void runTextureStages(out vec4 samples[4]){']
    lines += [f'samples[{i}]=vec4(0);float product{i}=0.0;' for i in range(4)]

    def previous(i): return 0 if i == 1 else (words[56] >> ((i+2)*4)) & 3
    def dot(i):
        p = previous(i)
        mapping = (words[55] >> ((i-1)*4)) & 7
        if mapping > 2: raise ValueError('Unproved HILO/signed dot mapping')
        v = f'samples[{p}].rgb'
        v = [v, f'({v}*255.0-vec3(128))/127.0', f'({v}*2.0-vec3(1))'][mapping]
        return f'dot(stageCoordinates[{i}].xyz,{v})'

    for i,mode in enumerate(modes):
        s = _u(i)
        if mode in (1,2):
            coord = (f'vec3(stageCoordinates[{i}].xy/TEXTURE_COORDINATE_W({s}),0.0)' if mode == 1
                     else f'stageCoordinates[{i}].xyz/TEXTURE_COORDINATE_W({s})')
            lines += [f'if(TEXTURE_VOLUME({s}))samples[{i}]=fetchStage3D({s},{coord});',
                      f'else samples[{i}]=TEXTURE_PROJECTED_2D({s});']
            valid[i] = True
        elif mode == 3:
            lines += [f'samples[{i}]=fetchStageCube({s},stageCoordinates[{i}].xyz);'];valid[i] = True
        elif mode == 4:
            lines += [f'samples[{i}]=clamp(stageCoordinates[{i}],0.0,1.0);'];valid[i] = True
        elif mode == 5:
            for axis in range(4):
                compare = '>=' if words[42] & (1 << (4*i+axis)) else '<'
                lines += [f'if(stageCoordinates[{i}][{axis}]{compare}0.0){{TEXTURE_REJECT();}}']
        elif i and mode:
            p = previous(i)
            if p >= i or not valid[p] or modes[p] == 17: continue
            if mode == 6:
                lines += ['{', f'uint sign=TEXTURE_COLOR_SIGN({_u(p)});',
                          f'vec2 delta=vec2((sign&2u)!=0u?samples[{p}].b:textureSignedByte(samples[{p}].b),'
                          f'(sign&4u)!=0u?samples[{p}].g:textureSignedByte(samples[{p}].g));',
                          f'vec4 matrix=TEXTURE_BUMP_MATRIX({s});',
                          'delta=mat2(matrix.xy,matrix.zw)*delta;',
                          f'samples[{i}]=fetchStage2D({s},stageCoordinates[{i}].xy+delta);','}'];valid[i]=True
                continue
            if mode in (15,16):
                lines += [f'samples[{i}]=fetchStage2D({s},samples[{p}].'+('ar' if mode==15 else 'gb')+');'];valid[i]=True
                continue
            lines += [f'product{i}={dot(i)};']
            if mode == 17: valid[i]=True;continue
            if i < 2 or not valid[i-1]: continue
            if mode == 9 and modes[i-1] == 17:
                lines += [f'samples[{i}]=fetchStage2D({s},vec2(product{i-1},product{i}));'];valid[i]=True
            if mode == 11 and i == 2 and modes[i-1] == 17 and modes[i+1] == 12:
                n = previous(i+1)
                if n < i-1 and valid[n]:
                    lines += [f'samples[{i}]=fetchStageCube({s},vec3(product{i-1},product{i},{dot(i+1)}));'];valid[i]=True
            if i == 3 and valid[i-2] and modes[i-2] == 17:
                normal = f'vec3(product{i-2},product{i-1},product{i})'
                if mode == 14 and modes[i-1] == 17:
                    lines += [f'samples[{i}]=fetchStageCube({s},{normal});'];valid[i]=True
                if mode == 12 and modes[i-1] in (17,11):
                    lines += ['{',f'vec3 n={normal},e=vec3(stageCoordinates[{i-2}].w,stageCoordinates[{i-1}].w,stageCoordinates[{i}].w);',
                              f'samples[{i}]=fetchStageCube({s},2.0*dot(n,e)/dot(n,n)*n-e);','}'];valid[i]=True
    lines += ['}']
    kill = ['void textureAlphaKill(vec4 samples[4]){']
    for i,mode in enumerate(modes):
        if mode in (1,2,3,9,11,12,14,15,16):
            kill += [f'if((material.textureEffects[{i}].x&4u)!=0u && samples[{i}].a==0.0)discard;']
    kill += ['}']
    return '\n'.join(lines)+'\n','\n'.join(kill)+'\n',modes


def lower_fragment_program(words):
    if not isinstance(words,list) or len(words)!=60 or any(type(x)is not int or not 0<=x<=0xffffffff for x in words):
        raise ValueError('Fragment kernel requires all60 literal uint32 words')
    validate_combiner_controls(words)
    textures,kill,modes = _textures(words)
    return {'combiner_glsl':_combiner(words),'texture_glsl':textures,
            'alpha_kill_glsl':kill,'modes':modes,'words':list(words)}


def lower_combiner_program(words, *, color_expression=None, preserve_source_operations=False):
    """Lower a complete source combiner definition to fixed arithmetic.

    This public entry point is shared by the finite weather/effect compilers.
    Texture addressing stays with each family's independently owned wrapper;
    only the eighteen packed color words remain runtime uniform data.
    preserve_source_operations retains AB/CD/SUM rounding boundaries for
    families whose rendered source comparison requires them.
    """
    if not isinstance(words,list) or len(words)!=60 or any(type(x)is not int or not 0<=x<=0xffffffff for x in words):
        raise ValueError('Combiner kernel requires all60 literal uint32 words')
    validate_combiner_controls(words)
    return _combiner(words,color_expression,preserve_source_operations)


def build_fragment_source(source, lowered, *, authored_vertex=True):
    marker = '#include "world_combiner_specialization.glsl"\n#include "combiner.glsl"'
    if source.count(marker)!=1 or source.count('#include "texture_program.glsl"')!=1:
        raise ValueError('World fragment ABI markers changed')
    source = source.replace(marker,lowered['combiner_glsl'])
    source = source.replace('#include "texture_program.glsl"',lowered['texture_glsl'])
    start = source.index('void textureAlphaKill('); end = source.index('void main()',start)
    source = source[:start]+lowered['alpha_kill_glsl']+source[end:]
    # Keep the source static-lightmap, forward and fixed-input branches. They
    # select ordinary material data, not an interpreted program. The presence
    # of an authored vertex program is a compile-time kernel property.
    source = re.sub(r'\bmaterial\.flags\.w\s*!=\s*0u',
                    'true' if authored_vertex else 'false',source)
    words=lowered['words']
    def literal_word(match):
        index=int(match.group(1))
        if not 0<=index<60:
            raise ValueError('World fragment control read escapes complete program')
        if index in {*range(10,26),43,44}:
            return f'material.program[{index//4}][{index%4}]'
        return f'{words[index]}u'
    source = re.sub(r'\bword\s*\(\s*(\d+)u?\s*\)',literal_word,source)
    if re.search(r'\bword\s*\(|vertexInstructions\[.*\].*=',source):
        raise ValueError('Native fragment retained runtime program-word decoding')
    return source
