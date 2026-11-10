"""Read-only original observer extension for the predicate's live x87 prefixes.

Only the disposable diagnostic observer gains a returned register-file field.
The original program, metadata instructions and input/outcome ownership are
unchanged. The shared observer/helper files are never edited.
"""
import hashlib
from pathlib import Path
from validate_native_tu15_prt_helpers import physical_original
OBSERVER=Path(__file__).resolve().parent/'observe_native_campaign_ai_vector_math_tu15.cpp'
OBSERVER_PIN='2251db1c3798ffaf339662f94c980efcca1d3e022491ace9e0a1824bebb9605e'
MARKER='if(c.contains("stop_before_entries"))result.back()["stopped_at"]=std::uint32_t(regs.rip);'
EXTENSION='''{
 const unsigned boundary_top=(fp.swd>>11u)&7u;unsigned boundary_depth=0;
 while(boundary_depth<8&&(fp.ftw&(1u<<((boundary_top+boundary_depth)&7u))))++boundary_depth;
 for(unsigned slot=boundary_depth;slot<8;++slot)check(!(fp.ftw&(1u<<((boundary_top+slot)&7u))),"Original geometry prefix has sparse x87 topology");
 J live=J::array();for(unsigned slot=0;slot<boundary_depth;++slot)live.push_back(hex(std::span(reinterpret_cast<const std::byte*>(fp.st_space)+slot*16,10)));
 result.back()["boundary_live_x87"]=std::move(live);
}'''
def observe(image,rows,scratch,**options):
    original_reader=Path.read_text
    def read(path,*args,**kwargs):
        text=original_reader(path,*args,**kwargs)
        if path.resolve()==OBSERVER:
            if hashlib.sha256(text.encode()).hexdigest()!=OBSERVER_PIN or text.count(MARKER)!=1:raise ValueError('Complete original geometry diagnostic observer drift')
            return text.replace(MARKER,EXTENSION+MARKER,1)
        return text
    Path.read_text=read
    try:result,authority=physical_original(image,rows,scratch,**options)
    finally:Path.read_text=original_reader
    authority.update(extended_boundary_scope='Dense logical 80-bit values observed at the actual original boundary; diagnostic-only read extension',boundary_observer_base_sha256=OBSERVER_PIN,boundary_observer_extension_sha256=hashlib.sha256(EXTENSION.encode()).hexdigest())
    return result,authority
