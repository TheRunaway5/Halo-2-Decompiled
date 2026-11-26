#version 450

layout(binding=0) uniform sampler2D rawDepth;
layout(binding=1) uniform sampler2D sceneColor;
layout(location=0) in vec4 currentClip;
layout(location=1) in vec4 previousClip;
layout(location=0) out vec4 pixel;
layout(push_constant) uniform MotionBlur {
    mat4 matrix;
    ivec4 bounds;
    vec2 extent;
    float shutterFraction;
    uint wDepth;
} blur;

void main() {
    // Z and reciprocal W are affine in screen space. Use the local plane
    // to keep the gather on sloping surfaces and reject depth discontinuities.
    bool wDepth = blur.wDepth != 0u;
    float plane = wDepth ? 1024.0 * gl_FragCoord.w : gl_FragCoord.z;
    vec2 slope = vec2(dFdx(plane), dFdy(plane));
    float expected = wDepth ? 1.0 / plane : plane;
    ivec2 center = ivec2(gl_FragCoord.xy);
    float actual = texelFetch(rawDepth, center, 0).r;
    float tolerance = wDepth ? max(0.000002, abs(expected) * 0.0002) : 0.000002;
    if (actual >= 1.0 || abs(actual - expected) > tolerance || previousClip.w <= 0.0)
        discard;

    vec2 motion = (currentClip.xy / currentClip.w - previousClip.xy / previousClip.w)
        * 0.25 * blur.extent * blur.shutterFraction;
    float radius = length(motion);
    if (!(radius >= 0.5) || isinf(radius))
        discard;
    float maxRadius = clamp(24.0 * abs(blur.extent.y) / 1080.0, 8.0, 48.0);
    motion *= min(1.0, maxRadius / radius);

    vec4 color = texelFetch(sceneColor, center, 0);
    vec3 sum = color.rgb;
    float weightSum = 1.0;
    for (int tap = -4; tap <= 4; ++tap) {
        if (tap == 0)
            continue;
        float fraction = float(tap) * 0.25;
        ivec2 point = ivec2(floor(gl_FragCoord.xy + fraction * motion));
        if (any(lessThan(point, blur.bounds.xy)) || any(greaterThanEqual(point, blur.bounds.zw))
            || all(equal(point, center)))
            continue;
        float samplePlane = plane + dot(slope, vec2(point - center));
        if (wDepth && samplePlane <= 0.0)
            continue;
        float sampleExpected = wDepth ? 1.0 / samplePlane : samplePlane;
        float sampleDepth = texelFetch(rawDepth, point, 0).r;
        if (sampleDepth >= 1.0 || abs((sampleDepth - actual) - (sampleExpected - expected)) > 2.0 * tolerance)
            continue;
        float weight = 1.0 - 0.6 * abs(fraction);
        sum += weight * texelFetch(sceneColor, point, 0).rgb;
        weightSum += weight;
    }
    if (weightSum == 1.0)
        discard;
    pixel = vec4(sum / weightSum, color.a);
}
