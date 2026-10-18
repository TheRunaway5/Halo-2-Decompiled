#!/usr/bin/env python3
"""Authenticate canonical damage-observer PCs against complete TU1.5 routines.

This emits comparison metadata only. Native arithmetic, public canonical entry
IDs, observer records and original executable instructions are not modified.
"""
import argparse,json,re
from pathlib import Path
from audit_rendering import file_sha
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code,_trace,PIN
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN,full_tokens

BASELINE='32a7d51b0159e2927cf34a75dcf5d00b6be28c42bd2ae42b3e5465c7f45a6142'


def mapping(source):
    assert file_sha(source)==BASELINE,'Re-audit changed damage delivery source'
    text=source.read_text()
    selection=re.search(r'std::uint32_t selected\(.*?\n',text).group()
    addresses={int(p,16):int(q,16) for p,q in re.findall(r'case 0x([0-9a-f]+)u:return 0x([0-9a-f]+)u;',selection)}
    bodies={int(p,16):body for p,body in re.findall(r'(?:static )?void run_([0-9a-f]+)\(WorldState &s\) \{\n(.*?)\n\}',text,re.S)}
    declared={int(p,16) for p in re.findall(r'static void run_([0-9a-f]+)\(WorldState &\);',text)}
    assert set(bodies)==declared and len(bodies)==90
    old_path=ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'
    assert file_sha(old_path)==PIN and file_sha(NEW_DEFAULT)==NEW_PIN
    old,new=Xbe(old_path),Xbe(NEW_DEFAULT);oc,nc=_Code(old,'.text'),_Code(new,'.text')
    targets={addresses[p] for p in bodies};result={};owners=[];store_sites=set()
    for entry,body in bodies.items():
        sites=sorted({int(p,16) for p in re.findall(r's\.services\.instruction\(0x([0-9a-f]+)u\)',body)})
        assert sites==sorted(_trace(oc,entry,set(bodies)).code),hex(entry)
        actual=sorted(_trace(nc,addresses[entry],targets).code)
        assert len(sites)==len(actual),hex(entry)
        assert full_tokens([oc.at(p) for p in sites],old)==full_tokens([nc.at(p) for p in actual],new),hex(entry)
        owners.append(dict(canonical_entry=entry,tu15_entry=addresses[entry],instructions=len(sites)))
        store_sites.update(int(p,16) for p in re.findall(r'(?:w|vw)\(s\.memory,s\.observe,0x([0-9a-f]+)u',body))
        for p,q in zip(sites,actual):
            assert p not in result or result[p]==q
            result[p]=q
    assert store_sites<=result.keys()
    return result,owners,store_sites


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source',type=Path,default=ROOT/'src/source_damage_delivery_runtime_generated.cpp')
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();assert not args.output.exists()
    pcs,owners,store_sites=mapping(args.source)
    result=dict(passed=True,scope=__doc__,source=str(args.source.absolute()),source_sha256=file_sha(args.source),
        original_map_pack_sha256=PIN,original_tu15_sha256=NEW_PIN,whole_routines_authenticated=len(owners),
        static_instruction_sites=len(pcs),owners=owners,instruction_map={f'{p:08x}':q for p,q in sorted(pcs.items())},
        canonical_memory_store_pcs=sorted(store_sites),
        source_and_observer_records_modified=False,tool_sha256=file_sha(Path(__file__)),full_weapon_parity=False)
    args.output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({k:result[k] for k in ('passed','whole_routines_authenticated','static_instruction_sites')}))


if __name__=='__main__':main()
