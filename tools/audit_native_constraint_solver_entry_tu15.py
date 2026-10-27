#!/usr/bin/env python3
"""Source CFG audit of actual326A40 entry dependencies, including SIMD lanes.

This is a static original-code audit, not a runtime parity claim. It tracks
must-definitions from only the genuine stack identity and treats initializer
and custom callbacks as clobbering volatile GP/SIMD values. Callee-save pushes
at entry preserve caller outputs and are excluded from physics input uses.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from capstone.x86_const import X86_OP_IMM, X86_OP_REG, X86_OP_MEM
from oracle_smg_firing_1_1 import ROOT, Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import complete_body

GP = ('eax', 'ecx', 'edx', 'ebx', 'esp', 'ebp', 'esi', 'edi')
ALIASES = {}
for n, low16, low8, high8 in zip(GP, ('ax', 'cx', 'dx', 'bx', 'sp', 'bp', 'si', 'di'), ('al', 'cl', 'dl', 'bl', None, None, None, None), ('ah', 'ch', 'dh', 'bh', None, None, None, None)):
    ALIASES[n] = (n, 0xffffffff)
    ALIASES[low16] = (n, 0xffff)
    if low8:
        ALIASES[low8] = (n, 0xff)
        ALIASES[high8] = (n, 0xff00)
SAVED_PUSHES = {0x326a40, 0x326a53, 0x326a54, 0x326a55}


def sha(path):
    return hashlib.file_digest(Path(path).open('rb'), 'sha256').hexdigest()


def analyze(image):
    code = _Code(image, '.text')
    body = complete_body(code, 0x326a40)
    assert len(body.code) == 2208
    instructions = {pc: code.at(pc) for pc in body.code}
    dispatch = {0x3287a4: 17, 0x3287e8: 7, 0x328804: 8}
    edges = {}
    for pc, instruction in instructions.items():
        following = pc + instruction.size
        if instruction.mnemonic == 'ret':
            targets = []
        elif instruction.mnemonic == 'jmp':
            operand = instruction.operands[0]
            if operand.type == X86_OP_IMM:
                targets = [operand.imm]
            else:
                assert operand.type == X86_OP_MEM and operand.mem.disp in dispatch, (hex(pc), instruction.op_str)
                count = dispatch[operand.mem.disp]
                targets = list(struct.unpack('<' + 'I'*count, image.read(operand.mem.disp, count*4)))
        elif instruction.mnemonic.startswith('j'):
            assert instruction.operands[0].type == X86_OP_IMM
            targets = [following, instruction.operands[0].imm]
        else:
            targets = [following]
        assert all(p in instructions for p in targets), (hex(pc), targets)
        edges[pc] = targets
    registers = (*GP, *(f'xmm{n}' for n in range(8)), 'eflags')
    maximum = {name: 0xffffffff if name in GP else 0xf if name.startswith('xmm') else 1 for name in registers}
    entry = {name: 0 for name in registers}
    entry['esp'] = 0xffffffff
    incoming = {pc: dict(maximum) for pc in instructions}
    incoming[0x326a40] = entry
    reachable = {0x326a40}

    def effects(pc, state, violations=None):
        instruction = instructions[pc]
        reads, writes = instruction.regs_access()
        read_masks, write_masks = {}, {}
        def add(target, name, mask):
            target[name] = target.get(name, 0) | mask
        for reg in reads:
            name = instruction.reg_name(reg)
            if name in ALIASES:
                base, mask = ALIASES[name]
                if not (pc in SAVED_PUSHES and base != 'esp'):
                    add(read_masks, base, mask)
            elif name.startswith('xmm'):
                add(read_masks, name, 0xf)
            elif name == 'eflags':
                add(read_masks, name, 1)
        for reg in writes:
            name = instruction.reg_name(reg)
            if name in ALIASES:
                base, mask = ALIASES[name]
                add(write_masks, base, mask)
            elif name.startswith('xmm'):
                add(write_masks, name, 0xf)
            elif name == 'eflags':
                add(write_masks, name, 1)
        operands = instruction.operands
        if instruction.mnemonic == 'xor' and len(operands) == 2 and operands[0].type == operands[1].type == X86_OP_REG and operands[0].reg == operands[1].reg:
            name = instruction.reg_name(operands[0].reg)
            read_masks.pop(ALIASES[name][0], None)
        if instruction.mnemonic in ('movss', 'addss'):
            for name in tuple(read_masks):
                if name.startswith('xmm'):
                    read_masks[name] = 1
            for name in tuple(write_masks):
                if name.startswith('xmm'):
                    write_masks[name] = 0xf if instruction.mnemonic == 'movss' and operands[1].type == X86_OP_MEM else 1
        if instruction.mnemonic == 'shufps':
            destination, source = (instruction.reg_name(operands[n].reg) for n in (0, 1))
            immediate = operands[2].imm
            read_masks.pop(destination, None)
            read_masks.pop(source, None)
            add(read_masks, destination, (1 << (immediate & 3)) | (1 << ((immediate >> 2) & 3)))
            add(read_masks, source, (1 << ((immediate >> 4) & 3)) | (1 << ((immediate >> 6) & 3)))
        if instruction.mnemonic == 'call' and operands[0].type == X86_OP_IMM and operands[0].imm in (0x326620, 0x326630):
            # The actual cursor helper consumes ECX in addition to its pushed
            # count. CALL's own metadata does not describe child inputs.
            add(read_masks, 'ecx', 0xffffffff)
        if violations is not None:
            for name, mask in read_masks.items():
                missing = mask & ~state[name]
                if missing:
                    violations.append(dict(instruction=pc, text=instruction.mnemonic + ' ' + instruction.op_str,
                                           register=name, missing_bits=missing))
        result = dict(state)
        for name, mask in write_masks.items():
            result[name] |= mask
        if instruction.mnemonic == 'call':
            if operands[0].type == X86_OP_IMM and operands[0].imm in (0x326620, 0x326630):
                # Authenticated complete4/5-site cursor helpers: only EAX and
                # arithmetic flags change; ECX and all XMM values survive.
                result['eax'] = 0xffffffff
                result['eflags'] = 1
            else:
                assert pc in (0x326a58, 0x3281ab)
                for name in ('eax', 'ecx', 'edx', *(f'xmm{n}' for n in range(8)), 'eflags'):
                    result[name] = 0
        return result

    changed = True
    passes = 0
    while changed:
        changed = False
        passes += 1
        for pc in sorted(instructions):
            if pc not in reachable:
                continue
            output = effects(pc, incoming[pc])
            for target in edges[pc]:
                candidate = {name: incoming[target][name] & output[name] for name in registers}
                if target not in reachable or candidate != incoming[target]:
                    incoming[target] = candidate
                    reachable.add(target)
                    changed = True
        assert passes < 1000
    violations = []
    for pc in sorted(reachable):
        effects(pc, incoming[pc], violations)
    # A separate stack-depth walk proves that native callbacks occur between
    # complete row expressions. Only genuine preserved incoming x87 values
    # can be live there; no private solver impulse is delegated to a callee.
    pending=[(0x326a40,0)];fp_states=set();fp_errors=[];maximum_private=0;call_depths={}
    while pending:
        pc,depth=pending.pop()
        if(pc,depth)in fp_states:continue
        fp_states.add((pc,depth));instruction=instructions[pc];name=instruction.mnemonic
        floating=name.startswith('f')and name!='fnstsw'
        minimum=1 if floating else 0
        for operand in instruction.operands:
            if operand.type==X86_OP_REG:
                register=instruction.reg_name(operand.reg)
                if register.startswith('st('):minimum=max(minimum,int(register[3:-1])+1)
        if name=='fld':minimum=0 if instruction.operands[0].type==X86_OP_MEM else minimum;change=1
        elif name in('fstp','fcomp','faddp','fmulp','fsubp','fsubrp'):change=-1;minimum=max(minimum,2 if name.endswith('p')and name not in('fstp','fcomp')else 1)
        elif name=='fcompp':change=-2;minimum=2
        else:change=0
        if depth<minimum:fp_errors.append(dict(instruction=hex(pc),depth=depth,required=minimum))
        after=depth+change;maximum_private=max(maximum_private,depth,after)
        if after<0 or after>8:fp_errors.append(dict(instruction=hex(pc),unsupported_depth=after));continue
        if name=='call':call_depths.setdefault(hex(pc),set()).add(depth)
        if name=='ret'and depth!=0:fp_errors.append(dict(instruction=hex(pc),return_private_depth=depth))
        pending.extend((target,after)for target in edges[pc])
    listings = {}
    for owner, count in ((0x326a40,2208),(0x326620,4),(0x326630,5)):
        child = complete_body(code, owner)
        assert len(child.code) == count
        text = ''.join(f'{pc:x} {code.at(pc).bytes.hex()} {code.at(pc).mnemonic} {code.at(pc).op_str}\n' for pc in sorted(child.code))
        listings[hex(owner)] = hashlib.sha256(text.encode()).hexdigest()
    return dict(passed=not violations and not fp_errors, root_sites=2208, reachable_sites=len(reachable), fixpoint_passes=passes,
                entry_inputs=['actual ESP/4cdecl stack arguments','explicit incoming FP control/status and preserved live x87 values'],
                ignored_save_only_pushes=[hex(pc) for pc in sorted(SAVED_PUSHES)],
                callback_clobber_sites=['326A58 actual3268D0','3281AB actualcustomschema14'],
                cursor_helper_summaries=['326620: ECX+stackcount*20 -> EAX; RET4','326630: ECX+stackcount*30 -> EAX; RET4'],
                SIMD_lane_rules=['MOVSS memory->register defines all128bits','MOVSS register->register and ADDSS consume/write onlylow32bits; untouched lanes retain must-def state','SHUFPS consumes only selected source lanes and definesall4'],
                undefined_entry_uses=violations, listing_sha256=listings,
                x87_private_stack=dict(maximum_private_depth=maximum_private,callsite_private_depths={pc:sorted(depths)for pc,depths in call_depths.items()},errors=fp_errors,assumption='Declared actual initializer/custom/cursor dependencies preserve incoming x87 depth and live values; original caller has zero private temporaries at each call'),
                limits=['static must-definition audit, not dynamic instruction/math parity','callee-save values are restored outputs, not physics inputs','FP stack liveness independently requires actual balanced dependency contracts'])


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    assert sha(NEW_DEFAULT)==NEW_PIN
    report=analyze(Xbe(NEW_DEFAULT))
    report.update(image_sha256=NEW_PIN,source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__)})
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k] for k in ('passed','root_sites','reachable_sites','fixpoint_passes','undefined_entry_uses')}))
    return 0 if report['passed'] else 1


if __name__=='__main__':
    raise SystemExit(main())
