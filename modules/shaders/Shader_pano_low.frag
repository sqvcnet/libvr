const char* shader_pano_frag = STRINGIFY(

precision highp float;

varying vec3 position;
varying vec2 texcoord;

uniform sampler2D texY;
uniform sampler2D texU;
uniform sampler2D texV;
uniform vec4 uvOffset;
uniform float uvHeight;

const float PI = (3.141593);
const float PI2 = (6.283186);
const float HALF_PI = (1.570796);

// YUV offset
const vec3 offset = vec3(-0.0625, -0.5, -0.5);
// RGB coefficients
const vec3 Rcoeff = vec3( 1.164, 0.000,  1.596);
const vec3 Gcoeff = vec3( 1.164, -0.391, -0.813);
const vec3 Bcoeff = vec3( 1.164, 2.018,  0.000);

float ACos(float inX)
{
    float x = abs(inX);
    float res = (-0.156583) * x + HALF_PI;
    res *= sqrt(1.0 - x);
    if (inX >= 0.0) {
        return res;
    } else {
        return PI - res;
    }
}

float ASin(float x)
{
    return HALF_PI - ACos(x);
}

float ATanPos(float x)
{ 
    float t0 = (x < 1.0) ? x : 1.0 / x;
    float t1 = t0 * t0;
    float poly = 0.0872929;
    poly = (-0.301895) + poly * t1;
    poly = 1.0 + poly * t1;
    poly = poly * t0;
    return (x < 1.0) ? poly : (HALF_PI - poly);
}

float ATan(float x)
{     
    float t0 = ATanPos(abs(x));     
    return (x < 0.0) ? -t0 : t0;
}

void main() {

//<<<<<<< HEAD
//    float x = position.x;
//    float z = -position.y;
//    float y = position.z;
////
////    float inX = abs(y/x);
////    float t0 = (inX < 1.0) ? inX : 1.0 / inX;
////    float t1 = t0 * t0;
////    float poly = 0.0872929;
////    poly = (-0.301895) + poly * t1;
////    poly = 1.0 + poly * t1;
////    poly = poly * t0;
////    float ret = (inX < 1.0) ? poly : (HALF_PI - poly);
////    
////    float theta = y/x < 0.0 ? -ret : ret;
//    
////    float theta = ATan(y/x)/2.0;
//    float theta = atan(y, x);
////    float theta = 0.3;
////    float phi = sqrt(x*x + y*y);
////    float phi = ATan(sqrt(x*x + y*y)/z)*2.0;
//    float phi = atan(sqrt(x*x + y*y), z);
//    
//    vec2 uv;
//    uv.x = (-theta + PI) / PI2 + 0.25;
//    uv.y = (phi / PI) * uvHeight;
//    uv += uvOffset.xy;
//=======
//    // float x = position.x;
//    // float z = -position.y;
//    // float y = position.z;
//
//    // float theta = atan(y, x);
//    // float phi = atan(sqrt(x*x + y*y), z);
//
//    // vec2 uv;
//    // uv.x = (-theta + M_PI) / M_2PI + 0.25;
//    // uv.y = (phi / M_PI) * uvHeight;
//    // uv += uvOffset.xy;
//>>>>>>> 0498621d1275d5e7614c40ea76c7af140f5a6f1c

    // uv.x = 1.0 - uv.x;
    // uv.y = 1.0 - uv.y;

    vec2 uv = texcoord;

//    y = texture2D(texY, uv).r;
//    float u = texture2D(texU, uv).r;
//    float v = texture2D(texV, uv).r;
//    
//    y = 1.1643 * (y - 0.0625);
//    u -= 0.5;
//    v -= 0.5;
//    
//    float r = y + 1.5958 * v;
//    float g = y - 0.39173 * u - 0.81290 * v;
//    float b = y + 2.017 * u;
//    
//    gl_FragColor = vec4(r, g, b, 1.0);
    
    
    vec3 yuv;
    yuv.x = texture2D(texY, uv).r;
    yuv.y = texture2D(texU, uv).r;
    yuv.z = texture2D(texV, uv).r;
    
    vec3 rgb;
    yuv = clamp(yuv, 0.0, 1.0);
    yuv += offset;
    rgb.x = dot(yuv, Rcoeff);
    rgb.y = dot(yuv, Gcoeff);
    rgb.z = dot(yuv, Bcoeff);
    
    gl_FragColor = vec4(rgb, 1.0);
    
//    y = texture2D(texY, uv).b;
//    gl_FragColor = vec4(y, y, y, 1.0);
    
//    gl_FragColor = vec4(texture2D(texY, uv).rgb, 1.0);
}

);