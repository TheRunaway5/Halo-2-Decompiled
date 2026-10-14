#!/usr/bin/env python3
"""Lower authenticated complete contact CFGs to directly compiled C++ paths.

No binary blob, opcode fetch or production emulator is emitted. Each original
instruction is lowered at generation time to explicit integer/SSE/x87 arithmetic,
memory ownership, a C++ label/branch, or a statically bound native callee.
Unknown operations fail generation; required external owners remain explicit.
"""
import argparse, hashlib, json, os, struct
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT, Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from functions import Function, _descend, _merge_tables
PIN=NEW_PIN


REGS={}
for index,(full,word,lo,hi) in enumerate((('eax','ax','al','ah'),('ecx','cx','cl','ch'),
    ('edx','dx','dl','dh'),('ebx','bx','bl','bh'),('esp','sp',None,None),('ebp','bp',None,None),
    ('esi','si',None,None),('edi','di',None,None))):
    for name,n,shift in ((full,4,0),(word,2,0),(lo,1,0),(hi,1,8)):
        if name:REGS[name]=(index,n,shift)
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def generate(a,paired,process_heap=False,*,complete_listings=False,
             runtime_header=None,runtime_function=None,extra_instruction=None,
             extra_preamble='',instruction_guard=None,extra_instruction_factory=None,
             code_override=None):
    assert paired['image']=='English TU1.5' and paired['original_sha256']==NEW_PIN
    assert sha(NEW_DEFAULT)==NEW_PIN
    complete_listings=True
    ENTRIES=tuple(sorted(int(x,16) for x in paired['entries']))
    # These are explicit native resource boundaries of this composition. Their
    # callers must reach required_owner rather than an uninitialized Xbox CRT
    # exit table or Xbox file handle implementation.
    assert not set(ENTRIES)&{0x32b0b4,0x125870,0x1a3b70}
    old,new=Xbe(NEW_DEFAULT),Xbe(NEW_DEFAULT);code=code_override or _Code(old,'.text')
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
        if op.type==3:
            assert op.size<=4,(hex(i.address),i.mnemonic,op.size)
            return f'r(s.memory,{addr(i,op)},{op.size})'
        assert op.type==1;register=name(i,op)
        if register.startswith('mm'):return f's.mmx({int(register[2:])})'
        if register.startswith('xmm'):return f's.xmm[{int(register[3:])}]'
        if register.startswith('st('):return f's.fp.st({int(register[3:-1])})'
        index,n,shift=REGS[register];return f's.reg({index},{n},{shift})'
    def put(i,op,value):
        if op.type==3:
            assert op.size<=4
            return f'w(s.memory,s.observe,0x{i.address:x}u,{addr(i,op)},{op.size},{value});'
        assert op.type==1;register=name(i,op)
        if register.startswith('mm'):return f's.set_mmx({int(register[2:])},{value});'
        if register.startswith('xmm'):return f's.xmm[{int(register[3:])}]={value};'
        if register.startswith('st('):return f's.fp.st({int(register[3:-1])})={value};'
        index,n,shift=REGS[register];return f's.set({index},{value},{n},{shift});'
    def xv(i,op):
        if op.type!=3:return get(i,op)
        # A scalar SSE source owns four bytes. A vector load here would read
        # unrelated memory and can fault beyond the original operand extent.
        assert op.size in (4,16),(hex(i.address),i.mnemonic,op.size)
        return f'_mm_set_ss(f({get(i,op)}))' if op.size==4 else f'v(s.memory,{addr(i,op)})'
    def xf(i,op):
        if op.type!=3:return get(i,op)
        assert op.size in (4,8,10),(hex(i.address),i.mnemonic,op.size)
        if op.size==10:return f'native_extended_load(s.memory,{addr(i,op)})'
        if op.size==8:
            return f'static_cast<long double>(std::bit_cast<double>(native_read64(s.memory,{addr(i,op)})))'
        return f'ext(f({get(i,op)}))'
    if extra_instruction_factory:
        assert extra_instruction is None
        extra_instruction=extra_instruction_factory(get=get,put=put,addr=addr,xf=xf)
    def emit(i):
        op=list(i.operands);mn=i.mnemonic;p=i.address;next=p+i.size
        if extra_instruction:
            extended=extra_instruction(i)
            if extended is not None:return extended
        if mn=='nop':return ';'
        if mn=='movq':
            assert all(q.size==8 for q in op)
            value=f'native_read64(s.memory,{addr(i,op[1])})' if op[1].type==3 else get(i,op[1])
            if op[0].type==3:return f'w(s.memory,s.observe,0x{p:x}u,{addr(i,op[0])},8,{value});'
            return put(i,op[0],value)
        if mn=='pxor' and op[0].type==1 and name(i,op[0]).startswith('mm'):
            assert all(q.size==8 for q in op)
            # XOR with itself defines all64 bits even when the incoming MMX
            # register has only partially known lanes from its x87 alias.
            if op[1].type==1 and op[0].reg==op[1].reg:
                return put(i,op[0],'std::uint64_t{0}')
            source=f'native_read64(s.memory,{addr(i,op[1])})' if op[1].type==3 else get(i,op[1])
            return put(i,op[0],f'({get(i,op[0])}^{source})')
        if mn=='movd':
            if op[0].type==1 and name(i,op[0]).startswith('mm'):
                assert op[1].size==4
                return put(i,op[0],f'std::uint64_t({get(i,op[1])})')
            assert op[1].type==1 and name(i,op[1]).startswith('mm') and op[0].size==4
            return put(i,op[0],f's.mmx_low_dword({int(name(i,op[1])[2:])})')
        if mn=='punpcklbw':
            assert op[0].type==1 and name(i,op[0]).startswith('mm')
            # This form consumes only the low four source bytes.
            source=(get(i,op[1]) if op[1].type==3 else
                    f's.mmx_low_dword({int(name(i,op[1])[2:])})')
            assert op[1].type!=3 or op[1].size==4
            return put(i,op[0],f'native_unpack_low_bytes(s.mmx_low_dword({int(name(i,op[0])[2:])}),{source})')
        if mn=='cvtps2pi':
            assert op[0].type==1 and name(i,op[0]).startswith('mm') and op[1].type==1
            return put(i,op[0],f'native_convert_float_pair({get(i,op[1])})')
        if mn=='pshufw':
            assert op[2].type==2 and op[1].type==1 and name(i,op[1]).startswith('mm')
            return put(i,op[0],f'native_shuffle_mmx_words({get(i,op[1])},{op[2].imm}u)')
        if mn in ('psubw','pmulhw'):
            assert op[0].type==1 and name(i,op[0]).startswith('mm')
            source=f'native_read64(s.memory,{addr(i,op[1])})' if op[1].type==3 else get(i,op[1])
            return put(i,op[0],f'native_mmx_word_math({get(i,op[0])},{source},{str(mn=="pmulhw").lower()})')
        if mn in ('psllw','psraw'):
            assert op[0].type==1 and name(i,op[0]).startswith('mm') and op[1].type==2
            return put(i,op[0],f'native_shift_mmx_words({get(i,op[0])},{op[1].imm}u,{str(mn=="psraw").lower()})')
        if mn=='packuswb':
            assert op[0].type==1 and name(i,op[0]).startswith('mm')
            source=f'native_read64(s.memory,{addr(i,op[1])})' if op[1].type==3 else get(i,op[1])
            return put(i,op[0],f'native_pack_mmx_unsigned_bytes({get(i,op[0])},{source})')
        if mn in ('pcmpgtw','paddw','pand'):
            assert op[0].type==1 and name(i,op[0]).startswith('mm')
            assert all(q.size==8 for q in op)
            source=(f'native_read64(s.memory,{addr(i,op[1])})'
                    if op[1].type==3 else get(i,op[1]))
            if mn=='pand':return put(i,op[0],f'({get(i,op[0])}&{source})')
            return put(i,op[0],f'native_mmx_words({get(i,op[0])},{source},{str(mn=="pcmpgtw").lower()})')
        if mn in ('punpcklwd','punpckhwd'):
            assert all(q.type==1 and name(i,q).startswith('mm') for q in op)
            return f's.unpack_mmx_words({int(name(i,op[0])[2:])},{int(name(i,op[1])[2:])},{str(mn=="punpckhwd").lower()});'
        if mn=='psrad':
            assert op[0].type==1 and name(i,op[0]).startswith('mm')
            return f's.shift_mmx_dwords({int(name(i,op[0])[2:])},{get(i,op[1])});'
        if mn=='cvtpi2ps':
            return put(i,op[0],f'native_convert_mmx_dwords({get(i,op[0])},{get(i,op[1])})')
        if mn=='emms':return 's.emms();'
        if mn=='stmxcsr':return put(i,op[0],'_mm_getcsr()')
        if mn=='ldmxcsr':return f'_mm_setcsr({get(i,op[0])});'
        if mn=='wait':return 'asm volatile("fwait");'
        if mn=='sahf':return '{const auto ah=s.gp[0]>>8u;s.flags.cf=ah&1u;s.flags.pf=ah&4u;s.flags.af=ah&16u;s.flags.zf=ah&64u;s.flags.sf=ah&128u;}'
        if mn=='fnstcw':return put(i,op[0],'native_control_word()')
        if mn=='fldcw':return f'native_load_control_word(std::uint16_t({get(i,op[0])}));'
        if mn in ('prefetchnta','prefetcht0'):return ';' # Cache hints; no architectural load or fault.
        if mn=='bsf':
            return '{const auto value='+get(i,op[1])+';s.flags.zf=!value;if(value){'+put(i,op[0],'std::countr_zero(value)')+'}}'
        if mn=='bsr':
            return '{const auto value='+get(i,op[1])+';s.flags.zf=!value;if(value){'+put(i,op[0],'31u-std::countl_zero(value)')+'}}'
        if mn=='adc':
            n=op[0].size
            return ('{const auto a='+get(i,op[0])+',b='+get(i,op[1])+f',mask=Flags::mask({n});'
                    'const auto full=std::uint64_t(a&mask)+(b&mask)+unsigned(s.flags.cf);'
                    'const auto x=std::uint32_t(full)&mask;s.flags.cf=full>mask;'
                    f's.flags.of=((~(a^b)&(a^x))&(1u<<({n}*8u-1u)))!=0;'
                    f's.flags.af=((a^b^x)&16u)!=0;s.flags.result(x,{n});'+put(i,op[0],'x')+'}')
        if mn=='lea':return put(i,op[0],addr(i,op[1]))
        if mn in ('mov','movzx','movsx'):
            value=get(i,op[1])
            if mn=='movsx':value=f'sign_extend({value},{op[1].size})'
            return put(i,op[0],value)
        if mn=='movaps':
            if op[0].type==3:return f'vw(s.memory,s.observe,0x{p:x}u,{addr(i,op[0])},{xv(i,op[1])});'
            return put(i,op[0],xv(i,op[1]))
        if mn in ('movlps','movhps'):
            offset=0 if mn=='movlps' else 8
            if op[0].type==3:
                return f'native_store_half(s,0x{p:x}u,{addr(i,op[0])},{get(i,op[1])},{offset});'
            assert op[1].type==3 and op[1].size==8
            return put(i,op[0],f'native_load_half(s.memory,{addr(i,op[1])},{get(i,op[0])},{offset})')
        if mn=='movss':
            if op[0].type==3:return f'w(s.memory,s.observe,0x{p:x}u,{addr(i,op[0])},4,u(_mm_cvtss_f32({get(i,op[1])})));'
            if op[1].type==3:return put(i,op[0],f'_mm_set_ss(f({get(i,op[1])}))')
            return put(i,op[0],f'_mm_move_ss({get(i,op[0])},{get(i,op[1])})')
        if mn in ('movhlps','movlhps'):
            intrinsic='_mm_movehl_ps' if mn=='movhlps' else '_mm_movelh_ps'
            return put(i,op[0],f'{intrinsic}({get(i,op[0])},{get(i,op[1])})')
        if mn=='cmpneqss':
            return put(i,op[0],f'_mm_cmpneq_ss({get(i,op[0])},{xv(i,op[1])})')
        if mn=='cmpltss':
            return put(i,op[0],f'_mm_cmplt_ss({get(i,op[0])},{xv(i,op[1])})')
        if mn in ('addps','subps','mulps','addss','subss','mulss','xorps','andps','maxps','minps','cmpltps','cmpleps','divss'):
            intrinsic={'addps':'_mm_add_ps','subps':'_mm_sub_ps','mulps':'_mm_mul_ps','addss':'_mm_add_ss','subss':'_mm_sub_ss','mulss':'_mm_mul_ss','xorps':'_mm_xor_ps','andps':'_mm_and_ps','maxps':'_mm_max_ps','minps':'_mm_min_ps','cmpltps':'_mm_cmplt_ps','cmpleps':'_mm_cmple_ps','divss':'_mm_div_ss'}[mn]
            return put(i,op[0],f'{intrinsic}({get(i,op[0])},{xv(i,op[1])})')
        if mn=='cvtsi2ss':return put(i,op[0],f'_mm_cvtsi32_ss({get(i,op[0])},si({get(i,op[1])}))')
        if mn=='cvttss2si':return put(i,op[0],f'std::uint32_t(_mm_cvttss_si32({xv(i,op[1])}))')
        if mn=='movmskps':return put(i,op[0],f'std::uint32_t(_mm_movemask_ps({get(i,op[1])}))')
        if mn=='sqrtss':return put(i,op[0],f'_mm_move_ss({get(i,op[0])},_mm_sqrt_ss({xv(i,op[1])}))')
        if mn=='rsqrtss':
            return put(i,op[0],f'_mm_move_ss({get(i,op[0])},_mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(source_estimate(s,u(_mm_cvtss_f32({xv(i,op[1])})))))))')
        if mn=='rsqrtps':return put(i,op[0],f'native_estimate_vector(s,{xv(i,op[1])})')
        if mn=='rcpss':
            # Explicit native CPU estimate policy, like the retained owner's
            # RSQRTSS binding. Xbox estimate-bit equivalence is not inferred.
            return put(i,op[0],f'_mm_move_ss({get(i,op[0])},_mm_rcp_ss({xv(i,op[1])}))')
        if mn=='shufps':return put(i,op[0],f'_mm_shuffle_ps({get(i,op[0])},{get(i,op[1])},{op[2].imm})')
        if mn in ('add','sub','cmp','test','xor','and','or','sbb'):
            x,y=get(i,op[0]),get(i,op[1]);n=op[0].size
            if mn=='sbb':value=f's.flags.borrow({x},{y},{n})'
            elif mn in ('add','sub','cmp'):value=f's.flags.{"plus" if mn=="add" else "minus"}({x},{y},{n})'
            else:value=f's.flags.logic(({x}){ {"test":"&","and":"&","xor":"^","or":"|"}[mn]}({y}),{n})'
            return value+';' if mn in ('cmp','test') else put(i,op[0],value)
        if mn in ('inc','dec'):return put(i,op[0],f's.flags.increment({get(i,op[0])},{op[0].size},{str(mn=="dec").lower()})')
        if mn=='shl':return put(i,op[0],f's.flags.shift({get(i,op[0])},{get(i,op[1])},{op[0].size})')
        if mn in ('shld','shrd'):
            assert op[0].size==op[1].size==4
            left=mn=='shld'
            value='(a<<count)|(b>>(32u-count))' if left else '(a>>count)|(b<<(32u-count))'
            carry='(a>>(32u-count))&1u' if left else '(a>>(count-1u))&1u'
            overflow='((x>>31u)!=s.flags.cf)' if left else '((a^x)>>31u)!=0'
            return ('{const auto a='+get(i,op[0])+',b='+get(i,op[1])+';const auto count=('+get(i,op[2])+')&31u;'
                'if(count){const auto x='+value+';s.flags.cf='+carry+';if(count==1)s.flags.of='+overflow+';'
                's.flags.af=false;s.flags.result(x,4);'+put(i,op[0],'x')+'}}')
        if mn in ('shr','sar'):return put(i,op[0],f's.flags.right({get(i,op[0])},{get(i,op[1])},{op[0].size},{str(mn=="sar").lower()})')
        if mn=='rcr':
            width=op[0].size*8
            return ('{auto x='+get(i,op[0])+f';auto count=({get(i,op[1])}&31u)%{width+1}u;'
                f'if(count){{const auto original_count=count;while(count--){{const auto carry=x&1u;'
                f'x=(x>>1u)|(std::uint32_t(s.flags.cf)<<{width-1}u);s.flags.cf=carry;}}'
                f'if(original_count==1)s.flags.of=((x>>{width-1}u)^(x>>{width-2}u))&1u;'
                +put(i,op[0],'x')+'}}')
        if mn=='neg':return put(i,op[0],f's.flags.minus(0,{get(i,op[0])},{op[0].size})')
        if mn=='not':return put(i,op[0],f'~({get(i,op[0])})')
        if mn=='mul':
            assert op[0].size==4
            return '{const auto x=std::uint64_t(s.gp[0])*'+get(i,op[0])+';s.gp[0]=std::uint32_t(x);s.gp[2]=std::uint32_t(x>>32u);s.flags.cf=s.flags.of=s.gp[2]!=0;}'
        if mn=='imul' and len(op)==1:return f'{{const auto x=std::int64_t(si(s.gp[0]))*si({get(i,op[0])});s.gp[0]=std::uint32_t(x);s.gp[2]=std::uint32_t(std::uint64_t(x)>>32u);s.flags.cf=s.flags.of=x!=std::int64_t(si(s.gp[0]));}}'
        if mn=='imul':return put(i,op[0],f's.flags.product({get(i,op[-2])},{get(i,op[-1])})')
        if mn in ('div','idiv'):return f's.quotient({get(i,op[0])},{str(mn=="idiv").lower()});'
        if mn=='cdq':return 's.gp[2]=si(s.gp[0])<0?0xffffffffu:0u;'
        if mn=='push':return f's.push(0x{p:x}u,{get(i,op[0])});'
        if mn=='pop':return put(i,op[0],'s.pop()')
        if mn=='leave':return 's.gp[4]=s.gp[5];s.gp[5]=s.pop();'
        if mn=='std':return 's.other_flags|=1024u;'
        if mn=='cld':return 's.other_flags&=~1024u;'
        if mn=='ret':return f's.pop();s.gp[4]+={op[0].imm if op else 0}u;return;'
        if mn=='rdtsc':return 'if(!s.services.timestamp_counter)throw std::runtime_error("Required original contact RDTSC owner unavailable");{const auto x=s.services.timestamp_counter();s.gp[0]=std::uint32_t(x);s.gp[2]=std::uint32_t(x>>32u);}'
        if mn in ('rep stosd','rep stosb'):
            width=4 if mn=='rep stosd' else 1
            return f'while(s.gp[1]){{w(s.memory,s.observe,0x{p:x}u,s.gp[7],{width},s.gp[0]);const auto delta=(s.other_flags&1024u)?{0x100000000-width}u:{width}u;s.gp[7]+=delta;--s.gp[1];if(s.services.instruction)s.services.instruction(0x{p:x}u);}}'
        if mn in ('stosd','stosw','stosb'):
            width={'stosd':4,'stosw':2,'stosb':1}[mn]
            return f'{{w(s.memory,s.observe,0x{p:x}u,s.gp[7],{width},s.gp[0]);const auto delta=(s.other_flags&1024u)?{0x100000000-width}u:{width}u;s.gp[7]+=delta;}}'
        if mn in ('rep movsd','rep movsb'):
            width=4 if mn=='rep movsd' else 1
            return f'while(s.gp[1]){{const auto x=r(s.memory,s.gp[6],{width});w(s.memory,s.observe,0x{p:x}u,s.gp[7],{width},x);const auto delta=(s.other_flags&1024u)?{0x100000000-width}u:{width}u;s.gp[6]+=delta;s.gp[7]+=delta;--s.gp[1];if(s.services.instruction)s.services.instruction(0x{p:x}u);}}'
        if mn in ('movsd','movsw','movsb'):
            assert len(op)==2 and all(v.type==3 for v in op)
            width={'movsd':4,'movsw':2,'movsb':1}[mn]
            return f'{{const auto x=r(s.memory,s.gp[6],{width});w(s.memory,s.observe,0x{p:x}u,s.gp[7],{width},x);const auto delta=(s.other_flags&1024u)?{0x100000000-width}u:{width}u;s.gp[6]+=delta;s.gp[7]+=delta;}}'
        if mn in ('repe cmpsd','repe cmpsw','repe cmpsb'):
            width={'repe cmpsd':4,'repe cmpsw':2,'repe cmpsb':1}[mn]
            return f'while(s.gp[1]){{s.flags.minus(r(s.memory,s.gp[6],{width}),r(s.memory,s.gp[7],{width}),{width});const auto delta=(s.other_flags&1024u)?{0x100000000-width}u:{width}u;s.gp[6]+=delta;s.gp[7]+=delta;--s.gp[1];if(!s.flags.zf)break;if(s.services.instruction)s.services.instruction(0x{p:x}u);}}'
        if mn in ('comiss','ucomiss'):return f's.flags.scalar_compare({get(i,op[0])},{xv(i,op[1])},{str(mn=="ucomiss").lower()});'
        if mn=='lahf':return 's.set(0,s.flags.bits()|2u,1,8);'
        if mn.startswith('set'):
            condition={'e':'s.flags.zf','ne':'!s.flags.zf',
                'g':'!s.flags.zf&&(s.flags.sf==s.flags.of)',
                'ge':'s.flags.sf==s.flags.of','l':'s.flags.sf!=s.flags.of',
                'le':'s.flags.zf||(s.flags.sf!=s.flags.of)',
                'a':'!s.flags.cf&&!s.flags.zf','ae':'!s.flags.cf',
                'b':'s.flags.cf','be':'s.flags.cf||s.flags.zf',
                's':'s.flags.sf','ns':'!s.flags.sf','o':'s.flags.of',
                'no':'!s.flags.of','p':'s.flags.pf','np':'!s.flags.pf'}[mn[3:]]
            return put(i,op[0],condition)
        if mn.startswith('cmov'):
            condition={'e':'s.flags.zf','ne':'!s.flags.zf',
                'g':'!s.flags.zf&&(s.flags.sf==s.flags.of)',
                'ge':'s.flags.sf==s.flags.of','l':'s.flags.sf!=s.flags.of',
                'le':'s.flags.zf||(s.flags.sf!=s.flags.of)',
                'a':'!s.flags.cf&&!s.flags.zf','ae':'!s.flags.cf',
                'b':'s.flags.cf','be':'s.flags.cf||s.flags.zf',
                's':'s.flags.sf','ns':'!s.flags.sf','o':'s.flags.of',
                'no':'!s.flags.of','p':'s.flags.pf','np':'!s.flags.pf'}[mn[4:]]
            # CMOV reads the source even when the condition is false.
            return '{const auto x='+get(i,op[1])+';if('+condition+'){'+put(i,op[0],'x')+'}}'
        if mn=='call':
            used_addresses.add(next)
            if p==0x1d94ce:
                # Keep the actual point-impulse ABI observable after this
                # virtual target becomes internal. The live owner executes
                # the complete selected callee and can retain read-only
                # pre/post motion bytes for original-code comparisons.
                assert i.bytes.hex()=='ff5060'
                target=get(i,op[0]);externals[f'{p:08x}']=dict(target=i.op_str,full_guest_abi=True)
                return '{const auto target='+target+f';s.push(0x{p:x}u,{imm(next)});s.required(0x{p:x}u,target,false);'+'}'
            if p in (0xd1fbf,0x101db8,0x183e17):
                # Preserve native publication even when the complete callee
                # becomes internal to this composition. The required owner
                # observes the real ABI and runs the original callee itself.
                expected={0xd1fbf:0xd13d0,0x101db8:0x178e30,0x183e17:0x439c0}[p]
                assert op[0].type==2 and op[0].imm==expected
                target=get(i,op[0]);externals[f'{p:08x}']=dict(target=i.op_str,full_guest_abi=True)
                return f's.push(0x{p:x}u,{imm(next)});s.required(0x{p:x}u,{target},false);'
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
            target=op[0].imm
            if target in current_sites:return f'if({condition})goto L{target:x};'
            if target in ENTRIES:return f'if({condition}){{run_{target:x}(s);return;}}'
            externals[f'{p:08x}']=dict(target=i.op_str,tail=True,conditional=True,full_guest_abi=True)
            return f'if({condition}){{dispatch(s,{imm(target)},0x{p:x}u,true);return;}}'
        if mn=='jmp':
            assert op[0].type==3
            table=op[0].mem.disp
            if table not in tables:return f'dispatch(s,{get(i,op[0])},0x{p:x}u,true);return;'
            targets=table_targets(table,old)
            new_targets=table_targets(table,new,True)
            for x,y in zip(targets,new_targets):assert (x==y==0) or instruction_map[x]==y
            branches.update({x:instruction_map[x] for x in targets if x})
            if 0 in targets:branches[0]=0
            return 'switch(canonical_branch('+get(i,op[0])+',s.image)){'+''.join(f'case 0x{x:x}u:goto L{x:x};' if x else f'case 0:s.required(0x{p:x}u,0u,true);return;' for x in sorted(set(targets)))+'default:throw std::runtime_error("Required original solver dispatch target unavailable");}'
        if mn=='fistp':
            if op[0].size==8:
                assert op[0].type==3
                return f'w(s.memory,s.observe,0x{p:x}u,{addr(i,op[0])},8,native_integer_spill64(s.fp.st(0)));s.fp.pop();'
            return put(i,op[0],'integer_spill(s.fp.st(0))')+'s.fp.pop();'
        if mn=='fild':
            if op[0].size==8:
                assert op[0].type==3
                return f's.fp.push(static_cast<long double>(std::bit_cast<std::int64_t>(native_read64(s.memory,{addr(i,op[0])}))));'
            return f's.fp.push(static_cast<long double>(si({get(i,op[0])})));'
        if mn=='fimul':
            assert len(op)==1 and op[0].type==3 and op[0].size==4
            return ('{std::uint16_t status;s.fp.st(0)=native_integer_multiply(s.fp.st(0),'
                f'si({get(i,op[0])}),status);s.fp.status=(s.fp.status&~0x200u)|(status&0x23fu);}}')
        if mn=='fchs':return 's.fp.st(0)=-s.fp.st(0);'
        if mn in ('fldz','fld1'):return f's.fp.push({"0.L" if mn=="fldz" else "1.L"});'
        if mn=='fld':return f's.fp.push({xf(i,op[0])});'
        if mn in ('fst','fstp'):
            value='s.fp.st(0)'
            if op[0].type==3 and op[0].size==8:
                return f'w(s.memory,s.observe,0x{p:x}u,{addr(i,op[0])},8,native_double_spill({value}));'+('s.fp.pop();' if mn=='fstp' else '')
            if op[0].type==3:value=f'u(spill({value}))'
            return put(i,op[0],value)+('s.fp.pop();' if mn=='fstp' else '')
        if mn=='fxch':
            # Capstone includes implicit ST(0) before the encoded ST(i).
            assert len(op)==2 and name(i,op[0])=='st(0)'
            return f'std::swap(s.fp.st(0),{get(i,op[1])});'
        if mn=='fcompi':return f's.compare_and_pop({get(i,op[0])});'
        if mn=='fucompi':return f's.unordered_compare_and_pop({get(i,op[0])});'
        if mn=='fldpi':return 's.fp.push(native_fldpi());s.fp.status&=~0x200u;'
        if mn=='fldl2e':return 's.fp.push(native_fldl2e());s.fp.status&=~0x200u;'
        if mn=='fnstsw':return put(i,op[0],'s.fp.word()')
        if mn in ('fcom','fcomp','fcompp','fucompp'):
            value='s.fp.st(1)' if mn in ('fcompp','fucompp') else xf(i,op[0])
            return f's.fp.comparison({value},{ {"fcom":0,"fcomp":1,"fcompp":2,"fucompp":2}[mn]});'
        if mn=='fabs':return 's.fp.st(0)=std::fabs(s.fp.st(0));'
        if mn=='fsqrt':return 's.fp.st(0)=root(s.fp.st(0));'
        if mn=='fpatan':return 's.fp.st(1)=native_fpatan(s.fp.st(1),s.fp.st(0));s.fp.pop();'
        if mn=='fptan':
            return '{std::uint16_t status;s.fp.st(0)=native_fptan(s.fp.st(0),status);if(!(status&0x400u))s.fp.push(1.L);s.fp.status=(s.fp.status&~0x600u)|(status&0x600u);}'
        if mn=='fyl2x':return '{std::uint16_t status;s.fp.st(1)=native_fyl2x(s.fp.st(1),s.fp.st(0),status);s.fp.pop();s.fp.status=(s.fp.status&~0x200u)|(status&0x200u);}'
        if mn=='f2xm1':return '{std::uint16_t status;s.fp.st(0)=native_f2xm1(s.fp.st(0),status);s.fp.status=(s.fp.status&~0x200u)|(status&0x200u);}'
        if mn=='fscale':return '{std::uint16_t status;s.fp.st(0)=native_fscale(s.fp.st(0),s.fp.st(1),status);s.fp.status=(s.fp.status&~0x200u)|(status&0x200u);}'
        if mn in ('fnsave','frstor'):
            # The ABI retains live logical registers, TOP, SW and CW, but not
            # FIP/FDP or dead physical register bytes. Never serialize a host
            # environment as if it were that missing original CPU state.
            # Keep this path explicitly unavailable until that owner exists.
            return f'throw std::runtime_error("Required original TU1.5 {mn.upper()} full x87 environment owner unavailable at {p:08x}");'
        if mn=='frndint':return '{std::uint16_t status;s.fp.st(0)=native_frndint(s.fp.st(0),status);s.fp.status=(s.fp.status&~0x200u)|(status&0x200u);}'
        if mn in ('fsin','fcos'):
            return '{std::uint16_t status;s.fp.st(0)=native_'+mn+'(s.fp.st(0),status);s.fp.status=(s.fp.status&~0x600u)|(status&0x600u);}'
        if mn in ('fadd','fmul','fsub','fsubr','fdiv','fdivr','fdivp','fdivrp','fsubp','fsubrp','fmulp','faddp'):
            operation='add' if 'add' in mn else 'mul' if 'mul' in mn else 'sub' if 'sub' in mn else 'divide'
            popping=mn.endswith('p');reverse=mn in ('fdivr','fdivrp','fsubr','fsubrp')
            if not popping and len(op)==2:
                # DC register forms write ST(i), preserving ST(0). Source
                #134A08/134A0A multiply separate packed-color lanes this way.
                assert all(q.type==1 and name(i,q).startswith('st(') for q in op)
                x,y=get(i,op[0]),get(i,op[1])
                if reverse:x,y=y,x
                return put(i,op[0],f'{operation}({x},{y})')
            x,y=(get(i,op[0]),'s.fp.st(0)') if popping else ('s.fp.st(0)',xf(i,op[0]))
            if reverse:x,y=y,x
            value=f'{operation}({x},{y})'
            return (put(i,op[0],value)+'s.fp.pop();') if popping else 's.fp.st(0)='+value+';'
        raise AssertionError((hex(p),mn,i.op_str))
    tables={q['disc']:q for row in paired['routines'] for q in row['tables']}
    def table_targets(table,image,later=False):
        row=tables[table]
        indices=row.get('indices',list(range(row.get('count',0))))
        base=addresses[table] if later else table
        return tuple(struct.unpack('<I',image.read(base+index*4,4))[0] for index in indices)
    bodies=[]
    for entry in ENTRIES:
        sites=paired['body_sites'][f'{entry:08x}'] if complete_listings else _trace(code,entry,set(ENTRIES)).code
        ins=[code.at(p) for p in sorted(sites)]
        current_sites={i.address for i in ins}
        targets={i.operands[0].imm for i in ins if i.mnemonic.startswith('j') and i.operands[0].type==2}
        for i in ins:
            if i.mnemonic=='jmp' and i.operands[0].type==3:
                table=i.operands[0].mem.disp
                if table not in tables:continue
                targets.update(table_targets(table,old))
        lines=[f'static void run_{entry:x}(WorldState &s) {{']
        for i in ins:
            translated=emit(i);lowered[f'{i.address:08x}']=dict(mnemonic=i.mnemonic,source_bytes=i.bytes.hex())
            label=f'L{i.address:x}: ' if i.address in targets else ''
            guard=instruction_guard(i) if instruction_guard else ''
            lines.append(f'{label}{guard}if(s.services.instruction){{s.services.instruction(0x{i.address:x}u);}} {translated}')
        lines.append('throw std::runtime_error("Original solver fell outside authenticated CFG");\n}')
        bodies.append('\n'.join(lines))
    used_addresses.update(ENTRIES)
    assert used_addresses<=set(addresses),(used_addresses-set(addresses))
    addresses.update({int(k,16):v for k,v in paired['entries'].items()})
    used_addresses.update(ENTRIES)
    assert used_addresses<=set(addresses),(used_addresses-set(addresses))
    preamble='''// Generated by tools/generate_physics_live_support_tu15.py.
// Complete authenticated bodies compile to native arithmetic and static calls.
#include "physics_world_runtime_internal.hpp"
#include "halo2/physics_live_support_tu15.hpp"
namespace halo2 {
namespace {
using namespace physics_world_runtime_detail;
// Source296180 interleaves unknown dead MMX words, then PSRAD16 discards
// every unknown bit before conversion. Track those bits without inventing a
// dead physical register value or requiring an irrelevant captured register.
struct LiveState : physics_world_runtime_detail::WorldState {
    using physics_world_runtime_detail::WorldState::WorldState;
    void unordered_compare_and_pop(long double other) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        const auto value=fp.st(0);unsigned char carry,zero,parity;
        // FUCOMIP retains extended operands and its unordered-NaN semantics.
        asm volatile("fldt %4\\n\\tfldt %3\\n\\tfucomip %%st(1),%%st\\n\\tsetb %0\\n\\tsete %1\\n\\tsetp %2\\n\\tfstp %%st(0)"
            :"=qm"(carry),"=qm"(zero),"=qm"(parity):"m"(value),"m"(other):"cc","st");
        flags.cf=carry;flags.zf=zero;flags.pf=parity;flags.af=flags.sf=flags.of=false;
        fp.status&=~0x200u;fp.pop();
#else
        throw std::runtime_error("Original world unordered comparison backend unavailable");
#endif
    }
    std::array<std::uint64_t,8> partial_known{};
    std::uint8_t partial_valid{};
    std::pair<std::uint64_t,std::uint64_t> partial(unsigned index) {
        if(!active_mmx)partial_valid=0;
        begin_mmx();std::uint64_t value=0;
        for(unsigned i=0;i<8;++i)value|=std::uint64_t(std::to_integer<unsigned>(active_mmx->physical[index][i]))<<(i*8u);
        const auto known=(active_mmx->known_mask&(1u<<index))?~std::uint64_t{}:
            (partial_valid&(1u<<index))?partial_known[index]:0u;
        return {value,known};
    }
    void set_partial(unsigned index,std::uint64_t value,std::uint64_t known) {
        physics_world_runtime_detail::WorldState::set_mmx(index,value);
        partial_valid|=std::uint8_t(1u<<index);partial_known[index]=known;
        if(known!=~std::uint64_t{})active_mmx->known_mask&=std::uint8_t(~(1u<<index));
    }
    void set_mmx(unsigned index,std::uint64_t value) {set_partial(index,value,~std::uint64_t{});}
    std::uint32_t mmx_low_dword(unsigned index) {
        const auto [value,known]=partial(index);
        if(std::uint32_t(known)!=0xffffffffu)
            throw std::runtime_error("Required original MMX low dword unavailable");
        return std::uint32_t(value);
    }
    void unpack_mmx_words(unsigned destination,unsigned source,bool high) {
        const auto [a,ak]=partial(destination);const auto [b,bk]=partial(source);
        std::uint64_t value=0,known=0;
        for(unsigned i=0;i<2;++i) {
            const auto in=(i+(high?2u:0u))*16u,out=i*32u;
            value|=((a>>in)&65535u)<<out;value|=((b>>in)&65535u)<<(out+16u);
            known|=((ak>>in)&65535u)<<out;known|=((bk>>in)&65535u)<<(out+16u);
        }
        set_partial(destination,value,known);
    }
    void shift_mmx_dwords(unsigned destination,unsigned count) {
        const auto [a,ak]=partial(destination);std::uint64_t value=0,known=0;
        const auto shift=std::min(count,31u);
        for(unsigned i=0;i<2;++i) {
            const auto bits=std::uint32_t(a>>(i*32u)),mask=std::uint32_t(ak>>(i*32u));
            const auto result=std::uint32_t(std::bit_cast<std::int32_t>(bits)>>shift);
            auto result_mask=mask>>shift;
            if(shift && (mask&0x80000000u))result_mask|=0xffffffffu<<(32u-shift);
            value|=std::uint64_t(result)<<(i*32u);known|=std::uint64_t(result_mask)<<(i*32u);
        }
        set_partial(destination,value,known);
    }
    void emms() {physics_world_runtime_detail::WorldState::emms();partial_valid=0;}
};
using WorldState=LiveState;
long double native_fldpi() {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    long double value;asm volatile("fldpi":"=t"(value));return value;
#else
    throw std::runtime_error("Original x87 pi constant backend unavailable");
#endif
}
long double native_fldl2e() {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    long double value;asm volatile("fldl2e":"=t"(value));return value;
#else
    throw std::runtime_error("Required original x87 FLDL2E native implementation unavailable");
#endif
}
long double native_extended_load(M &memory,std::uint32_t address) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    std::array<std::byte,10> bytes;memory.read(address,bytes);
    long double value;asm volatile("fldt %1":"=t"(value):"m"(bytes));return value;
#else
    throw std::runtime_error("Original extended x87 memory load backend unavailable");
#endif
}
std::uint64_t native_mmx_words(std::uint64_t a,std::uint64_t b,bool compare) {
    std::uint64_t result=0;
    for(unsigned lane=0;lane<4;++lane) {
        const auto x=std::uint16_t(a>>(lane*16u)),y=std::uint16_t(b>>(lane*16u));
        const auto word=compare?
            std::uint16_t(std::bit_cast<std::int16_t>(x)>std::bit_cast<std::int16_t>(y)?65535u:0u):
            std::uint16_t(unsigned(x)+unsigned(y));
        result|=std::uint64_t(word)<<(lane*16u);
    }
    return result;
}
std::uint64_t native_unpack_low_bytes(std::uint32_t a,std::uint32_t b) {
    std::uint64_t result{};
    for(unsigned lane=0;lane<4;++lane)
        result|=std::uint64_t(((a>>(lane*8u))&255u)|(((b>>(lane*8u))&255u)<<8u))<<(lane*16u);
    return result;
}
std::uint64_t native_convert_float_pair(__m128 value) {
    // CVTPS2PI consumes two floats. Zero the other two before using SSE2 so
    // their unconsumed bits cannot produce extra MXCSR exceptions.
    const auto converted=_mm_cvtps_epi32(_mm_movelh_ps(value,_mm_setzero_ps()));
    const auto words=std::bit_cast<std::array<std::uint32_t,4>>(converted);
    return std::uint64_t(words[0])|(std::uint64_t(words[1])<<32u);
}
std::uint64_t native_shuffle_mmx_words(std::uint64_t value,unsigned order) {
    std::uint64_t result{};
    for(unsigned lane=0;lane<4;++lane)
        result|=((value>>(((order>>(lane*2u))&3u)*16u))&65535u)<<(lane*16u);
    return result;
}
std::uint64_t native_mmx_word_math(std::uint64_t a,std::uint64_t b,bool high_product) {
    std::uint64_t result{};
    for(unsigned lane=0;lane<4;++lane) {
        const auto x=std::uint16_t(a>>(lane*16u)),y=std::uint16_t(b>>(lane*16u));
        const auto value=high_product?
            std::uint16_t(std::bit_cast<std::uint32_t>(
                std::int32_t(std::bit_cast<std::int16_t>(x))*std::int32_t(std::bit_cast<std::int16_t>(y)))>>16u):
            std::uint16_t(unsigned(x)-unsigned(y));
        result|=std::uint64_t(value)<<(lane*16u);
    }
    return result;
}
std::uint64_t native_shift_mmx_words(std::uint64_t value,unsigned count,bool arithmetic) {
    std::uint64_t result{};
    for(unsigned lane=0;lane<4;++lane) {
        const auto word=std::uint16_t(value>>(lane*16u));
        const auto shifted=arithmetic?std::uint16_t(std::int32_t(std::bit_cast<std::int16_t>(word))>>std::min(count,15u)):
            count>=16u?std::uint16_t(0):std::uint16_t(unsigned(word)<<count);
        result|=std::uint64_t(shifted)<<(lane*16u);
    }
    return result;
}
std::uint64_t native_pack_mmx_unsigned_bytes(std::uint64_t a,std::uint64_t b) {
    std::uint64_t result{};
    for(unsigned lane=0;lane<8;++lane) {
        const auto word=std::bit_cast<std::int16_t>(std::uint16_t((lane<4?a:b)>>((lane&3u)*16u)));
        result|=std::uint64_t(std::clamp(int(word),0,255))<<(lane*8u);
    }
    return result;
}
__m128 native_convert_mmx_dwords(__m128 previous,std::uint64_t bits) {
    const auto converted=_mm_cvtepi32_ps(_mm_set_epi32(0,0,
        std::bit_cast<std::int32_t>(std::uint32_t(bits>>32u)),
        std::bit_cast<std::int32_t>(std::uint32_t(bits))));
    return _mm_movelh_ps(converted,_mm_movehl_ps(previous,previous));
}
std::uint64_t native_read64(const M &memory,std::uint32_t address) {
    std::array<std::byte,8> bytes;memory.read(address,bytes);std::uint64_t result=0;
    for(unsigned i=0;i<8;++i)result|=std::uint64_t(std::to_integer<unsigned>(bytes[i]))<<(i*8u);
    return result;
}
std::uint64_t native_integer_spill64(long double x) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    std::int64_t result;
    asm volatile("fistpq %0":"=m"(result):"t"(x):"st");
    return std::bit_cast<std::uint64_t>(result);
#else
    throw std::runtime_error("Required TU1.5 FISTP64 backend unavailable");
#endif
}
std::uint64_t native_double_spill(long double x) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    double result;
    asm volatile("fstpl %0":"=m"(result):"t"(x):"st");
    return std::bit_cast<std::uint64_t>(result);
#else
    throw std::runtime_error("Required TU1.5 FSTP64 backend unavailable");
#endif
}
std::uint16_t native_control_word() {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    std::uint16_t word;asm volatile("fnstcw %0":"=m"(word));return word;
#else
    throw std::runtime_error("Required TU1.5 FNSTCW backend unavailable");
#endif
}
void native_load_control_word(std::uint16_t word) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("fldcw %0"::"m"(word):"memory");
#else
    throw std::runtime_error("Required TU1.5 FLDCW backend unavailable");
#endif
}
__m128 native_load_half(M &memory,std::uint32_t address,__m128 value,unsigned offset) {
    std::array<std::byte,16> bytes;std::memcpy(bytes.data(),&value,16);
    memory.read(address,std::span(bytes).subspan(offset,8));
    std::memcpy(&value,bytes.data(),16);return value;
}
void native_store_half(WorldState &s,std::uint32_t instruction,std::uint32_t address,__m128 value,unsigned offset) {
    std::uint64_t bits;
    std::memcpy(&bits,reinterpret_cast<const std::byte*>(&value)+offset,8);
    w(s.memory,s.observe,instruction,address,8,bits);
}
long double native_fpatan(long double y,long double x) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    long double result;
    asm volatile("fpatan":"=t"(result):"0"(x),"u"(y):"st(1)");
    return result;
#else
    throw std::runtime_error("Required TU1.5 FPATAN backend unavailable");
#endif
}
long double native_integer_multiply(long double x,std::int32_t integer,std::uint16_t &status) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    // Original FIMUL m32int: signed memory operand, retained precision/rounding
    // control, and C1 captured before the compiler spills the extended result.
    long double result;
    asm volatile("fimull %3\\n\\tfnstsw %1":"=t"(result),"=m"(status):"0"(x),"m"(integer));
    return result;
#else
    throw std::runtime_error("Required TU1.5 FIMUL32 backend unavailable");
#endif
}
// Intel SDM Vol.2: execute the original transcendental instruction under the
// retained CW and capture C1 before the compiler spills the extended result.
long double native_fyl2x(long double y,long double x,std::uint16_t &status) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    long double result;
    asm volatile("fyl2x\\n\\tfnstsw %1":"=t"(result),"=m"(status):"0"(x),"u"(y):"st(1)");
    return result;
#else
    throw std::runtime_error("Required TU1.5 FYL2X backend unavailable");
#endif
}
long double native_f2xm1(long double x,std::uint16_t &status) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    long double result;
    asm volatile("f2xm1\\n\\tfnstsw %1":"=t"(result),"=m"(status):"0"(x));
    return result;
#else
    throw std::runtime_error("Required TU1.5 F2XM1 backend unavailable");
#endif
}
long double native_fscale(long double x,long double exponent,std::uint16_t &status) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    long double result;
    asm volatile("fldt %3\\n\\tfldt %2\\n\\tfscale\\n\\tfnstsw %1\\n\\tfstpt %0\\n\\tfstp %%st(0)"
        :"=m"(result),"=m"(status):"m"(x),"m"(exponent):"st","st(1)");
    return result;
#else
    throw std::runtime_error("Required TU1.5 FSCALE backend unavailable");
#endif
}
long double native_frndint(long double x,std::uint16_t &status) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    long double result;
    asm volatile("frndint\\n\\tfnstsw %1":"=t"(result),"=m"(status):"0"(x));
    return result;
#else
    throw std::runtime_error("Required TU1.5 FRNDINT backend unavailable");
#endif
}
long double native_fcos(long double x,std::uint16_t &status) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    long double result;
    asm volatile("fcos\\n\\tfnstsw %1":"=t"(result),"=m"(status):"0"(x));
    return result;
#else
    throw std::runtime_error("Required TU1.5 FCOS backend unavailable");
#endif
}
long double native_fptan(long double x,std::uint16_t &status) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    long double result;
    // C2 means range reduction failed and no extra1 was pushed. Balance the
    // actual hardware stack in both cases, retaining the original status.
    asm volatile("fldt %2\\n\\tfptan\\n\\tfnstsw %%ax\\n\\ttestw $0x400,%%ax\\n\\tjnz 1f\\n\\tfstp %%st(0)\\n\\t1: fstpt %1"
        :"=a"(status),"=m"(result):"m"(x):"cc","st","st(1)");
    return result;
#else
    throw std::runtime_error("Required TU1.5 FPTAN backend unavailable");
#endif
}
long double native_fsin(long double x,std::uint16_t &status) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    long double result;
    asm volatile("fsin\\n\\tfnstsw %1":"=t"(result),"=m"(status):"0"(x));
    return result;
#else
    throw std::runtime_error("Required TU1.5 FSIN backend unavailable");
#endif
}
std::uint32_t source_estimate(WorldState &s,std::uint32_t input) {
    if(!s.services.reciprocal_sqrt)throw std::runtime_error("Required original contact RSQRTSS owner unavailable");
    return s.services.reciprocal_sqrt(input);
}
__m128 native_estimate_vector(WorldState &s,__m128 input) {
    std::array<std::uint32_t,4> bits;std::memcpy(bits.data(),&input,16);
    for(auto &lane:bits)lane=source_estimate(s,lane);
    std::memcpy(&input,bits.data(),16);return input;
}
std::uint32_t sign_extend(std::uint32_t x,unsigned n) {
    if(n==1)return std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(x))));
    if(n==2)return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(x))));
    throw std::runtime_error("Unverified contact sign-extension width");
}
'''
    preamble+=extra_preamble
    preamble+='std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage image){if(image==SourcePhysicsContactImage::MapPack11)return p;switch(p){'+''.join(f'case 0x{p:x}u:return 0x{addresses[p]:x}u;' for p in sorted(used_addresses))+'default:throw std::runtime_error("Unbound original contact executable address");}}\n'
    preamble+='std::uint32_t canonical_branch(std::uint32_t p,SourcePhysicsContactImage image){if(image==SourcePhysicsContactImage::MapPack11)return p;switch(p){'+''.join(f'case 0x{q:x}u:return 0x{p:x}u;' for p,q in sorted(branches.items()))+'default:throw std::runtime_error("Unbound TU1.5 contact dispatch target");}}\n'
    preamble+='\n'.join(f'static void run_{p:x}(WorldState &);' for p in ENTRIES)+'\n'
    preamble+='static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail){'+''.join(f'if(target==selected(0x{p:x}u,s.image)){{run_{p:x}(s);return;}}' for p in ENTRIES)+'s.required(site,target,tail);}\n'
    ending='''
} // namespace
void source_run_physics_live_support_tu15(M &m,std::uint32_t entry,
    SourcePhysicsContactRuntimeAbi &abi,FP fp,
    const SourcePhysicsContactRuntimeServices &services,const O &observe) {
    constexpr auto image=SourcePhysicsContactImage::TitleUpdate15;
    FpScope scope(fp);LiveState s{m,observe,services,image};s.load(abi);
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
    if process_heap:
        preamble=preamble.replace('tools/generate_physics_live_support_tu15.py',
            'tools/generate_source_process_heap_runtime.py')
        preamble=preamble.replace('#include "physics_world_runtime_internal.hpp"',
            '#include "physics_world_runtime_internal.hpp"\n#include "halo2/source_process_heap_runtime.hpp"')
        ending=ending.replace('source_run_physics_world_runtime','source_run_process_heap_runtime')
    if runtime_header:preamble=preamble.replace('#include "physics_world_runtime_internal.hpp"',
        '#include "physics_world_runtime_internal.hpp"\n#include "'+runtime_header+'"')
    if runtime_function:ending=ending.replace('source_run_physics_world_runtime',runtime_function)
    ending=ending.replace('} // namespace halo2',
        'bool source_physics_live_support_tu15_owns_entry(std::uint32_t p) {switch(p){'+
        ''.join(f'case 0x{p:x}u:' for p in ENTRIES)+'return true;default:return false;}}\n} // namespace halo2')
    a.output.parent.mkdir(parents=True,exist_ok=True)
    temporary=a.output.with_name('.'+a.output.name+f'.{os.getpid()}.tmp')
    try:
        temporary.write_text(preamble+'\n\n'.join(bodies)+ending)
        temporary.replace(a.output)
    finally:
        temporary.unlink(missing_ok=True)
    report=dict(schema=1,generator_sha256=sha(Path(__file__)),correspondence_sha256=sha(a.correspondence),
        additional_entries_sha256=sha(a.additional_entries),
        original_sha256=PIN,later_original_sha256=NEW_PIN,generated_cpp_sha256=sha(a.output),
        complete_native_entries=[f'{p:08x}' for p in ENTRIES if not any(
            code.at(q).mnemonic in ('fnsave','frstor') for q in paired['body_sites'][f'{p:08x}'])],
        full_x87_environment_required={p:row for p,row in lowered.items() if row['mnemonic'] in ('fnsave','frstor')},
        lowered_instructions=lowered,
        selected_addresses={f'{p:08x}':addresses[p] for p in sorted(used_addresses)},
        external_owners=externals,dispatch_targets={f'{p:08x}':q for p,q in sorted(branches.items())},
        production_opcode_decoder=False,production_original_code_execution=False)
    a.manifest.parent.mkdir(parents=True,exist_ok=True);a.manifest.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(instructions=len(lowered),generated_cpp_sha256=sha(a.output),manifest_sha256=sha(a.manifest))))
# Actual TU1.5 bodies selected from calls reached by fresh live construction.
# No TU1.1 image or procedure executes during recovery or generation.
ENTRIES=(0x2e0c80,0x2ddeb9,0x2dd367,0x2dd524,
         0xb87f0,0xd6d70,0x10d990,0xc1fd0,0x1268e0,0x191040,0x1d2e70,0x29ff80,
         0x1183f0,0x1190f0,0xc5a70,0x10b510,0x16e3d0,
         0x32ba80,0x32d258,0x32d1fd,0x32d221,0x32d1ca,
         0x1255c0,0x21b630,0x2e10a0,0x2e1040,0x381370,0x383970,
         0x149050,0x1490b0,0x1f3830,0x1f4700,0x1f4760,
         0xb8a00,0xb8be0,0xb9960,0xb9b70,0x1caf40,0x14dfe0,
         0xd6dd0,0xc2070,0x118430,0x119130,0xc5ab0,0x10b590,0x10cae0,
         0xc1cf0,0xc20d0,0x10da10,
         0x10daf0,0x10d950,0x10db40,0x10dc30,0x10dca0,0x10e440,
         0x2e1c0,0xba570,0xba630,0xba850,0xba8c0,0xbaba0,0xbbd50,
         0xbc400,0xbc570,0xbc5c0,0xbe2b0,0xbe330,0xbe3b0,0xbf050,
         0xc0200,0xc0270,0xc06c0,0xc0910,0xc0f60,0xc1540,0xc19d0,
         0xd80a0,0x179610,0x1cc4d0,0x1d3180,0x1d3270,0x1d3350,0x1d3380,
         0xde9e0,0x1789f0,0x178ab0,0x176c80,0x1cb200,0x10e3b0,0x10e410,
         0xc6440,0xdea60,0xc0790,0xc6850,0xc68b0,0xdea70,
         0xe0a10,0x111940,0x1175a0,0x111b10,0xbbe00,0xbaf10,
         0x1910a0,0x191130,0x191350,0x1913f0,0x191480,0x191220,0x1912e0,0x33b590,
         0x1d5320,0x1d0250,0x1d0290,0x14e0b0,
         0x14ebe0,0x14e9e0,0x14e7b0,0x15ab10,0x14eb10,0x14f620,
         0x159f80,0x159ff0,0x3825d0,
         # Actual15A43B reaches the complete base engine team-admission
         # callback during Team Slayer reset. Retain its AL-only return and
         # callee stack cleanup through the same instruction lowerer.
         0x25bb50,0x15a2c0,0x15a1a0,0x154ef0,0x1cb030,
         # Actual12054/12818 constructors,191A57/191A89 activation,
         # resource-stage table45F944 rows6/7, and selector191CF4.
         0x125c90,0x125eb0,0x125d40,0x1303c0,0x130500,0x130600,
         0x191ca0,0x13fc50,0x2db4ca,
         0x153070,0x152e80,0x152e00,0x152d60,0xe8a30,0xe65a0,
         0xc93d0,0x30bd0,0x122b60,0xcdcf0,0xc92c0,
         0x13b6f0,0xb8f40,0xb9090,0xb9180,
         # Actual D7B48 authored placement factory and 15C89A/15C8A2
         # complete deactivation/deletion calls, authenticated below.
         0xc1120,0xd75a0,0x120920,0xd7fb0,0x1346c0,
         0xb1840,0x10dbc0,0xc1080,0xb1a70,0xba3e0,0xba490,
         0x168ba3,0x168bc5,
         # Actual observer174654/174933/174A44 visibility and full segment
         # owners. The located world/body state is shared with game physics.
         0x3f4f0,0x171440,0x16c620,
         # Reached from actual10DB94's source type callback during visible
         # multiplayer scenario construction (live-integration-v5).
         0x20fe50,0x18b5c0,
         # Original1918D9 map preparation; includes literal11F02A push0
         # for physical models and the original vehicle shape preparation.
         0x11ef70,0x125790,0x11f060,0x165b30,0x20d810,0x21a580,
         0x1613b0,0x1a8b50,
         # Actual startup row3 random-stream construction, including the
         # original CRT rand/time consumers. Kernel time and current native
         # thread backing are explicit platform inputs of the live owner.
         0x148d40,0x32bbae,0x32c075,0x330179,0x332383,0x32ae00,0x33a4d3,0x2dc124,
         # Complete authored netgame equipment factory: source vehicles,
         # weapons and variant-selected collection members share this owner.
         0x15d4f0,0x171120,0x12ebe0,0x12ecf0,0x17e640,0x17e700,
         # Actual127F48/128012/12801F/12802C/12803E sound-storage owners.
         # Xbox DirectSound device activation remains a separate boundary.
         0x21efe0,0x11f5e0,0x220550,0x220ad0,0x223ea0,0x2247b0,
         0x3835f0,0x383c90,0x3233e0,0x3233d0,0x14fec0,
         # TU1.5 seated camera:15850D selects245EA0;189FD0 resolves the
         # actual parent/seat camera block.1888A0 constructs the input bank;
         # 18A0C0 owns binding its unit.381710..381770 are the four real
         # static observer default writers, not serialized BSS defaults.
         0x245ea0,0x189fd0,0x245c70,0x2455f0,0x158340,
         0x1888a0,0x18a0c0,0x381710,0x381730,0x381750,0x381770,
         0x3d260,0x2b480,0x2b520,0x186cb0,0x186d30,
         # Remaining literal BA3E0 deactivation notifications. All eight
         # original table words and the indexed CALL reader are checked below.
         0x2118a0,0x17e250,
         # BA490's complete light, physical, observer and header retirement
         # owners; authenticated direct call sites are retained below.
         0xc0e90,0x1cc390,0x10dca0,0xbe330,
         #13AB49 advances retained effect instances after object retirement.
         0x179cd0,0x17c370,0x17c5c0)

def complete_body(code, entry):
    # The formatter's jump at 32D27E crosses an immediate ending in CC.
    # That byte belongs to the preceding instruction, not function padding.
    # Recover its entire CFG through the actual epilogue RET at 32D9BE.
    # The following eight original switch pointers are data, not code.
    if entry != 0x32d258:
        return _trace(code, entry, set(ENTRIES))
    end = 0x32d9bf
    assert code.at(end-1).mnemonic == 'ret'
    fn = Function(entry, entry)
    _descend(code, entry, fn, fn.code, [entry],
             lambda p: entry <= p < end,
             lambda p: not (entry <= p < end), set())
    _merge_tables(fn)
    assert fn.tables == [(0x32d9bf, 4, 8)]
    assert not fn.tail_jumps
    assert all(entry <= p < end for p in fn.code)
    return fn
def main():
    global ENTRIES
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output',type=Path,required=True);ap.add_argument('--manifest',type=Path,required=True)
    ap.add_argument('--additional-entries',type=Path,default=Path(__file__).with_name('physics_live_support_tu15_entries.json'))
    a=ap.parse_args();assert sha(NEW_DEFAULT)==NEW_PIN
    assert Xbe(NEW_DEFAULT).read(0x25bb50,5)==bytes.fromhex('b001c20400')
    team_reset=_Code(Xbe(NEW_DEFAULT),'.text').at(0x15a4ab)
    assert team_reset.bytes.hex()=='e810feffff' and team_reset.operands[0].imm==0x15a2c0
    assert Xbe(NEW_DEFAULT).read(0x15a4b0,5)==bytes.fromhex('e8ebfcffff')
    assert Xbe(NEW_DEFAULT).read(0x15a253,5)==bytes.fromhex('e898acffff')
    assert struct.unpack('<I',Xbe(NEW_DEFAULT).read(0x468be8+0x68,4))[0]==0x1cb030
    additions=json.loads(a.additional_entries.read_text())
    assert additions['image_sha256']==NEW_PIN
    ENTRIES=tuple(sorted(set(ENTRIES)|{int(r['target'],16) for r in additions['entries']}))
    original=Xbe(NEW_DEFAULT);code=_Code(original,'.text')
    assert code.at(0x101db8).bytes.hex()=='e873700700'
    camera_selection=code.at(0x15850d)
    assert camera_selection.bytes.hex()=='baa05e2400'
    for entry,destination in ((0x381710,0x55d6e4),(0x381730,0x55d6e8),
                              (0x381750,0x55d6ec),(0x381770,0x55d6f0)):
        assert original.read(entry+8,9)==bytes.fromhex('f30f1105')+struct.pack('<I',destination)+b'\xc3'
    assert struct.unpack('<I',original.read(0x44b548,4))[0]==0x18b5c0
    assert struct.unpack('<I',original.read(0x44af3c,4))[0]==0x148d40
    assert struct.unpack('<I',original.read(0x44b308,4))[0]==0x1f4700
    assert struct.unpack('<I',original.read(0x44b310,4))[0]==0x1f4760
    assert struct.unpack('<II',original.read(0x473a00,8))==(0x1183f0,0x118430)
    assert struct.unpack('<I',original.read(0x44b44c,4))[0]==0x17e640
    assert struct.unpack('<I',original.read(0x44b454,4))[0]==0x17e700
    assert struct.unpack('<III',original.read(0x44b398,12))==(0x186cb0,0x31d400,0x186d30)
    assert struct.unpack('<III',original.read(0x44b350,12))==(0x2b480,0x2b5a0,0x2b520)
    assert original.read(0xba407,6)==bytes.fromhex('ff96c44b4700')
    assert struct.unpack('<8I',original.read(0x474bc4,32))==(0xbd8b0,0xc36a0,
        0x1d21e0,0x155820,0x1693ed,0x1611e0,0x2118a0,0x17e250)
    assert struct.unpack('<I',original.read(0x46d7c8,4))[0]==0x3835f0
    for site,target in ((0xd7b48,0xc1120),(0x15c89a,0xba3e0),(0x15c8a2,0xba490),
                        (0xba4e7,0xc0e90),(0xba53f,0x1cc390),
                        (0xba554,0x10dca0),(0xba55a,0xbe330),
                        (0x1410ac,0x168ba3),(0x1410c2,0x168bc5),
                        (0xc1182,0xd75a0),(0xd76bc,0x120920),(0xd76c3,0xd7fb0),(0xd7706,0x1346c0),
                        (0xc119a,0xb1840),(0xc11b6,0x10dbc0),(0xc11c9,0xc1080),(0xc11cf,0xb1a70),
                        (0x174654,0x3f4f0),(0x3f539,0x171440),
                        (0x1918d9,0x11ef70),(0x11f02c,0x1f2c70),
                        (0x152098,0x1613b0),(0x161401,0x1a8b50),
                        (0x15dbca,0x15d4f0),
                        (0x11ea20,0x171120),
                        (0x1281d,0x12ebe0),(0x191aaf,0x12ecf0),
                        (0x13aaff,0x14fec0),
                        (0x13ab49,0x179cd0),
                        (0x127f48,0x21efe0),(0x127f9b,0x11f5e0),
                        (0x128012,0x220550),(0x12801f,0x220ad0),
                        (0x12802c,0x223ea0),(0x12803e,0x2247b0),
                        (0x174933,0x16c620),(0x174a44,0x16c620)):
        i=code.at(site)
        assert i.mnemonic=='call' and i.operands[0].type==2 and i.operands[0].imm==target
    for row in additions['entries']:
        if 'source_table' in row:
            # Startup callbacks have a literal table owner, rather than a
            # direct CALL instruction. Authenticate the complete pointer.
            slot=int(row['source_table'],16)
            raw=original.read(slot,4)
            assert raw.hex()==row['source_pointer_hex']
            assert struct.unpack('<I',raw)[0]==int(row['target'],16)
            continue
        site=int(row.get('source_site',row.get('reached_site')),16)
        instruction=code.at(site)
        assert instruction.bytes.hex()==row['source_instruction_hex']
        assert instruction.mnemonic=='call' or instruction.mnemonic.startswith('j')
        if instruction.operands[0].type==2:
            assert instruction.operands[0].imm==int(row['target'],16)
        if dispatch:=row.get('source_dispatch'):
            # Literal TU1.5 unit-action descriptors retain the original four
            # callback ABIs. Registration proves bytes and the matching reader;
            # it does not assert that an unvisited action has executed.
            offset=dispatch['callback_offset']
            readers={0:(0xe8e57,0xe8e67),4:(0xe8dca,0xe8dde),
                     8:(0xe8ed1,0xe8ee1),12:(0xe8f31,0xe8f41)}
            reader,call=readers[offset]
            index=dispatch['index'];assert 0<=index<60
            bank=int(dispatch['bank'],16);assert bank==0x473d28
            slot=int(dispatch['pointer_slot'],16);assert slot==bank+index*4
            descriptor=int(dispatch['descriptor'],16)
            assert struct.unpack('<I',original.read(slot,4))[0]==descriptor
            raw=original.read(descriptor,16);assert raw.hex()==dispatch['descriptor_hex']
            assert struct.unpack_from('<I',raw,offset)[0]==int(row['target'],16)
            load=code.at(reader)
            assert reader==int(dispatch['reader'],16) and load.bytes.hex()==dispatch['reader_hex']
            assert load.mnemonic=='mov' and load.operands[1].type==3 and load.operands[1].mem.disp==bank
            assert site==call and instruction.operands[0].type!=2
            if offset:
                callback=code.at(reader+load.size)
                assert callback.mnemonic=='mov' and callback.operands[1].type==3 and callback.operands[1].mem.disp==offset
    entries={f'{p:08x}':p for p in ENTRIES};sites={};instructions={};addresses={};routines=[]
    for entry in ENTRIES:
        body=complete_body(code,entry)
        tables=[dict(disc=p,count=n) for p,width,n in body.tables if width==4]
        assert all(width in (1,4) for _,width,_ in body.tables)
        translations=[dict(address=p,width=width,count=n,raw_hex=original.read(p,width*n).hex())
            for p,width,n in body.tables if width==1]
        sites[f'{entry:08x}']=sorted(body.code);routines.append(dict(entry=entry,tables=tables,byte_tables=translations))
        for p in sorted(body.code):
            i=code.at(p);assert original.read(p,i.size)==bytes(i.bytes)
            instructions[f'{p:08x}']=p;addresses[p]=p;addresses[p+i.size]=p+i.size
            for op in i.operands:
                if op.type==2:addresses[op.imm&0xffffffff]=op.imm&0xffffffff
                if op.type==3:addresses[op.mem.disp&0xffffffff]=op.mem.disp&0xffffffff
    paired=dict(image='English TU1.5',original_sha256=NEW_PIN,complete_bodies=len(ENTRIES),
        entries=entries,body_sites=sites,instruction_map=instructions,
        address_map={f'{p:08x}':q for p,q in addresses.items()},routines=routines)
    a.correspondence=a.manifest.with_name('tu15-bodies.json')
    a.correspondence.parent.mkdir(parents=True,exist_ok=True)
    a.correspondence.write_text(json.dumps(paired,sort_keys=True,indent=2)+'\n')
    generate(a,paired,complete_listings=True)
if __name__=='__main__':main()
