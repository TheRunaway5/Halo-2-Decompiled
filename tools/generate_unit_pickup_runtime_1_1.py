#!/usr/bin/env python3
"""Lower complete, pinned Xbox TU1.1 inventory and pickup owners to native C++.

Only generation decodes executable instructions. Production uses static native
control flow and retained source banks. Reached external owners are mandatory.
"""
import argparse, hashlib, json, struct
from oracle_smg_firing_1_1 import ROOT, PIN, Xbe, _Code, _trace
from generate_unit_animation_selection_runtime_1_1 import REGS
ENTRIES = (0xb8250, 0xcd1a0, 0xcd2b0, 0xce370, 0xce440, 0xce510,
           0xce930, 0xcea10, 0xcea70, 0xceaf0, 0xcec00, 0xcf300,
           0xcfe60, 0xcfed0, 0xe7ef0, 0xe7f30, 0xe7ff0, 0xea660,
           0x1057f0, 0x1058f0, 0x105ed0, 0x10a1d0, 0x10a240,
           0x111490, 0x152080, 0x15b410)
TABLES = {0xce90c: 7, 0xea7c8: 7}
def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output',type=__import__('pathlib').Path,required=True)
    ap.add_argument('--manifest',type=__import__('pathlib').Path,required=True)
    a=ap.parse_args(); path=ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'
    assert sha(path)==PIN
    xbe=Xbe(path);code=_Code(xbe,'.text');lowered={};external={}
    def imm(x): return f'0x{x&0xffffffff:x}u'
    def addr(i,o):
        assert o.type==3 and not o.mem.segment
        terms=[imm(o.mem.disp)]
        for register,scale in ((o.mem.base,1),(o.mem.index,o.mem.scale)):
            if register:
                index,width,shift=REGS[i.reg_name(register)];assert width==4 and shift==0
                terms.append(f's.gp[{index}]*{scale}u')
        return '('+'+'.join(terms)+')'
    def get(i,o):
        if o.type==2:return imm(o.imm)
        if o.type==3:assert o.size<=4;return f'r(s.memory,{addr(i,o)},{o.size})'
        name=i.reg_name(o.reg)
        if name.startswith('xmm'):return f's.xmm[{int(name[3:])}]'
        index,width,shift=REGS[name];return f's.reg({index},{width},{shift})'
    def put(i,o,value):
        if o.type==3:return f'w(s.memory,s.observe,{imm(i.address)},{addr(i,o)},{o.size},{value});'
        name=i.reg_name(o.reg)
        if name.startswith('xmm'):return f's.xmm[{int(name[3:])}]={value};'
        index,width,shift=REGS[name];return f's.set({index},{value},{width},{shift});'
    conditions={'je':'s.flags.zf','jne':'!s.flags.zf','ja':'!s.flags.cf&&!s.flags.zf',
        'jbe':'s.flags.cf||s.flags.zf','jl':'s.flags.sf!=s.flags.of','jle':'s.flags.zf||(s.flags.sf!=s.flags.of)',
        'jge':'s.flags.sf==s.flags.of','jnp':'!s.flags.pf','jns':'!s.flags.sf'}
    def emit(i,sites):
        o=list(i.operands);mn=i.mnemonic;p=i.address
        if mn=='nop':return ';'
        if mn=='lea':return put(i,o[0],addr(i,o[1]))
        if mn in ('mov','movzx','movsx'):
            value=get(i,o[1])
            if mn=='movsx':value=f'sign_extend({value},{o[1].size})'
            return put(i,o[0],value)
        if mn=='movss':
            if o[0].type==3:return put(i,o[0],f'u(_mm_cvtss_f32({get(i,o[1])}))')
            return put(i,o[0],f'_mm_set_ss(f({get(i,o[1])}))'if o[1].type==3 else f'_mm_move_ss({get(i,o[0])},{get(i,o[1])})')
        if mn=='xorps':return put(i,o[0],f'_mm_xor_ps({get(i,o[0])},{get(i,o[1])})')
        if mn in ('comiss','ucomiss'):
            rhs=f'_mm_set_ss(f({get(i,o[1])}))'if o[1].type==3 else get(i,o[1])
            return f's.flags.scalar_compare({get(i,o[0])},{rhs},{str(mn=="ucomiss").lower()});'
        if mn in ('not','neg'):return put(i,o[0],f'~({get(i,o[0])})'if mn=='not'else f's.flags.minus(0,{get(i,o[0])},{o[0].size})')
        if mn in ('add','sub','cmp','and','or','xor','test','sbb'):
            left,right=get(i,o[0]),get(i,o[1]);width=o[0].size
            if mn=='sbb':value=f's.flags.borrow({left},{right},{width})'
            elif mn in ('add','sub','cmp'):value=f's.flags.{"plus"if mn=="add"else"minus"}({left},{right},{width})'
            else:value=f's.flags.logic(({left})'+{'and':'&','test':'&','or':'|','xor':'^'}[mn]+f'({right}),{width})'
            return value+';'if mn in ('cmp','test')else put(i,o[0],value)
        if mn in ('inc','dec'):return put(i,o[0],f's.flags.increment({get(i,o[0])},{o[0].size},{str(mn=="dec").lower()})')
        if mn=='imul':assert len(o)>=2;return put(i,o[0],f's.flags.product({get(i,o[-2])},{get(i,o[-1])})')
        if mn=='shl':return put(i,o[0],f's.flags.shift({get(i,o[0])},{get(i,o[1])},{o[0].size})')
        if mn in ('shr','sar'):return put(i,o[0],f's.flags.right({get(i,o[0])},{get(i,o[1])},{o[0].size},{str(mn=="sar").lower()})')
        if mn in ('setne','setg'):return put(i,o[0],'!s.flags.zf'if mn=='setne'else'!s.flags.zf&&(s.flags.sf==s.flags.of)')
        if mn=='cmovle':return '{const auto value='+get(i,o[1])+';if(s.flags.zf||(s.flags.sf!=s.flags.of)){'+put(i,o[0],'value')+'}}'
        if mn=='lahf':return 's.set(0,s.flags.bits()|2u,1,8);'
        if mn=='push':return f's.push({imm(p)},{get(i,o[0])});'
        if mn=='pop':return put(i,o[0],'s.pop()')
        if mn=='ret':return f's.pop();s.gp[4]+={o[0].imm if o else 0}u;return;'
        if mn=='call':
            if o[0].type==2 and o[0].imm in ENTRIES:return f's.push({imm(p)},{imm(p+i.size)});run_{o[0].imm:x}(s);'
            external[f'{p:08x}']={'target':i.op_str,'tail':False}
            return '{const auto target='+get(i,o[0])+f';s.push({imm(p)},{imm(p+i.size)});dispatch(s,target,{imm(p)},false);'+'}'
        if mn=='jmp':
            if o[0].type==2:
                if o[0].imm in sites:return f'goto L{o[0].imm:x};'
                if o[0].imm in ENTRIES:return f'run_{o[0].imm:x}(s);return;'
                external[f'{p:08x}']={'target':i.op_str,'tail':True}
                return f'dispatch(s,{imm(o[0].imm)},{imm(p)},true);return;'
            assert o[0].type==3 and o[0].mem.disp in TABLES
            table=o[0].mem.disp;targets=struct.unpack('<'+'I'*TABLES[table],xbe.read(table,4*TABLES[table]))
            assert set(targets)<=sites
            return 'switch('+get(i,o[0])+'){'+''.join(f'case {imm(t)}:goto L{t:x};'for t in sorted(set(targets)))+'default:throw std::runtime_error("Invalid original pickup mode target");}'
        if mn in conditions:return f'if({conditions[mn]})goto L{o[0].imm:x};'
        raise AssertionError((hex(p),mn,i.op_str))
    bodies=[]
    for entry in sorted(ENTRIES):
        instructions=[code.at(p)for p in sorted(_trace(code,entry,set(ENTRIES)).code)]
        sites={i.address for i in instructions};labels={i.operands[0].imm for i in instructions if i.mnemonic.startswith('j')and i.operands[0].type==2}
        for i in instructions:
            if i.mnemonic=='jmp'and i.operands[0].type==3:
                table=i.operands[0].mem.disp;labels.update(struct.unpack('<'+'I'*TABLES[table],xbe.read(table,4*TABLES[table])))
        lines=[f'[[gnu::noinline]] static void run_{entry:x}(State &s) {{']
        for i in instructions:
            translated=emit(i,sites);lowered[f'{i.address:08x}']={'mnemonic':i.mnemonic,'source_bytes':i.bytes.hex()}
            lines.append((f'L{i.address:x}: 'if i.address in labels else'')+f'if(s.services.instruction)s.services.instruction({imm(i.address)}); '+translated)
        lines.append('throw std::runtime_error("Original pickup left authenticated CFG");\n}');bodies.append('\n'.join(lines))
    preamble='''// Generated by tools/generate_unit_pickup_runtime_1_1.py; do not edit.
#include "halo2/unit_pickup_runtime.hpp"
#include "physics/runtime/physics_contact_runtime_internal.hpp"
namespace halo2 { namespace {
using namespace physics_contact_runtime_detail;
struct State : physics_contact_runtime_detail::State {
  State(M &m,const O &o,const SourcePhysicsContactRuntimeServices &owners,SourcePhysicsContactImage image)
    : physics_contact_runtime_detail::State{m,o,owners,image} {}
  void required(std::uint32_t site,std::uint32_t target,bool tail) {
    if(!services.required_owner)throw std::runtime_error("Required original inventory/pickup callee unavailable");
    auto a=save();
    try {services.required_owner(memory,{site,target,tail},a,observe);}
    catch(...) {load(a);throw;}
    load(a);
  }
};
std::uint32_t sign_extend(std::uint32_t x,unsigned n) {
  if(n==1)return std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(x))));
  if(n==2)return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(x))));
  throw std::runtime_error("Unverified inventory sign extension width");
}
'''
    preamble+='\n'.join(f'static void run_{e:x}(State &);'for e in sorted(ENTRIES))+'\n'
    preamble+='static void dispatch(State &s,std::uint32_t target,std::uint32_t site,bool tail){'+''.join(f'if(target=={imm(e)}){{run_{e:x}(s);return;}}'for e in ENTRIES)+'s.required(site,target,tail);}\n'
    ending='''
using PickupState=State;
} // namespace
void source_run_unit_pickup_1_1(SourcePhysicsContactMemory &memory,
    std::uint32_t entry,SourcePhysicsContactRuntimeAbi &abi,
    SourcePhysicsFactoryFloatingPoint fp,const SourcePhysicsContactRuntimeServices &services,
    const SourcePhysicsContactRecordObserver &observe) {
  FpScope scope(fp);PickupState s{memory,observe,services,SourcePhysicsContactImage::MapPack11};s.load(abi);
  try {switch(entry) {
'''+''.join(f'case {imm(e)}:run_{e:x}(s);break;\n'for e in sorted(ENTRIES))+'''
  default:throw std::runtime_error("Unported original inventory/pickup entry");
  }}catch(...){abi=s.save();throw;}
  abi=s.save();
}
} // namespace halo2
'''
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(preamble+'\n\n'.join(bodies)+ending)
    report={'schema':1,'original_sha256':PIN,'generator_sha256':sha(__import__('pathlib').Path(__file__)),
        'generated_cpp_sha256':sha(a.output),'complete_native_entries':[f'{e:08x}'for e in sorted(ENTRIES)],
        'lowered_instructions':lowered,'external_owners':external,'production_opcode_decoder':False,
        'production_original_code_execution':False,'captured_runtime_results_used':False,'live_gameplay_proof':False}
    a.manifest.parent.mkdir(parents=True,exist_ok=True);a.manifest.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps({'entries':len(ENTRIES),'instructions':len(lowered),'external_call_sites':len(external),'generated_cpp_sha256':sha(a.output)}))
if __name__=='__main__':main()
