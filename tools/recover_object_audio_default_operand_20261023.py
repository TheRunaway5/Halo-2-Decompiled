#!/usr/bin/env python3
"""Recover the immutable default operand reached by spatial loop location.

Actual18F9B0 selects this file-backed NONE word for nonzero location types.
The failed per-object request trace reaches its read at18F9F9. No active
listener, cluster mask, sound readiness or voice result is supplied.
"""
import hashlib,json,os
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from build_launcher_engine import source_hashes
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'analysis/combat-presentation-audit-20261022/repair-scenery-audio'
def sha(data):return hashlib.sha256(data).hexdigest()
def main():
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);source=owner/'source'
    follow=OUT.parent/'repair-followup-20261023';pins=json.loads((follow/'source-hashes.json').read_text());assert source_hashes(source)==pins
    image=Xbe(NEW_DEFAULT);assert sha(image.data)==NEW_PIN;code=_Code(image,'.text')
    raw=image.read(0x44ebd8,4);assert raw==bytes(4)
    assert code.at(0x18f9f4).bytes.hex()=='b8d8eb4400'
    assert code.at(0x18f9f9).bytes.hex()=='8b00'
    rel='src/platform/native_tu15_world_memory.cpp';before=(ROOT/rel).read_text();assert sha(before.encode())==pins[rel]
    needle='    // Actual18F900 clears ceil(active BSP clusters/32) DWORDs here before\n'
    assert before.count(needle)==1
    addition='''    // Actual18F9F4 selects this immutable default object reference when
    // a looping request uses an attached/spatial location.18F9F9 copies its
    // literal zero word; it supplies no listener or audio readiness.
    share_immutable_word(memory,0x44ebd8u,0u,
        "native TU1.5 authored spatial-loop default reference operand");
'''
    text=before.replace(needle,addition+needle)
    (owner/'world-memory-before-loop-default.cpp').write_text(before)
    assert (ROOT/rel).read_text()==before
    (ROOT/rel).write_text(text);(source/rel).write_text(text)
    updated=source_hashes(source);assert {p for p in pins if pins[p]!=updated[p]}=={rel}
    receipt=dict(scope=__doc__,path=rel,before_sha256=sha(before.encode()),after_sha256=sha(text.encode()),
        original_xbe_sha256=NEW_PIN,original_operand=dict(address=0x44ebd8,raw_hex=raw.hex(),selector=0x18f9f4,read=0x18f9f9),
        root_production_changed=True,runtime_installed=False,readiness_or_voice_result_supplied=False)
    (OUT/'original-spatial-loop-default-operand.json').write_text(json.dumps(receipt,indent=2)+'\n')
    (follow/'source-hashes.json').write_text(json.dumps(updated,indent=2,sort_keys=True)+'\n');print(json.dumps(receipt),flush=True)
if __name__=='__main__':main()
