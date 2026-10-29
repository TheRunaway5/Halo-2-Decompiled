#!/usr/bin/env python3
"""Prove ordered original oPos equivalence before extending depth admission.

This offline analysis never evaluates a vertex or rewrites a floating-point
operation. It retains scalar operation order, signed operands, paired-read
snapshots and r12/oPos aliasing. Only dead computations and register names drop
out of the resulting dependency DAG. Production receives complete byte arrays.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

SEEDS = frozenset('''
1ec6f1ce76483bdaa311057ed082c237242a8999d76a3d2b3e498dbfbab51423
437a671eb1919896f7d0d3a72f1643d3addd881cbb549cbf791fc775183f85c7
46ff53294ae335c47103b1c5ec0b8b84cde853490d2e761df919f8e2589e8083
591de020207b9a4406ddc3131600e8c45c02ecfccdc36d102fa0b2d885bf4d92
8957bc85acbc060af5e15fbb07736d6e8a152f579224db7953a6a648e5baada3
93b43dc7d372f16948a821b3540c7ca2ccc359f0dd07846506e5ae4f89603132
9fd0b0d3e4f18cdb6d421fbe8c1b1032b43c66327e964402ef85dbb3412b232a
a37e3ae948caee7a6b00ddeb389fca00117dfeedc5adb2da27e37224734b6207
b2a3dce063b276cd8f7b56e774fc3a4aa035be6e752c87e714aa9ae3feb6b337
c260225837a4f3a4e9a3a9cb6b51a5bc4d3885ed4caf758e672920ad1675a470
ee2742c030b1dcbd0ff0f5ad76643f304513d2fbae16ed419ff5f03db2d44ffd
'''.split())


def sha(data):
    return hashlib.sha256(data).hexdigest()


def position_dag(code, *, output_index=0, input_lanes=None):
    if len(code)<20 or code[:2]!=b'\x78\x20':
        raise ValueError('Original vertex header missing')
    count=struct.unpack_from('<H',code,2)[0]
    if not 1<=count<=136 or len(code)!=4+count*16:
        raise ValueError('Original vertex extent changed')
    nodes={}
    def node(op,*args):
        key=(op,*args)
        if key not in nodes:nodes[key]=sha(json.dumps(key,separators=(',',':')).encode())
        return nodes[key]
    zero=node('f32-bits',0);one=node('f32-bits',0x3f800000)
    # Substituted literal fetch lanes retain their input identities in the
    # liveness walk; their hashes may have been formed by a preceding proof.
    for reg in input_lanes or {}:
        for lane in range(4):node('input',reg,lane)
    temp=[[zero]*4 for _ in range(13)]
    output=[[zero,zero,zero,one] for _ in range(16)]
    address=node('i32',0)
    def source(t,which):
        if which==0:
            mux,reg,negative=(t[2]>>26)&3,t[2]>>28,(t[1]>>8)&1
            swizzle=[(t[1]>>s)&3 for s in (6,4,2,0)]
        elif which==1:
            mux,reg,negative=(t[2]>>11)&3,(t[2]>>13)&15,(t[2]>>25)&1
            swizzle=[(t[2]>>s)&3 for s in (23,21,19,17)]
        else:
            mux,reg,negative=(t[3]>>28)&3,(t[2]&3)*4+(t[3]>>30),(t[2]>>10)&1
            swizzle=[(t[2]>>s)&3 for s in (8,6,4,2)]
        if mux==1:
            if reg>12:raise ValueError('Original temporary index exceeds r12')
            value=output[0] if reg==12 else temp[reg]
        elif mux==2:
            reg=(t[1]>>9)&15
            value=input_lanes[reg] if input_lanes is not None and reg in input_lanes else [node('input',reg,lane) for lane in range(4)]
        elif mux==3:
            reg=(t[1]>>13)&255
            if reg>=192:raise ValueError('Original constant index exceeds c191')
            value=[node('relative-constant',reg,address,lane) if t[3]&2 else
                   node('constant',reg,lane) for lane in range(4)]
        else:raise ValueError('Active original operand has no owner')
        return [node('negate',value[s]) if negative else value[s] for s in swizzle]
    def write(target,value,mask):
        for lane in range(4):
            if mask&(8>>lane):target[lane]=value[lane]
    for ordinal in range(count):
        t=struct.unpack_from('<4I',code,4+ordinal*16)
        if bool(t[3]&1)!=(ordinal+1==count):raise ValueError('Original END marker changed')
        mac,ilu=(t[1]>>21)&15,(t[1]>>25)&7
        if mac>13:raise ValueError('Unknown original MAC operation')
        used={0} if mac else set()
        if mac==2 or 4<=mac<=12:used.add(1)
        if ilu or mac in (3,4):used.add(2)
        # Every operand snapshots before either paired destination writes.
        a,b,c=[source(t,i) if i in used else [zero]*4 for i in range(3)]
        m=[zero]*4;l=[zero]*4
        multiply=lambda x,y:node('nv-f32-multiply',x,y)
        add=lambda x,y:node('ordered-f32-add',x,y)
        if mac==1:m=a
        elif mac==2:m=[multiply(x,y) for x,y in zip(a,b)]
        elif mac==3:m=[add(x,y) for x,y in zip(a,c)]
        elif mac==4:m=[add(multiply(x,y),z) for x,y,z in zip(a,b,c)]
        elif mac in (5,6,7):
            value=add(add(multiply(a[0],b[0]),multiply(a[1],b[1])),multiply(a[2],b[2]))
            if mac==6:value=add(value,b[3])
            if mac==7:value=add(value,multiply(a[3],b[3]))
            m=[value]*4
        elif mac==8:m=[one,multiply(a[1],b[1]),a[2],b[3]]
        elif mac in (9,10,11,12):
            op={9:'minimum',10:'maximum',11:'less-than',12:'greater-equal'}[mac]
            m=[node(op,x,y) for x,y in zip(a,b)]
        if ilu==1:l=c
        elif ilu in (2,3,4):l=[node({2:'reciprocal',3:'signed-clamped-reciprocal',4:'rsqrt-absolute'}[ilu],c[0])]*4
        elif ilu in (5,6,7):l=[node('ilu-'+str(ilu),lane,*c) for lane in range(4)]
        if mac==13:address=node('floor-to-address',a[0])
        reg,mm,im,om=(t[3]>>20)&15,(t[3]>>24)&15,(t[3]>>16)&15,(t[3]>>12)&15
        if reg>12 and (mm or (ilu and im and not mac)):raise ValueError('Original destination exceeds r12')
        if mac not in (0,13) and mm and not (ilu and reg==1):write(output[0] if reg==12 else temp[reg],m,mm)
        if ilu and im:
            ir=1 if mac else reg
            write(output[0] if ir==12 else temp[ir],l,im)
        if om:
            destination=(t[3]>>3)&255
            if not t[3]&2048 or destination>15:raise ValueError('Original constant write is outside the admitted output bank')
            write(output[destination],l if t[3]&4 else m,om)
    inverse={value:key for key,value in nodes.items()};seen=set();inputs=set()
    def visit(value):
        if value in seen or value not in inverse:return
        seen.add(value);item=inverse[value]
        if item[0]=='input':inputs.add(item[1])
        for child in item[1:]:
            if isinstance(child,str):visit(child)
    for lane in output[output_index]:visit(lane)
    return {'lanes':output[output_index],'sha256':sha(''.join(output[output_index]).encode()),'nodes':len(nodes),
            'input_registers':sorted(inputs)}


def audit(registry):
    programs={}
    for row in registry['entries']:
        declaration=row.get('declaration')
        if not declaration or declaration['class_index']!=0 or row['relative_constants']:continue
        if not declaration['semantics'] or declaration['semantics'][0]!=0:continue
        code=bytes.fromhex(row['vertex_program_hex']);key=sha(code)
        item=programs.setdefault(key,{'code':code,'dag':position_dag(code),'aliases':set()})
        item['aliases'].update(a['vertex_shader'] for a in row['aliases'] if 'vertex_shader' in a)
    if SEEDS-set(programs):raise ValueError('Authenticated original position seed missing from current catalog')
    signatures={programs[key]['dag']['sha256']:key for key in sorted(SEEDS)}
    admitted={key:{**value,'seed':signatures[value['dag']['sha256']]} for key,value in programs.items()
              if value['dag']['sha256'] in signatures}
    return programs,admitted


def header(admitted):
    lines=['#pragma once','#include <algorithm>','#include <array>','#include <cstddef>','#include <span>',
           'namespace halo2::source_world_depth_private {',
           '// Generated by tools/audit_source_world_depth_position.py. Complete',
           '// original tokens must have an ordered oPos DAG equal to a proven seed.',
           'inline bool position_program_admitted(std::span<const std::byte> code) {']
    for index,(key,item) in enumerate(sorted(admitted.items())):
        lines += ['    // Original token SHA256 '+key,
                  '    // Exact ordered oPos seed '+item['seed'],
                  f'    static constexpr std::array<unsigned char,{len(item["code"])}> p{index}{{'+','.join(map(str,item['code']))+'};',
                  f'    if(code.size()==p{index}.size() && std::equal(code.begin(),code.end(),p{index}.begin(),[](std::byte a,unsigned char b){{return std::to_integer<unsigned char>(a)==b;}}))return true;']
    return '\n'.join([*lines,'    return false;','}','}',''])


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--registry',type=Path,required=True);p.add_argument('--report',type=Path,required=True)
    p.add_argument('--header',type=Path);args=p.parse_args()
    raw=args.registry.read_bytes();programs,admitted=audit(json.loads(raw))
    if args.header:args.header.write_text(header(admitted))
    report={'schema':'HALO2_SOURCE_DEPTH_ORDERED_POSITION_DAG_V1','registry_sha256':sha(raw),
        'trusted_seeds':sorted(SEEDS),'programs':len(programs),'admitted':len(admitted),
        'new_admissions':len(set(admitted)-SEEDS),'algebraic_simplification':False,
        'runtime_interpretation':False,'entries':[{ 'program_sha256':key,'original_bytes':len(value['code']),
            'ordered_position_dag':value['dag'],'seed_program_sha256':value['seed'],
            'vertex_shader_aliases':sorted(value['aliases'])} for key,value in sorted(admitted.items())],
        'unknown':[{ 'program_sha256':key,'ordered_position_dag_sha256':value['dag']['sha256'],
            'vertex_shader_aliases':sorted(value['aliases'])} for key,value in sorted(programs.items()) if key not in admitted],
        'scope':'Ordered original position dependency equality; complete program bytes remain the production admission key. This does not prove whole-frame or new pixel-program parity.'}
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({key:report[key] for key in ('programs','admitted','new_admissions')}))


if __name__=='__main__':main()
