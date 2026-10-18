"""Actual cache reads at the explicitly declared native animation I/O boundary.

The lower Xbox async submission/worker body is outside this contract. No result
is read from an expected after-bank: sectors and job identities have real owners.
"""
import hashlib,json,subprocess
from pathlib import Path
from unicorn import x86_const as R


def sha(path):
    with Path(path).open('rb') as stream:return hashlib.file_digest(stream,'sha256').hexdigest()


class NativeShadowAnimationResources:
    def __init__(self,checkpoint,provider,map_path,shared,xbe):
        self.events=[];self.provider=Path(provider);self.process=None
        manifest=json.loads((self.provider.parent/'manifest.json').read_text())
        for name,digest in manifest['files'].items():assert sha(name)==digest,name
        assert manifest['files'][str(self.provider.absolute())]==sha(self.provider)
        self.process=subprocess.Popen([str(self.provider.absolute()),str(map_path),str(shared),str(xbe),str(checkpoint)],
            stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
        ready=self.receive();assert ready.get('ready'),ready
        self.identity=ready['identity']

    def receive(self):
        line=self.process.stdout.readline()
        if not line:raise RuntimeError('Native resource provider stopped: '+self.process.stderr.read()[-2000:])
        result=json.loads(line)
        if 'error' in result:raise RuntimeError(result['error'])
        return result

    def request(self,value):
        self.process.stdin.write(json.dumps(value)+'\n');self.process.stdin.flush()
        return self.receive()

    def checkpoint(self):return self.request(dict(op='checkpoint'))

    def close(self):
        if self.process:
            self.process.stdin.close()
            try:status=self.process.wait(timeout=10)
            except subprocess.TimeoutExpired:self.process.terminate();self.process.wait();raise
            error=self.process.stderr.read();self.process=None
            if status:raise RuntimeError('Native resource provider failed: '+error[-2000:])

    def animation(self,cpu,caller_instruction,store):
        u=lambda p:int.from_bytes(cpu.mem_read(p,4),'little')
        sp=cpu.reg_read(R.UC_X86_REG_ESP);tracking=cpu.reg_read(R.UC_X86_REG_EDI)
        returned=u(sp);descriptor=u(tracking+12);size=u(sp+4);destination=u(sp+8)
        offset=cpu.reg_read(R.UC_X86_REG_ECX)
        assert caller_instruction==0x126139 and returned==0x12613e
        assert u(sp+12)==0 and u(sp+16)==tracking+4 and u(sp+20)==6
        assert offset==u(descriptor+8)
        result=self.request(dict(op='read',descriptor=descriptor,destination=destination,bytes=size,encoded_offset=offset))
        authored=bytes.fromhex(result['descriptor_hex']);payload=bytes.fromhex(result['payload_hex'])
        assert len(authored)==20 and authored[:12]==bytes(cpu.mem_read(descriptor,12))
        assert size==((result['authored_bytes']+511)&~511) and len(payload)==size
        store(destination,payload)
        job=self.request(dict(op='commit'))['job']
        store(tracking+4,b'\x01')
        self.events.append(dict(kind='actual native synchronous animation resource read',
            caller_instruction=caller_instruction,target=0x1a3b70,returned=returned,
            descriptor=descriptor,destination=destination,bytes=size,encoded_offset=offset,
            payload_sha256=hashlib.sha256(payload).hexdigest(),allocated_job=job,
            actual_cache_catalog=self.identity,source_return_supplied=True,
            ABI='EAX actual retained job; consume28 stack bytes; preserve other GP/FP/flags',
            original_async_queue_worker_verified=False,Xbox_kernel_instructions_or_clobbers_verified=False))
        cpu.reg_write(R.UC_X86_REG_EAX,job);cpu.reg_write(R.UC_X86_REG_ESP,sp+28)
        cpu.reg_write(R.UC_X86_REG_EIP,returned)
