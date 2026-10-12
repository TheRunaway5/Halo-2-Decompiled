"""Extract immutable depth expressions and prepare isolated GPU-test inputs.

This CPU tool does not run Vulkan or edit a production shader. Negative/clamp
controls are encoding-stage inputs, not complete original fragment admission.
"""
from pathlib import Path
import argparse
import hashlib
import json
import math
import re
import struct

ROOT = Path(__file__).resolve().parents[1]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def f32(value):
    return struct.unpack('<f', struct.pack('<f', value))[0]


def main():
    parser=argparse.ArgumentParser();parser.add_argument('output',type=Path)
    parser.add_argument('--native',type=Path,default=ROOT/'shaders/world.frag',
        help='Exact pre-correction native shader to diagnose, retained by hash.')
    args=parser.parse_args();out=args.output.resolve();out.mkdir(parents=True,exist_ok=True)
    native=args.native.resolve()
    primary=ROOT/'analysis/multiplayer-shading/stipple-coordinate-audit/xemu-retained/hw/xbox/nv2a/pgraph/glsl/psh.c'
    captured=ROOT/'analysis/multiplayer-shading/appearance-residual-replay/source/8878-fragment.spvasm'
    text=native.read_text()
    matches=re.findall(r'gl_FragDepth\s*=\s*(.*?);',text,re.S)
    assert len(matches)==1
    current=matches[0].strip()
    assert 'material.sourceCameraControl.x!=0u ? titleDepth.x : titleDepth.x*16777215.0' in current
    assert current.endswith('/16777215.0 : gl_FragCoord.z')
    source='uintBitsToFloat(floatBitsToUint(floor(zvalue) / 16777216.0) + 1u)'
    assert f'gl_FragDepth = {source};' in primary.read_text()
    observed=captured.read_text()
    for token in ('GLSL.std.450::Floor(_662)', '_663 / 16777216.0000', '_666 + 1', '*gl_FragDepth = _668'):
        assert token in observed
    generated='''// Isolated extraction; the current expression below is verbatim world.frag.
struct MaterialProxy { uvec4 sourceCameraControl; };
float currentEncoding(float w) {
    MaterialProxy material;
    material.sourceCameraControl=uvec4(pc.sourceControl,0,0,0);
    vec2 titleDepth=vec2(w,uintBitsToFloat(pc.wMode));
    return '''+current+''';
}
// The pinned expression is verbatim; zvalue is an admitted encoding input.
float sourceEncoding(float zvalue) { return '''+source+'''; }
'''
    (out/'depth_expressions.glsl').write_text(generated)
    values=[-f32(3.4028234663852886e38),-16777216.,-1.,-f32(2**-149),-0.,0.,f32(2**-149),.25,.5,.75,
            f32(1.-2**-24),1.,f32(1.+2**-23),1.5,1.75,2.,255.5,65535.5,65536.,65536.5,
            8388606.5,8388607.,8388607.5,8388608.,8388609.,16777213.,16777214.,16777215.,16777216.,
            f32(3.4028234663852886e38)]
    assert all(math.isfinite(f32(value)) for value in values)
    (out/'fractional-w.bin').write_bytes(struct.pack(f'<{len(values)}f',*values))
    controls=[0.,1.,8388607.,8388608.,16777214.,16777215.,-1.,16777216.]
    (out/'compare-w.bin').write_bytes(struct.pack('<8f',*controls))
    files=[native,primary,captured,Path(__file__),out/'depth_expressions.glsl',out/'fractional-w.bin',out/'compare-w.bin']
    report=dict(scope=__doc__,current_expression=current,source_expression=source,
        source_xemu_commit='fc24584ce88f0915ad7f04775bb7712c2e3f49ee',
        exhaustive_integers=1<<24,sourceCameraControl_x=1,titleDepth_y_word='3F800000',
        fractional_cases=len(values),comparison_cases=len(controls),
        common_source_input_clamp=[0,16777215],
        limits=['No original interpolation, discard, camera or all-map draw claim.',
                'Common clamp is explicit diagnostic input preparation, not an inferred original negative-W branch.',
                'The pinned xemu encoding is not physical Xbox raster precision authority.'],
        files=[dict(path=str(path.relative_to(ROOT)) if path.is_relative_to(ROOT) else str(path),
                    bytes=path.stat().st_size,sha256=sha(path)) for path in files])
    (out/'inputs.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(output=str(out),current_sha256=sha(native),fractional_cases=len(values)),indent=2))


if __name__=='__main__':main()
