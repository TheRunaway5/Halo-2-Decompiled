#!/usr/bin/env python3
"""Same genuine314710 proof with source-unwritten simplex rows guarded out.

Source30DE defines only first0,second0,count and difference0 in the selected
case. This proves the native child never reads/writes the other input rows;
it does not qualify30DE's later unconditional copies of those private bytes.
"""
import hashlib,inspect,json,sys
from pathlib import Path
import validate_native_solver_hull_query_actual_tu15 as actual
def observer_source():
 text=actual.observer_source()
 # Source313560 fully publishes direction16,point16,distance4,weight4.
 assert text.count('std::pair{args[5],16u}')==1
 text=text.replace('std::pair{args[5],16u}','std::pair{args[5],40u}')
 before='bool track;mutable J ledger=J::array();Tracked(ProcessMemory&v,const std::function<bool(std::uint32_t,std::size_t)>&r,bool t):m(v),retained(r),track(t){}'
 after='''bool track;mutable unsigned unprovided_attempts{};const std::array<std::uint32_t,6>&args;mutable J ledger=J::array();Tracked(ProcessMemory&v,const std::function<bool(std::uint32_t,std::size_t)>&r,bool t,const std::array<std::uint32_t,6>&a):m(v),retained(r),track(t),args(a){}void guard(std::uint32_t p,std::size_t n)const{for(auto [first,size]:{std::pair{args[3]+16u,48u},std::pair{args[3]+80u,48u},std::pair{args[4]+16u,48u}})if(std::uint64_t(p)+n>first&&p<std::uint64_t(first)+size){++unprovided_attempts;throw std::runtime_error("source-unprovided-arithmetic-row");}}'''
 assert text.count(before)==1;text=text.replace(before,after)
 text=text.replace('override{m.read(p,b);if(track&&retained','override{guard(p,b.size());m.read(p,b);if(track&&retained')
 text=text.replace('override{m.write(p,b);}};','override{guard(p,b.size());m.write(p,b);}};')
 assert text.count('Tracked m(memory,eligible,read_pass);')==1;text=text.replace('Tracked m(memory,eligible,read_pass);','Tracked m(memory,eligible,read_pass,args);')
 needle='std::size_t compared{};for(const auto&b:banks)'
 controls='''check(m.unprovided_attempts==0,"Native child consumed Source-unwritten row");unsigned guard_controls{};for(auto hole:{args[3]+16u,args[3]+80u,args[4]+16u})for(bool writing:{false,true}){std::array<std::byte,4>bytes{};bool refused{};try{if(writing)m.write(hole,bytes);else m.read(hole,bytes);}catch(const std::runtime_error&e){refused=std::string(e.what())=="source-unprovided-arithmetic-row";}check(refused,"Unprovided DATA guard did not refuse before access");++guard_controls;}'''
 assert text.count(needle)==1;text=text.replace(needle,controls+needle)
 text=text.replace('{"native_result",result}', '{"native_result",result},{"native_unprovided_accesses",0},{"unprovided_guard_controls",guard_controls}')
 return text
def main():
 text=inspect.getsource(actual.main);namespace=dict(actual.__dict__,observer_source=observer_source,__doc__=__doc__);exec(text,namespace);rc=namespace['main']()
 p=Path(sys.argv[sys.argv.index('--output')+1])/'report.json';r=json.loads(p.read_text());r['guarded_unprovided_rows']=True;r['output_plane_compared_bytes']=40;r['source_definition_sites']={'simplex_first0':'30DEA9','simplex_second0':'30DEAE','simplex_count':'30DFE7','difference0':'30DE8F'};r['unprovided_rows']=['simplex first1..3','simplex second1..3','difference1..3'];r['new_proof_tool_sha256']=hashlib.sha256(Path(__file__).read_bytes()).hexdigest();r['later_parent_unconditional_copy_qualified']=False;p.write_text(json.dumps(r,indent=2,sort_keys=True)+'\n');return rc
if __name__=='__main__':raise SystemExit(main())
