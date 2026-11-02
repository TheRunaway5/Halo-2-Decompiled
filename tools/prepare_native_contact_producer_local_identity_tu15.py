#!/usr/bin/env python3
"""Stage the actual304387 semantic producer-identity DATA publication.

The written value is the ordinary typed producer argument, not a GP image.
This does not resolve earlier constructor-preimage provenance or admit a
native304350 World path; it preserves one verified local DATA lifetime phase.
"""
import argparse,hashlib,json,difflib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);a=parser.parse_args();out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
 relative=Path('src/physics/native_tu15_contact_producer.cpp');path=ROOT/relative;before=path.read_text()
 needle='void profile(Memory&m,bool ending,const NativeTu15ContactProducerServices&s){const auto cursor=m.word(0x48c974u);if(cursor>=m.word(0x48c978u))return;'
 replacement='void profile(Memory&m,bool ending,const NativeTu15ContactProducerServices&s,std::uint32_t producer_local=0,std::uint32_t producer=0){const auto cursor=m.word(0x48c974u),limit=m.word(0x48c978u);if(!ending)m.word(0x304387u,producer_local,producer);if(cursor>=limit)return;'
 assert before.count(needle)==1 and before.count('profile(m,false,s);')==1
 after=before.replace(needle,replacement).replace('profile(m,false,s);','profile(m,false,s,p+0x1cu,agent);')
 target=out/'staged'/relative;target.parent.mkdir(parents=True);target.write_text(after)
 (out/'delta.patch').write_text(''.join(difflib.unified_diff(before.splitlines(True),after.splitlines(True),fromfile='a/'+str(relative),tofile='b/'+str(relative))))
 listing=ROOT/'analysis/physics/native-retained-contact-producer-source-20261102/00304350.asm'
 assert '00304373 8bf1 mov esi, ecx\n'in listing.read_text()and '00304387 8974241c mov dword ptr [esp + 0x1c], esi\n'in listing.read_text()
 report={'qualified':False,'production_applied':False,'scope':__doc__,'base_hashes':{str(relative):sha(path)},'staged_hashes':{str(relative):sha(target)},'source_listing_sha256':sha(listing),'semantic_value':'producer.identity, copied from actual this argument before any child call','source_phase':'After genuine profiling cursor/limit reads, before profile conditional branch, at304387; unconditional even when profiling is disabled','remaining':['Earlier constructor preimage and cross-call DATA ownership','Native remaining services, World and visible multiplayer qualification'],'retained_large_artifacts':[]}
 (out/'source-pins.json').write_text(json.dumps(report,indent=2)+'\n');print({'stage':str(out),'base':sha(path),'candidate':sha(target)})
if __name__=='__main__':main()
