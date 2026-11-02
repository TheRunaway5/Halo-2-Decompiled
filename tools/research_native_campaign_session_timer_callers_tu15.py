#!/usr/bin/env python3
"""Primary-only direct caller and volatile-return chain for777E0 timer retirement."""
import argparse,bisect,csv,hashlib,json,os,struct
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
OWNER_ENTRY_RECOVERIES=(0x553a0,0x6e830,0x6ebb0,0x2cf449)
ROOTS=(0x777e0,0x77ab0,0x77af0,0x77be0,0x78020)
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser();p.add_argument('--report',type=Path,required=True);p.add_argument('--taint',action='store_true');p.add_argument('--compact',action='store_true');p.add_argument('--extra-entry',action='append',type=lambda x:int(x,0),default=[]);a=p.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH']);assert scratch.is_dir();assert sha(NEW_DEFAULT)==NEW_PIN
 image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');inventory=ROOT/'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv';starts=sorted({int(r['address'],16)for r in csv.DictReader(inventory.open(),delimiter='\t')}|set(OWNER_ENTRY_RECOVERIES));traces={};owners={}
 for entry in(0x553a0,0x2cf449):assert code.byte(entry-1)in(0xcc,0xc3)
 assert code.at(0x6e830).bytes.hex()=='51'
 def trace(entry):
  if entry not in traces:traces[entry]=_trace(code,entry,set(starts))
  return traces[entry]
 def owner(pc):
  nearest=bisect.bisect_right(starts,pc)-1
  for entry in reversed(starts[max(0,nearest-2):nearest+1]):
   if pc in trace(entry).code:return entry
  return None
 def describe(entry):
  if entry in owners:return
  body=trace(entry);listing=[]
  for pc in sorted(body.code):
   ins=code.at(pc);assert image.read(pc,ins.size)==bytes(ins.bytes);listing.append(dict(pc=f'0x{pc:x}',bytes=ins.bytes.hex(),op=ins.mnemonic,args=ins.op_str,registers_read=[ins.reg_name(r)for r in ins.regs_access()[0]],registers_written=[ins.reg_name(r)for r in ins.regs_access()[1]]))
  owners[entry]=dict(entry=f'0x{entry:x}',sites=len(body.code),listing=listing,listing_sha256=hashlib.sha256(json.dumps(listing,separators=(',',':')).encode()).hexdigest(),direct_calls=[dict(site=f'0x{pc:x}',target=f'0x{target:x}',kind=kind)for pc,target,kind in body.sites],tables=body.tables)
 def references(target):
  edges=[];unqualified=[];pointers=[];data=code.bytes
  for offset in range(len(data)-4):
   if data[offset]not in(0xe8,0xe9):continue
   pc=code.lo+offset
   if(pc+5+struct.unpack_from('<i',data,offset+1)[0])&0xffffffff!=target:continue
   ins=code.at(pc)
   if ins.mnemonic not in('call','jmp')or not ins.operands or ins.operands[0].type!=2 or ins.operands[0].imm!=target:continue
   entry=owner(pc)
   if entry is None:unqualified.append(dict(site=f'0x{pc:x}',bytes=ins.bytes.hex(),reason='Not in nearest complete primary CFG'));continue
   describe(entry);edges.append(dict(site=f'0x{pc:x}',kind=ins.mnemonic,target=f'0x{target:x}',owner=f'0x{entry:x}',return_site=f'0x{pc+ins.size:x}',bytes=ins.bytes.hex()))
  needle=struct.pack('<I',target)
  for section in image.sections:
   raw=image.section_bytes(section);cursor=0
   while(at:=raw.find(needle,cursor))>=0:
    va=section.va+at;cursor=at+1;row=dict(section=section.name,address=f'0x{va:x}',bytes=needle.hex());pcowners=[]
    if code.inside(va):
     nearest=bisect.bisect_right(starts,va)-1
     for entry in reversed(starts[max(0,nearest-2):nearest+1]):
      for pc in trace(entry).code:
       ins=code.at(pc)
       if pc<=va<pc+ins.size and any(op.type==2 and op.imm==target for op in ins.operands):pcowners.append(dict(owner=f'0x{entry:x}',pc=f'0x{pc:x}',op=ins.mnemonic,args=ins.op_str));describe(entry)
    row['primary_code_operand_refs']=pcowners;pointers.append(row)
  return dict(target=f'0x{target:x}',direct_edges=edges,unqualified_byte_candidates=unqualified,literal_pointer_occurrences=pointers)
 pending=list(ROOTS);done=set();refs=[]
 # Finite two-hop discovery; exact EAX propagation conclusions are reviewed from
 # whole primary listings rather than guessed function signatures.
 for depth in range(3):
  following=[]
  for target in pending:
   if target in done:continue
   done.add(target);describe(target);ref=references(target);ref['depth']=depth;refs.append(ref);following +=[int(e['owner'],16)for e in ref['direct_edges']]
  pending=following
 for entry in a.extra_entry:describe(entry)
 liveness=[]
 if a.taint:
  refcache={int(r['target'],16):r for r in refs}
  def refs_for(target):
   if target not in refcache:refcache[target]=references(target)
   return refcache[target]
  register_masks={'eax':0xffffffff,'ax':0xffff,'al':0xff,'ah':0xff00}
  for initial in refs_for(0x777e0)['direct_edges']:
   first=int(initial['return_site'],16);frame=int(initial['owner'],16)
   pending=[(first,0xffffffff,frame,(),())];seen=set();ends=[];count=0
   while pending:
    pc,mask,frame,stack,path=pending.pop();key=(pc,mask,frame,stack)
    if key in seen:continue
    seen.add(key);count+=1
    if count>20000:
     ends.append(dict(kind='unclosed_capacity',site=f'0x{pc:x}',mask=hex(mask)));break
    ins=code.at(pc);assert ins is not None;describe(frame)
    # The decoded instruction must belong to a genuine complete body. Nested
    # call entries and their return continuations are kept distinct in stack.
    current_owner=owner(pc)
    if current_owner is None:
     ends.append(dict(kind='unqualified_instruction',site=f'0x{pc:x}',mask=hex(mask)));continue
    describe(current_owner);trail=path+(f'{pc:x} {ins.mnemonic} {ins.op_str}',)
    reads,writes=ins.regs_access();readmask=0;writemask=0
    for r in reads:readmask|=register_masks.get(ins.reg_name(r),0)
    for r in writes:writemask|=register_masks.get(ins.reg_name(r),0)
    zero=ins.mnemonic in('xor','sub')and len(ins.operands)==2 and ins.operands[0].type==1 and ins.operands[1].type==1 and ins.operands[0].reg==ins.operands[1].reg
    if zero:readmask=0
    if len(ins.operands)==2 and ins.operands[1].type==2:
     width=register_masks.get(ins.reg_name(ins.operands[0].reg),0)if ins.operands[0].type==1 else 0
     if width and((ins.mnemonic=='or'and ins.operands[1].imm&width==width)or(ins.mnemonic=='and'and ins.operands[1].imm&width==0)):readmask=0
    if mask&readmask:
     ends.append(dict(kind='consumed',site=f'0x{pc:x}',owner=f'0x{current_owner:x}',remaining_mask=hex(mask),read_mask=hex(readmask),instruction=trail[-1],path=list(trail)));continue
    after=mask&~writemask
    if not after:
     ends.append(dict(kind='killed',site=f'0x{pc:x}',owner=f'0x{current_owner:x}',instruction=trail[-1],path=list(trail)));continue
    mask=after;following=pc+ins.size
    if ins.mnemonic=='call':
     if ins.operands[0].type!=2:
      ends.append(dict(kind='required_indirect_callee_input_contract',site=f'0x{pc:x}',owner=f'0x{current_owner:x}',remaining_mask=hex(mask),path=list(trail)));continue
     target=ins.operands[0].imm
     if not code.inside(target):
      ends.append(dict(kind='required_external_callee_input_contract',site=f'0x{pc:x}',target=hex(target),remaining_mask=hex(mask),path=list(trail)));continue
     if len(stack)>24:
      ends.append(dict(kind='unclosed_call_depth',site=f'0x{pc:x}',remaining_mask=hex(mask)));continue
     pending.append((target,mask,target,stack+((following,frame),),trail));continue
    if ins.mnemonic.startswith('ret'):
     if stack:
      nextpc,nextframe=stack[-1];pending.append((nextpc,mask,nextframe,stack[:-1],trail));continue
     outer=refs_for(frame)
     for edge in outer['direct_edges']:
      callpc=int(edge['site'],16);outerframe=int(edge['owner'],16)
      if edge['kind']=='call':pending.append((int(edge['return_site'],16),mask,outerframe,(),trail+(f'OUTER CALL {callpc:x} -> {frame:x}',)))
      else:pending.append((callpc,mask,outerframe,(),trail+(f'OUTER TAIL {callpc:x} -> {frame:x}',)))
     pointers=[p for p in outer['literal_pointer_occurrences']if p['section']in('.rdata','.data')or p['primary_code_operand_refs']]
     if pointers:ends.append(dict(kind='unclosed_indirect_return_users',entry=hex(frame),remaining_mask=hex(mask),pointers=pointers,path=list(trail)))
     if not outer['direct_edges']and not pointers:ends.append(dict(kind='no_direct_outer_caller',entry=hex(frame),remaining_mask=hex(mask),path=list(trail)))
     continue
    if ins.mnemonic=='jmp':
     if ins.operands[0].type==2:pending.append((ins.operands[0].imm,mask,frame,stack,trail))
     else:
      operand=ins.operands[0];tables=trace(current_owner).tables
      table=next((t for t in tables if operand.type==3 and operand.mem.disp==t[0]and t[1]==4),None)
      if table:
       for n in range(table[2]):pending.append((code.dword(table[0]+n*4),mask,frame,stack,trail))
      else:ends.append(dict(kind='required_indirect_jump_contract',site=hex(pc),remaining_mask=hex(mask),path=list(trail)))
     continue
    if ins.mnemonic.startswith('j')and ins.operands and ins.operands[0].type==2:pending.append((ins.operands[0].imm,mask,frame,stack,trail))
    pending.append((following,mask,frame,stack,trail))
   liveness.append(dict(timer_call=initial,conservative_states=count,terminals=ends))
  refs=list(refcache.values())
 report=dict(passed=True,scope=__doc__,original_sha256=NEW_PIN,inventory_sha256=sha(inventory),additional_primary_owner_entries=[hex(x)for x in OWNER_ENTRY_RECOVERIES],references=refs,volatile_eax_liveness=liveness,owners=list(owners.values()),source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__)},owned_scratch=str(scratch),instruction_patches=[],runtime_execution=False,limits=['Direct xrefs authenticated in complete canonical CFGs; raw literal occurrences are not assumed indirect execution.','Three-hop discovery does not itself establish volatile outputs dead; explicit read/kill conclusions require review of each continuation.'])
 if a.compact:
  for row in report['owners']:
   row['listing']=[i['pc']+' '+i['bytes']+' '+i['op']+' '+i['args']for i in row['listing']]
  report['listing_format']='primaryPC bytes mnemonic operands; original decoded register access metadata used only by conservative EAX walk'
 a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(owners=len(owners),sites=sum(x['sites']for x in owners.values()),refs=[dict(target=r['target'],direct=r['direct_edges'],pointers=r['literal_pointer_occurrences'])for r in refs])))
if __name__=='__main__':main()
