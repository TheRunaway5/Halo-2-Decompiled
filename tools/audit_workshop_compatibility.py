"""Read-only Workshop feature inventory and authenticated script candidates.

MCC/TU executable reads are offline evidence only. Matching declarations are
candidates, never proof that the evaluator or its engine effects are implemented.
Run through tools/run_test.py; only the explicitly requested small report remains.
"""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import struct
import sys
import xml.etree.ElementTree as ET

from audit_rendering import FormatError
from mcc_content import MccCache, MAX_CACHE
import steam_workshop as workshop

ROOT = Path(__file__).resolve().parents[1]
MCC_PIN = 'de65b4f4fdbf3f0a5eab7431fe530da17dd815599182dfd6ae9b7e21cf171946'
TU_PIN = '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
CATALOG = ROOT / 'tools/reference/assembly/src/Blamite/Formats/Halo2Xbox/H2X_Scripting.xml'


class Pe64:
    def __init__(self, path):
        self.data = path.read_bytes()
        workshop.require(hashlib.sha256(self.data).hexdigest() == MCC_PIN, 'MCC audit image changed')
        header = struct.unpack_from('<I', self.data, 0x3c)[0]
        workshop.require(self.data[header:header+4] == b'PE\0\0', 'invalid PE signature')
        optional = header + 24
        workshop.require(struct.unpack_from('<H', self.data, optional)[0] == 0x20b, 'expected PE32+')
        self.base = struct.unpack_from('<Q', self.data, optional+24)[0]
        self.sections = []
        for index in range(struct.unpack_from('<H', self.data, header+6)[0]):
            at = optional + struct.unpack_from('<H', self.data, header+20)[0] + index*40
            _, va, size, raw = struct.unpack_from('<4I', self.data, at+8)
            self.sections.append((self.base+va, size, raw))

    def read(self, address, size):
        for first, extent, offset in self.sections:
            if first <= address and address+size <= first+extent:
                at = offset+address-first
                workshop.require(at+size <= len(self.data), 'PE access escapes encoded file')
                return self.data[at:at+size]
        raise FormatError('PE address has no complete encoded owner: ' + hex(address))

    def word(self, address, size=8):
        return int.from_bytes(self.read(address, size), 'little')

    def string(self, address):
        result = bytearray()
        for index in range(4096):
            value = self.read(address+index, 1)
            if value == b'\0':
                return result.decode('ascii')
            result.extend(value)
        raise FormatError('PE string exceeds audit limit')

    def builtin(self, opcode):
        workshop.require(0 <= opcode < 2048, 'MCC builtin index exceeds registry audit bound')
        at = self.word(0x180e04850 + opcode*8)
        arity = self.word(at+56, 2)
        workshop.require(arity <= 8, 'MCC declaration exceeds argument bound')
        return dict(mcc_opcode=opcode, name=self.string(self.word(at+8)),
                    return_type=self.word(at, 2), arguments=list(struct.unpack('<'+'H'*arity, self.read(at+58,arity*2))),
                    mcc_flags=self.word(at+16,2), mcc_descriptor=hex(at),
                    mcc_evaluator=hex(self.word(at+32)))


def xbox_catalog(image):
    tree = ET.parse(CATALOG)
    types = {row.get('name'): int(row.get('opcode'),16) for row in tree.findall('.//valueTypes/type')}
    result = {}
    for row in tree.findall('.//function'):
        opcode = int(row.get('opcode'),16)
        descriptor = int.from_bytes(image.read(0x480c00+opcode*4,4),'little')
        returned, flags, evaluator, _, arity = struct.unpack('<HHIIH',image.read(descriptor,14))
        arguments = list(struct.unpack('<'+'H'*arity,image.read(descriptor+14,arity*2)))
        workshop.require(returned == types[row.get('returnType')] and
                         arguments == [types[arg.get('type')] for arg in row.findall('arg')],
                         'Xbox catalogue disagrees with authenticated declaration: ' + row.get('name'))
        result.setdefault(row.get('name'),[]).append(dict(xbox_opcode=opcode, xbox_descriptor=hex(descriptor),
            xbox_return_type=returned, xbox_arguments=arguments, xbox_flags=flags, xbox_evaluator=hex(evaluator)))
    return result


