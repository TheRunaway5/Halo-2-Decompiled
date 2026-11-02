#!/usr/bin/env python3
"""Stream exact modern C++/Rust caller comparisons; no played-world claims."""
import argparse
from collections import Counter
import hashlib
import json
import os
from pathlib import Path
import subprocess


def corpus():
    for operation in ['publish','validate']:
        for teams in [False,True]:
            base=dict(operation=operation,teams=teams)
            yield base
            for member_count in [0,1,2,4,8,16,17]:
                yield base|dict(roster=[dict(slot=i,team=i%2) for i in range(member_count)])
            for slot in [0,3,15,16,65535,2**32-1]:
                yield base|dict(roster=[dict(slot=slot,team=0)])
            yield base|dict(roster=[dict(slot=0,team=0),dict(slot=0,team=1)])
            for key,values in [('finished',[False,True]),('finalized',[False,True]),('countdown',[-1,0,1,300]),('accuracy',[False,True]),('retained',[0,1,2,3,7,128,255])]:
                for value in values:yield base|dict(state={key:value})
            for flags in [0,1,0x20,0x40,2**32-1]:yield base|dict(round_flags=flags)
            for key,values in [('statistics',[False,True]),('enabled',[0,1,2,255]),('frozen',[0,1,2,255])]:
                for value in values:yield base|dict(recording={key:value})
            for changing in [False,True]:
                for membership in [-128,-1,0,1,7,8,127]:
                    yield base|dict(changing=changing,players={'3':dict(membership=membership)})
                for team in [-128,-1,0,7,8,127]:
                    yield base|dict(changing=changing,roster=[dict(slot=3,team=team)])
            for key,values in [('connected',[False,True]),('live_connected',[False,True]),('profile',[False,True]),('datum',[0,3,0x12340003,0xffff0003,2**32-1])]:
                for value in values:yield base|dict(players={'3':{key:value}})
            for key in ['kills','deaths','assists','suicides']:
                for value in [-32768,-1,0,1,32767,32768,65535]:yield base|dict(players={'3':{key:value}})
            for score in [-32768,-1,0,1,32767]:yield base|dict(players={'3':dict(score=score)})
            for offset,value in [(0x44,1),(0x44,9),(0x48,0),(0x48,1),(0x48,0x10),(0x48,0x40),(303,255)]:yield base|dict(variant=[[offset,value]])
            if operation=='validate':
                for size in [0,1,0xdc77,0xdc78,0xdc79]:yield base|dict(raw_size=size)
                offsets={0,1,0x10,0x11,0x13,*range(0x20,0x150)}
                for slot in range(16):
                    at=0x370+slot*0x94;offsets.add(at)
                    if slot in (0,3,15):
                        offsets.update(range(at+2,at+10));offsets.update(range(at+12,at+76));offsets.update([at+0x88,at+0x90,at+0x92,at+0x93])
                        for stat in [7,9,8,11]:offsets.update([0xe30+2*(slot*437+stat),0xe30+2*(slot*437+stat)+1])
                        for weapon in range(34):
                            for kind in range(2):
                                address=0xecc+2*(slot*437+(weapon+5)*8+4+kind);offsets.update([address,address+1])
                for team in range(16):
                    at=0xcb0+team*0x18;offsets.add(at)
                    if teams and team<2:offsets.update([at+1,at+2,at+3])
                for offset in sorted(offsets):
                    for bit in [1,128]:yield base|dict(corrupt=[[offset,bit]])
                for slot in (0,3,15):
                    for weapon in [0,17,33]:
                        for kind in (0,1):
                            for value in [0,32767,32768,65535]:yield base|dict(accuracy=[[slot,weapon,kind,value]])


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--binary',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    assert os.environ.get('HALO2_TEST_SCRATCH'),'Use owned test storage'
    report={'passed':False,'scope':'Exact captured modern C++ result caller policy versus Rust over declared gameplay/profile/rank/writer IO; not original arbitration or played-world proof',
            'commands':0,'accepted':0,'rejected':0,'io_refusals':0,'native_exception_identity_checks':0,'mismatches':[]}
    process=None;digest=hashlib.sha256();operations=Counter();preamble=[];seeds=[]
    try:
        process=subprocess.Popen([str(args.binary)],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,bufsize=1)
        def compare(command):
            line=json.dumps(command,sort_keys=True,separators=(',',':'))
            process.stdin.write(line+'\n');process.stdin.flush()
            while True:
                answer=process.stdout.readline()
                assert answer,'Native result fixture exited before its response'
                if answer.startswith('{'):break
                assert len(preamble)<8,'Unexpected native output before JSON';preamble.append(answer.strip())
            parsed=json.loads(answer);assert 'fixture_error' not in parsed,parsed
            report['commands']+=1;operations[command.get('operation','validate')]+=1
            digest.update(line.encode());digest.update(b'\n');digest.update(json.dumps(parsed,sort_keys=True,separators=(',',':')).encode());digest.update(b'\n')
            old,new=parsed['cpp'],parsed['rust']
            if old!=new and len(report['mismatches'])<10:report['mismatches'].append(dict(index=report['commands'],input=command,cpp=old,rust=new))
            if old['ok']:report['accepted']+=1
            else:report['rejected']+=1
            if old['category']=='io':
                report['io_refusals']+=1
                assert old['native_exception_identity'] and new['native_exception_identity'],'Native exception identity was lost'
                report['native_exception_identity_checks']+=1
            return old
        for command in corpus():
            result=compare(command)
            if not command.keys()-{'operation','teams'}:seeds.append((command,result))
        for command,result in seeds:
            for event in dict.fromkeys(result['trace']):compare(command|dict(fault=event))
        process.stdin.close();assert process.wait(timeout=10)==0
        report['sha256']=digest.hexdigest();report['operations']=dict(operations);report['native_preamble']=preamble
        report['binary_sha256']=hashlib.sha256(args.binary.read_bytes()).hexdigest()
        assert not report['mismatches'],f'Caller comparison mismatch; see {args.output}'
        assert report['commands']>4000 and report['io_refusals']>30
        report['passed']=True
    finally:
        if process is not None and process.poll() is None:
            process.terminate()
            try:process.wait(timeout=3)
            except subprocess.TimeoutExpired:process.kill();process.wait()
        args.output.parent.mkdir(parents=True,exist_ok=True)
        data=json.dumps(report,indent=2).encode();assert len(data)<=1048576;args.output.write_bytes(data+b'\n')
    print('PASS exact C++/Rust native result callers:',report['commands'],'commands;',report['native_exception_identity_checks'],'native exception identities;',report['sha256'])


if __name__=='__main__':main()
