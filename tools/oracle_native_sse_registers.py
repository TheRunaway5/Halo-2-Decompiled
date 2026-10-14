"""Offline hardware backend for TU1.5 scalar SSE arithmetic/status.

Unicorn's x86 SSE operations omit MXCSR sticky exception updates. Execute the
authenticated operation on the native x86-64 CPU; verify Unicorn's result lanes
and import the hardware status. Register instructions execute byte-for-byte.
An absolute scalar memory operand is copied from actual emulated memory into
retained scratch and its hardware addressing is re-encoded for that scratch.
Original emulated game instructions and operands remain unchanged. Never used
by the game/runtime and never executes a code section.
"""
import ctypes,mmap,platform,struct
class NativeSseRegisters:
 def __init__(self):
  assert platform.machine() in ('x86_64','AMD64')
  self.cache={};self.libc=ctypes.CDLL(None,use_errno=True)
  self.libc.mprotect.argtypes=[ctypes.c_void_p,ctypes.c_size_t,ctypes.c_int]
 def evaluate(self,instruction,gp,xmm,mxcsr,read_scalar=None):
  assert instruction.mnemonic in ('cvtsi2ss','mulss','divss','addss','subss')
  opcode=bytes(instruction.bytes)
  scalar=b'\0'*4;hardware_opcode=opcode
  if all(o.type==1 for o in instruction.operands):
   assert all(instruction.reg_name(o.reg)in ('eax','ecx',*(f'xmm{i}'for i in range(8))) for o in instruction.operands)
  else:
   assert instruction.mnemonic in ('mulss','divss','addss','subss') and len(instruction.operands)==2
   destination,source=instruction.operands
   assert destination.type==1 and instruction.reg_name(destination.reg)in tuple(f'xmm{i}'for i in range(8))
   assert source.type==3 and source.size==4 and not source.mem.base and not source.mem.index and not source.mem.segment
   assert len(opcode)==8 and opcode[:2]==b'\xf3\x0f' and opcode[3]&0xc7==5
   assert int.from_bytes(opcode[4:],'little')==source.mem.disp&0xffffffff and read_scalar is not None
   scalar=bytes(read_scalar(source.mem.disp&0xffffffff,4));assert len(scalar)==4
   # SSE arithmetic/lanes/status are unchanged by relocating its read-only
   # operand. R11 already owns our retained buffer; scalar scratch is +164.
   hardware_opcode=b'\xf3\x41\x0f'+opcode[2:3]+bytes([0x83|(opcode[3]&0x38)])+struct.pack('<I',164)
  if opcode not in self.cache:
   # R11 retains the buffer pointer. Host ABI callee-saved GP registers are
   # untouched; EAX/ECX and XMM0..7 are caller-saved on the System V ABI.
   code=bytearray.fromhex('4883ec080fae1c244989fb410fae93a0000000418b03418b4b04')
   for i in range(8):code+=bytes([0xf3,0x41,0x0f,0x6f,0x83+8*i])+struct.pack('<I',32+16*i)
   code+=hardware_opcode+bytes.fromhex('410fae9ba0000000')
   for i in range(8):code+=bytes([0xf3,0x41,0x0f,0x7f,0x83+8*i])+struct.pack('<I',32+16*i)
   code+=bytes.fromhex('0fae14244883c408c3')
   pages=mmap.mmap(-1,4096,prot=mmap.PROT_READ|mmap.PROT_WRITE);pages.write(code)
   address=ctypes.addressof(ctypes.c_char.from_buffer(pages))
   if self.libc.mprotect(address,4096,mmap.PROT_READ|mmap.PROT_EXEC):raise OSError(ctypes.get_errno(),'native SSE reference protection failed')
   self.cache[opcode]=(pages,ctypes.CFUNCTYPE(None,ctypes.c_void_p)(address))
  raw=struct.pack('<8I',*gp)+b''.join(v.to_bytes(16,'little')for v in xmm)+struct.pack('<I',mxcsr)+scalar
  buffer=ctypes.create_string_buffer(raw,len(raw));self.cache[opcode][1](ctypes.addressof(buffer));result=buffer.raw
  return [int.from_bytes(result[32+i*16:48+i*16],'little')for i in range(8)],struct.unpack_from('<I',result,160)[0]
