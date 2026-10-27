"""Preserve separate source combiner arithmetic before SPIR-V optimization.

Only typed float32 arithmetic in the generated combiner/color functions is
decorated. Reference functions and texture/family wrappers are left untouched.
The compiler must run spirv-val before and after this pass and after optimization.
"""
import re
import struct

ARITHMETIC=frozenset((129,131,133,136,142,148))  # FAdd/Sub/Mul/Div, VectorTimesScalar, Dot.
NATIVE_FUNCTION=re.compile(r'^(?:runCombiners|argb|runOriginalCombiners|originalArgb)\d*\(')


def fixed_combiner_precision(raw):
    if len(raw)<20 or len(raw)%4:
        raise ValueError('Native precision pass requires complete SPIR-V words')
    words=list(struct.unpack('<'+str(len(raw)//4)+'I',raw))
    if words[0]!=0x07230203 or words[4]!=0:
        raise ValueError('Native precision pass requires an ordinary SPIR-V module')
    names={};types={};existing=set();records=[];function=None;function_ids=set()
    position=5;insertion=None
    while position<len(words):
        size,opcode=words[position]>>16,words[position]&65535
        if not size or position+size>len(words):raise ValueError('Truncated SPIR-V precision input')
        values=words[position+1:position+size]
        if opcode==5:
            if len(values)<2:raise ValueError('Truncated SPIR-V function name')
            names[values[0]]=struct.pack('<'+'I'*(len(values)-1),*values[1:]).split(b'\0',1)[0].decode('utf-8')
        if 19<=opcode<=39 and insertion is None:insertion=position
        if opcode==22:
            if len(values)!=2:raise ValueError('Invalid SPIR-V float type')
            types[values[0]]=('float',values[1])
        elif opcode==23:
            if len(values)!=3:raise ValueError('Invalid SPIR-V vector type')
            types[values[0]]=('vector',values[1],values[2])
        if opcode==54:
            if len(values)!=4:raise ValueError('Invalid SPIR-V function')
            function=values[1];function_ids.add(function)
        elif opcode==56:function=None
        if opcode in ARITHMETIC:
            if len(values)<4:raise ValueError('Truncated SPIR-V arithmetic')
            records.append((function,opcode,values[0],values[1]))
        if opcode==12 and len(values)>=4 and values[3]==50:
            # Source combiners do not contain an explicit GLSL Fma. Do not
            # relabel a module after contraction has already occurred.
            records.append((function,'fma',values[0],values[1]))
        if opcode==71 and len(values)>=2 and values[1]==42:existing.add(values[0])
        position+=size
    selected={identity for identity in function_ids if NATIVE_FUNCTION.match(names.get(identity,''))}
    if not selected or not any(names.get(identity,'').startswith(('runCombiners','runOriginalCombiners')) for identity in selected):
        raise ValueError('Native precision pass lacks original generated combiner function ownership')
    results=[];operations={}
    for owner,opcode,type_id,result in records:
        if owner not in selected:continue
        if opcode=='fma':raise ValueError('Native combiner was contracted before its precision pass')
        shape=types.get(type_id)
        component=types.get(shape[1]) if shape and shape[0]=='vector' else shape
        if component!=('float',32):raise ValueError('Native combiner arithmetic is not typed float32')
        if not 0<result<words[3]:raise ValueError('Native arithmetic result escapes the SPIR-V ID bound')
        results.append(result);operations[str(opcode)]=operations.get(str(opcode),0)+1
    if len(results)!=len(set(results)):raise ValueError('Native precision pass found repeated arithmetic result ownership')
    if insertion is None:raise ValueError('Native precision input has no type section')
    annotations=[]
    for result in sorted(set(results)-existing):annotations += [(3<<16)|71,result,42]
    words[insertion:insertion]=annotations
    return struct.pack('<'+str(len(words))+'I',*words),dict(
        schema='HALO2_NATIVE_COMBINER_PRECISION_V1',separate_source_arithmetic=True,
        function_names=sorted(names[identity] for identity in selected),
        float32_arithmetic_results=len(results),new_no_contraction_decorations=len(annotations)//3,
        arithmetic_opcodes=operations)
