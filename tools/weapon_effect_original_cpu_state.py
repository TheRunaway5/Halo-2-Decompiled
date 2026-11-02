"""Declared caller FP files and read-only snapshots for owned i386 references."""
import ctypes
import struct
from oracle_weapon_effect_state_tu15 import ALIASES,DEBUG_FLAGS
from oracle_weapon_effect_dispatch_tu15 import STACK,NONE


class EffectCpuState:
    def __init__(self,host):self.host=host;self.depth=0;self.mmx=False

    def floating_point(self):
        buffer=ctypes.create_string_buffer(512);self.host.trace(14,0,ctypes.addressof(buffer));raw=buffer.raw
        result=dict(cw=struct.unpack_from('<H',raw,0)[0],status=struct.unpack_from('<H',raw,2)[0],mxcsr=struct.unpack_from('<I',raw,24)[0],
                    stack=[raw[32+i*16:42+i*16].hex() for i in range(self.depth)],xmm=[raw[160+i*16:176+i*16].hex() for i in range(8)])
        if self.mmx:
            assert raw[4]==255 and not result['status']&0x3800,'Original MMX tags/TOP changed'
            result['mmx']=[raw[32+i*16:42+i*16].hex() for i in range(8)]
        return result

    def load(self,fp):
        self.depth=len(fp['stack']);self.mmx='mmx' in fp;top=(fp['status']>>11)&7
        buffer=ctypes.create_string_buffer(512);self.host.trace(14,0,ctypes.addressof(buffer));raw=bytearray(buffer.raw)
        struct.pack_into('<HHH',raw,0,fp['cw'],fp['status'],sum(1<<((top+i)&7) for i in range(self.depth)))
        struct.pack_into('<I',raw,24,fp['mxcsr'])
        for i,value in enumerate(fp['stack']):raw[32+i*16:42+i*16]=bytes.fromhex(value)
        if self.mmx:
            assert not self.depth and top==0
            struct.pack_into('<H',raw,4,255)
            for i,value in enumerate(fp['mmx']):raw[32+i*16:42+i*16]=bytes.fromhex(value)
        for i,value in enumerate(fp['xmm']):raw[160+i*16:176+i*16]=bytes.fromhex(value)
        buffer=ctypes.create_string_buffer(bytes(raw),512);self.host.trace(15,0,ctypes.addressof(buffer))

    def snapshot(self,regs):
        return dict(gp=[getattr(regs,n)&NONE for n in ALIASES],eflags=regs.eflags&~DEBUG_FLAGS,
                    fp=self.floating_point(),stack=self.host.read(STACK-96,160).hex())
