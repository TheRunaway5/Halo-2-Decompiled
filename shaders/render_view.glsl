// Retail 00012FA0 c8..c11. Coordinates are pixels in the active target.
// Texel offsets for a sampled image must use that image's dimensions instead.
vec4 retailViewportConstant(uint index,vec4 viewport,float requestedScale,
                            bool scaled,bool scalingDisabled,uint mode) {
    precise float width=viewport.z,height=viewport.w,left=viewport.x,top=viewport.y;
    precise float scale=scaled && !scalingDisabled?clamp(requestedScale,.0625,1.0):1.0;
    precise float bias=.03125;
    if(mode==2u){width=128.0;height=128.0;left=0.0;top=0.0;scale=1.0;bias=0.0;}
    precise vec4 value=vec4(0);
    if(index==8u)value=vec4(width*.5*scale,0,0,((width+1.0)*.5+left)*scale+bias);
    if(index==9u)value=vec4(0,height*-.5*scale,0,((height+1.0)*.5+top)*scale+bias);
    if(index==10u)value=vec4(width*.5*scale,height*-.5*scale,16777215,0);
    if(index==11u)value=vec4((width*.5+left)*scale+.5,(height*.5+top)*scale+.5,0,0);
    return value;
}
