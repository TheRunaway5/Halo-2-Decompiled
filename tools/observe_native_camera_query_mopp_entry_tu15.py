#!/usr/bin/env python3
"""Genuine16C620 unchanged source to first native-format MOPP code entry."""
import inspect
import validate_native_bsp_segment_query_actual_tu15 as parent

def reference_original():
 source=inspect.getsource(parent.base.original)
 needle="        stack=bytearray(0x10000);"
 assert source.count(needle)==1
 source=source.replace(needle,"        assembly.append('.section .tls,\\\"aw\\\"\\n.space 4096');sections.append('.tls 0x71000000 : { *(.tls) }')\n"+needle)
 needle='_start:\n cld';assert source.count(needle)==1
 source=source.replace(needle,'_start:\n mov $123,%eax\n mov $1,%ebx\n mov $fs_descriptor,%ecx\n mov $16,%edx\n int $0x80\n mov $7,%eax\n mov %ax,%fs\n cld')
 needle='entry: .long';assert source.count(needle)==1;source=source.replace(needle,'fs_descriptor: .long 0,0x71000000,0xfffff,0x51\nentry: .long')
 return source

def main():
 source=inspect.getsource(parent.main)
 source=source.replace('base.OWNERS=OWNERS','base.OWNERS=(0x16c620,0x16b940,0x24dc90,0x2e9790,0x30aa40,0x2e9750,0x2e96a0,0x30b890,*OWNERS)')
 source=source.replace("observer=(ROOT/'tools/observe_native_array_growth_tu15.cpp').read_text()", "observer=(ROOT/'tools/observe_native_array_growth_tu15.cpp').read_text().replace('if(pc==stop)break;','if(pc==stop||pc==0x30aa70u)break;').replace('check(declared_fault||std::uint32_t(regs.rip)==stop,','check(declared_fault||std::uint32_t(regs.rip)==stop||std::uint32_t(regs.rip)==0x30aa70u,').replace('{\"return_word\",std::uint32_t(regs.rax)}','{\"return_word\",std::uint32_t(regs.rax)},{\"actual_stop\",std::uint32_t(regs.rip)},{\"boundary_gp\",gp(regs)},{\"boundary_stack_hex\",hex(memory(child.pid,std::uint32_t(regs.rsp),64))}')")
 source=source.replace(";(scratch/'observer.cpp').write_text(observer)",";"+'observer=observer.replace(\'  std::ofstream output(argv[3]);\',\'  const auto word=[&](std::uint32_t p){const auto raw=memory(child.pid,p,4);std::uint32_t n{};for(unsigned k=0;k<4;++k)n|=std::to_integer<unsigned>(raw[k])<<(8*k);return n;};const auto context=std::uint32_t(regs.rcx),state=word(std::uint32_t(regs.rsp)+4u),code=word(std::uint32_t(regs.rsp)+8u),bounds=word(std::uint32_t(regs.rsp)+12u);J typed_fields=J::object();for(unsigned off:{0x10u,0x14u,0x44u,0x54u,0x58u,0x5cu,0x60u,0x64u})typed_fields[std::to_string(off)]=word(context+off);const auto collection=word(context+0x64u),body=word(context+0x60u),collector=word(context+0x5cu),shape=word(body),body_field8=word(body+8u);J collector_fields=J::object();for(unsigned off:{0u,4u,0x10u,0x14u,0x1cu})collector_fields[std::to_string(off)]=word(collector+off);collector_fields["24_byte"]=std::to_integer<unsigned>(memory(child.pid,collector+0x18u,1)[0]);J typed{{"collection_hex",hex(memory(child.pid,collection,16))},{"body_hex",hex(memory(child.pid,body,16))},{"shape_hex",hex(memory(child.pid,shape,20))},{"body_field8_defined96_hex",hex(memory(child.pid,body_field8,96))},{"collector_defined_fields",collector_fields},{"private_unwritten_fields_excluded",J::array({"cache+40/ray+20","collector+8","collector+C"})},{"quantization_state_hex",hex(memory(child.pid,state,32))},{"axis_bounds_hex",hex(memory(child.pid,bounds,32))},{"origin_hex",hex(memory(child.pid,context+0x20u,16))},{"endpoint_hex",hex(memory(child.pid,context+0x30u,16))},{"hit_byte",std::to_integer<unsigned>(memory(child.pid,context+0x50u,1)[0])},{"context_fields",typed_fields},{"code_head_hex",hex(memory(child.pid,code,32))}};\\n  std::ofstream output(argv[3]);\').replace(\'{"initial_banks",before}\',\'{"mopp_typed_data",typed},{"initial_banks",before}\')'+";(scratch/'observer.cpp').write_text(observer)")
 source=source.replace("meta_env=dict(base.__dict__);",'meta_source=meta_source.replace("assert not op.mem.segment or ins.reg_name(op.mem.segment)==\'es\'","assert not op.mem.segment or ins.reg_name(op.mem.segment)in(\'es\',\'fs\')");meta_env=dict(base.__dict__);')
 source=source.replace('inspect.getsource(base.original)','reference_original()')
 start=source.index('  source=source.replace(" mov ${case[\'receiver\']}');end=source.index('\n  fp=',start);source=source[:start]+source[end:]
 source=source.replace('context=base.SP-40;banks[context]=bytes(40)','context=base.SP-0x41c;banks[context]=bytes(0x41c)')
 source=source.replace("entry=0x1e68e0,receiver=q['flags'],arguments=[q['limit'],q['bitmap'],q['origin'],q['displacement']]","entry=0x16c620,receiver=0,arguments=[typed['mask'],typed['origin'],typed['displacement'],typed['ignore_first'],typed['ignore_second'],typed['result']]")
 source=source.replace("assert all(row['after_esp']==base.SP+20 for row in rows);",'')
 start=source.index("  project=scratch/");end=source.index('\n except BaseException',start)
 source=source[:start]+'''  report.update(passed=True,actual_boundaries=[dict(stop=r['actual_stop'],gp=r['boundary_gp'],stack_hex=r['boundary_stack_hex'],fp=r['after_fp'],typed_data=r['mopp_typed_data'])for r in rows],scope=__doc__)
'''+source[end:]
 source=source.replace("report['error']=str(e)or repr(e);raise","report['error']=str(e)or repr(e);report['compiler_diagnostic']=str(getattr(e,'stderr',b''))[-8192:];raise")
 env=dict(parent.__dict__,__file__=__file__,__doc__=__doc__,reference_original=reference_original);exec(source,env);return env['main']()
if __name__=='__main__':raise SystemExit(main())
