"""Declared whole136A80/1365C0 inputs for the native2CBD0 World bridge.

The original 22 whole-parent rows come from Claude Opus physical-audit input
factories (analysis/renderer-native-bank-append-20261102/opus-model-append-retirement-audit.md).
Expected outcomes are obtained anew from the pinned executable; none live here.
"""
import copy
import struct

STOP=0x63000000
STACK=0x600e8000
STACK_BASE=0x600e0000
STACK_END=0x60100000
SECTIONS_MAP={'rdata44a':(0x44a000,4096),'rdata456':(0x456000,4096),'rdata46a':(0x46a000,4096),
    'view':(0x4c6000,0x9000),'objects':(0x4ec000,4096),'tags':(0x4f0000,4096),
    'resources':(0x4f2000,4096),'policy':(0x475000,4096),'force':(0x515000,4096),
    'contexts':(0x559000,0x4000),'data':(0x1000000,0x10000),'stack':(STACK_BASE,STACK_END-STACK_BASE)}


def patch(row,address,value,size=4):
    for bank in row['banks']:
        data=bytearray.fromhex(bank['before']);offset=address-bank['address']
        if 0<=offset and offset+size<=len(data):
            data[offset:offset+size]=(value&((1<<(8*size))-1)).to_bytes(size,'little');bank['before']=data.hex();return
    raise ValueError('Unmapped declared model input '+hex(address))


def resource(row,*,publish=False,target=None):
    d=0x1000000;lightmaps=d+0xc000;group=d+0xc100;checksums=d+0xb000;references=d+0xd000
    resources=d+0xe000;header=resources+12;physical=d+0xe400;pool=d+0xe500;records=d+0xe600
    inflight=d+0xf000;inflight_rows=d+0xf100;allocation=d+0xf800
    for at,value,size in [(0x4ecc14,lightmaps,4),(lightmaps+0x80,1,4),(lightmaps+0x84,group,4),
        (group+0x1c,0,4),(group+4,0x12345678,4),(d+0x8000+8,0x12345678,4),
        (group+0x5c,checksums,4),(d+0x100+8,0xabcdef01,4),(checksums+12*0x123+8,0xabcdef01,4),
        (group+0x64,references,4),(references+12*0x123+2,0,2),(group+0x44,resources,4),
        (header+8,16,4),(header+0xc,0,4),(header+0x1c,0x24,2),(header+0x1e,not publish,1),(header+0x20,1,4),
        (0x4f2d6c,physical,4),(physical+0x64,pool,4),(pool+0x44,records,4),(physical+0x38,0xc10c136a,4),
        (0x4f2d5c,inflight,4),(inflight+0x44,inflight_rows,4),(inflight_rows+12+2,1,1),
        (physical+0x34,0,4),(records+24+8,0,4),(0x4f2d64,allocation,4)]:patch(row,at,int(value),size)
    if target=='record':
        patch(row,header+0x1c,d+0x6000+0x1c0-header,2)
    elif target=='count':
        # A real type1 publication targets the live counter. The second part
        # must use the published ordinal, not the first append's saved value.
        patch(row,0x4f2d64,0x4cd398-8);patch(row,header+0x10,1);patch(row,header+0x14,d+0xf400)
        patch(row,d+0xf400,1,1);patch(row,d+0xf404,0,2);patch(row,d+0xf408,9);patch(row,d+0xf40c,0)


