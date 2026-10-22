#version 450
layout(location=0) flat in uvec4 nativeBits;
layout(location=1) flat in uvec4 fetchedBits;
layout(location=0) out uvec4 nativeResult;
layout(location=1) out uvec4 fetchedResult;
void main(){nativeResult=nativeBits;fetchedResult=fetchedBits;}
