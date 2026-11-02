#!/usr/bin/env python3
"""Original EP input acquisition over the qualified original GP output values.

Original CPU upload/descriptor bodies acquire the EP loader and module5 anew.
Original commands10/6, module304, five7 reads, two8 calls,15 diagnostics and11
half toggles execute. The caller context and synchronous DMA model are declared;
this is no boot, physical format, hardware clock or endpoint acceptance.
"""
import argparse,gzip,hashlib,importlib.util,json,os,shutil,struct,subprocess
from pathlib import Path


def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);root=Path.cwd()
 if a.output.exists():p.error('Preserve original EP input results')
 base=root/'analysis/asset-setup/mcc-native-20261015';stage=base/'native-sound-ep-stereo-algorithm-recovery-v1';profile=base/'native-sound-ep-conditional-ALU-proposal-v1'
 spec=importlib.util.spec_from_file_location('ep_input_acquisition',stage/'acquire-inputs.py');module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
 seed,acquisition=module.acquire(0)
 oracle=scratch/'oracle';core=scratch/'core';shutil.copytree(profile/'oracle',oracle);shutil.copytree(profile/'reference-core-v5',core)
 sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest();pins={str(path.relative_to(profile)):sha(path)for folder in ('oracle','reference-core-v5')for path in (profile/folder).rglob('*')if path.is_file()}
 source=oracle/'ep-input-reference.c';text=(stage/'loader-module-reference.c').read_text();before=hashlib.sha256(text.encode()).hexdigest()
 marker='static unsigned numeric_entry_status;';assert text.count(marker)==1
 text=text.replace(marker,'static uint64_t acquired_hash;static unsigned input_dma_count,input_dma_address[5],input_dma_bytes[5];\n'+marker)
 old='if(pc==0x32e)numeric_entry_status=current->core.registers[DSP_REG_SR];';assert text.count(old)==1
 text=text.replace(old,'''if(pc==0x32e){numeric_entry_status=current->core.registers[DSP_REG_SR];acquired_hash=UINT64_C(14695981039346656037);
 for(unsigned i=0;i<5*256;i++){uint32_t w=frame_peek(current,DSP_SPACE_X,i);for(unsigned b=0;b<4;b++){acquired_hash^=(w>>(b*8))&255;acquired_hash*=UINT64_C(1099511628211);}}}''')
 old='if(d)memcpy(scratch_bytes+a,p,n);else memcpy(p,scratch_bytes+a,n);';assert text.count(old)==1
 text=text.replace(old,'''if(!d&&a>=0x10000){require(input_dma_count<5,"more than five source input reads");input_dma_address[input_dma_count]=a;input_dma_bytes[input_dma_count++]=n;}
 '''+old)
 old='fifo_calls=input_calls=output_calls=diagnostic_calls=0;';assert text.count(old)==1;text=text.replace(old,old+'input_dma_count=0;acquired_hash=0;')
 old='printf("],\\\"numeric_entry_SR\\\":%u,\\\"fifo_hex\\\":\\\"",numeric_entry_status);';assert text.count(old)==1,old
 text=text.replace(old,'''require(input_dma_count==5,"missing real input DMA requests");
 printf("],\\\"input_hash\\\":\\\"%016"PRIx64"\\\",\\\"input_DMA\\\":[",acquired_hash);
 for(unsigned j=0;j<5;j++)printf("%s[%u,%u]",j?",":"",input_dma_address[j],input_dma_bytes[j]);
 '''+old)
 source.write_text(text)
 argv=['cc','-std=c11','-O2','-fno-strict-aliasing','-I'+str(oracle),'-I'+str(core),str(source),str(core/'dsp.c'),str(core/'dsp_cpu.c'),str(core/'dsp_dma.c'),'-o',str(scratch/'original-ep-input')]
 built=subprocess.run(argv,capture_output=True);assert built.returncode==0,built.stderr.decode()[-6000:]
 vectors=root/'tests/data/native_tu15_sound_gp_output_values.bin.gz';assert sha(vectors)=='608c66769b5cdc75badaa98bcd4edc54445f33191d8a03668243ae34b1fc67f6'
 raw=gzip.decompress(vectors.read_bytes());assert struct.unpack_from('<2I',raw)==(0x47504f31,36)
 stride=1+6+3712+1024;assert len(raw)==8+36*stride*4;cases=[]
 for record in range(36):
  words=struct.unpack_from('<'+str(stride)+'I',raw,8+record*stride*4)
  if not words[0]:continue
  index=len(cases);cases.append(dict(record=record,entry_status=(index*37)&255,scale_enabled=index%2,scale=[0x7fffff,0x400000,0,0x800000,0xffffff,0x5a8279][index%6],rings=list(words[7:7+6*512])))
 assert len(cases)==33
 payload=b'EPLOAD01'+seed+struct.pack('<2I',acquisition['source_config_byte_offset'],len(cases))+b''.join(struct.pack('<3075I',row['entry_status'],row['scale_enabled'],row['scale'],*row['rings'])for row in cases)
 run=subprocess.run([str(scratch/'original-ep-input')],input=payload,capture_output=True,timeout=120)
 assert run.returncode==0,(run.returncode,run.stderr.decode()[-6000:],run.stdout[-1000:].decode())
 rows=[json.loads(line)for line in run.stdout.splitlines()];assert len(rows)==66
 for row in rows:
  source_case=cases[row['case']];phase=row['half_phase'];assert phase in(0,256)
  h=0xcbf29ce484222325
  for channel in range(5):
   for word in source_case['rings'][channel*512+phase:channel*512+phase+256]:
    for b in range(4):h=((h^((word>>(8*b))&255))*0x100000001b3)&0xffffffffffffffff
  assert row['input_hash']==f'{h:016x}',('original channel/half mapping',row['case'],phase)
  assert row['input_DMA']==[[0x10000+j*2048+phase*4,1024]for j in range(5)],row['input_DMA']
  row.update(record=source_case['record'],entry_status=source_case['entry_status'],scale_enabled=source_case['scale_enabled'],scale=source_case['scale'])
 result=dict(scope=__doc__,original_xbe_sha256=acquisition['original_xbe_sha256'],CPU_acquisition=acquisition,GP_output_fixture_sha256=sha(vectors),GP_output_fixture_raw_sha256=hashlib.sha256(raw).hexdigest(),
  source_ring_order=[0,2,1,4,5,3],input_channels=5,input_words_per_channel=256,ring_words_per_channel=512,original_full_module_returns=66,original_input_DMA_requests=330,source_half_toggles=66,
  cases=rows,reference_pins=pins,reference_wrapper_before_sha256=before,reference_wrapper_after_sha256=sha(source),compile_command=argv,tool_sha256=sha(Path(__file__)),
  original_program_patches=[],supplied_callee_or_clock_results=[],declared_post_initialize_source_context=True,physical_DMA_format_or_feedback_or_clock_verified=False,retained_large_artifacts=[])
 data=json.dumps(result,separators=(',',':'))+'\n';assert len(data)<1024*1024,len(data);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(data)
 print(json.dumps(dict(original_GP_outputs=33,original_EP_returns=len(rows),original_input_DMA_requests=330,bytes=len(data))))


if __name__=='__main__':main()
