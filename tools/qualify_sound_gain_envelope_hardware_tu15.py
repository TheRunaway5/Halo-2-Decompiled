#!/usr/bin/env python3
"""Observe unchanged127CF0 gain envelopes on physical i386, without code patches.

Child-only debug-register breakpoints read EAX at127D2D and127D3B; original
execution continues normally through RET. Rebuildable images/captures stay
in run_test.py owned scratch. No game/voice/device readiness is supplied.
"""
import argparse,ctypes,hashlib,json,os,signal,struct,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from qualify_loop_combined_publication_hardware_tu15 import ASM


def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--input',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
 if a.output.exists():p.error('Preserve existing reference evidence')
 ref=json.loads(a.input.read_text());assert ref['original_xbe_sha256']==NEW_PIN and len(ref['scalar'])==768
 assert ref['every_executed_site_has_rooted_provenance'] and not ref['source_instruction_patches'] and not ref['substituted_calls']
 x=Xbe(NEW_DEFAULT);assert hashlib.sha256(x.data).hexdigest()==NEW_PIN;image=bytearray(0x800000)
 for section in x.sections:
  raw=x.section_bytes(section);image[section.va-0x10000:section.va-0x10000+len(raw)]=raw
 (scratch/'original.bin').write_bytes(image);stack=ref['stack'];stop=0x103f000
 assembly=ASM.replace('entry: .long 0x2216d0','entry: .long 0x127cf0').replace('ORIGINAL_IMAGE',str(scratch/'original.bin'))
 assembly=assembly.replace('OUTPUT_ADDRESS',str(ref['output'])).replace('CALLER_ARGUMENTS',str(stack+4))
 (scratch/'reference.s').write_text(assembly)
 (scratch/'link.ld').write_text('ENTRY(_start)\nSECTIONS {\n.original 0x10000 : { *(.original) }\n.banks 0x1000000 : { *(.banks) }\n.return 0x103f000 : { *(.return) }\n.bridge 0x70000000 : { *(.bridge) }\n}\n')
 commands=[['as','--32',str(scratch/'reference.s'),'-o',str(scratch/'reference.o')],
  ['ld','-melf_i386','-T',str(scratch/'link.ld'),str(scratch/'reference.o'),'-o',str(scratch/'original-hardware')],
  ['objcopy','--dump-section','.original='+str(scratch/'verified.bin'),str(scratch/'original-hardware')]]
 for argv in commands:subprocess.run(argv,check=True,capture_output=True)
 assert (scratch/'verified.bin').read_bytes()==image
 text=next(s for s in x.sections if s.name=='.text')
 def patches(rows):
  data=bytearray(struct.pack('<I',len(rows)))
  for at,value in rows:
   raw=bytes.fromhex(value);assert at+len(raw)<=text.va or at>=text.va+text.vsize
   data+=struct.pack('<II',at,len(raw))+raw
  return data
 def word(value):return struct.pack('<I',value&0xffffffff).hex()
 payload=patches(ref['memory'])
 for row in ref['scalar']:
  payload+=patches(ref['accepted_base_cases'][row['base_case']]['memory']+row['memory']+
   [[stack,word(stop)],[stack+4,word(row['offset'])],[stack+8,word(row['control'])]])
  payload+=struct.pack('<I',0);environment=bytearray(108);struct.pack_into('<H',environment,0,row['fp_control']);struct.pack_into('<H',environment,8,0xffff)
  payload+=struct.pack('<10I',0,0,0,0,stack,0,ref['definition'],0,2,row['mxcsr'])+environment+bytes(128)
 payload+=struct.pack('<I',0);(scratch/'inputs.bin').write_bytes(payload)
 libc=ctypes.CDLL(None,use_errno=True);libc.ptrace.restype=ctypes.c_long;libc.ptrace.argtypes=[ctypes.c_uint,ctypes.c_uint,ctypes.c_void_p,ctypes.c_void_p]
 def trace(request,pid,address=0,data=0):
  if libc.ptrace(request,pid,ctypes.c_void_p(address),ctypes.c_void_p(data))==-1:raise OSError(ctypes.get_errno(),'child ptrace')
 def child():trace(0,0)
 class Registers(ctypes.Structure):
  _fields_=[(s,ctypes.c_ulonglong) for s in ('r15','r14','r13','r12','rbp','rbx','r11','r10','r9','r8','rax','rcx','rdx','rsi','rdi','orig_rax','rip','cs','eflags','rsp','ss','fs_base','gs_base','ds','es','fs','gs')]
 observations=[]
 with (scratch/'inputs.bin').open('rb') as incoming,(scratch/'capture.bin').open('wb') as outgoing:
  process=subprocess.Popen([str(scratch/'original-hardware')],stdin=incoming,stdout=outgoing,preexec_fn=child)
  try:
   pid,status=os.waitpid(process.pid,0);assert os.WIFSTOPPED(status) and os.WSTOPSIG(status)==signal.SIGTRAP
   trace(6,pid,848,0x127d2d);trace(6,pid,856,0x127d3b);trace(6,pid,904,5);trace(7,pid)
   while True:
    _,status=os.waitpid(pid,0)
    if os.WIFEXITED(status):assert os.WEXITSTATUS(status)==0;process.returncode=0;break
    assert os.WIFSTOPPED(status) and os.WSTOPSIG(status)==signal.SIGTRAP,status
    regs=Registers();trace(12,pid,0,ctypes.addressof(regs));assert regs.rip==(0x127d2d if len(observations)%2==0 else 0x127d3b)
    observations.append(regs.rax&0xffffffff);assert len(observations)<=2*len(ref['scalar']);trace(7,pid)
  finally:
   if process.returncode is None:process.kill();process.wait()
 raw=(scratch/'capture.bin').read_bytes();assert len(raw)==372*len(ref['scalar']) and len(observations)==2*len(ref['scalar'])
 for i,row in enumerate(ref['scalar']):
  assert observations[2*i]==row['expected'] and observations[2*i+1]==row['ducked_class_gain'],i
  assert struct.unpack_from('<I',raw,i*372)[0]==row['expected_total'],i
 report=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,cases=len(ref['scalar']),all_envelope_class_and_final_bits_equal=True,
  original_instructions_patched=[],substituted_calls=[],reference_sha256=hashlib.sha256(a.input.read_bytes()).hexdigest(),
  hardware_tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),commands=commands,retained_large_artifacts=[],
  physical_xbox_or_audible_device_comparison=False)
 a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2)+'\n')
 print('PASS768 physical original gain envelopes, class gains and complete returns')


if __name__=='__main__':main()
