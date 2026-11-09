#!/usr/bin/env python3
"""Private dense x87 ownership, against a separate physical i386 instruction oracle.

This qualifies the ownership interface, not a native binding of a TU1.5 owner.
The i386 helper executes real x87/SSE instructions with complete FSAVE input;
no arithmetic outcomes, instructions, or provider-completion results are supplied.
"""
import argparse,hashlib,json,os,platform,struct,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
INPUTS=('src/physics/runtime/native_tu15_raw_x87_scope.hpp','cmake/NativeTu15RawX87Scope.cmake','src/physics/runtime/physics_step_internal.hpp','include/halo2/physics_contact_runtime.hpp','tests/native_tu15_raw_x87_scope_tests.cpp','tests/native_tu15_raw_x87_fault_observer.cpp','tools/validate_native_raw_x87_scope_tu15.py','src/physics/runtime/physics_contact_runtime_internal.hpp','src/physics/runtime/physics_world_runtime_internal.hpp','src/physics/runtime/physics_solver_internal.hpp','src/platform/native_tu15_tick_conversion.hpp')
ASSEMBLY=r'''.code32
.section .bss
.balign 16
input:.space 128
output:.space 116
.section .text
.globl _start
_start:
 xorl %edi,%edi
read_more:
 movl $3,%eax
 xorl %ebx,%ebx
 leal input(%edi),%ecx
 movl $128,%edx
 subl %edi,%edx
 int $0x80
 testl %eax,%eax
 jz done
 js failed
 addl %eax,%edi
 cmpl $128,%edi
 jne read_more
 movl $0,output+112
 fninit
 frstor input
 ldmxcsr input+108
 movl input+112,%eax
 cmpl $0,%eax
 je finish
 cmpl $1,%eax
 jne op2
 flds input+116
 jmp finish
op2:cmpl $2,%eax
 jne op3
 fadds input+116
 jmp finish
op3:cmpl $3,%eax
 jne op4
 fdivs input+116
 jmp finish
op4:cmpl $4,%eax
 jne op5
 fchs
 jmp finish
op5:cmpl $5,%eax
 jne op6
 fsts output+112
 jmp finish
op6:cmpl $6,%eax
 jne op7
 fstps output+112
 jmp finish
op7:cmpl $7,%eax
 jne op8
 fsqrt
 jmp finish
op8:cmpl $8,%eax
 jne op9
 faddp
 jmp finish
op9:cmpl $9,%eax
 jne op10
 movss input+116,%xmm0
 divss input+116,%xmm0
 jmp finish
op10:cmpl $10,%eax
 jne op11
 fldcw input+116
 jmp finish
op11:cmpl $11,%eax
 jne failed
 ldmxcsr input+116
finish:
 fnsave output
 stmxcsr output+108
 movl $4,%eax
 movl $1,%ebx
 movl $output,%ecx
 movl $116,%edx
 int $0x80
 cmpl $116,%eax
 jne failed
 jmp _start
done:
 testl %edi,%edi
 jnz failed
 xorl %ebx,%ebx
 jmp exit
failed:movl $1,%ebx
exit:movl $1,%eax
 int $0x80
.section .note.GNU-stack,"",@progbits
'''
VALUES=('00000000000000a0ff3f','0100000000000080ff3f','35c26821a2da0fc90040','00000000000000000080','01000000000000000000','45230100000000c0ff7f','4523010000000080ff7f','0000000000000080ff7f','ffffffffffffffff7f40','0000000000000080693f')
def tag(value):
 sig,exp=struct.unpack('<QH',value);exp&=0x7fff
 return 1 if not exp and not sig else 2 if not exp or exp==0x7fff or not sig>>63 else 0
