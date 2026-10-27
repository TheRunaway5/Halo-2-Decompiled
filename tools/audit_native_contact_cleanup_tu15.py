#!/usr/bin/env python3
"""Authenticate source contact cleanup maps, stages and frame-action ownership."""
import argparse,hashlib,json,os,struct
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
def sha(p):
 with Path(p).open('rb')as f:return hashlib.file_digest(f,'sha256').hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');entries=(0x303c70,0x303d00,0x304350,0x303b70,0x303f50,0x37ed60,0x37ede0,0x37edeb,0x37ed68,0x37edf6,0x32c193,0x3314a9,0x330d50,0x3315c0);bodies={entry:{pc:code.at(pc)for pc in complete_body(code,entry).code}for entry in entries};assert [len(bodies[e])for e in entries[:5]]==[51,53,534,16,16]
 maps=[]
 for handler,info,expected in((0x37ed68,0x46c02c,[(0xffffffff,0x37ed60)]),(0x37edf6,0x46c084,[(0xffffffff,0x37ede0),(0,0x37edeb)])):
  h=bodies[handler];assert h[handler].mnemonic=='mov'and h[handler].operands[1].imm==info and h[handler+5].mnemonic=='jmp'and h[handler+5].operands[0].imm==0x32c193;magic,count,table=struct.unpack('<III',image.read(info,12));assert magic==0x19930520 and count==len(expected);rows=[struct.unpack('<II',image.read(table+k*8,8))for k in range(count)];assert rows==expected;maps.append(dict(handler=hex(handler),func_info=hex(info),header_bytes=image.read(info,12).hex(),magic=hex(magic),state_count=count,map=hex(table),map_bytes=image.read(table,count*8).hex(),actions=[dict(state=k,next_state=-1 if to==0xffffffff else to,entry=hex(action))for k,(to,action)in enumerate(rows)]))
 stages=[dict(owner='0x303c70',arm='0x303ca9',after_reads=['allocator identity','allocator remaining bytes'],cleanup='0x303b70',normal_completion='0x303cf3',frame='establisher+0x0c = actual constructor entrySP',object='word(frame-0x10)'),dict(owner='0x303d00',arm='0x303d33',cleanup='0x303b70',disarm='0x303d66',disarm_after='capacity read0x303d61',disarm_before='owned-data release0x303d86',frame='establisher+0x0c = actual destructor entrySP',object='word(frame-0x10)'),dict(owner='0x304350',states=[dict(state=0,arm='0x30440a',after='initial component count read0x304402',before='component constructor0x304411',action='0x303f50',object='frame-0x220'),dict(state=1,arm='0x304422',after='post-constructor count/transform/container reads0x304416/0x30441b/0x30441e',action='0x303d00',object='frame-0x2c4',disarm_to=0,disarm='0x304a62',before='explicit component destructor0x304a6a'),dict(state=-1,arm='0x304a78',after='record capacity read0x304a6f',before='explicit owned-record release0x304aa0')],frame='aligned producer establisher+0x0c; retained packet base+0x330')]
 for row in stages:
  owner=int(row['owner'],16)
  for pc in([row['arm']]if 'arm'in row else[s['arm']for s in row['states']]):assert int(pc,16)in bodies[owner]
 # Runtime evidence is taken from actual code rather than an assumed ABI:
 # 330D50 reads state atframe+8, checks maximum atFuncInfo+4, indexes its
 # map atFuncInfo+8 withstate*8, writes next-state BEFORE calling the action.
 # 3315C0 establishes EBP=establisher+0C for the genuine action CALL EAX.
 assert bodies[0x330d50][0x330d8e].mnemonic=='shl'and bodies[0x330d50][0x330d8e].operands[1].imm==3
 assert bodies[0x330d50][0x330da8].mnemonic=='mov'and bodies[0x330d50][0x330db8].operands[0].imm==0x3315c0
 assert bodies[0x3315c0][0x3315cb].mnemonic=='add'and bodies[0x3315c0][0x3315cb].operands[1].imm==12 and bodies[0x3315c0][0x3315e5].mnemonic=='call'
 listings={hex(e):[dict(pc=hex(pc),bytes=i.bytes.hex(),mnemonic=i.mnemonic,operands=i.op_str)for pc,i in sorted(body.items())]for e,body in bodies.items()};lp=a.output/'listings.json';lp.write_text(json.dumps(dict(image_sha256=NEW_PIN,bodies=listings),indent=2)+'\n');report=dict(completed=True,image_sha256=NEW_PIN,source_sha256=sha(__file__),listings_sha256=sha(lp),maps=maps,stages=stages,actual_runtime_evidence=dict(dispatch='0x3314e4 calls0x330d50 during original unwind',state_read='0x330d5f',max_state_check='0x330d82',map_pointer_read='0x330d91',map_index_scale='0x330d8e',next_state_write_before_action='0x330da8',action_call='0x330db8',frame_adjustment='0x3315cb',actual_action_dispatch='0x3315e5'),native_policy='Native C++ exception guards advance/disarm before cleanup and follow these actual resource stages/order. No guest FS registration, register-file reconstruction or source exception dispatcher is implemented.',native_exception_cleanup_proven=False,source_fs_raw_prefix_gate='RED unchanged',remaining=['Physical original action versus native cleanup comparisons','Native primary/prewrite/observer exception transitions','Secondary-throw/termination platform details','Producer opaque-input read/lifetime and future reuse closure','Native-only live binding/full physics closure'],owned_scratch=os.environ['HALO2_TEST_SCRATCH'],retained_large_artifacts=[]);(a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(completed=True,source_entries=len(bodies),unwind_maps=len(maps))))
if __name__=='__main__':main()
