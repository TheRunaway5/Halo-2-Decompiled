"""Real cache I/O at the existing native animation/geometry/texture boundaries.

Each original source caller must own its descriptor, tracking record and cache
pages before actual sectors are read. Completion and a fresh retained job are
published only after the real read and destination write. Xbox async worker
scheduling and GPU residency are outside this explicit native resource ABI.
"""
import hashlib
from unicorn import x86_const as R
from shadow_native_animation_resource_tu15 import NativeShadowAnimationResources


class NativeShadowWeaponResources(NativeShadowAnimationResources):
    caller_sites=(0x126139,0x130f68,0x1301b0)

    def resource(self,cpu,caller_instruction,store):
        if caller_instruction==0x126139:return self.animation(cpu,caller_instruction,store)
        u=lambda address:int.from_bytes(cpu.mem_read(address,4),'little')
        sp=cpu.reg_read(R.UC_X86_REG_ESP);tracking=cpu.reg_read(R.UC_X86_REG_ESI)
        size=u(sp+4);destination=u(sp+8);encoded=cpu.reg_read(R.UC_X86_REG_ECX);returned=u(sp)
        assert size and size&511==0 and u(sp+12)==0
        if caller_instruction==0x130f68:
            descriptor=cpu.reg_read(R.UC_X86_REG_EBP);pool=u(0x4f2d5c);rows=u(pool+0x44)
            cache=u(0x4f2d6c);pages=u(cache+0x64);token=u(descriptor+0x20);index=token&65535
            page=u(pages+0x44)+index*0x18;shift=u(cache+0x34)&31
            expected=(u(0x4f2d64)+(u(page+8)<<shift))&0xffffffff
            assert returned==0x130f6d and tracking>=rows and (tracking-rows)%12==0
            assert (tracking-rows)//12<u(pool+0x38) and u(tracking+8)==descriptor
            assert token!=0xffffffff and index<u(pages+0x38) and destination==expected
            assert encoded==u(descriptor) and size==((u(descriptor+4)+511)&~511)
            assert u(sp+16)==tracking+2 and u(sp+20)==4 and u(sp+24)==cpu.reg_read(R.UC_X86_REG_EBX)
            done=tracking+2;kind='geometry'
        elif caller_instruction==0x1301b0:
            descriptor=u(tracking+0x10);level=u(tracking+4);pool=u(0x4f2d24);rows=u(pool+0x44)
            assert returned==0x1301b5 and level<3 and tracking>=rows and (tracking-rows)%0x28==0
            assert (tracking-rows)//0x28<u(pool+0x38)
            assert encoded==u(descriptor+0x1c+level*4) and size==((u(descriptor+0x34+level*4)+511)&~511)
            assert destination==u(descriptor+0x40+level*4) and u(sp+16)==tracking+3 and u(sp+20)==3
            done=tracking+3;kind='texture'
        else:raise RuntimeError('Unowned native resource caller; no result supplied')
        result=self.request(dict(op='read-sectors',descriptor=descriptor,destination=destination,bytes=size,encoded_offset=encoded))
        payload=bytes.fromhex(result['payload_hex']);assert len(payload)==size and not result['descriptor_hex']
        store(destination,payload);job=self.request(dict(op='commit'))['job'];store(done,b'\x01')
        self.events.append(dict(kind='actual native synchronous '+kind+' resource read',caller_instruction=caller_instruction,
            target=0x1a3b70,returned=returned,descriptor=descriptor,destination=destination,bytes=size,
            encoded_offset=encoded,completion_address=done,payload_sha256=hashlib.sha256(payload).hexdigest(),
            allocated_job=job,actual_cache_catalog=self.identity,source_return_supplied=True,
            ABI='EAX actual retained job; consume28 stack bytes; preserve other GP/FP/flags',
            original_async_queue_worker_verified=False,Xbox_kernel_instructions_or_clobbers_verified=False))
        cpu.reg_write(R.UC_X86_REG_EAX,job);cpu.reg_write(R.UC_X86_REG_ESP,sp+28);cpu.reg_write(R.UC_X86_REG_EIP,returned)