def corpus():
 rows=[]
 for count in (0,1,4,8):
  for top in range(8):
   for pc in (0,2,3):
    for rc in range(4):
     for op in ((0,1,9,10,11) if count==0 else (0,2,3,4,5,6,7,8,9,10,11) if count>1 else (0,2,3,4,5,6,7,9,10,11)):
      n=len(rows);raw=bytearray(108);cw=0x7f|(pc<<8)|(rc<<10);sw=(top<<11)|((n%8)<<8)|((n//7)%64)|(((n//11)%2)<<14)
      # C1 and all sticky exception bits are actual incoming hardware state.
      sw&=~0x8080
      if (n//13)%2:sw|=0x41  # Stale masked stack-fault/invalid sticky bits, dense current topology.
      struct.pack_into('<HHHHHH',raw,0,cw,0xffff,sw,0xffff,0xffff,0xffff);tags=0xffff
      for i in range(count):
       value=bytes.fromhex(VALUES[(i+n//9)%len(VALUES)]);raw[28+10*i:38+10*i]=value;shift=((top+i)&7)*2;tags=(tags&~(3<<shift))|(tag(value)<<shift)
      struct.pack_into('<H',raw,8,tags)
      mx=0x1f80|(rc<<13)|(0x8040 if n%2 else 0)|((n//5)%64)
      operand=(0x7f|(({0:2,2:3,3:0}[pc])<<8)|(((rc+1)%4)<<10))if op==10 else (0x1f80|(((rc+1)%4)<<13)|((n+5)%64)|(0x8040 if not n%2 else 0))if op==11 else (0x3f800001,0x40400000,0x00000001,0x7f800001,0,0x80000000,0x3f000000)[n%7]
      rows.append(dict(name=f'dense-{count}-top-{top}-pc-{pc}-rc-{rc}-op-{op}',count=count,op=op,operand=operand,before=dict(x87=raw.hex(),mxcsr=mx)))
 return rows

def child_oracles(xbe,scratch):
 from validate_native_tu15_prt_helpers import physical_original,PIN
 if hashlib.sha256(xbe.read_bytes()).hexdigest()!=PIN:raise ValueError('Wrong English TU1.5 original image')
 sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
 original=Xbe(xbe);children=[];proofs=[]
 for kind in ('conversion','integer'):
  rows=[]
  for depth in ((0,1,4,6) if kind=='conversion' else (8,)):
   for pc in (0,2,3):
    for rc in range(4):
     n=len(children)+len(rows);number=(733333333,733333334,(1<<53)+1,-(1<<63),-1,0)[n%6]&0xffffffffffffffff
     sp=0x62001000+n%8;bank=bytearray(8192);entry=0x22a5c0 if kind=='conversion' else 0x32acd0
     args=(0x63000000,number&0xffffffff,number>>32)if kind=='conversion'else(0x63000000,number&0xffffffff,number>>32,0x80000003,0)
     struct.pack_into('<'+'I'*len(args),bank,sp-0x62000000,*args)
     gp=[0x67890000+i+n*16 for i in range(8)];gp[4]=sp;cw=0x7f|(pc<<8)|(rc<<10);sw=((n%8)<<11)|(n%64)|(((n//3)%8)<<8)|(((n//2)%2)<<14);sw&=~0x8080
     rows.append(dict(id=len(rows),name=f'{kind}-depth-{depth}-pc-{pc}-rc-{rc}',entry=entry,gp=gp,eflags=0x647,matrix=args[1],source=args[2],before_fp=[cw,sw,0x1f80|(rc<<13)|(n%64)|(0x8040 if n%2 else 0)],live_x87=[VALUES[(i+n)%len(VALUES)]for i in range(depth)],xmm=[[0x12340000+i*4+k+n*32 for k in range(4)]for i in range(8)],max_steps=1000,banks=[dict(address=0x62000000,before=bank.hex())]))
  owned=scratch/kind;owned.mkdir()
  traces=[dict(entry=pc,arguments=0,gp=True,xmm=True,x87_delta=delta)for pc,delta in ((0x32ae70,0),(0x32acd0,0),(0x22a601,1),(0x22a615,2),(0x22a619,1),(0x22a61e,1))]if kind=='conversion'else None
  actual,receipt=physical_original(original,rows,owned,entries=(0x22a5c0,0x32ae70,0x32acd0)if kind=='conversion'else(0x32acd0,),regions=((0x62000000,8192),),trace_calls=traces,trace_extended=kind=='conversion',return_x87=kind=='conversion')
  proofs.append(receipt)
  for before,after in zip(rows,actual['cases'],strict=True):
   def packet(gp,flags,fp,xmm,live,bank,stores):return dict(gp=gp,eflags=flags,fp=fp,xmm=xmm,live_x87=live,bank=bank,stores=stores)
   child=dict(name=before['name'],entry=before['entry'],gp=before['gp'],eflags=before['eflags'],fp=before['before_fp'],live_x87=before['live_x87'],xmm=before['xmm'],bank=before['banks'][0]['before'])
   child['after']=packet(after['boundary_gp'],after['boundary_eflags'],after['after_fp'],after['boundary_xmm'],([after['returned_extended']]+before['live_x87'])if kind=='conversion'else before['live_x87'],after['banks'][0]['after'],after['writes'])
   if kind=='conversion':
    phase=next(call for call in after['calls']if call['entry']==0x22a615);last=next(i for i,event in enumerate(after['writes'])if event['instruction']==0x22a609);stores=after['writes'][:last+1];bank=bytearray.fromhex(child['bank'])
    for event in stores:
     value=bytes.fromhex(event['written']);offset=event['address']-0x62000000;bank[offset:offset+len(value)]=value
    child['partial']=packet(phase['gp'],phase['eflags'],phase['fp'],phase['xmm'],phase['live_x87'],bank.hex(),stores)
   children.append(child)
 return children,proofs

def run(command,**kw):
 result=subprocess.run(command,capture_output=True,**kw)
 if result.returncode:
  stderr=result.stderr.decode(errors='replace') if isinstance(result.stderr,bytes)else result.stderr
  raise ValueError('Command failed '+str(command)+' exit='+str(result.returncode)+': '+stderr[:4000]+'\n[tail]\n'+stderr[-2500:])
 return result

def negative_controls(scratch,command,fixture,env):
 source=(ROOT/INPUTS[0]).read_text()
 mutations=(
  ('early-spill-commit','const auto after=capture();restore(before);publish(before);','const auto after=capture();restore(after);publish(after);'),
  ('provider-restores-caller','catch(...) {restore(source);publish(source);throw;}','catch(...) {restore(host_);publish(host_);throw;}'),
  ('drops-source-status','return_host(source);','restore(host_);'),
  ('drops-child-continuation','try {image=imported();}','try {image=host_;}'),
  ('truncates-extended-value','image.x87.data()+28+n*10,&state_.fp.lanes[n],10','image.x87.data()+28+n*10,&state_.fp.lanes[n],8'),
 )
 receipts=[]
 for name,old,new in mutations:
  if source.count(old)!=1:raise ValueError('Native x87 mutant marker changed '+name)
  owned=scratch/name;header=owned/'physics/runtime/native_tu15_raw_x87_scope.hpp';header.parent.mkdir(parents=True);header.write_text(source.replace(old,new));binary=owned/'mutant'
  actual=command.copy();actual.insert(1,'-I'+str(owned));actual[actual.index('-o')+1]=str(binary);run(actual)
  result=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,env=env)
  if not result.returncode:raise ValueError('Native x87 semantic mutant survived '+name)
  receipts.append(dict(name=name,exit=result.returncode,diagnostic=result.stderr[-700:],mutated_header_sha256=hashlib.sha256(header.read_bytes()).hexdigest()))
 return receipts

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--compiler',default='c++');p.add_argument('--xbe',type=Path,required=True);p.add_argument('--sanitizers',action='store_true');p.add_argument('--negative-controls',action='store_true');p.add_argument('--report',type=Path,required=True);args=p.parse_args()
 if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Use tools/run_test.py')
 if sys.platform!='linux' or platform.machine().lower()not in('x86_64','amd64'):print('SKIP private raw x87 proof needs Linux x86-64 ptrace');return 77
 if not args.xbe.is_file():print('SKIP missing original TU1.5 image');return 77
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']);pins={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest()for name in INPUTS}
 rows=corpus();assembly=scratch/'oracle.s';assembly.write_text(ASSEMBLY);obj=scratch/'oracle.o';oracle=scratch/'oracle'
 run(['as','--32',str(assembly),'-o',str(obj)]);run(['ld','-melf_i386',str(obj),'-o',str(oracle)])
 incoming=b''.join(bytes.fromhex(row['before']['x87'])+struct.pack('<III',row['before']['mxcsr'],row['op'],row['operand'])+b'\0'*8 for row in rows)
 proof=run([str(oracle)],input=incoming).stdout
 if len(proof)!=116*len(rows):raise ValueError('Physical i386 output length differs')
 for i,row in enumerate(rows):
  packet=proof[116*i:116*(i+1)];row['after']=dict(x87=packet[:108].hex(),mxcsr=struct.unpack_from('<I',packet,108)[0]);row['bits']=struct.unpack_from('<I',packet,112)[0]
 children,child_proofs=child_oracles(args.xbe,scratch)
 fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(cases=rows,children=children),separators=(',',':')))
 fault_assembly=scratch/'fault-oracle.s';fault_text=ASSEMBLY.replace(' movl input+112,%eax',' int3\n movl input+112,%eax').replace('fsts output+112','fsts 0').replace('fstps output+112','fstps 0');fault_assembly.write_text(fault_text)
 run(['as','--32',str(fault_assembly),'-o',str(scratch/'fault-oracle.o')]);run(['ld','-melf_i386',str(scratch/'fault-oracle.o'),'-o',str(scratch/'fault-oracle')])
 fault_observer=scratch/'fault-observer';run([args.compiler,'-std=c++20','-O2',str(ROOT/'tests/native_tu15_raw_x87_fault_observer.cpp'),'-o',str(fault_observer)])
 fault_proof=json.loads(run([str(fault_observer),str(scratch/'fault-oracle'),str(fixture)],text=True).stdout)
 binary=scratch/'native-x87-scope';command=[args.compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(ROOT/'tests/native_tu15_raw_x87_scope_tests.cpp'),'-o',str(binary)]
 if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
 run(command);env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0');result=run([str(binary),str(fixture)],text=True,env=env);native=json.loads(result.stdout)
 mutants=negative_controls(scratch,command,fixture,env)if args.negative_controls else []
 if pins!={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest()for name in INPUTS}:raise ValueError('Private FP scope source drift')
 report=dict(passed=True,private_unbound=True,english_tu15_xbe_sha256=hashlib.sha256(args.xbe.read_bytes()).hexdigest(),source_sha256=pins,compiler=args.compiler,sanitizers=args.sanitizers,physical_i386_cases=len(rows),oracle_assembly_sha256=hashlib.sha256(ASSEMBLY.encode()).hexdigest(),oracle_transfer_sha256=hashlib.sha256(proof).hexdigest(),native=native,negative_controls=mutants,actual_write_faults=fault_proof,fault_oracle_assembly_sha256=hashlib.sha256(fault_text.encode()).hexdigest(),genuine_child_physical_proofs=child_proofs,qualified=dict(stale_masked_sf_status=True,changed_source_controls_survive_final_save=True,source_only_fp_io=True,retained_child_outcomes='Actual native Tick and integer owners through State.required; real original i386 results and22A615 partial state',dense_incoming_depths=[0,1,4,8],all_top_values=True,precision_controls=[24,53,64],all_rounding_modes=True,full_logical80_tags_status_controls=True,source_mxcsr_across_io=True,staged_fst_fstp=True,provider_failures=True,completed_observer_failures=True,caller_hardware_stack_preserved=True,genuine_tick_and_integer_children=True,partial_thrown_child_fp=True,nested_child_scope=True),open_domains=['Not bound to a production owner','Sparse tags and MMX cannot be represented by dense State.fp','Unmasked exceptions, stack overflow/underflow, asynchronous FP signals','FIP/FDP/opcode have no retained State ABI fields','Compiler-owned x87 temporaries cannot span this native scope','Existing outer fenv scope does not restore caller-saved x87 C0/C1/C2/C3' ])
 args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(result.stdout.strip());return 0
if __name__=='__main__':raise SystemExit(main())
