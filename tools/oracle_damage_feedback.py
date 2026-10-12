#!/usr/bin/env python3
"""Execute pinned retail recent-damage report, fade and shader-input owners.

Report execution ends at D9814, before impulse/reaction services. D5DE0 is
entered at its recent-damage tail D6592, with explicit actor-bounds/null and
prior-active inputs. D8B50 and the six BCC20 vitality inputs run to return.
This is scalar/state evidence, not damage admission, a draw or pixel proof.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct

from oracle_multiplayer_hud_positions import Retail, ROOT, STACK
from unicorn.x86_const import (UC_X86_REG_EDI, UC_X86_REG_EBX, UC_X86_REG_ESI,
                              UC_X86_REG_EDX, UC_X86_REG_EBP, UC_X86_REG_ESP)


def bits(value):
    return struct.unpack('<I', struct.pack('<f', value))[0]


def word(value):
    return f'{value & 0xffffffff:08x}'


class FeedbackOracle(Retail):
    def hook(self, uc, address, size, data):
        pass  # All selected owners execute unchanged, without supplied calls.

    def __init__(self):
        super().__init__()
        self.unit = self.alloc(bytes(0x400))
        self.report = self.alloc(bytes(0x80))
        self.settings = self.alloc(bytes(0x70))
        self.rate = self.alloc(bytes(8))
        self.output = self.alloc(bytes(8))
        pool = self.alloc(bytes(0x48))
        headers = self.alloc(bytes(12))
        dummy_tag = self.alloc(bytes(0x80))
        self.w(pool+0x44, headers)
        self.w(headers+8, self.unit)
        self.w(0x4e0300, pool)
        self.w(0x510c54, self.rate)
        self.w(self.table+8, dummy_tag)
        self.w(self.table+16+8, dummy_tag)
        self.w(self.report+8, 1)
        self.w(self.report+0x10, 0xffffffff)  # no lethal equipment owner

    def state(self, values):
        for offset, value in zip((0xf4, 0xfc, 0x108, 0xf8, 0x100, 0x109), values):
            if offset in (0x108, 0x109):
                self.g.mem_write(self.unit+offset, bytes([value & 255]))
            else:
                self.w(self.unit+offset, value)

    def read_state(self):
        result = []
        for offset in (0xf4, 0xfc, 0x108, 0xf8, 0x100, 0x109):
            result.append(self.read(self.unit+offset, '<b')[0] if offset in (0x108, 0x109)
                          else self.r(self.unit+offset))
        return result

    def bounds(self, rate, custom, values):
        self.g.mem_write(self.rate+2, struct.pack('<h', rate))
        for i, value in enumerate(values):
            self.w(self.settings+0x30+4*i, value)
        return self.settings if custom else 0


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path,
                        default=ROOT/'analysis/damage-shields-goal-20261011')
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    original = FeedbackOracle()
    rng = random.Random(0xd8b50)
    rows, counts = [], dict(report=0, fade=0, update=0, shader_input=0)

    def emit(kind, values):
        rows.append(kind+' '+' '.join(map(word, values)))
        counts[dict(R='report', F='fade', U='update', I='shader_input')[kind]] += 1

    def state():
        return [bits(rng.uniform(-.25, 1.5)), bits(rng.uniform(-.25, 1.5)),
                rng.choice([-128, -2, -1, 0, 1, 59, 60, 126, 127]),
                bits(rng.uniform(-.25, 1.5)), bits(rng.uniform(-.25, 1.5)),
                rng.choice([-1, 0, 1, 126, 127])]

    amounts = [bits(x) for x in (0., -.1, .0001, .00010001, .1, .9999, 1., 1.5)]
    amounts += [0x80000000, 0x7fc12345, 0xffc12345, 0x7f800000, 0xff800000]
    for n in range(1600):
        before = state()
        body, shield = (rng.choice(amounts) for _ in range(2))
        flags, special = rng.randrange(4), rng.randrange(2)
        original.state(before)
        original.w(original.report+4, flags)
        original.w(original.report+0x44, body)
        original.w(original.report+0x48, shield)
        original.g.mem_write(original.unit+0x10a, struct.pack('<H', special << 11))
        original.run(0xd9640, stop=0xd9814, argv=[0],
                     registers=[(UC_X86_REG_EDI, original.report)])
        emit('R', [*before, body, shield, flags, special, *original.read_state()])

    delays = [bits(x) for x in (-1., 0., 1/60, .5, 1., 2., 4., 4.25)]
    seconds = [bits(x) for x in (-1., 0., .0001, .00010001, .25, 1., 2.)]
    for n in range(2400):
        before = state()
        rate, custom = rng.choice([30, 60, 120]), rng.randrange(2)
        definition = [rng.choice(delays), rng.choice(seconds),
                      rng.choice(delays), rng.choice(seconds)]
        if n < 40:
            before[0] = rng.choice(amounts)
            before[1] = rng.choice(amounts)
            definition[n % 4] = rng.choice([0x7fc12345, 0x7f800000, 0xff800000])
        pointer = original.bounds(rate, custom, definition)
        original.state(before)
        reset = rng.randrange(2)
        active = original.run(0xd8b50, argv=[reset], registers=[
            (UC_X86_REG_EDI, original.unit+0xf4),
            (UC_X86_REG_EBX, original.unit+0xfc),
            (UC_X86_REG_ESI, original.unit+0x108), (UC_X86_REG_EDX, pointer)]) & 255
        emit('F', [*before[:3], rate, custom, *definition, reset,
                   *original.read_state()[:3], active])

    for n in range(1600):
        before = state()
        rate, custom = rng.choice([30, 60, 120]), rng.randrange(2)
        definition = [rng.choice(delays), rng.choice(seconds),
                      rng.choice(delays), rng.choice(seconds)]
        pointer = original.bounds(rate, custom, definition)
        dead, depleted, special = (rng.randrange(2) for _ in range(3))
        original.state(before)
        original.g.mem_write(original.unit+0x10a,
                             struct.pack('<H', (dead<<2)|(depleted<<3)|(special<<11)))
        original.g.mem_write(STACK-0x80, bytes(0x80))
        original.w(STACK-0x80+0x14, pointer)
        active = original.run(0xd6592, registers=[(UC_X86_REG_EBP, original.unit),
                                                (UC_X86_REG_ESP, STACK-0x80)]) & 255
        emit('U', [*before, rate, custom, *definition, dead, depleted, special,
                   *original.read_state(), active])

    names = [0x0500055b, 0x0d000558, 0x0f000559, 0x13000556,
             0x15000557, 0x1800055a, 0xdeadbeef]
    for n in range(1400):
        before = state()
        if n < 100:
            before[0], before[3] = rng.choice(amounts), rng.choice(amounts)
        body, shield, dead, name = rng.choice(amounts), rng.choice(amounts), rng.randrange(2), rng.choice(names)
        original.state(before)
        original.w(original.unit+0xec, body)
        original.w(original.unit+0xf0, shield)
        original.g.mem_write(original.unit+0x10a, struct.pack('<H', dead<<2))
        found = original.run(0xbcc20, argv=[0, name, original.output, original.output+4]) & 255
        emit('I', [*before, body, shield, dead, name, found, original.r(original.output),
                   original.read(original.output+4, '<B')[0]])

    original.g.mem_write(original.unit+0x108, b'\x12\x34')
    original.run(0xb7db8, stop=0xb7dc6, registers=[(UC_X86_REG_EDI, original.unit)])
    assert original.read(original.unit+0x108, '<2b') == [-1, -1]
    fixture = args.output/'damage-feedback.tsv'
    fixture.write_text('# Unchanged retail instructions; synthetic explicit inputs\n'+'\n'.join(rows)+'\n')
    dependencies = [ROOT/'assets/disc/default.xbe', Path(__file__).resolve(),
                    ROOT/'tools/oracle_multiplayer_hud_positions.py']
    (args.output/'damage-feedback-original.json').write_text(json.dumps(dict(
        method=__doc__, cases=counts, constructor_timers=[-1, -1],
        fixtures_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),
        dependencies={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest()
                      for p in dependencies}), indent=2)+'\n')
    # Actual stock SMG amount, shield/body kernels and report construction in
    # TU1.1, followed by its unchanged report-store prefix. Retail D5DE0 tail
    # supplies the explicitly selected ordinary fade trajectory for this test.
    from oracle_mp_spawn_smg_damage import DamageOracle, Cache, OBJECTS, UNIT, RESULT
    from oracle_mp_spawn import CACHE_PIN
    cache=Cache(ROOT/'assets/disc/maps/lockout.map')
    if hashlib.sha256(cache.data).hexdigest()!=CACHE_PIN:
        raise ValueError('Original Lockout cache changed')
    smg=DamageOracle(cache)
    live_rows=[]
    for case,(shield,body) in enumerate(((1.,1.),(0.,1.),(0.,.01))):
        smg.fixture(cache,shield=shield,body=body)
        for offset in (0xf4,0xf8,0xfc,0x100):smg.w(OBJECTS+offset,0)
        smg.g.mem_write(OBJECTS+0x108,b'\xff\xff')
        smg.kernels();smg.produce_result()
        smg.stop_at=0xdae14
        smg.call(0xdac40,(UNIT,),((UC_X86_REG_EDI,RESULT),),end=0xdae14)
        after=[]
        for offset in (0xf4,0xfc,0x108,0xf8,0x100,0x109):
            after.append(struct.unpack('<b',smg.g.mem_read(OBJECTS+offset,1))[0]
                         if offset in (0x108,0x109) else smg.u(OBJECTS+offset))
        flags=smg.h(OBJECTS+0x10a)
        original.state(after)
        original.g.mem_write(original.unit+0x10a,struct.pack('<H',flags))
        original.bounds(30,False,[0]*4)
        for tick in range(7):
            live_rows.append(' '.join(map(word,[case,tick,*original.read_state()])))
            original.g.mem_write(STACK-0x80,bytes(0x80))
            original.run(0xd6592,registers=[(UC_X86_REG_EBP,original.unit),
                                           (UC_X86_REG_ESP,STACK-0x80)])
    live=args.output/'smg-feedback-live.tsv'
    live.write_text('# Actual Lockout TU1.1 SMG reports; retail ordinary fade tail\n'+'\n'.join(live_rows)+'\n')
    live_dependencies=[ROOT/'assets/disc/maps/lockout.map',
        ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03',
        ROOT/'tools/oracle_mp_spawn_smg_damage.py',*dependencies]
    (args.output/'smg-feedback-live-original.json').write_text(json.dumps(dict(
        method='Actual TU1.1 SMG kernels/report/prefix plus retail ordinary update tail; no world contact or draw',
        cases=3,ticks_per_case=7,fixtures_sha256=hashlib.sha256(live.read_bytes()).hexdigest(),
        dependencies={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest()
                      for p in live_dependencies}),indent=2)+'\n')
    # Replay BAB40 ->108D90 ->BCC20 and BA8C0 ->13BB90 with actual authored
    # biped function records, complete handler tables and unchanged curves.
    page=cache.magic&~4095
    original.g.mem_map(page,((len(cache.data)+cache.magic-page+4095)//4096)*4096)
    original.g.mem_write(cache.magic,bytes(cache.data))
    for datum,tag in cache.tags.items():
        if tag['offset'] is not None:
            original.w(original.table+(datum&65535)*16+8,tag['offset']+cache.magic)
    authored=[]
    finite=[bits(x) for x in (-1.,0.,.00001,.0001,.00010001,.125,.5,1.,1.5,2.)]+[0x80000000]
    for tag in cache.tags.values():
        if tag['group']!='bipd' or not tag['name'].endswith(('masterchief','masterchief_mp','elite_mp')):
            continue
        datum=int(tag['datum'],16)
        original.w(original.unit,datum)
        original.w(original.unit+0x14,0xffffffff)
        original.w(original.unit+0xc0,0)  # ordinary, no interpolation override
        original.w(original.unit+0x10a,0)
        for value in finite:
            original.w(original.unit+0xf4,value)
            for name in (0x0a0012b4,0x12001a0b,0x15000557,0xdeadbeef):
                active=original.run(0xbab40,argv=[0,name,original.output])&255
                authored.append(' '.join(map(word,[datum,value,name,active,original.r(original.output)])))
    aliases=args.output/'shield-function-inputs.tsv'
    aliases.write_text('# Actual original biped functions; complete retail BAB40 chain\n'+'\n'.join(authored)+'\n')
    (args.output/'shield-function-inputs-original.json').write_text(json.dumps(dict(
        method='Unchanged retail BAB40/108D90/BCC20/BA8C0/13BB90; actual original Lockout functions; no interpolation override',
        cases=len(authored),fixtures_sha256=hashlib.sha256(aliases.read_bytes()).hexdigest(),
        dependencies={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest()
                      for p in dependencies+[ROOT/'assets/disc/maps/lockout.map']}),indent=2)+'\n')
    print(json.dumps(counts))


if __name__ == '__main__':
    main()
