#!/usr/bin/env python3
"""Private typed-DATA overload for original320260's repurposed formal word."""
import argparse,difflib,shutil
from pathlib import Path
from build_native_tu15_constraint_before_reference import sha,save
ROOT=Path(__file__).resolve().parents[1]
def once(s,a,b):assert s.count(a)==1,(a,s.count(a));return s.replace(a,b)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);files=['include/halo2/native_tu15_pair_handlers.hpp','src/physics/native_tu15_pair_handlers.cpp'];before={r:(ROOT/r).read_text()for r in files}
 h=before[files[0]];anchor='void native_tu15_delete_pair('
 start=h.index(anchor);end=h.index(';',start)+1;declaration=h[start:end]
 additional=declaration.replace('NativeTu15PhysicsFrameFp&','NativeTu15PairDeletionScratch,NativeTu15PhysicsFrameFp&',1)
 h=h[:start]+'// Ordinary source-owned formal DATA, repurposed by3203C9; no CPU frame.\nstruct NativeTu15PairDeletionScratch {std::uint32_t formal_pair_data{};};\n'+additional+'\n'+h[start:]
 cpp=once(before[files[1]],'#include <stdexcept>','#include <stdexcept>\n#include <optional>')
 cpp=once(cpp,'void native_tu15_delete_pair(SourcePhysicsContactMemory& memory,std::uint32_t handler,\n    std::uint32_t pair,NativeTu15PhysicsFrameFp& fp,','static void delete_pair_data(SourcePhysicsContactMemory& memory,std::uint32_t handler,\n    std::uint32_t pair,std::optional<NativeTu15PairDeletionScratch> scratch,NativeTu15PhysicsFrameFp& fp,')
 cpp=once(cpp,'if(!deferred){second_handle=m.word(pair+4u);const auto second_body=',
  'if(!deferred){second_handle=m.word(pair+4u);if(scratch)m.word(0x3203c9u,scratch->formal_pair_data,second_handle);const auto second_body=')
 wrappers='''void native_tu15_delete_pair(SourcePhysicsContactMemory&memory,std::uint32_t handler,
    std::uint32_t pair,NativeTu15PairDeletionScratch scratch,NativeTu15PhysicsFrameFp&fp,
    const NativeTu15PairHandlerServices&services,const SourcePhysicsContactRecordObserver&observe){
    delete_pair_data(memory,handler,pair,scratch,fp,services,observe);
}
void native_tu15_delete_pair(SourcePhysicsContactMemory&memory,std::uint32_t handler,
    std::uint32_t pair,NativeTu15PhysicsFrameFp&fp,
    const NativeTu15PairHandlerServices&services,const SourcePhysicsContactRecordObserver&observe){
    delete_pair_data(memory,handler,pair,std::nullopt,fp,services,observe);
}
''';cpp=once(cpp,'} // namespace halo2',wrappers+'} // namespace halo2')
 for relative,text in zip(files,(h,cpp)):
  target=a.output/'staged'/relative;target.parent.mkdir(parents=True,exist_ok=True);target.write_text(text)
 (a.output/'delta.patch').write_text(''.join(''.join(difflib.unified_diff(before[r].splitlines(True),(a.output/'staged'/r).read_text().splitlines(True),fromfile='a/'+r,tofile='b/'+r))for r in files))
 save(a.output/'source-pins.json',dict(qualified=False,production_applied=False,base_hashes={r:sha(ROOT/r)for r in files},staged_hashes={r:sha(a.output/'staged'/r)for r in files},
  scope='Typed public320260 overload preserves original3203C9 repurposed formal DATA; existing prototype behavior unchanged. Source-owned DATA identity, no GP/stack/CPU reconstruction. Not wired or fully qualified.'))
 print(dict(prepared=True));return 0
if __name__=='__main__':raise SystemExit(main())
