#!/usr/bin/env python3
"""Publish compiled Dire Valley with an owned native Halo 2 material bank.

Run inside tools/run_test.py. This imports Classic lightmap resources and the
existing main-game HUD; shader programs come from the port's preserved Halo 2
library. A compiler cache is never aliased to a stock whole-map owner.
"""
import argparse
import copy
import json
import os
from pathlib import Path
import shutil

from port_dire_valley import save, sha
from prepare_dust2_native import prepare_compiled_lighting
from mcc_native_map_owners import bank_identity
from mcc_native_map_owner_publication import publish_staged_owner_index
from validate_dire_valley_geometry import validate as validate_geometry


def publish(source,imported,output,main_hud,report):
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    output=output.resolve()
    assert output.is_relative_to(scratch) and not output.exists()
    maps=output/'halo2/h2_maps_win64_dx11';maps.mkdir(parents=True)
    shutil.copytree(imported/'halo2/h2_fonts',output/'halo2/h2_fonts')
    shutil.copyfile(imported/'halo2/h2_maps_win64_dx11/shared.map',maps/'shared.map')
    relative='halo2/Mods/dire_valley/dire_valley.map';target=output/relative
    # Collision leaves and accelerator mappings address the compiler's exact
    # subparts. Merging those rows invalidates CPU lighting attribution and
    # lets original45A70 write masks beyond the query arena.
    lighting=prepare_compiled_lighting(source,target,main_hud,imported,compact=False)
    geometry=validate_geometry(source,target)
    for name in ['textures.dat','sounds_en.dat','sounds_neutral.dat','sounds_remastered.dat']:
        os.link(target,maps/name)
    bankdir='native-shaders/maps/dire_valley';bank=output/bankdir
    (bank/'payloads').mkdir(parents=True)
    records={};donors=[];first=None
    source_banks=imported/'native-shaders'
    names=['coagulation','lockout','01b_spacestation','00a_introduction']
    names+=sorted(p.name for p in source_banks.iterdir() if (p/'library.json').is_file() and p.name not in names)
    for name in names:
        directory=source_banks/name;lib=json.loads((directory/'library.json').read_text())
        if first is None:first=copy.deepcopy(lib)
        donors.append(dict(map=name,library_sha256=sha(directory/'library.json')))
        for row in lib['shader_tags']:
            key=row['group'],row['name']
            if key in records:continue
            records[key]=copy.deepcopy(row)
            dest=bank/'payloads'/row['payload']
            if dest.exists():assert sha(dest)==sha(directory/'payloads'/row['payload'])
            else:shutil.copyfile(directory/'payloads'/row['payload'],dest)
    rasterizer=first['rasterizer']
    shutil.copyfile(source_banks/names[0]/'payloads'/rasterizer['payload'],bank/'payloads'/rasterizer['payload'])
    first.update(map='dire_valley',shader_tags=list(records.values()),custom_authoring=dict(
        donors=donors,source='Danger Canyon CE geometry with Halo 2 shader bindings',
        teleporters='Halo 2 2way_teleporter base and receiver; original CE channel/destinations'))
    save(bank/'library.json',first)
    for dest in [bank/'function-tables.bin',output/'native-shaders/function-tables.bin']:
        shutil.copyfile(source_banks/'function-tables.bin',dest)
    save(output/'native-shaders/catalog.json',dict(schema='H2_PRESERVED_NATIVE_SHADER_CATALOG_V1',maps=[]))
    inventory={str(p.relative_to(output/'halo2')):dict(bytes=p.stat().st_size,sha256=sha(p)) for p in [maps/'shared.map',target]}
    provenance=dict(schema='H2_MCC_TU15_PREPARATION_V1',external_installation_requirement='Halo MCC only',
        source_files=inventory,native_shader_library=dict(maps={}),custom_authoring=lighting)
    stock=inventory['h2_maps_win64_dx11/shared.map']
    save(output/'tag-conversion.json',dict(profile=provenance['schema'],maps=[dict(map='shared.map',role='resources',
        source_sha256=stock['sha256'],output_sha256=stock['sha256'])]))
    save(output/'provenance.json',provenance)
    bank_id=bank_identity(bankdir,(bank/'library.json').read_bytes())
    receipt=dict(map=relative,input_bytes=target.stat().st_size,input_sha256=sha(target),source_sha256=sha(target),
        cache_role='scenario',shared_directory='halo2/h2_maps_win64_dx11',native_mode='owned-bank',bank_owner_map=relative,
        producer='prepare_dire_valley_native.py',profile='H2_DIRE_VALLEY_NATIVE_AUTHORING_V1',
        **{k:bank_id[k] for k in ['bank_directory','bank_namespace','bank_library_sha256']})
    raw=(json.dumps(receipt,sort_keys=True)+'\n').encode();(output/'custom-owner-receipt.json').write_bytes(raw)
    provenance=publish_staged_owner_index(output,provenance,custom_receipts=[raw]);save(output/'provenance.json',provenance)
    save(report,dict(schema='H2_DIRE_VALLEY_NATIVE_PUBLICATION_V1',source_sha256=sha(source),output_sha256=sha(target),
        lighting=lighting,geometry=geometry,material_records=len(records),donors=donors,runtime_verified=False))


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for flag in ['source','imported','output','main-hud','report']:p.add_argument('--'+flag,required=True,type=Path)
    publish(**vars(p.parse_args()))
