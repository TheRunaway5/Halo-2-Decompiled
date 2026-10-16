#!/usr/bin/env python3
"""Validate retained typed records8/9 from compact original numeric boundaries.

No original executable, program, processor core or runtime state bank is used.
Two real State owners and one544-word Workspace persist across each case.
Source input/prior-output words remain explicit numeric boundary fixtures.
"""
from pathlib import Path
import argparse,gzip,hashlib,json,struct,subprocess
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--probe',type=Path,required=True)
parser.add_argument('--vectors',type=Path,required=True)
parser.add_argument('--report',type=Path,required=True)
args=parser.parse_args()
def sha(data):return hashlib.sha256(data).hexdigest()
def pack(words):return struct.pack('<'+str(len(words))+'I',*words)
def require(ok,message):
    if not ok:raise RuntimeError(message)
compressed=args.vectors.read_bytes();vectors=json.loads(gzip.decompress(compressed))
require(vectors['schema']=='H2_TU15_RING_EFFECT_RETAINED_NUMERIC_VECTORS_V1','unsupported numeric vectors')
require(vectors['calls']==882 and vectors['arithmetic_values']==871392 and vectors['frames']==441,'incomplete source matrix')
sizes=vectors['ring_word_counts'];require(sizes==[6000,6000,6000,6000,680,612,640,720,1392,1684,2044,2476,24000,24000],'incorrect history owners')
prefix_words=554+544+32+128+sum(sizes)
rows=[];calls=0;total_arithmetic=0
for case in vectors['cases']:
    scratch=bytearray(0xc9000);seen_frames=set();process=subprocess.Popen([str(args.probe.resolve())],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    def exact(count):
        data=process.stdout.read(count)
        require(len(data)==count,'truncated retained native output: '+case['name'])
        return data
    case_calls=0;case_arithmetic=0
    try:
        for call in case['calls']:
            record=call['record'];frame=call['frame'];begin=call['begin'];end=call['end']
            require(record in(8,9)and len(begin['state_words'])==len(end['state_words'])==554,'incorrect source state boundary')
            stamps=[record]
            if frame not in seen_frames:stamps.insert(0,0);seen_frames.add(frame)
            for stamp,space,address,value in case['raw_fixture_frames'][frame]:
                if stamp in stamps and space==2:struct.pack_into('<I',scratch,address,value)
            base=begin['scratch_base'];extent=begin['scratch_bytes']
            require(sha(scratch[base:base+extent])==begin['scratch_sha256'],'source begin history mismatch')
            rings=list(struct.unpack('<'+str(sum(sizes))+'I',scratch[base:base+sum(sizes)*4]))
            patches=call['state_patch_offsets'];history=call['history_patch_positions']
            values=[0x22d if record==8 else 0x457,len(patches)]+patches+[len(history)]+sum(history,[])
            values += [begin['sr']]+[0xffffff]*8+begin['state_words']+begin['workspace']+begin['input']+sum(begin['outputs'],[])+rings
            process.stdin.write(pack(values));process.stdin.flush()
            actual=list(struct.unpack('<'+str(prefix_words)+'I',exact(prefix_words*4)))
            count=struct.unpack('<I',exact(4))[0];require(count<=1024,'native arithmetic trace overflow')
            trace=list(struct.unpack('<'+str(count)+'I',exact(count*4)))
            cursor=0
            for name,expected in [('state_words',end['state_words']),('workspace',end['workspace']),('input',end['input']),('outputs',sum(end['outputs'],[]))]:
                require(actual[cursor:cursor+len(expected)]==expected,f'{case["name"]} record{record} frame{frame}: {name} differs')
                cursor+=len(expected)
            for node in call['dma']:
                node_base=node['node_words'][5];node_extent=node['node_words'][6]+1;offset=node['byte_offset']
                positions=[node_base+(offset-node_base+j*4)%node_extent for j in range(node['bytes']//4)]
                if node['direction']=='dsp_to_scratch':
                    content=pack(node['raw_history_words']);require(len(content)==node['bytes'],'incorrect source DMA write size')
                    require(sha(content)==node['transfer_sha256'],'incorrect source DMA write hash')
                    for j,position in enumerate(positions):scratch[position:position+4]=content[j*4:j*4+4]
                else:
                    require(node['direction']=='scratch_to_dsp','unsupported source DMA direction')
                    content=b''.join(scratch[position:position+4]for position in positions)
                    require(sha(content)==node['transfer_sha256'],'source DMA read history mismatch')
            require(sha(scratch[base:base+extent])==end['scratch_sha256'],'source end history mismatch')
            ring_offset=base
            for index,length in enumerate(sizes):
                content=pack(actual[cursor:cursor+length]);expected=scratch[ring_offset:ring_offset+length*4]
                require(content==expected,f'{case["name"]} record{record} frame{frame}: retained ring{index} differs')
                cursor+=length;ring_offset+=length*4
            require(cursor==prefix_words,'incorrect native output layout')
            require(trace==call['arithmetic'],f'{case["name"]} record{record} frame{frame}: ordered arithmetic differs')
            case_calls+=1;case_arithmetic+=count
        process.stdin.close();process.wait(timeout=30)
        require(process.returncode==0 and not process.stderr.read()and not process.stdout.read(),'retained native probe failed or has trailing output')
    except BaseException:
        if process.poll()is None:process.kill()
        process.wait();raise
    calls+=case_calls;total_arithmetic+=case_arithmetic
    rows.append(dict(case=case['name'],calls=case_calls,arithmetic_values=case_arithmetic,passed=True))
require(calls==882 and total_arithmetic==871392 and len(rows)==vectors['case_count'],'incomplete retained native replay')
report=dict(passed=True,frames=441,calls=calls,cases=len(rows),arithmetic_values=total_arithmetic,
            source_words_compared_per_call=554,shared_workspace_words=544,input_words_per_call=32,output_words_per_call=128,
            full_ring_words_compared_per_call=sum(sizes),real_retained_State_owners=2,shared_workspace_refilled_between_calls=False,
            native_allocations_stable_above_4GiB=True,source_inputs_and_prior_outputs_explicit_numeric_boundaries=True,
            runtime_original_program_or_core_loaded=False,source_voice_or_PCM_producer_authenticated=False,
            whole_DSP_ready=False,initializer45_complete=False,physical_audio_parity=False,
            fixture_sha256=sha(compressed),probe_sha256=sha(args.probe.read_bytes()),rows=rows)
args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({key:value for key,value in report.items()if key!='rows'}))
