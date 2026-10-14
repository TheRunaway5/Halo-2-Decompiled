vec4 r[13],v[16],outv[16];int address;
vec4 constant(int index){return particleConstants[index];}
vec4 multiply(vec4 a,vec4 b) {
 vec4 az=mix(a,vec4(1),isnan(a)),bz=mix(b,vec4(1),isnan(b));
 vec4 zeros=sign(az)*sign(bz),ret=a*b;
 if(zeros.x==0)ret.x=0;if(zeros.y==0)ret.y=0;
 if(zeros.z==0)ret.z=0;if(zeros.w==0)ret.w=0;
 return ret;
}
float clampAwayZeroInf(float t) {
 return t>0 || floatBitsToUint(t)==0u?
 clamp(t,uintBitsToFloat(528482304u),uintBitsToFloat(1602224128u)):
 clamp(t,uintBitsToFloat(3749707776u),uintBitsToFloat(2675965952u));
}
void particleProgram() {
  for(int i=0;i<13;++i)r[i]=vec4(0);
  for(int i=0;i<16;++i){outv[i]=vec4(0,0,0,1);v[i]=particleAttributes[i];}
  address=0;
  { // original instruction 0
    vec4 a=(v[0]).xyzw,b=vec4(0),c=vec4(0);
    vec4 m=a,l=vec4(0);
    r[2].xyz=m.xyz;
  }
  { // original instruction 1
    vec4 a=(v[1]).xyyy,b=(constant(18)).xyyy,c=(constant(18)).zwww;
    vec4 m=multiply(a,b)+c,l=vec4(0);
    outv[9].xy=m.xy;
  }
  { // original instruction 2
    vec4 a=(r[2]).xyzw,b=(v[12]).xyzw,c=vec4(0);
    vec4 m=vec4(dot(vec4(a.xyz,1),b)),l=vec4(0);
    r[3].x=m.x;
  }
  { // original instruction 3
    vec4 a=(r[2]).xyzw,b=(v[13]).xyzw,c=vec4(0);
    vec4 m=vec4(dot(vec4(a.xyz,1),b)),l=vec4(0);
    r[3].y=m.y;
  }
  { // original instruction 4
    vec4 a=(r[2]).xyzw,b=(v[14]).xyzw,c=vec4(0);
    vec4 m=vec4(dot(vec4(a.xyz,1),b)),l=vec4(0);
    r[3].z=m.z;
  }
  { // original instruction 5
    vec4 a=(v[1]).xyyy,b=(constant(19)).xyyy,c=(constant(19)).zwww;
    vec4 m=multiply(a,b)+c,l=vec4(0);
    outv[10].xy=m.xy;
  }
  { // original instruction 6
    vec4 a=(r[3]).xyzw,b=(constant(0)).xyzw,c=vec4(0);
    vec4 m=vec4(dot(vec4(a.xyz,1),b)),l=vec4(0);
    outv[0].x=m.x;
  }
  { // original instruction 7
    vec4 a=(r[3]).xyzw,b=(constant(1)).xyzw,c=vec4(0);
    vec4 m=vec4(dot(vec4(a.xyz,1),b)),l=vec4(0);
    outv[0].y=m.y;
  }
  { // original instruction 8
    vec4 a=(r[3]).xyzw,b=(constant(2)).xyzw,c=vec4(0);
    vec4 m=vec4(dot(vec4(a.xyz,1),b)),l=vec4(0);
    outv[0].z=m.z;
  }
  { // original instruction 9
    vec4 a=(r[3]).xyzw,b=(constant(3)).xyzw,c=vec4(0);
    vec4 m=vec4(dot(vec4(a.xyz,1),b)),l=vec4(0);
    outv[0].w=m.w;
  }
  { // original instruction 10
    vec4 a=(v[1]).xyyy,b=(constant(20)).xyyy,c=(constant(20)).zwww;
    vec4 m=multiply(a,b)+c,l=vec4(0);
    outv[11].xy=m.xy;
  }
  { // original instruction 11
    vec4 a=(v[1]).xyyy,b=(constant(21)).xyyy,c=(constant(21)).zwww;
    vec4 m=multiply(a,b)+c,l=vec4(0);
    outv[12].xy=m.xy;
  }
  { // original instruction 12
    vec4 a=(v[2]).xyzw,b=vec4(0),c=vec4(0);
    vec4 m=a,l=vec4(0);
    outv[3].xyzw=m.xyzw;
  }
  { // original instruction 13
    vec4 a=(outv[0]).xyzw,b=(constant(10)).xyzw,c=(outv[0]).wwww;
    vec4 m=multiply(a,b),l=vec4(clampAwayZeroInf(1.0/c.x));
    r[1].w=l.w;
    outv[0].xyz=m.xyz;
  }
  { // original instruction 14
    vec4 a=(outv[0]).xyzw,b=(r[1]).wwww,c=(constant(11)).xyzz;
    vec4 m=multiply(a,b)+c,l=vec4(0);
    outv[0].xyz=m.xyz;
  }
}
