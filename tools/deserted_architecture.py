"""Select authored architecture by compiled material ownership, never batch ID."""
import numpy as np

SHADERS = {
    'scenarios/shaders/solo/earthcity/new_mombassa/nm_concrete_floor',
    'scenarios/shaders/solo/earthcity/base_stucco3',
    'scenarios/shaders/solo/earthcity/base_stucco1_grim',
    'scenarios/shaders/solo/earthcity/base_stucco4',
    'scenarios/shaders/multi/zanzibar/massive_concrete',
    'scenarios/shaders/multi/zanzibar/steel_weathered',
    'scenarios/shaders/multi/turf/windows_storefront',
    'scenarios/shaders/multi/turf/light_long_a',
    'scenarios/shaders/solo/earthcity/base_paver',
    'scenarios/shaders/solo/earthcity/base_sidewalk',
    'scenarios/shaders/solo/earthcity/base_stucco1_door',
    'scenarios/shaders/solo/earthcity/base_metal',
    'scenarios/shaders/solo/earthcity/base_concrete1',
    'scenarios/multi/dust2_mombasa/shaders/polished_road',
    'scenarios/multi/dust2_mombasa/shaders/polished_arch_emitter',
    'scenarios/multi/dust2_mombasa/shaders/polished_practical_emitter',
}


def architecture_sections(cache, bsp):
    materials = cache.block(bsp['offset']+164, 32)
    selected = {i for i,a in enumerate(materials)
        if cache.tags[cache.u32(a+12)]['name'].replace('\\','/') in SHADERS}
    assert selected, 'No authored architectural materials in compiled BSP'
    definitions = cache.block(bsp['offset']+312, 200)
    result = {}
    for ii, inst in enumerate(cache.block(bsp['offset']+320, 88)):
        at = definitions[cache.u16(inst+52)]
        if not cache.u16(at):continue
        base = cache.u32(at+40)+cache.u32(at+48)+8
        resources = cache.block(at+56,16)
        parts = resources[0]
        mask = np.zeros(cache.u16(at+2), dtype=bool)
        for p in range(cache.u32(parts+8)//72):
            part=base+cache.u32(parts+12)+p*72
            if cache.u16(part+4) not in selected:continue
            first,count=cache.u16(part+6),cache.u16(part+8)
            assert first%3==0 and count%3==0
            mask[first//3:(first+count)//3]=True
        if mask.any():result[ii]=mask
    assert result, 'No compiled architectural triangles selected'
    return result
