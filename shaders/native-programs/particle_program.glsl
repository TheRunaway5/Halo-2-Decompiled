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
  v[0]=particleCorner;
  { // original instruction 0
    vec4 a=(constant(148)).wwww,b=(v[5]).yyyy,c=vec4(0);
    vec4 m=multiply(a,b),l=vec4(0);
    r[9].x=m.x;
  }
  { // original instruction 1
    vec4 a=(v[0]).xyzz,b=vec4(0),c=vec4(0);
    vec4 m=a,l=vec4(0);
    r[10].xyz=m.xyz;
  }
  { // original instruction 2
    vec4 a=(v[4]).xyzw,b=(constant(174)).xyzw,c=(r[9]).xxxx;
    vec4 m=vec4(dot(vec4(a.xyz,1),b)),l=vec4(exp2(floor(c.x)),c.x-floor(c.x),exp2(c.x),1);
    r[0].x=m.x;
    r[1].y=l.y;
  }
  { // original instruction 3
    vec4 a=(r[10]).xyyy,b=(v[6]).zwww,c=(v[6]).xyyy;
    vec4 m=multiply(a,b)+c,l=vec4(0);
    r[10].xy=m.xy;
  }
  { // original instruction 4
    vec4 a=(constant(148)).yyyy,b=(r[1]).yyyy,c=(constant(148)).xxxx;
    vec4 m=multiply(a,b)+c,l=vec4(0);
    r[9].w=m.w;
  }
  { // original instruction 5
    vec4 a=(r[10]).xyyy,b=(v[5]).xxxx,c=vec4(0);
    vec4 m=multiply(a,b),l=vec4(0);
    r[10].xy=m.xy;
  }
  { // original instruction 6
    vec4 a=(r[9]).wwww,b=vec4(0),c=(r[9]).wwww;
    vec4 m=vec4(0),l=vec4(exp2(floor(c.x)),c.x-floor(c.x),exp2(c.x),1);
    address=int(floor(a.x+0.001));
    r[1].y=l.y;
  }
  { // original instruction 7
    vec4 a=(v[4]).xyzw,b=(constant(175)).xyzw,c=vec4(0);
    vec4 m=vec4(dot(vec4(a.xyz,1),b)),l=vec4(0);
    r[0].y=m.y;
  }
  { // original instruction 8
    vec4 a=(constant(96+address)).zwww,b=(r[1]).yyyy,c=(constant(96+address)).xyyy;
    vec4 m=multiply(a,b)+c,l=vec4(0);
    r[9].xy=m.xy;
  }
  { // original instruction 9
    vec4 a=(v[4]).xyzw,b=(constant(176)).xyzw,c=vec4(0);
    vec4 m=vec4(dot(vec4(a.xyz,1),b)),l=vec4(0);
    r[0].z=m.z;
  }
  { // original instruction 10
    vec4 a=-(r[9]).xxxx,b=vec4(0),c=(v[8]).zyxx;
    vec4 m=a,l=c;
    r[9].z=m.z;
    outv[4].xyz=l.xyz;
  }
  { // original instruction 11
    vec4 a=(r[10]).xyzz,b=(r[9]).xyww,c=(v[2]).xxxx;
    vec4 m=vec4(dot(a.xyz,b.xyz)),l=c;
    r[11].y=m.y;
    outv[3].w=l.w;
  }
  { // original instruction 12
    vec4 a=(r[10]).xyzz,b=(r[9]).yzww,c=vec4(0);
    vec4 m=vec4(dot(a.xyz,b.xyz)),l=vec4(0);
    r[11].x=m.x;
  }
  { // original instruction 13
    vec4 a=(r[11]).yyyy,b=(constant(4)).xyzz,c=vec4(0);
    vec4 m=multiply(a,b),l=vec4(0);
    r[9].xyz=m.xyz;
  }
  { // original instruction 14
    vec4 a=(r[11]).xxxx,b=(constant(5)).xyzz,c=(r[9]).xyzz;
    vec4 m=multiply(a,b)+c,l=vec4(0);
    r[9].xyz=m.xyz;
  }
  { // original instruction 15
    vec4 a=(r[0]).xyzz,b=vec4(0),c=(r[9]).xyzz;
    vec4 m=a+c,l=vec4(0);
    r[0].xyz=m.xyz;
  }
  { // original instruction 16
    vec4 a=(constant(4)).wwww,b=vec4(0),c=-(constant(4)).wwww;
    vec4 m=a+c,l=vec4(0);
    r[10].x=m.x;
  }
  { // original instruction 17
    vec4 a=(r[0]).xyzw,b=(constant(13)).xyzw,c=vec4(0);
    vec4 m=vec4(dot(vec4(a.xyz,1),b)),l=vec4(0);
    r[8].x=m.x;
  }
  { // original instruction 18
    vec4 a=(r[0]).xyzw,b=(constant(14)).xyzw,c=vec4(0);
    vec4 m=vec4(dot(vec4(a.xyz,1),b)),l=vec4(0);
    r[8].y=m.y;
  }
  { // original instruction 19
    vec4 a=(r[0]).xyzw,b=(constant(12)).xyzw,c=vec4(0);
    vec4 m=vec4(dot(vec4(a.xyz,1),b)),l=vec4(0);
    r[8].z=m.z;
  }
  { // original instruction 20
    vec4 a=(constant(4)).wwww,b=vec4(0),c=-(r[8]).xyzz;
    vec4 m=a+c,l=vec4(0);
    r[8].xyz=m.xyz;
  }
  { // original instruction 21
    vec4 a=(r[10]).xxxx,b=(r[10]).xxxx,c=vec4(0);
    vec4 m=multiply(a,b),l=vec4(0);
    r[10].x=m.x;
  }
  { // original instruction 22
    vec4 a=(r[8]).xyzz,b=(constant(15)).yyyy,c=vec4(0);
    vec4 m=max(a,b),l=vec4(0);
    r[8].xyz=m.xyz;
  }
  { // original instruction 23
    vec4 a=(r[8]).xyzz,b=(r[8]).xyzz,c=vec4(0);
    vec4 m=multiply(a,b),l=vec4(0);
    r[8].xyz=m.xyz;
  }
  { // original instruction 24
    vec4 a=(r[8]).xyzz,b=(constant(4)).wwww,c=vec4(0);
    vec4 m=min(a,b),l=vec4(0);
    r[8].xyz=m.xyz;
  }
  { // original instruction 25
    vec4 a=(r[8]).xxxx,b=vec4(0),c=(r[8]).yyyy;
    vec4 m=a+c,l=vec4(0);
    r[8].x=m.x;
  }
  { // original instruction 26
    vec4 a=(r[10]).xxxx,b=(r[10]).xxxx,c=vec4(0);
    vec4 m=multiply(a,b),l=vec4(0);
    r[10].x=m.x;
  }
  { // original instruction 27
    vec4 a=(r[8]).xxxx,b=(constant(4)).wwww,c=vec4(0);
    vec4 m=min(a,b),l=vec4(0);
    r[8].x=m.x;
  }
  { // original instruction 28
    vec4 a=(constant(4)).wwww,b=vec4(0),c=-(r[8]).xyzz;
    vec4 m=a+c,l=vec4(0);
    r[8].xyz=m.xyz;
  }
  { // original instruction 29
    vec4 a=(v[0]).yyxx,b=vec4(0),c=vec4(0);
    vec4 m=a,l=vec4(0);
    r[10].yz=m.yz;
  }
  { // original instruction 30
    vec4 a=(r[8]).xyzz,b=(r[8]).xyzz,c=vec4(0);
    vec4 m=multiply(a,b),l=vec4(0);
    r[8].xyz=m.xyz;
  }
  { // original instruction 31
    vec4 a=(r[8]).yyyy,b=vec4(0),c=-(r[8]).xxxx;
    vec4 m=a+c,l=vec4(0);
    r[8].y=m.y;
  }
  { // original instruction 32
    vec4 a=(constant(4)).wwww,b=vec4(0),c=-(r[10]).xxxx;
    vec4 m=a+c,l=vec4(0);
    r[10].x=m.x;
  }
  { // original instruction 33
    vec4 a=(constant(15)).xxxx,b=(r[8]).yyyy,c=(r[8]).xxxx;
    vec4 m=multiply(a,b)+c,l=vec4(0);
    r[8].w=m.w;
  }
  { // original instruction 34
    vec4 a=(constant(4)).wwww,b=vec4(0),c=-(v[15]).wwww;
    vec4 m=a+c,l=vec4(0);
    r[10].w=m.w;
  }
  { // original instruction 35
    vec4 a=(r[8]).zzzw,b=(constant(15)).zzzw,c=vec4(0);
    vec4 m=multiply(a,b),l=vec4(0);
    r[8].zw=m.zw;
  }
  { // original instruction 36
    vec4 a=-(r[8]).zzzw,b=vec4(0),c=(constant(4)).wwww;
    vec4 m=a+c,l=vec4(0);
    r[8].zw=m.zw;
  }
  { // original instruction 37
    vec4 a=(r[0]).xyzw,b=(constant(0)).xyzw,c=vec4(0);
    vec4 m=vec4(dot(vec4(a.xyz,1),b)),l=vec4(0);
    outv[0].x=m.x;
  }
  { // original instruction 38
    vec4 a=(r[0]).xyzw,b=(constant(1)).xyzw,c=vec4(0);
    vec4 m=vec4(dot(vec4(a.xyz,1),b)),l=vec4(0);
    outv[0].y=m.y;
  }
  { // original instruction 39
    vec4 a=(r[0]).xyzw,b=(constant(2)).xyzw,c=vec4(0);
    vec4 m=vec4(dot(vec4(a.xyz,1),b)),l=vec4(0);
    outv[0].z=m.z;
  }
  { // original instruction 40
    vec4 a=(r[0]).xyzw,b=(constant(3)).xyzw,c=vec4(0);
    vec4 m=vec4(dot(vec4(a.xyz,1),b)),l=vec4(0);
    outv[0].w=m.w;
  }
  { // original instruction 41
    vec4 a=(r[8]).zzzz,b=(r[8]).wwww,c=vec4(0);
    vec4 m=multiply(a,b),l=vec4(0);
    r[8].w=m.w;
  }
  { // original instruction 42
    vec4 a=(v[15]).wwww,b=(r[10]).xxxx,c=(r[10]).wwww;
    vec4 m=multiply(a,b)+c,l=vec4(0);
    r[10].x=m.x;
  }
  { // original instruction 43
    vec4 a=(r[10]).zyyy,b=(v[3]).zwww,c=(v[3]).xyyy;
    vec4 m=multiply(a,b)+c,l=vec4(0);
    r[11].xy=m.xy;
  }
  { // original instruction 44
    vec4 a=(r[8]).wwww,b=(r[10]).xxxx,c=(outv[0]).wwww;
    vec4 m=multiply(a,b),l=vec4(clampAwayZeroInf(1.0/c.x));
    r[8].w=m.w;
    r[1].w=l.w;
  }
  { // original instruction 45
    vec4 a=(r[10]).zzzy,b=(v[7]).zzzw,c=(v[7]).xxxy;
    vec4 m=multiply(a,b)+c,l=vec4(0);
    r[11].zw=m.zw;
  }
  { // original instruction 46
    vec4 a=(outv[0]).xyzz,b=(constant(10)).xyzz,c=vec4(0);
    vec4 m=multiply(a,b),l=vec4(0);
    outv[0].xyz=m.xyz;
  }
  { // original instruction 47
    vec4 a=(r[11]).xyyy,b=(constant(18)).xyyy,c=(constant(18)).zwww;
    vec4 m=multiply(a,b)+c,l=vec4(0);
    outv[9].xy=m.xy;
  }
  { // original instruction 48
    vec4 a=(r[11]).zwww,b=(constant(18)).xyyy,c=(constant(18)).zwww;
    vec4 m=multiply(a,b)+c,l=vec4(0);
    outv[10].xy=m.xy;
  }
  { // original instruction 49
    vec4 a=(v[8]).wwww,b=(r[8]).wwww,c=vec4(0);
    vec4 m=multiply(a,b),l=vec4(0);
    outv[4].w=m.w;
  }
  { // original instruction 50
    vec4 a=(outv[0]).xyzz,b=(r[1]).wwww,c=(constant(11)).xyzz;
    vec4 m=multiply(a,b)+c,l=vec4(0);
    outv[0].xyz=m.xyz;
  }
}
