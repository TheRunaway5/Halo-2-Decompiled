#!/usr/bin/env python3
"""Original-only counterfactual private-row controls after the genuine copy.

No live game inputs, code, callee outcomes or native-kernel inputs are changed.
SNaN/arbitrary patterns replace only the four already-unprovided private rows;
all captured retained bytes and final FP must equal the untouched original run.
"""
import inspect,json,sys,hashlib
from pathlib import Path
import study_native_solver_unused_rows_tu15 as watch
ROOT=Path(__file__).resolve().parents[1]
def observer_source():
 text=watch.observer_source()
 anchor='for(unsigned profile=0;profile<4;++profile){'
 assert text.count(anchor)==1
 text=text.replace(anchor,anchor+'std::vector<std::vector<std::byte>>baseline_banks;J baseline_fp;for(unsigned control=0;control<3;++control){')
 anchor='copied=true;copy_sites='
 assert text.count(anchor)==1
 text=text.replace(anchor,'''copied=true;if(control){std::array<std::byte,16>poison{};const std::uint32_t pattern=control==1?0x7f800001u:0xa5a5a5a5u;for(unsigned lane=0;lane<4;++lane)std::memcpy(poison.data()+lane*4u,&pattern,4);for(auto offset:{0x30u,0x40u,0xf0u,0x100u})child.write(work+offset,poison);}copy_sites=''')
 anchor='  report["profiles"].push_back('
 assert text.count(anchor)==1
 text=text.replace(anchor,'''  std::size_t compared{},different{},ordinal{};for(const auto&region:capture.at("regions")){const auto bytes=child.read(region.at("address"),region.at("size"));compared+=bytes.size();if(!control)baseline_banks.push_back(bytes);else{check(ordinal<baseline_banks.size()&&bytes.size()==baseline_banks[ordinal].size(),"Original private-row bank set differs");for(std::size_t n=0;n<bytes.size();++n)different+=bytes[n]!=baseline_banks[ordinal][n];}++ordinal;}const auto final_fp=fp_words(child.fp());if(!control)baseline_fp=final_fp;check(different==0&&final_fp==baseline_fp,"Original private-row poison changes retained DATA/FP");
  report["profiles"].push_back(''')
 text=text.replace('{"work_identity",work}', '{"private_row_control",control},{"retained_bytes_compared",compared},{"retained_byte_differences",different},{"fp_matches_unmodified_original",final_fp==baseline_fp},{"work_identity",work}')
 anchor='\n }\n report["passed"]=true;'
 assert text.count(anchor)==1;text=text.replace(anchor,'\n }}\n report["passed"]=true;')
 return text
def main():
 text=inspect.getsource(watch.main);namespace=dict(watch.__dict__,observer_source=observer_source,__doc__=__doc__);exec(text,namespace);rc=namespace['main']()
 output=Path(sys.argv[sys.argv.index('--output')+1]);path=output/'report.json';r=json.loads(path.read_text());r['source_only_private_row_invariance']=True;r['private_row_controls']=['untouched original','four SNaN words per unprovided row','arbitrary A5 pattern per unprovided row'];r['source_hashes'][str(Path(__file__).relative_to(ROOT))]=hashlib.sha256(Path(__file__).read_bytes()).hexdigest();r['runtime_input_or_native_admission']=False;path.write_text(json.dumps(r,indent=2,sort_keys=True)+'\n');return rc
if __name__=='__main__':raise SystemExit(main())
