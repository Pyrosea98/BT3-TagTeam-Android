#version 450
// EXPERIMENT (lite): the cheapest fragment stage with gs.frag's interface, to measure what gs.frag's own work costs
layout(location = 0) in vec4 vColor;
layout(location = 1) in vec3 vStq;
layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outAux;
layout(set = 2, binding = 0) uniform sampler2D tex;
layout(set = 2, binding = 1) uniform sampler2D dateTex;
layout(set = 3, binding = 0) uniform Params { ivec4 mode; vec4 misc; vec4 rect; vec4 orig; vec4 extra; } p;
void main() {
    vec4 t = p.mode.x != 0 ? texture(tex, vStq.xy / vStq.z) : vec4(1.0);
    outColor = clamp(t * vColor * 2.0, 0.0, 1.0);
    outAux = vec4(outColor.a, 0.0, 0.0, 1.0);
}
