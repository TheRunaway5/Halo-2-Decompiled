#!/usr/bin/env python3
"""Build the existing isolated cold constructor and inspect primary operand ownership."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--operands',type=Path,required=True)
    parser.add_argument('--report',type=Path,required=True)
    args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    assert scratch.is_dir()
    inputs=json.loads(args.operands.read_text())
    fields=sorted({(int(row['address'],16),row['bytes'])
                   for row in inputs['absolute_data_operands']})
    assert len(fields)<100
    declaration=',\n'.join('{'+hex(address)+','+str(size)+'}' for address,size in fields)
    source=scratch/'ownership.cpp'
    source.write_text('''#include "halo2/native_tu15_world_memory.hpp"
#include <array>
#include <iostream>
int main() {
 auto m=halo2::native_tu15_world_memory({0x60212000u,{},"isolated graphics data ownership clock"});
 const std::array<std::pair<unsigned,unsigned>,'''+str(len(fields))+'''> fields{{
'''+declaration+'''}};
 unsigned missing{};
 for(auto [address,bytes]:fields) {
  std::cout<<std::hex<<address<<' '<<std::dec<<bytes<<' ';
  try {
   const auto view=m.view(address,bytes);
   std::cout<<(m.is_read_only(address,bytes)?"RO":"RW")<<' ';
   m.visit_region_metadata([&](const auto& r){if(r.address<=address &&
    std::uint64_t(address)+bytes<=std::uint64_t(r.address)+r.bytes)
    std::cout<<std::hex<<r.address<<' '<<std::dec<<r.bytes<<' '<<r.owner;});
  } catch(const std::exception& e) {std::cout<<"MISSING "<<e.what();++missing;}
  std::cout<<'\\n';
 }
 return missing?2:0;
}
''')
    module=Path('cmake/NativeTu15ColdData.cmake')
    # Use exactly the existing cold-only target's production source owners.
    sources=re.findall(r'^\s+(src/[^\s)]+\.cpp)\s*$',module.read_text(),re.M)
    assert len(sources)>30
    compiler=os.environ.get('CXX','c++')
    binary=scratch/'ownership'
    command=[compiler,'-std=c++20','-O0','-ffunction-sections','-fdata-sections',
        '-ffp-contract=off','-Iinclude','-Isrc',str(source),*sources,'-pthread',
        '-Wl,--gc-sections','-o',str(binary)]
    result=subprocess.run(command,capture_output=True,text=True)
    log=scratch/'compile.log'
    log.write_text(result.stdout+result.stderr)
    rows=[]
    if result.returncode==0:
        result=subprocess.run([str(binary)],capture_output=True,text=True)
        for line in result.stdout.splitlines():
            address,size,access,*rest=line.split(' ',3)
            rows.append(dict(address='0x'+address,bytes=int(size),access=access,
                             owner=' '.join(rest)))
    report=dict(passed=result.returncode==0,component='Actual isolated native_tu15_world_memory constructor',
        fields=rows,missing=[row for row in rows if row['access']=='MISSING'],
        command=command,source_owners={path:hashlib.sha256(Path(path).read_bytes()).hexdigest() for path in sources},
        operands_sha256=hashlib.sha256(args.operands.read_bytes()).hexdigest(),
        returncode=result.returncode,error_tail=result.stderr[-4000:],
        scope='Bounded absolute data ownership only. No generated CPU body, renderer or campaign completion execution.',
        retained_large_artifacts=[])
    args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(passed=report['passed'],fields=len(rows),missing=report['missing'],
                         error_tail=report['error_tail'])))
    raise SystemExit(result.returncode)


if __name__=='__main__':
    main()
