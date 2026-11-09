"""Observer-only dense x87 projection for private raw MOPP ray qualification.

The shared physical runner is unchanged. Its owned observer derives logical
occupancy from physical FTW/TOP and exports actual live 80-bit boundary values;
no instruction, numerical value, GP state or dependency result is supplied.
"""
import inspect

import validate_native_tu15_prt_helpers as shared


DENSE = ('([&]{unsigned count=0;const auto top=(FP.swd>>11u)&7u;'
         'while(count<8&&((FP.ftw>>((top+count)&7u))&1u))++count;'
         'for(unsigned n=count;n<8;++n)check(!((FP.ftw>>((top+n)&7u))&1u),'
         '"Physical ray x87 register file has holes");return count;}())')


def physical_original_dense(*args, allow_unbalanced_selected_stop=False, **kwargs):
    source = inspect.getsource(shared.physical_original)
    old = 'const auto observed_depth=std::int64_t(depth)+trace.value("x87_delta",0);'
    assert source.count(old) == 1
    source = source.replace(old, 'const auto observed_depth='+DENSE.replace('FP.', 'current.')+';')
    marker = '    private_observer = directory / \'observer.cpp\''
    assert source.count(marker) == 1
    result_marker = 'if(c.contains("stop_before_entries"))result.back()["stopped_at"]=std::uint32_t(regs.rip);'
    projection = ('result.back()["boundary_live_x87"]=J::array();'
                  'const auto boundary_depth='+DENSE.replace('FP.', 'fp.')+';'
                  'for(unsigned n=0;n<boundary_depth;++n)result.back()["boundary_live_x87"].push_back('
                  'hex(std::span(reinterpret_cast<const std::byte*>(fp.st_space)+n*16,10)));')
    addition = ('    dense_marker = '+repr(result_marker)+'\n'
                '    if observer_text.count(dense_marker) != 1:\n'
                '        raise ValueError("Physical ray boundary projection changed")\n'
                '    observer_text = observer_text.replace(dense_marker, '+repr(projection)+' + dense_marker)\n')
    if allow_unbalanced_selected_stop:
        assert not kwargs.get('preserve_x87_prefix'), 'A numerical continuation is not an FP-neutral prefix'
        topology='if(finished){check(fp.cwd==entry_fp.cwd&&((fp.swd^entry_fp.swd)&0x3800u)==0&&fp.ftw==entry_fp.ftw,'
        qualification='{"live_x87_unchanged",finished}'
        replacement='{"live_x87_unchanged",finished&&std::uint32_t(regs.rip)==stop},{"selected_numerical_boundary",finished&&std::uint32_t(regs.rip)!=stop}'
        addition+=('    topology_marker = '+repr(topology)+'\n'
                   '    qualification_marker = '+repr(qualification)+'\n'
                   '    if observer_text.count(topology_marker)!=1 or observer_text.count(qualification_marker)!=1:\n'
                   '        raise ValueError("Physical numerical continuation invariant changed")\n'
                   '    observer_text=observer_text.replace(topology_marker, '+repr(topology.replace('if(finished){','if(finished&&std::uint32_t(regs.rip)==stop){'))+')\n'
                   '    observer_text=observer_text.replace(qualification_marker, '+repr(replacement)+')\n')
    source = source.replace(marker, addition+marker)
    namespace = dict(vars(shared))
    exec(compile(source, __file__, 'exec'), namespace)
    proof, authority = namespace['physical_original'](*args, **kwargs)
    authority['x87_projection'] = 'Actual physical FTW/TOP dense logical occupancy and live80; no input-depth delta'
    authority['numerical_selected_stop'] = allow_unbalanced_selected_stop
    return proof, authority
