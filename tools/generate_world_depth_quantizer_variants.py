"""Create bounded depth-writer controls from retained original fragment SPIR-V.

This does not execute or compile a shader. All texture/color arithmetic, ordered
W reconstruction, clip demotion, interfaces and descriptor bindings are retained.
The legacy variant only models the current denominator/next-float difference;
it does NOT model the native smooth-W interpolation or its vertex program.
"""
import hashlib
import struct


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def build_variant(raw, kind):
    words = list(struct.unpack('<' + 'I' * (len(raw) // 4), raw))
    if len(raw) % 4 or words[0] != 0x07230203:
        raise ValueError('original fragment module is not SPIR-V')
    instructions = []
    names = {}
    definitions = {}
    at = 5
    # Only the result-producing instruction classes used by the writer chain
    # are needed. Do not pretend every instruction has a result in operand1.
    result_ops = {12, 43, 61, 124, 128, 136}
    while at < len(words):
        size, opcode = words[at] >> 16, words[at] & 0xffff
        if not size or at + size > len(words):
            raise ValueError('malformed original SPIR-V instruction')
        operands = words[at + 1:at + size]
        instructions.append((at, opcode, operands))
        if opcode == 5:
            names[operands[0]] = struct.pack(
                '<' + 'I' * (len(operands) - 1), *operands[1:]).split(b'\0')[0].decode()
        if opcode in result_ops:
            definitions[operands[1]] = (at, opcode, operands)
        at += size
    depth_ids = [id_ for id_, name in names.items() if name == 'gl_FragDepth']
    if len(depth_ids) != 1:
        raise ValueError('expected one named original depth output')
    stores = [(at, operands) for at, opcode, operands in instructions
              if opcode == 62 and operands[0] == depth_ids[0]]
    if len(stores) != 1:
        raise ValueError('expected one original fragment-depth store')
    store_at, store = stores[0]
    cast_at, cast_op, cast = definitions[store[1]]
    if cast_op != 124:
        raise ValueError('depth writer must end in a float bitcast')
    add_at, add_op, add = definitions[cast[2]]
    if add_op != 128:
        raise ValueError('depth writer must add one to the encoded float bits')
    _, one_op, one = definitions[add[3]]
    if one_op != 43 or one[2:] != [1]:
        raise ValueError('depth writer next-float constant differs')
    _, bits_op, bits = definitions[add[2]]
    if bits_op != 124:
        raise ValueError('depth writer must bitcast the normalized depth to uint')
    div_at, div_op, div = definitions[bits[2]]
    if div_op != 136:
        raise ValueError('depth writer must divide the floored W')
    _, denominator_op, denominator = definitions[div[3]]
    if denominator_op != 43 or denominator[2:] != [0x4b800000]:
        raise ValueError('original depth denominator is not literal 2^24')
    _, floor_op, floor = definitions[div[2]]
    if floor_op != 12 or floor[3] != 8:
        raise ValueError('depth numerator is not GLSL Floor')
    proof = {
        'kind': kind, 'original_sha256': sha(raw),
        'depth_output_id': depth_ids[0], 'depth_store_word': store_at,
        'original_floor_result_id': floor[1], 'division_result_id': div[1],
        'division_word': div_at, 'next_float_add_word': add_at,
        'changes': [],
        'scope': __doc__,
        'original_reconstruction_preserved': True,
        'original_clip_demotion_preserved': True,
        'native_full_fragment_implementation_verified': False}
    constant = None
    if kind == 'identity':
        pass
    elif kind == 'no-next-float':
        words[store_at + 2] = div[1]
        proof['changes'].append('depth store takes the existing division result; drops next-float output step')
    elif kind in ('legacy-quantizer', 'denominator-only'):
        fresh = words[3]
        constant = [(4 << 16) | 43, denominator[0], fresh, 0x4b7fffff]
        words[3] = fresh + 1
        words[div_at + 4] = fresh
        proof['changes'].append('only depth-writer denominator becomes literal 2^24-1 through a fresh constant')
        if kind == 'legacy-quantizer':
            words[store_at + 2] = div[1]
            proof['changes'].append('depth store takes division result without the next-float output step')
    else:
        raise ValueError('unknown depth-writer control: ' + kind)
    if constant:
        first_function = next(at for at, opcode, _ in instructions if opcode == 54)
        words[first_function:first_function] = constant
    result = struct.pack('<' + 'I' * len(words), *words)
    proof['variant_sha256'] = sha(result)
    proof['fresh_constant_word_count'] = len(constant) if constant else 0
    proof['original_demote_instruction_count'] = sum(opcode == 5380 for _, opcode, _ in instructions)
    if proof['original_demote_instruction_count'] == 0:
        raise ValueError('original clip-demotion authority is missing')
    return result, proof
