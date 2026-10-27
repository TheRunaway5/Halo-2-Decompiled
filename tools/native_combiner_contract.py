"""Defined NV2A combiner encodings accepted by the offline native compiler.

Register/mapping facts are corroborated by the authored TU1.5 program corpus,
the NV2A PS_REGISTER/PS_INPUTMAPPING/PS_COMBINEROUTPUT enum definitions, and
NVIDIA's NV_register_combiners legal input/output tables. No translator code
is incorporated here.
https://raw.githubusercontent.com/xemu-project/xemu/master/hw/xbox/nv2a/pgraph/psh_regs.h
https://registry.khronos.org/OpenGL/extensions/NV/NV_register_combiners.txt
"""

READABLE = frozenset((0,1,2,3,4,5,8,9,10,11,12,13))
WRITABLE = frozenset((0,4,5,8,9,10,11,12,13))
OUTPUT_MAPPINGS = frozenset((0,1,2,3,4,6))


def validate_combiner_controls(words):
    if (not isinstance(words,list) or len(words)!=60 or
            any(type(word)is not int or not 0<=word<=0xffffffff for word in words)):
        raise ValueError('Combiner kernel requires all60 literal uint32 words')
    count=words[53]&255
    if count>8:
        raise ValueError('Combiner stage count exceeds the eight defined source stages')
    if words[53]&~0x111ff:
        raise ValueError('Combiner stage controls contain reserved flags')
    for stage in range(count):
        for offset in (0,34):
            for shift in (24,16,8,0):
                register=(words[offset+stage]>>shift)&15
                if register not in READABLE:
                    raise ValueError(f'Combiner stage {stage} reads an undefined or final-only register {register}')
        for offset in (26,45):
            operation=words[offset+stage]
            # Match the source menu-fragment validator: RGB owns dot and
            # blue-to-alpha flags; neither output accepts bits above bit19.
            if operation&~0xfffff or (offset==26 and operation&0xc3000):
                raise ValueError(f'Combiner stage {stage} contains reserved output flags')
            if (operation>>15)&7 not in OUTPUT_MAPPINGS:
                raise ValueError(f'Combiner stage {stage} uses an undefined output mapping')
            for shift in (0,4,8):
                register=(operation>>shift)&15
                if register not in WRITABLE:
                    raise ValueError(f'Combiner stage {stage} writes a read-only or undefined register {register}')
    if not (words[8] or words[9]):
        return
    if words[9]&0x1f:
        raise ValueError('Final combiner contains reserved low flags')
    for variable,index,shift in (('A',8,24),('B',8,16),('C',8,8),('D',8,0),
                                 ('E',9,24),('F',9,16),('G',9,8)):
        operand=(words[index]>>shift)&255
        register=operand&15
        if register not in READABLE|{14,15}:
            raise ValueError(f'Final combiner {variable} reads an undefined register {register}')
        if operand>>5 not in (0,1):
            raise ValueError(f'Final combiner {variable} uses an undefined final input mapping')
        if register>=14 and ((operand&16) or variable in 'EFG' or (variable=='A' and register==14)):
            raise ValueError(f'Final combiner {variable} uses an undefined pseudo-register dependency')
