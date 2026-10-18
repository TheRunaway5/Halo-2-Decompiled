// Literal surface program from the native Prophet collar capture, event860.
// Execute the production interpreter and viewport conversion, so this fixture
// reproduces the actual surface/cap arithmetic mismatch (not a matrix-only proxy).
struct CapturedMaterial {uvec4 flags;uvec4 vertexInstructions[26];vec4 vertexAttributes[16];};
CapturedMaterial material;
#define H2_VERTEX_REFERENCE_INPUTS
#define H2_VERTEX_REFERENCE_CONSTANTS
vec4 vpConstant(uint index);
#include "vertex_program.glsl"
vec4 vpConstant(uint index) {
    if(index<4u)return transpose(camera.matrix)[index];
    if(index>=8u && index<12u)return retailViewportConstant(index,camera.viewport,1.0,false,false,0u);
    return vec4(0);
}
const uvec4 capturedInstructions[26]=uvec4[26](
    uvec4(0u,6348826u,2080380008u,776994816u),
    uvec4(0u,12582939u,137762816u,548440064u),
    uvec4(0u,10487834u,1412730880u,681574400u),
    uvec4(0u,10487322u,1412722688u,614465536u),
    uvec4(0u,10486810u,1412714496u,580911104u),
    uvec4(0u,12591131u,137771008u,548423680u),
    uvec4(0u,10485786u,2754955264u,564133888u),
    uvec4(0u,12599323u,137779200u,548415488u),
    uvec4(0u,146825243u,137788414u,2427525120u),
    uvec4(0u,9798165u,405510911u,1016070144u),
    uvec4(0u,111150106u,674540543u,296222720u),
    uvec4(0u,8536085u,2485803772u,3163605064u),
    uvec4(0u,4195416u,704595968u,665845760u),
    uvec4(0u,4194330u,2784897024u,782237696u),
    uvec4(0u,10709274u,3157561344u,562036736u),
    uvec4(0u,10708762u,3157553152u,679477248u),
    uvec4(0u,10708250u,3157544960u,578813952u),
    uvec4(0u,4276250u,3291830272u,547416064u),
    uvec4(0u,8437978u,3053378665u,1353771088u),
    uvec4(0u,8552469u,2485820157u,816891992u),
    uvec4(0u,10701082u,2889125888u,548440160u),
    uvec4(0u,10700570u,2889117696u,548423776u),
    uvec4(0u,10700058u,2889109504u,548415584u),
    uvec4(0u,8429594u,2784934909u,1889593368u),
    uvec4(0u,8429770u,2248063997u,1889593376u),
    uvec4(0u,8478746u,3288344682u,4038125569u)
);
const uvec4 capturedHandInstructions[12]=uvec4[12](
    uvec4(0u,12582939u,137762924u,544245760u),
    uvec4(0u,12591131u,137762924u,544229376u),
    uvec4(0u,12599323u,137762924u,544221184u),
    uvec4(0u,12607515u,137762924u,544217088u),
    uvec4(0u,9798171u,136977148u,1016074232u),
    uvec4(0u,2102299u,137760876u,544274456u),
    uvec4(0u,8536091u,2485787388u,1016121416u),
    uvec4(0u,104939547u,3291880447u,269608960u),
    uvec4(0u,8544283u,2485787388u,812697680u),
    uvec4(0u,4358171u,2485786732u,544262232u),
    uvec4(0u,4358171u,2508068972u,544262240u),
    uvec4(0u,8478747u,3321768040u,812705793u)
);
void capturedSurface() {
    bool hand=abs(camera.color.w)>3.5;
    material.flags=uvec4(0,0,0,hand?12u:26u);
    for(uint i=0u;i<material.flags.w;++i)material.vertexInstructions[i]=hand?capturedHandInstructions[i]:capturedInstructions[i];
    for(uint i=0u;i<16u;++i)material.vertexAttributes[i]=vec4(0);
    material.vertexAttributes[0]=vec4(point.xyz,1);
    runVertexProgram();
    vec4 screen=vpOut[0];
    screen.xy=trunc(screen.xy*16.0)/16.0;
    screen.w=(screen.w<0.0?-1.0:1.0)*clamp(abs(screen.w),5.421010862427522e-20,1.8446744073709552e19);
    vec2 ndc=(screen.xy-camera.viewport.xy)*2.0/camera.viewport.zw-1.0;
    gl_Position=vec4(ndc*screen.w,(screen.z/16777215.0)*screen.w,screen.w);
    sourceDepth=dot(vec4(point.xyz,1),vpConstant(3u));
}
