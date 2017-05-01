const char* shader_Position_vert = STRINGIFY(

precision mediump float;

attribute vec3 vertexPosition_modelspace;
uniform mat4 MVP;

varying vec3 position;

void main() {
    position = vertexPosition_modelspace;
    gl_Position = MVP * vec4(vertexPosition_modelspace, 1);
}

);