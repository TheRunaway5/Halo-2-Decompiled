#!/usr/bin/env python3
"""Translate resident original particle programs into a direct GPU probe.

This is a diagnostic compiler for the observed opcode subset, not a production
particle renderer. Reads the actual disc tokens and preserves parallel MAC/ILU
inputs and writes. Arithmetic follows the captured Xemu Vulkan shader. No input
values, constants or expected output vertices are embedded in the generated
program. Unsupported instructions fail instead of receiving approximations.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

from audit_rendering import Cache

ROOT=Path(__file__).resolve().parents[1]


def source(t,which):
    _,y,z,w=t
    if which==0:
        mux=(z>>26)&3;reg=z>>28;neg=(y>>8)&1
        sw=[(y>>shift)&3 for shift in (6,4,2,0)]
    elif which==1:
        mux=(z>>11)&3;reg=(z>>13)&15;neg=(z>>25)&1
        sw=[(z>>shift)&3 for shift in (23,21,19,17)]
    else:
        mux=(w>>28)&3;reg=(z&3)*4+(w>>30);neg=(z>>10)&1
        sw=[(z>>shift)&3 for shift in (8,6,4,2)]
    if mux==1:
        if reg>12:raise ValueError('temporary register outside NV2A bank')
        base='outv[0]' if reg==12 else f'r[{reg}]'
    elif mux==2:base=f'v[{(y>>9)&15}]'
    elif mux==3:base=f'constant({(y>>13)&255}'+('+address' if w&2 else '')+')'
    else:raise ValueError('unsupported source register')
    return ('-' if neg else '')+f'({base}).'+''.join('xyzw'[i]for i in sw)


def write(target,value,mask):
    sw=''.join('xyzw'[i]for i in range(4)if mask&(8>>i))
    return f'    {target}.{sw}={value}.{sw};' if sw else ''


def translate(code,name,attribute_expression='fixtures[fixtureIndex].attributes[i]',corner_expression=None):
    header,count=struct.unpack_from('<2H',code)
    assert header==0x2078 and len(code)==4+count*16 and 0<count<=136
    lines=[f'void {name}() {{',
        '  for(int i=0;i<13;++i)r[i]=vec4(0);',
        f'  for(int i=0;i<16;++i){{outv[i]=vec4(0,0,0,1);v[i]={attribute_expression};}}',
        '  address=0;']
    if corner_expression:lines.append(f'  v[0]={corner_expression};')
    for index,t in enumerate(struct.iter_unpack('<4I',code[4:])):
        _,y,z,w=t;mac=(y>>21)&15;ilu=(y>>25)&7
        assert (w&1)==int(index+1==count)
        a=source(t,0)if mac else 'vec4(0)'
        b=source(t,1)if mac==2 or 4<=mac<=12 else 'vec4(0)'
        c=source(t,2)if ilu or mac in (3,4)else 'vec4(0)'
        m={0:'vec4(0)',1:'a',2:'multiply(a,b)',3:'a+c',4:'multiply(a,b)+c',
           5:'vec4(dot(a.xyz,b.xyz))',6:'vec4(dot(vec4(a.xyz,1),b))',
           7:'vec4(dot(a,b))',9:'min(a,b)',10:'max(a,b)',
           11:'vec4(lessThan(a,b))',12:'vec4(greaterThanEqual(a,b))',13:'vec4(0)'}.get(mac)
        l={0:'vec4(0)',1:'c',2:'vec4(1.0/c.x)',3:'vec4(clampAwayZeroInf(1.0/c.x))',
           4:'vec4(inversesqrt(abs(c.x)))',
           5:'vec4(exp2(floor(c.x)),c.x-floor(c.x),exp2(c.x),1)'}.get(ilu)
        if m is None or l is None:raise ValueError(f'unsupported probe opcode {mac}/{ilu}')
        lines += [f'  {{ // original instruction {index}',f'    vec4 a={a},b={b},c={c};',
                  f'    vec4 m={m},l={l};']
        reg=(w>>20)&15;mm=(w>>24)&15;im=(w>>16)&15;om=(w>>12)&15
        if mac==13:lines.append('    address=int(floor(a.x+0.001));')
        elif mac and mm and not(ilu and reg==1):
            if reg>12:raise ValueError('temporary destination outside NV2A bank')
            lines.append(write('outv[0]' if reg==12 else f'r[{reg}]','m',mm))
        if ilu and im:
            ir=1 if mac else reg
            if ir>12:raise ValueError('temporary destination outside NV2A bank')
            lines.append(write('outv[0]' if ir==12 else f'r[{ir}]','l',im))
        if om:
            if not w&0x800 or (w>>3)&255>15:raise ValueError('constant destination unsupported')
            lines.append(write(f'outv[{(w>>3)&15}]','l' if w&4 else 'm',om))
        lines.append('  }')
    return '\n'.join(lines+['}'])


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,default=ROOT/'analysis/frontend/title-particle-specialized.comp')
    parser.add_argument('--classifications',type=int,nargs='+',default=[7],help='explicit original classifications to translate')
    args=parser.parse_args()
    cache=Cache(ROOT/'assets/disc/maps/mainmenu.map');_,root=cache.tag(0xe20e0099,'vrtx')
    programs=[];functions=[]
    for classification,at in enumerate(cache.block(root+4,28)):
        if classification not in args.classifications:continue
        size=cache.u32(at+12)
        if not size:continue
        start=cache.pointer(cache.u32(at+16),cache.magic,size);code=bytes(cache.data[start:start+size])
        functions.append(translate(code,f'program{classification}'))
        programs.append({'classification':classification,'sha256':hashlib.sha256(code).hexdigest()})
    if set(args.classifications)!={r['classification']for r in programs}:
        parser.error('requested classification is missing or has no compiled program')
    preamble='''#version 450
layout(local_size_x=1) in;
struct Fixture {uvec4 instructions[136];vec4 attributes[16];uvec4 flags;vec4 constants[192];};
layout(set=0,binding=0,std430) readonly buffer Inputs {Fixture fixtures[];};
layout(set=0,binding=1,std430) writeonly buffer Outputs {vec4 results[];};
vec4 r[13],v[16],outv[16];int address;uint fixtureIndex;
vec4 constant(int index){return fixtures[fixtureIndex].constants[index];}
vec4 multiply(vec4 a,vec4 b) {
  vec4 az=mix(a,vec4(1),isnan(a)),bz=mix(b,vec4(1),isnan(b));
  vec4 zeros=sign(az)*sign(bz),ret=a*b;
  if(zeros.x==0)ret.x=0;if(zeros.y==0)ret.y=0;
  if(zeros.z==0)ret.z=0;if(zeros.w==0)ret.w=0;
  return ret;
}
float clampAwayZeroInf(float t) {
  return t>0 || floatBitsToUint(t)==0u?
    clamp(t,uintBitsToFloat(528482304u),uintBitsToFloat(1602224128u)):
    clamp(t,uintBitsToFloat(3749707776u),uintBitsToFloat(2675965952u));
}
'''
    main=['void main() {','  fixtureIndex=gl_GlobalInvocationID.x;',
          '  switch(fixtures[fixtureIndex].flags.z) {']
    for row in programs:main.append(f'    case {row["classification"]}u:program{row["classification"]}();break;')
    main+=['    default:return;','  }','  for(int i=0;i<16;++i)results[fixtureIndex*16u+uint(i)]=outv[i];','}']
    generated=preamble+'\n'.join(functions)+'\n'+'\n'.join(main)+'\n'
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(generated)
    args.output.with_suffix('.json').write_text(json.dumps({'original_vertex_datum':'e20e0099','programs':programs,
        'source_sha256':hashlib.sha256(generated.encode()).hexdigest(),
        'diagnostic_only':True,'expected_vertices_embedded':False,'particle_renderer_implemented':False},indent=2)+'\n')
    print(f'{len(programs)} original programs translated to {args.output}')


if __name__=='__main__':main()
