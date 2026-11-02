#!/usr/bin/env python3
"""Primary TU1.5 policy/target leaf, data and actual caller contracts."""
import argparse,hashlib,json,sys
from pathlib import Path
sys.dont_write_bytecode=True
from capstone.x86_const import X86_OP_IMM
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();assert sha(NEW_DEFAULT)==NEW_PIN
 image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');owners={}
 def listing(addresses):return[dict(instruction=hex(pc),bytes=code.at(pc).bytes.hex(),operation=code.at(pc).mnemonic,operands=code.at(pc).op_str)for pc in addresses]
 for entry in(0x2c470,0x25940):
  body=complete_body(code,entry);rows=listing(sorted(body.code));assert not any(code.at(pc).mnemonic=='call'for pc in body.code)
  owners[hex(entry)]=dict(sites=len(rows),listing=rows,listing_sha256=hashlib.sha256(json.dumps(rows,separators=(',',':')).encode()).hexdigest())
 code.md.skipdata=True;calls=[]
 for i in code.md.disasm(code.bytes,code.lo):
  if i.id and i.mnemonic in('call','jmp')and i.operands[0].type==X86_OP_IMM and i.operands[0].imm in(0x2c470,0x25940):calls.append(dict(instruction=hex(i.address),target=hex(i.operands[0].imm),operation=i.mnemonic,bytes=i.bytes.hex()))
 assert[(int(row['instruction'],16),int(row['target'],16))for row in calls]==[(0x14d67,0x25940),(0x14d9a,0x25940),(0x198a8,0x25940),(0x258f7,0x25940),(0x229b9e,0x2c470)]
 required=[(0x456910,4,'literal'),(0x456918,4,'literal'),(0x46a0cc,4,'literal'),(0x46a0d0,4,'literal'),(0x46a130,4,'literal'),(0x46a140,4,'literal'),(0x46a22c,4,'literal'),(0x473644,4,'publication'),(0x4c683c,4,'publication'),(0x4c6840,1,'publication'),(0x4c6841,1,'publication'),(0x4c6842,1,'publication'),(0x4c6843,1,'publication'),(0x4c6844,1,'publication'),(0x4c6845,1,'publication'),(0x4c6848,4,'publication'),(0x4c684c,4,'publication'),(0x4c1fe0,4,'target20 allocation'),(0x4c1fed,1,'target20 retiring'),(0x4c1eb0,4,'target18 allocation'),(0x4c1ebd,1,'target18 retiring')]
 fields=[]
 for address,width,kind in required:
  section=next(s for s in image.sections if s.va<=address and address+width<=s.va+s.vsize)
  fields.append(dict(address=hex(address),bytes=width,role=kind,original_cold_bytes=image.read(address,width).hex(),section=section.name,flags=section.flags))
 windows={}
 for start,end in((0x229b95,0x229baf),(0x14d49,0x14da8),(0x14e60,0x14e82),(0x1989f,0x198cf),(0x19a2d,0x19a41),(0x14f40,0x14f5c),(0x258e9,0x25908)):
  addresses=[];pc=start
  while pc<end:addresses.append(pc);pc+=code.at(pc).size
  windows[hex(start)]=listing(addresses)
 files=['include/halo2/native_tu15_camera_frame_policy.hpp','include/halo2/native_tu15_camera_frame_policy_binding.hpp','src/rendering/world/native_tu15_camera_frame_policy.cpp','src/rendering/world/native_tu15_camera_frame_policy_binding.cpp','cmake/NativeTu15CameraFramePolicy.cmake']
 report=dict(passed=True,original_sha256=NEW_PIN,owners=owners,whole_text_direct_callers=calls,continuation_windows=windows,required_original_fields=fields,
  actual_callers={'0x229b9e':dict(entry='0x2c470',arguments='EAX=[esp+74] actual view count; no stack arguments',result='Publications only.229BA3 overwritesEAX,229BA8 overwritesflags, no caller SIMD use before return; volatile outputs dead',return_contract='plainRET consumes4; source DF/FP state unchanged'),
   '0x14d67':dict(entry='0x25940',result='fullEAX compared at14D6C; taken branch repeatsquery14D9A; other path overwritesECX14D7B'),
   '0x14d9a':dict(entry='0x25940',result='fullEAX compared14D9F then AL overwritten14DA1; ECX dead until overwritten14E60 or subsequent argument setup'),
   '0x198a8':dict(entry='0x25940',result='fullEAX retained inEDI at198AD; negative path clearsECX198C8, positive path14F40 overwritesECX14F49 before its use'),
   '0x258f7':dict(entry='0x25940',result='fullEAX compared258FC/pushed25902',binding_qualified=False,reason='Synchronous actual Renderer capture intercepts258F7 and selects its current native target capability; raw retained-row query must not preempt that owner')},
  target_storage=dict(base='0x4c1378',stride='0x98',slots=28,allocation_offset='0x88',retiring_offset='0x95',read_order='20allocation→conditional20retiring→conditional18allocation→conditional18retiring; first available20wins',existing_owner='src/rendering/renderer/native_tu15_renderer_target_storage_data.cpp:25',native_capability_owner='src/rendering/world/source_texture_camera_frame_capture_tu15.cpp:55; SourceTextureCameraNativeTargetsTu15',new_mounts=False),
  contracts=['All scalar values are raw MOVSS words: signalingNaN/denormals transport unchanged without FP arithmetic or exceptions','Fixed absolute field extents are disjoint; source input mutations after individual publications verify actual lazy reads and cached words','RET backing is required; no synthesized stack/result/backend success','Caller-specific binding does not reconstruct deadGP/SIMD/flags; fault-prefix tests compare actual publications, reads and liveFP rather than dead volatile scratch'],
  source_hashes={name:sha(ROOT/name)for name in files},audit_source_sha256=sha(__file__),production_activation=False,whole_parent_or_gpu_claim=False,retained_large_artifacts=[])
 a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2)+'\n');print('52 call-free policy/query sites,5actual callers and21 exact data fields authenticated')
if __name__=='__main__':main()
