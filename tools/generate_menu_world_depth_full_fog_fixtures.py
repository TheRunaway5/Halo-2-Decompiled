#!/usr/bin/env python3
"""Export pinned full-fog inputs offline; no compilation, original execution or GPU.

The eye bank/background/depth is actual event1872. The selected HILO bank is
unchanged-XBE execution, not a captured underwater framebuffer. Uses stdlib
only, including literal PNG byte decoding without color-space conversion.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys
import zlib

ROOT = Path(__file__).resolve().parents[1]
CAPTURE = ROOT / 'analysis/frontend/xemu-capture'
PINS = {
    'selected-fog-after.json': '0ef63cbb1cd06903bf41d5c00f7bb5e104772436ffb42d88382bbdff62338875',
    'primary-fog-details.json': 'e4fdfecd3b3b2d8cb5a3b0744942f5297cddc1160669f66eaedc1d82c54286ea',
    'pass-1872-4-uniforms.bin': '741ab6feef83846b296c03cccf1eaf8224f0cd3d10e705a650c28d9ee47b5688',
    'title-haze-gradient.bin': 'b4980da59a29dbe1498a9fbc802d7f5fdff66c262b4c55ec3a22c41260a08c85',
    'eye-fog-depth.bin': 'b85697a801046855a8a1ed1c6ff9a548df4774c8970c49e69633294d2dbc77e7',
    'pass-1865.png': 'fd007ba3f4309f9076624b7ee4c6506d3fcd8775d88243832b8abafeeb264cf3',
    'pass-1872.png': '467f1d505e016a09b546e993ad5122b00df00032c90760ca85314e728b9f6308',
}
XBE_SHA = '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
MAP_SHA = '1b5de78212ee7439b086ef3554885a4188884864ce1040b24ec6604af4a0143b'


def require(value, message):
    if not value:
        raise ValueError(message)


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def pinned(path, identity):
    raw = path.read_bytes()
    require(sha(raw) == identity, 'source identity differs: ' + str(path))
    return raw


def rgba_png(raw):
    require(raw[:8] == b'\x89PNG\r\n\x1a\n', 'expected PNG')
    at, packed, header, ended = 8, bytearray(), None, False
    while at < len(raw):
        require(at + 12 <= len(raw), 'truncated PNG chunk')
        length = struct.unpack_from('>I', raw, at)[0]
        kind = raw[at + 4:at + 8]
        require(at + 12 + length <= len(raw), 'PNG chunk outside allocation')
        data = raw[at + 8:at + 8 + length]
        require(zlib.crc32(kind + data) == struct.unpack_from('>I', raw, at + 8 + length)[0], 'PNG CRC differs')
        if kind == b'IHDR':
            require(header is None and length == 13, 'PNG IHDR differs')
            header = struct.unpack('>IIBBBBB', data)
        elif kind == b'IDAT':
            packed.extend(data)
        elif kind == b'IEND':
            require(length == 0, 'PNG IEND differs')
            ended = True
        at += length + 12
    require(ended and at == len(raw) and header == (640, 480, 8, 6, 0, 0, 0), 'unsupported captured PNG')
    scanlines = zlib.decompress(packed)
    stride = 640 * 4
    require(len(scanlines) == 480 * (stride + 1), 'PNG scanline size differs')
    previous, result = bytearray(stride), bytearray()
    for y in range(480):
        at = y * (stride + 1)
        mode, row = scanlines[at], bytearray(scanlines[at + 1:at + 1 + stride])
        require(mode <= 4, 'unknown PNG row filter')
        for x in range(stride):
            left, up, corner = row[x - 4] if x >= 4 else 0, previous[x], previous[x - 4] if x >= 4 else 0
            if mode == 1:
                prediction = left
            elif mode == 2:
                prediction = up
            elif mode == 3:
                prediction = (left + up) // 2
            elif mode == 4:
                estimate = left + up - corner
                distances = [abs(estimate - v) for v in (left, up, corner)]
                prediction = (left, up, corner)[distances.index(min(distances))]
            else:
                prediction = 0
            row[x] = (row[x] + prediction) & 255
        result.extend(row)
        previous = row
    return bytes(result)


def authored_gradient(path):
    sys.path.insert(0, str(ROOT / 'tools'))
    from audit_rendering import Cache
    cache = Cache(path)
    try:
        matg = next(t for t in cache.tags.values() if t['group'] == 'matg')
        bank = cache.block(matg['address'] - cache.magic + 0x108, 0x1c8)
        require(len(bank) == 1, 'rasterizer count differs')
        datum = cache.u32(bank[0] + 0x14)
        require(datum == 0xe4570237, 'gradient datum differs')
        record = cache.bitmaps[cache.bitmap(datum)]['records'][0]
        require([record[k] for k in ('width', 'height', 'depth', 'type', 'format', 'declared_additional_mips')] == [16, 16, 1, 0, 11, 0], 'gradient layout differs')
        require(record['flags'] & 8 and record['resource_sizes'][0] == 1024, 'gradient resource differs')
        pointer = record['resource_pointers'][0]
        cache.require(pointer, 1024)
        raw = cache.data[pointer:pointer + 1024]
        result = bytearray()
        for y in range(16):
            for x in range(16):
                at = sum((((x >> i) & 1) << (2 * i)) | (((y >> i) & 1) << (2 * i + 1)) for i in range(4)) * 4
                b, g, r, a = raw[at:at + 4]
                result.extend((r, g, b, a))
        return bytes(result)
    finally:
        cache.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT / 'analysis/frontend/title-camera-matrix-audit/world-depth-full-fog-fixtures')
    parser.add_argument('--verify-color-export', type=Path, help='owner-run typed raw export; emits the required RGBA-authority guard only after exact comparison')
    args = parser.parse_args()
    raw = {name: pinned(CAPTURE / name, identity) for name, identity in PINS.items()}
    pinned(ROOT / 'assets/disc/default.xbe', XBE_SHA)
    pinned(ROOT / 'assets/disc/maps/mainmenu.map', MAP_SHA)
    require(authored_gradient(ROOT / 'assets/disc/maps/mainmenu.map') == raw['title-haze-gradient.bin'], 'captured gradient differs from authored image')
    selected = json.loads(raw['selected-fog-after.json'])
    require(selected['cases'] == 3598 and selected['failures'] == 0 and selected['xbe_sha256'] == XBE_SHA, 'selected original execution proof differs')
    hilo = selected['fixtures'][0]
    require(hilo['mode'] == 5 and hilo['words'][54] == 0x2621 and hilo['eye_z'] == 215.82656042480468, 'selected HILO branch differs')
    eye_words = [0] * 60
    eye_words[8], eye_words[9], eye_words[43], eye_words[53] = 0x01200000, 0x1100, 0x41201b16, 0x11001
    eye = struct.pack('<60I8f', *eye_words, *([0.] * 8))
    require(struct.unpack_from('<4f', raw['pass-1872-4-uniforms.bin'], 544 + 16 * 16) == tuple(struct.unpack('<4f', struct.pack('<4f', *(v / 255 for v in (32, 27, 22, 65))))), 'captured eye final constant differs')
    report = json.loads(raw['primary-fog-details.json'])
    blend = next(r for r in report['passes'] if r['event'] == 1872)['blend']
    require(blend['blendFactor'] == [1., 1., 1., 0.] and not blend['alphaToCoverageEnable'] and not blend['alphaToOneEnable'], 'captured fog blend factor differs')
    attachment = blend['blends'][0]
    require(len(blend['blends']) == 1 and attachment['enabled'] and not attachment['logicOperationEnabled'] and attachment['writeMask'] == 15, 'captured blend policy differs')
    require(all([attachment[k][v] for v in ('source', 'destination', 'operation')] == [11, 7, 0] for k in ('colorBlend', 'alphaBlend')), 'captured blend operands differ')
    # RD FactorRGB11/InvSrcAlpha7/Add0 -> Vulkan CONSTANT_COLOR10/7/0.
    blend_words = [0x46424731, 10, 7, 0, 10, 7, 0, 15, 0x3f800000, 0x3f800000, 0x3f800000, 0]
    pixels = 640 * 480
    require(len(raw['eye-fog-depth.bin']) == pixels * 5, 'captured eye depth/stencil length differs')
    depths = [v[0] & 0xffffff for v in struct.iter_unpack('<I', raw['eye-fog-depth.bin'][:pixels * 4])]
    require(min(depths) == 97960 and depths.count(0xffffff) == 118320, 'captured eye depth boundary differs')
    visibility = bytes(255 if n == 0xffffff else 0 for n in depths)
    files = {
        'eye272.bin': eye,
        'hilo272.bin': struct.pack('<60I8f', *hilo['words'], *hilo['coordinates']),
        'gradient.rgba': raw['title-haze-gradient.bin'],
        'eye-depth-u32.bin': struct.pack('<' + 'I' * pixels, *depths),
        'eye-stencil-u8.bin': raw['eye-fog-depth.bin'][pixels * 4:],
        'eye-before.rgba': rgba_png(raw['pass-1865.png']),
        'eye-after.rgba': rgba_png(raw['pass-1872.png']),
        'eye-visibility.r8': visibility,
        'blend-state.bin': struct.pack('<12I', *blend_words),
    }
    require(sha(files['eye272.bin']) == 'b5f3d45af8ef267ca50625ff7759be63957de190d978bb7457e4f1cbd6f1520c' and sha(files['hilo272.bin']) == 'e689b84844539c17aa29dd4f2094b1293d19f733d0d1086a90259e372cd18a92', 'fog uniform identity differs')
    metadata = dict(version='H2_WORLD_DEPTH_FULL_FOG_V1', source_pins=PINS, xbe_sha256=XBE_SHA, map_sha256=MAP_SHA,
        eye_authority='captured event1872', hilo_authority='unchanged selected-XBE execution; not captured underwater framebuffer',
        uniform_layout=dict(words=60, lookup_float4_offsets=[240, 256], bytes=272),
        gradient=dict(width=16, height=16, bitmap=0, datum='e4570237', authored_and_capture_bytes_equal=True),
        controlled_background='Actual pre1872 RGBA used as controlled input for other depths/HILO; only own eye depth is captured-frame comparison',
        sampled_prior_D32='Controlled GPU seed, not actual prior native draw history',
        compiled=False, gpu_executed=False, files={name: dict(bytes=len(b), sha256=sha(b)) for name, b in files.items()})
    files['manifest.tsv'] = ('H2_WORLD_DEPTH_FULL_FOG_V1\n640\t480\t118320\t' + '\t'.join(files) + '\n').encode()
    files['manifest.json'] = (json.dumps(metadata, indent=2, sort_keys=True) + '\n').encode()
    if args.verify_color_export:
        source = args.verify_color_export / 'original-color.json'
        report_bytes = source.read_bytes()
        report = json.loads(report_bytes)
        require(report['status'] == 'exported' and not report['failures'] and report['capture_sha256'] == '077524887d852e46af2e3cb8eb85c52dd777b8bda3e86039c29e904a2450b431', 'raw-color capture identity differs')
        require([r['event'] for r in report['events']] == [1865, 1872], 'raw-color event order differs')
        proof = []
        for row in report['events']:
            d, fmt = row['description'], row['format']
            require([d[k] for k in ('width', 'height', 'depth', 'arraysize', 'mips', 'msSamp')] == [640, 480, 1, 1, 1, 1], 'raw-color image layout differs')
            require([fmt[k] for k in ('type', 'compByteWidth', 'compCount', 'compType')] == [0, 1, 4, 2], 'expected original regular four-byte UNorm color')
            require(type(row['bgra_order']) is bool and row['format_name'], 'typed original color-order metadata missing')
            name = row['raw']['file']
            require(Path(name).name == name, 'raw-color file escapes export')
            color = pinned(args.verify_color_export / name, row['raw']['sha256'])
            require(len(color) == row['raw']['bytes'] == pixels * 4, 'raw-color size differs')
            rgba = bytearray(color)
            if row['bgra_order']:
                rgba[0::4], rgba[2::4] = color[2::4], color[0::4]
            expected = files['eye-before.rgba' if row['event'] == 1865 else 'eye-after.rgba']
            require(rgba == expected, 'typed raw captured RGBA differs from PNG; no fitted mapping admitted')
            proof.append(dict(event=row['event'], format_name=row['format_name'], bgra_order=row['bgra_order'], raw_sha256=sha(color),
                rgba_sha256=sha(rgba), png_and_raw_different_bytes=0, alpha_and_raw_different_bytes=0))
        files['color-authority.bin'] = struct.pack('<I', 0x46474331)
        files['color-authority.json'] = (json.dumps(dict(status='verified', capture_sha256=report['capture_sha256'],
            raw_export_report_sha256=sha(report_bytes), source_export=str(source.resolve()), events=proof,
            authority='GetTextureData typed UNorm/BGRAOrder mapping; all four channels exact, no guessed alpha'), indent=2) + '\n').encode()
    args.output.mkdir(parents=True, exist_ok=True)
    for name, data in files.items():
        path = args.output / name
        require(not path.exists() or path.read_bytes() == data, 'refusing different retained fixture: ' + str(path))
    for name, data in files.items():
        (args.output / name).write_bytes(data)
    print(json.dumps(dict(status='prepared', files=len(files), manifest_sha256=sha(files['manifest.tsv']), gpu_executed=False)))


if __name__ == '__main__':
    main()
