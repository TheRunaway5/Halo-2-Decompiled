"""Real native wait backing for a declared reference-CPU platform boundary.

Only a captured quiescent ownership checkpoint can restore aliases. A zero-time
probe must actually acquire the object before an infinite source wait receives
STATUS_SUCCESS. A pending/failed probe stops original execution without a source
return. This is host synchronization policy, not Xbox kernel instruction proof.
"""
import ctypes,hashlib,json
from pathlib import Path
from unicorn import x86_const as R


def sha(p):
    with Path(p).open('rb') as stream:return hashlib.file_digest(stream,'sha256').hexdigest()


class NativeShadowSynchronization:
    def __init__(self,checkpoint,library):
        self.events=[];self.owner=None
        self.checkpoint=Path(checkpoint);self.library=Path(library)
        data=json.loads(self.checkpoint.read_text());assert data['exported'] and data['version']==1
        manifest=json.loads((self.library.parent/'manifest.json').read_text())
        assert any(Path(name).resolve()==self.library.resolve() and pin==sha(self.library)
                   for name,pin in manifest['files'].items())
        self.native=ctypes.CDLL(str(self.library.resolve()))
        u32=ctypes.c_uint32;u64=ctypes.c_uint64;ptr=ctypes.c_void_p
        self.native.shadow_sync_restore.argtypes=[u32,u64,u64,ctypes.POINTER(u32),u32]
        self.native.shadow_sync_restore.restype=ptr
        self.native.shadow_sync_wait.argtypes=[ptr,u32,u32,ctypes.POINTER(ctypes.c_int)]
        self.native.shadow_sync_wait.restype=ctypes.c_int
        self.native.shadow_sync_release_mutex.argtypes=[ptr,u32];self.native.shadow_sync_release_mutex.restype=ctypes.c_int
        self.native.shadow_sync_release_semaphore.argtypes=[ptr,u32,ctypes.c_int32,ctypes.POINTER(ctypes.c_int32)]
        self.native.shadow_sync_release_semaphore.restype=ctypes.c_int
        self.native.shadow_sync_destroy.argtypes=[ptr];self.native.shadow_sync_destroy.restype=None
        self.native.shadow_sync_error.argtypes=[];self.native.shadow_sync_error.restype=ctypes.c_char_p
        rows=[word&0xffffffff for obj in data['objects']for word in (obj['handle'],obj['kind'],obj['count'],obj['limit'])]
        backing=(u32*len(rows))(*rows)
        self.owner=self.native.shadow_sync_restore(data['first'],data['next'],data['limit'],backing,len(data['objects']))
        assert self.owner,self.native.shadow_sync_error().decode()
        self.handles={obj['handle']for obj in data['objects']}

    def close(self):
        if self.owner:self.native.shadow_sync_destroy(self.owner);self.owner=None

    def kernel(self,cpu,pc):
        sp=cpu.reg_read(R.UC_X86_REG_ESP)
        u=lambda p:int.from_bytes(cpu.mem_read(p,4),'little')
        caller=u(sp)
        if pc==0x800000ea:
            handle,mode,alertable,timeout=[u(sp+4+4*n)for n in range(4)]
            assert handle in self.handles,'Wait alias has no captured native owner'
            assert mode in (0,1) and alertable==0 and timeout==0,'This actual adapter admits only the observed nonalertable infinite wait'
            error=ctypes.c_int();state=self.native.shadow_sync_wait(self.owner,handle,0,ctypes.byref(error))
            self.events.append(dict(kernel=pc,caller=caller,arguments=[handle,mode,alertable,timeout],
                actual_state=state,native_error=error.value,probe_timeout_ms=0,
                source_return_supplied=state==0 and error.value==0))
            if state!=0 or error.value:
                raise RuntimeError('Actual native infinite-wait backing is pending/failed; original receives no return')
            count=4
        elif pc==0x800000dd:
            handle,previous=[u(sp+4+4*n)for n in range(2)]
            assert handle in self.handles and previous==0,'Unresolved actual mutant previous-count output'
            error=self.native.shadow_sync_release_mutex(self.owner,handle)
            self.events.append(dict(kernel=pc,caller=caller,arguments=[handle,previous],
                native_error=error,source_return_supplied=error==0))
            if error:raise RuntimeError('Actual native mutex release failed; original receives no return')
            count=2
        elif pc==0x800000de:
            handle,count_value,previous=[u(sp+4+4*n)for n in range(3)]
            assert handle in self.handles and count_value<0x80000000,'Unresolved actual semaphore release'
            output=ctypes.c_int32()
            error=self.native.shadow_sync_release_semaphore(self.owner,handle,count_value,ctypes.byref(output))
            self.events.append(dict(kernel=pc,caller=caller,arguments=[handle,count_value,previous],
                native_error=error,actual_previous_count=output.value if not error else None,source_return_supplied=error==0))
            if error:raise RuntimeError('Actual native semaphore release failed; original receives no return')
            if previous:cpu.mem_write(previous,int(output.value&0xffffffff).to_bytes(4,'little'))
            count=3
        else:raise RuntimeError(f'Unimplemented original kernel boundary {pc:08x}; no result supplied')
        cpu.reg_write(R.UC_X86_REG_EAX,0)
        cpu.reg_write(R.UC_X86_REG_ESP,sp+4+4*count)
        cpu.reg_write(R.UC_X86_REG_EIP,caller)
