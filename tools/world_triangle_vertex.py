"""Build-time geometry-free wrapper for the admitted compact world GLSL ABI.

Original program arithmetic and epilogue remain verbatim. Only declared raw
input fetches and the entry wrapper change; no runtime compiler or CPU plane
calculation is introduced. Unknown input/function layouts are rejected.
"""
from pathlib import Path
import re


def triangle_vertex_source(source: str, semantics: list[int], consumed: list[int], shader_root: Path) -> str:
    marker='void main() {\n'
    if source.count(marker)!=1 or not source.endswith('}\n'):
        raise ValueError('world triangle wrapper requires the admitted main-function layout')
    prelude,body=source.split(marker)
    inputs={}
    pattern=r'layout\(location=(\d+)\) in (vec4|int) (v\d+(?:_cmp)?);\n'
    for match in re.finditer(pattern,prelude):
        location=int(match[1]);expected_type='int' if semantics[location] in (4,5,6) else 'vec4'
        expected_name=f'v{location}'+('_cmp' if expected_type=='int' else '')
        if location in inputs or match[2]!=expected_type or match[3]!=expected_name:
            raise ValueError('world triangle wrapper changed raw input declaration')
        inputs[location]=(match[2],match[3])
    if sorted(inputs)!=list(range(len(semantics))):
        raise ValueError('world triangle wrapper lacks complete raw input declarations')
    prelude=re.sub(pattern,'',prelude)
    if re.search(r'layout\([^\n]*\)\s+in\b',prelude):
        raise ValueError('world triangle wrapper has an unadmitted input interface')
    for location in consumed:
        if location>=len(semantics):
            if body.count(f'    vpInput[{location}]=source.attributes[{location}];\n')!=1:
                raise ValueError('world triangle wrapper lost external attribute binding')
            continue
        kind,name=inputs[location]
        old=f'    vpInput[{location}]='+('sourcePackedNormal('+name+')' if kind=='int' else name)+';\n'
        if body.count(old)!=1:
            raise ValueError('world triangle wrapper changed raw input assignment')
        fetch=f'sourceWorldFetchInput({location}u,sourceVertex)'
        new=f'    vpInput[{location}]='+('sourcePackedNormal(int('+fetch+'.x))' if kind=='int' else 'uintBitsToFloat('+fetch+')')+';\n'
        body=body.replace(old,new)
    if re.search(r'\bv\d+(?:_cmp)?\b',body):
        raise ValueError('world triangle wrapper retains an unbound raw attribute')
    # The final captured epilogue must continue to publish the original screen
    # coordinates, not a new camera or a host-evaluated transform.
    if body.count('v_vtxPos0=screen;v_vtxPos1=screen;v_vtxPos2=screen;v_triMZ=0.0;')!=1:
        raise ValueError('world triangle wrapper changed source epilogue')
    return prelude+'''
layout(set=1,binding=2,std430) readonly buffer SourceWorldRawVertices {uint words[];} sourceWorldRaw;
layout(set=1,binding=3,std430) readonly buffer SourceWorldTriangles {
    uvec4 streams[16];
    uint indices[];
} sourceWorldTriangles;
uint sourceWorldVertexWord(uint index){return sourceWorldRaw.words[index];}
'''+(shader_root/'source_world_vertex_fetch.glsl').read_text()+(shader_root/'world_depth_geometry.glsl').read_text()+'''
uvec4 sourceWorldFetchInput(uint reg,uint vertex) {
    uvec4 stream=sourceWorldTriangles.streams[reg];
    return sourceWorldVertexFetch(stream.x+vertex*stream.y,stream.z);
}
void sourceWorldRunVertex(uint sourceVertex) {
'''+body+'''
void main() {
    uint ordinal=uint(gl_VertexIndex);
    uint first=(ordinal/3u)*3u;
    uint corner=ordinal%3u;
    vec4 windows[3];
    // Execute the other corners first and the selected corner last. This
    // retains its complete original varying/clip outputs without storing or
    // interpolating another corner's attributes. Integer order alone changes.
    for(uint turn=1u;turn<=3u;++turn) {
        uint lane=(corner+turn)%3u;
        sourceWorldRunVertex(sourceWorldTriangles.indices[first+lane]);
        windows[lane]=v_vtxPos0;
    }
    mat4 triangle=world_depth_triangle(windows[0],windows[1],windows[2]);
    v_vtxPos0=triangle[0];v_vtxPos1=triangle[1];v_vtxPos2=triangle[2];
    v_triMZ=world_depth_finite_slope(triangle);
}
'''
