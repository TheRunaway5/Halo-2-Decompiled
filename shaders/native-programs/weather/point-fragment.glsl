vec4 registers[16];
vec4 argb(uint c){return vec4((c>>16u)&255u,(c>>8u)&255u,c&255u,c>>24u)/255.0;}
vec4 runCombiners(vec4 textures[4],vec4 v0,vec4 v1){
registers[0]=vec4(0);
registers[1]=vec4(0);
registers[2]=vec4(0);
registers[3]=vec4(0);
registers[4]=vec4(0);
registers[5]=vec4(0);
registers[6]=vec4(0);
registers[7]=vec4(0);
registers[8]=vec4(0);
registers[9]=vec4(0);
registers[10]=vec4(0);
registers[11]=vec4(0);
registers[12]=vec4(0);
registers[13]=vec4(0);
registers[14]=vec4(0);
registers[15]=vec4(0);
#ifdef H2_COMBINER_FOG
registers[3]=H2_COMBINER_FOG();
#endif
registers[4]=v0;registers[5]=v1;
registers[8]=textures[0];
registers[9]=textures[1];
registers[10]=textures[2];
registers[11]=textures[3];
registers[12].a=1.0;
{
registers[1]=vec4(0,0,0,0);
registers[2]=vec4(0,0,0,0);
vec3 rrawab=(registers[4].rgb*registers[11].rgb),rrawcd=(max(registers[0].rgb,vec3(0))*max(registers[0].rgb,vec3(0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(vec3(registers[4].a)*vec3(registers[11].a)),arawcd=(max(vec3(registers[0].a),vec3(0))*max(vec3(registers[0].a),vec3(0)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
registers[12].rgb=rab;
registers[12].a=aab.x;
}
registers[1]=vec4(0,0,0,0);registers[2]=vec4(0,0,0,0);
registers[14]=vec4(clamp((registers[5].rgb+registers[12].rgb),0.0,1.0),0);
registers[15]=vec4(max(registers[0].rgb,vec3(0))*max(registers[0].rgb,vec3(0)),0);
return clamp(vec4(max(registers[12].rgb,vec3(0))*(vec3(1)-clamp(registers[0].rgb,0.0,1.0))+(vec3(1)-max(registers[12].rgb,vec3(0)))*max(registers[0].rgb,vec3(0))+max(registers[0].rgb,vec3(0)),max(vec3(registers[12].a),vec3(0)).x),0.0,1.0);}
