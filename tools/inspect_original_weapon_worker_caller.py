#!/usr/bin/env python3
"""Identify original main-thread request callers before the first kernel wait."""
import argparse
import json
import os
from pathlib import Path
import shutil
from original_xbox_pose_case import sha
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for key in ('oracle','output'):parser.add_argument('--'+key,type=Path,required=True)
    args=parser.parse_args();assert 'HALO2_TEST_SCRATCH'in os.environ
    output=args.output.absolute();output.mkdir(exist_ok=False)
    (output/'attempt.json').write_text(json.dumps(dict(runner_scratch=os.environ['HALO2_TEST_SCRATCH']),indent=2)+'\n')
    report=json.loads((args.oracle/'report.json').read_text());assert report['exit']==0x80020f5a
    assert sha(NEW_DEFAULT)==NEW_PIN
    trace=json.loads((args.oracle/'instructions.json').read_text());code=_Code(Xbe(NEW_DEFAULT),'.text')
    calls=[];stack=[];unmatched=[]
    for index,pc in enumerate(trace):
        operation=code.at(pc);following=trace[index+1]if index+1<len(trace)else report['exit']
        if operation.mnemonic=='call':
            call=dict(site=pc,target=following,return_address=pc+operation.size)
            if following in (0x1a3b70,0x1aab20,0x123270,0x2e037f):calls.append(dict(**call,calling_trace=list(stack)))
            stack.append(call)
        elif operation.mnemonic=='ret':
            matches=[i for i,frame in enumerate(stack)if frame['return_address']==following]
            if matches:del stack[matches[-1]:]
            else:unmatched.append(dict(instruction=pc,next=following))
    resource=[r for r in calls if r['target']==0x1a3b70]
    result=dict(passed=not unmatched,scope=__doc__,oracle_sha256=sha(args.oracle/'report.json'),
        original_XBE_sha256=NEW_PIN,request_calls=resource,selected_nested_calls=calls,
        active_call_chain_at_kernel_boundary=stack,unmatched_returns=unmatched,
        original_XBE_bytes_unchanged=True,native_world_resource_handlers=[0x1301b0,0x130f68,0x126139],
        direct_request_sites_covered_by_those_handlers=[r['site']for r in resource if r['site']in (0x1301b0,0x130f68,0x126139)],
        runner_scratch=os.environ['HALO2_TEST_SCRATCH'],full_weapon_parity=False,tool_sha256=sha(Path(__file__)))
    (output/'report.json').write_text(json.dumps(result,indent=2)+'\n');shutil.copyfile(Path(__file__),output/Path(__file__).name)
    print(json.dumps(dict(passed=result['passed'],request_sites=[hex(r['site'])for r in resource],
        active_call_chain=[(hex(r['site']),hex(r['target']))for r in stack],unmatched_returns=unmatched)),flush=True)


if __name__=='__main__':main()
