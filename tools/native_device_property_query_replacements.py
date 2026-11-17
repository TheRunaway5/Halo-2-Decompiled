"""Exact TU1.5 MapWorld 10BE20 entry with separate source spill profiles."""
import importlib.util
from pathlib import Path
import re

_pair_spec=importlib.util.spec_from_file_location('native_device_map_world_context',
    Path(__file__).with_name('native_map_world_property_query_replacements.py'))
pair=importlib.util.module_from_spec(_pair_spec)
_pair_spec.loader.exec_module(pair)

ENTRY='10be20'
HEADER=pair.header(ENTRY)
BODY=re.compile(r'^'+re.escape(HEADER)+r'.*?^\}',re.M|re.S)
OWNER='native_device_property_query_owner_detail'
CORE='native_device_property_query_detail'
BASENAME='native_tu15_device_property_query_source_owner.hpp'
INCLUDE='#include "gameplay/objects/'+BASENAME+'"\n'
ANCHOR=pair.INCLUDE
HELPER=re.compile(r'^std::uint32_t native_single_spill\(long double x,std::uint16_t &status\) \{.*?^\}',re.M|re.S)
HELPER_PIN='e7000a01d437d173c93ab95f59cfe991ba439b4db8cb86f014fc966a2730aad1'
PARTS=(pair.PARTS[2],pair.PARTS[0],pair.PARTS[1],pair.PARTS[3],pair.PARTS[4])
PROFILES={
    (False, False): (
        '3b6fca521e2de89dc0dc754a9246f070b191acb27d956504ed1ed7bb1122106f',
        '086a7e9e0eaa7ac687afb8b8094118d1211250698a0911a0beac4447b116ec46',
        '6df9d932e8db914ca60dfe34f67353abaca30ba9147ce5ff1ae1c01b3e0cc3b9',
        'f754c70cbc7f2e502ea1e757ef67688c89a38f38b665e7e2ba66f711571dc84c',
        '7eea402b148be600ea6ed5d42d41558af6cbb06b45379e66d8196035a9fa4b53',
        '198b2f6d6c55da3880d6f52f5a1623a9d6e276602a70f71c376eb2b84cd13554',
    ),
    (True, False): (
        'eb5bd46cbfedf1ff11b76698359eb71bc2056a67104471ba9d7d797492464eda',
        '086a7e9e0eaa7ac687afb8b8094118d1211250698a0911a0beac4447b116ec46',
        '6df9d932e8db914ca60dfe34f67353abaca30ba9147ce5ff1ae1c01b3e0cc3b9',
        'abcda7bb63b840d4f2ce25ec163dd66c9964b2a928af4ac897bd7b33765aa599',
        '7eea402b148be600ea6ed5d42d41558af6cbb06b45379e66d8196035a9fa4b53',
        '198b2f6d6c55da3880d6f52f5a1623a9d6e276602a70f71c376eb2b84cd13554',
    ),
    (False, True): (
        '21750a90d6aeab3834664b4662200482198b182afb7e3910a4a9a472452fc420',
        '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575',
        '5dda1bc027ca22bb0dfb680fea8c6ac399b3de4002aea0733e29373ed7f7b08d',
        'e32f2927198be567ac60d36c8f00767d8941832313a643fd964f3297e651f5f4',
        '4f4ae0165fbbbb4d37d893eb82f487055376425b7aebf9385dc0ac152f184739',
        '766f168a1357ae9fb01edc1e8489cb71741e3971db052a6ae4d80a0799646c49',
    ),
    (True, True): (
        '5449e7be2724f6a759b0f3e9b8b3c03eabe1ce41a18a819bc131df355ed4556d',
        '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575',
        '5dda1bc027ca22bb0dfb680fea8c6ac399b3de4002aea0733e29373ed7f7b08d',
        'e32f2927198be567ac60d36c8f00767d8941832313a643fd964f3297e651f5f4',
        '4f4ae0165fbbbb4d37d893eb82f487055376425b7aebf9385dc0ac152f184739',
        '766f168a1357ae9fb01edc1e8489cb71741e3971db052a6ae4d80a0799646c49',
    ),
}


def prefix(literal,fresh):
    tag=lambda x:'true'if x else'false'
    spill='native_single_spill'if fresh else'nullptr'
    return '\n    if(halo2::'+OWNER+'::bridge<'+tag(literal)+','+tag(fresh)+'>(s,'+spill+'))return;'


