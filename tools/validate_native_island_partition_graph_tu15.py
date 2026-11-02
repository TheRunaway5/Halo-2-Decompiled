#!/usr/bin/env python3
"""Physical TU1.5 pure connectivity helper proof on declared valid forests."""
import argparse,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
from build_native_tu15_constraint_before_reference import sha,save
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],captured_live_partition_input=False)
 files=[ROOT/'src/physics/native_tu15_island_partition_graph.cpp',ROOT/'include/halo2/native_tu15_island_partition_graph.hpp',ROOT/'tools/observe_native_island_partition_graph_tu15.cpp',ROOT/'tools/observe_native_activation_agents_tu15.cpp',Path(__file__)];pins={str(p.relative_to(ROOT)):sha(p)for p in files}
 try:
  from capstone import CS_AC_READ,CS_AC_WRITE
  from capstone.x86 import X86_OP_MEM
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');metadata={};sites={};listings={}
  for owner in(0x37c170,0x37c190,0x37c250):
   body=complete_body(code,owner);listing=[];sites[hex(owner)]=len(body.code)
   for pc in sorted(body.code):
    i=code.at(pc);listing.append(f'{pc:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n');operands=[]
    if i.mnemonic!='lea':
     for op in i.operands:
      if op.type!=X86_OP_MEM or op.mem.segment:continue
      operands.append(dict(base=i.reg_name(op.mem.base)or'',index=i.reg_name(op.mem.index)or'',scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(op.access&CS_AC_READ),write=bool(op.access&CS_AC_WRITE)))
    metadata[str(pc)]=dict(size=i.size,memory=operands)
   listings[hex(owner)]=hashlib.sha256(''.join(listing).encode()).hexdigest()
  save(scratch/'metadata.json',metadata)
  cases=[dict(name='constructor-preserves-unwritten-word',owner=0x37c170,parents=[],edges=[]),dict(name='union-forward-roots',owner=0x37c190,parents=[-1]*8,edges=[],first=1,second=6),dict(name='union-reverse-roots',owner=0x37c190,parents=[-1]*8,edges=[],first=6,second=1),dict(name='union-identical-root',owner=0x37c190,parents=[-1]*8,edges=[],first=4,second=4)]
  forest=[-3,0,1,-2,3,-1]
  for first,second in((2,4),(4,2),(2,1),(5,2)):cases.append(dict(name=f'union-compress-{first}-{second}',owner=0x37c190,parents=forest,edges=[2,4,5,1],first=first,second=second))
  cases+=[dict(name='compress-empty',owner=0x37c250,parents=[],edges=[]),dict(name='compress-chain-and-edge-roots',owner=0x37c250,parents=forest,edges=[2,4,5,1]),dict(name='compress-disconnected-roots',owner=0x37c250,parents=[-1]*8,edges=[7,0,6,1]),dict(name='compress-long-chain',owner=0x37c250,parents=[-16,*range(15)],edges=[15,14,13,0])];save(scratch/'cases.json',dict(cases=cases,declared_source_domain=True,captured_live_input=False))
  section=next(s for s in image.sections if s.name=='.text');(scratch/'text.bin').write_bytes(image.section_bytes(section));(scratch/'reference.s').write_text(f'''.code32
.section .original,"ax"
.incbin "{scratch/'text.bin'}"
.section .data_bank,"aw",@nobits
.skip 16384
.section .private_stack,"aw",@nobits
.skip 65536
.section .bridge,"ax"
.globl _start
_start:
 fninit
 mov $0x6000f000,%esp
 int3
 mov $1,%eax
 xor %ebx,%ebx
 int $0x80
''');(scratch/'link.ld').write_text(f'ENTRY(_start)\nSECTIONS{{.original 0x{section.va:x}:{{*(.original)}} .data_bank 0x1000000:{{*(.data_bank)}} .private_stack 0x60000000:{{*(.private_stack)}} .bridge 0x70000000:{{*(.bridge)}}}}\n')
  for command in[['as','--32',str(scratch/'reference.s'),'-o',str(scratch/'reference.o')],['ld','-melf_i386','-T',str(scratch/'link.ld'),str(scratch/'reference.o'),'-o',str(scratch/'reference')]]:subprocess.run(command,check=True,capture_output=True)
  project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include');prefix=(ROOT/'tools/observe_native_activation_agents_tu15.cpp').read_text().split('int main(int argc,char**argv)')[0];(project/'driver.cpp').write_text('#include "halo2/native_tu15_island_partition_graph.hpp"\n'+prefix+(ROOT/'tools/observe_native_island_partition_graph_tu15.cpp').read_text());text=(ROOT/'src/physics/native_tu15_island_partition_graph.cpp').read_text();(project/'native.cpp').write_text(text)
  variants={'wrong_union_root':text.replace('return less(first_root,second_root)?first_root:second_root;','return less(first_root,second_root)?second_root:first_root;'),'missing_edge_compression':text.replace('m.write(0x37c2adu,cursor,next);','break; /* Negative control omits required edge-root compression. */')}
  for name,s in variants.items():assert s!=text;(project/(name+'.cpp')).write_text(s)
  (project/'negative.py').write_text('import json,subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True);j=json.load(open(sys.argv[-1]));assert r.returncode!=0 and not j.get("error") and any(not c["passed"]for c in j["cases"]),(r.returncode,j)\n')
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(partitionGraph LANGUAGES CXX)\ninclude(CTest)\n'
  for name in('native',*variants):
   cmake+=f'add_executable(graph_{name} driver.cpp {name}.cpp)\ntarget_include_directories(graph_{name} PRIVATE "{project}/include")\ntarget_compile_features(graph_{name} PRIVATE cxx_std_20)\n';negative=''if name=='native'else f'"{sys.executable}" "{project}/negative.py" ';cmake+=f'add_test(NAME graph_{name} COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- {negative}$<TARGET_FILE:graph_{name}> "{scratch}/reference" "{scratch}/cases.json" "{scratch}/metadata.json" "${{CMAKE_BINARY_DIR}}/{name}.json")\n'
  cmake+='set_tests_properties(graph_native graph_wrong_union_root graph_missing_edge_compression PROPERTIES TIMEOUT 90)\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in('g++','clang++'):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','graph_native','--target','graph_wrong_union_root','--target','graph_missing_edge_compression','--filter','^graph_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1',*sum((['--retain-report',name+'.json']for name in('native',*variants)),[])];r=subprocess.run(command,check=False);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
  report.update(passed=all(r['exit_code']==0 for r in results)and all(sha(ROOT/p)==h for p,h in pins.items()),compiler_results=results,profiles_per_compiler=len(cases)*16,declared_cases=len(cases),source_hashes=pins,original_image_sha256=NEW_PIN,source_sites=sites,listing_sha256=listings,negative_controls=list(variants),scope='Pure140-site graph helper frontier on declared valid forests; no live3187C0 branch or original caller captured')
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
