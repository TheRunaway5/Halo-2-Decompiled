#!/usr/bin/env python3
"""Audit the original SMG/BR/arm class and stream contract without live IO.

Runs unchanged 1CAA0/1CB20 and 1C330 on literal authored part/stream metadata.
Only the downstream selection/state consumers of 1CAA0 are intercepted for
observation; no selected class arithmetic is replaced. This is authored
availability and explicit part selection, not current visibility/animation.
"""
from pathlib import Path
import collections
import hashlib
import json
import struct
import sys
from audit_rendering import Cache
from audit_dynamic_model_shader_assets import MAPS
from audit_world_vertex_declarations import OriginalDeclaration, raw_model_streams

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe


class OriginalClass:
    def __init__(self, xbe):
        from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
        from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EAX, UC_X86_REG_ECX
        self.u = Uc(UC_ARCH_X86, UC_MODE_32)
        self.u.mem_map(0x10000, 0x600000)
        for s in xbe.sections:
            self.u.mem_write(s.va, xbe.section_bytes(s))
        self.u.mem_map(0x1000000, 0x100000)
        self.u.mem_map(0x3000000, 4096)
        self.u.mem_write(0x4e3b44, struct.pack('<I', 0x1000200))
        self.u.mem_write(0x1000200+16+8, struct.pack('<I', 0x1000300))
        self.u.mem_write(0x1000300+4, struct.pack('<I', 7))
        self.selected = []
        def consumer(uc, address, size, user):
            if address not in (0x1c590, 0x1cb70, 0x1c8c0):
                return
            esp = uc.reg_read(UC_X86_REG_ESP)
            if address == 0x1cb70:
                self.selected.append(uc.reg_read(UC_X86_REG_ECX))
                uc.reg_write(UC_X86_REG_EAX, 0)
            if address == 0x1c8c0:
                uc.reg_write(UC_X86_REG_EAX, 1)
            uc.reg_write(UC_X86_REG_EIP, struct.unpack('<I', bytes(uc.mem_read(esp,4)))[0])
            uc.reg_write(UC_X86_REG_ESP, esp+4+{0x1c590:4,0x1cb70:0,0x1c8c0:16}[address])
        self.u.hook_add(UC_HOOK_CODE, consumer)

    def run(self, mode, opaque, transparent, part):
        from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EBX
        u = self.u
        u.mem_write(0x1000010, bytes([opaque,transparent,0,0])+struct.pack('<H',mode))
        u.mem_write(0x1000100, struct.pack('<H',part))
        u.mem_write(0x10fff00, struct.pack('<5I',0x3000000,0x1000300,0,0,0))
        u.reg_write(UC_X86_REG_ESP,0x10fff00)
        u.reg_write(UC_X86_REG_EAX,0x1000000)
        u.reg_write(UC_X86_REG_ECX,0x1000100)
        u.reg_write(UC_X86_REG_EBX,1)
        u.emu_start(0x1caa0,0x3000000,count=10000)
        return self.selected[-1]


