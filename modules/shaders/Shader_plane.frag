const char* shader_plane_frag = STRINGIFY(

precision highp float;

varying vec3 position;

uniform sampler2D texY;
uniform sampler2D texU;
uniform sampler2D texV;
                                          uniform sampler2D texNV;
                                          uniform sampler2D texRGB;
                                          uniform int texWidth;
                                          uniform int texPixFmt;
uniform float uWidth;
uniform float uOffset;
         
// YUV offset
const vec3 offset = vec3(-0.0625, -0.5, -0.5);
// RGB coefficients
const vec3 Rcoeff = vec3( 1.164, 0.000,  1.596);
const vec3 Gcoeff = vec3( 1.164, -0.391, -0.813);
const vec3 Bcoeff = vec3( 1.164, 2.018,  0.000);
                                          
                                          void yuv2rgb(float y, float u, float v) {
                                              y = 1.1643 * (y - 0.0625);
                                              u -= 0.5;
                                              v -= 0.5;
                                              
                                              float r = y + 1.5958 * v;
                                              float g = y - 0.39173 * u - 0.81290 * v;
                                              float b = y + 2.017 * u;
                                              
                                              gl_FragColor = vec4(r, g, b, 1.0);
                                          }
                                          
                                          void yuv(vec2 uv) {
                                              float y = texture2D(texY, uv).r;
                                              float u = texture2D(texU, uv).r;
                                              float v = texture2D(texV, uv).r;
                                              
                                              yuv2rgb(y, u, v);
                                          }
                                          
                                          void nv12(vec2 uv) {
                                              float y = texture2D(texY, uv).r;
                                              float u = texture2D(texNV, uv).r;
                                              float v = texture2D(texNV, uv).a;
                                              
//                                              float x = uv.x * float(texWidth);
//                                              if (mod(x, 2.0) > 0.999999) {
//                                                  v = texture2D(texNV, uv).r;
//                                                  uv.x = (x - 1.0) / float(texWidth);
//                                                  u = texture2D(texNV, uv).r;
//                                              } else {
//                                                  u = texture2D(texNV, uv).r;
//                                                  uv.x = (x + 1.0) / float(texWidth);
//                                                  v = texture2D(texNV, uv).r;
//                                              }
                                              
                                              yuv2rgb(y, u, v);
                                          }
                                          
                                          void nv21(vec2 uv) {
                                              float y = texture2D(texY, uv).r;
                                              float u = texture2D(texNV, uv).r;
                                              float v = texture2D(texNV, uv).a;
                                              
//                                              float x = uv.x * float(texWidth);
//                                              if (mod(x, 2.0) > 0.999999) {
//                                                  u = texture2D(texNV, uv).r;
//                                                  uv.x = (x - 1.0) / float(texWidth);
//                                                  v = texture2D(texNV, uv).r;
//                                              } else {
//                                                  v = texture2D(texNV, uv).r;
//                                                  uv.x = (x + 1.0) / float(texWidth);
//                                                  u = texture2D(texNV, uv).r;
//                                              }
                                              
                                              yuv2rgb(y, u, v);
                                          }

void main() {
    float x = position.x;
    float y = position.y;

    vec2 uv;
    uv.x = ((x + 1.0) / 2.0) * uWidth + uOffset;
    uv.y = (y + 1.0) / 2.0;

    uv.x = 1.0 - uv.x;
    uv.y = 1.0 - uv.y;
    
    if (texPixFmt == 0) {
        yuv(uv);
    } else if (texPixFmt == 1) {
        nv12(uv);
    } else if (texPixFmt == 2) {
        nv21(uv);
    } else {
        gl_FragColor = vec4(texture2D(texRGB, uv).rgb, 1.0);
    }
    
//    vec3 yuv;
//    yuv.x = texture2D(texY, uv).r;
//    yuv.y = texture2D(texU, uv).r;
//    yuv.z = texture2D(texV, uv).r;
//    
//    vec3 rgb;
//    yuv = clamp(yuv, 0.0, 1.0);
//    yuv += offset;
//    rgb.x = dot(yuv, Rcoeff);
//    rgb.y = dot(yuv, Gcoeff);
//    rgb.z = dot(yuv, Bcoeff);
//  
//    gl_FragColor = vec4(rgb, 1.0);
}

);
