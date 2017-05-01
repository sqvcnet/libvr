const char* shader_Position_vert = STRINGIFY(

precision mediump float;

attribute vec3 vertexPosition_modelspace;
uniform mat4 MVP;


uniform vec4 uvOffset;
uniform float uvHeight;

varying vec3 position;
varying vec2 texcoord;

const float M_PI = (3.14);
const float M_2PI = (M_PI*2.0);

void main() {
    position = vertexPosition_modelspace;

    float x = position.x;
    float z = -position.y;
    float y = position.z;

    float theta = atan(y, x);
    float phi = atan(sqrt(x*x + y*y), z);

    vec2 uv;
    uv.x = (-theta + M_PI) / M_2PI + 0.25;
    uv.y = (phi / M_PI) * uvHeight;
    uv += uvOffset.xy;

    uv.x = 1.0 - uv.x;
    uv.y = 1.0 - uv.y;

    texcoord = uv;
    gl_Position = MVP * vec4(vertexPosition_modelspace, 1);

//    gl_Position = vec4(vertexPosition_modelspace, 1);
//    position = gl_Position.xyz;
}

);