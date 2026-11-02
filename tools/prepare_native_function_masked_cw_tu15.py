#!/usr/bin/env python3
"""Private normal masked-CW pow extension; never edit the frozen evaluator."""
import argparse,difflib,shutil
from pathlib import Path
from build_native_tu15_constraint_before_reference import sha,save
ROOT=Path(__file__).resolve().parents[1]
def once(s,a,b):assert s.count(a)==1,(a,s.count(a));return s.replace(a,b)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);relative='src/core/native_tu15_function_evaluation_extended.cpp';original=(ROOT/relative).read_text();s=original
 s=once(s,'  // The normal CRT\'s alternate-CW precision publication has a distinct owner.\n  if(previous!=0x27fu)unsupported();','''  // Actual332099 retains precision and canonicalizes rounding/masks. The
  // caller's masked precision bit makes332192 skip error publication.
  const auto restore_control=[&](){asm volatile("fldcw %0"::"m"(previous):"memory");};''')
 s=once(s,'  asm volatile("fldl %0"::"m"(base_double):"memory","st");','''  if(previous!=0x27fu){const auto canonical=std::uint16_t((previous&0x0300u)|0x007fu);
    asm volatile("fldcw %0"::"m"(canonical):"memory");}
  asm volatile("fldl %0"::"m"(base_double):"memory","st");''')
 s=once(s,'if((exponent&0x7fffffffu)==0u){asm volatile("fstp %%st(0);fld1":::"memory","st");return;}','if((exponent&0x7fffffffu)==0u){asm volatile("fstp %%st(0);fld1":::"memory","st");restore_control();return;}')
 s=once(s,'asm volatile("fldz":::"memory","st");if(negate)asm volatile("fchs":::"memory","st");return;','asm volatile("fldz":::"memory","st");if(negate)asm volatile("fchs":::"memory","st");restore_control();return;')
 s=once(s,'    if(magnitude==0u||magnitude==0x7ff0000000000000ull)unsupported();}\n}','    if(magnitude==0u||magnitude==0x7ff0000000000000ull)unsupported();}\n  restore_control();\n}')
 target=a.output/'staged'/relative;target.parent.mkdir(parents=True);target.write_text(s)
 (a.output/'delta.patch').write_text(''.join(difflib.unified_diff(original.splitlines(True),s.splitlines(True),fromfile='a/'+relative,tofile='b/'+relative)))
 save(a.output/'source-pins.json',dict(qualified=False,working_tree_modified=False,base_hashes={relative:sha(ROOT/relative)},staged_hashes={relative:sha(target)},
  source_audit='332099 normalizes CW to(savedPC&300)|7F;332192 tests savedCW precision mask20, so masked alternate CW does not select kind8 publication.',
  remaining=['BothcompilerPC0/2/3 andfourrounding wholeparent/edge qualification','All11 positivePART/point/gameplay recomposition','Exceptionalunmasked/domain/range/error profiles']))
 print(dict(prepared=True,semantic_files=1));return 0
if __name__=='__main__':raise SystemExit(main())
