#!/usr/bin/env python3
"""Admit the loader correction only after the resident detail omission fails.

Only LoopingSound's definition and Cache::load_looping_sound change. Existing
authored metadata is read, never rewritten; this supplies no playback result.
"""
import hashlib,json,os
from pathlib import Path
from build_launcher_engine import source_hashes
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'analysis/combat-presentation-audit-20261022/repair-scenery-audio'
def sha(data):return hashlib.sha256(data).hexdigest()
def main():
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);source=owner/'source'
    follow=OUT.parent/'repair-followup-20261023';pins=json.loads((follow/'source-hashes.json').read_text());assert source_hashes(source)==pins
    reds=[p for p in OUT.glob('detail-metadata*') if (p/'tests.log').exists() and
        'Resident looping detail sounds were omitted' in (p/'tests.log').read_text()]
    assert reds,'A completed omission regression is required, not a link/setup failure'
    red=max(reds,key=lambda p:p.stat().st_mtime_ns);report=json.loads((red/'report.json').read_text());assert not report['passed']
    changes={}
    for rel,start,end in (
        ('include/halo2/content.hpp','struct LoopingSound {','} // namespace halo2'),
        ('src/assets/content.cpp','LoopingSound Cache::load_looping_sound','std::string LevelContent::describe')):
        path=source/rel;before=path.read_text();after=(ROOT/rel).read_text()
        a=before.index(start);b=before.index(end,a);c=after.index(start);d=after.index(end,c)
        assert before[:a]==after[:c] and before[b:]==after[d:],('Unrelated source drift',rel)
        changes[rel]=dict(before=sha(before.encode()),after=sha(after.encode()))
        path.write_text(after)
    updated=source_hashes(source);assert {p for p in pins if pins[p]!=updated[p]}==set(changes)
    (follow/'source-hashes.json').write_text(json.dumps(updated,indent=2,sort_keys=True)+'\n')
    receipt=dict(scope=__doc__,files=changes,red_reference=str(red),root_production_changed=True,
        playback_verified=False,runtime_installed=False,
        MCC_layout_reference=dict(commit='fd14ef3c46135e9422966c6a64cd926fb0bd23fb',
            path='src/Assembly/Plugins/Halo2MCC/lsnd.xml',sha256='ee0da239263d31995840f31ca193028a89a1efc71a5a40a2185d487aa81f63bd'),
        original_detail_row_bytes=52,authored_metadata_modified=False)
    (OUT/'detail-loader-source-refresh.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt),flush=True)
if __name__=='__main__':main()
