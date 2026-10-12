#!/usr/bin/env python3
"""Immutable source-only original lobby/countdown/local-user caller evidence."""
import argparse,csv,hashlib,json
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from oracle_mp_spawn_local_map_protection import FROZEN as OLDER
from functions import _trace
OUT=ROOT/'analysis/multiplayer/integration/spawn-source/lobby-source'
BASE_PIN='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
FROZEN={**OLDER,
 'tools/oracle_mp_spawn_local_map_protection.py':'1a716ec285fbd08ed14de2e4e05e6b5eea4f909f316b8640a676baf41a5fffb6',
 'docs/MP_LOCAL_MAP_PROTECTION_RESEARCH.md':'8c3eda9da6d8b5b339b446251f1d36a65b236bf498824aa2ab33f48e8901d8a6',
 'analysis/multiplayer/integration/spawn-source/local-map-protection/report.json':'aaa95ea9b7904ae16182a8e7a19396b25cd204f71b597fb05d17d932ab5076cc',
 'analysis/multiplayer/integration/spawn-source/local-map-protection/independent-freeze-verification.json':'72e4ad33ca7aa0ce9017ddc2fdb67763b32d92285493b7381931c23dc50e573b'}
ENTRIES=(0x25106c,0x24fbea,0x19a78e,0x19a2ce,0x19a902,0x19a8d0,
 0x199c47,0x199c94,0x647a0,0x64900,0x5df30,0x5bf10,0x6e360,0x6e410,
 0x75870,0x75890,0x8dfc0,0x8df50,0x59230,0x592f0,0x14f190,0x14f270,
 0x18fb34,0x18f58c,0x18fee9,0x190d4b,0x64610,0x64690,0x3314b0,0x14b410,0x190262)
