"""Observer-only changing x87 topology for the exact EMMS packed-rotation leaf."""
import hashlib,inspect
from pathlib import Path
import validate_native_tu15_prt_helpers as shared
from native_mopp_ray_dense_original import DENSE

def physical_original_rotation(*args,opaque_input=False,**kwargs):
    if tuple(kwargs.get('entries',()))!=(0x296180,):raise ValueError('Packed rotation observer has a different owner')
    if kwargs.get('preserve_x87_prefix')or kwargs.get('return_x87'):raise ValueError('Packed rotation is not FP-neutral or an ST0-return owner')
    source=inspect.getsource(shared.physical_original)
    if opaque_input:
        from native_sphere_frustum_mmx_original import BRIDGE,SEED,SNAPSHOT
        assembly_marker="    (directory / 'reference.s').write_text(assembly)"
        assert source.count(assembly_marker)==1;source=source.replace(assembly_marker,'    assembly += '+repr(BRIDGE)+'\n'+assembly_marker)
        source=source.replace('    observer_text = observer_source.read_text()','    observer_text = observer_source.read_text().replace("#include <map>","#include <map>\\n#include <optional>")')
    old='const auto observed_depth=std::int64_t(depth)+trace.value("x87_delta",0);'
    assert source.count(old)==1;source=source.replace(old,'const auto observed_depth='+DENSE.replace('FP.','current.')+';')
    marker="    private_observer = directory / 'observer.cpp'"
    topology='if(finished){check(fp.cwd==entry_fp.cwd&&((fp.swd^entry_fp.swd)&0x3800u)==0&&fp.ftw==entry_fp.ftw,"Original path start changed live x87 topology");for(unsigned n=0;n<depth;++n)check(!std::memcmp(reinterpret_cast<const std::byte*>(fp.st_space)+n*16,reinterpret_cast<const std::byte*>(entry_fp.st_space)+n*16,10),"Original path start changed a live x87 input");}'
    replacement='if(finished){check(fp.cwd==entry_fp.cwd&&(fp.swd&0x3800u)==0&&fp.ftw==0,"Original packed rotation did not complete its EMMS interval");}'
    result='if(c.contains("stop_before_entries"))result.back()["stopped_at"]=std::uint32_t(regs.rip);'
    projection=('result.back()["boundary_live_x87"]=J::array();const auto boundary_depth='+DENSE.replace('FP.','fp.')+';'
                'for(unsigned n=0;n<boundary_depth;++n)result.back()["boundary_live_x87"].push_back('
                'hex(std::span(reinterpret_cast<const std::byte*>(fp.st_space)+n*16,10)));')
    addition=''
    for old,new in((topology,replacement),('{"live_x87_unchanged",finished}','{"live_x87_unchanged",false},{"source_emms_return_checked",finished}'),(result,projection+result)):
        addition+='    if observer_text.count('+repr(old)+')!=1:raise ValueError("Packed rotation observer boundary drift")\n'
        addition+='    observer_text=observer_text.replace('+repr(old)+','+repr(new)+')\n'
    if opaque_input:
        seed='request(PTRACE_SETFPREGS,child.pid,nullptr,&fp);'
        snapshot='  std::vector<std::byte>stack(12);'
        call='    calls.push_back(observed);'
        capture='if(c.contains("active_mmx")&&pc<=0x2961a5u){observed["active_mmx"]=full_mmx();observed["live_x87"]=J::array();}'
        final='if(c.contains("active_mmx")&&std::uint32_t(regs.rip)<=0x2961a5u){result.back()["active_mmx"]=full_mmx();result.back()["boundary_live_x87"]=J::array();}'
        for old,new in((seed,seed+'if(c.contains("active_mmx")){'+SEED+'}'),(snapshot,SNAPSHOT+snapshot),(call,capture+call),(result,final+result)):
            addition+='    if observer_text.count('+repr(old)+')!=1:raise ValueError("Packed rotation opaque observer boundary drift")\n'
            addition+='    observer_text=observer_text.replace('+repr(old)+','+repr(new)+')\n'
    assert source.count(marker)==1;source=source.replace(marker,addition+marker)
    namespace=dict(vars(shared));exec(compile(source,__file__,'exec'),namespace)
    result,proof=namespace['physical_original'](*args,**kwargs)
    proof.update(observer_wrapper_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),x87_projection='Actual physical FTW/TOP dense occupancy and80-bit values',source_emms_changes_topology=True,opaque_incoming_observed=opaque_input,mmx_intermediate_qualified=False)
    return result,proof