def cases(image):
    rows=[r for r in model_cases(image,SECTIONS_MAP,STACK)if not r.get('stop_before_entries')]
    rows+=presentation_cases(image,SECTIONS_MAP,STACK)
    plain=rows[0];variant=next(r for r in rows if r['name'].startswith('0x80 variant:'))
    def clone(original,name):
        row=copy.deepcopy(original);row['name']=name;rows.append(row);return row
    for fp in ([0x37f,0x4500,0x1f80],[0x67f,0x4000,0x3f80]):
        row=clone(variant,'presentation carried FP '+str(fp));row['before_fp']=fp
    for count in (849,850):
        row=clone(plain,'two-part ordinal limit '+str(count));patch(row,0x4cd398,count)
        patch(row,0x1006000+0x18,2);patch(row,0x1006000+0x76,6,1)
    for count in (191,192):
        row=clone(variant,'variant override limit '+str(count));patch(row,0x4cdd48,count)
    row=clone(plain,'late geometry refusal');patch(row,0x1001000+92*5+0x34,0)
    row=clone(plain,'empty subpart mask');patch(row,0x1001000+92*5+4+0x24,0,2)
    row=clone(plain,'reserved mask arena');patch(row,0x5598f8+0xa5c,511,2)
    for publish,target in ((False,None),(True,None),(True,'record'),(True,'count')):
        row=clone(plain,'model resource '+('publish'if publish else'resident')+' '+str(target))
        if target:
            patch(row,0x1006000+0x18,2);patch(row,0x1006000+0x76,6,1)
        resource(row,publish=publish,target=target)
    for publish in (False,True):
        row=clone(plain,'model datum NONE, two resource requests, publish='+str(publish))
        resource(row,publish=publish);patch(row,0x1006000+0x5c,0xffffffff)
        clusters=0x1002000;instances=0x1003000;section=clusters+176*5
        for at,value,size in [(0x1008000+0xa0,clusters,4),(0x1008000+0x144,instances,4),
            (0x100c100+0x54,0x100d000,4),(section+0x14,2,2),(section+0x24,40,2),
            (section+0x50,0x1004000,4),(section+0x28+0x1e,1,1),(section+0x28+0x20,1,4)]:patch(row,at,value,size)
    for index,row in enumerate(rows):
        row['id']=index;row['max_steps']=100000
    return rows

