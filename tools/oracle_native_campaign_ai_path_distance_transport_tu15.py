#!/usr/bin/env python3
"""Real TU1.5 CALL/RET path-distance packets; no supplied outcome/frame.

Each actual CALL creates its own return word. Retained actor and geometry
inputs come from the unchanged-original component matrix. Rejections stop
before a source read; no interrupted process is resumed with a supplied value.
"""
import argparse,copy,gzip,json,os,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_ai_area_tu15 as areas
import oracle_native_campaign_ai_path_distance_tu15 as distance
import oracle_native_campaign_screen_lifecycle_tu15 as common
import validate_native_geometry_tu15 as geometry

CALLERS=((0x1b88b0,0x1b8938),(0x1bbee0,0x1bbf3b))


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',type=Path)
    args=p.parse_args();assert not args.output.exists();assert common.sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');common.ENTRIES=distance.ENTRIES+tuple(root for root,site in CALLERS)
    metadata=common.metadata(image);metadata['private_stack']=dict(begin=geometry.STACK,bytes=65536)
    for site in metadata['sites']:
        if code.at(site['instruction']).mnemonic in ('cmp','test'):
            for operand in site['operands']:operand.update(read=True,write=False)
        if site['instruction']==0x200d2a:
            site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
    declared=[]
    for root,site in CALLERS:
        assert code.at(site).mnemonic=='call' and code.at(site).op_str=='0x200c00'
        sources=distance.inputs(image)
        seed=next(row for row in sources if row['name']=='fault_seed_0')
        for label,value in(('unit',0x3f800000),('zero',0),('snan',0x7f812345)):
            extra=copy.deepcopy(seed);extra['name']='fault_seed_scale_'+label
            at=distance.BODY+0x200
            bank=next(bank for bank in extra['banks']if bank['address']<=at and at+4<=bank['address']+len(bank['before'])//2)
            raw=bytearray.fromhex(bank['before']);offset=at-bank['address'];raw[offset:offset+4]=value.to_bytes(4,'little');bank['before']=raw.hex();sources.append(extra)
        for source in sources:
            row=copy.deepcopy(source);row.update(id=len(declared),name=f'{site:x}_'+source['name'],
                entry=site,caller_call=True,stop_address=site+5,call_site=site,caller_root=root)
            row['banks'].append(dict(address=row['gp'][4]-4,before='5b5b5b5b'))
            row['banks'].sort(key=lambda bank:bank['address']);declared.append(row)
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);owned=areas.prepare(image,metadata,scratch)
    result=subprocess.run(['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_ai_path_distance_transport_tu15.cpp'),'-o',str(owned/'observer')],capture_output=True,text=True)
    assert result.returncode==0,result.stderr
    complete=geometry.physical(owned,declared,'complete');prefix=[]
    for seed in(r for r in complete if '_fault_seed_'in r['name']):
        for ordinal in range(len(seed['reads'])):
            row=copy.deepcopy(seed);row.update(id=len(complete)+len(prefix),name=seed['name']+'_before_read_'+str(ordinal),fault=dict(phase='before_read',ordinal=ordinal));prefix.append(row)
    rows=complete+geometry.physical(owned,prefix,'prefix')
    assert all(not row['writes']and row['callee_entry']is not None for row in rows)
    fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=metadata['owners'],instruction_patches=[],supplied_original_outcomes=False,cases=rows)
    data=gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0)
    if args.verify:assert data==args.verify.read_bytes(),'Physical path-distance transport repeat differs'
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_bytes(data)
    report=dict(passed=True,original_sha256=NEW_PIN,cases=len(rows),complete_calls=sum(r['completed']for r in complete),
        real_faults=sum(not r['completed']for r in complete),fault_prefixes=len(prefix),
        late_return_prefixes=sum(r['boundary']['instruction']==0x200d2a for r in rows if r['boundary']),
        pending_results=sum(bool(r['pending_extended'])for r in rows),ordered_reads=sum(len(r['reads'])for r in rows),
        caller_sites=[hex(site)for root,site in CALLERS],strict_retained_operands=True,
        fixture=str(args.output),fixture_bytes=len(data),fixture_sha256=common.sha(args.output),
        independent_repeat_identical=True if args.verify else None,native_binding_qualified=False,actual_world_route_qualified=False,
        source_sha256={str(path.relative_to(ROOT)):common.sha(path)for path in (Path(__file__),ROOT/'tools/observe_native_campaign_ai_path_distance_transport_tu15.cpp',ROOT/'tools/oracle_native_campaign_ai_path_distance_tu15.py')},
        owned_scratch=str(scratch),retained_large_artifacts=[])
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({key:report[key]for key in ('cases','complete_calls','fault_prefixes','late_return_prefixes','pending_results')}))


if __name__=='__main__':main()
