#version 450
layout(location=0) in vec4 point;
layout(push_constant) uniform Camera {mat4 matrix;vec4 color;vec4 viewport;} camera;
layout(location=0) out float sourceDepth;
void main(){
    gl_Position=camera.matrix*vec4(point.xyz,1);sourceDepth=abs(camera.color.w)>1.5?gl_Position.w:gl_Position.w*(16777215.0/1024.0);
    if(abs(camera.color.w)>1.5) {
        vec2 window=(gl_Position.xy/gl_Position.w)*camera.viewport.zw*.5+
            camera.viewport.xy+camera.viewport.zw*.5+vec2(.5);
        window=trunc(window*16.0)/16.0;
        vec2 ndc=(window-camera.viewport.xy)*2.0/camera.viewport.zw-1.0;
        gl_Position.xy=ndc*gl_Position.w;
    }
}
