"""Replay the port's declared English interface-language input.

The actual compiled World supplies this platform configuration at the two
authenticated XGetLanguage callers. It does not query a console EEPROM. No
kernel settings status or gameplay predicate is supplied by this adapter.
"""
import hashlib,json,re
from pathlib import Path
from unicorn import x86_const as R


def sha(path):
    with Path(path).open('rb') as stream:return hashlib.file_digest(stream,'sha256').hexdigest()


class NativeShadowInterfaceLanguage:
    def __init__(self,case):
        build=json.loads((Path(case)/'build.json').read_text());found=[]
        for object_,record in build['override_sources'].items():
            source=Path(record['source'])
            if 'world' not in source.name:continue
            text=source.read_text();anchor='if((c.instruction==0x1a9da2u || c.instruction==0x1a82abu) && c.target==0x33a4a6u && !c.tail) {'
            if anchor not in text:continue
            assert sha(source)==record['source_sha256']
            assert sha(object_)==build['linked_files_sha256'][object_]
            body=text.split(anchor,1)[1].split('\n        }',1)[0]
            match=re.search(r'a\.gp\[0\]=(\d+);a\.gp\[4\]\+=4u;return;',body)
            assert match and 'TU1.5 native language caller differs' in body
            raw=int(match[1]);assert raw==1 and 'English' in body
            found.append((source,record['source_sha256'],object_,build['linked_files_sha256'][object_],raw))
        assert len(found)==1,'Actual compiled native language configuration unavailable'
        self.source,self.source_sha,self.object,self.object_sha,self.raw=found[0];self.events=[]

    def apply(self,cpu,caller_instruction):
        assert caller_instruction in (0x1a9da2,0x1a82ab)
        sp=cpu.reg_read(R.UC_X86_REG_ESP)
        returned=int.from_bytes(cpu.mem_read(sp,4),'little')
        assert returned==caller_instruction+5
        # This is the real configured native interface locale, not a return
        # sampled from a gameplay function or an expected after-bank.
        cpu.reg_write(R.UC_X86_REG_EAX,self.raw)
        cpu.reg_write(R.UC_X86_REG_ESP,sp+4);cpu.reg_write(R.UC_X86_REG_EIP,returned)
        self.events.append(dict(kind='actual native interface-language input',target=0x33a4a6,
            caller_instruction=caller_instruction,returned=returned,raw_language=self.raw,locale='English',
            native_configuration_source=str(self.source),native_configuration_source_sha256=self.source_sha,
            native_configuration_object=str(self.object),native_configuration_object_sha256=self.object_sha,
            ABI='EAX configured native interface language; consume return address; preserve other GP/FP/flags',
            source_return_supplied=True,Xbox_EEPROM_or_XGetLanguage_body_verified=False))