def inspect_map(path, profile):
    if path.stat().st_size > MAX_CACHE:
        return dict(map=path.name, bytes=path.stat().st_size, error='exceeds current 1 GiB cache reader limit')
    cache = MccCache(path)
    try:
        scenario = cache.tags[cache.u32(cache.index+12)]
        base = scenario['offset']
        nodes = list(cache.block(base+0x238,20))
        strings = cache.pointer(cache.u32(base+0x1b4),cache.magic,cache.u32(base+0x1b0))
        calls, missing, globals_ = Counter(), Counter(), Counter()
        known = {row['mcc_opcode']:row for row in profile['builtins']}
        known_globals = {row['mcc_index']:row for row in profile['external_globals']}
        for at in nodes:
            salt, opcode, _, flags, _, text, value = struct.unpack_from('<4H3I',cache.data,at)
            if not salt or flags == 0xbaba:
                continue
            if flags&4:
                if value&0x8000:
                    name = cache.string(strings+text,4096)
                    index = value&0x7fff
                    if index not in known_globals or known_globals[index]['name'] != name:
                        globals_[(index,name)] += 1
                continue
            if flags&3:
                continue
            function = nodes[value&65535]
            name = cache.string(strings+cache.u32(function+12),4096)
            calls[(opcode,name)] += 1
            row = known.get(opcode)
            native_clock = opcode in (0x38f,0x390) and name == {
                0x38f:'game_tick_rate',0x390:'game_tick_rate_scalar'}.get(opcode)
            if row is None or row['name'] != name or (row['xbox_opcode'] is None and not native_clock):
                missing[(opcode,name)] += 1
        selectors = Counter()
        for tag in cache.tags.values():
            if tag['group'] == 'bitm' and tag['offset'] is not None:
                for frame in cache.block(tag['offset']+68,168):
                    for part in range(3):
                        if cache.u32(frame+80+part*4):
                            selectors[cache.u32(frame+56+part*4)>>30] += 1
        return dict(map=path.name, bytes=path.stat().st_size, decoded_bytes=len(cache.data),
                    scenario=scenario['name'], scenario_type=cache.u16(base+16),
                    groups=dict(Counter(tag['group'] for tag in cache.tags.values())),
                    script_nodes=len(nodes), scripts=cache.u32(base+0x1b8),
                    builtin_calls=sum(calls.values()), missing_builtins=[dict(opcode=op,name=name,calls=count)
                        for (op,name),count in sorted(missing.items())],
                    missing_globals=[dict(index=index,name=name,references=count)
                        for (index,name),count in sorted(globals_.items())],
                    bitmap_selectors=dict(selectors), inherited_tags=sum(not tag['address'] for tag in cache.tags.values()),
                    material_templates=sorted(tag['name'] for tag in cache.tags.values() if tag['group']=='stem'))
    finally:
        cache.close()


