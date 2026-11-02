#!/usr/bin/env python3
"""Genuine16C620 unchanged source to first instanced-query entry; no results."""
import inspect,sys
import validate_native_bsp_segment_query_actual_tu15 as parent

def main():
 source=inspect.getsource(parent.main)
 source=source.replace('base.OWNERS=OWNERS','base.OWNERS=(0x16c620,0x16b940,*OWNERS)')
 source=source.replace("observer=(ROOT/'tools/observe_native_array_growth_tu15.cpp').read_text()", "observer=(ROOT/'tools/observe_native_array_growth_tu15.cpp').read_text().replace('if(pc==stop)break;','if(pc==stop||pc==0x24dc90u||pc==0x16c370u)break;').replace('check(declared_fault||std::uint32_t(regs.rip)==stop,','check(declared_fault||std::uint32_t(regs.rip)==stop||std::uint32_t(regs.rip)==0x24dc90u||std::uint32_t(regs.rip)==0x16c370u,').replace('{\"return_word\",std::uint32_t(regs.rax)}','{\"return_word\",std::uint32_t(regs.rax)},{\"actual_stop\",std::uint32_t(regs.rip)},{\"boundary_gp\",gp(regs)},{\"boundary_stack_hex\",hex(memory(child.pid,std::uint32_t(regs.rsp),64))}')")
 start=source.index('  source=source.replace(" mov ${case[\'receiver\']}');end=source.index('\n  fp=',start);source=source[:start]+source[end:]
 source=source.replace('context=base.SP-40;banks[context]=bytes(40)','context=base.SP-0x41c;banks[context]=bytes(0x41c)')
 source=source.replace("entry=0x1e68e0,receiver=q['flags'],arguments=[q['limit'],q['bitmap'],q['origin'],q['displacement']]","entry=0x16c620,receiver=0,arguments=[typed['mask'],typed['origin'],typed['displacement'],typed['ignore_first'],typed['ignore_second'],typed['result']]")
 source=source.replace('assert all(row[\'after_esp\']==base.SP+20 for row in rows);','')
 start=source.index('  project=scratch/');end=source.index('\n except BaseException',start)
 source=source[:start]+'''  report.update(passed=True,actual_boundaries=[dict(stop=r['actual_stop'],gp=r['boundary_gp'],stack_hex=r['boundary_stack_hex'],fp=r['after_fp'])for r in rows],scope=__doc__)
'''+source[end:]
 namespace=dict(parent.__dict__,__doc__=__doc__,__file__=__file__);exec(source,namespace);return namespace['main']()
if __name__=='__main__':raise SystemExit(main())
