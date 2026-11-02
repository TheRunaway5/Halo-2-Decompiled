#!/usr/bin/env python3
"""Compose the same native30A1E0/2E9670 and309940 through real full returns."""
import inspect,json,struct
from pathlib import Path
import validate_native_mopp_aabb_setup_tu15 as parent
ROOT=Path(__file__).resolve().parents[1]
OWNERS=(0x30a1e0,0x2e9670,0x309940)
def metadata(image):
 parent.support.OWNERS=OWNERS;parent.support.BOUNDARIES=()
 text=inspect.getsource(parent.support.metadata).replace('assert counts==[276,38,20];','assert counts==[100,15,464];')
 text=text.replace('assert not op.mem.segment;',"assert not op.mem.segment or ins.reg_name(op.mem.segment)=='es';")
 text=text.replace("read=bool(op.access&CS_AC_READ)or ins.mnemonic=='test',write=bool(op.access&CS_AC_WRITE)","read=False if ins.mnemonic in('fst','fstp','fistp')else bool(op.access&CS_AC_READ)or ins.mnemonic=='test',write=bool(op.access&CS_AC_WRITE)or ins.mnemonic in('fst','fstp','fistp')")
 env=dict(parent.support.__dict__);exec(text,env);return env['metadata'](image)
def cases(image):
 rows=parent.cases(image)
 for index,c in enumerate(rows):
  b=c['banks'][0];data=bytearray.fromhex(b['before']);code=0x1000800
  program=([0]if index%4==0 else[0x30+(index%32)]if index%4==1 else[0x26,64,192,0x30]if index%4==2 else[1,1,2,3,0x26,64,192,0x30])
  data[code-0x1000000+0x30:code-0x1000000+0x30+len(program)]=bytes(program)
  for p,v in((0x1001400,0x1001500),(0x1001404,0),(0x1001408,0x80000020)):struct.pack_into('<I',data,p-0x1000000,v)
  b['before']=data.hex()
 return rows
def main():
 original_metadata,original_cases=parent.metadata,parent.cases
 # Keep the parent's ordinary input constructor; the modified function's
 # namespace below binds the composed wrapper without recursive monkeypatch.
 parent.base.metadata=metadata
 source=inspect.getsource(parent.main)
 source=source.replace('base.metadata=metadata;base.cases=cases','base.metadata=metadata;base.cases=cases')
 source=source.replace(".replace('J::array({220,2,2,2})','J::array({100,15})')",".replace('J::array({220,2,2,2})','J::array({100,15,464})')")
 source=source.replace("driver=driver.replace('#include", "driver=driver.replace('#include")
 needle=" text=text.replace(\"ROOT/'include/halo2/native_tu15_contact_pressure.hpp'"
 position=source.index(needle)
 inject=''' text=text.replace("(project/'types.cpp').write_text('');shutil.copyfile", "shutil.copyfile(ROOT/'src/physics/native_tu15_mopp_aabb_traversal.cpp',project/'traversal.cpp');shutil.copyfile(ROOT/'src/physics/native_tu15_array_growth.cpp',project/'growth.cpp');(project/'types.cpp').write_text('');shutil.copyfile")
 text=text.replace("+'.cpp types.cpp)","+'.cpp types.cpp traversal.cpp growth.cpp)")
 text=text.replace('NativeTu15MoppAabbSetupServices services;', 'NativeTu15MoppAabbSetupServices services;')
'''
 source=source[:position]+inject+source[position:]
 # Add the traversal API at the generated-driver-code layer. repr keeps
 # its intended literal newline escape intact through both Python parsers.
 old_header='#include "halo2/native_tu15_mopp_aabb_setup.hpp"'
 new_header=old_header+'\\n#include "halo2/native_tu15_mopp_aabb_traversal.hpp"'
 marker=' text=text.replace("shutil.copyfile'
 position=source.index(marker)
 source=source[:position]+' injected=injected.replace('+repr(old_header)+','+repr(new_header)+')\n'+source[position:]
 position=source.index(' env=dict(base.__dict__')
 source=source[:position]+''' services='''+repr('''   services.traverse=[&](auto cache,auto program,const auto&variables,auto&fp,auto depth){NativeTu15MoppAabbTraversalServices native;native_tu15_traverse_mopp_aabb(m,cache,program,variables,fp,depth,native,observer);};
''')+'\n'+source[position:]
 env=dict(parent.__dict__,__file__=__file__,metadata=metadata,cases=cases,OWNERS=OWNERS)
 exec(source,env);return env['main']()
if __name__=='__main__':raise SystemExit(main())
