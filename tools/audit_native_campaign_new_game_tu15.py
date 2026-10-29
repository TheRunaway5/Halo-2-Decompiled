#!/usr/bin/env python3
"""Read actual MCC opening scripts and original TU1.5 lifecycle owner graph."""
import argparse,hashlib,json,os,struct
from pathlib import Path
from capstone.x86 import X86_OP_IMM
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from export_campaign_hs_function_registry_tu15 import registry
from mcc_content import MccCache
NAMES={'map_name','game_load','map_reset','game_won','game_revert','game_save','game_save_cancel','game_save_unsafe','game_save_cinematic_skip','game_saving'}
def sha(raw):return hashlib.sha256(raw).hexdigest()
def opening(path):
    cache=MccCache(path);scenario=[t for t in cache.tags.values()if t['group']=='scnr'];assert len(scenario)==1
    scenario=scenario[0];at=scenario['offset'];script_records=cache.block(at+0x1b8,0x28);nodes=cache.block(at+0x238,20);strings=cache.block(at+0x1b0,1)
    def node(datum):
        assert datum!=0xffffffff and datum&0xffff<len(nodes)
        row=struct.unpack_from('<4H3I',cache.data,nodes[datum&0xffff]);assert row[0]==datum>>16
        return row
    def text(offset):assert 0<=offset<len(strings);return cache.string(strings[0]+offset,len(strings)-offset)
    scripts=[dict(index=i,name=cache.string(p,32),kind=cache.u16(p+32),root=cache.u32(p+36))for i,p in enumerate(script_records)]
    calls=[]
    for i,p in enumerate(nodes):
        n=struct.unpack_from('<4H3I',cache.data,p)
        if n[3]&(1|4) or n[6]==0xffffffff:continue
        try:function=node(n[6]);name=text(function[5])
        except (AssertionError,IndexError,ValueError):continue
        if name not in NAMES:continue
        arguments=[];current=function[4];seen=set()
        while current!=0xffffffff:
            assert current not in seen;seen.add(current);a=node(current)
            value=dict(datum=hex(current),opcode=a[1],type=a[2],flags=a[3],value=hex(a[6]))
            if a[3]&1 and a[1] in (2,9):value['text']=text(a[5])
            arguments.append(value);current=a[4]
        calls.append(dict(datum=hex((n[0]<<16)|i),name=name,opcode=n[1],type=n[2],flags=n[3],arguments=arguments,authored_owner_scripts=[]))
    targets={int(c['datum'],16):c for c in calls}
    for script in scripts:
        pending=[script['root']];seen=set()
        while pending:
            datum=pending.pop()
            if datum==0xffffffff or datum in seen:continue
            seen.add(datum);n=node(datum)
            if datum in targets:targets[datum]['authored_owner_scripts'].append(script['name'])
            if n[4]!=0xffffffff:pending.append(n[4])
            if not n[3]&(1|4) and n[6]!=0xffffffff:pending.append(n[6])
            if n[3]&2 and not n[3]&1 and n[1]<len(scripts):pending.append(scripts[n[1]]['root'])
    return dict(path=str(path),file_bytes=path.stat().st_size,file_sha256=sha(path.read_bytes()),scenario=scenario['name'],script_count=len(scripts),node_count=len(nodes),strings_sha256=sha(cache.data[strings[0]:strings[-1]+1]),nodes_sha256=sha(b''.join(cache.data[p:p+20]for p in nodes)),startup_scripts=[s for s in scripts if s['kind']==0],lifecycle_calls=calls,lifecycle_counts={name:sum(c['name']==name for c in calls)for name in sorted(NAMES)},scope='Authored static closure, not executed progress or timeline acceptance')
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--maps',type=Path,required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args();assert Path(os.environ['HALO2_TEST_SCRATCH']).is_dir()
    image=Xbe(NEW_DEFAULT);assert sha(image.data)==NEW_PIN;catalogue,_,_=registry(image);profile=json.loads((ROOT/'tools/mcc-script-layout.json').read_text());selected={r['name']:catalogue[r['xbox_opcode']]for r in profile['builtins']if r['name']in NAMES and r['xbox_opcode']is not None};code=_Code(image,'.text');entries={r['evaluator']for r in catalogue if r['evaluator']};owners={};queue=[(r['evaluator'],0)for r in selected.values()]
    while queue:
        entry,depth=queue.pop(0)
        if entry in owners:continue
        body=_trace(code,entry,entries);listing=[];calls=[]
        for pc in sorted(body.code):
            ins=code.at(pc);listing.append(dict(pc=hex(pc),bytes=ins.bytes.hex(),operation=ins.mnemonic,operands=ins.op_str))
            if ins.mnemonic=='call':
                target=ins.operands[0].imm if ins.operands[0].type==X86_OP_IMM else None
                calls.append(dict(site=hex(pc),target=hex(target)if target is not None else ins.op_str))
                # Existing native ScriptVm owns argument evaluation/completion;
                # do not treat the original HS CPU evaluator as a new runtime.
                if target is not None and depth<1 and target not in (0x211da0,):queue.append((target,depth+1))
        owners[hex(entry)]=dict(depth=depth,sites=len(listing),calls=calls,listing=listing,listing_sha256=sha(json.dumps(listing,sort_keys=True).encode()))
    report=dict(passed=True,original_sha256=NEW_PIN,mcc_profile_sha256=sha((ROOT/'tools/mcc-script-layout.json').read_bytes()),builtin_definitions=selected,absent_admitted_builtins=sorted(NAMES-set(selected)),owners=owners,maps=[opening(a.maps/(name+'.map'))for name in ('00a_introduction','01a_tutorial')],runtime_progress_qualified=False,original_runtime_execution=False,retained_large_artifacts=[])
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(owners={k:dict(sites=v['sites'],calls=v['calls'])for k,v in owners.items()},maps=[dict(path=r['path'],lifecycle_counts=r['lifecycle_counts'])for r in report['maps']]),indent=2))
if __name__=='__main__':main()