def model_cases(image,section_map,stack):
    CTX = 0x5598f8; D = 0x1000000; TAGS = D; MODELDEF = D + 0x100; SECTIONS = D + 0x1000; GEOM = D + 0x4000
    REC = D + 0x6000; TOKENS = D + 0x7000; BSP = D + 0x8000; OBJS = D + 0xa000; HDRS = D + 0xa100; BODY = D + 0xa800
    VDEF = D + 0xb000; MODEL = 0xabcd0001; VEHICLE_DEF = 0xbeef0004; SP = stack
    def f2u(x): return struct.unpack('<I', struct.pack('<f', x))[0]
    rows = []
    def row(name, *, models=1, parts=(1,), sections=((5,),), flags170=0, flags1ac=0x42, section=31, override=0xffffffff,
            coverage=3.5, filter_byte=0, owners=(0xffffffff,), tokens=True, vehicle=False, stop=None, xmm=None,
            fade_selector=1, used=0):
        banks = [dict(address=at, before=bytearray(size)) for at, size in section_map.values()]
        def put(at, v, size=4):
            for b in banks:
                if b['address'] <= at and at + size <= b['address'] + len(b['before']):
                    b['before'][at - b['address']:at - b['address'] + size] = (v & ((1 << (8 * size)) - 1)).to_bytes(size, 'little'); return
            raise ValueError(hex(at))
        for b in banks:
            if b['address'] in (0x44a000, 0x456000, 0x46a000, 0x475000): b['before'][:] = image.read(b['address'], 4096)
        put(0x4cd3a0, CTX); put(0x4cd398, 7); put(0x4cd39c, 3); put(CTX + 0x2a78, 0); put(CTX + 0xa5c, used, 2)
        put(0x4f0414, TAGS); put(TAGS + 16 * (MODEL & 0xffff) + 8, MODELDEF); put(MODELDEF + 0x28, SECTIONS)
        for s in (5, 6):
            sec = SECTIONS + 92 * s; put(sec + 4 + 0x14, 2, 2); put(sec + 4 + 0x24, 40, 2); put(sec + 0x34, GEOM)
        put(0x4ecc14, 0); put(0x4ecc18, BSP); put(0x4ecbd0, OBJS); put(OBJS + 0x44, HDRS)
        # Rectangle world (opus-rectangle-30270-audit default): eye, identity basis, projection, viewport.
        for i, v in enumerate((1.0, 2.0, 3.0)): put(0x4c65c0 + 4 * i, f2u(v))
        put(0x4c65c0 + 0x30, 0, 2); put(0x4c65c0 + 0x32, 0, 2); put(0x4c65c0 + 0x34, 720, 2); put(0x4c65c0 + 0x36, 1280, 2)
        J = 0x4c6634; put(J, f2u(1.0))
        for i, v in enumerate((1, 0, 0, 0, 1, 0, 0, 0, 1)): put(J + 4 + 4 * i, f2u(float(v)))
        put(J + 0x78, f2u(0.9)); put(J + 0x8c, f2u(-1.6)); put(J + 0x98, f2u(0.05)); put(J + 0x9c, f2u(-0.02))
        # Stale target slots (residue inheritance is part of the compare set).
        for n in range(7, 11):
            u = 0x4c6958 + 32 * n
            for k in range(8): put(u + 4 * k, 0xa5a5a5a5)
            put(0x4cd3a4 + n, 0x5a, 1); put(0x4cd6f6 + n, 0x5a, 1)
        # Declared render record (0x1E4 bytes): only fields 136A80 reads.
        put(REC + 0x166, models, 2)
        for k in range(models):
            put(REC + 0x18 + 4 * k, parts[k]); put(REC + 0x04 + 4 * k, 0x11110000 + k); put(REC + 0x5c + 4 * k, MODEL)
            token = TOKENS + 0x100 * k if tokens else 0
            put(REC + 0x48 + 4 * k, token)
            if token: put(token, owners[k % len(owners)])
            put(REC + 0x2c + k, 0x10 + k, 1); put(REC + 0x70 + k, k, 1)
            for j in range(parts[k]):
                put(REC + 0x75 + 16 * k + j, sections[k][j], 1); put(REC + 0xc6 + 32 * k + 2 * j, 0x9 + j, 2)
        put(REC + 0x16a, 0x0123, 2); put(REC + 0x16c, 0, 1); put(REC + 0x170, flags170)
        for i, v in enumerate((2.0, 2.5, -7.0)): put(REC + 0x194 + 4 * i, f2u(v))
        put(REC + 0x1a0, f2u(1.0)); put(REC + 0x1ac, flags1ac); put(REC + 0x1b0, 0); put(REC + 0x1b4, f2u(coverage))
        put(REC + 0x1bc, 0xc8, 1); put(REC + 0x1be, 0x1234, 2); put(REC + 0x1c0, section); put(REC + 0x1c4, 1 if vehicle else 0x22)
        put(REC + 0x1d0, override); put(REC + 0x1d4, fade_selector, 1); put(REC + 0x1d8, f2u(0.5)); put(REC + 0x1dc, f2u(0.25))
        put(REC + 0x1e0, f2u(12.5))
        if vehicle:
            put(TOKENS, 3); put(HDRS + 12 * 3 + 8, BODY); put(BODY, VEHICLE_DEF); put(BODY + 0xaa, 2, 1)
            put(TAGS + 16 * (VEHICLE_DEF & 0xffff) + 8, VDEF); put(VDEF + 0x290, 1, 2)
        # Stack residue above 136A80's frame: the byte locals at [esp+2C]/[esp+30] keep these upper bytes.
        for at in range(SP - 0x400, SP, 4): put(at, 0xc3c3c3c3)
        case = dict(id=len(rows), name=name, entry=0x136a80, gp=[REC, 0x0ccc0ccc, 0x0ddd0ddd, 0x0bbb0bbb, SP, 0x0eee0eee,
            0x05550555, 0x07770777], matrix=filter_byte, source=0x0f0f0f0f, stop_pc=STOP, max_steps=40000,
            before_fp=[0x27f, 0, 0x1f80], live_x87=[], banks=[dict(address=b['address'], before=b['before'].hex()) for b in banks])
        if stop: case['stop_before_entries'] = [stop]
        if xmm: case['xmm'] = xmm
        rows.append(case)
    XMM_B = [[0x7fc00000 + 16 * n + k for k in range(4)] for n in range(8)]
    row('one part, stop at 136CE5', stop=0x136ce5)
    row('one part, whole 136A80')
    row('one part, whole 136A80, other XMM entry', xmm=XMM_B)
    row('section 5 override 0x77 -> 30270, stop at 136CE5', section=5, override=0x77, stop=0x136ce5)
    row('section 5 override 0x77 -> 30270, whole', section=5, override=0x77)
    row('section 5 override 0x77 -> 30270, whole, other XMM', section=5, override=0x77, xmm=XMM_B)
    row('section 5 override 0x77, arg12 forced full (flags170=0xA)', section=5, override=0x77, flags170=0xa)
    row('two parts: section 5 then 0xFF (EAX=-1 refusal)', parts=(2,), sections=((5, 0xff),))
    row('two models, filter on, owners differ (model 1 skipped)', models=2, parts=(1, 1), sections=((5,), (6,)),
        filter_byte=1, owners=(0x00010001, 0x00020002))
    row('two models, filter on, owners equal', models=2, parts=(1, 1), sections=((5,), (6,)), filter_byte=1,
        owners=(0x00010001, 0x00010001))
    row('fade selector 1 via token, override 0x77 section 31', override=0x77)
    row('vehicle record copy (section 5/override 0x77 overridden)', vehicle=True, section=5, override=0x77)
    row('two models, owners equal, sections 5/6 override 0x77 (30270 twice)', models=2, parts=(1, 1), sections=((5,), (6,)),
        filter_byte=1, owners=(0x00010001, 0x00010001), section=5, override=0x77)
    row('same, other XMM entry', models=2, parts=(1, 1), sections=((5,), (6,)), filter_byte=1,
        owners=(0x00010001, 0x00010001), section=5, override=0x77, xmm=XMM_B)
    return rows


