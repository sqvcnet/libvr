#include "Util.h"

void Util::xyz2r(float x, float y, float z, float &r, float &theta, float &phi) {
    r = sqrt(x * x + y * y + z * z);
    theta = atan(z / x);
    phi = asin(y / r);
}

void Util::r2xyz(float r, float theta, float phi, float &x, float &y, float &z) {
    x = r * cos(phi) * cos(theta);
    y = r * sin(phi);
    z = r * cos(phi) * sin(theta);
}
