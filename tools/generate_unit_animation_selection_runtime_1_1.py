#!/usr/bin/env python3
"""Compile complete authenticated TU1.1 animation selection owners to C++.

Instructions are lowered at generation time. Production has native arithmetic,
static calls and branches, retained bank reads/stores, and explicit required
cache scheduler/clock owners. No binary decoder, opcode dispatch or fixture
replay executes in production. This generator does not connect the live Match.
"""
import argparse, hashlib, json, struct
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT, Xbe, _Code, _trace, PIN
REGS={}
for index,(full,word,lo,hi) in enumerate((('eax','ax','al','ah'),('ecx','cx','cl','ch'),
    ('edx','dx','dl','dh'),('ebx','bx','bl','bh'),('esp','sp',None,None),('ebp','bp',None,None),
    ('esi','si',None,None),('edi','di',None,None))):
    for name,n,shift in ((full,4,0),(word,2,0),(lo,1,0),(hi,1,8)):
        if name:REGS[name]=(index,n,shift)
ENTRIES=(0x1d0790,0x1de7d0,0x1cfde0,0x1cfce0,0x1deb60,0x1df670,
 0x1dea60,0x1e02b0,0x29c6a0,0x1e0f50,0x1deb10,0x1e16f0,
 0x1e1660,0x1e1580,0x1ceda0,0x1e1020,0x1e0fc0,0x1de890,
 0x1de9b0,0x1e13c0,0x1e1230,0x124e40,0x124dd0,0x16d2c0,0x1d1b70,
 0xb8250,0xbb240,0xc83d0,0x111b60,0x114bd0,0x115af0,0x1c9e60,
 0x1c9f50,0x1ca3a0,0x1cee00,0x1cef10,0x1cefe0,0x1cf360,
 0x1cf3c0,0x1cf4c0,0x1cfb90,0x1dcc30,0x1dce20,0x1cec20,0x1cfa90,0x1c9de0,0x110b00,0x1cf310,0x1d1bd0)
