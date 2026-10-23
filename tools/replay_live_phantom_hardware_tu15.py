#!/usr/bin/env python3
"""Run a live source-call capture through unchanged original i386 instructions.

The libc-free ELF only loads actual input bytes and captures output after the
original RET. A ptrace observer changes EIP at that completed return boundary;
no original instruction, callee or resulting physical state is replaced.
"""
import argparse,hashlib,json,struct,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,ROOT
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

ASM=r'''
.code32
.section .bridge,"awx"
.globl _start
_start:
 mov $harness_stack_end,%esp
 mov $243,%eax
 mov $tls,%ebx
 int $0x80
 test %eax,%eax
 jnz failure
 mov tls,%eax
 shl $3,%eax
 or $3,%eax
 mov %ax,%fs
 frstor input+40
 ldmxcsr input+36
 movdqu input+148,%xmm0
 movdqu input+164,%xmm1
 movdqu input+180,%xmm2
 movdqu input+196,%xmm3
 movdqu input+212,%xmm4
 movdqu input+228,%xmm5
 movdqu input+244,%xmm6
 movdqu input+260,%xmm7
 push input+32
 popfl
 mov input,%eax
 mov input+4,%ecx
 mov input+8,%edx
 mov input+12,%ebx
 mov input+20,%ebp
 mov input+24,%esi
 mov input+28,%edi
 mov input+16,%esp
 int3
 jmp *entry
.globl capture
capture:
 mov %eax,output
 mov %ecx,output+4
 mov %edx,output+8
 mov %ebx,output+12
 mov %esp,output+16
 mov %ebp,output+20
 mov %esi,output+24
 mov %edi,output+28
 mov $harness_stack_end,%esp
 pushfl
 pop output+32
 stmxcsr output+36
 fnsave output+40
 movdqu %xmm0,output+148
 movdqu %xmm1,output+164
 movdqu %xmm2,output+180
 movdqu %xmm3,output+196
 movdqu %xmm4,output+212
 movdqu %xmm5,output+228
 movdqu %xmm6,output+244
 movdqu %xmm7,output+260
 mov $output,%ecx
 mov $276,%edx
 call write_exact
EMIT_BANKS
 xor %ebx,%ebx
 jmp terminate
write_exact:
 test %edx,%edx
 jz io_done
 mov $4,%eax
 mov $1,%ebx
 int $0x80
 cmp $-4,%eax
 je write_exact
 test %eax,%eax
 jle failure
 add %eax,%ecx
 sub %eax,%edx
 jmp write_exact
io_done:
 ret
failure:
 mov $1,%ebx
terminate:
 mov $1,%eax
 int $0x80
.balign 16
entry: .long ENTRY
input: .incbin "INPUT_FILE"
tls: .long -1,FS_BASE,0xfffff,0x51
.balign 16
output: .space 276
harness_stack: .space 65536
harness_stack_end:
'''

def reference_text_gaps(image,capture,regions,output,section_name='.text',span=None):
 # MCC-native execution keeps compiled code outside SourceRuntimeMemory. The
 # independent CPU reference needs original instructions in its own process.
 # Preserve every captured input: authenticate shared text bytes, then mount
 # only absent code ranges from the pinned image. Never replace runtime data.
 text=next(s for s in image.sections if s.name==section_name);raw=image.section_bytes(text)
 first,end=text.va,text.va+len(raw)
 if span is not None:
  lo,hi=span;assert first<=lo<hi<=end
  raw=raw[lo-first:hi-first];first,end=lo,hi
 cursor=first;gaps=[];shared=[]
 for r in sorted(regions,key=lambda r:r['address']):
  lo=max(first,r['address']);hi=min(end,r['address']+r['size'])
  if lo>=hi:continue
  assert lo>=cursor,'Overlapping captured text owners'
  data=(capture/r['before_file']).read_bytes();offset=lo-r['address']
  assert data[offset:offset+hi-lo]==raw[lo-first:hi-first],'Captured original text bytes changed'
  if cursor<lo:gaps.append((cursor,lo))
  shared.append(dict(address=lo,bytes=hi-lo));cursor=hi
 if cursor<end:gaps.append((cursor,end))
 extra=[]
 for lo,hi in gaps:
  path=output/f'reference-{section_name}-{lo:x}.bin';path.write_bytes(raw[lo-first:hi-first])
  extra.append(dict(address=lo,size=hi-lo,reference_file=path))
 return text,extra,dict(source_section=section_name,source_code_start=first,source_code_bytes=len(raw),source_text_sha256=hashlib.sha256(raw).hexdigest(),
  added_reference_code_bytes=sum(r['size']for r in extra),shared_text_owners=shared,
  scope='Pinned TU1.5 code gaps only in isolated offline reference process; captured runtime inputs unchanged')