PREFIXES={profile:prefix(*profile)for profile in PROFILES}


def split_body(body):
    found=[key for key,marker in PREFIXES.items()if marker in body]
    if not found:
        if OWNER in body or CORE in body:raise ValueError('Unknown 10BE20 native entry')
        return body,None
    if len(found)!=1:raise ValueError('Ambiguous 10BE20 native entry')
    marker=PREFIXES[found[0]]
    if body.count(marker)!=1 or not body.startswith(HEADER+marker):
        raise ValueError('10BE20 native entry moved')
    return HEADER+body[len(HEADER)+len(marker):],found[0]


def canonical_body(body):
    return split_body(body)[0]


def authenticate(source,allow_old_tag=False):
    if 'void source_run_map_world_support_tu15('not in source:
        if any(x in source for x in (OWNER,CORE,BASENAME)):
            raise ValueError('10BE20 native entry lacks exact MapWorld owner')
        return None
    matches=list(BODY.finditer(source))
    if len(matches)!=1:raise ValueError('Missing or ambiguous full 10BE20 MapWorld body')
    match=matches[0];body,installed=split_body(match[0])
    raw=re.findall(r'if\(s\.services\.instruction\)\{s\.services\.instruction\(0x[0-9a-f]+u\);\}',body)
    literal=re.findall(r's\.instruction_literal\(0x[0-9a-f]+u\);',body)
    if (len(raw),len(literal))not in ((171,0),(0,171)):
        raise ValueError('10BE20 needs 171 uniform instruction sites')
    prepared=bool(literal)
    context=(pair.digest(body),*(pair.digest(pair.one(source,part,'10BE20 context'))for part in PARTS))
    profiles=[key for key,auth in PROFILES.items()if key[0]==prepared and context==auth]
    if len(profiles)!=1:raise ValueError('10BE20 complete correlated source profile changed')
    profile=profiles[0]
    if installed is not None and (profile[1]!=installed[1]or(prepared!=installed[0]and not allow_old_tag)):
        raise ValueError('Wrong 10BE20 spill policy or callback tag')
    if profile[1]:
        helper=HELPER.findall(source)
        if len(helper)!=1 or pair.digest(helper[0])!=HELPER_PIN:
            raise ValueError('Complete 10BE20 fresh spill helper changed')
    namespaces=re.findall(r'^[ \t]*(?:inline[ \t]+)?namespace[^\n]*',source,re.M)
    if namespaces!=['namespace halo2 {','namespace {']:
        raise ValueError('10BE20 complete namespace ownership changed')
    pair.installed_entries(source)
    owned=int(installed is not None)
    if source.count(OWNER)!=owned or source.count(BASENAME)!=owned or source.count(INCLUDE)!=owned or CORE in source:
        raise ValueError('Foreign or duplicate 10BE20 native marker')
    if source.count(ANCHOR)!=1 or(owned and source.count(ANCHOR+INCLUDE)!=1):
        raise ValueError('10BE20 include moved across its exact MapWorld sibling')
    return match,body,profile,installed


def replace_device_property_query(source):
    found=authenticate(source)
    if found is None or found[3]is not None:return source
    match,body,profile,_=found
    new=source[:match.start()]+HEADER+PREFIXES[profile]+body[len(HEADER):]+source[match.end():]
    new=new.replace(ANCHOR,ANCHOR+INCLUDE,1);authenticate(new);return new


def remove_device_property_query(source):
    found=authenticate(source)
    if found is None or found[3]is None:return source
    match,body,_,_=found
    return (source[:match.start()]+body+source[match.end():]).replace(INCLUDE,'',1)


def installed_entries(source):
    found=authenticate(source)
    if found is None or found[3]is None:raise ValueError('Complete 10BE20 default missing')
    return [ENTRY]


def retag_specialized(source,output):
    before=authenticate(source)
    if before is None or before[3]is None:return output
    match,body,profile,installed=authenticate(output,allow_old_tag=True)
    if installed!=before[3]or profile!=(True,before[2][1]):
        raise ValueError('Specializer moved or changed exact 10BE20 entry')
    result=output[:match.start()]+HEADER+PREFIXES[profile]+body[len(HEADER):]+output[match.end():]
    installed_entries(result);return result
