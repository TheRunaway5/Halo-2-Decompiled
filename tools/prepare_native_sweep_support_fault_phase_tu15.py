#!/usr/bin/env python3
"""Private Source2E5DB0 read/FSTP fault-phase correction; no Root mutation."""
import argparse,difflib
from pathlib import Path
from build_native_tu15_constraint_before_reference import sha,save
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);stage=a.output/'staged';relative='src/physics/native_tu15_contact_shape.cpp';source=ROOT/relative;before=source.read_text();after=before
 old='''        const auto value=m.word(scratch.scalar),zero=m.word(0x46a0e8u);if(!(compare(value,zero)&0x4100u)){'''
 new='''        const auto value=m.word(scratch.scalar);
        // Source2E5DF7 has already loaded ST0 when2E5DFB reads zero.
        asm volatile("flds %0"::"m"(value):"memory","st");
        const auto zero=m.word(0x46a0e8u);std::uint16_t relation;
        asm volatile("fcomps %1;fnstsw %0":"=a"(relation):"m"(zero):"memory","st");
        if(!(relation&0x4100u)){'''
 assert after.count(old)==1;after=after.replace(old,new)
 anchor='''    __m128 vector(std::uint32_t p)const{'''
 method='''    void pop_float(std::uint32_t pc,std::uint32_t p){
        SourcePhysicsContactRecordStore event{pc,p,{},{}};
        if(observe_){event.before.resize(4);memory_.read(p,event.before);}
        alignas(16)std::array<std::byte,108>before{};std::uint32_t bits;
        asm volatile("fnsave %0;frstor %0":"=m"(before)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        asm volatile("fstps %0":"=m"(bits)::"memory","st");
        std::array<std::byte,4>b{};for(unsigned n=0;n<4;++n)b[n]=std::byte(bits>>(8u*n));
        try{memory_.write(p,b);}catch(...){asm volatile("frstor %0"::"m"(before):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");throw;}
        if(observe_){event.written.assign(b.begin(),b.end());observe_(event);}
    }
'''
 assert after.count(anchor)==1;after=after.replace(anchor,method+anchor)
 old='''std::uint32_t rounded;asm volatile("fstps %0":"=m"(rounded)::"memory","st");m.word(0x2e5e14u,output+0xcu,rounded);''';new='''m.pop_float(0x2e5e14u,output+0xcu);''';assert after.count(old)==1;after=after.replace(old,new)
 target=stage/relative;target.parent.mkdir(parents=True,exist_ok=True);target.write_text(after);(a.output/'delta.patch').write_text(''.join(difflib.unified_diff(before.splitlines(True),after.splitlines(True),fromfile='a/'+relative,tofile='b/'+relative)))
 save(a.output/'source-pins.json',dict(qualified=False,root_reviewed=False,production_applied=False,base_hashes={relative:sha(source)},staged_hashes={relative:sha(target)},scope='Only actual2E5DB0 sweep-support read/FSTP fault phases. Hardware-native FLD precedes actual zero memory read. FSTP uses native hardware preinstruction checkpoint solely for write-fault rollback; successful store and post-store observers see completed pop. No GP file, supplied FP flags/postimage or new algorithm. Other support methods remain byte-identical.',known_raw_failure='analysis/physics/native-moving-shape-process-cache-20261102-second/report.json'))
if __name__=='__main__':main()
