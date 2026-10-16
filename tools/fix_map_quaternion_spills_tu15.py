#!/usr/bin/env python3
"""Repair the retained complete37C5D0 lowering's original FSTP C1 state.

The current shared generator already emits status-aware single spills. This
updates only that retained complete body, preserving every unrelated owner.
"""
import argparse,hashlib,json,re
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
HELPER='''// FSTP m32real replaces C1 with the actual rounding direction, even when
// its numeric float result is identical to the previous retained spill.
std::uint32_t native_single_spill(long double value,std::uint16_t &status) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    float result;std::uint16_t actual;
    asm volatile("fldt %2\\n\\tfstps %0\\n\\tfnstsw %1"
        :"=m"(result),"=m"(actual):"m"(value):"st");
    status=(status&~0x200u)|(actual&0x200u);
    return std::bit_cast<std::uint32_t>(result);
#else
    throw std::runtime_error("Required TU1.5 FSTP32 backend unavailable");
#endif
}
'''
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source',type=Path,default=Path('src/source_map_world_support_tu15_generated.cpp'));p.add_argument('--report',required=True,type=Path);p.add_argument('--canonical-world',action='store_true');a=p.parse_args()
 sha=lambda b:hashlib.sha256(b).hexdigest()
 assert sha(NEW_DEFAULT.read_bytes())==NEW_PIN;image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');body=complete_body(code,0x37c5d0)
 before=a.source.read_text();entry=0x378ab0 if a.canonical_world else 0x37c5d0
 bindings={}
 if a.canonical_world:
  binding_source=a.source.with_name('physics_phantom_world_tu15_generated.cpp').read_text()
  bindings={int(old,16):int(new,16) for old,new in re.findall(r'\{0x([0-9a-f]+)u,0x([0-9a-f]+)u\}',binding_source)}
  assert bindings[entry]==0x37c5d0
 start=before.index(f'void run_{entry:x}(WorldState &s) {{');end=before.index('\n}',start)+2;old=before[start:end]
 sites=[]
 for line in old.splitlines():
  if 'u(spill(s.fp.st(0)))' not in line:continue
  match=re.search(r'w\(s.memory,s.observe,0x([0-9a-f]+)u,',line);assert match
  compiled_site=int(match[1],16);site=bindings[compiled_site] if a.canonical_world else compiled_site;i=code.at(site);assert site in body.code and i.mnemonic=='fstp' and i.operands[0].size==4
  sites.append(dict(site=site,compiled_site=compiled_site,raw=i.bytes.hex(),mnemonic=i.mnemonic,operands=i.op_str))
 assert len(sites)==17 and 'std::uint32_t native_single_spill(' not in before
 new=old.replace('u(spill(s.fp.st(0)))','native_single_spill(s.fp.st(0),s.fp.status)')
 after=before[:start]+HELPER+new+before[end:];a.source.write_text(after)
 a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(dict(reference='English TU1.5 only',original_sha256=NEW_PIN,entry=0x37c5d0,canonical_entry=entry,complete_sites=len(body.code),repaired_spills=sites,before_sha256=sha(before.encode()),after_sha256=sha(after.encode()),boundary='Only17 authenticated FSTP m32 spills in the complete quaternion callback now retain actual C1; original values and unrelated bodies unchanged'),indent=2)+'\n')
 print('PASS repaired authenticated original quaternion spills',len(sites))
if __name__=='__main__':main()