def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--return-breakpoints',action='store_true');p.add_argument('--capture',required=True,type=Path);p.add_argument('--output',required=True,type=Path);a=p.parse_args()
 a.capture=a.capture.resolve();a.output=a.output.resolve();assert not a.output.exists();a.output.mkdir(parents=True)
 sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
 assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);d=json.loads((a.capture/'capture.json').read_text());assert d['entry'] in (0x2fc690,0xb9090,0x28c2c0,0x290030,0x2fbb70,0x2fc570,0x2fbb10,0x119060,0x297270,0x297400,0x296180,0xc1120,0xf0e70,0xb9180,0xc92c0,0xf2340,0xf48c0,0xce440,0x1da750,0x355d0,0x1e1170,0x1cc8d0,0x14fec0,0xb8f40,0x37c5d0,0x2ee660,0x2f1930,0x2fc5d0,0x158340,0x245ea0,0x18a300,0x18a2a0,0xec780,0xcb7a0,0x157fe0,0x158290,0x2f9d70,0x217470,0x217310,0x382230,0x170030,0x170080,0xbc440,0x158690) and d['reference']=='English TU1.5 only'
 initial=d['before'];env=bytearray(108);tag=0xffff;top=(initial['x87_status']>>11)&7
 for n,value in enumerate(initial['x87_values']):
  raw=bytes.fromhex(value);mantissa=int.from_bytes(raw[:8],'little');exponent=int.from_bytes(raw[8:],'little')&0x7fff
  kind=1 if exponent==0 and mantissa==0 else 2 if exponent in (0,0x7fff) else 0
  index=(top+n)&7;tag=(tag&~(3<<(index*2)))|(kind<<(index*2));env[28+n*10:38+n*10]=raw
 for off,value in [(0,initial['x87_control_word']),(4,initial['x87_status']),(8,tag)]:struct.pack_into('<H',env,off,value)
 data=struct.pack('<10I',*initial['gp'],initial['eflags'],initial['mxcsr'])+env+b''.join(struct.pack('<4I',*x) for x in initial['xmm']);assert len(data)==276
 (a.output/'input.bin').write_bytes(data)
 regions=sorted(d['regions'],key=lambda r:r['address']);assembly=[];sections=[]
 text,code_gaps,code_authentication=reference_text_gaps(image,a.capture,regions,a.output)
 # B8F40 can query original GPU resource-busy state during object retirement.
 # Authenticate every admitted instruction, including the non-.text XDK owner.
 # No original instruction, memory input, result or hardware poll is replaced.
 authenticated_sites=[];authenticated_external_code=[]
 if d['entry']==0xb8f40:
  from oracle_smg_firing_1_1 import _Code
  from generate_physics_live_support_tu15 import complete_body
  code=_Code(image,'D3D');body=complete_body(code,0x4057f0);assert len(body.code)==48
  instructions=[code.at(pc)for pc in sorted(body.code)]
  # This section is writable in the original image. Admit only the complete
  # contiguous130-byte, call-free query body; never seed missing GPU data.
  assert not any(i.mnemonic=='call'for i in instructions)
  assert all(a.address+len(a.bytes)==b.address for a,b in zip(instructions,instructions[1:]))
  start=instructions[0].address;end=instructions[-1].address+len(instructions[-1].bytes);assert end-start==130
  _,external_gaps,external_auth=reference_text_gaps(image,a.capture,regions,a.output,'D3D',(start,end))
  code_gaps.extend(external_gaps);authenticated_sites=[i.address for i in instructions]
  authenticated_external_code.append(dict(section='D3D',entry=0x4057f0,instruction_sites=len(authenticated_sites),code_authentication=external_auth))
 for n,r in enumerate(sorted([*regions,*code_gaps],key=lambda r:r['address'])):
  path=r.get('reference_file',a.capture/r.get('before_file',''))
  assembly.append(f'.section .bank{n},"awx"\n.incbin "{path}"\n')
  sections.append(f'.bank{n} 0x{r["address"]:x} : {{ *(.bank{n}) }}')
  assert not(r['address']<=0x70000000<r['address']+r['size'])
 emit='\n'.join(f' mov ${r["address"]},%ecx\n mov ${r["size"]},%edx\n call write_exact' for r in regions)
 asm=ASM.replace('EMIT_BANKS',emit).replace('ENTRY',str(d['entry'])).replace('INPUT_FILE',str(a.output/'input.bin')).replace('FS_BASE',str(initial['fs_base']))
 (a.output/'bridge.s').write_text(asm+'\n'+'\n'.join(assembly));(a.output/'link.ld').write_text('ENTRY(_start)\nSECTIONS {\n'+'\n'.join(sections)+'\n.bridge 0x70000000 : { *(.bridge) }\n}\n')
 subprocess.run(['as','--32',str(a.output/'bridge.s'),'-o',str(a.output/'bridge.o')],check=True,capture_output=True)
 subprocess.run(['ld','-melf_i386','-T',str(a.output/'link.ld'),str(a.output/'bridge.o'),'-o',str(a.output/'original-hardware')],check=True,capture_output=True)
 symbols={line.split()[2]:int(line.split()[0],16) for line in subprocess.check_output(['nm',str(a.output/'original-hardware')],text=True).splitlines() if len(line.split())==3}
 tracer=ROOT/('tools/live_physics_hardware_return_tu15.cpp' if a.return_breakpoints else 'tools/live_phantom_hardware_trace_tu15.cpp');subprocess.run(['c++','-std=c++20','-O2',str(tracer),'-o',str(a.output/'tracer')],check=True,capture_output=True)
 trace_command=[str(a.output/'tracer'),str(a.output/'original-hardware'),str(a.output/'output.bin'),hex(d['return']),hex(symbols['capture']),hex(text.va+text.vsize),str(a.output/'trace.json')]
 if a.return_breakpoints:
  from oracle_smg_firing_1_1 import _Code
  from generate_physics_live_support_tu15 import complete_body
  code=_Code(image,'.text');body=complete_body(code,d['entry']);returns=[q for q in sorted(body.code) if code.at(q).mnemonic.startswith('ret')];assert 0<len(returns)<=4
  trace_command.append(','.join(hex(q) for q in returns))
 if authenticated_sites and not a.return_breakpoints:trace_command.append(','.join(hex(pc)for pc in authenticated_sites))
 subprocess.run(trace_command,check=True,timeout=60)
 with (a.output/'output.bin').open('rb') as output:
  raw=output.read(276);assert len(raw)==276
  actual=dict(gp=list(struct.unpack_from('<8I',raw)),eflags=struct.unpack_from('<I',raw,32)[0],mxcsr=struct.unpack_from('<I',raw,36)[0],
   x87_control_word=struct.unpack_from('<H',raw,40)[0],x87_status=struct.unpack_from('<H',raw,44)[0],
   xmm=[list(struct.unpack_from('<4I',raw,148+16*n)) for n in range(8)],x87_values=[raw[68+n*10:78+n*10].hex() for n in range(len(d['after']['x87_values']))])
  # POPFL in this user-mode i386 harness retains the host IF bit. It
  # differs already at entry; compare every other flag and retain both raw
  # words. The original force instructions are never modified to clear IF.
  trace=json.loads((a.output/'trace.json').read_text());flag_mask=0xfffffdff
  assert (trace['entry_eflags']^initial['eflags'])&flag_mask==0,'Unexpected hardware entry flags'
  differences=[dict(field=k,original=v,native=d['after'][k]) for k,v in actual.items()
    if k=='eflags' and (v&flag_mask)!=(d['after'][k]&flag_mask)]
  differences += [dict(field=k,original=v,native=d['after'][k]) for k,v in actual.items()
    if k!='eflags' and v!=d['after'][k]]
  compared=0
  for r in regions:
   expected_path=a.capture/r.get('after_file',r['before_file']);expected=expected_path.read_bytes();original=output.read(r['size']);assert len(original)==r['size'];compared+=len(original)
   if original!=expected:
    first=next(n for n,(x,y) in enumerate(zip(original,expected)) if x!=y);differences.append(dict(field='memory',address=r['address']+first,size=r['size']))
  assert output.read(1)==b''
 trace=json.loads((a.output/'trace.json').read_text());report=dict(reference='English TU1.5 only',passed=not differences,original_instructions=trace['original_instructions'],compared_memory_bytes=compared,reference_code_authentication=code_authentication,
  differences=differences,original_abi=actual,original_sha256=NEW_PIN,harness_sha256=sha(a.output/'original-hardware'),tracer_sha256=sha(tracer),
  host='Linux i386 CPU execution with ptrace observation; not Xbox hardware',eflags_comparison_mask=flag_mask,hardware_entry_eflags=trace['entry_eflags'],native_entry_eflags=initial['eflags'],host_fixed_interrupt_flag=True,return_breakpoint_observation=a.return_breakpoints,authenticated_external_code=authenticated_external_code,entry=d['entry'],boundary='One real live physics call; unchanged original text, no replaced callee/result. All accessible final source bytes and GP/SSE/x87/FP controls/status compared.')
 (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items() if k!='original_abi'}));return 0 if report['passed'] else 1

if __name__=='__main__':raise SystemExit(main())
