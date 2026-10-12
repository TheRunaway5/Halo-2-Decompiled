#!/usr/bin/env python3
"""Lower authenticated complete solver CFGs to directly compiled C++ paths.

No binary blob, opcode fetch or production emulator is emitted. Each original
instruction is lowered at generation time to explicit integer/SSE/x87 arithmetic,
memory ownership, a C++ label/branch, or a statically bound native callee.
Unknown operations fail generation; required external owners remain explicit.
"""
import argparse, hashlib, json, struct
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT, Xbe, _Code, _trace, PIN
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

ENTRIES=(0x322ed0,0x322d60,0x322ad0,0x322ab0,0x322ac0)
REGS={}
for index,(full,word,lo,hi) in enumerate((('eax','ax','al','ah'),('ecx','cx','cl','ch'),
    ('edx','dx','dl','dh'),('ebx','bx','bl','bh'),('esp','sp',None,None),('ebp','bp',None,None),
    ('esi','si',None,None),('edi','di',None,None))):
    for name,n,shift in ((full,4,0),(word,2,0),(lo,1,0),(hi,1,8)):
        if name:REGS[name]=(index,n,shift)
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--correspondence',type=Path,required=True)
    ap.add_argument('--output',type=Path,required=True);ap.add_argument('--manifest',type=Path,required=True)
    a=ap.parse_args();paired=json.loads(a.correspondence.read_text())
    assert paired['complete_bodies']==25 and paired['disc_1_1_sha256']==PIN and paired['later_owned_xbe_sha256']==NEW_PIN
    assert paired['extension_tool_sha256']==sha(ROOT/'tools/research_physics_solver_20261011.py')
    path=ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'
    assert sha(path)==PIN and sha(NEW_DEFAULT)==NEW_PIN
    old,new=Xbe(path),Xbe(NEW_DEFAULT);code=_Code(old,'.text')
    instruction_map={int(k,16):v for k,v in paired['instruction_map'].items()}
    addresses={int(k,16):v for k,v in paired['address_map'].items()}
    for p,q in instruction_map.items():
        assert p not in addresses or addresses[p]==q
        addresses[p]=q
    used_addresses=set();branches={};lowered={};externals={}
    def imm(p):
        p&=0xffffffff
        if p in addresses:
            used_addresses.add(p);return f'selected(0x{p:x}u,s.image)'
        return f'0x{p:x}u'
    def name(i,op):return i.reg_name(op.reg)
    def addr(i,op):
        assert op.type==3 and not op.mem.segment
        parts=[imm(op.mem.disp)]
        if op.mem.base:
            register=i.reg_name(op.mem.base);assert register in REGS and REGS[register][1]==4
            parts.append(f's.gp[{REGS[register][0]}]')
        if op.mem.index:
            register=i.reg_name(op.mem.index);assert register in REGS and REGS[register][1]==4
            parts.append(f's.gp[{REGS[register][0]}]*{op.mem.scale}u')
        return '('+'+'.join(parts)+')'
    def get(i,op):
        if op.type==2:return imm(op.imm)
        if op.type==3:return f'r(s.memory,{addr(i,op)},{op.size})'
        assert op.type==1;register=name(i,op)
        if register.startswith('xmm'):return f's.xmm[{int(register[3:])}]'
        if register.startswith('st('):return f's.fp.st({int(register[3:-1])})'
        index,n,shift=REGS[register];return f's.reg({index},{n},{shift})'
    def put(i,op,value):
        if op.type==3:
            assert op.size<=4
            return f'w(s.memory,s.observe,0x{i.address:x}u,{addr(i,op)},{op.size},{value});'
        assert op.type==1;register=name(i,op)
        if register.startswith('xmm'):return f's.xmm[{int(register[3:])}]={value};'
        if register.startswith('st('):return f's.fp.st({int(register[3:-1])})={value};'
        index,n,shift=REGS[register];return f's.set({index},{value},{n},{shift});'
    def xv(i,op):return f'v(s.memory,{addr(i,op)})' if op.type==3 else get(i,op)
    def xf(i,op):return f'ext(f({get(i,op)}))' if op.type==3 else get(i,op)
    def emit(i):
        op=list(i.operands);mn=i.mnemonic;p=i.address;next=p+i.size
        if mn=='nop':return ';'
        if mn=='lea':return put(i,op[0],addr(i,op[1]))
        if mn in ('mov','movzx','movsx'):
            value=get(i,op[1])
            if mn=='movsx':value=f'sign_extend({value},{op[1].size})'
            return put(i,op[0],value)
        if mn=='movaps':
            if op[0].type==3:return f'vw(s.memory,s.observe,0x{p:x}u,{addr(i,op[0])},{xv(i,op[1])});'
            return put(i,op[0],xv(i,op[1]))
        if mn=='movss':
            if op[0].type==3:return f'w(s.memory,s.observe,0x{p:x}u,{addr(i,op[0])},4,u(_mm_cvtss_f32({get(i,op[1])})));'
            if op[1].type==3:return put(i,op[0],f'_mm_set_ss(f({get(i,op[1])}))')
            return put(i,op[0],f'_mm_move_ss({get(i,op[0])},{get(i,op[1])})')
        if mn in ('addps','subps','mulps','addss'):
            intrinsic={'addps':'_mm_add_ps','subps':'_mm_sub_ps','mulps':'_mm_mul_ps','addss':'_mm_add_ss'}[mn]
            return put(i,op[0],f'{intrinsic}({get(i,op[0])},{xv(i,op[1])})')
        if mn=='shufps':return put(i,op[0],f'_mm_shuffle_ps({get(i,op[0])},{get(i,op[1])},{op[2].imm})')
        if mn in ('add','sub','cmp','test','xor','and','or'):
            x,y=get(i,op[0]),get(i,op[1]);n=op[0].size
            if mn in ('add','sub','cmp'):value=f's.flags.{"plus" if mn=="add" else "minus"}({x},{y},{n})'
            else:value=f's.flags.logic(({x}){ {"test":"&","and":"&","xor":"^","or":"|"}[mn]}({y}),{n})'
            return value+';' if mn in ('cmp','test') else put(i,op[0],value)
        if mn in ('inc','dec'):return put(i,op[0],f's.flags.increment({get(i,op[0])},{op[0].size},{str(mn=="dec").lower()})')
        if mn=='shl':return put(i,op[0],f's.flags.shift({get(i,op[0])},{get(i,op[1])},{op[0].size})')
        if mn=='imul':return put(i,op[0],f's.flags.product({get(i,op[-2])},{get(i,op[-1])})')
        if mn in ('div','idiv'):return f's.quotient({get(i,op[0])},{str(mn=="idiv").lower()});'
        if mn=='cdq':return 's.gp[2]=si(s.gp[0])<0?0xffffffffu:0u;'
        if mn=='push':return f's.push(0x{p:x}u,{get(i,op[0])});'
        if mn=='pop':return put(i,op[0],'s.pop()')
        if mn=='ret':return f's.pop();s.gp[4]+={op[0].imm if op else 0}u;return;'
        if mn=='call':
            if op[0].type==2 and op[0].imm in ENTRIES:
                used_addresses.add(next)
                return f's.push(0x{p:x}u,{imm(next)});run_{op[0].imm:x}(s);'
            # All external call ABI is established by the complete body.
            if op[0].type==2:
                assert op[0].imm in (0x378aa0,0x2dfd80)
                count=0 if op[0].imm==0x378aa0 else 1
                target=imm(op[0].imm)
            else:
                assert p==0x32463b;count=2;target=get(i,op[0])
            externals[f'{p:08x}']=dict(arguments=count,cdecl=True)
            return f's.required(0x{p:x}u,{target},{imm(next)},std::nullopt,{count});'
        if mn=='jmp' and op[0].type==2:return f'goto L{op[0].imm:x};'
        if mn.startswith('j') and mn!='jmp':
            condition={'je':'s.flags.zf','jne':'!s.flags.zf','jp':'s.flags.pf','ja':'!s.flags.cf&&!s.flags.zf',
                'jle':'s.flags.zf||(s.flags.sf!=s.flags.of)','jl':'s.flags.sf!=s.flags.of',
                'jge':'s.flags.sf==s.flags.of'}[mn]
            return f'if({condition})goto L{op[0].imm:x};'
        if mn=='jmp':
            assert op[0].type==3
            table=op[0].mem.disp;count={0x324c34:17,0x324c78:7,0x324c94:8}[table]
            targets=struct.unpack('<'+'I'*count,old.read(table,4*count))
            actual_table=addresses[table];new_targets=struct.unpack('<'+'I'*count,new.read(actual_table,4*count))
            for x,y in zip(targets,new_targets):assert instruction_map[x]==y
            branches.update({x:instruction_map[x] for x in targets})
            return 'switch(canonical_branch('+get(i,op[0])+',s.image)){'+''.join(f'case 0x{x:x}u:goto L{x:x};' for x in sorted(set(targets)))+'default:throw std::runtime_error("Required original solver dispatch target unavailable");}'
        if mn=='fld':return f's.fp.push({xf(i,op[0])});'
        if mn in ('fst','fstp'):
            value='s.fp.st(0)'
            if op[0].type==3:value=f'u(spill({value}))'
            return put(i,op[0],value)+('s.fp.pop();' if mn=='fstp' else '')
        if mn=='fxch':
            # Capstone includes implicit ST(0) before the encoded ST(i).
            assert len(op)==2 and name(i,op[0])=='st(0)'
            return f'std::swap(s.fp.st(0),{get(i,op[1])});'
        if mn=='fnstsw':return put(i,op[0],'s.fp.word()')
        if mn in ('fcom','fcomp','fcompp'):
            value='s.fp.st(1)' if mn=='fcompp' else xf(i,op[0])
            return f's.fp.comparison({value},{ {"fcom":0,"fcomp":1,"fcompp":2}[mn]});'
        if mn=='fabs':return 's.fp.st(0)=std::fabs(s.fp.st(0));'
        if mn=='fsqrt':return 's.fp.st(0)=root(s.fp.st(0));'
        if mn in ('fadd','fmul','fsub','fdiv','fdivr','fsubp','fsubrp','fmulp','faddp'):
            operation='add' if 'add' in mn else 'mul' if 'mul' in mn else 'sub' if 'sub' in mn else 'divide'
            popping=mn.endswith('p');reverse=mn in ('fdivr','fsubrp')
            x,y=(get(i,op[0]),'s.fp.st(0)') if popping else ('s.fp.st(0)',xf(i,op[0]))
            if reverse:x,y=y,x
            value=f'{operation}({x},{y})'
            return (put(i,op[0],value)+'s.fp.pop();') if popping else 's.fp.st(0)='+value+';'
        raise AssertionError((hex(p),mn,i.op_str))
    bodies=[]
    for entry in ENTRIES:
        ins=[code.at(p) for p in sorted(_trace(code,entry,set(ENTRIES)).code)]
        targets={i.operands[0].imm for i in ins if i.mnemonic.startswith('j') and i.operands[0].type==2}
        for i in ins:
            if i.mnemonic=='jmp' and i.operands[0].type==3:
                table=i.operands[0].mem.disp;count={0x324c34:17,0x324c78:7,0x324c94:8}[table]
                targets.update(struct.unpack('<'+'I'*count,old.read(table,4*count)))
        lines=[f'static void run_{entry:x}(State &s) {{']
        for i in ins:
            translated=emit(i);lowered[f'{i.address:08x}']=dict(mnemonic=i.mnemonic,source_bytes=i.bytes.hex())
            label=f'L{i.address:x}: ' if i.address in targets else ''
            lines.append(f'{label}if(s.services.instruction){{s.services.instruction(0x{i.address:x}u);}} {translated}')
        lines.append('throw std::runtime_error("Original solver fell outside authenticated CFG");\n}')
        bodies.append('\n'.join(lines))
    used_addresses.update(ENTRIES)
    assert used_addresses<=set(addresses),(used_addresses-set(addresses))
    preamble='''// Generated by tools/generate_physics_solver_20261011.py from authenticated
// complete paired bodies. Edit the generator, not individual lowered paths.
#include "physics_solver_internal.hpp"
namespace halo2 {
namespace {
using namespace physics_solver_detail;
std::uint32_t sign_extend(std::uint32_t x,unsigned n) {
    if(n==1)return std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(x))));
    if(n==2)return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(x))));
    throw std::runtime_error("Unverified solver sign-extension width");
}
'''
    preamble+='std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage image){if(image==SourcePhysicsContactImage::MapPack11)return p;switch(p){'+''.join(f'case 0x{p:x}u:return 0x{addresses[p]:x}u;' for p in sorted(used_addresses))+'default:throw std::runtime_error("Unbound original solver executable address");}}\n'
    preamble+='std::uint32_t canonical_branch(std::uint32_t p,SourcePhysicsContactImage image){if(image==SourcePhysicsContactImage::MapPack11)return p;switch(p){'+''.join(f'case 0x{q:x}u:return 0x{p:x}u;' for p,q in sorted(branches.items()))+'default:throw std::runtime_error("Unbound TU1.5 solver dispatch target");}}\n'
    preamble+='\n'.join(f'static void run_{p:x}(State &);' for p in ENTRIES)+'\n'
    ending='''
} // namespace
std::uint32_t source_solve_physics_constraints(M &m,std::uint32_t configuration,
    std::uint32_t schemas,std::uint32_t accumulators,std::uint32_t impulses,
    std::uint32_t esp,SourcePhysicsContactImage image,FP fp,
    const SourcePhysicsSolverServices &services,const O &observe,SourcePhysicsSolverAbi abi) {
    if(image!=SourcePhysicsContactImage::MapPack11&&image!=SourcePhysicsContactImage::TitleUpdate15)
        throw std::runtime_error("Unverified physics solver executable image");
    FpScope scope(fp);State s{m,observe,services,image};
    s.gp={0,0,0,abi.ebx,esp,abi.ebp,abi.esi,abi.edi};
    const std::array<std::uint32_t,4> args{configuration,schemas,accumulators,impulses};
    // External native-to-source ABI marshalling. Original caller writes are
    // outside this owner's observed interval, just as in the original oracle.
    for(unsigned i=0;i<4;++i)w(m,{},0,esp+4u+i*4u,4,args[i]);
    run_322ed0(s);
    if(s.fp.count)throw std::runtime_error("Unbalanced original solver x87 return");
    return s.gp[0];
}
} // namespace halo2
'''
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(preamble+'\n\n'.join(bodies)+ending)
    report=dict(schema=1,generator_sha256=sha(Path(__file__)),correspondence_sha256=sha(a.correspondence),
        original_sha256=PIN,later_original_sha256=NEW_PIN,generated_cpp_sha256=sha(a.output),
        complete_native_entries=[f'{p:08x}' for p in ENTRIES],lowered_instructions=lowered,
        selected_addresses={f'{p:08x}':addresses[p] for p in sorted(used_addresses)},
        external_owners=externals,dispatch_targets={f'{p:08x}':q for p,q in sorted(branches.items())},
        production_opcode_decoder=False,production_original_code_execution=False)
    a.manifest.parent.mkdir(parents=True,exist_ok=True);a.manifest.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(instructions=len(lowered),generated_cpp_sha256=sha(a.output),manifest_sha256=sha(a.manifest))))
if __name__=='__main__':main()
