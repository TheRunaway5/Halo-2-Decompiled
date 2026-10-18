"""Authenticated physical RAM, real page tables and CPU state for pose fixtures."""
import contextlib
import hashlib
import importlib.util
import json
import mmap
from pathlib import Path
import re
import struct

ROOT=Path(__file__).absolute().parents[1]
BASE=ROOT/'analysis/weapons-tu15-goal-20261013'

def sha(path):
    with Path(path).open('rb')as source:return hashlib.file_digest(source,'sha256').hexdigest()

def cpu(case,label):
    value=json.loads((case/(label+'-cpu.json')).read_text())
    stub=(case/'xemu-gdbstub.c').read_text();dump=(case/'xemu-cpu-dump.c').read_text()
    assert 'return gdb_get_reg32(mem_buf, 0); /* ftag */'in stub
    assert 'fptag |= ((!env->fptags[i]) << i);'in dump
    monitor=value['monitor_registers'];ftw=int(re.search(r'\bFTW=([0-9a-fA-F]+)',monitor).group(1),16)
    physical={int(i):struct.pack('<QH',int(lo,16),int(hi,16)).hex()
              for i,lo,hi in re.findall(r'FPR(\d)=([0-9a-fA-F]{16}) ([0-9a-fA-F]{4})',monitor)}
    assert set(physical)==set(range(8))
    top=(value['x87_status']>>11)&7
    assert all(value['x87_register_bytes'][i]==physical[(i+top)&7]for i in range(8))
    # This observed call has an empty file. Do not synthesize classifications
    # for nonempty tags; those need a separate raw-register classification audit.
    assert ftw==0
    value['debugger_placeholder_x87_tag']=value['x87_tag']
    value['x87_tag']=0xffff
    value['x87_tag_authority']='Pinned xemu monitor FTW: bit1=nonempty; all observed physical tags empty'
    value['monitor_abridged_tag']=ftw
    gdt=re.search(r'^GDT=\s+([0-9a-fA-F]+)\s+([0-9a-fA-F]+)',monitor,re.M);assert gdt
    value['gdtr']=[int(gdt.group(1),16),int(gdt.group(2),16)]
    value['selectors']={name.lower():int(selector,16)for name,selector in re.findall(r'^(ES|CS|SS|DS|FS|GS)\s*=\s*([0-9a-fA-F]+)',monitor,re.M)}
    return value

@contextlib.contextmanager
def physical_ram(case,label):
    index=json.loads((case/'physical-memory-retention.json').read_text())['files'][label]
    spec=importlib.util.spec_from_file_location('content',BASE/'archive-content-retention-v1.py')
    content=importlib.util.module_from_spec(spec);spec.loader.exec_module(content)
    recipe=Path(index['content_recipe']);assert sha(recipe)==index['content_recipe_sha256']
    with mmap.mmap(-1,index['raw_bytes'])as bank:
        assert content.reconstruct(index['raw_sha256'],bank)==index['raw_bytes']
        bank.seek(0);yield bank

@contextlib.contextmanager
def reconstructed_replay_ram(case,oracle):
    """Reconstruct an observed independent boundary, never a supplied return.

    CPU paging traces use physical addresses. Earlier alias traces use captured
    virtual addresses. Every old byte and the available whole-bank boundary
    hash are checked; implicit CPU RAM updates are explicitly interleaved.
    """
    report=json.loads((oracle/'report.json').read_text())
    rows=json.loads((oracle/'writes.json').read_text())
    assert len(rows)==report['ordered_source_writes']
    updates={}
    for update in report.get('implicit_CPU_RAM_updates',[]):
        updates.setdefault(update['before_source_write_index'],[]).append(update)
    with physical_ram(case,'before')as bank:
        memory=PageTableMemory(bank,cpu(case,'before'))
        physical=report.get('write_and_read_trace_address_space','virtual')=='physical'
        for index in range(len(rows)+1):
            for update in updates.get(index,[]):
                address=update['address'];old=bytes.fromhex(update['before']);new=bytes.fromhex(update['written'])
                assert len(old)==len(new)and bank[address:address+len(old)]==old
                bank[address:address+len(old)]=new
            if index==len(rows):break
            row=rows[index];address=row['address'];size=row['bytes'];raw=bytes.fromhex(row['written'])
            assert len(raw)==size
            assert (bank[address:address+size]if physical else memory.read(address,size)).hex()==row['before']
            offset=0
            while offset<size:
                destination=address+offset if physical else memory.translate(address+offset)[0]
                length=min(size-offset,4096-((address+offset)&4095))
                assert destination+length<=len(bank)
                bank[destination:destination+length]=raw[offset:offset+length];offset+=length
        digest=report.get('complete_intermediate_RAM_reconstruction_sha256')
        if digest:assert hashlib.sha256(bank).hexdigest()==digest
        yield bank,memory,report

class PageTableMemory:
    """The captured 32-bit non-PAE mapping, backed by one physical RAM bank."""
    def __init__(self,bank,state):
        self.bank=bank;self.state=state
        control=state['control_registers']
        assert control['CR0']&0x80000000 and not control['CR4']&0x20
        self.cr3=control['CR3']&0xfffff000;self.pse=bool(control['CR4']&16)

    def word(self,physical):
        assert physical+4<=len(self.bank)
        return struct.unpack_from('<I',self.bank,physical)[0]

    def translate(self,address):
        assert 0<=address<=0xffffffff
        directory=self.word(self.cr3+(address>>22)*4)
        if not directory&1:raise LookupError(f'Unmapped original PDE for {address:08x}')
        if directory&128:
            assert self.pse
            physical=(directory&0xffc00000)+(address&0x3fffff);writable=bool(directory&2)
        else:
            table=self.word((directory&0xfffff000)+((address>>12)&1023)*4)
            if not table&1:raise LookupError(f'Unmapped original PTE for {address:08x}')
            physical=(table&0xfffff000)+(address&4095);writable=bool(directory&table&2)
        assert physical<len(self.bank),f'Original RAM fixture accessed non-RAM physical address {physical:08x}'
        return physical,writable

    def read(self,address,size):
        result=bytearray()
        while size:
            physical,_=self.translate(address);length=min(size,4096-(address&4095))
            assert physical+length<=len(self.bank)
            result.extend(self.bank[physical:physical+length]);address+=length;size-=length
        return bytes(result)