def presentation_cases(image,section_map,stack):
    CTX = 0x5598f8; D = 0x1000000; TAGS = D; MODELDEF = D + 0x100; SECTIONS = D + 0x1000; GEOM = D + 0x4000
    REC = D + 0x6000; TOKEN = D + 0x7000; BSP = D + 0x8000; OBJS = D + 0xa000; HDRS = D + 0xa100; BODY = D + 0xa800
    COUNTER = D + 0xc000; MODEL = 0xabcd0001; SP = stack
    def f2u(x): return struct.unpack('<I', struct.pack('<f', x))[0]
    rows = []
    def row(name, *, flags170=0, children=0, skip=0, override=0xffffffff, variant_override=0x66, presented=0, xmm=None):
        banks = [dict(address=at, before=bytearray(size)) for at, size in section_map.values()]
        def put(at, v, size=4):
            for b in banks:
                if b['address'] <= at and at + size <= b['address'] + len(b['before']):
                    b['before'][at - b['address']:at - b['address'] + size] = (v & ((1 << (8 * size)) - 1)).to_bytes(size, 'little'); return
            raise ValueError(hex(at))
        for b in banks:
            if b['address'] in (0x44a000, 0x456000, 0x46a000, 0x475000): b['before'][:] = image.read(b['address'], 4096)
        put(0x4cd3a0, CTX); put(0x4cd398, 7); put(0x4cd39c, 3); put(CTX + 0x2a78, 0)
        put(0x4f0414, TAGS); put(TAGS + 16 * (MODEL & 0xffff) + 8, MODELDEF); put(MODELDEF + 0x28, SECTIONS)
        sec = SECTIONS + 92 * 5; put(sec + 4 + 0x14, 2, 2); put(sec + 4 + 0x24, 40, 2); put(sec + 0x34, GEOM)
        put(0x4ecc14, 0); put(0x4ecc18, BSP); put(0x4ecbd0, OBJS); put(OBJS + 0x44, HDRS)
        put(HDRS + 12 * 3 + 8, BODY); put(BODY + 0x10, 0xffffffff)          # no attached children
        for i, v in enumerate((1.0, 2.0, 3.0)): put(0x4c65c0 + 4 * i, f2u(v))
        for i, v in enumerate((0.0, 0.0, 1.0)): put(0x4c65cc + 4 * i, f2u(v))   # forward
        put(0x4c65c0 + 0x34, 720, 2); put(0x4c65c0 + 0x36, 1280, 2)
        J = 0x4c6634; put(J, f2u(1.0))
        for i, v in enumerate((1, 0, 0, 0, 1, 0, 0, 0, 1)): put(J + 4 + 4 * i, f2u(float(v)))
        put(J + 0x78, f2u(0.9)); put(J + 0x8c, f2u(-1.6)); put(J + 0x98, f2u(0.05)); put(J + 0x9c, f2u(-0.02))
        for n in range(7, 12):
            for k in range(8): put(0x4c6958 + 32 * n + 4 * k, 0xa5a5a5a5)
        put(REC, 0x00030003)                                               # object handle (index 3)
        put(REC + 0x166, 1, 2); put(REC + 0x18, 1); put(REC + 0x04, 0x11110000); put(REC + 0x5c, MODEL)
        put(REC + 0x48, TOKEN); put(TOKEN, 0x00030003); put(REC + 0x2c, 0x10, 1); put(REC + 0x75, 5, 1); put(REC + 0xc6, 9, 2)
        put(REC + 0x16a, 0x0123, 2); put(REC + 0x170, flags170)
        for i, v in enumerate((2.0, 2.5, -7.0)): put(REC + 0x194 + 4 * i, f2u(v))
        put(REC + 0x180, variant_override); put(REC + 0x190, f2u(0.75))
        put(REC + 0x1a0, f2u(1.0)); put(REC + 0x1ac, 0x42); put(REC + 0x1b0, 0x10); put(REC + 0x1b4, f2u(3.5))
        put(REC + 0x1bc, 0xc8, 1); put(REC + 0x1be, 0x1234, 2); put(REC + 0x1c0, 31); put(REC + 0x1c4, 0x22)
        put(REC + 0x1c8, COUNTER); put(COUNTER, 4); put(REC + 0x1cc, presented, 1); put(REC + 0x1d0, override)
        put(REC + 0x1d4, 1, 1); put(REC + 0x1d8, f2u(0.5)); put(REC + 0x1dc, f2u(0.25)); put(REC + 0x1e0, f2u(12.5))
        for at in range(SP - 0x2000, SP, 4): put(at, 0xc3c3c3c3)
        put(SP + 12, skip); put(SP + 16, 0x55)
        rows.append(dict(id=len(rows), name=name, entry=0x1365c0, gp=[0x0aaa0aaa, CTX, 0x0ddd0ddd, 0x0bbb0bbb, SP, 0x0eee0eee,
            0x05550555, 0x07770777], matrix=REC, source=children, stop_pc=STOP, max_steps=60000,
            before_fp=[0x27f, 0, 0x1f80], live_x87=[], banks=[dict(address=b['address'], before=b['before'].hex()) for b in banks]))
        if xmm: rows[-1]['xmm'] = xmm
    XMM_B = [[0x7fc00000 + 16 * n + k for k in range(4)] for n in range(8)]
    row('plain record, no variants')
    row('0x80 variant: section counter + recursive copy', flags170=0x80)
    row('0x80 variant, already presented (+1CC=1)', flags170=0x80, presented=1)
    row('2 variant (outer record not drawn directly)', flags170=0x02)
    row('0x82 both variants', flags170=0x82)
    row('1 and 0x20 variants (flags cleared, 0x8000 marker)', flags170=0x21)
    row('skip byte set: direct 136A80', flags170=0x82, skip=1)
    row('0x80 variant, variant override -1', flags170=0x80, variant_override=0xffffffff)
    row('0x82 both variants, other XMM entry', flags170=0x82, xmm=XMM_B)
    row('1 and 0x20 variants, other XMM entry', flags170=0x21, xmm=XMM_B)
    return rows