def digest(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'report.json');args=ap.parse_args();args.output.parent.mkdir(parents=True,exist_ok=True)
 for s,h in FROZEN.items():assert digest(ROOT/s)==h,s
 x=Xbe(ROOT/'assets/disc/default.xbe');assert hashlib.sha256(x.data).hexdigest()==BASE_PIN
 code=_Code(x,'.text');starts={int(r['address'],16)for r in csv.DictReader((ROOT/'analysis/retail/functions.tsv').open(),delimiter='\t')};starts|=set(ENTRIES)
 directory=args.output.parent/'instructions';directory.mkdir(exist_ok=True);listings=[]
 for entry in ENTRIES:
  r=_trace(code,entry,starts);ins=[code.at(a)for a in sorted(r.code)]
  for i in ins:assert x.read(i.address,len(i.bytes))==i.bytes
  p=directory/f'{entry:08x}.asm';p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins))
  listings.append(dict(entry=f'{entry:08x}',file=p.name,sha256=digest(p),rows=len(ins),direct_calls=[dict(site=f'{a:08x}',target=f'{b:08x}')for a,b,k in r.sites if k=='call'],tail_jumps=[dict(site=f'{a:08x}',target=f'{b:08x}')for a,b,k in r.sites if k=='tail']))
 assert x.read(0x25107e,4).hex()=='6a036a0a'and x.read(0x251092,4).hex()=='6a036a0a'
 assert code.at(0x251082).op_str==code.at(0x251096).op_str=='0x19a78e'
 assert x.read(0x199c75,6).hex()=='4683fe047ceb'
 selected={f'{a:08x}':dict(bytes=code.at(a).bytes.hex(),mnemonic=code.at(a).mnemonic,operands=code.at(a).op_str)for a in(0x25107e,0x251080,0x251082,0x251092,0x251094,0x251096,0x19a7a0,0x19a7ab,0x19a7c0,0x19a7ca,0x19a7d1,0x19a7d7,0x199c66,0x199c68,0x199c6d,0x199c6f,0x199c71,0x199c72,0x199c75,0x199c76,0x199c79,0x199c7f,0x199c83)}
 paths=['tools/reference/halo2-decompiled/src/'+s for s in('unknown_19987f.cpp','unknown_250155.cpp','unknown_24fa12.cpp','unknown_054fe0.cpp','unknown_059ad0.cpp','unknown_058dd0.cpp','unknown_075870.cpp','unknown_08e090.cpp','unknown_14b560.cpp','unknown_18f576.cpp','unknown_190001.cpp')]
 out=dict(schema=1,kind='source_only_original_static_caller_evidence',base_xbe_sha256=BASE_PIN,tool_sha256=digest(__file__),frozen_pins=FROZEN,reference_sources={s:digest(ROOT/s)for s in paths},instruction_listings=listings,instruction_manifest_sha256=hashlib.sha256(json.dumps(listings,sort_keys=True,separators=(',',':')).encode()).hexdigest(),verified_static_addresses=sum(r['rows']for r in listings),selected_original_instructions=selected,executed_guest_instructions=0,provided_readiness_or_membership_or_clock_inputs=[],instruction_patches=[],claims=[
 'Untouched base25106C ordinary screen action has two branches; both push3 then10 before19A78E. EAX receives caller controller from[ESP+4]. These are original caller countdown/minimum literals, not a recovered full countdown lifecycle.',
 'Base19A78E reads54E8E0+controller*C70+4, rejectsNONE, requires source state9 or10; state9 additionally checks592F0 then requests its caller countdown. State10 reads19A8D0, compares against minimum, decrements only if greater and forwards to64900. No state, controller/session-user table or countdown value is supplied or executed here.',
 'Base199C47 walks indices0..3 ascending, invokes647A0 with ECX=index/EDX=current output cursor, incrementsEDI only for trueAL and advances output cursor byC. It then supplies ECX=EDIcount to59230. Base647A0 reads valid4CD904+index*D0 and, only when nonzero, copies exactly12bytes from4CD905+index*D0 and returnsAL1. The reconstructed reference XUID type is not used as the runtime stride authority.',
 'Base24FBEA separately requests3/3 under its distinct static automatic branch. Source6E360 decrements an observed nonnegative positive countdown only after a1000ms elapsed comparison; this does not prove owning session startup, clock scheduling or elapsed-time preservation under a large time step.',
 'Base75870/75890 and8DF50/8DFC0 establish static cached network-time/read/update paths using510548/51054C and original GetTickCount call sites. No cached time value, provider, session, storage/readiness or clock result is supplied or executed by this source-only suite.',
 'Base14F190/14F270 statically own local-user/controller count updates on actual attach/detach. They require the original source-owned player globals, player and local device/view membership lifecycle; four available storage slots are not two actual participants.',
 'Base18F58C is static local-controller initialization. Base18FB34 calls identity producer14B410 and64610 when controller flag10 isclear; Base18FEE9 calls64610 when controller+4 isNONE.64610 searches the four actual valid bytes, owns slot clear/valid/12-byte identity writes, with original14B410 identity producer when its pointer input isNULL. These routines are listed, not executed; raw profile/device identity/storage ownership and initialization callbacks remain unresolved.',
 'This is exclusively original static byte evidence. No complete menu/lobby/countdown execution, actual profile/controller/session ownership, current local participant count, full original map-start, TU address mapping or live Xbox behavior is claimed. Production and all prior frozen suites remain unchanged.'])
 raw=(json.dumps(out,indent=2,sort_keys=True)+'\n').encode()
 if args.output.exists():assert args.output.read_bytes()==raw
 else:args.output.write_bytes(raw)
 for s,h in FROZEN.items():assert digest(ROOT/s)==h,s
 print(json.dumps(dict(report=str(args.output),sha256=digest(args.output),tool_sha256=out['tool_sha256'],listings=len(listings),static_addresses=out['verified_static_addresses'],manifest=out['instruction_manifest_sha256'])))
if __name__=='__main__':main()
