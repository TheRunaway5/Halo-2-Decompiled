#!/usr/bin/env python3
"""Authenticated original direct callers, public result/argument contracts and constant provenance."""
import argparse,hashlib,json,os,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);a=p.parse_args();assert 'HALO2_TEST_SCRATCH'in os.environ
 image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN;code=_Code(image,'.text');section=image.section('.text');raw=bytes(image.section_bytes(section));hits=[]
 for offset,opcode in enumerate(raw[:-5]):
  if opcode not in(0xe8,0xe9):continue
  target=(section.va+offset+5+int.from_bytes(raw[offset+1:offset+5],'little',signed=True))&0xffffffff
  if target not in(0x1fc10,0x1fd00):continue
  i=code.at(section.va+offset);assert i.mnemonic in('call','jmp')and i.operands[0].imm==target
  hits.append(dict(instruction=i.address,target=target,operation=i.mnemonic,bytes=i.bytes.hex()))
 assert {(x['instruction'],x['target'])for x in hits}=={(0x1366c,0x1fd00),(0x13b73,0x1fd00),(0x3f2cf,0x1fd00),(0x2b4fa5,0x1fc10)}
 for pc in(0x1366a,0x13b71,0x3f2cd):assert code.at(pc).mnemonic=='xor'and code.at(pc).op_str=='eax, eax'
 def listing(begin,count):
  out=[];pc=begin
  for _ in range(count):
   i=code.at(pc);assert i;out.append(dict(instruction=pc,bytes=i.bytes.hex(),operation=i.mnemonic,operands=i.op_str));pc+=i.size
  return out
 owners={}
 for entry in(0x1fc10,0x1fd00):
  body=complete_body(code,entry);assert not body.calls and not body.tail_jumps
  owners[hex(entry)]=dict(sites=len(body.code),listing_sha256=hashlib.sha256(''.join(f'{pc:x} {code.at(pc).bytes.hex()} {code.at(pc).mnemonic} {code.at(pc).op_str}\n'for pc in sorted(body.code)).encode()).hexdigest(),returns=[dict(instruction=pc,operands=code.at(pc).op_str)for pc in sorted(body.code)if code.at(pc).mnemonic=='ret'])
 absolute=[]
 for entry in(0x1fc10,0x1fd00):
  for s in image.sections:
   data=bytes(image.section_bytes(s));offset=0
   while(offset:=data.find(struct.pack('<I',entry),offset))>=0:absolute.append(dict(entry=entry,section=s.name,address=s.va+offset));offset+=1
 assert not absolute
 literals={0x46a0d0:'0000803f',0x46a0ec:'17b7d138',0x46a0f0:'0000000000000000',0x46a1d0:'000000000000f03f'}
 for address,value in literals.items():assert image.read(address,len(value)//2).hex()==value
 descriptor=0x45a19c;assert image.read(descriptor,28).hex()=='04000000604f2b000000000007000600060006000600060006000600'
 assert int.from_bytes(image.read(0x480c00+706*4,4),'little')==descriptor
 report=dict(passed=True,original_xbe_sha256=NEW_PIN,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),direct_calls_and_tails=hits,absolute_entry_references=absolute,owners=owners,
  caller_prefixes={'13660':listing(0x13660,20),'13660_later_definition':listing(0x136bf,20),'13B70':listing(0x13b70,18),'3F2B0':listing(0x3f2b0,22),'3F2B0_no_DOF':listing(0x3f3cd,2),'2B4F60_arguments_and_continuation':listing(0x2b4f89,20),'211DA0_void_publication':listing(0x211e03,39)},
  integer_query_callees={hex(e):listing(e,len(complete_body(code,e).code))for e in(0x226f0,0x16aa0)},
  readonly_numerical_literals={hex(at):value for at,value in literals.items()},retained_vector_storage={'pointer474C78':image.read(0x474c78,4).hex(),'pointer474C80':image.read(0x474c80,4).hex(),'vector44AAFC':image.read(0x44aafc,12).hex(),'vector44AB1C':image.read(0x44ab1c,12).hex()},
  authored_void_descriptor=dict(function_index=706,definition=descriptor,return_type=4,evaluator=0x2b4f60,real_arguments=7,bytes=image.read(descriptor,28).hex()),
  owed_contract={'1FC10':'Void; focus lowerXMM0 plus six lazy stackREALs; consume actualRET +24 argument bytes. The sole authored evaluator immediately zeroesEAX and publishes its void(type4) result through211DA0. Callee-saved registers and live x87 input values survive.',
   '1FD00':'Noargs; return active bank pointer inEAX. All three original direct callers zero incomingEAX; absent/inactive preserves it. ActualRET +4 consumed. Other volatile return state is outside ordinary caller ABI.',
   'flags':'Each continuation defines integer flags withCMP/TEST/XOR before its conditional use; private arithmetic condition flags are dead.',
   'SIMD':'13B70 reloadsXMM0 at13B7E;13660 mode2 zerosXMM0 at1367D and ordinary path zerosit at136D4 (the intervening226F0/16AA0 predicates are call-free integer leaves).3F2B0 reloadsDOF XMM0 at3F2E7 on the active path; on the noDOF path it invokes the no-scalar-argument3D5F0 owner. No independent SIMD return is advertised by these ordinary interfaces.',
   'FP':'CW, persistent SW condition/exception bits andMXCSR retained exactly; completedTOP/tags/depth and live caller x87 values preserved. Private in-flight arithmetic values/TOP at a fatal storage exception are not resumable source ABI; binding retains callerTOP and does not fabricate private stack values.'},
  limitations=['Static inventory covers direct relative32 calls/tails and literal entry references; arbitrary computed invocation is outside ordinary callers.',
   'No transitive all-engine proof for arbitrary hidden register contracts inside downstream indirect calls; contract is the authenticated ordinary void/pointer interface.',
   'Physical original uses this x86-64 host executing unchangedi386 instructions offline; original Xbox silicon/rendered parity is separate.'],production_original_execution=False,production_instruction_decoder=False,retained_large_artifacts=[])
 a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print('Authenticated four ordinary timed-screen direct callers and native FP/result contracts')
if __name__=='__main__':main()
