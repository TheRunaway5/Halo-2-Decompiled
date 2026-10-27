#!/usr/bin/env python3
"""Read-only full-salt metadata ownership for a selected MCC gameplay pause.

Run via tools/run_test.py. This reports the existing Cache unloaded-only
shared-directory rule; it neither supplies a substitute UI nor changes assets.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
from mcc_content import MccCache


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--maps',type=Path,required=True)
    parser.add_argument('--report',type=Path,required=True)
    args=parser.parse_args();Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    caches={name:MccCache(args.maps/(name+'.map')) for name in ('lockout','shared','single_player_shared','mainmenu')}
    requested=0xe1744268
    def row(tag):
        return None if tag is None else dict(datum=f"{tag['datum']:08x}",group=tag['group'],name=tag['name'],
            address=f"{tag['address']:08x}",size=tag['size'],file_offset=tag['offset'],resident=bool(tag['address']))
    reports=[]
    for name,c in caches.items():
        reports.append(dict(cache=name,path=str(c.path),sha256=hashlib.sha256(c.path.read_bytes()).hexdigest(),
            header_scenario=f'{c.u32(c.index+12):08x}',header_globals=f'{c.u32(c.index+16):08x}',
            requested=row(c.tags.get(requested)),groups={g:[row(t) for t in c.tags.values() if t['group']==g and t['address']]
                for g in ('matg','wgtz','wigl')},
            pause18=[row(t) for t in c.tags.values() if t['group']=='wgit' and t['address'] and c.u16(t['offset']+4)==18]))
    primary=caches['lockout'];shared=caches['shared'];resolved=[]
    def resolve(datum,group):
        local=primary.tags.get(datum);other=shared.tags.get(datum)
        if local and local['address']:selected,owner=local,primary
        else:
            assert other and other['address'],(hex(datum),group,'No resident authenticated shared owner')
            if local:assert (local['datum'],local['group'],local['name'])==(other['datum'],other['group'],other['name'])
            selected,owner=other,shared
        assert selected['group']==group
        resolved.append(dict(reference=f'{datum:08x}',group=group,owner=str(owner.path),local=row(local),resolved=row(selected)))
        return selected,owner
    scnr,sc=resolve(primary.u32(primary.index+12),'scnr');kind=struct.unpack_from('<h',sc.data,scnr['offset']+0x10)[0]
    matg,mc=resolve(primary.u32(primary.index+16),'matg')
    count,pointer=struct.unpack_from('<II',mc.data,matg['offset']+0x110);assert count and pointer!=0xffffffff
    interface=mc.pointer(pointer,mc.magic,0x98)
    wgtz,wc=resolve(mc.u32(interface+(0x8c if kind==0 else 0x94 if kind==1 else 0x84)),'wgtz')
    wigl,gc=resolve(wc.u32(wgtz['offset']+4),'wigl')
    report=dict(passed=True,scope=__doc__,scenario_type=kind,requested_datum=f'{requested:08x}',caches=reports,
        authoritative_chain=resolved,metadata_merge_owner='Cache::initialize_mcc + merge_shared_tag_directory(unloaded_only)',
        original_inputs_modified=False,raw_banks_retained=False)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(passed=True,scenario_type=kind,requested=f'{requested:08x}',
        chain=[dict(datum=r['reference'],group=r['group'],owner=r['owner'],name=r['resolved']['name']) for r in resolved],report=str(args.report))))


if __name__=='__main__':main()