POSE_ENTRIES=(0x11ef40,0x11f2d0,0x11f4a0,0x124ef0,0x124f30,0x16d390,0x16d480,0x16d4c0,0x1ca820,0x1ca890,0x1ca950,0x1cad30,0x1cf840,0x1cf8b0,0x1d0f80,0x1d18f0,0x1d1a00,0x1d1b00,0x1de9e0,0x1e11b0,0x1e1270,0x1e12e0,0x1e1530,0x27fb10,0x27fb50,0x280030,0x2803b0,0x280630,0x280670,0x2806e0,0x280750,0x2807c0,0x280830,0x2808a0,0x2808d0,0x280900,0x280930,0x280960,0x280990,0x280d70,0x280f90,0x2812d0,0x281480,0x281910,0x281bd0,0x282010,0x282290,0x282740,0x282a00,0x282e40,0x2830d0,0x2836d0,0x283a20,0x283fc0,0x2842a0,0x284960,0x284d90,0x285410,0x2857e0,0x285ec0,0x286300,0x286990,0x286d70,0x287340,0x287620,0x287b90,0x287e10,0x2884a0,0x288860,0x288e90,0x289210,0x2898b0,0x289c80,0x28a2c0,0x28a650,0x28acd0,0x28b090,0x28b6b0,0x28ba00,0x28c140,0x28c5d0,0x28ccb0,0x28d0f0,0x28d840,0x28dce0,0x28e3d0,0x28e820,0x28ee80,0x28f230,0x28f830,0x28fb60,0x290250,0x290690,0x290d50,0x291120,0x291820,0x291c70,0x292340,0x292720,0x292790,0x2927c0,0x2927e0,0x292890,0x292970,0x2929b0,0x2929f0,0x292ae0,0x292bc0,0x292c20,0x292ca0,0x292e30,0x292f80,0x293070,0x293200,0x293350,0x293430,0x2935d0,0x293720,0x293810,0x2939a0,0x293af0,0x2cb160,0x2cb1a0,0x2cb1e0,)
ENTRIES=tuple(sorted(set(ENTRIES)|set(POSE_ENTRIES)|{0xbea50,0xbe860,0x10d1e0,0xbaeb0,0x1dce60,0x1dcd60,0x1100a0,0x1dd060,0x13fca0,0x1c9e30,0x113830,0x114580,0xbbcb0,0x10d260,0x1d7310,0x1d7710,0x1d42d0,0x143580,0x1436d0,0x143ad0,0x142d00,0x1441d0,0x9d900,0x2e0b60,0x2e09e0,0xddbc0,0xbacc0,0xe5680,0x1e9540,0x1d10b0,0x1ca6f0,0x1ca7b0,0x20e440,0x1ea130,0xe1800,0x142e80,0x143f60,0x1d1290,0xdf7c0,0xe4f40,0x30bd0,0xb7650}))
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
ENTRIES=tuple(sorted(set(ENTRIES)|{0xc04f0,0xe8050,0xe6670,0xe63c0}))
RUNTIME_HEADER='halo2/unit_animation_selection_runtime.hpp'
RUNTIME_FUNCTION='source_run_unit_animation_selection_1_1'
CONFIGURATION_TOOL=None
EXTRA_TABLES={}
FORWARD_TIMESTAMP_COUNTER=False
FORWARD_BREAKPOINT=False
def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output',type=Path,required=True)
    ap.add_argument('--manifest',type=Path,required=True);a=ap.parse_args()
    path=ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'
    assert sha(path)==PIN
    original=Xbe(path);code=_Code(original,'.text');lowered={};externals={};used_addresses=set()
    tables={0x11221c:5,0x114ee4:3,0x1ced84:6,0x1dce0c:4,0xde6b4:6,0xe6844:9}
    tables.update(EXTRA_TABLES)
    def imm(p):return f'0x{p&0xffffffff:x}u'
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
        if op.type==3:return f'r64(s.memory,{addr(i,op)})' if op.size==8 else f'r(s.memory,{addr(i,op)},{op.size})'
        assert op.type==1;register=name(i,op)
        if register.startswith('xmm'):return f's.xmm[{int(register[3:])}]'
        if register.startswith('mm'):return f's.mmx[{int(register[2:])}]'
        if register.startswith('st('):return f's.fp.st({int(register[3:-1])})'
        index,n,shift=REGS[register];return f's.reg({index},{n},{shift})'
    def put(i,op,value):
        if op.type==3:
            assert op.size<=8
            return f'w(s.memory,s.observe,0x{i.address:x}u,{addr(i,op)},{op.size},{value});'
        assert op.type==1;register=name(i,op)
        if register.startswith('xmm'):return f's.xmm[{int(register[3:])}]={value};'
        if register.startswith('mm'):return f's.mmx[{int(register[2:])}]={value};'
        if register.startswith('st('):return f's.fp.st({int(register[3:-1])})={value};'
        index,n,shift=REGS[register];return f's.set({index},{value},{n},{shift});'
    def xv(i,op):
        if op.type==3:return f'_mm_set_ss(f({get(i,op)}))' if op.size==4 else f'v(s.memory,{addr(i,op)})'
        return get(i,op)
    def xf(i,op):
        if op.type!=3:return get(i,op)
        if op.size==8:return f'static_cast<long double>(std::bit_cast<double>({get(i,op)}))'
        assert op.size==4,(hex(i.address),'x87 floating load width',op.size)
        return f'ext(f({get(i,op)}))'
    def emit(i):
        op=list(i.operands);mn=i.mnemonic;p=i.address;next=p+i.size
        # Cache prefetch hints have no architectural register/memory effect.
        if mn in ('nop','prefetcht0','prefetchnta'):return ';'
        if mn=='int3':return 'throw std::runtime_error("Original animation debug-break owner reached");'
        if mn=='emms':return 's.fp.count=0;'
        if mn=='stmxcsr':return put(i,op[0],'_mm_getcsr()')
        if mn=='ldmxcsr':return f'_mm_setcsr({get(i,op[0])});'
        if mn=='bsr':return '{const auto x='+get(i,op[1])+';s.flags.zf=!x;if(x){'+put(i,op[0],'31u-std::countl_zero(x)')+'}}'
        if mn in ('cmovg','cmovbe','cmova','cmovle','cmove','cmovne','cmovge'):
            cond={'cmove':'s.flags.zf','cmovne':'!s.flags.zf','cmovg':'!s.flags.zf&&(s.flags.sf==s.flags.of)','cmovbe':'s.flags.cf||s.flags.zf','cmova':'!s.flags.cf&&!s.flags.zf','cmovle':'s.flags.zf||(s.flags.sf!=s.flags.of)','cmovge':'s.flags.sf==s.flags.of'}[mn]
            # X86 loads a memory source even when the condition is false.
            return '{const auto source='+get(i,op[1])+';if('+cond+'){'+put(i,op[0],'source')+'}}'
        if mn=='movhps':
            if op[0].type==3:return f'w(s.memory,s.observe,0x{p:x}u,{addr(i,op[0])},8,high_words({get(i,op[1])}));'
            return put(i,op[0],f'load_high({get(i,op[0])},{get(i,op[1])})')
        if mn=='movlps':
            if op[0].type==3:return f'w(s.memory,s.observe,0x{p:x}u,{addr(i,op[0])},8,std::bit_cast<std::uint64_t>(_mm_cvtsi128_si64(_mm_castps_si128({get(i,op[1])}))));'
            return put(i,op[0],f'_mm_movelh_ps(_mm_castsi128_ps(_mm_cvtsi64_si128(std::bit_cast<std::int64_t>({get(i,op[1])}))),_mm_movehl_ps({get(i,op[0])},{get(i,op[0])}))')
        if mn in ('fsin','fcos'):return 's.fp.st(0)=original_trigonometric(s.fp.st(0),'+str(mn=='fcos').lower()+',s.fp.status);'
        if mn=='fpatan':return 's.fp.st(1)=original_atan(s.fp.st(1),s.fp.st(0));s.fp.pop();'
        if mn=='movq':return put(i,op[0],get(i,op[1]))
        if mn=='pand':return put(i,op[0],f'({get(i,op[0])}&{get(i,op[1])})')
        if mn in ('pcmpgtw','paddw','psubw'):
            mode={'pcmpgtw':0,'paddw':1,'psubw':2}[mn]
            return put(i,op[0],f'packed_words({get(i,op[0])},{get(i,op[1])},{mode})')
        if mn in ('punpcklwd','punpckhwd'):return put(i,op[0],f'unpack_words({get(i,op[0])},{get(i,op[1])},{str(mn=="punpckhwd").lower()})')
        if mn=='psrad':return put(i,op[0],f'shift_dwords({get(i,op[0])},{get(i,op[1])})')
        if mn=='cvtpi2ps':return put(i,op[0],f'convert_dwords({get(i,op[0])},{get(i,op[1])})')
        if mn=='movhlps':return put(i,op[0],f'_mm_movehl_ps({get(i,op[0])},{get(i,op[1])})')
        if mn=='movlhps':return put(i,op[0],f'_mm_movelh_ps({get(i,op[0])},{get(i,op[1])})')
        if mn=='rcpss':return put(i,op[0],f'_mm_move_ss({get(i,op[0])},_mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(source_estimate(s,false,u(_mm_cvtss_f32({xv(i,op[1])})))))))')
        if mn=='rsqrtps':return put(i,op[0],f'estimate_vector(s,{xv(i,op[1])})')
        if mn=='fcompi':return 's.flags.extended_compare(s.fp.st(0),s.fp.st(1));s.fp.pop();'
        if mn=='fucompi':return 's.flags.unordered_extended_compare(s.fp.st(0),s.fp.st(1));s.fp.pop();'

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
        if mn=='cvttss2si':return put(i,op[0],f'std::uint32_t(_mm_cvttss_si32({xv(i,op[1])}))')
        if mn=='cvtsi2ss':return put(i,op[0],f'_mm_cvtsi32_ss({get(i,op[0])},si({get(i,op[1])}))')
        if mn=='int3' and FORWARD_BREAKPOINT:
            return f's.required(0x{i.address:x}u,3u,false);'
        if mn=='rep stosd' or mn=='rep stosb':
            width=4 if mn=='rep stosd' else 1
            return f'while(s.gp[1]){{w(s.memory,s.observe,0x{p:x}u,s.gp[7],{width},s.gp[0]);const auto delta=(s.other_flags&1024u)?{0x100000000-width}u:{width}u;s.gp[7]+=delta;--s.gp[1];if(s.services.instruction)s.services.instruction(0x{p:x}u);}}'
        if mn=='movmskps':return put(i,op[0],f'std::uint32_t(_mm_movemask_ps({get(i,op[1])}))')
        if mn=='sqrtss':return put(i,op[0],f'_mm_move_ss({get(i,op[0])},_mm_sqrt_ss({xv(i,op[1])}))')
        if mn=='rsqrtss':
            return put(i,op[0],f'_mm_move_ss({get(i,op[0])},_mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(source_estimate(s,true,u(_mm_cvtss_f32({xv(i,op[1])})))))))')
        if mn=='shufps':return put(i,op[0],f'_mm_shuffle_ps({get(i,op[0])},{get(i,op[1])},{op[2].imm})')
        if mn in ('add','sub','cmp','test','xor','and','or','sbb','adc'):
            x,y=get(i,op[0]),get(i,op[1]);n=op[0].size
            if mn=='adc':
                value=f'([&](){{const auto mask=Flags::mask({n}),sign=1u<<({n}*8u-1u);const auto a=({x})&mask,b=({y})&mask,carry=unsigned(s.flags.cf);const auto wide=std::uint64_t(a)+b+carry;const auto value=std::uint32_t(wide)&mask;s.flags.cf=wide>mask;s.flags.of=((~(a^b)&(a^value))&sign)!=0;s.flags.af=((a^b^value)&16u)!=0;s.flags.result(value,{n});return value;}}())'
            elif mn=='sbb':value=f's.flags.borrow({x},{y},{n})'
            elif mn in ('add','sub','cmp'):value=f's.flags.{"plus" if mn=="add" else "minus"}({x},{y},{n})'
            else:value=f's.flags.logic(({x}){ {"test":"&","and":"&","xor":"^","or":"|"}[mn]}({y}),{n})'
            return value+';' if mn in ('cmp','test') else put(i,op[0],value)
        if mn in ('inc','dec'):return put(i,op[0],f's.flags.increment({get(i,op[0])},{op[0].size},{str(mn=="dec").lower()})')
        if mn=='shl':return put(i,op[0],f's.flags.shift({get(i,op[0])},{get(i,op[1])},{op[0].size})')
        if mn in ('shr','sar'):return put(i,op[0],f's.flags.right({get(i,op[0])},{get(i,op[1])},{op[0].size},{str(mn=="sar").lower()})')
        if mn=='rcr':
            # Literal unsigned64 divide327290 uses one-bit DWORD rotates.
            # RCR preserves ZF/SF/PF/AF; count1 defines only CF and OF.
            assert op[0].size==4 and op[1].type==2 and op[1].imm==1
            return ('{const auto before='+get(i,op[0])+';const auto value=(before>>1u)|'
                '(s.flags.cf?0x80000000u:0u);'+put(i,op[0],'value')+
                's.flags.cf=(before&1u)!=0;s.flags.of=((value>>31u)^(value>>30u))&1u;}')
        if mn=='neg':return put(i,op[0],f's.flags.minus(0,{get(i,op[0])},{op[0].size})')
        if mn=='imul' and len(op)==1:return f'{{const auto x=std::int64_t(si(s.gp[0]))*si({get(i,op[0])});s.gp[0]=std::uint32_t(x);s.gp[2]=std::uint32_t(std::uint64_t(x)>>32u);s.flags.cf=s.flags.of=x!=std::int64_t(si(s.gp[0]));}}'
        if mn=='mul':
            assert len(op)==1 and op[0].size==4
            return f'{{const auto x=std::uint64_t(s.gp[0])*({get(i,op[0])});s.gp[0]=std::uint32_t(x);s.gp[2]=std::uint32_t(x>>32u);s.flags.cf=s.flags.of=s.gp[2]!=0;}}'
        if mn=='imul':return put(i,op[0],f's.flags.product({get(i,op[-2])},{get(i,op[-1])})')
        if mn in ('div','idiv'):return f's.quotient({get(i,op[0])},{str(mn=="idiv").lower()});'
        if mn=='cdq':return 's.gp[2]=si(s.gp[0])<0?0xffffffffu:0u;'
        if mn=='push':return f's.push(0x{p:x}u,{get(i,op[0])});'
        if mn=='leave':return 's.gp[4]=s.gp[5];s.gp[5]=s.pop();'
        if mn=='pop':return put(i,op[0],'s.pop()')
        if mn=='ret':return f's.pop();s.gp[4]+={op[0].imm if op else 0}u;return;'
        if mn=='rdtsc':return 'if(!s.services.timestamp_counter)throw std::runtime_error("Required original contact RDTSC owner unavailable");{const auto x=s.services.timestamp_counter();s.gp[0]=std::uint32_t(x);s.gp[2]=std::uint32_t(x>>32u);}'
        if mn in ('movsd','movsb') and bytes(i.bytes) in (b'\xa5',b'\xa4'):
            width=4 if mn=='movsd' else 1
            return f'{{const auto x=r(s.memory,s.gp[6],{width});w(s.memory,s.observe,0x{p:x}u,s.gp[7],{width},x);const auto delta=(s.other_flags&1024u)?{0x100000000-width}u:{width}u;s.gp[6]+=delta;s.gp[7]+=delta;}}'
        if mn in ('rep movsd','rep movsb'):
            width=4 if mn=='rep movsd' else 1
            return f'while(s.gp[1]){{const auto x=r(s.memory,s.gp[6],{width});w(s.memory,s.observe,0x{p:x}u,s.gp[7],{width},x);const auto delta=(s.other_flags&1024u)?{0x100000000-width}u:{width}u;s.gp[6]+=delta;s.gp[7]+=delta;--s.gp[1];if(s.services.instruction)s.services.instruction(0x{p:x}u);}}'
        if mn in ('repe cmpsd','repe cmpsw','repe cmpsb'):
            width={'repe cmpsd':4,'repe cmpsw':2,'repe cmpsb':1}[mn]
            return f'while(s.gp[1]){{const auto a=r(s.memory,s.gp[6],{width}),b=r(s.memory,s.gp[7],{width});s.flags.minus(a,b,{width});const auto delta=(s.other_flags&1024u)?{0x100000000-width}u:{width}u;s.gp[6]+=delta;s.gp[7]+=delta;--s.gp[1];if(s.services.instruction)s.services.instruction(0x{p:x}u);if(!s.flags.zf)break;}}'
        if mn in ('comiss','ucomiss'):return f's.flags.scalar_compare({get(i,op[0])},{xv(i,op[1])},{str(mn=="ucomiss").lower()});'
        if mn=='lahf':return 's.set(0,s.flags.bits()|2u,1,8);'
        if mn in ('sete','setne','setb','setle','setl','setg','seta','setge'):
            return put(i,op[0],dict(sete='s.flags.zf',setne='!s.flags.zf',setb='s.flags.cf',setle='s.flags.zf||(s.flags.sf!=s.flags.of)',setl='s.flags.sf!=s.flags.of',setg='!s.flags.zf&&(s.flags.sf==s.flags.of)',seta='!s.flags.cf&&!s.flags.zf',setge='s.flags.sf==s.flags.of')[mn])
        if mn=='not':return put(i,op[0],f'~({get(i,op[0])})')
        if mn=='call':
            used_addresses.add(next)
            if op[0].type==2 and op[0].imm in ENTRIES:
                return f's.push(0x{p:x}u,{imm(next)});'+(f'if(s.animation.resource_delivery){{auto a=s.save();s.animation.resource_delivery(s.memory,{{0x{p:x}u,0x124f30u,false}},a,s.observe);s.load(a);}}else run_124f30(s);' if op[0].imm==0x124f30 else f'run_{op[0].imm:x}(s);')
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
            if op[0].type==1 or (op[0].type==3 and not op[0].mem.index):
                externals[f'{p:08x}']=dict(target=i.op_str,tail=True,full_guest_abi=True)
                return '{const auto target='+get(i,op[0])+f';dispatch(s,target,0x{p:x}u,true);return;'+'}'
            assert op[0].type==3 and op[0].mem.disp in tables
            targets=struct.unpack('<'+'I'*tables[op[0].mem.disp],original.read(op[0].mem.disp,4*tables[op[0].mem.disp]))
            cases=[]
            for x in sorted(set(targets)):
                if x in current_sites:body=f'goto L{x:x};'
                elif x==0:body='throw std::runtime_error("Original source jump-table null target reached");'
                else:body=f'dispatch(s,0x{x:x}u,0x{p:x}u,true);return;'
                cases.append(f'case 0x{x:x}u:'+body)
            return 'switch('+get(i,op[0])+'){'+''.join(cases)+'default:throw std::runtime_error("Invalid original animation action dispatch target");}'
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
        if mn=='fnstsw':return put(i,op[0],'s.fp.word()')
        if mn in ('fcom','fcomp','fcompp','fucompp'):
            value='s.fp.st(1)' if mn in ('fcompp','fucompp') else xf(i,op[0])
            return f's.fp.comparison({value},{ {"fcom":0,"fcomp":1,"fcompp":2,"fucompp":2}[mn]});'
        if mn=='fabs':return 's.fp.st(0)=std::fabs(s.fp.st(0));'
        if mn=='fsqrt':return 's.fp.st(0)=root(s.fp.st(0));'
        if mn in ('fadd','fmul','fsub','fsubr','fdiv','fdivr','fsubp','fsubrp','fmulp','faddp','fdivp'):
            operation='add' if 'add' in mn else 'mul' if 'mul' in mn else 'sub' if 'sub' in mn else 'divide'
            popping=mn.endswith('p');reverse=mn in ('fdivr','fsubr','fsubrp')
            x,y=(get(i,op[0]),'s.fp.st(0)') if popping else ('s.fp.st(0)',xf(i,op[0]))
            if reverse:x,y=y,x
            value=f'{operation}({x},{y})'
            return (put(i,op[0],value)+'s.fp.pop();') if popping else 's.fp.st(0)='+value+';'
        raise AssertionError((hex(p),mn,i.op_str))
    bodies=[]
    for entry in sorted(ENTRIES):
        ins=[code.at(p) for p in sorted(_trace(code,entry,set(ENTRIES)).code)]
        current_sites={i.address for i in ins}
        targets={i.operands[0].imm for i in ins if i.mnemonic.startswith('j') and i.operands[0].type==2}
        for i in ins:
            if i.mnemonic=='jmp'and i.operands[0].type==3 and i.operands[0].mem.index:
                table=i.operands[0].mem.disp;assert table in tables,(hex(entry),hex(i.address),i.op_str)
                targets.update(struct.unpack('<'+'I'*tables[table],original.read(table,4*tables[table])))
        lines=[f'[[gnu::noinline]] static void run_{entry:x}(State &s) {{']
        for i in ins:
            translated=emit(i)
            lowered[f'{i.address:08x}']=dict(mnemonic=i.mnemonic,source_bytes=i.bytes.hex())
            label=f'L{i.address:x}: ' if i.address in targets else ''
            lines.append(f'{label}if(s.services.instruction){{s.services.instruction(0x{i.address:x}u);}} {translated}')
        lines.append('throw std::runtime_error("Original animation fell outside authenticated CFG");\n}')
        bodies.append('\n'.join(lines))
    preamble='''// Generated by tools/generate_unit_animation_selection_runtime_1_1.py.
// Complete authenticated bodies compile to native arithmetic and static calls.
#include "halo2/unit_animation_selection_runtime.hpp"
#include "physics/runtime/physics_contact_runtime_internal.hpp"
namespace halo2 {
namespace {
using namespace physics_contact_runtime_detail;
struct State : physics_contact_runtime_detail::State {
    const SourceUnitAnimationRuntimeServices &animation;
    std::array<std::uint64_t,8> mmx{};
    struct AnimationFlags : Flags {
        void extended_compare(long double a,long double b) {
            const auto status=physics_solver_detail::compare(a,b);
            cf=status&0x100u;pf=status&0x400u;zf=status&0x4000u;
            af=sf=of=false;
        }
        void unordered_extended_compare(long double a,long double b) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
            unsigned char carry,parity,zero;
            asm volatile("fldt %4\\n\\tfldt %3\\n\\tfucomip %%st(1),%%st\\n\\tsetb %0\\n\\tsetp %1\\n\\tsete %2\\n\\tfstp %%st(0)"
                :"=qm"(carry),"=qm"(parity),"=qm"(zero):"m"(a),"m"(b):"st","cc");
            cf=carry;pf=parity;zf=zero;af=sf=of=false;
#else
            throw std::runtime_error("Original unordered x87 flag comparison backend unavailable");
#endif
        }
    } flags;
    State(M &m,const O &o,const SourcePhysicsContactRuntimeServices &shared,
          const SourceUnitAnimationRuntimeServices &owners)
        : physics_contact_runtime_detail::State{m,o,shared,SourcePhysicsContactImage::MapPack11},animation(owners) {}
    void load(const SourceUnitAnimationRuntimeAbi &a) {
        physics_contact_runtime_detail::State::load(a);flags.load(a.eflags);mmx=a.mmx;
    }
    SourceUnitAnimationRuntimeAbi save()const {
        SourceUnitAnimationRuntimeAbi a;
        static_cast<SourcePhysicsContactRuntimeAbi &>(a)=physics_contact_runtime_detail::State::save();
        a.eflags=other_flags|flags.bits();a.mmx=mmx;return a;
    }
    void quotient(std::uint32_t denominator,bool signed_division) {
        physics_solver_detail::State scalar{memory,observe,{},SourcePhysicsContactImage::MapPack11};
        scalar.gp=gp;scalar.quotient(denominator,signed_division);gp=scalar.gp;
    }
    void required(std::uint32_t site,std::uint32_t target,bool tail) {
        if(!animation.required_owner)throw std::runtime_error("Required original animation runtime callee unavailable");
        auto a=save();try {animation.required_owner(memory,{site,target,tail},a,observe);}
        catch(...) {load(a);throw;}load(a);
    }
};
long double original_trigonometric(long double x,bool cosine,std::uint16_t &status) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    long double result;std::uint16_t hardware;
    if(cosine)asm volatile("fldt %2\\n\\tfcos\\n\\tfnstsw %1\\n\\tfstpt %0":"=m"(result),"=m"(hardware):"m"(x):"st");
    else asm volatile("fldt %2\\n\\tfsin\\n\\tfnstsw %1\\n\\tfstpt %0":"=m"(result),"=m"(hardware):"m"(x):"st");
    status=(status&~0x600u)|(hardware&0x600u);return result;
#else
    throw std::runtime_error("Original biped input x87 trigonometric backend unavailable");
#endif
}
long double original_atan(long double y,long double x) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    long double result;
    asm volatile("fldt %1\\n\\tfldt %2\\n\\tfpatan\\n\\tfstpt %0":"=m"(result):"m"(y),"m"(x):"st");
    return result;
#else
    throw std::runtime_error("Original local-pose x87 atan backend unavailable");
#endif
}
std::uint64_t high_words(__m128 input) {
    return std::bit_cast<std::uint64_t>(_mm_cvtsi128_si64(_mm_castps_si128(_mm_movehl_ps(input,input))));
}
__m128 load_high(__m128 input,std::uint64_t words) {
    return _mm_movelh_ps(input,_mm_castsi128_ps(_mm_cvtsi64_si128(std::bit_cast<std::int64_t>(words))));
}
std::uint64_t r64(const M &m,std::uint32_t p) {
    return std::uint64_t(r(m,p))|(std::uint64_t(r(m,p+4))<<32u);
}
std::uint64_t unpack_words(std::uint64_t a,std::uint64_t b,bool high) {
    std::uint64_t result=0;
    for(unsigned i=0;i<2;++i) {
        const auto shift=(i+(high?2u:0u))*16u;
        result|=((a>>shift)&65535u)<<(i*32u);
        result|=((b>>shift)&65535u)<<(i*32u+16u);
    }
    return result;
}
std::uint64_t packed_words(std::uint64_t a,std::uint64_t b,unsigned mode) {
    std::uint64_t result=0;
    for(unsigned i=0;i<4;++i) {
        const auto x=std::uint16_t(a>>(i*16u)),y=std::uint16_t(b>>(i*16u));
        const auto lane=mode==0 ? std::uint16_t(
            std::bit_cast<std::int16_t>(x)>std::bit_cast<std::int16_t>(y)?65535u:0u)
            : mode==1 ? std::uint16_t(unsigned(x)+unsigned(y))
            : std::uint16_t(unsigned(x)-unsigned(y));
        result|=std::uint64_t(lane)<<(i*16u);
    }
    return result;
}
std::uint64_t shift_dwords(std::uint64_t a,unsigned count) {
    count=std::min(count,31u);std::uint64_t result=0;
    for(unsigned i=0;i<2;++i) {
        const auto x=std::bit_cast<std::int32_t>(std::uint32_t(a>>(i*32u)));
        result|=std::uint64_t(std::uint32_t(x>>count))<<(i*32u);
    }
    return result;
}
__m128 convert_dwords(__m128 original,std::uint64_t a) {
    const auto converted=_mm_cvtepi32_ps(_mm_cvtsi64_si128(std::bit_cast<std::int64_t>(a)));
    return _mm_movelh_ps(converted,_mm_movehl_ps(original,original));
}
std::uint32_t source_estimate(State &s,bool square_root,std::uint32_t input) {
    if(!s.animation.estimate)throw std::runtime_error("Required original animation CPU estimate owner unavailable");
    return s.animation.estimate(square_root?SourceUnitAnimationEstimate::ReciprocalSquareRoot:SourceUnitAnimationEstimate::Reciprocal,input);
}
__m128 estimate_vector(State &s,__m128 input) {
    std::array<std::uint32_t,4> bits;
    _mm_storeu_si128(reinterpret_cast<__m128i *>(bits.data()),_mm_castps_si128(input));
    for(auto &x:bits)x=source_estimate(s,true,x);
    return _mm_castsi128_ps(_mm_loadu_si128(reinterpret_cast<const __m128i *>(bits.data())));
}
std::uint32_t sign_extend(std::uint32_t x,unsigned n) {
    if(n==1)return std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(x))));
    if(n==2)return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(x))));
    throw std::runtime_error("Unverified animation sign-extension width");
}
'''
    preamble+='\n'.join(f'static void run_{p:x}(State &);' for p in sorted(ENTRIES))+'\n'
    preamble+='static void dispatch(State &s,std::uint32_t target,std::uint32_t site,bool tail){'+''.join(f'if(target==0x{p:x}u){{run_{p:x}(s);return;}}' for p in sorted(ENTRIES))+'s.required(site,target,tail); }\n'
    ending='''
using AnimationSelectionState=State;
} // namespace
void source_run_unit_animation_selection_1_1(M &m,std::uint32_t entry,
    SourceUnitAnimationRuntimeAbi &abi,FP fp,
    const SourceUnitAnimationRuntimeServices &services,const O &observe) {
    SourcePhysicsContactRuntimeServices retained;
    retained.instruction=services.instruction;

    FpScope scope(fp);AnimationSelectionState s{m,observe,retained,services};s.load(abi);
    try {switch(entry) {
'''+''.join(f'case 0x{p:x}u:run_{p:x}(s);break;\n' for p in sorted(ENTRIES))+'''
        default:throw std::runtime_error("Required original animation selection entry unavailable");
    }}catch(...) {abi=s.save();throw;}
    abi=s.save();
}
} // namespace halo2
'''
    a.output.parent.mkdir(parents=True,exist_ok=True)
    source=preamble+'\n\n'.join(bodies)+ending
    if FORWARD_TIMESTAMP_COUNTER:
        source=source.replace('retained.instruction=services.instruction;',
            'retained.instruction=services.instruction;\n    retained.timestamp_counter=services.timestamp_counter;')
    source=source.replace('halo2/unit_animation_selection_runtime.hpp',RUNTIME_HEADER)
    source=source.replace('source_run_unit_animation_selection_1_1',RUNTIME_FUNCTION)
    if not a.output.exists() or a.output.read_text()!=source:
        a.output.write_text(source)
    report=dict(schema=1,generator_sha256=sha(__file__),original_sha256=PIN,
        generated_cpp_sha256=sha(a.output),complete_native_entries=[f'{p:08x}' for p in sorted(ENTRIES)],
        lowered_instructions=lowered,external_owners=externals,
        production_opcode_decoder=False,production_original_code_execution=False,
        captured_runtime_results_used=False,live_gameplay_proof=False)
    if CONFIGURATION_TOOL:report['configuration_tool_sha256']=sha(CONFIGURATION_TOOL)
    a.manifest.parent.mkdir(parents=True,exist_ok=True)
    a.manifest.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(entries=len(ENTRIES),instructions=len(lowered),external_call_sites=len(externals),
        generated_cpp_sha256=sha(a.output),manifest_sha256=sha(a.manifest))))
if __name__=='__main__':main()
