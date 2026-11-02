#!/usr/bin/env python3
"""Compare captured native party callers with the actual Rust/CXX controller.

Worker, frame, member and voice observations are declared synthetic IO. This
qualifies modern caller/state equivalence, not original Live services, native
device internals, physical input or installed runtime delivery. Raw outputs
stay inside the enclosing run_tests.py/run_test.py scratch lifecycle.
"""
import argparse
import hashlib
import itertools
import json
import os
from pathlib import Path
import subprocess


def frame(screen=26, context=2, destination=31, value=0, **extra):
    return dict(screen=screen, context=context, instance=0xFFFFFFFE00000042,
                entries=[dict(destination=destination, value=value)], **extra)


def cases():
    rows = [dict(handle=[255, 0, 65], controller=-1), dict(handle=[254, 0, 66], controller=2)]
    incoming = [dict(id=[255, 0, 73], name="duplicate", version=2**64-1)]
    party = dict(leader=True, version=2**64-7,
                 privacy=dict(mode=0, revision=2**64-10, context=[255, 0, 67]),
                 outgoing=[dict(id=[254, 0, 79], name="duplicate")])
    for screen, context in [(14,0),(26,2),(216,0),(31,42),(31,45),(26,1),(186,0),(0x1000E,0)]:
        for destination, value, shape in itertools.product(
                [-82,-36,-37,-34,-33,-5,23,26,31,167,219],
                [-2,-1,0,1,2,3,5,6,7,8,9,10,999,1000,1001,2000,2001], range(3)):
            initial = dict(frame=frame(screen,context,destination,value), rows=rows,
                           party=None if shape==0 else party | dict(leader=shape==2),
                           invitations=incoming, friends=[dict(id=[255,0,70], state="friend")],
                           clan=[dict(id=[254,0,67])], voice_client=True, voice_party=shape==2,
                           joined=shape==2, muted=[[255,0,65]])
            yield dict(label=f"accept:{screen}:{context}:{destination}:{value}:{shape}",
                       initial=initial, operations=[dict(op="accept")])
    for controller in [-2,-1,0,1,2,3]:
        for fail in [-1,0,1,2,3,4]:
            for nonstd in [False,True]:
                yield dict(label=f"member:{controller}:{fail}:{nonstd}", nonstd=nonstd,
                           initial=dict(frame=frame(),rows=[dict(handle=[255,0],controller=controller)],fail=fail),
                           operations=[dict(op="accept")])
    for status, busy, launch, resource, withdraw, accept in itertools.product(
            [0,5,6,7,8,9,0x10006,2**32-1],[False,True],[False,True],[False,True],[False,True],[False,True]):
        yield dict(label=f"notice:{status}:{busy}:{launch}:{resource}:{withdraw}:{accept}",
                   initial=dict(party=None,invitations=incoming,status=status,busy=busy,launch=launch,
                                account_client=resource,withdraw_queue=withdraw,accept=accept),
                   operations=[dict(op="notice",accept=True),dict(op="notice",accept=False),dict(op="prune"),dict(op="clear")])
    for source in [(31,45),(31,42),(26,2)]:
        yield dict(label=f"tab:{source}",initial=dict(frame=frame(*source)),
                   operations=[dict(op="tab",direction=d) for d in [0,1,0,-1,-2**31,2**31-1]])
    for mode, leader, busy, resource, value, stale in itertools.product(
            [1,3],[False,True],[False,True],[False,True],[-1,0,1,2,3],[False,True]):
        operations=[dict(op="privacy_open")]
        if stale:operations.append(dict(op="set",value=dict(party=party | dict(privacy=party["privacy"] | dict(context="changed")))))
        operations.append(dict(op="privacy_select",value=value))
        yield dict(label=f"privacy-guard:{mode}:{leader}:{busy}:{resource}:{value}:{stale}",
                   initial=dict(mode=mode,party=party | dict(leader=leader),busy=busy,account_client=resource,
                                frame=frame(25,0,-2,value)),operations=operations)
    for successful, serial, revision, mode, current, changed, busy in itertools.product(
            [None,False,True],[2**64-16,2**64-17],[2**64-10,2**64-9],[0,1],[False,True],[False,True],[False,True]):
        ack_party=party | dict(privacy=dict(mode=mode,revision=revision,context="different" if changed else [255,0,67]))
        update=dict(successful=successful,serial=serial,party=ack_party,busy=busy,message=[255,0,69])
        if not current:update["frame"]=frame(25,0,-2,1,selected=0) | dict(instance=123)
        yield dict(label=f"privacy-ack:{successful}:{serial}:{revision}:{mode}:{current}:{changed}:{busy}",
                   initial=dict(party=party,frame=frame(25,0,-2,1)),
                   operations=[dict(op="privacy_open"),dict(op="privacy_select",value=1),dict(op="set",value=update),dict(op="privacy_observe")])
    for fail in range(-1,8):
        for nonstd in [False,True]:
            yield dict(label=f"privacy-fault:{fail}:{nonstd}",nonstd=nonstd,
                       initial=dict(party=party,frame=frame(25,0,-2,1),fail=fail),
                       operations=[dict(op="privacy_open"),dict(op="privacy_select",value=1)])
    for action, resource, accepted, screen, context in itertools.product(
            ["create","accept","remove",[255,0,65]],[False,True],[False,True],[6,14,26,31,186],[0,2,42]):
        yield dict(label=f"command:{action}:{resource}:{accepted}:{screen}:{context}",
                   initial=dict(account_client=resource,accept=accepted,frame=frame(screen,context)),
                   operations=[dict(op="command",value=dict(action=action,value=[255,0,86],playlist=[254,0],revision=[253],
                                   version=2**64-1,privacy=2**32-1,privacy_revision=2**64-2,
                                   privacy_context=[252,0],join_context=[251,0]))])
    for serial, mode in itertools.product([1,2**32,2**64-16],[0,1,2]):
        yield dict(label=f"native-reentry:{serial}:{mode}",
                   initial=dict(party=party,frame=frame(25,0,-2,mode),worker_serial=serial,
                                reenter_update=True,command_busy=True),
                   operations=[dict(op="privacy_open"),dict(op="privacy_select",value=mode),
                               dict(op="set",value=dict(serial=serial,successful=True,busy=False,
                                    party=party | dict(privacy=party["privacy"] | dict(mode=mode,revision=2**64-9)))),
                               dict(op="privacy_observe")])
    yield dict(label="native-state-copy-assignment-move-independence",
               initial=dict(party=None,invitations=incoming,frame=frame(31,45)),
               operations=[dict(op="notice",accept=False),dict(op="tab",direction=1),
                           dict(op="set",value=dict(party=party,frame=frame(25,0,-2,1))),
                           dict(op="privacy_open"),dict(op="privacy_select",value=1),
                           dict(op="set",value=dict(frame=frame(),rows=rows)),dict(op="accept"),dict(op="copy")])


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('native',type=Path)
    p.add_argument('--report',type=Path,required=True)
    args=p.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    inputs=list(cases())
    request=scratch/'party-flow-input.jsonl';output=scratch/'party-flow-output.jsonl'
    request.write_text(''.join(json.dumps(value,separators=(',',':'))+'\n' for value in inputs))
    with request.open('rb') as source,output.open('wb') as sink:
        run=subprocess.run([str(args.native.resolve())],stdin=source,stdout=sink,stderr=subprocess.PIPE)
    differences=[];observations=steps=faults=preserved=0
    actual=output.read_text().splitlines()
    framing=[];transport_error=None
    if actual[:1]==['Shutdown handler: initialize.'] and actual[-1:]==['Shutdown handler: cleanup.']:
        framing=[actual[0],actual[-1]];actual=actual[1:-1]
    for index,(case,line) in enumerate(zip(inputs,actual)):
        try:
            value=json.loads(line);old=value['before'];rust=value['rust']
            if len(old)!=len(case['operations']) or len(rust)!=len(case['operations']):
                raise ValueError('Native replay omitted an operation')
        except (ValueError,KeyError,TypeError) as error:
            transport_error=dict(index=index,error=str(error),line=line[:300]);break
        steps+=len(old)
        if old!=rust and len(differences)<8:differences.append(dict(index=index,label=case['label'],case=case,before=old,rust=rust))
        if old:observations+=len(old[-1]['native']['calls'])
        for before,after in zip(old,rust):
            if before.get('error') in ('native','int'):
                faults+=1
                preserved+=after.get('error')==before['error'] and (after.get('same_object') is True if before['error']=='native' else after.get('value')==before.get('value'))
    passed=run.returncode==0 and len(actual)==len(inputs) and not differences and faults==preserved and transport_error is None
    report=dict(passed=passed,cases=len(inputs),state_observations=steps,native_io_observations=observations,
                fault_observations=faults,preserved_native_exception_identity=preserved,differences=differences,
                native_exit_code=run.returncode,native_output_lines=len(actual),stderr=run.stderr.decode(errors='replace')[-4000:],
                native_sha256=hashlib.sha256(args.native.read_bytes()).hexdigest(),scope=__doc__,
                admitted_native_framing=framing,transport_error=transport_error)
    args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items() if k not in ('differences','scope')}))
    if differences:print(json.dumps(differences[0])[:8000])
    return 0 if passed else 1


if __name__=='__main__':
    raise SystemExit(main())
