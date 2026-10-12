"""Run owned original32-bit audio CPU instructions on a Linux x86 host.

The child ELF maps unchanged XBE section bytes at their original addresses.
Declared ABI backing is supplied separately. Hardware execution breakpoints
stop before Xbox SDK calls or at the caller's return boundary. No source opcode
is patched and no SDK/kernel/callee result is supplied. This proves host CPU
execution of original code, not Xbox OS, DSP or physical-console equivalence.
"""
import ctypes
import os
from pathlib import Path
import signal
import struct


class Registers(ctypes.Structure):
    _fields_=[(name,ctypes.c_ulonglong)for name in
        ('r15','r14','r13','r12','rbp','rbx','r11','r10','r9','r8','rax','rcx','rdx','rsi','rdi',
         'orig_rax','rip','cs','eflags','rsp','ss','fs_base','gs_base','ds','es','fs','gs')]


class OriginalAudioCpu:
    def __init__(self,image,elf_path:Path,control_word:int,mxcsr:int,extra_declared_regions=()):
        if os.uname().machine!='x86_64':raise RuntimeError('Original32-bit host CPU runner requires Linux x86_64')
        self.libc=ctypes.CDLL(None,use_errno=True)
        self.libc.ptrace.argtypes=[ctypes.c_uint,ctypes.c_uint,ctypes.c_void_p,ctypes.c_void_p]
        self.libc.ptrace.restype=ctypes.c_long
        self.pid=None;self.memory_fd=None;self.control_word=control_word;self.mxcsr=mxcsr
        raw=bytearray(0x800000)
        self.code_ranges=[]
        for section in image.sections:
            value=image.section_bytes(section);offset=section.va-0x10000
            assert 0<=offset and offset+len(value)<=len(raw)
            raw[offset:offset+len(value)]=value
            if section.name in ('.text','DSOUND','WMADEC','XGRPH','XONLINE','XNET','D3D','DSOUND','XAPI'):
                self.code_ranges.append((section.va,section.va+len(value)))
        ident=b'\x7fELF\x01\x01\x01'+bytes(9)
        # Optional caller-declared MMIO backing is ordinary host memory, never
        # an emulated device or actual hardware readiness. Existing runners
        # retain their exact three-segment ELF when no extra backing is given.
        declared=list(extra_declared_regions)
        for address,size in declared:
            assert address%4096==size%4096==0 and address>=0x4000000 and size>0
            assert address+size<=0x100000000
        intervals=sorted((a,a+n)for a,n in declared)
        assert all(a[1]<=b[0]for a,b in zip(intervals,intervals[1:]))
        header=struct.pack('<16sHHIIIIIHHHHHH',ident,2,3,1,0x3000000,52,0,0,52,32,3+len(declared),0,0,0)
        segments=[struct.pack('<8I',1,4096,0x10000,0x10000,len(raw),len(raw),7,4096),
                  struct.pack('<8I',1,4096+len(raw),0x1000000,0x1000000,0,0x10000,7,4096),
                  struct.pack('<8I',1,4096+len(raw),0x3000000,0x3000000,0,4096,7,4096)]
        segments.extend(struct.pack('<8I',1,4096+len(raw),address,address,0,size,6,4096)for address,size in declared)
        prefix=header+b''.join(segments)
        assert not elf_path.exists()
        elf_path.write_bytes(prefix+bytes(4096-len(prefix))+raw);elf_path.chmod(0o700)
        child=os.fork()
        if child==0:
            if self.libc.ptrace(0,0,None,None)!=0:os._exit(125)
            try:os.execv(str(elf_path.resolve()),[str(elf_path.resolve())])
            except BaseException:os._exit(126)
        self.pid=child
        try:
            _,status=os.waitpid(child,0)
            if not os.WIFSTOPPED(status)or os.WSTOPSIG(status)!=signal.SIGTRAP:
                raise RuntimeError(f'Original32-bit ELF execution unavailable: wait status={status:#x}')
            self.initial=self.registers()
            if self.initial.cs!=0x23:raise RuntimeError(f'Original CPU child is not in32-bit compatibility mode: CS={self.initial.cs:#x}')
            self.fp=ctypes.create_string_buffer(512);self.trace(14,0,ctypes.addressof(self.fp))
            self.trace(0x4200,0,0x100000) # PTRACE_O_EXITKILL: own child only.
            self.memory_fd=os.open(f'/proc/{child}/mem',os.O_RDWR)
            # Linux x86_64 struct user.u_debugreg offset, independently checked
            # with a compile-time offsetof assertion in the validation harness.
            self.debug_offset=848
            for i,address in enumerate((0x37c668,0x37c6e5,0x37b68b,0x3000000)):
                self.trace(6,self.debug_offset+8*i,address)
            self.trace(6,self.debug_offset+8*7,0x55)
            for section in image.sections:
                value=image.section_bytes(section)
                assert self.read(section.va,len(value))==value
        except BaseException:
            self.close();raise

    def trace(self,request,address=0,data=0):
        ctypes.set_errno(0)
        result=self.libc.ptrace(request,self.pid,ctypes.c_void_p(address),ctypes.c_void_p(data))
        error=ctypes.get_errno()
        if result==-1 and error:raise OSError(error,os.strerror(error))
        return result

    def registers(self):
        result=Registers();self.trace(12,0,ctypes.addressof(result));return result

    def read(self,address,size):
        raw=os.pread(self.memory_fd,size,address)
        assert len(raw)==size
        return raw

    def write(self,address,raw):
        assert all(address+len(raw)<=low or address>=high for low,high in self.code_ranges),'Original source code write forbidden'
        assert os.pwrite(self.memory_fd,raw,address)==len(raw)

    def prepare(self,entry,stack,inputs):
        regs=Registers.from_buffer_copy(bytes(self.initial))
        regs.rip=entry;regs.rsp=stack;regs.eflags=0x202
        for name,value in inputs.items():setattr(regs,name,value&0xffffffff)
        self.trace(13,0,ctypes.addressof(regs))
        context=bytearray(self.fp.raw)
        struct.pack_into('<HHH',context,0,self.control_word,0,0)
        struct.pack_into('<I',context,24,self.mxcsr)
        native_fp=ctypes.create_string_buffer(bytes(context),512)
        self.trace(15,0,ctypes.addressof(native_fp));self.trace(6,self.debug_offset+8*6,0)

    def wait_stop(self):
        _,status=os.waitpid(self.pid,0)
        if not os.WIFSTOPPED(status)or os.WSTOPSIG(status)!=signal.SIGTRAP:
            raise RuntimeError(f'Original CPU execution failed: wait status={status:#x}')
        return self.registers()

    def run(self,entry,stack,**inputs):
        self.prepare(entry,stack,inputs)
        self.trace(7)
        result=self.wait_stop()
        assert result.rip in (0x37c668,0x37c6e5,0x37b68b,0x3000000),hex(result.rip)
        return result

    def run_traced(self,entry,stack,code,store_low,store_size,stop_addresses=None,**inputs):
        self.prepare(entry,stack,inputs);regs=self.registers();stores=[];executed=set()
        stops=(0x37c668,0x37c6e5,0x37b68b,0x3000000) if stop_addresses is None else tuple(stop_addresses)
        if not stops:raise ValueError('Original CPU trace requires an explicit boundary')
        aliases={'eax':'rax','ebx':'rbx','ecx':'rcx','edx':'rdx','esi':'rsi','edi':'rdi','ebp':'rbp','esp':'rsp'}
        def register(name):return 0 if not name else getattr(regs,aliases[name])&0xffffffff
        for _ in range(100000):
            address=regs.rip
            if address in stops:return regs,stores,executed
            instruction=code.at(address);assert instruction
            assert self.read(address,len(instruction.bytes))==instruction.bytes
            executed.add(address);pending=[]
            for operand in instruction.operands:
                # Capstone marks x87 FST/FSTP memory operands as read-only in
                # this version. Their original opcode semantics establish a
                # store; observe it before/after the unchanged single step.
                x87_store=instruction.mnemonic in ('fst','fstp','fist','fistp','fisttp')
                if operand.type!=3 or not (operand.access&2 or x87_store):continue
                mem=operand.mem
                target=(register(instruction.reg_name(mem.base))+register(instruction.reg_name(mem.index))*mem.scale+mem.disp)&0xffffffff
                if store_low<=target and target+operand.size<=store_low+store_size:
                    pending.append((target,operand.size,self.read(target,operand.size)))
            self.trace(9);regs=self.wait_stop() # PTRACE_SINGLESTEP, no opcode patch.
            for target,size,before in pending:
                stores.append((address,target,size,int.from_bytes(before,'little'),int.from_bytes(self.read(target,size),'little')))
        raise RuntimeError('Original CPU instruction limit reached')

    def close(self):
        if self.memory_fd is not None:os.close(self.memory_fd);self.memory_fd=None
        if self.pid is not None:
            try:os.kill(self.pid,signal.SIGKILL)
            except ProcessLookupError:pass
            try:os.waitpid(self.pid,0)
            except ChildProcessError:pass
            self.pid=None

    def __enter__(self):return self
    def __exit__(self,*args):self.close()
