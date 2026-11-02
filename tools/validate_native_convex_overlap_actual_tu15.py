#!/usr/bin/env python3
"""Full2F8410 query with native30E930 and real collector on source-derived DATA.

The unchanged PROCESS prefix derives the agent and ordinary body/configuration
inputs. A separate declared query invocation uses those inputs and an owned
collector. This is not a captured live query ABI. No solver result is supplied.
Every captured bank, collector byte, ordered retained operand/store and FP field
is compared. Raw private CPU spills are not semantic outputs.
"""
import hashlib,inspect,json,sys
from pathlib import Path
import validate_native_convex_simplex_solver_actual_tu15 as core

ROOT=Path(__file__).resolve().parents[1]
ORIGINAL_OBSERVER=core.observer_source
UNITS=('native_tu15_wrapped_contact_query.cpp','native_tu15_contact_pair_collectors.cpp',
       'native_tu15_convex_overlap_query.cpp','native_tu15_inverse_geometry.cpp',
       'native_tu15_geometry.cpp')+core.UNITS

def observer_source():
 core.PRODUCTION_BINDING=True
 core.OVERLAP_INPUTS=False
 text=ORIGINAL_OBSERVER()
 text=text.replace('std::array<std::byte,4096>data{}','std::array<std::byte,8192>data{}').replace('<=4096;', '<=8192;')
 old='if(c.regs().rip==0x30e930u)return;'
 assert text.count(old)==1
 text=text.replace(old,'''if(c.regs().rip==0x2f8b60u){
    auto call=c.regs();const auto sp=std::uint32_t(call.rsp),collector=sp-0x2000u;
    const auto put=[&](std::uint32_t p,std::uint32_t n){std::array<std::byte,4>b{};for(unsigned k=0;k<4;++k)b[k]=std::byte(n>>(k*8u));c.write(p,b);};
    put(collector,collector+16u);put(collector+4u,0xa5a5a5a5u);put(collector+16u,0x2f0a50u);put(collector+20u,0x30ba80u);
    put(sp+16u,collector);call.rip=0x2f8410u;request(PTRACE_SETREGS,c.pid,nullptr,&call);return;
  }''')
 text=text.replace('std::array<std::uint32_t,5>args','std::array<std::uint32_t,4>args').replace('k<5;++k)args','k<4;++k)args')
 old='for(auto [p,n]:{std::pair{work+0xcu,1u},std::pair{args[2],64u}})inputs'
 new='for(auto [p,n]:{std::pair{args[0],16u},std::pair{args[1],16u},std::pair{original.word(args[0]+8u),96u},std::pair{original.word(args[1]+8u),96u},std::pair{args[3],24u}})inputs'
 assert text.count(old)==1;text=text.replace(old,new)
 start=text.index('  const auto defined_read=');stop=text.index('J reads=',start)
 text=text[:start]+'''  const auto defined_read=[&](std::uint32_t p,std::size_t n){return captured(p,n)||contains(inputs,p,n);};
  const auto defined_output=[&](std::uint32_t p,std::size_t n){return p>=args[3]&&std::uint64_t(p-args[3])+n<=24u;};
'''+text[stop:]
 text=text.replace('sp+24u','sp+20u').replace('Source30E RET20','Source2F8410 RET16')
 start=text.index('  for(auto [p,n]:',text.index('const auto expected_fp='));stop=text.index('  J passes=',start)
 text=text[:start]+'''  outputs.push_back({args[3],{},original.read(args[3],24u)});
'''+text[stop:]
 text=text.replace('NativeTu15ConvexSimplexMemory view(tracked);view.withhold(work,0x140u);','NativeTu15ConvexSimplexMemory view(tracked);')
 old='for(const auto&b:inputs)if(b.address>=work&&b.address-work<0x140u)view.write(b.address,b.before);'
 assert text.count(old)==1;text=text.replace(old,'')
 old='const auto result=bound.solve_simplex(work,{args[0]},{args[1]},args[2],args[3],args[4],fp)'
 assert text.count(old)==1
 text=text.replace(old,'''NativeTu15ConvexOverlapQueryServices query;query.geometry=std::move(bound);
   NativeTu15OverlapQueryBindingStats queries;NativeTu15OverlapQueryBinding query_dispatch(view,{p+0x1000u,512u},query,{},&queries);
   query_dispatch.query({work},{0x2f8410u},{args[0]},{args[1]},{args[2]},{args[3]},fp);
   check(queries.calls==1&&queries.completed==1&&queries.convex==1&&queries.marked_pairs==1,"Native actual overlap/collector activity differs");
   const auto result=0u''')
 text=text.replace('check(result==source_result,"Native30E result differs");','')
 # Check the actual native query's work lifetime, rather than the independent
 # source query's private CPU frame or the ordinary agent's backing bytes.
 for old,new in [('view.read_vector(work+offset,b)','view.read_vector(p+0x1080u+offset,b)'),
                 ('view.write(work+0x30u,word)','view.write(p+0x10b0u,word)'),
                 ('view.read(work+0x30u,word)','view.read(p+0x10b0u,word)'),
                 ('view.read_vector(work+0x30u,b)','view.read_vector(p+0x10b0u,b)')]:
  assert text.count(old)==1;text=text.replace(old,new)
 text='}\n#include "halo2/native_tu15_overlap_query_binding.hpp"\nnamespace {\n'+text
 return text

def main():
 # Reuse the pinned input reader, independent original callers, immutable code
 # tables, compiler cohorts and owned storage. Extend its actual body closure.
 text=inspect.getsource(core.main)
 old="parent.OWNERS=(0x30e930,0x30de60,0x2e5e20,0x3070f0,0x314710,"
 assert text.count(old)==1
 text=text.replace(old,"parent.OWNERS=(0x30e930,0x2f8410,0x30ba80,0x2e4550,0x30de60,0x2e5e20,0x3070f0,0x314710,")
 start=text.index(' mutations={');stop=text.index(' def checked_run',start)
 text=text[:start]+''' mutations={
  'wrong_query_flag':('p+0x7cu,1u,1u','p+0x7cu,0u,1u'),
  'skip_collector':('if(collect){','if(false&&collect){'),
  'wrong_cache':('cache_agent+12u,p+0x20u','cache_agent+16u,p+0x20u'),
 }
 kernel=ROOT/'src/physics/native_tu15_convex_overlap_query.cpp'
'''+text[stop:]
 env=dict(core.__dict__,UNITS=tuple(dict.fromkeys(UNITS)),observer_source=observer_source,__doc__=__doc__)
 if '--production-binding'not in sys.argv:sys.argv.append('--production-binding')
 assert '--overlap-inputs'not in sys.argv,'The real query owns its flag input'
 exec(text,env);rc=env['main']()
 p=Path(sys.argv[sys.argv.index('--output')+1])/'report.json';r=json.loads(p.read_text())
 r.update(scope=__doc__,qualified_query_owner=0x2f8410,query_invocation='Declared ordinary query over independent source-derived process DATA; no captured query ABI',void_volatile_return_compared=False,retained_collector_compared=True,full_world_or_live_qualification=False,new_query_tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
 for name in ('include/halo2/native_tu15_overlap_query_binding.hpp','include/halo2/native_tu15_convex_overlap_query.hpp','include/halo2/native_tu15_wrapped_contact_query.hpp','include/halo2/native_tu15_contact_pair_collectors.hpp'):
  r['source_hashes'][name]=hashlib.sha256((ROOT/name).read_bytes()).hexdigest()
 p.write_text(json.dumps(r,indent=2,sort_keys=True)+'\n');return rc

if __name__=='__main__':raise SystemExit(main())