def audit(args):
    profile = json.loads((ROOT/'tools/mcc-script-layout.json').read_text())
    pe = Pe64(args.dll)
    sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
    from xbe import Xbe
    image = Xbe(args.xbe)
    workshop.require(hashlib.sha256(image.data).hexdigest() == TU_PIN, 'TU audit image changed')
    workshop.require(hashlib.sha256(CATALOG.read_bytes()).hexdigest() == profile['xbox_catalogue_sha256'],
                     'Xbox naming authority changed')
    xbox = xbox_catalog(image)
    # Authenticate this parser against every already-admitted MCC declaration.
    for row in profile['builtins']:
        actual = pe.builtin(row['mcc_opcode'])
        workshop.require(all(actual[key] == row[key] for key in ('name','return_type','arguments','mcc_descriptor','mcc_evaluator')),
                         'MCC declaration parser disagrees with admitted profile: '+row['name'])
    report = dict(schema='H2_WORKSHOP_COMPATIBILITY_AUDIT_V1', mcc_sha256=MCC_PIN, xbox_tu15_sha256=TU_PIN,
                  catalogue_sha256=profile['xbox_catalogue_sha256'], existing_declarations_verified=len(profile['builtins']),
                  mods=[], declaration_candidates=[], engine_effects_qualified=False)
    # The global table is an inline array of 24-byte records, not pointers.
    # Prove null backing independently of the Xbox catalogue's null annotation.
    report['null_global_evidence'] = []
    for row in profile['external_globals']:
        if row.get('mcc_backing') != '0x0':
            continue
        address=0x180e086f0+row['mcc_index']*24
        workshop.require(pe.string(pe.word(address))==row['name'] and
                         pe.word(address+8,2)==row['mcc_type'] and pe.word(address+16)==0,
                         'MCC null global declaration differs: '+row['name'])
        xbox_address=int.from_bytes(image.read(0x47fb88+row['xbox_index']*4,4),'little')
        workshop.require(int.from_bytes(image.read(xbox_address,2),'little')==row['xbox_type'] and
                         int.from_bytes(image.read(xbox_address+4,4),'little')==0,
                         'Xbox null global declaration differs: '+row['name'])
        report['null_global_evidence'].append(dict(row,mcc_address=hex(address),xbox_address=hex(xbox_address)))
    needs = set()
    for directory in workshop.roots():
        for item in sorted(directory.iterdir()):
            if not item.name.isdecimal() or item.is_symlink() or not item.is_dir():
                continue
            try:
                info = workshop.metadata(item,'ModInfo.json')
                if info.get('Engine') != 'Halo2':
                    continue
                row = dict(id=item.name,title=workshop.localized(info.get('Title')),flags=info.get('GameModContents',{}),
                           inheritance=info.get('InheritSharedFiles'),maps=[],file_extensions=dict(Counter(
                               path.suffix.lower() for path in item.rglob('*') if path.is_file())))
                for path in sorted(workshop.source_owned(item,'maps').glob('*.map')):
                    if args.maps and path.stem not in args.maps:
                        continue
                    try:
                        result = inspect_map(path,profile)
                    except (OSError,ValueError,KeyError,FormatError) as error:
                        result = dict(map=path.name,error=str(error))
                    row['maps'].append(result)
                    needs.update((entry['opcode'],entry['name']) for entry in result.get('missing_builtins',[]))
                report['mods'].append(row)
            except (OSError,ValueError,TypeError,FormatError) as error:
                report['mods'].append(dict(id=item.name,error=str(error)))
    for opcode,name in sorted(needs):
        declaration = pe.builtin(opcode)
        workshop.require(declaration['name'] == name, 'map builtin identity differs from installed MCC: '+name)
        matches = [row for row in xbox.get(name,[]) if row['xbox_return_type']==declaration['return_type'] and
                   row['xbox_arguments']==declaration['arguments']]
        report['declaration_candidates'].append(dict(**declaration,matches=matches,
            status='matching-declaration-needs-engine-evaluator-proof' if len(matches)==1 else 'needs-native-adapter'))
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--dll',type=Path,default=Path.home()/'.local/share/Steam/steamapps/common/Halo The Master Chief Collection/halo2/halo2.dll')
    parser.add_argument('--xbe',type=Path,default=Path.home()/'.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe')
    parser.add_argument('--maps',nargs='*')
    parser.add_argument('--report',type=Path,required=True)
    args = parser.parse_args()
    result = audit(args)
    workshop.save(args.report,result)
    print(json.dumps(dict(mods=len(result['mods']),candidates=len(result['declaration_candidates']),
        existing_declarations_verified=result['existing_declarations_verified'],
        matched=sum(len(row['matches'])==1 for row in result['declaration_candidates']))))


if __name__ == '__main__':
    main()
