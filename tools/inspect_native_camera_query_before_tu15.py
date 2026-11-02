#!/usr/bin/env python3
"""Authenticate typed camera-query BEFORE banks and inspect current DATA only."""
import argparse,bisect,hashlib,json,lzma,os,subprocess,tarfile
from pathlib import Path

def main():
 p=argparse.ArgumentParser();p.add_argument('--archive',required=True,type=Path);p.add_argument('--sha256',required=True);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 assert hashlib.file_digest(a.archive.open('rb'),'sha256').hexdigest()==a.sha256
 capture=json.loads(a.archive.with_name('capture-native-before.json').read_text());assert capture['schema']=='native-tu15-camera-query-typed-before-v1'and not capture['physics_result_supplied']and not capture['guest_abi_fabricated'];wanted={r['before_file']:r for r in capture['regions']};manifest=json.loads(lzma.decompress(a.archive.with_name('all-file-hashes.json.xz').read_bytes()));banks={};proc=subprocess.Popen(['zstd','-dc',str(a.archive)],stdout=subprocess.PIPE)
 with tarfile.open(fileobj=proc.stdout,mode='r|')as tar:
  for member in tar:
   name=str(Path(member.name));name=name[2:]if name.startswith('./')else name
   if name not in wanted:continue
   raw=tar.extractfile(member).read();row=wanted[name];proof=manifest.get(name,manifest.get('./'+name));assert len(raw)==row['size']and hashlib.sha256(raw).hexdigest()==proof['sha256'];banks[row['address']]=raw
 assert proc.wait()==0 and len(banks)==len(wanted);starts=sorted(banks)
 def read(p,n=4):
  i=bisect.bisect_right(starts,p)-1;assert i>=0;(first,raw)=(starts[i],banks[starts[i]]);assert p+n<=first+len(raw),(hex(p),n);return raw[p-first:p-first+n]
 def word(p,n=4):return int.from_bytes(read(p,n),'little')
 typed=capture['input'];descriptor=word(0x4ecc10);bsp=word(0x4ecc18);bitmap_root=word(0x4f9b50);slot=word(0x474c24,2);slot_signed=slot if slot<32768 else slot-65536
 cluster=46;cluster_record=word(bsp+0xa0)+cluster*0xb0
 output=dict(actual_first_source_cluster=cluster,cluster_record=cluster_record,cluster_instanced_count=word(cluster_record+0xa8),cluster_instanced_records=word(cluster_record+0xac),passed=True,archive=str(a.archive),archive_sha256=a.sha256,actual_input=typed,banks=len(banks),bank_bytes=sum(map(len,banks.values())),bsp_descriptor=descriptor,bsp_global=bsp,bitmap=bitmap_root+slot_signed*32+1,slot=slot,bsp_fields={hex(n):word(descriptor+n)for n in(0,4,8,12,16,20,24,28,32,36,40,44,48,52,56,60)},origin_words=[word(typed['origin']+n*4)for n in range(3)],displacement_words=[word(typed['displacement']+n*4)for n in range(3)],initial_bsp_node=read(word(descriptor+4),8).hex(),raw_or_binary_outputs_retained=[],owned_scratch=os.environ['HALO2_TEST_SCRATCH'])
 (a.output/'report.json').write_text(json.dumps(output,indent=2,sort_keys=True)+'\n');print(json.dumps(output))
if __name__=='__main__':main()
