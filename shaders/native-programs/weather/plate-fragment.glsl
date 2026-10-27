// Generated finite original 3A5C0 control tuples. Constants are uniform data.
layout(constant_id=0)const uint nativeWeatherPlateVariant=0u;
vec4 nativeWeatherPlate0Registers[16];
vec4 nativeWeatherPlate0Argb(uint c){return vec4((c>>16u)&255u,(c>>8u)&255u,c&255u,c>>24u)/255.0;}
vec4 nativeWeatherPlate0(vec4 textures[4],vec4 v0,vec4 v1){
nativeWeatherPlate0Registers[0]=vec4(0);
nativeWeatherPlate0Registers[1]=vec4(0);
nativeWeatherPlate0Registers[2]=vec4(0);
nativeWeatherPlate0Registers[3]=vec4(0);
nativeWeatherPlate0Registers[4]=vec4(0);
nativeWeatherPlate0Registers[5]=vec4(0);
nativeWeatherPlate0Registers[6]=vec4(0);
nativeWeatherPlate0Registers[7]=vec4(0);
nativeWeatherPlate0Registers[8]=vec4(0);
nativeWeatherPlate0Registers[9]=vec4(0);
nativeWeatherPlate0Registers[10]=vec4(0);
nativeWeatherPlate0Registers[11]=vec4(0);
nativeWeatherPlate0Registers[12]=vec4(0);
nativeWeatherPlate0Registers[13]=vec4(0);
nativeWeatherPlate0Registers[14]=vec4(0);
nativeWeatherPlate0Registers[15]=vec4(0);
#ifdef H2_COMBINER_FOG
nativeWeatherPlate0Registers[3]=H2_COMBINER_FOG();
#endif
nativeWeatherPlate0Registers[4]=v0;nativeWeatherPlate0Registers[5]=v1;
nativeWeatherPlate0Registers[8]=textures[0];
nativeWeatherPlate0Registers[9]=textures[1];
nativeWeatherPlate0Registers[10]=textures[2];
nativeWeatherPlate0Registers[11]=textures[3];
nativeWeatherPlate0Registers[12].a=textures[0].a;
{
nativeWeatherPlate0Registers[1]=material.constants[0];
nativeWeatherPlate0Registers[2]=material.constants[1];
vec3 rrawab=vec3(dot(max(nativeWeatherPlate0Registers[8].rgb,vec3(0)),max(nativeWeatherPlate0Registers[1].rgb,vec3(0)))),rrawcd=vec3(dot(max(nativeWeatherPlate0Registers[8].rgb,vec3(0)),max(nativeWeatherPlate0Registers[2].rgb,vec3(0))));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate0Registers[0].b),vec3(0))*max(vec3(nativeWeatherPlate0Registers[0].b),vec3(0))),arawcd=(max(vec3(nativeWeatherPlate0Registers[0].b),vec3(0))*max(vec3(nativeWeatherPlate0Registers[0].b),vec3(0)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate0Registers[4].rgb=rab;
nativeWeatherPlate0Registers[5].rgb=rcd;
}
{
nativeWeatherPlate0Registers[1]=material.constants[2];
nativeWeatherPlate0Registers[2]=material.constants[3];
vec3 rrawab=(max(nativeWeatherPlate0Registers[4].rgb,vec3(0))*(vec3(1)-clamp(nativeWeatherPlate0Registers[0].rgb,0.0,1.0))),rrawcd=(max(nativeWeatherPlate0Registers[1].aaa,vec3(0))*(2.0*max(nativeWeatherPlate0Registers[0].rgb,vec3(0))-vec3(1)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate0Registers[4].b),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate0Registers[0].b),0.0,1.0))),arawcd=((max(vec3(nativeWeatherPlate0Registers[1].a),vec3(0))-vec3(0.5))*(2.0*max(vec3(nativeWeatherPlate0Registers[0].b),vec3(0))-vec3(1)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate0Registers[12].rgb=rsum;
nativeWeatherPlate0Registers[12].a=asum.x;
}
{
nativeWeatherPlate0Registers[1]=material.constants[4];
nativeWeatherPlate0Registers[2]=material.constants[5];
vec3 rrawab=(max(nativeWeatherPlate0Registers[0].rgb,vec3(0))*max(nativeWeatherPlate0Registers[0].rgb,vec3(0))),rrawcd=((vec3(1)-clamp(nativeWeatherPlate0Registers[0].rgb,0.0,1.0))*(vec3(1)-clamp(nativeWeatherPlate0Registers[0].rgb,0.0,1.0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp(((nativeWeatherPlate0Registers[12].a>=0.5)?rrawcd:rrawab),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate0Registers[8].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate0Registers[0].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate0Registers[1].a),vec3(0))*(2.0*max(vec3(nativeWeatherPlate0Registers[0].b),vec3(0))-vec3(1)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate0Registers[13].rgb=rsum;
nativeWeatherPlate0Registers[13].a=asum.x;
}
{
nativeWeatherPlate0Registers[1]=material.constants[6];
nativeWeatherPlate0Registers[2]=material.constants[7];
vec3 rrawab=(nativeWeatherPlate0Registers[12].rgb*(vec3(1)-clamp(nativeWeatherPlate0Registers[0].rgb,0.0,1.0))),rrawcd=((vec3(1)-clamp(nativeWeatherPlate0Registers[13].rgb,0.0,1.0))*(vec3(1)-clamp(nativeWeatherPlate0Registers[0].rgb,0.0,1.0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(vec3(nativeWeatherPlate0Registers[13].a)*(vec3(1)-clamp(vec3(nativeWeatherPlate0Registers[0].b),0.0,1.0))),arawcd=((vec3(1)-clamp(vec3(nativeWeatherPlate0Registers[13].b),0.0,1.0))*(-vec3(nativeWeatherPlate0Registers[1].a)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate0Registers[12].rgb=rsum;
nativeWeatherPlate0Registers[13].a=asum.x;
}
{
nativeWeatherPlate0Registers[1]=material.constants[8];
nativeWeatherPlate0Registers[2]=material.constants[9];
vec3 rrawab=vec3(dot(max(nativeWeatherPlate0Registers[9].rgb,vec3(0)),max(nativeWeatherPlate0Registers[1].rgb,vec3(0)))),rrawcd=vec3(dot(max(nativeWeatherPlate0Registers[10].rgb,vec3(0)),max(nativeWeatherPlate0Registers[2].rgb,vec3(0))));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate0Registers[13].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate0Registers[0].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate0Registers[13].a),vec3(0))*max(vec3(nativeWeatherPlate0Registers[0].b),vec3(0)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate0Registers[4].rgb=rab;
nativeWeatherPlate0Registers[5].rgb=rcd;
nativeWeatherPlate0Registers[13].a=asum.x;
}
{
nativeWeatherPlate0Registers[1]=material.constants[10];
nativeWeatherPlate0Registers[2]=material.constants[11];
vec3 rrawab=((vec3(1)-clamp(nativeWeatherPlate0Registers[4].rgb,0.0,1.0))*(vec3(1)-clamp(nativeWeatherPlate0Registers[5].rgb,0.0,1.0))),rrawcd=vec3(dot(max(nativeWeatherPlate0Registers[11].rgb,vec3(0)),max(nativeWeatherPlate0Registers[1].rgb,vec3(0))));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate0Registers[13].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate0Registers[0].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate0Registers[13].a),vec3(0))*max(vec3(nativeWeatherPlate0Registers[0].b),vec3(0)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate0Registers[4].rgb=rab;
nativeWeatherPlate0Registers[5].rgb=rcd;
nativeWeatherPlate0Registers[13].a=asum.x;
}
{
nativeWeatherPlate0Registers[1]=material.constants[12];
nativeWeatherPlate0Registers[2]=material.constants[13];
vec3 rrawab=(max(nativeWeatherPlate0Registers[4].rgb,vec3(0))*(vec3(1)-clamp(nativeWeatherPlate0Registers[5].rgb,0.0,1.0))),rrawcd=(max(nativeWeatherPlate0Registers[0].rgb,vec3(0))*max(nativeWeatherPlate0Registers[0].rgb,vec3(0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate0Registers[13].a),vec3(0))*max(vec3(nativeWeatherPlate0Registers[2].a),vec3(0))),arawcd=(max(vec3(nativeWeatherPlate0Registers[12].b),vec3(0))*max(vec3(nativeWeatherPlate0Registers[2].b),vec3(0)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate0Registers[13].rgb=rab;
nativeWeatherPlate0Registers[13].a=asum.x;
}
{
nativeWeatherPlate0Registers[1]=material.constants[14];
nativeWeatherPlate0Registers[2]=material.constants[15];
vec3 rrawab=(max(nativeWeatherPlate0Registers[0].rgb,vec3(0))*max(nativeWeatherPlate0Registers[0].rgb,vec3(0))),rrawcd=(max(nativeWeatherPlate0Registers[0].rgb,vec3(0))*max(nativeWeatherPlate0Registers[0].rgb,vec3(0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate0Registers[13].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate0Registers[13].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate0Registers[0].b),vec3(0))*max(vec3(nativeWeatherPlate0Registers[0].b),vec3(0)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate0Registers[13].a=aab.x;
}
nativeWeatherPlate0Registers[1]=material.constants[16];nativeWeatherPlate0Registers[2]=material.constants[17];
nativeWeatherPlate0Registers[14]=vec4((nativeWeatherPlate0Registers[5].rgb+nativeWeatherPlate0Registers[12].rgb),0);
nativeWeatherPlate0Registers[15]=vec4(max(nativeWeatherPlate0Registers[0].rgb,vec3(0))*max(nativeWeatherPlate0Registers[0].rgb,vec3(0)),0);
return clamp(vec4(max(nativeWeatherPlate0Registers[13].aaa,vec3(0))*max(nativeWeatherPlate0Registers[1].rgb,vec3(0))+(vec3(1)-max(nativeWeatherPlate0Registers[13].aaa,vec3(0)))*max(nativeWeatherPlate0Registers[0].rgb,vec3(0))+max(nativeWeatherPlate0Registers[0].rgb,vec3(0)),max(vec3(nativeWeatherPlate0Registers[13].a),vec3(0)).x),0.0,1.0);}

vec4 nativeWeatherPlate1Registers[16];
vec4 nativeWeatherPlate1Argb(uint c){return vec4((c>>16u)&255u,(c>>8u)&255u,c&255u,c>>24u)/255.0;}
vec4 nativeWeatherPlate1(vec4 textures[4],vec4 v0,vec4 v1){
nativeWeatherPlate1Registers[0]=vec4(0);
nativeWeatherPlate1Registers[1]=vec4(0);
nativeWeatherPlate1Registers[2]=vec4(0);
nativeWeatherPlate1Registers[3]=vec4(0);
nativeWeatherPlate1Registers[4]=vec4(0);
nativeWeatherPlate1Registers[5]=vec4(0);
nativeWeatherPlate1Registers[6]=vec4(0);
nativeWeatherPlate1Registers[7]=vec4(0);
nativeWeatherPlate1Registers[8]=vec4(0);
nativeWeatherPlate1Registers[9]=vec4(0);
nativeWeatherPlate1Registers[10]=vec4(0);
nativeWeatherPlate1Registers[11]=vec4(0);
nativeWeatherPlate1Registers[12]=vec4(0);
nativeWeatherPlate1Registers[13]=vec4(0);
nativeWeatherPlate1Registers[14]=vec4(0);
nativeWeatherPlate1Registers[15]=vec4(0);
#ifdef H2_COMBINER_FOG
nativeWeatherPlate1Registers[3]=H2_COMBINER_FOG();
#endif
nativeWeatherPlate1Registers[4]=v0;nativeWeatherPlate1Registers[5]=v1;
nativeWeatherPlate1Registers[8]=textures[0];
nativeWeatherPlate1Registers[9]=textures[1];
nativeWeatherPlate1Registers[10]=textures[2];
nativeWeatherPlate1Registers[11]=textures[3];
nativeWeatherPlate1Registers[12].a=textures[0].a;
{
nativeWeatherPlate1Registers[1]=material.constants[0];
nativeWeatherPlate1Registers[2]=material.constants[1];
vec3 rrawab=vec3(dot(max(nativeWeatherPlate1Registers[8].rgb,vec3(0)),max(nativeWeatherPlate1Registers[1].rgb,vec3(0)))),rrawcd=vec3(dot(max(nativeWeatherPlate1Registers[8].rgb,vec3(0)),max(nativeWeatherPlate1Registers[2].rgb,vec3(0))));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate1Registers[0].b),vec3(0))*max(vec3(nativeWeatherPlate1Registers[0].b),vec3(0))),arawcd=(max(vec3(nativeWeatherPlate1Registers[0].b),vec3(0))*max(vec3(nativeWeatherPlate1Registers[0].b),vec3(0)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate1Registers[4].rgb=rab;
nativeWeatherPlate1Registers[5].rgb=rcd;
}
{
nativeWeatherPlate1Registers[1]=material.constants[2];
nativeWeatherPlate1Registers[2]=material.constants[3];
vec3 rrawab=(max(nativeWeatherPlate1Registers[4].rgb,vec3(0))*(vec3(1)-clamp(nativeWeatherPlate1Registers[0].rgb,0.0,1.0))),rrawcd=(max(nativeWeatherPlate1Registers[1].aaa,vec3(0))*(2.0*max(nativeWeatherPlate1Registers[0].rgb,vec3(0))-vec3(1)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate1Registers[4].b),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate1Registers[0].b),0.0,1.0))),arawcd=((max(vec3(nativeWeatherPlate1Registers[1].a),vec3(0))-vec3(0.5))*(2.0*max(vec3(nativeWeatherPlate1Registers[0].b),vec3(0))-vec3(1)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate1Registers[12].rgb=rsum;
nativeWeatherPlate1Registers[12].a=asum.x;
}
{
nativeWeatherPlate1Registers[1]=material.constants[4];
nativeWeatherPlate1Registers[2]=material.constants[5];
vec3 rrawab=(max(nativeWeatherPlate1Registers[0].rgb,vec3(0))*max(nativeWeatherPlate1Registers[0].rgb,vec3(0))),rrawcd=((vec3(1)-clamp(nativeWeatherPlate1Registers[0].rgb,0.0,1.0))*(vec3(1)-clamp(nativeWeatherPlate1Registers[0].rgb,0.0,1.0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp(((nativeWeatherPlate1Registers[12].a>=0.5)?rrawcd:rrawab),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate1Registers[8].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate1Registers[0].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate1Registers[1].a),vec3(0))*(2.0*max(vec3(nativeWeatherPlate1Registers[0].b),vec3(0))-vec3(1)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate1Registers[13].rgb=rsum;
nativeWeatherPlate1Registers[13].a=asum.x;
}
{
nativeWeatherPlate1Registers[1]=material.constants[6];
nativeWeatherPlate1Registers[2]=material.constants[7];
vec3 rrawab=(nativeWeatherPlate1Registers[12].rgb*(vec3(1)-clamp(nativeWeatherPlate1Registers[0].rgb,0.0,1.0))),rrawcd=((vec3(1)-clamp(nativeWeatherPlate1Registers[13].rgb,0.0,1.0))*(vec3(1)-clamp(nativeWeatherPlate1Registers[0].rgb,0.0,1.0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(vec3(nativeWeatherPlate1Registers[13].a)*(vec3(1)-clamp(vec3(nativeWeatherPlate1Registers[0].b),0.0,1.0))),arawcd=((vec3(1)-clamp(vec3(nativeWeatherPlate1Registers[13].b),0.0,1.0))*(-vec3(nativeWeatherPlate1Registers[1].a)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate1Registers[12].rgb=rsum;
nativeWeatherPlate1Registers[13].a=asum.x;
}
{
nativeWeatherPlate1Registers[1]=material.constants[8];
nativeWeatherPlate1Registers[2]=material.constants[9];
vec3 rrawab=vec3(dot(max(nativeWeatherPlate1Registers[9].aaa,vec3(0)),max(nativeWeatherPlate1Registers[1].rgb,vec3(0)))),rrawcd=vec3(dot(max(nativeWeatherPlate1Registers[10].aaa,vec3(0)),max(nativeWeatherPlate1Registers[2].rgb,vec3(0))));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate1Registers[13].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate1Registers[0].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate1Registers[13].a),vec3(0))*max(vec3(nativeWeatherPlate1Registers[0].b),vec3(0)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate1Registers[4].rgb=rab;
nativeWeatherPlate1Registers[5].rgb=rcd;
nativeWeatherPlate1Registers[13].a=asum.x;
}
{
nativeWeatherPlate1Registers[1]=material.constants[10];
nativeWeatherPlate1Registers[2]=material.constants[11];
vec3 rrawab=((vec3(1)-clamp(nativeWeatherPlate1Registers[4].rgb,0.0,1.0))*(vec3(1)-clamp(nativeWeatherPlate1Registers[5].rgb,0.0,1.0))),rrawcd=vec3(dot(max(nativeWeatherPlate1Registers[11].aaa,vec3(0)),max(nativeWeatherPlate1Registers[1].rgb,vec3(0))));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate1Registers[13].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate1Registers[0].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate1Registers[13].a),vec3(0))*max(vec3(nativeWeatherPlate1Registers[0].b),vec3(0)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate1Registers[4].rgb=rab;
nativeWeatherPlate1Registers[5].rgb=rcd;
nativeWeatherPlate1Registers[13].a=asum.x;
}
{
nativeWeatherPlate1Registers[1]=material.constants[12];
nativeWeatherPlate1Registers[2]=material.constants[13];
vec3 rrawab=(max(nativeWeatherPlate1Registers[4].rgb,vec3(0))*(vec3(1)-clamp(nativeWeatherPlate1Registers[5].rgb,0.0,1.0))),rrawcd=(max(nativeWeatherPlate1Registers[0].rgb,vec3(0))*max(nativeWeatherPlate1Registers[0].rgb,vec3(0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate1Registers[13].a),vec3(0))*max(vec3(nativeWeatherPlate1Registers[2].a),vec3(0))),arawcd=(max(vec3(nativeWeatherPlate1Registers[12].b),vec3(0))*max(vec3(nativeWeatherPlate1Registers[2].b),vec3(0)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate1Registers[13].rgb=rab;
nativeWeatherPlate1Registers[13].a=asum.x;
}
{
nativeWeatherPlate1Registers[1]=material.constants[14];
nativeWeatherPlate1Registers[2]=material.constants[15];
vec3 rrawab=(max(nativeWeatherPlate1Registers[0].rgb,vec3(0))*max(nativeWeatherPlate1Registers[0].rgb,vec3(0))),rrawcd=(max(nativeWeatherPlate1Registers[0].rgb,vec3(0))*max(nativeWeatherPlate1Registers[0].rgb,vec3(0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate1Registers[13].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate1Registers[13].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate1Registers[0].b),vec3(0))*max(vec3(nativeWeatherPlate1Registers[0].b),vec3(0)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate1Registers[13].a=aab.x;
}
nativeWeatherPlate1Registers[1]=material.constants[16];nativeWeatherPlate1Registers[2]=material.constants[17];
nativeWeatherPlate1Registers[14]=vec4((nativeWeatherPlate1Registers[5].rgb+nativeWeatherPlate1Registers[12].rgb),0);
nativeWeatherPlate1Registers[15]=vec4(max(nativeWeatherPlate1Registers[0].rgb,vec3(0))*max(nativeWeatherPlate1Registers[0].rgb,vec3(0)),0);
return clamp(vec4(max(nativeWeatherPlate1Registers[13].aaa,vec3(0))*max(nativeWeatherPlate1Registers[1].rgb,vec3(0))+(vec3(1)-max(nativeWeatherPlate1Registers[13].aaa,vec3(0)))*max(nativeWeatherPlate1Registers[0].rgb,vec3(0))+max(nativeWeatherPlate1Registers[0].rgb,vec3(0)),max(vec3(nativeWeatherPlate1Registers[13].a),vec3(0)).x),0.0,1.0);}

vec4 nativeWeatherPlate2Registers[16];
vec4 nativeWeatherPlate2Argb(uint c){return vec4((c>>16u)&255u,(c>>8u)&255u,c&255u,c>>24u)/255.0;}
vec4 nativeWeatherPlate2(vec4 textures[4],vec4 v0,vec4 v1){
nativeWeatherPlate2Registers[0]=vec4(0);
nativeWeatherPlate2Registers[1]=vec4(0);
nativeWeatherPlate2Registers[2]=vec4(0);
nativeWeatherPlate2Registers[3]=vec4(0);
nativeWeatherPlate2Registers[4]=vec4(0);
nativeWeatherPlate2Registers[5]=vec4(0);
nativeWeatherPlate2Registers[6]=vec4(0);
nativeWeatherPlate2Registers[7]=vec4(0);
nativeWeatherPlate2Registers[8]=vec4(0);
nativeWeatherPlate2Registers[9]=vec4(0);
nativeWeatherPlate2Registers[10]=vec4(0);
nativeWeatherPlate2Registers[11]=vec4(0);
nativeWeatherPlate2Registers[12]=vec4(0);
nativeWeatherPlate2Registers[13]=vec4(0);
nativeWeatherPlate2Registers[14]=vec4(0);
nativeWeatherPlate2Registers[15]=vec4(0);
#ifdef H2_COMBINER_FOG
nativeWeatherPlate2Registers[3]=H2_COMBINER_FOG();
#endif
nativeWeatherPlate2Registers[4]=v0;nativeWeatherPlate2Registers[5]=v1;
nativeWeatherPlate2Registers[8]=textures[0];
nativeWeatherPlate2Registers[9]=textures[1];
nativeWeatherPlate2Registers[10]=textures[2];
nativeWeatherPlate2Registers[11]=textures[3];
nativeWeatherPlate2Registers[12].a=textures[0].a;
{
nativeWeatherPlate2Registers[1]=material.constants[0];
nativeWeatherPlate2Registers[2]=material.constants[1];
vec3 rrawab=vec3(dot(max(nativeWeatherPlate2Registers[8].rgb,vec3(0)),max(nativeWeatherPlate2Registers[1].rgb,vec3(0)))),rrawcd=vec3(dot(max(nativeWeatherPlate2Registers[8].rgb,vec3(0)),max(nativeWeatherPlate2Registers[2].rgb,vec3(0))));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate2Registers[0].b),vec3(0))*max(vec3(nativeWeatherPlate2Registers[0].b),vec3(0))),arawcd=(max(vec3(nativeWeatherPlate2Registers[0].b),vec3(0))*max(vec3(nativeWeatherPlate2Registers[0].b),vec3(0)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate2Registers[4].rgb=rab;
nativeWeatherPlate2Registers[5].rgb=rcd;
}
{
nativeWeatherPlate2Registers[1]=material.constants[2];
nativeWeatherPlate2Registers[2]=material.constants[3];
vec3 rrawab=(max(nativeWeatherPlate2Registers[4].rgb,vec3(0))*(vec3(1)-clamp(nativeWeatherPlate2Registers[0].rgb,0.0,1.0))),rrawcd=(max(nativeWeatherPlate2Registers[1].aaa,vec3(0))*(2.0*max(nativeWeatherPlate2Registers[0].rgb,vec3(0))-vec3(1)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate2Registers[4].b),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate2Registers[0].b),0.0,1.0))),arawcd=((max(vec3(nativeWeatherPlate2Registers[1].a),vec3(0))-vec3(0.5))*(2.0*max(vec3(nativeWeatherPlate2Registers[0].b),vec3(0))-vec3(1)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate2Registers[12].rgb=rsum;
nativeWeatherPlate2Registers[12].a=asum.x;
}
{
nativeWeatherPlate2Registers[1]=material.constants[4];
nativeWeatherPlate2Registers[2]=material.constants[5];
vec3 rrawab=(max(nativeWeatherPlate2Registers[0].rgb,vec3(0))*max(nativeWeatherPlate2Registers[0].rgb,vec3(0))),rrawcd=((vec3(1)-clamp(nativeWeatherPlate2Registers[0].rgb,0.0,1.0))*(vec3(1)-clamp(nativeWeatherPlate2Registers[0].rgb,0.0,1.0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp(((nativeWeatherPlate2Registers[12].a>=0.5)?rrawcd:rrawab),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate2Registers[8].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate2Registers[0].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate2Registers[1].a),vec3(0))*(2.0*max(vec3(nativeWeatherPlate2Registers[0].b),vec3(0))-vec3(1)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate2Registers[13].rgb=rsum;
nativeWeatherPlate2Registers[13].a=asum.x;
}
{
nativeWeatherPlate2Registers[1]=material.constants[6];
nativeWeatherPlate2Registers[2]=material.constants[7];
vec3 rrawab=(nativeWeatherPlate2Registers[12].rgb*(vec3(1)-clamp(nativeWeatherPlate2Registers[0].rgb,0.0,1.0))),rrawcd=((vec3(1)-clamp(nativeWeatherPlate2Registers[13].rgb,0.0,1.0))*(vec3(1)-clamp(nativeWeatherPlate2Registers[0].rgb,0.0,1.0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(vec3(nativeWeatherPlate2Registers[13].a)*(vec3(1)-clamp(vec3(nativeWeatherPlate2Registers[0].b),0.0,1.0))),arawcd=((vec3(1)-clamp(vec3(nativeWeatherPlate2Registers[13].b),0.0,1.0))*(-vec3(nativeWeatherPlate2Registers[1].a)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate2Registers[12].rgb=rsum;
nativeWeatherPlate2Registers[13].a=asum.x;
}
{
nativeWeatherPlate2Registers[1]=material.constants[8];
nativeWeatherPlate2Registers[2]=material.constants[9];
vec3 rrawab=vec3(dot(max(nativeWeatherPlate2Registers[9].rgb,vec3(0)),max(nativeWeatherPlate2Registers[1].rgb,vec3(0)))),rrawcd=vec3(dot(max(nativeWeatherPlate2Registers[10].rgb,vec3(0)),max(nativeWeatherPlate2Registers[2].rgb,vec3(0))));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate2Registers[13].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate2Registers[0].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate2Registers[13].a),vec3(0))*max(vec3(nativeWeatherPlate2Registers[0].b),vec3(0)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate2Registers[4].rgb=rab;
nativeWeatherPlate2Registers[5].rgb=rcd;
nativeWeatherPlate2Registers[13].a=asum.x;
}
{
nativeWeatherPlate2Registers[1]=material.constants[10];
nativeWeatherPlate2Registers[2]=material.constants[11];
vec3 rrawab=((vec3(1)-clamp(nativeWeatherPlate2Registers[4].rgb,0.0,1.0))*(vec3(1)-clamp(nativeWeatherPlate2Registers[5].rgb,0.0,1.0))),rrawcd=vec3(dot(max(nativeWeatherPlate2Registers[11].rgb,vec3(0)),max(nativeWeatherPlate2Registers[1].rgb,vec3(0))));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate2Registers[13].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate2Registers[0].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate2Registers[13].a),vec3(0))*max(vec3(nativeWeatherPlate2Registers[0].b),vec3(0)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate2Registers[4].rgb=rab;
nativeWeatherPlate2Registers[5].rgb=rcd;
nativeWeatherPlate2Registers[13].a=asum.x;
}
{
nativeWeatherPlate2Registers[1]=material.constants[12];
nativeWeatherPlate2Registers[2]=material.constants[13];
vec3 rrawab=(max(nativeWeatherPlate2Registers[4].rgb,vec3(0))*(vec3(1)-clamp(nativeWeatherPlate2Registers[5].rgb,0.0,1.0))),rrawcd=(max(nativeWeatherPlate2Registers[0].rgb,vec3(0))*max(nativeWeatherPlate2Registers[0].rgb,vec3(0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate2Registers[13].a),vec3(0))*max(vec3(nativeWeatherPlate2Registers[2].a),vec3(0))),arawcd=(max(vec3(nativeWeatherPlate2Registers[12].b),vec3(0))*max(vec3(nativeWeatherPlate2Registers[2].b),vec3(0)));
vec3 aab=clamp((arawab*2.0),-1.0,1.0),acd=clamp((arawcd*2.0),-1.0,1.0),asum=clamp(((arawab+arawcd)*2.0),-1.0,1.0);
nativeWeatherPlate2Registers[13].rgb=rab;
nativeWeatherPlate2Registers[13].a=asum.x;
}
{
nativeWeatherPlate2Registers[1]=material.constants[14];
nativeWeatherPlate2Registers[2]=material.constants[15];
vec3 rrawab=(max(nativeWeatherPlate2Registers[0].rgb,vec3(0))*max(nativeWeatherPlate2Registers[0].rgb,vec3(0))),rrawcd=(max(nativeWeatherPlate2Registers[0].rgb,vec3(0))*max(nativeWeatherPlate2Registers[0].rgb,vec3(0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate2Registers[13].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate2Registers[13].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate2Registers[0].b),vec3(0))*max(vec3(nativeWeatherPlate2Registers[0].b),vec3(0)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate2Registers[13].a=aab.x;
}
nativeWeatherPlate2Registers[1]=material.constants[16];nativeWeatherPlate2Registers[2]=material.constants[17];
nativeWeatherPlate2Registers[14]=vec4((nativeWeatherPlate2Registers[5].rgb+nativeWeatherPlate2Registers[12].rgb),0);
nativeWeatherPlate2Registers[15]=vec4(max(nativeWeatherPlate2Registers[0].rgb,vec3(0))*max(nativeWeatherPlate2Registers[0].rgb,vec3(0)),0);
return clamp(vec4(max(nativeWeatherPlate2Registers[13].aaa,vec3(0))*max(nativeWeatherPlate2Registers[1].rgb,vec3(0))+(vec3(1)-max(nativeWeatherPlate2Registers[13].aaa,vec3(0)))*max(nativeWeatherPlate2Registers[0].rgb,vec3(0))+max(nativeWeatherPlate2Registers[0].rgb,vec3(0)),max(vec3(nativeWeatherPlate2Registers[13].a),vec3(0)).x),0.0,1.0);}

vec4 nativeWeatherPlate3Registers[16];
vec4 nativeWeatherPlate3Argb(uint c){return vec4((c>>16u)&255u,(c>>8u)&255u,c&255u,c>>24u)/255.0;}
vec4 nativeWeatherPlate3(vec4 textures[4],vec4 v0,vec4 v1){
nativeWeatherPlate3Registers[0]=vec4(0);
nativeWeatherPlate3Registers[1]=vec4(0);
nativeWeatherPlate3Registers[2]=vec4(0);
nativeWeatherPlate3Registers[3]=vec4(0);
nativeWeatherPlate3Registers[4]=vec4(0);
nativeWeatherPlate3Registers[5]=vec4(0);
nativeWeatherPlate3Registers[6]=vec4(0);
nativeWeatherPlate3Registers[7]=vec4(0);
nativeWeatherPlate3Registers[8]=vec4(0);
nativeWeatherPlate3Registers[9]=vec4(0);
nativeWeatherPlate3Registers[10]=vec4(0);
nativeWeatherPlate3Registers[11]=vec4(0);
nativeWeatherPlate3Registers[12]=vec4(0);
nativeWeatherPlate3Registers[13]=vec4(0);
nativeWeatherPlate3Registers[14]=vec4(0);
nativeWeatherPlate3Registers[15]=vec4(0);
#ifdef H2_COMBINER_FOG
nativeWeatherPlate3Registers[3]=H2_COMBINER_FOG();
#endif
nativeWeatherPlate3Registers[4]=v0;nativeWeatherPlate3Registers[5]=v1;
nativeWeatherPlate3Registers[8]=textures[0];
nativeWeatherPlate3Registers[9]=textures[1];
nativeWeatherPlate3Registers[10]=textures[2];
nativeWeatherPlate3Registers[11]=textures[3];
nativeWeatherPlate3Registers[12].a=textures[0].a;
{
nativeWeatherPlate3Registers[1]=material.constants[0];
nativeWeatherPlate3Registers[2]=material.constants[1];
vec3 rrawab=vec3(dot(max(nativeWeatherPlate3Registers[8].rgb,vec3(0)),max(nativeWeatherPlate3Registers[1].rgb,vec3(0)))),rrawcd=vec3(dot(max(nativeWeatherPlate3Registers[8].rgb,vec3(0)),max(nativeWeatherPlate3Registers[2].rgb,vec3(0))));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate3Registers[0].b),vec3(0))*max(vec3(nativeWeatherPlate3Registers[0].b),vec3(0))),arawcd=(max(vec3(nativeWeatherPlate3Registers[0].b),vec3(0))*max(vec3(nativeWeatherPlate3Registers[0].b),vec3(0)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate3Registers[4].rgb=rab;
nativeWeatherPlate3Registers[5].rgb=rcd;
}
{
nativeWeatherPlate3Registers[1]=material.constants[2];
nativeWeatherPlate3Registers[2]=material.constants[3];
vec3 rrawab=(max(nativeWeatherPlate3Registers[4].rgb,vec3(0))*(vec3(1)-clamp(nativeWeatherPlate3Registers[0].rgb,0.0,1.0))),rrawcd=(max(nativeWeatherPlate3Registers[1].aaa,vec3(0))*(2.0*max(nativeWeatherPlate3Registers[0].rgb,vec3(0))-vec3(1)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate3Registers[4].b),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate3Registers[0].b),0.0,1.0))),arawcd=((max(vec3(nativeWeatherPlate3Registers[1].a),vec3(0))-vec3(0.5))*(2.0*max(vec3(nativeWeatherPlate3Registers[0].b),vec3(0))-vec3(1)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate3Registers[12].rgb=rsum;
nativeWeatherPlate3Registers[12].a=asum.x;
}
{
nativeWeatherPlate3Registers[1]=material.constants[4];
nativeWeatherPlate3Registers[2]=material.constants[5];
vec3 rrawab=(max(nativeWeatherPlate3Registers[0].rgb,vec3(0))*max(nativeWeatherPlate3Registers[0].rgb,vec3(0))),rrawcd=((vec3(1)-clamp(nativeWeatherPlate3Registers[0].rgb,0.0,1.0))*(vec3(1)-clamp(nativeWeatherPlate3Registers[0].rgb,0.0,1.0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp(((nativeWeatherPlate3Registers[12].a>=0.5)?rrawcd:rrawab),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate3Registers[8].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate3Registers[0].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate3Registers[1].a),vec3(0))*(2.0*max(vec3(nativeWeatherPlate3Registers[0].b),vec3(0))-vec3(1)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate3Registers[13].rgb=rsum;
nativeWeatherPlate3Registers[13].a=asum.x;
}
{
nativeWeatherPlate3Registers[1]=material.constants[6];
nativeWeatherPlate3Registers[2]=material.constants[7];
vec3 rrawab=(nativeWeatherPlate3Registers[12].rgb*(vec3(1)-clamp(nativeWeatherPlate3Registers[0].rgb,0.0,1.0))),rrawcd=((vec3(1)-clamp(nativeWeatherPlate3Registers[13].rgb,0.0,1.0))*(vec3(1)-clamp(nativeWeatherPlate3Registers[0].rgb,0.0,1.0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(vec3(nativeWeatherPlate3Registers[13].a)*(vec3(1)-clamp(vec3(nativeWeatherPlate3Registers[0].b),0.0,1.0))),arawcd=((vec3(1)-clamp(vec3(nativeWeatherPlate3Registers[13].b),0.0,1.0))*(-vec3(nativeWeatherPlate3Registers[1].a)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate3Registers[12].rgb=rsum;
nativeWeatherPlate3Registers[13].a=asum.x;
}
{
nativeWeatherPlate3Registers[1]=material.constants[8];
nativeWeatherPlate3Registers[2]=material.constants[9];
vec3 rrawab=vec3(dot(max(nativeWeatherPlate3Registers[9].aaa,vec3(0)),max(nativeWeatherPlate3Registers[1].rgb,vec3(0)))),rrawcd=vec3(dot(max(nativeWeatherPlate3Registers[10].aaa,vec3(0)),max(nativeWeatherPlate3Registers[2].rgb,vec3(0))));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate3Registers[13].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate3Registers[0].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate3Registers[13].a),vec3(0))*max(vec3(nativeWeatherPlate3Registers[0].b),vec3(0)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate3Registers[4].rgb=rab;
nativeWeatherPlate3Registers[5].rgb=rcd;
nativeWeatherPlate3Registers[13].a=asum.x;
}
{
nativeWeatherPlate3Registers[1]=material.constants[10];
nativeWeatherPlate3Registers[2]=material.constants[11];
vec3 rrawab=((vec3(1)-clamp(nativeWeatherPlate3Registers[4].rgb,0.0,1.0))*(vec3(1)-clamp(nativeWeatherPlate3Registers[5].rgb,0.0,1.0))),rrawcd=vec3(dot(max(nativeWeatherPlate3Registers[11].aaa,vec3(0)),max(nativeWeatherPlate3Registers[1].rgb,vec3(0))));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate3Registers[13].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate3Registers[0].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate3Registers[13].a),vec3(0))*max(vec3(nativeWeatherPlate3Registers[0].b),vec3(0)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate3Registers[4].rgb=rab;
nativeWeatherPlate3Registers[5].rgb=rcd;
nativeWeatherPlate3Registers[13].a=asum.x;
}
{
nativeWeatherPlate3Registers[1]=material.constants[12];
nativeWeatherPlate3Registers[2]=material.constants[13];
vec3 rrawab=(max(nativeWeatherPlate3Registers[4].rgb,vec3(0))*(vec3(1)-clamp(nativeWeatherPlate3Registers[5].rgb,0.0,1.0))),rrawcd=(max(nativeWeatherPlate3Registers[0].rgb,vec3(0))*max(nativeWeatherPlate3Registers[0].rgb,vec3(0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate3Registers[13].a),vec3(0))*max(vec3(nativeWeatherPlate3Registers[2].a),vec3(0))),arawcd=(max(vec3(nativeWeatherPlate3Registers[12].b),vec3(0))*max(vec3(nativeWeatherPlate3Registers[2].b),vec3(0)));
vec3 aab=clamp((arawab*2.0),-1.0,1.0),acd=clamp((arawcd*2.0),-1.0,1.0),asum=clamp(((arawab+arawcd)*2.0),-1.0,1.0);
nativeWeatherPlate3Registers[13].rgb=rab;
nativeWeatherPlate3Registers[13].a=asum.x;
}
{
nativeWeatherPlate3Registers[1]=material.constants[14];
nativeWeatherPlate3Registers[2]=material.constants[15];
vec3 rrawab=(max(nativeWeatherPlate3Registers[0].rgb,vec3(0))*max(nativeWeatherPlate3Registers[0].rgb,vec3(0))),rrawcd=(max(nativeWeatherPlate3Registers[0].rgb,vec3(0))*max(nativeWeatherPlate3Registers[0].rgb,vec3(0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate3Registers[13].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate3Registers[13].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate3Registers[0].b),vec3(0))*max(vec3(nativeWeatherPlate3Registers[0].b),vec3(0)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate3Registers[13].a=aab.x;
}
nativeWeatherPlate3Registers[1]=material.constants[16];nativeWeatherPlate3Registers[2]=material.constants[17];
nativeWeatherPlate3Registers[14]=vec4((nativeWeatherPlate3Registers[5].rgb+nativeWeatherPlate3Registers[12].rgb),0);
nativeWeatherPlate3Registers[15]=vec4(max(nativeWeatherPlate3Registers[0].rgb,vec3(0))*max(nativeWeatherPlate3Registers[0].rgb,vec3(0)),0);
return clamp(vec4(max(nativeWeatherPlate3Registers[13].aaa,vec3(0))*max(nativeWeatherPlate3Registers[1].rgb,vec3(0))+(vec3(1)-max(nativeWeatherPlate3Registers[13].aaa,vec3(0)))*max(nativeWeatherPlate3Registers[0].rgb,vec3(0))+max(nativeWeatherPlate3Registers[0].rgb,vec3(0)),max(vec3(nativeWeatherPlate3Registers[13].a),vec3(0)).x),0.0,1.0);}

vec4 nativeWeatherPlate4Registers[16];
vec4 nativeWeatherPlate4Argb(uint c){return vec4((c>>16u)&255u,(c>>8u)&255u,c&255u,c>>24u)/255.0;}
vec4 nativeWeatherPlate4(vec4 textures[4],vec4 v0,vec4 v1){
nativeWeatherPlate4Registers[0]=vec4(0);
nativeWeatherPlate4Registers[1]=vec4(0);
nativeWeatherPlate4Registers[2]=vec4(0);
nativeWeatherPlate4Registers[3]=vec4(0);
nativeWeatherPlate4Registers[4]=vec4(0);
nativeWeatherPlate4Registers[5]=vec4(0);
nativeWeatherPlate4Registers[6]=vec4(0);
nativeWeatherPlate4Registers[7]=vec4(0);
nativeWeatherPlate4Registers[8]=vec4(0);
nativeWeatherPlate4Registers[9]=vec4(0);
nativeWeatherPlate4Registers[10]=vec4(0);
nativeWeatherPlate4Registers[11]=vec4(0);
nativeWeatherPlate4Registers[12]=vec4(0);
nativeWeatherPlate4Registers[13]=vec4(0);
nativeWeatherPlate4Registers[14]=vec4(0);
nativeWeatherPlate4Registers[15]=vec4(0);
#ifdef H2_COMBINER_FOG
nativeWeatherPlate4Registers[3]=H2_COMBINER_FOG();
#endif
nativeWeatherPlate4Registers[4]=v0;nativeWeatherPlate4Registers[5]=v1;
nativeWeatherPlate4Registers[8]=textures[0];
nativeWeatherPlate4Registers[9]=textures[1];
nativeWeatherPlate4Registers[10]=textures[2];
nativeWeatherPlate4Registers[11]=textures[3];
nativeWeatherPlate4Registers[12].a=textures[0].a;
{
nativeWeatherPlate4Registers[1]=material.constants[0];
nativeWeatherPlate4Registers[2]=material.constants[1];
vec3 rrawab=vec3(dot(max(nativeWeatherPlate4Registers[8].rgb,vec3(0)),max(nativeWeatherPlate4Registers[1].rgb,vec3(0)))),rrawcd=vec3(dot(max(nativeWeatherPlate4Registers[8].rgb,vec3(0)),max(nativeWeatherPlate4Registers[2].rgb,vec3(0))));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate4Registers[0].b),vec3(0))*max(vec3(nativeWeatherPlate4Registers[0].b),vec3(0))),arawcd=(max(vec3(nativeWeatherPlate4Registers[0].b),vec3(0))*max(vec3(nativeWeatherPlate4Registers[0].b),vec3(0)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate4Registers[4].rgb=rab;
nativeWeatherPlate4Registers[5].rgb=rcd;
}
{
nativeWeatherPlate4Registers[1]=material.constants[2];
nativeWeatherPlate4Registers[2]=material.constants[3];
vec3 rrawab=(max(nativeWeatherPlate4Registers[4].rgb,vec3(0))*(vec3(1)-clamp(nativeWeatherPlate4Registers[0].rgb,0.0,1.0))),rrawcd=(max(nativeWeatherPlate4Registers[1].aaa,vec3(0))*(2.0*max(nativeWeatherPlate4Registers[0].rgb,vec3(0))-vec3(1)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate4Registers[4].b),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate4Registers[0].b),0.0,1.0))),arawcd=((max(vec3(nativeWeatherPlate4Registers[1].a),vec3(0))-vec3(0.5))*(2.0*max(vec3(nativeWeatherPlate4Registers[0].b),vec3(0))-vec3(1)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate4Registers[12].rgb=rsum;
nativeWeatherPlate4Registers[12].a=asum.x;
}
{
nativeWeatherPlate4Registers[1]=material.constants[4];
nativeWeatherPlate4Registers[2]=material.constants[5];
vec3 rrawab=(max(nativeWeatherPlate4Registers[0].rgb,vec3(0))*max(nativeWeatherPlate4Registers[0].rgb,vec3(0))),rrawcd=((vec3(1)-clamp(nativeWeatherPlate4Registers[0].rgb,0.0,1.0))*(vec3(1)-clamp(nativeWeatherPlate4Registers[0].rgb,0.0,1.0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp(((nativeWeatherPlate4Registers[12].a>=0.5)?rrawcd:rrawab),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate4Registers[8].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate4Registers[0].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate4Registers[1].a),vec3(0))*(2.0*max(vec3(nativeWeatherPlate4Registers[0].b),vec3(0))-vec3(1)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate4Registers[13].rgb=rsum;
nativeWeatherPlate4Registers[13].a=asum.x;
}
{
nativeWeatherPlate4Registers[1]=material.constants[6];
nativeWeatherPlate4Registers[2]=material.constants[7];
vec3 rrawab=(nativeWeatherPlate4Registers[12].rgb*(vec3(1)-clamp(nativeWeatherPlate4Registers[0].rgb,0.0,1.0))),rrawcd=((vec3(1)-clamp(nativeWeatherPlate4Registers[13].rgb,0.0,1.0))*(vec3(1)-clamp(nativeWeatherPlate4Registers[0].rgb,0.0,1.0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(vec3(nativeWeatherPlate4Registers[13].a)*(vec3(1)-clamp(vec3(nativeWeatherPlate4Registers[0].b),0.0,1.0))),arawcd=((vec3(1)-clamp(vec3(nativeWeatherPlate4Registers[13].b),0.0,1.0))*(-vec3(nativeWeatherPlate4Registers[1].a)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate4Registers[12].rgb=rsum;
nativeWeatherPlate4Registers[13].a=asum.x;
}
{
nativeWeatherPlate4Registers[1]=material.constants[8];
nativeWeatherPlate4Registers[2]=material.constants[9];
vec3 rrawab=vec3(dot(max(nativeWeatherPlate4Registers[9].rgb,vec3(0)),max(nativeWeatherPlate4Registers[1].rgb,vec3(0)))),rrawcd=vec3(dot(max(nativeWeatherPlate4Registers[10].rgb,vec3(0)),max(nativeWeatherPlate4Registers[2].rgb,vec3(0))));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate4Registers[13].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate4Registers[0].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate4Registers[13].a),vec3(0))*max(vec3(nativeWeatherPlate4Registers[0].b),vec3(0)));
vec3 aab=clamp((arawab*2.0),-1.0,1.0),acd=clamp((arawcd*2.0),-1.0,1.0),asum=clamp(((arawab+arawcd)*2.0),-1.0,1.0);
nativeWeatherPlate4Registers[4].rgb=rab;
nativeWeatherPlate4Registers[5].rgb=rcd;
nativeWeatherPlate4Registers[13].a=asum.x;
}
{
nativeWeatherPlate4Registers[1]=material.constants[10];
nativeWeatherPlate4Registers[2]=material.constants[11];
vec3 rrawab=((vec3(1)-clamp(nativeWeatherPlate4Registers[4].rgb,0.0,1.0))*(vec3(1)-clamp(nativeWeatherPlate4Registers[5].rgb,0.0,1.0))),rrawcd=vec3(dot(max(nativeWeatherPlate4Registers[11].rgb,vec3(0)),max(nativeWeatherPlate4Registers[1].rgb,vec3(0))));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate4Registers[13].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate4Registers[0].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate4Registers[13].a),vec3(0))*max(vec3(nativeWeatherPlate4Registers[0].b),vec3(0)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate4Registers[4].rgb=rab;
nativeWeatherPlate4Registers[5].rgb=rcd;
nativeWeatherPlate4Registers[13].a=asum.x;
}
{
nativeWeatherPlate4Registers[1]=material.constants[12];
nativeWeatherPlate4Registers[2]=material.constants[13];
vec3 rrawab=(max(nativeWeatherPlate4Registers[4].rgb,vec3(0))*(vec3(1)-clamp(nativeWeatherPlate4Registers[5].rgb,0.0,1.0))),rrawcd=(max(nativeWeatherPlate4Registers[0].rgb,vec3(0))*max(nativeWeatherPlate4Registers[0].rgb,vec3(0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate4Registers[13].a),vec3(0))*max(vec3(nativeWeatherPlate4Registers[2].a),vec3(0))),arawcd=(max(vec3(nativeWeatherPlate4Registers[12].b),vec3(0))*max(vec3(nativeWeatherPlate4Registers[2].b),vec3(0)));
vec3 aab=clamp((arawab*2.0),-1.0,1.0),acd=clamp((arawcd*2.0),-1.0,1.0),asum=clamp(((arawab+arawcd)*2.0),-1.0,1.0);
nativeWeatherPlate4Registers[13].rgb=rab;
nativeWeatherPlate4Registers[13].a=asum.x;
}
{
nativeWeatherPlate4Registers[1]=material.constants[14];
nativeWeatherPlate4Registers[2]=material.constants[15];
vec3 rrawab=(max(nativeWeatherPlate4Registers[0].rgb,vec3(0))*max(nativeWeatherPlate4Registers[0].rgb,vec3(0))),rrawcd=(max(nativeWeatherPlate4Registers[0].rgb,vec3(0))*max(nativeWeatherPlate4Registers[0].rgb,vec3(0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate4Registers[13].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate4Registers[13].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate4Registers[0].b),vec3(0))*max(vec3(nativeWeatherPlate4Registers[0].b),vec3(0)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate4Registers[13].a=aab.x;
}
nativeWeatherPlate4Registers[1]=material.constants[16];nativeWeatherPlate4Registers[2]=material.constants[17];
nativeWeatherPlate4Registers[14]=vec4((nativeWeatherPlate4Registers[5].rgb+nativeWeatherPlate4Registers[12].rgb),0);
nativeWeatherPlate4Registers[15]=vec4(max(nativeWeatherPlate4Registers[0].rgb,vec3(0))*max(nativeWeatherPlate4Registers[0].rgb,vec3(0)),0);
return clamp(vec4(max(nativeWeatherPlate4Registers[13].aaa,vec3(0))*max(nativeWeatherPlate4Registers[1].rgb,vec3(0))+(vec3(1)-max(nativeWeatherPlate4Registers[13].aaa,vec3(0)))*max(nativeWeatherPlate4Registers[0].rgb,vec3(0))+max(nativeWeatherPlate4Registers[0].rgb,vec3(0)),max(vec3(nativeWeatherPlate4Registers[13].a),vec3(0)).x),0.0,1.0);}

vec4 nativeWeatherPlate5Registers[16];
vec4 nativeWeatherPlate5Argb(uint c){return vec4((c>>16u)&255u,(c>>8u)&255u,c&255u,c>>24u)/255.0;}
vec4 nativeWeatherPlate5(vec4 textures[4],vec4 v0,vec4 v1){
nativeWeatherPlate5Registers[0]=vec4(0);
nativeWeatherPlate5Registers[1]=vec4(0);
nativeWeatherPlate5Registers[2]=vec4(0);
nativeWeatherPlate5Registers[3]=vec4(0);
nativeWeatherPlate5Registers[4]=vec4(0);
nativeWeatherPlate5Registers[5]=vec4(0);
nativeWeatherPlate5Registers[6]=vec4(0);
nativeWeatherPlate5Registers[7]=vec4(0);
nativeWeatherPlate5Registers[8]=vec4(0);
nativeWeatherPlate5Registers[9]=vec4(0);
nativeWeatherPlate5Registers[10]=vec4(0);
nativeWeatherPlate5Registers[11]=vec4(0);
nativeWeatherPlate5Registers[12]=vec4(0);
nativeWeatherPlate5Registers[13]=vec4(0);
nativeWeatherPlate5Registers[14]=vec4(0);
nativeWeatherPlate5Registers[15]=vec4(0);
#ifdef H2_COMBINER_FOG
nativeWeatherPlate5Registers[3]=H2_COMBINER_FOG();
#endif
nativeWeatherPlate5Registers[4]=v0;nativeWeatherPlate5Registers[5]=v1;
nativeWeatherPlate5Registers[8]=textures[0];
nativeWeatherPlate5Registers[9]=textures[1];
nativeWeatherPlate5Registers[10]=textures[2];
nativeWeatherPlate5Registers[11]=textures[3];
nativeWeatherPlate5Registers[12].a=textures[0].a;
{
nativeWeatherPlate5Registers[1]=material.constants[0];
nativeWeatherPlate5Registers[2]=material.constants[1];
vec3 rrawab=vec3(dot(max(nativeWeatherPlate5Registers[8].rgb,vec3(0)),max(nativeWeatherPlate5Registers[1].rgb,vec3(0)))),rrawcd=vec3(dot(max(nativeWeatherPlate5Registers[8].rgb,vec3(0)),max(nativeWeatherPlate5Registers[2].rgb,vec3(0))));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate5Registers[0].b),vec3(0))*max(vec3(nativeWeatherPlate5Registers[0].b),vec3(0))),arawcd=(max(vec3(nativeWeatherPlate5Registers[0].b),vec3(0))*max(vec3(nativeWeatherPlate5Registers[0].b),vec3(0)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate5Registers[4].rgb=rab;
nativeWeatherPlate5Registers[5].rgb=rcd;
}
{
nativeWeatherPlate5Registers[1]=material.constants[2];
nativeWeatherPlate5Registers[2]=material.constants[3];
vec3 rrawab=(max(nativeWeatherPlate5Registers[4].rgb,vec3(0))*(vec3(1)-clamp(nativeWeatherPlate5Registers[0].rgb,0.0,1.0))),rrawcd=(max(nativeWeatherPlate5Registers[1].aaa,vec3(0))*(2.0*max(nativeWeatherPlate5Registers[0].rgb,vec3(0))-vec3(1)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate5Registers[4].b),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate5Registers[0].b),0.0,1.0))),arawcd=((max(vec3(nativeWeatherPlate5Registers[1].a),vec3(0))-vec3(0.5))*(2.0*max(vec3(nativeWeatherPlate5Registers[0].b),vec3(0))-vec3(1)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate5Registers[12].rgb=rsum;
nativeWeatherPlate5Registers[12].a=asum.x;
}
{
nativeWeatherPlate5Registers[1]=material.constants[4];
nativeWeatherPlate5Registers[2]=material.constants[5];
vec3 rrawab=(max(nativeWeatherPlate5Registers[0].rgb,vec3(0))*max(nativeWeatherPlate5Registers[0].rgb,vec3(0))),rrawcd=((vec3(1)-clamp(nativeWeatherPlate5Registers[0].rgb,0.0,1.0))*(vec3(1)-clamp(nativeWeatherPlate5Registers[0].rgb,0.0,1.0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp(((nativeWeatherPlate5Registers[12].a>=0.5)?rrawcd:rrawab),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate5Registers[8].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate5Registers[0].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate5Registers[1].a),vec3(0))*(2.0*max(vec3(nativeWeatherPlate5Registers[0].b),vec3(0))-vec3(1)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate5Registers[13].rgb=rsum;
nativeWeatherPlate5Registers[13].a=asum.x;
}
{
nativeWeatherPlate5Registers[1]=material.constants[6];
nativeWeatherPlate5Registers[2]=material.constants[7];
vec3 rrawab=(nativeWeatherPlate5Registers[12].rgb*(vec3(1)-clamp(nativeWeatherPlate5Registers[0].rgb,0.0,1.0))),rrawcd=((vec3(1)-clamp(nativeWeatherPlate5Registers[13].rgb,0.0,1.0))*(vec3(1)-clamp(nativeWeatherPlate5Registers[0].rgb,0.0,1.0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(vec3(nativeWeatherPlate5Registers[13].a)*(vec3(1)-clamp(vec3(nativeWeatherPlate5Registers[0].b),0.0,1.0))),arawcd=((vec3(1)-clamp(vec3(nativeWeatherPlate5Registers[13].b),0.0,1.0))*(-vec3(nativeWeatherPlate5Registers[1].a)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate5Registers[12].rgb=rsum;
nativeWeatherPlate5Registers[13].a=asum.x;
}
{
nativeWeatherPlate5Registers[1]=material.constants[8];
nativeWeatherPlate5Registers[2]=material.constants[9];
vec3 rrawab=vec3(dot(max(nativeWeatherPlate5Registers[9].aaa,vec3(0)),max(nativeWeatherPlate5Registers[1].rgb,vec3(0)))),rrawcd=vec3(dot(max(nativeWeatherPlate5Registers[10].aaa,vec3(0)),max(nativeWeatherPlate5Registers[2].rgb,vec3(0))));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate5Registers[13].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate5Registers[0].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate5Registers[13].a),vec3(0))*max(vec3(nativeWeatherPlate5Registers[0].b),vec3(0)));
vec3 aab=clamp((arawab*2.0),-1.0,1.0),acd=clamp((arawcd*2.0),-1.0,1.0),asum=clamp(((arawab+arawcd)*2.0),-1.0,1.0);
nativeWeatherPlate5Registers[4].rgb=rab;
nativeWeatherPlate5Registers[5].rgb=rcd;
nativeWeatherPlate5Registers[13].a=asum.x;
}
{
nativeWeatherPlate5Registers[1]=material.constants[10];
nativeWeatherPlate5Registers[2]=material.constants[11];
vec3 rrawab=((vec3(1)-clamp(nativeWeatherPlate5Registers[4].rgb,0.0,1.0))*(vec3(1)-clamp(nativeWeatherPlate5Registers[5].rgb,0.0,1.0))),rrawcd=vec3(dot(max(nativeWeatherPlate5Registers[11].aaa,vec3(0)),max(nativeWeatherPlate5Registers[1].rgb,vec3(0))));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate5Registers[13].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate5Registers[0].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate5Registers[13].a),vec3(0))*max(vec3(nativeWeatherPlate5Registers[0].b),vec3(0)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate5Registers[4].rgb=rab;
nativeWeatherPlate5Registers[5].rgb=rcd;
nativeWeatherPlate5Registers[13].a=asum.x;
}
{
nativeWeatherPlate5Registers[1]=material.constants[12];
nativeWeatherPlate5Registers[2]=material.constants[13];
vec3 rrawab=(max(nativeWeatherPlate5Registers[4].rgb,vec3(0))*(vec3(1)-clamp(nativeWeatherPlate5Registers[5].rgb,0.0,1.0))),rrawcd=(max(nativeWeatherPlate5Registers[0].rgb,vec3(0))*max(nativeWeatherPlate5Registers[0].rgb,vec3(0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate5Registers[13].a),vec3(0))*max(vec3(nativeWeatherPlate5Registers[2].a),vec3(0))),arawcd=(max(vec3(nativeWeatherPlate5Registers[12].b),vec3(0))*max(vec3(nativeWeatherPlate5Registers[2].b),vec3(0)));
vec3 aab=clamp((arawab*2.0),-1.0,1.0),acd=clamp((arawcd*2.0),-1.0,1.0),asum=clamp(((arawab+arawcd)*2.0),-1.0,1.0);
nativeWeatherPlate5Registers[13].rgb=rab;
nativeWeatherPlate5Registers[13].a=asum.x;
}
{
nativeWeatherPlate5Registers[1]=material.constants[14];
nativeWeatherPlate5Registers[2]=material.constants[15];
vec3 rrawab=(max(nativeWeatherPlate5Registers[0].rgb,vec3(0))*max(nativeWeatherPlate5Registers[0].rgb,vec3(0))),rrawcd=(max(nativeWeatherPlate5Registers[0].rgb,vec3(0))*max(nativeWeatherPlate5Registers[0].rgb,vec3(0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate5Registers[13].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate5Registers[13].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate5Registers[0].b),vec3(0))*max(vec3(nativeWeatherPlate5Registers[0].b),vec3(0)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate5Registers[13].a=aab.x;
}
nativeWeatherPlate5Registers[1]=material.constants[16];nativeWeatherPlate5Registers[2]=material.constants[17];
nativeWeatherPlate5Registers[14]=vec4((nativeWeatherPlate5Registers[5].rgb+nativeWeatherPlate5Registers[12].rgb),0);
nativeWeatherPlate5Registers[15]=vec4(max(nativeWeatherPlate5Registers[0].rgb,vec3(0))*max(nativeWeatherPlate5Registers[0].rgb,vec3(0)),0);
return clamp(vec4(max(nativeWeatherPlate5Registers[13].aaa,vec3(0))*max(nativeWeatherPlate5Registers[1].rgb,vec3(0))+(vec3(1)-max(nativeWeatherPlate5Registers[13].aaa,vec3(0)))*max(nativeWeatherPlate5Registers[0].rgb,vec3(0))+max(nativeWeatherPlate5Registers[0].rgb,vec3(0)),max(vec3(nativeWeatherPlate5Registers[13].a),vec3(0)).x),0.0,1.0);}

vec4 nativeWeatherPlate6Registers[16];
vec4 nativeWeatherPlate6Argb(uint c){return vec4((c>>16u)&255u,(c>>8u)&255u,c&255u,c>>24u)/255.0;}
vec4 nativeWeatherPlate6(vec4 textures[4],vec4 v0,vec4 v1){
nativeWeatherPlate6Registers[0]=vec4(0);
nativeWeatherPlate6Registers[1]=vec4(0);
nativeWeatherPlate6Registers[2]=vec4(0);
nativeWeatherPlate6Registers[3]=vec4(0);
nativeWeatherPlate6Registers[4]=vec4(0);
nativeWeatherPlate6Registers[5]=vec4(0);
nativeWeatherPlate6Registers[6]=vec4(0);
nativeWeatherPlate6Registers[7]=vec4(0);
nativeWeatherPlate6Registers[8]=vec4(0);
nativeWeatherPlate6Registers[9]=vec4(0);
nativeWeatherPlate6Registers[10]=vec4(0);
nativeWeatherPlate6Registers[11]=vec4(0);
nativeWeatherPlate6Registers[12]=vec4(0);
nativeWeatherPlate6Registers[13]=vec4(0);
nativeWeatherPlate6Registers[14]=vec4(0);
nativeWeatherPlate6Registers[15]=vec4(0);
#ifdef H2_COMBINER_FOG
nativeWeatherPlate6Registers[3]=H2_COMBINER_FOG();
#endif
nativeWeatherPlate6Registers[4]=v0;nativeWeatherPlate6Registers[5]=v1;
nativeWeatherPlate6Registers[8]=textures[0];
nativeWeatherPlate6Registers[9]=textures[1];
nativeWeatherPlate6Registers[10]=textures[2];
nativeWeatherPlate6Registers[11]=textures[3];
nativeWeatherPlate6Registers[12].a=textures[0].a;
{
nativeWeatherPlate6Registers[1]=material.constants[0];
nativeWeatherPlate6Registers[2]=material.constants[1];
vec3 rrawab=vec3(dot(max(nativeWeatherPlate6Registers[8].rgb,vec3(0)),max(nativeWeatherPlate6Registers[1].rgb,vec3(0)))),rrawcd=vec3(dot(max(nativeWeatherPlate6Registers[8].rgb,vec3(0)),max(nativeWeatherPlate6Registers[2].rgb,vec3(0))));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate6Registers[0].b),vec3(0))*max(vec3(nativeWeatherPlate6Registers[0].b),vec3(0))),arawcd=(max(vec3(nativeWeatherPlate6Registers[0].b),vec3(0))*max(vec3(nativeWeatherPlate6Registers[0].b),vec3(0)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate6Registers[4].rgb=rab;
nativeWeatherPlate6Registers[5].rgb=rcd;
}
{
nativeWeatherPlate6Registers[1]=material.constants[2];
nativeWeatherPlate6Registers[2]=material.constants[3];
vec3 rrawab=(max(nativeWeatherPlate6Registers[4].rgb,vec3(0))*(vec3(1)-clamp(nativeWeatherPlate6Registers[0].rgb,0.0,1.0))),rrawcd=(max(nativeWeatherPlate6Registers[1].aaa,vec3(0))*(2.0*max(nativeWeatherPlate6Registers[0].rgb,vec3(0))-vec3(1)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate6Registers[4].b),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate6Registers[0].b),0.0,1.0))),arawcd=((max(vec3(nativeWeatherPlate6Registers[1].a),vec3(0))-vec3(0.5))*(2.0*max(vec3(nativeWeatherPlate6Registers[0].b),vec3(0))-vec3(1)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate6Registers[12].rgb=rsum;
nativeWeatherPlate6Registers[12].a=asum.x;
}
{
nativeWeatherPlate6Registers[1]=material.constants[4];
nativeWeatherPlate6Registers[2]=material.constants[5];
vec3 rrawab=(max(nativeWeatherPlate6Registers[0].rgb,vec3(0))*max(nativeWeatherPlate6Registers[0].rgb,vec3(0))),rrawcd=((vec3(1)-clamp(nativeWeatherPlate6Registers[0].rgb,0.0,1.0))*(vec3(1)-clamp(nativeWeatherPlate6Registers[0].rgb,0.0,1.0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp(((nativeWeatherPlate6Registers[12].a>=0.5)?rrawcd:rrawab),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate6Registers[8].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate6Registers[0].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate6Registers[1].a),vec3(0))*(2.0*max(vec3(nativeWeatherPlate6Registers[0].b),vec3(0))-vec3(1)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate6Registers[13].rgb=rsum;
nativeWeatherPlate6Registers[13].a=asum.x;
}
{
nativeWeatherPlate6Registers[1]=material.constants[6];
nativeWeatherPlate6Registers[2]=material.constants[7];
vec3 rrawab=(nativeWeatherPlate6Registers[12].rgb*(vec3(1)-clamp(nativeWeatherPlate6Registers[0].rgb,0.0,1.0))),rrawcd=((vec3(1)-clamp(nativeWeatherPlate6Registers[13].rgb,0.0,1.0))*(vec3(1)-clamp(nativeWeatherPlate6Registers[0].rgb,0.0,1.0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(vec3(nativeWeatherPlate6Registers[13].a)*(vec3(1)-clamp(vec3(nativeWeatherPlate6Registers[0].b),0.0,1.0))),arawcd=((vec3(1)-clamp(vec3(nativeWeatherPlate6Registers[13].b),0.0,1.0))*(-vec3(nativeWeatherPlate6Registers[1].a)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate6Registers[12].rgb=rsum;
nativeWeatherPlate6Registers[13].a=asum.x;
}
{
nativeWeatherPlate6Registers[1]=material.constants[8];
nativeWeatherPlate6Registers[2]=material.constants[9];
vec3 rrawab=vec3(dot(max(nativeWeatherPlate6Registers[9].rgb,vec3(0)),max(nativeWeatherPlate6Registers[1].rgb,vec3(0)))),rrawcd=vec3(dot(max(nativeWeatherPlate6Registers[10].rgb,vec3(0)),max(nativeWeatherPlate6Registers[2].rgb,vec3(0))));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate6Registers[13].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate6Registers[0].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate6Registers[13].a),vec3(0))*max(vec3(nativeWeatherPlate6Registers[0].b),vec3(0)));
vec3 aab=clamp((arawab*4.0),-1.0,1.0),acd=clamp((arawcd*4.0),-1.0,1.0),asum=clamp(((arawab+arawcd)*4.0),-1.0,1.0);
nativeWeatherPlate6Registers[4].rgb=rab;
nativeWeatherPlate6Registers[5].rgb=rcd;
nativeWeatherPlate6Registers[13].a=asum.x;
}
{
nativeWeatherPlate6Registers[1]=material.constants[10];
nativeWeatherPlate6Registers[2]=material.constants[11];
vec3 rrawab=((vec3(1)-clamp(nativeWeatherPlate6Registers[4].rgb,0.0,1.0))*(vec3(1)-clamp(nativeWeatherPlate6Registers[5].rgb,0.0,1.0))),rrawcd=vec3(dot(max(nativeWeatherPlate6Registers[11].rgb,vec3(0)),max(nativeWeatherPlate6Registers[1].rgb,vec3(0))));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate6Registers[13].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate6Registers[0].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate6Registers[13].a),vec3(0))*max(vec3(nativeWeatherPlate6Registers[0].b),vec3(0)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate6Registers[4].rgb=rab;
nativeWeatherPlate6Registers[5].rgb=rcd;
nativeWeatherPlate6Registers[13].a=asum.x;
}
{
nativeWeatherPlate6Registers[1]=material.constants[12];
nativeWeatherPlate6Registers[2]=material.constants[13];
vec3 rrawab=(max(nativeWeatherPlate6Registers[4].rgb,vec3(0))*(vec3(1)-clamp(nativeWeatherPlate6Registers[5].rgb,0.0,1.0))),rrawcd=(max(nativeWeatherPlate6Registers[0].rgb,vec3(0))*max(nativeWeatherPlate6Registers[0].rgb,vec3(0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate6Registers[13].a),vec3(0))*max(vec3(nativeWeatherPlate6Registers[2].a),vec3(0))),arawcd=(max(vec3(nativeWeatherPlate6Registers[12].b),vec3(0))*max(vec3(nativeWeatherPlate6Registers[2].b),vec3(0)));
vec3 aab=clamp((arawab*2.0),-1.0,1.0),acd=clamp((arawcd*2.0),-1.0,1.0),asum=clamp(((arawab+arawcd)*2.0),-1.0,1.0);
nativeWeatherPlate6Registers[13].rgb=rab;
nativeWeatherPlate6Registers[13].a=asum.x;
}
{
nativeWeatherPlate6Registers[1]=material.constants[14];
nativeWeatherPlate6Registers[2]=material.constants[15];
vec3 rrawab=(max(nativeWeatherPlate6Registers[0].rgb,vec3(0))*max(nativeWeatherPlate6Registers[0].rgb,vec3(0))),rrawcd=(max(nativeWeatherPlate6Registers[0].rgb,vec3(0))*max(nativeWeatherPlate6Registers[0].rgb,vec3(0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate6Registers[13].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate6Registers[13].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate6Registers[0].b),vec3(0))*max(vec3(nativeWeatherPlate6Registers[0].b),vec3(0)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate6Registers[13].a=aab.x;
}
nativeWeatherPlate6Registers[1]=material.constants[16];nativeWeatherPlate6Registers[2]=material.constants[17];
nativeWeatherPlate6Registers[14]=vec4((nativeWeatherPlate6Registers[5].rgb+nativeWeatherPlate6Registers[12].rgb),0);
nativeWeatherPlate6Registers[15]=vec4(max(nativeWeatherPlate6Registers[0].rgb,vec3(0))*max(nativeWeatherPlate6Registers[0].rgb,vec3(0)),0);
return clamp(vec4(max(nativeWeatherPlate6Registers[13].aaa,vec3(0))*max(nativeWeatherPlate6Registers[1].rgb,vec3(0))+(vec3(1)-max(nativeWeatherPlate6Registers[13].aaa,vec3(0)))*max(nativeWeatherPlate6Registers[0].rgb,vec3(0))+max(nativeWeatherPlate6Registers[0].rgb,vec3(0)),max(vec3(nativeWeatherPlate6Registers[13].a),vec3(0)).x),0.0,1.0);}

vec4 nativeWeatherPlate7Registers[16];
vec4 nativeWeatherPlate7Argb(uint c){return vec4((c>>16u)&255u,(c>>8u)&255u,c&255u,c>>24u)/255.0;}
vec4 nativeWeatherPlate7(vec4 textures[4],vec4 v0,vec4 v1){
nativeWeatherPlate7Registers[0]=vec4(0);
nativeWeatherPlate7Registers[1]=vec4(0);
nativeWeatherPlate7Registers[2]=vec4(0);
nativeWeatherPlate7Registers[3]=vec4(0);
nativeWeatherPlate7Registers[4]=vec4(0);
nativeWeatherPlate7Registers[5]=vec4(0);
nativeWeatherPlate7Registers[6]=vec4(0);
nativeWeatherPlate7Registers[7]=vec4(0);
nativeWeatherPlate7Registers[8]=vec4(0);
nativeWeatherPlate7Registers[9]=vec4(0);
nativeWeatherPlate7Registers[10]=vec4(0);
nativeWeatherPlate7Registers[11]=vec4(0);
nativeWeatherPlate7Registers[12]=vec4(0);
nativeWeatherPlate7Registers[13]=vec4(0);
nativeWeatherPlate7Registers[14]=vec4(0);
nativeWeatherPlate7Registers[15]=vec4(0);
#ifdef H2_COMBINER_FOG
nativeWeatherPlate7Registers[3]=H2_COMBINER_FOG();
#endif
nativeWeatherPlate7Registers[4]=v0;nativeWeatherPlate7Registers[5]=v1;
nativeWeatherPlate7Registers[8]=textures[0];
nativeWeatherPlate7Registers[9]=textures[1];
nativeWeatherPlate7Registers[10]=textures[2];
nativeWeatherPlate7Registers[11]=textures[3];
nativeWeatherPlate7Registers[12].a=textures[0].a;
{
nativeWeatherPlate7Registers[1]=material.constants[0];
nativeWeatherPlate7Registers[2]=material.constants[1];
vec3 rrawab=vec3(dot(max(nativeWeatherPlate7Registers[8].rgb,vec3(0)),max(nativeWeatherPlate7Registers[1].rgb,vec3(0)))),rrawcd=vec3(dot(max(nativeWeatherPlate7Registers[8].rgb,vec3(0)),max(nativeWeatherPlate7Registers[2].rgb,vec3(0))));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate7Registers[0].b),vec3(0))*max(vec3(nativeWeatherPlate7Registers[0].b),vec3(0))),arawcd=(max(vec3(nativeWeatherPlate7Registers[0].b),vec3(0))*max(vec3(nativeWeatherPlate7Registers[0].b),vec3(0)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate7Registers[4].rgb=rab;
nativeWeatherPlate7Registers[5].rgb=rcd;
}
{
nativeWeatherPlate7Registers[1]=material.constants[2];
nativeWeatherPlate7Registers[2]=material.constants[3];
vec3 rrawab=(max(nativeWeatherPlate7Registers[4].rgb,vec3(0))*(vec3(1)-clamp(nativeWeatherPlate7Registers[0].rgb,0.0,1.0))),rrawcd=(max(nativeWeatherPlate7Registers[1].aaa,vec3(0))*(2.0*max(nativeWeatherPlate7Registers[0].rgb,vec3(0))-vec3(1)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate7Registers[4].b),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate7Registers[0].b),0.0,1.0))),arawcd=((max(vec3(nativeWeatherPlate7Registers[1].a),vec3(0))-vec3(0.5))*(2.0*max(vec3(nativeWeatherPlate7Registers[0].b),vec3(0))-vec3(1)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate7Registers[12].rgb=rsum;
nativeWeatherPlate7Registers[12].a=asum.x;
}
{
nativeWeatherPlate7Registers[1]=material.constants[4];
nativeWeatherPlate7Registers[2]=material.constants[5];
vec3 rrawab=(max(nativeWeatherPlate7Registers[0].rgb,vec3(0))*max(nativeWeatherPlate7Registers[0].rgb,vec3(0))),rrawcd=((vec3(1)-clamp(nativeWeatherPlate7Registers[0].rgb,0.0,1.0))*(vec3(1)-clamp(nativeWeatherPlate7Registers[0].rgb,0.0,1.0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp(((nativeWeatherPlate7Registers[12].a>=0.5)?rrawcd:rrawab),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate7Registers[8].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate7Registers[0].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate7Registers[1].a),vec3(0))*(2.0*max(vec3(nativeWeatherPlate7Registers[0].b),vec3(0))-vec3(1)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate7Registers[13].rgb=rsum;
nativeWeatherPlate7Registers[13].a=asum.x;
}
{
nativeWeatherPlate7Registers[1]=material.constants[6];
nativeWeatherPlate7Registers[2]=material.constants[7];
vec3 rrawab=(nativeWeatherPlate7Registers[12].rgb*(vec3(1)-clamp(nativeWeatherPlate7Registers[0].rgb,0.0,1.0))),rrawcd=((vec3(1)-clamp(nativeWeatherPlate7Registers[13].rgb,0.0,1.0))*(vec3(1)-clamp(nativeWeatherPlate7Registers[0].rgb,0.0,1.0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(vec3(nativeWeatherPlate7Registers[13].a)*(vec3(1)-clamp(vec3(nativeWeatherPlate7Registers[0].b),0.0,1.0))),arawcd=((vec3(1)-clamp(vec3(nativeWeatherPlate7Registers[13].b),0.0,1.0))*(-vec3(nativeWeatherPlate7Registers[1].a)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate7Registers[12].rgb=rsum;
nativeWeatherPlate7Registers[13].a=asum.x;
}
{
nativeWeatherPlate7Registers[1]=material.constants[8];
nativeWeatherPlate7Registers[2]=material.constants[9];
vec3 rrawab=vec3(dot(max(nativeWeatherPlate7Registers[9].aaa,vec3(0)),max(nativeWeatherPlate7Registers[1].rgb,vec3(0)))),rrawcd=vec3(dot(max(nativeWeatherPlate7Registers[10].aaa,vec3(0)),max(nativeWeatherPlate7Registers[2].rgb,vec3(0))));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate7Registers[13].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate7Registers[0].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate7Registers[13].a),vec3(0))*max(vec3(nativeWeatherPlate7Registers[0].b),vec3(0)));
vec3 aab=clamp((arawab*4.0),-1.0,1.0),acd=clamp((arawcd*4.0),-1.0,1.0),asum=clamp(((arawab+arawcd)*4.0),-1.0,1.0);
nativeWeatherPlate7Registers[4].rgb=rab;
nativeWeatherPlate7Registers[5].rgb=rcd;
nativeWeatherPlate7Registers[13].a=asum.x;
}
{
nativeWeatherPlate7Registers[1]=material.constants[10];
nativeWeatherPlate7Registers[2]=material.constants[11];
vec3 rrawab=((vec3(1)-clamp(nativeWeatherPlate7Registers[4].rgb,0.0,1.0))*(vec3(1)-clamp(nativeWeatherPlate7Registers[5].rgb,0.0,1.0))),rrawcd=vec3(dot(max(nativeWeatherPlate7Registers[11].aaa,vec3(0)),max(nativeWeatherPlate7Registers[1].rgb,vec3(0))));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate7Registers[13].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate7Registers[0].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate7Registers[13].a),vec3(0))*max(vec3(nativeWeatherPlate7Registers[0].b),vec3(0)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate7Registers[4].rgb=rab;
nativeWeatherPlate7Registers[5].rgb=rcd;
nativeWeatherPlate7Registers[13].a=asum.x;
}
{
nativeWeatherPlate7Registers[1]=material.constants[12];
nativeWeatherPlate7Registers[2]=material.constants[13];
vec3 rrawab=(max(nativeWeatherPlate7Registers[4].rgb,vec3(0))*(vec3(1)-clamp(nativeWeatherPlate7Registers[5].rgb,0.0,1.0))),rrawcd=(max(nativeWeatherPlate7Registers[0].rgb,vec3(0))*max(nativeWeatherPlate7Registers[0].rgb,vec3(0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate7Registers[13].a),vec3(0))*max(vec3(nativeWeatherPlate7Registers[2].a),vec3(0))),arawcd=(max(vec3(nativeWeatherPlate7Registers[12].b),vec3(0))*max(vec3(nativeWeatherPlate7Registers[2].b),vec3(0)));
vec3 aab=clamp((arawab*2.0),-1.0,1.0),acd=clamp((arawcd*2.0),-1.0,1.0),asum=clamp(((arawab+arawcd)*2.0),-1.0,1.0);
nativeWeatherPlate7Registers[13].rgb=rab;
nativeWeatherPlate7Registers[13].a=asum.x;
}
{
nativeWeatherPlate7Registers[1]=material.constants[14];
nativeWeatherPlate7Registers[2]=material.constants[15];
vec3 rrawab=(max(nativeWeatherPlate7Registers[0].rgb,vec3(0))*max(nativeWeatherPlate7Registers[0].rgb,vec3(0))),rrawcd=(max(nativeWeatherPlate7Registers[0].rgb,vec3(0))*max(nativeWeatherPlate7Registers[0].rgb,vec3(0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate7Registers[13].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate7Registers[13].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate7Registers[0].b),vec3(0))*max(vec3(nativeWeatherPlate7Registers[0].b),vec3(0)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate7Registers[13].a=aab.x;
}
nativeWeatherPlate7Registers[1]=material.constants[16];nativeWeatherPlate7Registers[2]=material.constants[17];
nativeWeatherPlate7Registers[14]=vec4((nativeWeatherPlate7Registers[5].rgb+nativeWeatherPlate7Registers[12].rgb),0);
nativeWeatherPlate7Registers[15]=vec4(max(nativeWeatherPlate7Registers[0].rgb,vec3(0))*max(nativeWeatherPlate7Registers[0].rgb,vec3(0)),0);
return clamp(vec4(max(nativeWeatherPlate7Registers[13].aaa,vec3(0))*max(nativeWeatherPlate7Registers[1].rgb,vec3(0))+(vec3(1)-max(nativeWeatherPlate7Registers[13].aaa,vec3(0)))*max(nativeWeatherPlate7Registers[0].rgb,vec3(0))+max(nativeWeatherPlate7Registers[0].rgb,vec3(0)),max(vec3(nativeWeatherPlate7Registers[13].a),vec3(0)).x),0.0,1.0);}

vec4 nativeWeatherPlate8Registers[16];
vec4 nativeWeatherPlate8Argb(uint c){return vec4((c>>16u)&255u,(c>>8u)&255u,c&255u,c>>24u)/255.0;}
vec4 nativeWeatherPlate8(vec4 textures[4],vec4 v0,vec4 v1){
nativeWeatherPlate8Registers[0]=vec4(0);
nativeWeatherPlate8Registers[1]=vec4(0);
nativeWeatherPlate8Registers[2]=vec4(0);
nativeWeatherPlate8Registers[3]=vec4(0);
nativeWeatherPlate8Registers[4]=vec4(0);
nativeWeatherPlate8Registers[5]=vec4(0);
nativeWeatherPlate8Registers[6]=vec4(0);
nativeWeatherPlate8Registers[7]=vec4(0);
nativeWeatherPlate8Registers[8]=vec4(0);
nativeWeatherPlate8Registers[9]=vec4(0);
nativeWeatherPlate8Registers[10]=vec4(0);
nativeWeatherPlate8Registers[11]=vec4(0);
nativeWeatherPlate8Registers[12]=vec4(0);
nativeWeatherPlate8Registers[13]=vec4(0);
nativeWeatherPlate8Registers[14]=vec4(0);
nativeWeatherPlate8Registers[15]=vec4(0);
#ifdef H2_COMBINER_FOG
nativeWeatherPlate8Registers[3]=H2_COMBINER_FOG();
#endif
nativeWeatherPlate8Registers[4]=v0;nativeWeatherPlate8Registers[5]=v1;
nativeWeatherPlate8Registers[8]=textures[0];
nativeWeatherPlate8Registers[9]=textures[1];
nativeWeatherPlate8Registers[10]=textures[2];
nativeWeatherPlate8Registers[11]=textures[3];
nativeWeatherPlate8Registers[12].a=textures[0].a;
{
nativeWeatherPlate8Registers[1]=material.constants[0];
nativeWeatherPlate8Registers[2]=material.constants[1];
vec3 rrawab=vec3(dot(max(nativeWeatherPlate8Registers[8].rgb,vec3(0)),max(nativeWeatherPlate8Registers[1].rgb,vec3(0)))),rrawcd=vec3(dot(max(nativeWeatherPlate8Registers[8].rgb,vec3(0)),max(nativeWeatherPlate8Registers[2].rgb,vec3(0))));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate8Registers[0].b),vec3(0))*max(vec3(nativeWeatherPlate8Registers[0].b),vec3(0))),arawcd=(max(vec3(nativeWeatherPlate8Registers[0].b),vec3(0))*max(vec3(nativeWeatherPlate8Registers[0].b),vec3(0)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate8Registers[4].rgb=rab;
nativeWeatherPlate8Registers[5].rgb=rcd;
}
{
nativeWeatherPlate8Registers[1]=material.constants[2];
nativeWeatherPlate8Registers[2]=material.constants[3];
vec3 rrawab=(max(nativeWeatherPlate8Registers[4].rgb,vec3(0))*(vec3(1)-clamp(nativeWeatherPlate8Registers[0].rgb,0.0,1.0))),rrawcd=(max(nativeWeatherPlate8Registers[1].aaa,vec3(0))*(2.0*max(nativeWeatherPlate8Registers[0].rgb,vec3(0))-vec3(1)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate8Registers[4].b),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate8Registers[0].b),0.0,1.0))),arawcd=((max(vec3(nativeWeatherPlate8Registers[1].a),vec3(0))-vec3(0.5))*(2.0*max(vec3(nativeWeatherPlate8Registers[0].b),vec3(0))-vec3(1)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate8Registers[12].rgb=rsum;
nativeWeatherPlate8Registers[12].a=asum.x;
}
{
nativeWeatherPlate8Registers[1]=material.constants[4];
nativeWeatherPlate8Registers[2]=material.constants[5];
vec3 rrawab=(max(nativeWeatherPlate8Registers[0].rgb,vec3(0))*max(nativeWeatherPlate8Registers[0].rgb,vec3(0))),rrawcd=((vec3(1)-clamp(nativeWeatherPlate8Registers[0].rgb,0.0,1.0))*(vec3(1)-clamp(nativeWeatherPlate8Registers[0].rgb,0.0,1.0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp(((nativeWeatherPlate8Registers[12].a>=0.5)?rrawcd:rrawab),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate8Registers[8].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate8Registers[0].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate8Registers[1].a),vec3(0))*(2.0*max(vec3(nativeWeatherPlate8Registers[0].b),vec3(0))-vec3(1)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate8Registers[13].rgb=rsum;
nativeWeatherPlate8Registers[13].a=asum.x;
}
{
nativeWeatherPlate8Registers[1]=material.constants[6];
nativeWeatherPlate8Registers[2]=material.constants[7];
vec3 rrawab=(nativeWeatherPlate8Registers[12].rgb*(vec3(1)-clamp(nativeWeatherPlate8Registers[0].rgb,0.0,1.0))),rrawcd=((vec3(1)-clamp(nativeWeatherPlate8Registers[13].rgb,0.0,1.0))*(vec3(1)-clamp(nativeWeatherPlate8Registers[0].rgb,0.0,1.0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(vec3(nativeWeatherPlate8Registers[13].a)*(vec3(1)-clamp(vec3(nativeWeatherPlate8Registers[0].b),0.0,1.0))),arawcd=((vec3(1)-clamp(vec3(nativeWeatherPlate8Registers[13].b),0.0,1.0))*(-vec3(nativeWeatherPlate8Registers[1].a)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate8Registers[12].rgb=rsum;
nativeWeatherPlate8Registers[13].a=asum.x;
}
{
nativeWeatherPlate8Registers[1]=material.constants[8];
nativeWeatherPlate8Registers[2]=material.constants[9];
vec3 rrawab=vec3(dot(max(nativeWeatherPlate8Registers[9].rgb,vec3(0)),max(nativeWeatherPlate8Registers[1].rgb,vec3(0)))),rrawcd=vec3(dot(max(nativeWeatherPlate8Registers[10].rgb,vec3(0)),max(nativeWeatherPlate8Registers[2].rgb,vec3(0))));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate8Registers[13].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate8Registers[0].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate8Registers[13].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate8Registers[0].b),0.0,1.0)));
vec3 aab=clamp((arawab*4.0),-1.0,1.0),acd=clamp((arawcd*4.0),-1.0,1.0),asum=clamp(((arawab+arawcd)*4.0),-1.0,1.0);
nativeWeatherPlate8Registers[4].rgb=rab;
nativeWeatherPlate8Registers[5].rgb=rcd;
nativeWeatherPlate8Registers[13].a=asum.x;
}
{
nativeWeatherPlate8Registers[1]=material.constants[10];
nativeWeatherPlate8Registers[2]=material.constants[11];
vec3 rrawab=((vec3(1)-clamp(nativeWeatherPlate8Registers[4].rgb,0.0,1.0))*(vec3(1)-clamp(nativeWeatherPlate8Registers[5].rgb,0.0,1.0))),rrawcd=vec3(dot(max(nativeWeatherPlate8Registers[11].rgb,vec3(0)),max(nativeWeatherPlate8Registers[1].rgb,vec3(0))));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate8Registers[13].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate8Registers[0].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate8Registers[13].a),vec3(0))*max(vec3(nativeWeatherPlate8Registers[0].b),vec3(0)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate8Registers[4].rgb=rab;
nativeWeatherPlate8Registers[5].rgb=rcd;
nativeWeatherPlate8Registers[13].a=asum.x;
}
{
nativeWeatherPlate8Registers[1]=material.constants[12];
nativeWeatherPlate8Registers[2]=material.constants[13];
vec3 rrawab=(max(nativeWeatherPlate8Registers[4].rgb,vec3(0))*(vec3(1)-clamp(nativeWeatherPlate8Registers[5].rgb,0.0,1.0))),rrawcd=(max(nativeWeatherPlate8Registers[0].rgb,vec3(0))*max(nativeWeatherPlate8Registers[0].rgb,vec3(0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate8Registers[13].a),vec3(0))*max(vec3(nativeWeatherPlate8Registers[2].a),vec3(0))),arawcd=(max(vec3(nativeWeatherPlate8Registers[12].b),vec3(0))*max(vec3(nativeWeatherPlate8Registers[2].b),vec3(0)));
vec3 aab=clamp((arawab*2.0),-1.0,1.0),acd=clamp((arawcd*2.0),-1.0,1.0),asum=clamp(((arawab+arawcd)*2.0),-1.0,1.0);
nativeWeatherPlate8Registers[13].rgb=rab;
nativeWeatherPlate8Registers[13].a=asum.x;
}
{
nativeWeatherPlate8Registers[1]=material.constants[14];
nativeWeatherPlate8Registers[2]=material.constants[15];
vec3 rrawab=(max(nativeWeatherPlate8Registers[0].rgb,vec3(0))*max(nativeWeatherPlate8Registers[0].rgb,vec3(0))),rrawcd=(max(nativeWeatherPlate8Registers[0].rgb,vec3(0))*max(nativeWeatherPlate8Registers[0].rgb,vec3(0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate8Registers[13].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate8Registers[13].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate8Registers[0].b),vec3(0))*max(vec3(nativeWeatherPlate8Registers[0].b),vec3(0)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate8Registers[13].a=aab.x;
}
nativeWeatherPlate8Registers[1]=material.constants[16];nativeWeatherPlate8Registers[2]=material.constants[17];
nativeWeatherPlate8Registers[14]=vec4((nativeWeatherPlate8Registers[5].rgb+nativeWeatherPlate8Registers[12].rgb),0);
nativeWeatherPlate8Registers[15]=vec4(max(nativeWeatherPlate8Registers[0].rgb,vec3(0))*max(nativeWeatherPlate8Registers[0].rgb,vec3(0)),0);
return clamp(vec4(max(nativeWeatherPlate8Registers[13].aaa,vec3(0))*max(nativeWeatherPlate8Registers[1].rgb,vec3(0))+(vec3(1)-max(nativeWeatherPlate8Registers[13].aaa,vec3(0)))*max(nativeWeatherPlate8Registers[0].rgb,vec3(0))+max(nativeWeatherPlate8Registers[0].rgb,vec3(0)),max(vec3(nativeWeatherPlate8Registers[13].a),vec3(0)).x),0.0,1.0);}

vec4 nativeWeatherPlate9Registers[16];
vec4 nativeWeatherPlate9Argb(uint c){return vec4((c>>16u)&255u,(c>>8u)&255u,c&255u,c>>24u)/255.0;}
vec4 nativeWeatherPlate9(vec4 textures[4],vec4 v0,vec4 v1){
nativeWeatherPlate9Registers[0]=vec4(0);
nativeWeatherPlate9Registers[1]=vec4(0);
nativeWeatherPlate9Registers[2]=vec4(0);
nativeWeatherPlate9Registers[3]=vec4(0);
nativeWeatherPlate9Registers[4]=vec4(0);
nativeWeatherPlate9Registers[5]=vec4(0);
nativeWeatherPlate9Registers[6]=vec4(0);
nativeWeatherPlate9Registers[7]=vec4(0);
nativeWeatherPlate9Registers[8]=vec4(0);
nativeWeatherPlate9Registers[9]=vec4(0);
nativeWeatherPlate9Registers[10]=vec4(0);
nativeWeatherPlate9Registers[11]=vec4(0);
nativeWeatherPlate9Registers[12]=vec4(0);
nativeWeatherPlate9Registers[13]=vec4(0);
nativeWeatherPlate9Registers[14]=vec4(0);
nativeWeatherPlate9Registers[15]=vec4(0);
#ifdef H2_COMBINER_FOG
nativeWeatherPlate9Registers[3]=H2_COMBINER_FOG();
#endif
nativeWeatherPlate9Registers[4]=v0;nativeWeatherPlate9Registers[5]=v1;
nativeWeatherPlate9Registers[8]=textures[0];
nativeWeatherPlate9Registers[9]=textures[1];
nativeWeatherPlate9Registers[10]=textures[2];
nativeWeatherPlate9Registers[11]=textures[3];
nativeWeatherPlate9Registers[12].a=textures[0].a;
{
nativeWeatherPlate9Registers[1]=material.constants[0];
nativeWeatherPlate9Registers[2]=material.constants[1];
vec3 rrawab=vec3(dot(max(nativeWeatherPlate9Registers[8].rgb,vec3(0)),max(nativeWeatherPlate9Registers[1].rgb,vec3(0)))),rrawcd=vec3(dot(max(nativeWeatherPlate9Registers[8].rgb,vec3(0)),max(nativeWeatherPlate9Registers[2].rgb,vec3(0))));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate9Registers[0].b),vec3(0))*max(vec3(nativeWeatherPlate9Registers[0].b),vec3(0))),arawcd=(max(vec3(nativeWeatherPlate9Registers[0].b),vec3(0))*max(vec3(nativeWeatherPlate9Registers[0].b),vec3(0)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate9Registers[4].rgb=rab;
nativeWeatherPlate9Registers[5].rgb=rcd;
}
{
nativeWeatherPlate9Registers[1]=material.constants[2];
nativeWeatherPlate9Registers[2]=material.constants[3];
vec3 rrawab=(max(nativeWeatherPlate9Registers[4].rgb,vec3(0))*(vec3(1)-clamp(nativeWeatherPlate9Registers[0].rgb,0.0,1.0))),rrawcd=(max(nativeWeatherPlate9Registers[1].aaa,vec3(0))*(2.0*max(nativeWeatherPlate9Registers[0].rgb,vec3(0))-vec3(1)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate9Registers[4].b),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate9Registers[0].b),0.0,1.0))),arawcd=((max(vec3(nativeWeatherPlate9Registers[1].a),vec3(0))-vec3(0.5))*(2.0*max(vec3(nativeWeatherPlate9Registers[0].b),vec3(0))-vec3(1)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate9Registers[12].rgb=rsum;
nativeWeatherPlate9Registers[12].a=asum.x;
}
{
nativeWeatherPlate9Registers[1]=material.constants[4];
nativeWeatherPlate9Registers[2]=material.constants[5];
vec3 rrawab=(max(nativeWeatherPlate9Registers[0].rgb,vec3(0))*max(nativeWeatherPlate9Registers[0].rgb,vec3(0))),rrawcd=((vec3(1)-clamp(nativeWeatherPlate9Registers[0].rgb,0.0,1.0))*(vec3(1)-clamp(nativeWeatherPlate9Registers[0].rgb,0.0,1.0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp(((nativeWeatherPlate9Registers[12].a>=0.5)?rrawcd:rrawab),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate9Registers[8].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate9Registers[0].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate9Registers[1].a),vec3(0))*(2.0*max(vec3(nativeWeatherPlate9Registers[0].b),vec3(0))-vec3(1)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate9Registers[13].rgb=rsum;
nativeWeatherPlate9Registers[13].a=asum.x;
}
{
nativeWeatherPlate9Registers[1]=material.constants[6];
nativeWeatherPlate9Registers[2]=material.constants[7];
vec3 rrawab=(nativeWeatherPlate9Registers[12].rgb*(vec3(1)-clamp(nativeWeatherPlate9Registers[0].rgb,0.0,1.0))),rrawcd=((vec3(1)-clamp(nativeWeatherPlate9Registers[13].rgb,0.0,1.0))*(vec3(1)-clamp(nativeWeatherPlate9Registers[0].rgb,0.0,1.0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(vec3(nativeWeatherPlate9Registers[13].a)*(vec3(1)-clamp(vec3(nativeWeatherPlate9Registers[0].b),0.0,1.0))),arawcd=((vec3(1)-clamp(vec3(nativeWeatherPlate9Registers[13].b),0.0,1.0))*(-vec3(nativeWeatherPlate9Registers[1].a)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate9Registers[12].rgb=rsum;
nativeWeatherPlate9Registers[13].a=asum.x;
}
{
nativeWeatherPlate9Registers[1]=material.constants[8];
nativeWeatherPlate9Registers[2]=material.constants[9];
vec3 rrawab=vec3(dot(max(nativeWeatherPlate9Registers[9].aaa,vec3(0)),max(nativeWeatherPlate9Registers[1].rgb,vec3(0)))),rrawcd=vec3(dot(max(nativeWeatherPlate9Registers[10].aaa,vec3(0)),max(nativeWeatherPlate9Registers[2].rgb,vec3(0))));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate9Registers[13].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate9Registers[0].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate9Registers[13].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate9Registers[0].b),0.0,1.0)));
vec3 aab=clamp((arawab*4.0),-1.0,1.0),acd=clamp((arawcd*4.0),-1.0,1.0),asum=clamp(((arawab+arawcd)*4.0),-1.0,1.0);
nativeWeatherPlate9Registers[4].rgb=rab;
nativeWeatherPlate9Registers[5].rgb=rcd;
nativeWeatherPlate9Registers[13].a=asum.x;
}
{
nativeWeatherPlate9Registers[1]=material.constants[10];
nativeWeatherPlate9Registers[2]=material.constants[11];
vec3 rrawab=((vec3(1)-clamp(nativeWeatherPlate9Registers[4].rgb,0.0,1.0))*(vec3(1)-clamp(nativeWeatherPlate9Registers[5].rgb,0.0,1.0))),rrawcd=vec3(dot(max(nativeWeatherPlate9Registers[11].aaa,vec3(0)),max(nativeWeatherPlate9Registers[1].rgb,vec3(0))));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate9Registers[13].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate9Registers[0].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate9Registers[13].a),vec3(0))*max(vec3(nativeWeatherPlate9Registers[0].b),vec3(0)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate9Registers[4].rgb=rab;
nativeWeatherPlate9Registers[5].rgb=rcd;
nativeWeatherPlate9Registers[13].a=asum.x;
}
{
nativeWeatherPlate9Registers[1]=material.constants[12];
nativeWeatherPlate9Registers[2]=material.constants[13];
vec3 rrawab=(max(nativeWeatherPlate9Registers[4].rgb,vec3(0))*(vec3(1)-clamp(nativeWeatherPlate9Registers[5].rgb,0.0,1.0))),rrawcd=(max(nativeWeatherPlate9Registers[0].rgb,vec3(0))*max(nativeWeatherPlate9Registers[0].rgb,vec3(0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate9Registers[13].a),vec3(0))*max(vec3(nativeWeatherPlate9Registers[2].a),vec3(0))),arawcd=(max(vec3(nativeWeatherPlate9Registers[12].b),vec3(0))*max(vec3(nativeWeatherPlate9Registers[2].b),vec3(0)));
vec3 aab=clamp((arawab*2.0),-1.0,1.0),acd=clamp((arawcd*2.0),-1.0,1.0),asum=clamp(((arawab+arawcd)*2.0),-1.0,1.0);
nativeWeatherPlate9Registers[13].rgb=rab;
nativeWeatherPlate9Registers[13].a=asum.x;
}
{
nativeWeatherPlate9Registers[1]=material.constants[14];
nativeWeatherPlate9Registers[2]=material.constants[15];
vec3 rrawab=(max(nativeWeatherPlate9Registers[0].rgb,vec3(0))*max(nativeWeatherPlate9Registers[0].rgb,vec3(0))),rrawcd=(max(nativeWeatherPlate9Registers[0].rgb,vec3(0))*max(nativeWeatherPlate9Registers[0].rgb,vec3(0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate9Registers[13].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate9Registers[13].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate9Registers[0].b),vec3(0))*max(vec3(nativeWeatherPlate9Registers[0].b),vec3(0)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate9Registers[13].a=aab.x;
}
nativeWeatherPlate9Registers[1]=material.constants[16];nativeWeatherPlate9Registers[2]=material.constants[17];
nativeWeatherPlate9Registers[14]=vec4((nativeWeatherPlate9Registers[5].rgb+nativeWeatherPlate9Registers[12].rgb),0);
nativeWeatherPlate9Registers[15]=vec4(max(nativeWeatherPlate9Registers[0].rgb,vec3(0))*max(nativeWeatherPlate9Registers[0].rgb,vec3(0)),0);
return clamp(vec4(max(nativeWeatherPlate9Registers[13].aaa,vec3(0))*max(nativeWeatherPlate9Registers[1].rgb,vec3(0))+(vec3(1)-max(nativeWeatherPlate9Registers[13].aaa,vec3(0)))*max(nativeWeatherPlate9Registers[0].rgb,vec3(0))+max(nativeWeatherPlate9Registers[0].rgb,vec3(0)),max(vec3(nativeWeatherPlate9Registers[13].a),vec3(0)).x),0.0,1.0);}

vec4 nativeWeatherPlate10Registers[16];
vec4 nativeWeatherPlate10Argb(uint c){return vec4((c>>16u)&255u,(c>>8u)&255u,c&255u,c>>24u)/255.0;}
vec4 nativeWeatherPlate10(vec4 textures[4],vec4 v0,vec4 v1){
nativeWeatherPlate10Registers[0]=vec4(0);
nativeWeatherPlate10Registers[1]=vec4(0);
nativeWeatherPlate10Registers[2]=vec4(0);
nativeWeatherPlate10Registers[3]=vec4(0);
nativeWeatherPlate10Registers[4]=vec4(0);
nativeWeatherPlate10Registers[5]=vec4(0);
nativeWeatherPlate10Registers[6]=vec4(0);
nativeWeatherPlate10Registers[7]=vec4(0);
nativeWeatherPlate10Registers[8]=vec4(0);
nativeWeatherPlate10Registers[9]=vec4(0);
nativeWeatherPlate10Registers[10]=vec4(0);
nativeWeatherPlate10Registers[11]=vec4(0);
nativeWeatherPlate10Registers[12]=vec4(0);
nativeWeatherPlate10Registers[13]=vec4(0);
nativeWeatherPlate10Registers[14]=vec4(0);
nativeWeatherPlate10Registers[15]=vec4(0);
#ifdef H2_COMBINER_FOG
nativeWeatherPlate10Registers[3]=H2_COMBINER_FOG();
#endif
nativeWeatherPlate10Registers[4]=v0;nativeWeatherPlate10Registers[5]=v1;
nativeWeatherPlate10Registers[8]=textures[0];
nativeWeatherPlate10Registers[9]=textures[1];
nativeWeatherPlate10Registers[10]=textures[2];
nativeWeatherPlate10Registers[11]=textures[3];
nativeWeatherPlate10Registers[12].a=textures[0].a;
{
nativeWeatherPlate10Registers[1]=material.constants[0];
nativeWeatherPlate10Registers[2]=material.constants[1];
vec3 rrawab=vec3(dot(max(nativeWeatherPlate10Registers[8].rgb,vec3(0)),max(nativeWeatherPlate10Registers[1].rgb,vec3(0)))),rrawcd=vec3(dot(max(nativeWeatherPlate10Registers[8].rgb,vec3(0)),max(nativeWeatherPlate10Registers[2].rgb,vec3(0))));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate10Registers[0].b),vec3(0))*max(vec3(nativeWeatherPlate10Registers[0].b),vec3(0))),arawcd=(max(vec3(nativeWeatherPlate10Registers[0].b),vec3(0))*max(vec3(nativeWeatherPlate10Registers[0].b),vec3(0)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate10Registers[4].rgb=rab;
nativeWeatherPlate10Registers[5].rgb=rcd;
}
{
nativeWeatherPlate10Registers[1]=material.constants[2];
nativeWeatherPlate10Registers[2]=material.constants[3];
vec3 rrawab=(max(nativeWeatherPlate10Registers[4].rgb,vec3(0))*(vec3(1)-clamp(nativeWeatherPlate10Registers[0].rgb,0.0,1.0))),rrawcd=(max(nativeWeatherPlate10Registers[1].aaa,vec3(0))*(2.0*max(nativeWeatherPlate10Registers[0].rgb,vec3(0))-vec3(1)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate10Registers[4].b),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate10Registers[0].b),0.0,1.0))),arawcd=((max(vec3(nativeWeatherPlate10Registers[1].a),vec3(0))-vec3(0.5))*(2.0*max(vec3(nativeWeatherPlate10Registers[0].b),vec3(0))-vec3(1)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate10Registers[12].rgb=rsum;
nativeWeatherPlate10Registers[12].a=asum.x;
}
{
nativeWeatherPlate10Registers[1]=material.constants[4];
nativeWeatherPlate10Registers[2]=material.constants[5];
vec3 rrawab=(max(nativeWeatherPlate10Registers[0].rgb,vec3(0))*max(nativeWeatherPlate10Registers[0].rgb,vec3(0))),rrawcd=((vec3(1)-clamp(nativeWeatherPlate10Registers[0].rgb,0.0,1.0))*(vec3(1)-clamp(nativeWeatherPlate10Registers[0].rgb,0.0,1.0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp(((nativeWeatherPlate10Registers[12].a>=0.5)?rrawcd:rrawab),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate10Registers[8].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate10Registers[0].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate10Registers[1].a),vec3(0))*(2.0*max(vec3(nativeWeatherPlate10Registers[0].b),vec3(0))-vec3(1)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate10Registers[13].rgb=rsum;
nativeWeatherPlate10Registers[13].a=asum.x;
}
{
nativeWeatherPlate10Registers[1]=material.constants[6];
nativeWeatherPlate10Registers[2]=material.constants[7];
vec3 rrawab=(nativeWeatherPlate10Registers[12].rgb*(vec3(1)-clamp(nativeWeatherPlate10Registers[0].rgb,0.0,1.0))),rrawcd=((vec3(1)-clamp(nativeWeatherPlate10Registers[13].rgb,0.0,1.0))*(vec3(1)-clamp(nativeWeatherPlate10Registers[0].rgb,0.0,1.0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(vec3(nativeWeatherPlate10Registers[13].a)*(vec3(1)-clamp(vec3(nativeWeatherPlate10Registers[0].b),0.0,1.0))),arawcd=((vec3(1)-clamp(vec3(nativeWeatherPlate10Registers[13].b),0.0,1.0))*(-vec3(nativeWeatherPlate10Registers[1].a)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate10Registers[12].rgb=rsum;
nativeWeatherPlate10Registers[13].a=asum.x;
}
{
nativeWeatherPlate10Registers[1]=material.constants[8];
nativeWeatherPlate10Registers[2]=material.constants[9];
vec3 rrawab=vec3(dot(max(nativeWeatherPlate10Registers[9].rgb,vec3(0)),max(nativeWeatherPlate10Registers[1].rgb,vec3(0)))),rrawcd=vec3(dot(max(nativeWeatherPlate10Registers[10].rgb,vec3(0)),max(nativeWeatherPlate10Registers[2].rgb,vec3(0))));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate10Registers[13].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate10Registers[0].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate10Registers[13].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate10Registers[0].b),0.0,1.0)));
vec3 aab=clamp((arawab*4.0),-1.0,1.0),acd=clamp((arawcd*4.0),-1.0,1.0),asum=clamp(((arawab+arawcd)*4.0),-1.0,1.0);
nativeWeatherPlate10Registers[4].rgb=rab;
nativeWeatherPlate10Registers[5].rgb=rcd;
nativeWeatherPlate10Registers[13].a=asum.x;
}
{
nativeWeatherPlate10Registers[1]=material.constants[10];
nativeWeatherPlate10Registers[2]=material.constants[11];
vec3 rrawab=((vec3(1)-clamp(nativeWeatherPlate10Registers[4].rgb,0.0,1.0))*(vec3(1)-clamp(nativeWeatherPlate10Registers[5].rgb,0.0,1.0))),rrawcd=vec3(dot(max(nativeWeatherPlate10Registers[11].rgb,vec3(0)),max(nativeWeatherPlate10Registers[1].rgb,vec3(0))));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate10Registers[13].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate10Registers[0].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate10Registers[13].a),vec3(0))*max(vec3(nativeWeatherPlate10Registers[0].b),vec3(0)));
vec3 aab=clamp((arawab*2.0),-1.0,1.0),acd=clamp((arawcd*2.0),-1.0,1.0),asum=clamp(((arawab+arawcd)*2.0),-1.0,1.0);
nativeWeatherPlate10Registers[4].rgb=rab;
nativeWeatherPlate10Registers[5].rgb=rcd;
nativeWeatherPlate10Registers[13].a=asum.x;
}
{
nativeWeatherPlate10Registers[1]=material.constants[12];
nativeWeatherPlate10Registers[2]=material.constants[13];
vec3 rrawab=(max(nativeWeatherPlate10Registers[4].rgb,vec3(0))*(vec3(1)-clamp(nativeWeatherPlate10Registers[5].rgb,0.0,1.0))),rrawcd=(max(nativeWeatherPlate10Registers[0].rgb,vec3(0))*max(nativeWeatherPlate10Registers[0].rgb,vec3(0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate10Registers[13].a),vec3(0))*max(vec3(nativeWeatherPlate10Registers[2].a),vec3(0))),arawcd=(max(vec3(nativeWeatherPlate10Registers[12].b),vec3(0))*max(vec3(nativeWeatherPlate10Registers[2].b),vec3(0)));
vec3 aab=clamp((arawab*2.0),-1.0,1.0),acd=clamp((arawcd*2.0),-1.0,1.0),asum=clamp(((arawab+arawcd)*2.0),-1.0,1.0);
nativeWeatherPlate10Registers[13].rgb=rab;
nativeWeatherPlate10Registers[13].a=asum.x;
}
{
nativeWeatherPlate10Registers[1]=material.constants[14];
nativeWeatherPlate10Registers[2]=material.constants[15];
vec3 rrawab=(max(nativeWeatherPlate10Registers[0].rgb,vec3(0))*max(nativeWeatherPlate10Registers[0].rgb,vec3(0))),rrawcd=(max(nativeWeatherPlate10Registers[0].rgb,vec3(0))*max(nativeWeatherPlate10Registers[0].rgb,vec3(0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate10Registers[13].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate10Registers[13].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate10Registers[0].b),vec3(0))*max(vec3(nativeWeatherPlate10Registers[0].b),vec3(0)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate10Registers[13].a=aab.x;
}
nativeWeatherPlate10Registers[1]=material.constants[16];nativeWeatherPlate10Registers[2]=material.constants[17];
nativeWeatherPlate10Registers[14]=vec4((nativeWeatherPlate10Registers[5].rgb+nativeWeatherPlate10Registers[12].rgb),0);
nativeWeatherPlate10Registers[15]=vec4(max(nativeWeatherPlate10Registers[0].rgb,vec3(0))*max(nativeWeatherPlate10Registers[0].rgb,vec3(0)),0);
return clamp(vec4(max(nativeWeatherPlate10Registers[13].aaa,vec3(0))*max(nativeWeatherPlate10Registers[1].rgb,vec3(0))+(vec3(1)-max(nativeWeatherPlate10Registers[13].aaa,vec3(0)))*max(nativeWeatherPlate10Registers[0].rgb,vec3(0))+max(nativeWeatherPlate10Registers[0].rgb,vec3(0)),max(vec3(nativeWeatherPlate10Registers[13].a),vec3(0)).x),0.0,1.0);}

vec4 nativeWeatherPlate11Registers[16];
vec4 nativeWeatherPlate11Argb(uint c){return vec4((c>>16u)&255u,(c>>8u)&255u,c&255u,c>>24u)/255.0;}
vec4 nativeWeatherPlate11(vec4 textures[4],vec4 v0,vec4 v1){
nativeWeatherPlate11Registers[0]=vec4(0);
nativeWeatherPlate11Registers[1]=vec4(0);
nativeWeatherPlate11Registers[2]=vec4(0);
nativeWeatherPlate11Registers[3]=vec4(0);
nativeWeatherPlate11Registers[4]=vec4(0);
nativeWeatherPlate11Registers[5]=vec4(0);
nativeWeatherPlate11Registers[6]=vec4(0);
nativeWeatherPlate11Registers[7]=vec4(0);
nativeWeatherPlate11Registers[8]=vec4(0);
nativeWeatherPlate11Registers[9]=vec4(0);
nativeWeatherPlate11Registers[10]=vec4(0);
nativeWeatherPlate11Registers[11]=vec4(0);
nativeWeatherPlate11Registers[12]=vec4(0);
nativeWeatherPlate11Registers[13]=vec4(0);
nativeWeatherPlate11Registers[14]=vec4(0);
nativeWeatherPlate11Registers[15]=vec4(0);
#ifdef H2_COMBINER_FOG
nativeWeatherPlate11Registers[3]=H2_COMBINER_FOG();
#endif
nativeWeatherPlate11Registers[4]=v0;nativeWeatherPlate11Registers[5]=v1;
nativeWeatherPlate11Registers[8]=textures[0];
nativeWeatherPlate11Registers[9]=textures[1];
nativeWeatherPlate11Registers[10]=textures[2];
nativeWeatherPlate11Registers[11]=textures[3];
nativeWeatherPlate11Registers[12].a=textures[0].a;
{
nativeWeatherPlate11Registers[1]=material.constants[0];
nativeWeatherPlate11Registers[2]=material.constants[1];
vec3 rrawab=vec3(dot(max(nativeWeatherPlate11Registers[8].rgb,vec3(0)),max(nativeWeatherPlate11Registers[1].rgb,vec3(0)))),rrawcd=vec3(dot(max(nativeWeatherPlate11Registers[8].rgb,vec3(0)),max(nativeWeatherPlate11Registers[2].rgb,vec3(0))));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate11Registers[0].b),vec3(0))*max(vec3(nativeWeatherPlate11Registers[0].b),vec3(0))),arawcd=(max(vec3(nativeWeatherPlate11Registers[0].b),vec3(0))*max(vec3(nativeWeatherPlate11Registers[0].b),vec3(0)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate11Registers[4].rgb=rab;
nativeWeatherPlate11Registers[5].rgb=rcd;
}
{
nativeWeatherPlate11Registers[1]=material.constants[2];
nativeWeatherPlate11Registers[2]=material.constants[3];
vec3 rrawab=(max(nativeWeatherPlate11Registers[4].rgb,vec3(0))*(vec3(1)-clamp(nativeWeatherPlate11Registers[0].rgb,0.0,1.0))),rrawcd=(max(nativeWeatherPlate11Registers[1].aaa,vec3(0))*(2.0*max(nativeWeatherPlate11Registers[0].rgb,vec3(0))-vec3(1)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate11Registers[4].b),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate11Registers[0].b),0.0,1.0))),arawcd=((max(vec3(nativeWeatherPlate11Registers[1].a),vec3(0))-vec3(0.5))*(2.0*max(vec3(nativeWeatherPlate11Registers[0].b),vec3(0))-vec3(1)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate11Registers[12].rgb=rsum;
nativeWeatherPlate11Registers[12].a=asum.x;
}
{
nativeWeatherPlate11Registers[1]=material.constants[4];
nativeWeatherPlate11Registers[2]=material.constants[5];
vec3 rrawab=(max(nativeWeatherPlate11Registers[0].rgb,vec3(0))*max(nativeWeatherPlate11Registers[0].rgb,vec3(0))),rrawcd=((vec3(1)-clamp(nativeWeatherPlate11Registers[0].rgb,0.0,1.0))*(vec3(1)-clamp(nativeWeatherPlate11Registers[0].rgb,0.0,1.0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp(((nativeWeatherPlate11Registers[12].a>=0.5)?rrawcd:rrawab),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate11Registers[8].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate11Registers[0].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate11Registers[1].a),vec3(0))*(2.0*max(vec3(nativeWeatherPlate11Registers[0].b),vec3(0))-vec3(1)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate11Registers[13].rgb=rsum;
nativeWeatherPlate11Registers[13].a=asum.x;
}
{
nativeWeatherPlate11Registers[1]=material.constants[6];
nativeWeatherPlate11Registers[2]=material.constants[7];
vec3 rrawab=(nativeWeatherPlate11Registers[12].rgb*(vec3(1)-clamp(nativeWeatherPlate11Registers[0].rgb,0.0,1.0))),rrawcd=((vec3(1)-clamp(nativeWeatherPlate11Registers[13].rgb,0.0,1.0))*(vec3(1)-clamp(nativeWeatherPlate11Registers[0].rgb,0.0,1.0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(vec3(nativeWeatherPlate11Registers[13].a)*(vec3(1)-clamp(vec3(nativeWeatherPlate11Registers[0].b),0.0,1.0))),arawcd=((vec3(1)-clamp(vec3(nativeWeatherPlate11Registers[13].b),0.0,1.0))*(-vec3(nativeWeatherPlate11Registers[1].a)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate11Registers[12].rgb=rsum;
nativeWeatherPlate11Registers[13].a=asum.x;
}
{
nativeWeatherPlate11Registers[1]=material.constants[8];
nativeWeatherPlate11Registers[2]=material.constants[9];
vec3 rrawab=vec3(dot(max(nativeWeatherPlate11Registers[9].aaa,vec3(0)),max(nativeWeatherPlate11Registers[1].rgb,vec3(0)))),rrawcd=vec3(dot(max(nativeWeatherPlate11Registers[10].aaa,vec3(0)),max(nativeWeatherPlate11Registers[2].rgb,vec3(0))));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate11Registers[13].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate11Registers[0].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate11Registers[13].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate11Registers[0].b),0.0,1.0)));
vec3 aab=clamp((arawab*4.0),-1.0,1.0),acd=clamp((arawcd*4.0),-1.0,1.0),asum=clamp(((arawab+arawcd)*4.0),-1.0,1.0);
nativeWeatherPlate11Registers[4].rgb=rab;
nativeWeatherPlate11Registers[5].rgb=rcd;
nativeWeatherPlate11Registers[13].a=asum.x;
}
{
nativeWeatherPlate11Registers[1]=material.constants[10];
nativeWeatherPlate11Registers[2]=material.constants[11];
vec3 rrawab=((vec3(1)-clamp(nativeWeatherPlate11Registers[4].rgb,0.0,1.0))*(vec3(1)-clamp(nativeWeatherPlate11Registers[5].rgb,0.0,1.0))),rrawcd=vec3(dot(max(nativeWeatherPlate11Registers[11].aaa,vec3(0)),max(nativeWeatherPlate11Registers[1].rgb,vec3(0))));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate11Registers[13].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate11Registers[0].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate11Registers[13].a),vec3(0))*max(vec3(nativeWeatherPlate11Registers[0].b),vec3(0)));
vec3 aab=clamp((arawab*2.0),-1.0,1.0),acd=clamp((arawcd*2.0),-1.0,1.0),asum=clamp(((arawab+arawcd)*2.0),-1.0,1.0);
nativeWeatherPlate11Registers[4].rgb=rab;
nativeWeatherPlate11Registers[5].rgb=rcd;
nativeWeatherPlate11Registers[13].a=asum.x;
}
{
nativeWeatherPlate11Registers[1]=material.constants[12];
nativeWeatherPlate11Registers[2]=material.constants[13];
vec3 rrawab=(max(nativeWeatherPlate11Registers[4].rgb,vec3(0))*(vec3(1)-clamp(nativeWeatherPlate11Registers[5].rgb,0.0,1.0))),rrawcd=(max(nativeWeatherPlate11Registers[0].rgb,vec3(0))*max(nativeWeatherPlate11Registers[0].rgb,vec3(0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate11Registers[13].a),vec3(0))*max(vec3(nativeWeatherPlate11Registers[2].a),vec3(0))),arawcd=(max(vec3(nativeWeatherPlate11Registers[12].b),vec3(0))*max(vec3(nativeWeatherPlate11Registers[2].b),vec3(0)));
vec3 aab=clamp((arawab*2.0),-1.0,1.0),acd=clamp((arawcd*2.0),-1.0,1.0),asum=clamp(((arawab+arawcd)*2.0),-1.0,1.0);
nativeWeatherPlate11Registers[13].rgb=rab;
nativeWeatherPlate11Registers[13].a=asum.x;
}
{
nativeWeatherPlate11Registers[1]=material.constants[14];
nativeWeatherPlate11Registers[2]=material.constants[15];
vec3 rrawab=(max(nativeWeatherPlate11Registers[0].rgb,vec3(0))*max(nativeWeatherPlate11Registers[0].rgb,vec3(0))),rrawcd=(max(nativeWeatherPlate11Registers[0].rgb,vec3(0))*max(nativeWeatherPlate11Registers[0].rgb,vec3(0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate11Registers[13].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate11Registers[13].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate11Registers[0].b),vec3(0))*max(vec3(nativeWeatherPlate11Registers[0].b),vec3(0)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate11Registers[13].a=aab.x;
}
nativeWeatherPlate11Registers[1]=material.constants[16];nativeWeatherPlate11Registers[2]=material.constants[17];
nativeWeatherPlate11Registers[14]=vec4((nativeWeatherPlate11Registers[5].rgb+nativeWeatherPlate11Registers[12].rgb),0);
nativeWeatherPlate11Registers[15]=vec4(max(nativeWeatherPlate11Registers[0].rgb,vec3(0))*max(nativeWeatherPlate11Registers[0].rgb,vec3(0)),0);
return clamp(vec4(max(nativeWeatherPlate11Registers[13].aaa,vec3(0))*max(nativeWeatherPlate11Registers[1].rgb,vec3(0))+(vec3(1)-max(nativeWeatherPlate11Registers[13].aaa,vec3(0)))*max(nativeWeatherPlate11Registers[0].rgb,vec3(0))+max(nativeWeatherPlate11Registers[0].rgb,vec3(0)),max(vec3(nativeWeatherPlate11Registers[13].a),vec3(0)).x),0.0,1.0);}

vec4 nativeWeatherPlate12Registers[16];
vec4 nativeWeatherPlate12Argb(uint c){return vec4((c>>16u)&255u,(c>>8u)&255u,c&255u,c>>24u)/255.0;}
vec4 nativeWeatherPlate12(vec4 textures[4],vec4 v0,vec4 v1){
nativeWeatherPlate12Registers[0]=vec4(0);
nativeWeatherPlate12Registers[1]=vec4(0);
nativeWeatherPlate12Registers[2]=vec4(0);
nativeWeatherPlate12Registers[3]=vec4(0);
nativeWeatherPlate12Registers[4]=vec4(0);
nativeWeatherPlate12Registers[5]=vec4(0);
nativeWeatherPlate12Registers[6]=vec4(0);
nativeWeatherPlate12Registers[7]=vec4(0);
nativeWeatherPlate12Registers[8]=vec4(0);
nativeWeatherPlate12Registers[9]=vec4(0);
nativeWeatherPlate12Registers[10]=vec4(0);
nativeWeatherPlate12Registers[11]=vec4(0);
nativeWeatherPlate12Registers[12]=vec4(0);
nativeWeatherPlate12Registers[13]=vec4(0);
nativeWeatherPlate12Registers[14]=vec4(0);
nativeWeatherPlate12Registers[15]=vec4(0);
#ifdef H2_COMBINER_FOG
nativeWeatherPlate12Registers[3]=H2_COMBINER_FOG();
#endif
nativeWeatherPlate12Registers[4]=v0;nativeWeatherPlate12Registers[5]=v1;
nativeWeatherPlate12Registers[8]=textures[0];
nativeWeatherPlate12Registers[9]=textures[1];
nativeWeatherPlate12Registers[10]=textures[2];
nativeWeatherPlate12Registers[11]=textures[3];
nativeWeatherPlate12Registers[12].a=textures[0].a;
{
nativeWeatherPlate12Registers[1]=material.constants[0];
nativeWeatherPlate12Registers[2]=material.constants[1];
vec3 rrawab=vec3(dot(max(nativeWeatherPlate12Registers[8].rgb,vec3(0)),max(nativeWeatherPlate12Registers[1].rgb,vec3(0)))),rrawcd=vec3(dot(max(nativeWeatherPlate12Registers[8].rgb,vec3(0)),max(nativeWeatherPlate12Registers[2].rgb,vec3(0))));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate12Registers[0].b),vec3(0))*max(vec3(nativeWeatherPlate12Registers[0].b),vec3(0))),arawcd=(max(vec3(nativeWeatherPlate12Registers[0].b),vec3(0))*max(vec3(nativeWeatherPlate12Registers[0].b),vec3(0)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate12Registers[4].rgb=rab;
nativeWeatherPlate12Registers[5].rgb=rcd;
}
{
nativeWeatherPlate12Registers[1]=material.constants[2];
nativeWeatherPlate12Registers[2]=material.constants[3];
vec3 rrawab=(max(nativeWeatherPlate12Registers[4].rgb,vec3(0))*(vec3(1)-clamp(nativeWeatherPlate12Registers[0].rgb,0.0,1.0))),rrawcd=(max(nativeWeatherPlate12Registers[1].aaa,vec3(0))*(2.0*max(nativeWeatherPlate12Registers[0].rgb,vec3(0))-vec3(1)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate12Registers[4].b),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate12Registers[0].b),0.0,1.0))),arawcd=((max(vec3(nativeWeatherPlate12Registers[1].a),vec3(0))-vec3(0.5))*(2.0*max(vec3(nativeWeatherPlate12Registers[0].b),vec3(0))-vec3(1)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate12Registers[12].rgb=rsum;
nativeWeatherPlate12Registers[12].a=asum.x;
}
{
nativeWeatherPlate12Registers[1]=material.constants[4];
nativeWeatherPlate12Registers[2]=material.constants[5];
vec3 rrawab=(max(nativeWeatherPlate12Registers[0].rgb,vec3(0))*max(nativeWeatherPlate12Registers[0].rgb,vec3(0))),rrawcd=((vec3(1)-clamp(nativeWeatherPlate12Registers[0].rgb,0.0,1.0))*(vec3(1)-clamp(nativeWeatherPlate12Registers[0].rgb,0.0,1.0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp(((nativeWeatherPlate12Registers[12].a>=0.5)?rrawcd:rrawab),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate12Registers[8].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate12Registers[0].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate12Registers[1].a),vec3(0))*(2.0*max(vec3(nativeWeatherPlate12Registers[0].b),vec3(0))-vec3(1)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate12Registers[13].rgb=rsum;
nativeWeatherPlate12Registers[13].a=asum.x;
}
{
nativeWeatherPlate12Registers[1]=material.constants[6];
nativeWeatherPlate12Registers[2]=material.constants[7];
vec3 rrawab=(nativeWeatherPlate12Registers[12].rgb*(vec3(1)-clamp(nativeWeatherPlate12Registers[0].rgb,0.0,1.0))),rrawcd=((vec3(1)-clamp(nativeWeatherPlate12Registers[13].rgb,0.0,1.0))*(vec3(1)-clamp(nativeWeatherPlate12Registers[0].rgb,0.0,1.0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(vec3(nativeWeatherPlate12Registers[13].a)*(vec3(1)-clamp(vec3(nativeWeatherPlate12Registers[0].b),0.0,1.0))),arawcd=((vec3(1)-clamp(vec3(nativeWeatherPlate12Registers[13].b),0.0,1.0))*(-vec3(nativeWeatherPlate12Registers[1].a)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate12Registers[12].rgb=rsum;
nativeWeatherPlate12Registers[13].a=asum.x;
}
{
nativeWeatherPlate12Registers[1]=material.constants[8];
nativeWeatherPlate12Registers[2]=material.constants[9];
vec3 rrawab=vec3(dot(max(nativeWeatherPlate12Registers[9].rgb,vec3(0)),max(nativeWeatherPlate12Registers[1].rgb,vec3(0)))),rrawcd=vec3(dot(max(nativeWeatherPlate12Registers[10].rgb,vec3(0)),max(nativeWeatherPlate12Registers[2].rgb,vec3(0))));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate12Registers[13].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate12Registers[0].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate12Registers[13].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate12Registers[0].b),0.0,1.0)));
vec3 aab=clamp((arawab*4.0),-1.0,1.0),acd=clamp((arawcd*4.0),-1.0,1.0),asum=clamp(((arawab+arawcd)*4.0),-1.0,1.0);
nativeWeatherPlate12Registers[4].rgb=rab;
nativeWeatherPlate12Registers[5].rgb=rcd;
nativeWeatherPlate12Registers[13].a=asum.x;
}
{
nativeWeatherPlate12Registers[1]=material.constants[10];
nativeWeatherPlate12Registers[2]=material.constants[11];
vec3 rrawab=((vec3(1)-clamp(nativeWeatherPlate12Registers[4].rgb,0.0,1.0))*(vec3(1)-clamp(nativeWeatherPlate12Registers[5].rgb,0.0,1.0))),rrawcd=vec3(dot(max(nativeWeatherPlate12Registers[11].rgb,vec3(0)),max(nativeWeatherPlate12Registers[1].rgb,vec3(0))));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate12Registers[13].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate12Registers[0].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate12Registers[13].a),vec3(0))*max(vec3(nativeWeatherPlate12Registers[0].b),vec3(0)));
vec3 aab=clamp((arawab*4.0),-1.0,1.0),acd=clamp((arawcd*4.0),-1.0,1.0),asum=clamp(((arawab+arawcd)*4.0),-1.0,1.0);
nativeWeatherPlate12Registers[4].rgb=rab;
nativeWeatherPlate12Registers[5].rgb=rcd;
nativeWeatherPlate12Registers[13].a=asum.x;
}
{
nativeWeatherPlate12Registers[1]=material.constants[12];
nativeWeatherPlate12Registers[2]=material.constants[13];
vec3 rrawab=(max(nativeWeatherPlate12Registers[4].rgb,vec3(0))*(vec3(1)-clamp(nativeWeatherPlate12Registers[5].rgb,0.0,1.0))),rrawcd=(max(nativeWeatherPlate12Registers[0].rgb,vec3(0))*max(nativeWeatherPlate12Registers[0].rgb,vec3(0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate12Registers[13].a),vec3(0))*max(vec3(nativeWeatherPlate12Registers[2].a),vec3(0))),arawcd=(max(vec3(nativeWeatherPlate12Registers[12].b),vec3(0))*max(vec3(nativeWeatherPlate12Registers[2].b),vec3(0)));
vec3 aab=clamp((arawab*2.0),-1.0,1.0),acd=clamp((arawcd*2.0),-1.0,1.0),asum=clamp(((arawab+arawcd)*2.0),-1.0,1.0);
nativeWeatherPlate12Registers[13].rgb=rab;
nativeWeatherPlate12Registers[13].a=asum.x;
}
{
nativeWeatherPlate12Registers[1]=material.constants[14];
nativeWeatherPlate12Registers[2]=material.constants[15];
vec3 rrawab=(max(nativeWeatherPlate12Registers[0].rgb,vec3(0))*max(nativeWeatherPlate12Registers[0].rgb,vec3(0))),rrawcd=(max(nativeWeatherPlate12Registers[0].rgb,vec3(0))*max(nativeWeatherPlate12Registers[0].rgb,vec3(0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate12Registers[13].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate12Registers[13].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate12Registers[0].b),vec3(0))*max(vec3(nativeWeatherPlate12Registers[0].b),vec3(0)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate12Registers[13].a=aab.x;
}
nativeWeatherPlate12Registers[1]=material.constants[16];nativeWeatherPlate12Registers[2]=material.constants[17];
nativeWeatherPlate12Registers[14]=vec4((nativeWeatherPlate12Registers[5].rgb+nativeWeatherPlate12Registers[12].rgb),0);
nativeWeatherPlate12Registers[15]=vec4(max(nativeWeatherPlate12Registers[0].rgb,vec3(0))*max(nativeWeatherPlate12Registers[0].rgb,vec3(0)),0);
return clamp(vec4(max(nativeWeatherPlate12Registers[13].aaa,vec3(0))*max(nativeWeatherPlate12Registers[1].rgb,vec3(0))+(vec3(1)-max(nativeWeatherPlate12Registers[13].aaa,vec3(0)))*max(nativeWeatherPlate12Registers[0].rgb,vec3(0))+max(nativeWeatherPlate12Registers[0].rgb,vec3(0)),max(vec3(nativeWeatherPlate12Registers[13].a),vec3(0)).x),0.0,1.0);}

vec4 nativeWeatherPlate13Registers[16];
vec4 nativeWeatherPlate13Argb(uint c){return vec4((c>>16u)&255u,(c>>8u)&255u,c&255u,c>>24u)/255.0;}
vec4 nativeWeatherPlate13(vec4 textures[4],vec4 v0,vec4 v1){
nativeWeatherPlate13Registers[0]=vec4(0);
nativeWeatherPlate13Registers[1]=vec4(0);
nativeWeatherPlate13Registers[2]=vec4(0);
nativeWeatherPlate13Registers[3]=vec4(0);
nativeWeatherPlate13Registers[4]=vec4(0);
nativeWeatherPlate13Registers[5]=vec4(0);
nativeWeatherPlate13Registers[6]=vec4(0);
nativeWeatherPlate13Registers[7]=vec4(0);
nativeWeatherPlate13Registers[8]=vec4(0);
nativeWeatherPlate13Registers[9]=vec4(0);
nativeWeatherPlate13Registers[10]=vec4(0);
nativeWeatherPlate13Registers[11]=vec4(0);
nativeWeatherPlate13Registers[12]=vec4(0);
nativeWeatherPlate13Registers[13]=vec4(0);
nativeWeatherPlate13Registers[14]=vec4(0);
nativeWeatherPlate13Registers[15]=vec4(0);
#ifdef H2_COMBINER_FOG
nativeWeatherPlate13Registers[3]=H2_COMBINER_FOG();
#endif
nativeWeatherPlate13Registers[4]=v0;nativeWeatherPlate13Registers[5]=v1;
nativeWeatherPlate13Registers[8]=textures[0];
nativeWeatherPlate13Registers[9]=textures[1];
nativeWeatherPlate13Registers[10]=textures[2];
nativeWeatherPlate13Registers[11]=textures[3];
nativeWeatherPlate13Registers[12].a=textures[0].a;
{
nativeWeatherPlate13Registers[1]=material.constants[0];
nativeWeatherPlate13Registers[2]=material.constants[1];
vec3 rrawab=vec3(dot(max(nativeWeatherPlate13Registers[8].rgb,vec3(0)),max(nativeWeatherPlate13Registers[1].rgb,vec3(0)))),rrawcd=vec3(dot(max(nativeWeatherPlate13Registers[8].rgb,vec3(0)),max(nativeWeatherPlate13Registers[2].rgb,vec3(0))));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate13Registers[0].b),vec3(0))*max(vec3(nativeWeatherPlate13Registers[0].b),vec3(0))),arawcd=(max(vec3(nativeWeatherPlate13Registers[0].b),vec3(0))*max(vec3(nativeWeatherPlate13Registers[0].b),vec3(0)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate13Registers[4].rgb=rab;
nativeWeatherPlate13Registers[5].rgb=rcd;
}
{
nativeWeatherPlate13Registers[1]=material.constants[2];
nativeWeatherPlate13Registers[2]=material.constants[3];
vec3 rrawab=(max(nativeWeatherPlate13Registers[4].rgb,vec3(0))*(vec3(1)-clamp(nativeWeatherPlate13Registers[0].rgb,0.0,1.0))),rrawcd=(max(nativeWeatherPlate13Registers[1].aaa,vec3(0))*(2.0*max(nativeWeatherPlate13Registers[0].rgb,vec3(0))-vec3(1)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate13Registers[4].b),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate13Registers[0].b),0.0,1.0))),arawcd=((max(vec3(nativeWeatherPlate13Registers[1].a),vec3(0))-vec3(0.5))*(2.0*max(vec3(nativeWeatherPlate13Registers[0].b),vec3(0))-vec3(1)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate13Registers[12].rgb=rsum;
nativeWeatherPlate13Registers[12].a=asum.x;
}
{
nativeWeatherPlate13Registers[1]=material.constants[4];
nativeWeatherPlate13Registers[2]=material.constants[5];
vec3 rrawab=(max(nativeWeatherPlate13Registers[0].rgb,vec3(0))*max(nativeWeatherPlate13Registers[0].rgb,vec3(0))),rrawcd=((vec3(1)-clamp(nativeWeatherPlate13Registers[0].rgb,0.0,1.0))*(vec3(1)-clamp(nativeWeatherPlate13Registers[0].rgb,0.0,1.0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp(((nativeWeatherPlate13Registers[12].a>=0.5)?rrawcd:rrawab),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate13Registers[8].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate13Registers[0].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate13Registers[1].a),vec3(0))*(2.0*max(vec3(nativeWeatherPlate13Registers[0].b),vec3(0))-vec3(1)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate13Registers[13].rgb=rsum;
nativeWeatherPlate13Registers[13].a=asum.x;
}
{
nativeWeatherPlate13Registers[1]=material.constants[6];
nativeWeatherPlate13Registers[2]=material.constants[7];
vec3 rrawab=(nativeWeatherPlate13Registers[12].rgb*(vec3(1)-clamp(nativeWeatherPlate13Registers[0].rgb,0.0,1.0))),rrawcd=((vec3(1)-clamp(nativeWeatherPlate13Registers[13].rgb,0.0,1.0))*(vec3(1)-clamp(nativeWeatherPlate13Registers[0].rgb,0.0,1.0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(vec3(nativeWeatherPlate13Registers[13].a)*(vec3(1)-clamp(vec3(nativeWeatherPlate13Registers[0].b),0.0,1.0))),arawcd=((vec3(1)-clamp(vec3(nativeWeatherPlate13Registers[13].b),0.0,1.0))*(-vec3(nativeWeatherPlate13Registers[1].a)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate13Registers[12].rgb=rsum;
nativeWeatherPlate13Registers[13].a=asum.x;
}
{
nativeWeatherPlate13Registers[1]=material.constants[8];
nativeWeatherPlate13Registers[2]=material.constants[9];
vec3 rrawab=vec3(dot(max(nativeWeatherPlate13Registers[9].aaa,vec3(0)),max(nativeWeatherPlate13Registers[1].rgb,vec3(0)))),rrawcd=vec3(dot(max(nativeWeatherPlate13Registers[10].aaa,vec3(0)),max(nativeWeatherPlate13Registers[2].rgb,vec3(0))));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate13Registers[13].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate13Registers[0].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate13Registers[13].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate13Registers[0].b),0.0,1.0)));
vec3 aab=clamp((arawab*4.0),-1.0,1.0),acd=clamp((arawcd*4.0),-1.0,1.0),asum=clamp(((arawab+arawcd)*4.0),-1.0,1.0);
nativeWeatherPlate13Registers[4].rgb=rab;
nativeWeatherPlate13Registers[5].rgb=rcd;
nativeWeatherPlate13Registers[13].a=asum.x;
}
{
nativeWeatherPlate13Registers[1]=material.constants[10];
nativeWeatherPlate13Registers[2]=material.constants[11];
vec3 rrawab=((vec3(1)-clamp(nativeWeatherPlate13Registers[4].rgb,0.0,1.0))*(vec3(1)-clamp(nativeWeatherPlate13Registers[5].rgb,0.0,1.0))),rrawcd=vec3(dot(max(nativeWeatherPlate13Registers[11].aaa,vec3(0)),max(nativeWeatherPlate13Registers[1].rgb,vec3(0))));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate13Registers[13].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate13Registers[0].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate13Registers[13].a),vec3(0))*max(vec3(nativeWeatherPlate13Registers[0].b),vec3(0)));
vec3 aab=clamp((arawab*4.0),-1.0,1.0),acd=clamp((arawcd*4.0),-1.0,1.0),asum=clamp(((arawab+arawcd)*4.0),-1.0,1.0);
nativeWeatherPlate13Registers[4].rgb=rab;
nativeWeatherPlate13Registers[5].rgb=rcd;
nativeWeatherPlate13Registers[13].a=asum.x;
}
{
nativeWeatherPlate13Registers[1]=material.constants[12];
nativeWeatherPlate13Registers[2]=material.constants[13];
vec3 rrawab=(max(nativeWeatherPlate13Registers[4].rgb,vec3(0))*(vec3(1)-clamp(nativeWeatherPlate13Registers[5].rgb,0.0,1.0))),rrawcd=(max(nativeWeatherPlate13Registers[0].rgb,vec3(0))*max(nativeWeatherPlate13Registers[0].rgb,vec3(0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate13Registers[13].a),vec3(0))*max(vec3(nativeWeatherPlate13Registers[2].a),vec3(0))),arawcd=(max(vec3(nativeWeatherPlate13Registers[12].b),vec3(0))*max(vec3(nativeWeatherPlate13Registers[2].b),vec3(0)));
vec3 aab=clamp((arawab*2.0),-1.0,1.0),acd=clamp((arawcd*2.0),-1.0,1.0),asum=clamp(((arawab+arawcd)*2.0),-1.0,1.0);
nativeWeatherPlate13Registers[13].rgb=rab;
nativeWeatherPlate13Registers[13].a=asum.x;
}
{
nativeWeatherPlate13Registers[1]=material.constants[14];
nativeWeatherPlate13Registers[2]=material.constants[15];
vec3 rrawab=(max(nativeWeatherPlate13Registers[0].rgb,vec3(0))*max(nativeWeatherPlate13Registers[0].rgb,vec3(0))),rrawcd=(max(nativeWeatherPlate13Registers[0].rgb,vec3(0))*max(nativeWeatherPlate13Registers[0].rgb,vec3(0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate13Registers[13].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate13Registers[13].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate13Registers[0].b),vec3(0))*max(vec3(nativeWeatherPlate13Registers[0].b),vec3(0)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate13Registers[13].a=aab.x;
}
nativeWeatherPlate13Registers[1]=material.constants[16];nativeWeatherPlate13Registers[2]=material.constants[17];
nativeWeatherPlate13Registers[14]=vec4((nativeWeatherPlate13Registers[5].rgb+nativeWeatherPlate13Registers[12].rgb),0);
nativeWeatherPlate13Registers[15]=vec4(max(nativeWeatherPlate13Registers[0].rgb,vec3(0))*max(nativeWeatherPlate13Registers[0].rgb,vec3(0)),0);
return clamp(vec4(max(nativeWeatherPlate13Registers[13].aaa,vec3(0))*max(nativeWeatherPlate13Registers[1].rgb,vec3(0))+(vec3(1)-max(nativeWeatherPlate13Registers[13].aaa,vec3(0)))*max(nativeWeatherPlate13Registers[0].rgb,vec3(0))+max(nativeWeatherPlate13Registers[0].rgb,vec3(0)),max(vec3(nativeWeatherPlate13Registers[13].a),vec3(0)).x),0.0,1.0);}

vec4 nativeWeatherPlate14Registers[16];
vec4 nativeWeatherPlate14Argb(uint c){return vec4((c>>16u)&255u,(c>>8u)&255u,c&255u,c>>24u)/255.0;}
vec4 nativeWeatherPlate14(vec4 textures[4],vec4 v0,vec4 v1){
nativeWeatherPlate14Registers[0]=vec4(0);
nativeWeatherPlate14Registers[1]=vec4(0);
nativeWeatherPlate14Registers[2]=vec4(0);
nativeWeatherPlate14Registers[3]=vec4(0);
nativeWeatherPlate14Registers[4]=vec4(0);
nativeWeatherPlate14Registers[5]=vec4(0);
nativeWeatherPlate14Registers[6]=vec4(0);
nativeWeatherPlate14Registers[7]=vec4(0);
nativeWeatherPlate14Registers[8]=vec4(0);
nativeWeatherPlate14Registers[9]=vec4(0);
nativeWeatherPlate14Registers[10]=vec4(0);
nativeWeatherPlate14Registers[11]=vec4(0);
nativeWeatherPlate14Registers[12]=vec4(0);
nativeWeatherPlate14Registers[13]=vec4(0);
nativeWeatherPlate14Registers[14]=vec4(0);
nativeWeatherPlate14Registers[15]=vec4(0);
#ifdef H2_COMBINER_FOG
nativeWeatherPlate14Registers[3]=H2_COMBINER_FOG();
#endif
nativeWeatherPlate14Registers[4]=v0;nativeWeatherPlate14Registers[5]=v1;
nativeWeatherPlate14Registers[8]=textures[0];
nativeWeatherPlate14Registers[9]=textures[1];
nativeWeatherPlate14Registers[10]=textures[2];
nativeWeatherPlate14Registers[11]=textures[3];
nativeWeatherPlate14Registers[12].a=textures[0].a;
{
nativeWeatherPlate14Registers[1]=material.constants[0];
nativeWeatherPlate14Registers[2]=material.constants[1];
vec3 rrawab=vec3(dot(max(nativeWeatherPlate14Registers[8].rgb,vec3(0)),max(nativeWeatherPlate14Registers[1].rgb,vec3(0)))),rrawcd=vec3(dot(max(nativeWeatherPlate14Registers[8].rgb,vec3(0)),max(nativeWeatherPlate14Registers[2].rgb,vec3(0))));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate14Registers[0].b),vec3(0))*max(vec3(nativeWeatherPlate14Registers[0].b),vec3(0))),arawcd=(max(vec3(nativeWeatherPlate14Registers[0].b),vec3(0))*max(vec3(nativeWeatherPlate14Registers[0].b),vec3(0)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate14Registers[4].rgb=rab;
nativeWeatherPlate14Registers[5].rgb=rcd;
}
{
nativeWeatherPlate14Registers[1]=material.constants[2];
nativeWeatherPlate14Registers[2]=material.constants[3];
vec3 rrawab=(max(nativeWeatherPlate14Registers[4].rgb,vec3(0))*(vec3(1)-clamp(nativeWeatherPlate14Registers[0].rgb,0.0,1.0))),rrawcd=(max(nativeWeatherPlate14Registers[1].aaa,vec3(0))*(2.0*max(nativeWeatherPlate14Registers[0].rgb,vec3(0))-vec3(1)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate14Registers[4].b),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate14Registers[0].b),0.0,1.0))),arawcd=((max(vec3(nativeWeatherPlate14Registers[1].a),vec3(0))-vec3(0.5))*(2.0*max(vec3(nativeWeatherPlate14Registers[0].b),vec3(0))-vec3(1)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate14Registers[12].rgb=rsum;
nativeWeatherPlate14Registers[12].a=asum.x;
}
{
nativeWeatherPlate14Registers[1]=material.constants[4];
nativeWeatherPlate14Registers[2]=material.constants[5];
vec3 rrawab=(max(nativeWeatherPlate14Registers[0].rgb,vec3(0))*max(nativeWeatherPlate14Registers[0].rgb,vec3(0))),rrawcd=((vec3(1)-clamp(nativeWeatherPlate14Registers[0].rgb,0.0,1.0))*(vec3(1)-clamp(nativeWeatherPlate14Registers[0].rgb,0.0,1.0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp(((nativeWeatherPlate14Registers[12].a>=0.5)?rrawcd:rrawab),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate14Registers[8].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate14Registers[0].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate14Registers[1].a),vec3(0))*(2.0*max(vec3(nativeWeatherPlate14Registers[0].b),vec3(0))-vec3(1)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate14Registers[13].rgb=rsum;
nativeWeatherPlate14Registers[13].a=asum.x;
}
{
nativeWeatherPlate14Registers[1]=material.constants[6];
nativeWeatherPlate14Registers[2]=material.constants[7];
vec3 rrawab=(nativeWeatherPlate14Registers[12].rgb*(vec3(1)-clamp(nativeWeatherPlate14Registers[0].rgb,0.0,1.0))),rrawcd=((vec3(1)-clamp(nativeWeatherPlate14Registers[13].rgb,0.0,1.0))*(vec3(1)-clamp(nativeWeatherPlate14Registers[0].rgb,0.0,1.0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(vec3(nativeWeatherPlate14Registers[13].a)*(vec3(1)-clamp(vec3(nativeWeatherPlate14Registers[0].b),0.0,1.0))),arawcd=((vec3(1)-clamp(vec3(nativeWeatherPlate14Registers[13].b),0.0,1.0))*(-vec3(nativeWeatherPlate14Registers[1].a)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate14Registers[12].rgb=rsum;
nativeWeatherPlate14Registers[13].a=asum.x;
}
{
nativeWeatherPlate14Registers[1]=material.constants[8];
nativeWeatherPlate14Registers[2]=material.constants[9];
vec3 rrawab=vec3(dot(max(nativeWeatherPlate14Registers[9].rgb,vec3(0)),max(nativeWeatherPlate14Registers[1].rgb,vec3(0)))),rrawcd=vec3(dot(max(nativeWeatherPlate14Registers[10].rgb,vec3(0)),max(nativeWeatherPlate14Registers[2].rgb,vec3(0))));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate14Registers[13].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate14Registers[0].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate14Registers[13].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate14Registers[0].b),0.0,1.0)));
vec3 aab=clamp((arawab*4.0),-1.0,1.0),acd=clamp((arawcd*4.0),-1.0,1.0),asum=clamp(((arawab+arawcd)*4.0),-1.0,1.0);
nativeWeatherPlate14Registers[4].rgb=rab;
nativeWeatherPlate14Registers[5].rgb=rcd;
nativeWeatherPlate14Registers[13].a=asum.x;
}
{
nativeWeatherPlate14Registers[1]=material.constants[10];
nativeWeatherPlate14Registers[2]=material.constants[11];
vec3 rrawab=((vec3(1)-clamp(nativeWeatherPlate14Registers[4].rgb,0.0,1.0))*(vec3(1)-clamp(nativeWeatherPlate14Registers[5].rgb,0.0,1.0))),rrawcd=vec3(dot(max(nativeWeatherPlate14Registers[11].rgb,vec3(0)),max(nativeWeatherPlate14Registers[1].rgb,vec3(0))));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate14Registers[13].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate14Registers[0].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate14Registers[13].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate14Registers[0].b),0.0,1.0)));
vec3 aab=clamp((arawab*4.0),-1.0,1.0),acd=clamp((arawcd*4.0),-1.0,1.0),asum=clamp(((arawab+arawcd)*4.0),-1.0,1.0);
nativeWeatherPlate14Registers[4].rgb=rab;
nativeWeatherPlate14Registers[5].rgb=rcd;
nativeWeatherPlate14Registers[13].a=asum.x;
}
{
nativeWeatherPlate14Registers[1]=material.constants[12];
nativeWeatherPlate14Registers[2]=material.constants[13];
vec3 rrawab=(max(nativeWeatherPlate14Registers[4].rgb,vec3(0))*(vec3(1)-clamp(nativeWeatherPlate14Registers[5].rgb,0.0,1.0))),rrawcd=(max(nativeWeatherPlate14Registers[0].rgb,vec3(0))*max(nativeWeatherPlate14Registers[0].rgb,vec3(0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate14Registers[13].a),vec3(0))*max(vec3(nativeWeatherPlate14Registers[2].a),vec3(0))),arawcd=(max(vec3(nativeWeatherPlate14Registers[12].b),vec3(0))*max(vec3(nativeWeatherPlate14Registers[2].b),vec3(0)));
vec3 aab=clamp((arawab*2.0),-1.0,1.0),acd=clamp((arawcd*2.0),-1.0,1.0),asum=clamp(((arawab+arawcd)*2.0),-1.0,1.0);
nativeWeatherPlate14Registers[13].rgb=rab;
nativeWeatherPlate14Registers[13].a=asum.x;
}
{
nativeWeatherPlate14Registers[1]=material.constants[14];
nativeWeatherPlate14Registers[2]=material.constants[15];
vec3 rrawab=(max(nativeWeatherPlate14Registers[0].rgb,vec3(0))*max(nativeWeatherPlate14Registers[0].rgb,vec3(0))),rrawcd=(max(nativeWeatherPlate14Registers[0].rgb,vec3(0))*max(nativeWeatherPlate14Registers[0].rgb,vec3(0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate14Registers[13].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate14Registers[13].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate14Registers[0].b),vec3(0))*max(vec3(nativeWeatherPlate14Registers[0].b),vec3(0)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate14Registers[13].a=aab.x;
}
nativeWeatherPlate14Registers[1]=material.constants[16];nativeWeatherPlate14Registers[2]=material.constants[17];
nativeWeatherPlate14Registers[14]=vec4((nativeWeatherPlate14Registers[5].rgb+nativeWeatherPlate14Registers[12].rgb),0);
nativeWeatherPlate14Registers[15]=vec4(max(nativeWeatherPlate14Registers[0].rgb,vec3(0))*max(nativeWeatherPlate14Registers[0].rgb,vec3(0)),0);
return clamp(vec4(max(nativeWeatherPlate14Registers[13].aaa,vec3(0))*max(nativeWeatherPlate14Registers[1].rgb,vec3(0))+(vec3(1)-max(nativeWeatherPlate14Registers[13].aaa,vec3(0)))*max(nativeWeatherPlate14Registers[0].rgb,vec3(0))+max(nativeWeatherPlate14Registers[0].rgb,vec3(0)),max(vec3(nativeWeatherPlate14Registers[13].a),vec3(0)).x),0.0,1.0);}

vec4 nativeWeatherPlate15Registers[16];
vec4 nativeWeatherPlate15Argb(uint c){return vec4((c>>16u)&255u,(c>>8u)&255u,c&255u,c>>24u)/255.0;}
vec4 nativeWeatherPlate15(vec4 textures[4],vec4 v0,vec4 v1){
nativeWeatherPlate15Registers[0]=vec4(0);
nativeWeatherPlate15Registers[1]=vec4(0);
nativeWeatherPlate15Registers[2]=vec4(0);
nativeWeatherPlate15Registers[3]=vec4(0);
nativeWeatherPlate15Registers[4]=vec4(0);
nativeWeatherPlate15Registers[5]=vec4(0);
nativeWeatherPlate15Registers[6]=vec4(0);
nativeWeatherPlate15Registers[7]=vec4(0);
nativeWeatherPlate15Registers[8]=vec4(0);
nativeWeatherPlate15Registers[9]=vec4(0);
nativeWeatherPlate15Registers[10]=vec4(0);
nativeWeatherPlate15Registers[11]=vec4(0);
nativeWeatherPlate15Registers[12]=vec4(0);
nativeWeatherPlate15Registers[13]=vec4(0);
nativeWeatherPlate15Registers[14]=vec4(0);
nativeWeatherPlate15Registers[15]=vec4(0);
#ifdef H2_COMBINER_FOG
nativeWeatherPlate15Registers[3]=H2_COMBINER_FOG();
#endif
nativeWeatherPlate15Registers[4]=v0;nativeWeatherPlate15Registers[5]=v1;
nativeWeatherPlate15Registers[8]=textures[0];
nativeWeatherPlate15Registers[9]=textures[1];
nativeWeatherPlate15Registers[10]=textures[2];
nativeWeatherPlate15Registers[11]=textures[3];
nativeWeatherPlate15Registers[12].a=textures[0].a;
{
nativeWeatherPlate15Registers[1]=material.constants[0];
nativeWeatherPlate15Registers[2]=material.constants[1];
vec3 rrawab=vec3(dot(max(nativeWeatherPlate15Registers[8].rgb,vec3(0)),max(nativeWeatherPlate15Registers[1].rgb,vec3(0)))),rrawcd=vec3(dot(max(nativeWeatherPlate15Registers[8].rgb,vec3(0)),max(nativeWeatherPlate15Registers[2].rgb,vec3(0))));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate15Registers[0].b),vec3(0))*max(vec3(nativeWeatherPlate15Registers[0].b),vec3(0))),arawcd=(max(vec3(nativeWeatherPlate15Registers[0].b),vec3(0))*max(vec3(nativeWeatherPlate15Registers[0].b),vec3(0)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate15Registers[4].rgb=rab;
nativeWeatherPlate15Registers[5].rgb=rcd;
}
{
nativeWeatherPlate15Registers[1]=material.constants[2];
nativeWeatherPlate15Registers[2]=material.constants[3];
vec3 rrawab=(max(nativeWeatherPlate15Registers[4].rgb,vec3(0))*(vec3(1)-clamp(nativeWeatherPlate15Registers[0].rgb,0.0,1.0))),rrawcd=(max(nativeWeatherPlate15Registers[1].aaa,vec3(0))*(2.0*max(nativeWeatherPlate15Registers[0].rgb,vec3(0))-vec3(1)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate15Registers[4].b),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate15Registers[0].b),0.0,1.0))),arawcd=((max(vec3(nativeWeatherPlate15Registers[1].a),vec3(0))-vec3(0.5))*(2.0*max(vec3(nativeWeatherPlate15Registers[0].b),vec3(0))-vec3(1)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate15Registers[12].rgb=rsum;
nativeWeatherPlate15Registers[12].a=asum.x;
}
{
nativeWeatherPlate15Registers[1]=material.constants[4];
nativeWeatherPlate15Registers[2]=material.constants[5];
vec3 rrawab=(max(nativeWeatherPlate15Registers[0].rgb,vec3(0))*max(nativeWeatherPlate15Registers[0].rgb,vec3(0))),rrawcd=((vec3(1)-clamp(nativeWeatherPlate15Registers[0].rgb,0.0,1.0))*(vec3(1)-clamp(nativeWeatherPlate15Registers[0].rgb,0.0,1.0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp(((nativeWeatherPlate15Registers[12].a>=0.5)?rrawcd:rrawab),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate15Registers[8].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate15Registers[0].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate15Registers[1].a),vec3(0))*(2.0*max(vec3(nativeWeatherPlate15Registers[0].b),vec3(0))-vec3(1)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate15Registers[13].rgb=rsum;
nativeWeatherPlate15Registers[13].a=asum.x;
}
{
nativeWeatherPlate15Registers[1]=material.constants[6];
nativeWeatherPlate15Registers[2]=material.constants[7];
vec3 rrawab=(nativeWeatherPlate15Registers[12].rgb*(vec3(1)-clamp(nativeWeatherPlate15Registers[0].rgb,0.0,1.0))),rrawcd=((vec3(1)-clamp(nativeWeatherPlate15Registers[13].rgb,0.0,1.0))*(vec3(1)-clamp(nativeWeatherPlate15Registers[0].rgb,0.0,1.0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(vec3(nativeWeatherPlate15Registers[13].a)*(vec3(1)-clamp(vec3(nativeWeatherPlate15Registers[0].b),0.0,1.0))),arawcd=((vec3(1)-clamp(vec3(nativeWeatherPlate15Registers[13].b),0.0,1.0))*(-vec3(nativeWeatherPlate15Registers[1].a)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate15Registers[12].rgb=rsum;
nativeWeatherPlate15Registers[13].a=asum.x;
}
{
nativeWeatherPlate15Registers[1]=material.constants[8];
nativeWeatherPlate15Registers[2]=material.constants[9];
vec3 rrawab=vec3(dot(max(nativeWeatherPlate15Registers[9].aaa,vec3(0)),max(nativeWeatherPlate15Registers[1].rgb,vec3(0)))),rrawcd=vec3(dot(max(nativeWeatherPlate15Registers[10].aaa,vec3(0)),max(nativeWeatherPlate15Registers[2].rgb,vec3(0))));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate15Registers[13].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate15Registers[0].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate15Registers[13].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate15Registers[0].b),0.0,1.0)));
vec3 aab=clamp((arawab*4.0),-1.0,1.0),acd=clamp((arawcd*4.0),-1.0,1.0),asum=clamp(((arawab+arawcd)*4.0),-1.0,1.0);
nativeWeatherPlate15Registers[4].rgb=rab;
nativeWeatherPlate15Registers[5].rgb=rcd;
nativeWeatherPlate15Registers[13].a=asum.x;
}
{
nativeWeatherPlate15Registers[1]=material.constants[10];
nativeWeatherPlate15Registers[2]=material.constants[11];
vec3 rrawab=((vec3(1)-clamp(nativeWeatherPlate15Registers[4].rgb,0.0,1.0))*(vec3(1)-clamp(nativeWeatherPlate15Registers[5].rgb,0.0,1.0))),rrawcd=vec3(dot(max(nativeWeatherPlate15Registers[11].aaa,vec3(0)),max(nativeWeatherPlate15Registers[1].rgb,vec3(0))));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate15Registers[13].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate15Registers[0].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate15Registers[13].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate15Registers[0].b),0.0,1.0)));
vec3 aab=clamp((arawab*4.0),-1.0,1.0),acd=clamp((arawcd*4.0),-1.0,1.0),asum=clamp(((arawab+arawcd)*4.0),-1.0,1.0);
nativeWeatherPlate15Registers[4].rgb=rab;
nativeWeatherPlate15Registers[5].rgb=rcd;
nativeWeatherPlate15Registers[13].a=asum.x;
}
{
nativeWeatherPlate15Registers[1]=material.constants[12];
nativeWeatherPlate15Registers[2]=material.constants[13];
vec3 rrawab=(max(nativeWeatherPlate15Registers[4].rgb,vec3(0))*(vec3(1)-clamp(nativeWeatherPlate15Registers[5].rgb,0.0,1.0))),rrawcd=(max(nativeWeatherPlate15Registers[0].rgb,vec3(0))*max(nativeWeatherPlate15Registers[0].rgb,vec3(0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate15Registers[13].a),vec3(0))*max(vec3(nativeWeatherPlate15Registers[2].a),vec3(0))),arawcd=(max(vec3(nativeWeatherPlate15Registers[12].b),vec3(0))*max(vec3(nativeWeatherPlate15Registers[2].b),vec3(0)));
vec3 aab=clamp((arawab*2.0),-1.0,1.0),acd=clamp((arawcd*2.0),-1.0,1.0),asum=clamp(((arawab+arawcd)*2.0),-1.0,1.0);
nativeWeatherPlate15Registers[13].rgb=rab;
nativeWeatherPlate15Registers[13].a=asum.x;
}
{
nativeWeatherPlate15Registers[1]=material.constants[14];
nativeWeatherPlate15Registers[2]=material.constants[15];
vec3 rrawab=(max(nativeWeatherPlate15Registers[0].rgb,vec3(0))*max(nativeWeatherPlate15Registers[0].rgb,vec3(0))),rrawcd=(max(nativeWeatherPlate15Registers[0].rgb,vec3(0))*max(nativeWeatherPlate15Registers[0].rgb,vec3(0)));
vec3 rab=clamp(rrawab,-1.0,1.0),rcd=clamp(rrawcd,-1.0,1.0),rsum=clamp((rrawab+rrawcd),-1.0,1.0);
vec3 arawab=(max(vec3(nativeWeatherPlate15Registers[13].a),vec3(0))*(vec3(1)-clamp(vec3(nativeWeatherPlate15Registers[13].b),0.0,1.0))),arawcd=(max(vec3(nativeWeatherPlate15Registers[0].b),vec3(0))*max(vec3(nativeWeatherPlate15Registers[0].b),vec3(0)));
vec3 aab=clamp(arawab,-1.0,1.0),acd=clamp(arawcd,-1.0,1.0),asum=clamp((arawab+arawcd),-1.0,1.0);
nativeWeatherPlate15Registers[13].a=aab.x;
}
nativeWeatherPlate15Registers[1]=material.constants[16];nativeWeatherPlate15Registers[2]=material.constants[17];
nativeWeatherPlate15Registers[14]=vec4((nativeWeatherPlate15Registers[5].rgb+nativeWeatherPlate15Registers[12].rgb),0);
nativeWeatherPlate15Registers[15]=vec4(max(nativeWeatherPlate15Registers[0].rgb,vec3(0))*max(nativeWeatherPlate15Registers[0].rgb,vec3(0)),0);
return clamp(vec4(max(nativeWeatherPlate15Registers[13].aaa,vec3(0))*max(nativeWeatherPlate15Registers[1].rgb,vec3(0))+(vec3(1)-max(nativeWeatherPlate15Registers[13].aaa,vec3(0)))*max(nativeWeatherPlate15Registers[0].rgb,vec3(0))+max(nativeWeatherPlate15Registers[0].rgb,vec3(0)),max(vec3(nativeWeatherPlate15Registers[13].a),vec3(0)).x),0.0,1.0);}

vec4 runCombiners(vec4 textures[4],vec4 v0,vec4 v1){
if(nativeWeatherPlateVariant==0u)return nativeWeatherPlate0(textures,v0,v1);
if(nativeWeatherPlateVariant==1u)return nativeWeatherPlate1(textures,v0,v1);
if(nativeWeatherPlateVariant==2u)return nativeWeatherPlate2(textures,v0,v1);
if(nativeWeatherPlateVariant==3u)return nativeWeatherPlate3(textures,v0,v1);
if(nativeWeatherPlateVariant==4u)return nativeWeatherPlate4(textures,v0,v1);
if(nativeWeatherPlateVariant==5u)return nativeWeatherPlate5(textures,v0,v1);
if(nativeWeatherPlateVariant==6u)return nativeWeatherPlate6(textures,v0,v1);
if(nativeWeatherPlateVariant==7u)return nativeWeatherPlate7(textures,v0,v1);
if(nativeWeatherPlateVariant==8u)return nativeWeatherPlate8(textures,v0,v1);
if(nativeWeatherPlateVariant==9u)return nativeWeatherPlate9(textures,v0,v1);
if(nativeWeatherPlateVariant==10u)return nativeWeatherPlate10(textures,v0,v1);
if(nativeWeatherPlateVariant==11u)return nativeWeatherPlate11(textures,v0,v1);
if(nativeWeatherPlateVariant==12u)return nativeWeatherPlate12(textures,v0,v1);
if(nativeWeatherPlateVariant==13u)return nativeWeatherPlate13(textures,v0,v1);
if(nativeWeatherPlateVariant==14u)return nativeWeatherPlate14(textures,v0,v1);
if(nativeWeatherPlateVariant==15u)return nativeWeatherPlate15(textures,v0,v1);
return vec4(0);}
