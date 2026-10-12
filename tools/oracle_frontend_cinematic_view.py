#!/usr/bin/env python3
"""Retain original cinematic-start/stop writes consumed by shader-detail setup.

The actual HS table resolves both evaluators. Execution follows each evaluator
into its original control routine, stopping after the interface flag is written
and before unrelated audio/bloom reset services. Allocated globals supply memory;
no guest instruction, state-producing routine or flag write is replaced.
"""
import hashlib, json, struct
from oracle_multiplayer_hud_positions import Retail, ROOT, PIN

r = Retail()
for address, size in [(0x4de2f4,0x90),(0x4f55d0,0x30),
                      (0x4e8c20,0x20),(0x510c50,0x30),(0x4e6948,0x1200)]:
    r.w(address,r.alloc(bytes(size)))
r.w(0x5093e0,0)
interface = r.r(0x510c50)
rows=[]
for opcode, stop, expected in [(0x22a,0x13c021,1),(0x22b,0x13cac5,0)]:
    definition=r.r(0x4744e0+opcode*4)
    evaluator=r.r(definition+4)
    for before in (0,1,7,255):
        r.g.mem_write(interface+5,bytes([before]))
        r.run(evaluator,argv=[opcode,0,0],stop=stop)
        after=bytes(r.g.mem_read(interface+5,1))[0]
        assert after==expected
        rows.append(dict(opcode=opcode,definition=f'{definition:08x}',
                         evaluator=f'{evaluator:08x}',stop=f'{stop:08x}',
                         before=before,after=after))
out=ROOT/'analysis/frontend/customization/cinematic-view-original.json'
out.write_text(json.dumps(dict(method=__doc__,xbe_sha256=PIN,
                              cases=rows,passed=True),indent=2)+'\n')
print(len(rows),'original lifecycle cases',hashlib.sha256(out.read_bytes()).hexdigest())
