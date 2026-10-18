"""Single-step owned host i386 code with actual native resource backing.

No original opcode is patched. The declared animation I/O entry is handled by
the same independently restored cache owner as the ordinary host run. All
other reached instructions execute on the host CPU without returned results.
"""
import ctypes,os,signal,struct
from pathlib import Path
from unicorn import x86_const as R
from original_audio_cpu import Registers


def trace_me():
    libc=ctypes.CDLL(None,use_errno=True)
    if libc.ptrace(0,0,None,None):os._exit(125)


class HostCpu:
    aliases={R.UC_X86_REG_EAX:'rax',R.UC_X86_REG_ECX:'rcx',R.UC_X86_REG_ESP:'rsp',
             R.UC_X86_REG_EDI:'rdi',R.UC_X86_REG_EIP:'rip',R.UC_X86_REG_ESI:'rsi',
             R.UC_X86_REG_EBP:'rbp',R.UC_X86_REG_EBX:'rbx'}
    def __init__(self,child):
        self.child=child;self.libc=ctypes.CDLL(None,use_errno=True)
        self.libc.ptrace.argtypes=[ctypes.c_uint,ctypes.c_uint,ctypes.c_void_p,ctypes.c_void_p]
        self.libc.ptrace.restype=ctypes.c_long;self.memory=None
    def request(self,operation,address=0,data=0):
        ctypes.set_errno(0)
        result=self.libc.ptrace(operation,self.child.pid,ctypes.c_void_p(address),ctypes.c_void_p(data))
        error=ctypes.get_errno()
        if result==-1 and error:raise OSError(error,os.strerror(error))
        return result
    def wait(self):
        pid,status=os.waitpid(self.child.pid,0);assert pid==self.child.pid
        assert os.WIFSTOPPED(status) and os.WSTOPSIG(status)==signal.SIGTRAP,hex(status)
    def registers(self):
        result=Registers();self.request(12,0,ctypes.addressof(result));return result
    def reg_read(self,register):return getattr(self.registers(),self.aliases[register])&0xffffffff
    def reg_write(self,register,value):
        regs=self.registers();setattr(regs,self.aliases[register],value&0xffffffff)
        self.request(13,0,ctypes.addressof(regs))
    def mem_read(self,address,size):
        data=os.pread(self.memory,size,address);assert len(data)==size
        return data
    def mem_write(self,address,data):
        assert address>=0x4000000,'No original executable bytes may be written by this resource adapter'
        assert os.pwrite(self.memory,data,address)==len(data)
    def close(self):
        if self.memory is not None:os.close(self.memory);self.memory=None


def execute(child,original,codes,resources,output,interface_language=None):
    cpu=HostCpu(child);trace=[];writes=[];previous=None
    try:
        cpu.wait();cpu.request(0x4200,0,0x100000) # Own child: PTRACE_O_EXITKILL.
        # The native x86_64 Linux struct user debug-register offset is the
        # same asserted ABI used by the repository's original audio runner.
        cpu.request(6,848,original['entry']);cpu.request(6,848+7*8,1)
        cpu.request(7);cpu.wait();regs=cpu.registers()
        assert regs.cs==0x23 and regs.rip==original['entry']
        cpu.request(6,848+7*8,0);cpu.request(6,848+6*8,0)
        cpu.memory=os.open(f'/proc/{child.pid}/mem',os.O_RDWR)
        fs_selector=regs.fs>>3;descriptor=ctypes.create_string_buffer(16)
        cpu.request(25,fs_selector,ctypes.addressof(descriptor))
        fs_base=struct.unpack_from('<I',descriptor.raw,4)[0]
        assert fs_base==0x60200000
        aliases={'eax':'rax','ebx':'rbx','ecx':'rcx','edx':'rdx','esi':'rsi','edi':'rdi','ebp':'rbp','esp':'rsp'}
        def register(name):
            if not name:return 0
            assert name in aliases,name
            return getattr(regs,aliases[name])&0xffffffff
        def store(address,payload):
            old=cpu.mem_read(address,len(payload));writes.append((previous,address,old,payload));cpu.mem_write(address,payload)
        admitted=original['executed_instruction_bytes']
        for count in range(1000000):
            regs=cpu.registers();pc=regs.rip
            if pc==0x60300000:break
            if pc==0x33a4a6 and interface_language:
                interface_language.apply(cpu,previous);continue
            if pc==0x1a3b70:
                if hasattr(resources,'resource'):resources.resource(cpu,previous,store)
                else:resources.animation(cpu,previous,store)
                continue
            assert f'{pc:08x}' in admitted,f'New hardware branch requires original CFG admission: {pc:08x}'
            code=next(code for code in codes if code.inside(pc));instruction=code.at(pc)
            assert bytes(instruction.bytes)==cpu.mem_read(pc,len(instruction.bytes))
            assert bytes(instruction.bytes).hex()==admitted[f'{pc:08x}']
            trace.append(pc);pending=[]
            for index,operand in enumerate(instruction.operands):
                x87_store=instruction.mnemonic in ('fst','fstp','fist','fistp','fisttp')
                # Capstone also labels the destination of0F17 MOVHPS as a
                # read. Its actual original opcode and memory-first form
                # establish a store; no CPU operation is substituted.
                raw=bytes(instruction.bytes)
                explicit_store=index==0 and (
                    (instruction.mnemonic=='movhps' and raw.startswith(b'\x0f\x17')) or
                    (instruction.mnemonic=='movlps' and raw.startswith(b'\x0f\x13')) or
                    (instruction.mnemonic=='movq' and raw.startswith(b'\x0f\x7f')))
                if operand.type!=3 or not (operand.access&2 or x87_store or explicit_store):continue
                # A zero-count REP executes without accessing its operand.
                if instruction.mnemonic.startswith('rep ') and regs.rcx&0xffffffff==0:continue
                mem=operand.mem;segment=instruction.reg_name(mem.segment) or ''
                assert segment in ('','fs','ds','ss','es'),segment
                address=(register(instruction.reg_name(mem.base))+register(instruction.reg_name(mem.index))*mem.scale+mem.disp+(fs_base if segment=='fs' else 0))&0xffffffff
                if 0x60000000<=address<0x60200000:continue
                pending.append((address,operand.size,cpu.mem_read(address,operand.size)))
            previous=pc;cpu.request(9);cpu.wait()
            for address,size,before in pending:writes.append((pc,address,before,cpu.mem_read(address,size)))
        else:raise RuntimeError('Independent host trace exceeded its instruction limit')
        cpu.request(7);pid,status=os.waitpid(child.pid,0);assert pid==child.pid
        assert os.WIFEXITED(status),hex(status)
        child.returncode=os.WEXITSTATUS(status)
        (output/'hardware-trace.bin').write_bytes(b''.join(struct.pack('<I',pc) for pc in trace))
        with (output/'hardware-stores.bin').open('wb') as stream:
            for pc,address,before,after in writes:
                stream.write(struct.pack('<III',pc,address,len(after)));stream.write(before);stream.write(after)
        return dict(instructions=len(trace),stores=len(writes),original_opcodes_patched=False,
            native_IO_serviced_by='independently restored parent cache owner at the declared native boundary',
            source_GP_FP_estimates_replaced=False)
    finally:cpu.close()
