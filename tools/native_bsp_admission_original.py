"""Disposable read-only occurrence-aware CPU instruction boundaries.

No original instruction, callee result, metadata access or bank is changed.
This extends only the pinned diagnostic observer. In particular an atomic
RMW stop happens before its operand read, rather than dropping a real native
provider read to imitate the CPU's unexecuted transaction.
"""
import hashlib
from pathlib import Path
from validate_native_tu15_prt_helpers import physical_original
from native_geometry_visibility_original import OBSERVER,OBSERVER_PIN,MARKER,EXTENSION
DECLARATION='  const auto max_steps=c.value("max_steps",5000u);'
BOUNDARY='   J pending_reads=J::array(),pending_writes=J::array();bool stop_before{};'
STOP='''   if(phase=="before_instruction"&&pc==fault.at("instruction").get<std::uint32_t>()&&instruction_occurrences[pc]++==ordinal){boundary=J{{"phase",phase},{"instruction",pc}};break;}
'''
def observe(image,rows,scratch,**options):
    original_reader=Path.read_text
    def read(path,*args,**kwargs):
        text=original_reader(path,*args,**kwargs)
        if path.resolve()==OBSERVER:
            if hashlib.sha256(text.encode()).hexdigest()!=OBSERVER_PIN:raise ValueError('Complete BSP original diagnostic observer drift')
            for needle in(DECLARATION,BOUNDARY,MARKER):
                if text.count(needle)!=1:raise ValueError('Exact BSP observer extension seam drift')
            text=text.replace(DECLARATION,'  std::map<std::uint32_t,unsigned>instruction_occurrences;\n'+DECLARATION,1).replace(BOUNDARY,STOP+BOUNDARY,1).replace(MARKER,EXTENSION+MARKER,1)
        return text
    Path.read_text=read
    try:result,authority=physical_original(image,rows,scratch,**options)
    finally:Path.read_text=original_reader
    authority.update(diagnostic_base_sha256=OBSERVER_PIN,occurrence_boundary_extension_sha256=hashlib.sha256((DECLARATION+BOUNDARY+STOP+EXTENSION).encode()).hexdigest(),boundary_scope='Occurrence-aware original beforeInstruction RMW stop before any architectural operand access; dense logical80 observed read-only; original text unmodified')
    return result,authority
