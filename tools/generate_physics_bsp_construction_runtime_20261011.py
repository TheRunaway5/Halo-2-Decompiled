#!/usr/bin/env python3
"""Lower authenticated complete contact CFGs to directly compiled C++ paths.

No binary blob, opcode fetch or production emulator is emitted. Each original
instruction is lowered at generation time to explicit integer/SSE/x87 arithmetic,
memory ownership, a C++ label/branch, or a statically bound native callee.
Unknown operations fail generation; required external owners remain explicit.
"""
import argparse, hashlib, json, struct
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT, Xbe, _Code, _trace, PIN
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN


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
    assert paired['complete_bodies']==398 and paired['disc_1_1_sha256']==PIN and paired['later_owned_xbe_sha256']==NEW_PIN
    assert paired['extension_tool_sha256']==sha(ROOT/'tools/research_physics_bsp_construction_runtime_20261011.py')
    ENTRIES=tuple(sorted(int(x,16) for x in paired['entries']))
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
        assert op.type==3
        assert not op.mem.segment or i.reg_name(op.mem.segment)=='fs'
        parts=[imm(op.mem.disp)]
        if op.mem.segment:parts.append('s.fs_base')
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
    def xv(i,op):
        if op.type!=3:return get(i,op)
        # A scalar SSE source owns four bytes. A vector load here would read
        # unrelated memory and can fault beyond the original operand extent.
        assert op.size in (4,16),(hex(i.address),i.mnemonic,op.size)
        return f'_mm_set_ss(f({get(i,op)}))' if op.size==4 else f'v(s.memory,{addr(i,op)})'
    def xf(i,op):return f'ext(f({get(i,op)}))' if op.type==3 else get(i,op)
    def emit(i):
        op=list(i.operands);mn=i.mnemonic;p=i.address;next=p+i.size
        if mn=='nop':return ';'
        if mn=='leave':return 's.gp[4]=s.gp[5];s.gp[5]=s.pop();'
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
        if mn in ('addps','subps','mulps','addss','subss','mulss','xorps','andps','maxps','minps','cmpltps','divss'):
            intrinsic={'addps':'_mm_add_ps','subps':'_mm_sub_ps','mulps':'_mm_mul_ps','addss':'_mm_add_ss','subss':'_mm_sub_ss','mulss':'_mm_mul_ss','xorps':'_mm_xor_ps','andps':'_mm_and_ps','maxps':'_mm_max_ps','minps':'_mm_min_ps','cmpltps':'_mm_cmplt_ps','divss':'_mm_div_ss'}[mn]
            return put(i,op[0],f'{intrinsic}({get(i,op[0])},{xv(i,op[1])})')
        if mn=='cvtsi2ss':return put(i,op[0],f'_mm_cvtsi32_ss({get(i,op[0])},si({get(i,op[1])}))')
        if mn=='movmskps':return put(i,op[0],f'std::uint32_t(_mm_movemask_ps({get(i,op[1])}))')
        if mn=='sqrtss':return put(i,op[0],f'_mm_move_ss({get(i,op[0])},_mm_sqrt_ss({xv(i,op[1])}))')
        if mn=='rsqrtss':
            return put(i,op[0],f'_mm_move_ss({get(i,op[0])},_mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(source_estimate(s,u(_mm_cvtss_f32({xv(i,op[1])})))))))')
        if mn=='shufps':return put(i,op[0],f'_mm_shuffle_ps({get(i,op[0])},{get(i,op[1])},{op[2].imm})')
        if mn in ('add','sub','cmp','test','xor','and','or','sbb'):
            x,y=get(i,op[0]),get(i,op[1]);n=op[0].size
            if mn=='sbb':value=f's.flags.borrow({x},{y},{n})'
            elif mn in ('add','sub','cmp'):value=f's.flags.{"plus" if mn=="add" else "minus"}({x},{y},{n})'
            else:value=f's.flags.logic(({x}){ {"test":"&","and":"&","xor":"^","or":"|"}[mn]}({y}),{n})'
            return value+';' if mn in ('cmp','test') else put(i,op[0],value)
        if mn in ('inc','dec'):return put(i,op[0],f's.flags.increment({get(i,op[0])},{op[0].size},{str(mn=="dec").lower()})')
        if mn=='shl':return put(i,op[0],f's.flags.shift({get(i,op[0])},{get(i,op[1])},{op[0].size})')
        if mn in ('shr','sar'):return put(i,op[0],f's.flags.right({get(i,op[0])},{get(i,op[1])},{op[0].size},{str(mn=="sar").lower()})')
        if mn=='neg':return put(i,op[0],f's.flags.minus(0,{get(i,op[0])},{op[0].size})')
        if mn=='not':return put(i,op[0],f'~({get(i,op[0])})')
        if mn=='imul' and len(op)==1:return f'{{const auto x=std::int64_t(si(s.gp[0]))*si({get(i,op[0])});s.gp[0]=std::uint32_t(x);s.gp[2]=std::uint32_t(std::uint64_t(x)>>32u);s.flags.cf=s.flags.of=x!=std::int64_t(si(s.gp[0]));}}'
        if mn=='imul':return put(i,op[0],f's.flags.product({get(i,op[-2])},{get(i,op[-1])})')
        if mn in ('div','idiv'):return f's.quotient({get(i,op[0])},{str(mn=="idiv").lower()});'
        if mn=='cdq':return 's.gp[2]=si(s.gp[0])<0?0xffffffffu:0u;'
        if mn=='push':return f's.push(0x{p:x}u,{get(i,op[0])});'
        if mn=='pop':return put(i,op[0],'s.pop()')
        if mn=='ret':return f's.pop();s.gp[4]+={op[0].imm if op else 0}u;return;'
        if mn=='rdtsc':return 'if(!s.services.timestamp_counter)throw std::runtime_error("Required original contact RDTSC owner unavailable");{const auto x=s.services.timestamp_counter();s.gp[0]=std::uint32_t(x);s.gp[2]=std::uint32_t(x>>32u);}'
        if mn in ('rep stosd','rep stosb'):
            width=4 if mn=='rep stosd' else 1
            return f'while(s.gp[1]){{w(s.memory,s.observe,0x{p:x}u,s.gp[7],{width},s.gp[0]);const auto delta=(s.other_flags&1024u)?{0x100000000-width}u:{width}u;s.gp[7]+=delta;--s.gp[1];if(s.services.instruction)s.services.instruction(0x{p:x}u);}}'
        if mn in ('rep movsd','rep movsb'):
            width=4 if mn=='rep movsd' else 1
            return f'while(s.gp[1]){{const auto x=r(s.memory,s.gp[6],{width});w(s.memory,s.observe,0x{p:x}u,s.gp[7],{width},x);const auto delta=(s.other_flags&1024u)?{0x100000000-width}u:{width}u;s.gp[6]+=delta;s.gp[7]+=delta;--s.gp[1];if(s.services.instruction)s.services.instruction(0x{p:x}u);}}'
        if mn in ('comiss','ucomiss'):return f's.flags.scalar_compare({get(i,op[0])},{xv(i,op[1])},{str(mn=="ucomiss").lower()});'
        if mn=='lahf':return 's.set(0,s.flags.bits()|2u,1,8);'
        if mn in ('sete','setne','setb','setle','setl','setg'):
            return put(i,op[0],dict(sete='s.flags.zf',setne='!s.flags.zf',setb='s.flags.cf',setle='s.flags.zf||(s.flags.sf!=s.flags.of)',setl='s.flags.sf!=s.flags.of',setg='!s.flags.zf&&(s.flags.sf==s.flags.of)')[mn])
        if mn in ('cmovge','cmove'):
            condition='s.flags.zf' if mn=='cmove' else 's.flags.sf==s.flags.of'
            return '{const auto x='+get(i,op[1])+';if('+condition+'){'+put(i,op[0],'x')+'}}'
        if mn=='call':
            used_addresses.add(next)
            if op[0].type==2 and op[0].imm in ENTRIES:
                return f's.push(0x{p:x}u,{imm(next)});run_{op[0].imm:x}(s);'
            target=get(i,op[0]);externals[f'{p:08x}']=dict(target=i.op_str,full_guest_abi=True)
            return '{const auto target='+target+f';s.push(0x{p:x}u,{imm(next)});dispatch(s,target,0x{p:x}u,false);'+'}'
        if mn=='jmp' and op[0].type==2:
            target=op[0].imm
            if target in current_sites:return f'goto L{target:x};'
            if target in ENTRIES:return f'run_{target:x}(s);return;'
            externals[f'{p:08x}']=dict(target=i.op_str,tail=True,full_guest_abi=True)
            return f'dispatch(s,{imm(target)},0x{p:x}u,true);return;'
        if mn.startswith('j') and mn!='jmp':
            condition={'je':'s.flags.zf','jne':'!s.flags.zf','jp':'s.flags.pf','ja':'!s.flags.cf&&!s.flags.zf',
                'jle':'s.flags.zf||(s.flags.sf!=s.flags.of)','jl':'s.flags.sf!=s.flags.of',
                'jge':'s.flags.sf==s.flags.of','js':'s.flags.sf','jns':'!s.flags.sf','jnp':'!s.flags.pf','jg':'!s.flags.zf&&(s.flags.sf==s.flags.of)','jbe':'s.flags.cf||s.flags.zf','jae':'!s.flags.cf','jb':'s.flags.cf'}[mn]
            return f'if({condition})goto L{op[0].imm:x};'
        if mn=='jmp':
            assert op[0].type==3
            table=op[0].mem.disp
            if table not in tables:return f'dispatch(s,{get(i,op[0])},0x{p:x}u,true);return;'
            count=tables[table]
            targets=struct.unpack('<'+'I'*count,old.read(table,4*count))
            actual_table=addresses[table];new_targets=struct.unpack('<'+'I'*count,new.read(actual_table,4*count))
            for x,y in zip(targets,new_targets):assert (x==y==0) or instruction_map[x]==y
            branches.update({x:instruction_map[x] for x in targets if x})
            if 0 in targets:branches[0]=0
            return 'switch(canonical_branch('+get(i,op[0])+',s.image)){'+''.join(f'case 0x{x:x}u:goto L{x:x};' if x else f'case 0:s.required(0x{p:x}u,0u,true);return;' for x in sorted(set(targets)))+'default:throw std::runtime_error("Required original solver dispatch target unavailable");}'
        if mn=='fistp':return put(i,op[0],'integer_spill(s.fp.st(0))')+'s.fp.pop();'
        if mn=='fild':return f's.fp.push(static_cast<long double>(si({get(i,op[0])})));'
        if mn=='fchs':return 's.fp.st(0)=-s.fp.st(0);'
        if mn=='fld':return f's.fp.push({xf(i,op[0])});'
        if mn in ('fst','fstp'):
            value='s.fp.st(0)'
            if op[0].type==3:value=f'u(spill({value}))'
            return put(i,op[0],value)+('s.fp.pop();' if mn=='fstp' else '')
        if mn=='fxch':
            # Capstone includes implicit ST(0) before the encoded ST(i).
            assert len(op)==2 and name(i,op[0])=='st(0)'
            return f'std::swap(s.fp.st(0),{get(i,op[1])});'
        if mn=='fcompi':return f's.compare_and_pop({get(i,op[0])});'
        if mn=='fnstsw':return put(i,op[0],'s.fp.word()')
        if mn in ('fcom','fcomp','fcompp','fucompp'):
            value='s.fp.st(1)' if mn in ('fcompp','fucompp') else xf(i,op[0])
            return f's.fp.comparison({value},{ {"fcom":0,"fcomp":1,"fcompp":2,"fucompp":2}[mn]});'
        if mn=='fabs':return 's.fp.st(0)=std::fabs(s.fp.st(0));'
        if mn=='fsqrt':return 's.fp.st(0)=root(s.fp.st(0));'
        if mn in ('fadd','fmul','fsub','fsubr','fdiv','fdivr','fsubp','fsubrp','fmulp','faddp'):
            operation='add' if 'add' in mn else 'mul' if 'mul' in mn else 'sub' if 'sub' in mn else 'divide'
            popping=mn.endswith('p');reverse=mn in ('fdivr','fsubr','fsubrp')
            x,y=(get(i,op[0]),'s.fp.st(0)') if popping else ('s.fp.st(0)',xf(i,op[0]))
            if reverse:x,y=y,x
            value=f'{operation}({x},{y})'
            return (put(i,op[0],value)+'s.fp.pop();') if popping else 's.fp.st(0)='+value+';'
        raise AssertionError((hex(p),mn,i.op_str))
    tables={q['disc']:q['count'] for row in paired['routines'] for q in row['tables']}
    bodies=[]
    for entry in ENTRIES:
        ins=[code.at(p) for p in sorted(_trace(code,entry,set(ENTRIES)).code)]
        current_sites={i.address for i in ins}
        targets={i.operands[0].imm for i in ins if i.mnemonic.startswith('j') and i.operands[0].type==2}
        for i in ins:
            if i.mnemonic=='jmp' and i.operands[0].type==3:
                table=i.operands[0].mem.disp
                if table not in tables:continue
                count=tables[table]
                targets.update(struct.unpack('<'+'I'*count,old.read(table,4*count)))
        lines=[f'static void run_{entry:x}(WorldState &s) {{']
        for i in ins:
            translated=emit(i);lowered[f'{i.address:08x}']=dict(mnemonic=i.mnemonic,source_bytes=i.bytes.hex())
            label=f'L{i.address:x}: ' if i.address in targets else ''
            lines.append(f'{label}if(s.services.instruction){{s.services.instruction(0x{i.address:x}u);}} {translated}')
        lines.append('throw std::runtime_error("Original solver fell outside authenticated CFG");\n}')
        bodies.append('\n'.join(lines))
    used_addresses.update(ENTRIES)
    assert used_addresses<=set(addresses),(used_addresses-set(addresses))
    addresses.update({int(k,16):v for k,v in paired['entries'].items()})
    used_addresses.update(ENTRIES)
    assert used_addresses<=set(addresses),(used_addresses-set(addresses))
    preamble='''// Generated by tools/generate_physics_world_platform_runtime_20261011.py.
// Complete authenticated bodies compile to native arithmetic and static calls.
#include "physics_world_runtime_internal.hpp"
namespace halo2 {
namespace {
using namespace physics_world_runtime_detail;
std::uint32_t source_estimate(WorldState &s,std::uint32_t input) {
    if(!s.services.reciprocal_sqrt)throw std::runtime_error("Required original contact RSQRTSS owner unavailable");
    return s.services.reciprocal_sqrt(input);
}
std::uint32_t sign_extend(std::uint32_t x,unsigned n) {
    if(n==1)return std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(x))));
    if(n==2)return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(x))));
    throw std::runtime_error("Unverified contact sign-extension width");
}
'''
    preamble+='std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage image){if(image==SourcePhysicsContactImage::MapPack11)return p;switch(p){'+''.join(f'case 0x{p:x}u:return 0x{addresses[p]:x}u;' for p in sorted(used_addresses))+'default:throw std::runtime_error("Unbound original contact executable address");}}\n'
    preamble+='std::uint32_t canonical_branch(std::uint32_t p,SourcePhysicsContactImage image){if(image==SourcePhysicsContactImage::MapPack11)return p;switch(p){'+''.join(f'case 0x{q:x}u:return 0x{p:x}u;' for p,q in sorted(branches.items()))+'default:throw std::runtime_error("Unbound TU1.5 contact dispatch target");}}\n'
    preamble+='\n'.join(f'static void run_{p:x}(WorldState &);' for p in ENTRIES)+'\n'
    preamble+='static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail){'+''.join(f'if(target==selected(0x{p:x}u,s.image)){{run_{p:x}(s);return;}}' for p in ENTRIES)+'s.required(site,target,tail);}\n'
    ending='''
} // namespace
void source_run_physics_world_runtime(M &m,std::uint32_t entry,
    SourcePhysicsContactRuntimeAbi &abi,SourcePhysicsContactImage image,FP fp,
    const SourcePhysicsContactRuntimeServices &services,const O &observe) {
    if(image!=SourcePhysicsContactImage::MapPack11&&image!=SourcePhysicsContactImage::TitleUpdate15)
        throw std::runtime_error("Unverified physics contact executable image");
    FpScope scope(fp);WorldState s{m,observe,services,image};s.load(abi);
    try {
        switch(entry) {
'''+''.join(f'case 0x{p:x}u:run_{p:x}(s);break;\n' for p in ENTRIES)+'''
        default:throw std::runtime_error("Required original contact entry unavailable");
        }
    } catch(...) {abi=s.save();throw;}
    abi=s.save();
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