def main():
    original = Xbe(ROOT/'assets/disc/default.xbe')
    selector, declaration = OriginalClass(original), OriginalDeclaration(original)
    inventory = []
    for name in MAPS:
        path = ROOT/('assets/map-pack/maps' if (ROOT/f'assets/map-pack/maps/{name}.map').is_file() else 'assets/disc/maps')/(name+'.map')
        cache = Cache(path)
        _, matg = cache.tag(next(t['datum'] for t in cache.tags.values() if t['group']=='matg'),'matg')
        reps = cache.block(matg+0x138,0xbc)
        roles = [('chief_hands',cache.u32(reps[2]+4)),('elite_hands',cache.u32(reps[3]+4))]
        for weapon in ('smg','battle_rifle'):
            tag = next(t for t in cache.tags.values() if t['group']=='weap' and t['name']=='objects\\weapons\\rifle\\'+weapon+'\\'+weapon)
            _, root = cache.tag(tag['datum'],'weap')
            fp = [cache.u32(at+4) for at in cache.block(root+0x2a8,16)]
            assert len(fp)==2 and fp[0]==fp[1]
            roles.append((weapon+'_fp',fp[0]))
        for role, mode in roles:
            tag, root = cache.tag(mode,'mode')
            sections = cache.block(root+0x24,0x5c)
            streams = raw_model_streams(cache,mode,original)
            shaders = [cache.u32(at+12) for at in cache.block(root+0x60,32)]
            owner = dict(map=name,role=role,model=f'{mode:08X}',name=tag['name'],sections=[])
            for record in streams:
                ordinal, section = record['section'], sections[record['section']]
                classification = cache.u16(section)
                opaque, transparent = cache.data[section+0x14:section+0x16]
                resource = cache.block(section+0x48,16)
                entry = next(r for r in resource if cache.u16(r+4)==0 and cache.u16(r+6)==72)
                pointer, size, header = cache.u32(section+0x38),cache.u32(section+0x3c),cache.u32(section+0x44)
                source = path if pointer>>30==0 else path.parent/('', 'mainmenu.map','shared.map','single_player_shared.map')[pointer>>30]
                if not source.exists():
                    source = ROOT/'assets/disc/maps'/source.name
                with source.open('rb') as f:
                    f.seek(pointer&0x3fffffff);raw=f.read(size)
                first, length = len(raw)-header-4+cache.u32(entry+12),cache.u32(entry+8)
                assert length%72==0 and 0<=first<=len(raw)-length
                parts=[]
                for part in range(length//72):
                    at=first+part*72
                    kind, material=struct.unpack_from('<Hxxh',raw,at)
                    actual=selector.run(classification,opaque,transparent,kind)
                    expected=classification if classification<3 else (transparent if kind in(4,5) else opaque)+2
                    assert actual==expected
                    parts.append(dict(part=part,type=kind,shader=f'{shaders[material]:08X}' if 0<=material<len(shaders) else 'FFFFFFFF',selected_class=actual,
                                      raw_words=[f'{w:08X}' for w in struct.unpack_from('<18I',raw,at)]))
                owner['sections'].append(dict(section=ordinal,classification=classification,opaque_influences=opaque,transparent_influences=transparent,
                    vertices=cache.u16(section+4),streams=record['streams'],parts=parts))
            inventory.append(owner)
        cache.close()
    # Independently execute the actual declaration producer on each selected
    # semantic list/stream layout. Semantic lists are frozen from the unchanged
    # authored class bank, independently of the native packed input adapter.
    models = {(x['map'],x['model']):x for x in inventory}
    caches, class_records, keys = {}, [], set()
    corpus = json.loads((ROOT/'analysis/multiplayer-shading/world-vertex-declarations.json').read_text())
    shader_to_models=collections.defaultdict(list)
    for owner in models.values():
        for section in owner['sections']:
            for part in section['parts']:
                shader_to_models[(owner['map'],int(part['shader'],16))].append((owner,section,part))
    for link in corpus['links']:
        for owner,section,part in shader_to_models.get((link['map'],link['shader']),[]):
            key=(owner['map'],link['vrtx'],part['selected_class'],tuple(s['type'] for s in section['streams']))
            if key in keys:continue
            keys.add(key)
            if owner['map'] not in caches:
                name=owner['map'];maps='assets/map-pack/maps' if (ROOT/f'assets/map-pack/maps/{name}.map').is_file() else 'assets/disc/maps'
                caches[name]=Cache(ROOT/maps/(name+'.map'))
            c=caches[owner['map']];tag,root=c.tag(link['vrtx'],'vrtx');classes=c.block(root+4,28)
            selected=part['selected_class'];assert selected<len(classes)
            at=classes[selected];semantics=[c.u16(s) for s in c.block(at+4,2)]
            code=bytes(c.data[c.u32(at+16)-c.magic:c.u32(at+16)-c.magic+c.u32(at+12)])
            if not code:continue
            original_words=declaration.run(semantics,list(key[3]))
            class_records.append(dict(map=owner['map'],model=owner['model'],section=section['section'],vrtx=f"{link['vrtx']:08X}",
                name=tag['name'],selected_class=selected,semantics=semantics,stream_types=list(key[3]),
                compiled_sha256=hashlib.sha256(code).hexdigest(),declaration_words=original_words))
    for c in caches.values():c.close()
    counts=collections.Counter(p['selected_class'] for o in inventory for s in o['sections'] for p in s['parts'])
    report=dict(executable_sha256=hashlib.sha256(original.data).hexdigest(),untouched_functions=['1CAA0','1CB20','1C330','35790','35850'],
        intercepted_state_consumers=['1C590','1CB70','1C8C0'],maps=len(MAPS),models=len(inventory),
        sections=sum(len(o['sections']) for o in inventory),parts=sum(counts.values()),part_class_counts=dict(counts),
        declaration_executions=len(class_records),declaration_words=len(class_records)*64,models_data=inventory,class_records=class_records,
        boundary='All authored SMG/BR/Chief+Elite hand sections, including LOD alternatives, are an explicit asset superset. Current section visibility, live animation, scope/HUD and first-person chronology are not inferred. Native class0 consumes CPU-posed/decompressed streams; original class2/4/5 input ordinals must not be applied to those streams.')
    out=ROOT/'analysis/multiplayer-shading/first-person-vertex-routes-original.json'
    out.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items() if k not in('models_data','class_records')},indent=2))


if __name__=='__main__':main()
