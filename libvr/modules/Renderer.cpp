#include "Renderer.h"
#include "VideoFile.hpp"
#include "Shader.h"
#include "Util.h"

#include <sys/types.h>
#include <sys/stat.h>

#include <string>
#include <vector>
#include <fstream>
#include <iostream>

using namespace std;

Renderer::Renderer()
: _w(0),
_h(0),
_panoProgramID(0),
_texturesPanoId{0},
_texturesPlaneId{0},
_texWidth(0),
_texHeight(0),
_texPixFmt(PixFmt::YUV420P),
_delegate(nullptr),
_inited(false),
_initedGL(false),
_readyOpen(false),
_opened(false),
_loadedTexture(false),
_readyClose(false),
_uvHeight(0.5),
_uWidth(1.0),
_uOffset(0.0),
_is360(true),
_isSingleView(false),
_rotateDegree(0),
_viewPortDegree(96),
_isLandscapeLeft(true)
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
,
_eglDisplay(nullptr),
_imageY(nullptr),
_imageU(nullptr),
_imageV(nullptr),
_graphicBufY(nullptr),
_graphicBufU(nullptr),
_graphicBufV(nullptr)
#endif
{
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
    initExtensions();
#endif

}

void Renderer::initGL() {
    if (_initedGL) {
        return;
    }

//    GLuint VertexArrayID;
//    glGenVertexArrays(1, &VertexArrayID);
//    glBindVertexArray(VertexArrayID);
    
    glEnable(GL_TEXTURE_2D);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
	LOGV("Renderer::initGL: GL_SHADING_LANGUAGE_VERSION is %s.\n", (char *)glGetString(GL_SHADING_LANGUAGE_VERSION));
    LOGV("Renderer::initGL: GL_VERSION is %s.\n", (char *)glGetString(GL_VERSION));
	_panoProgramID = LoadShaders(shader_Position_vert, shader_pano_frag);
    _planeProgramID = LoadShaders(shader_Position_vert, shader_plane_frag);

//    GLint num;
//    GLint compressedTexFormats[1024];
//    glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &num);
//    glGetIntegerv(GL_COMPRESSED_TEXTURE_FORMATS, compressedTexFormats);
//    for (int i = 0; i < num; i++) {
//        LOGD("Renderer::initGL: compressedTexFormat: %d, num: %d", compressedTexFormats[i], num);
//    }

    _initedGL = true;
}

void Renderer::init(int winWidth, int winHeight) {
	LOGD("Renderer::init Width: %d, Height: %d", winWidth, winHeight);
	_w = winWidth;
	_h = winHeight;
    _inited = true;
}

bool Renderer::open() {
    if (_opened) {
        return true;
    }
    if (!_readyOpen) {
        return false;
    }
    glUseProgram(_panoProgramID);
    genTextures(_texturesPanoId);
    GLuint texSampler = glGetUniformLocation(_panoProgramID, "texY");
    glUniform1i(texSampler, 0);
    texSampler = glGetUniformLocation(_panoProgramID, "texU");
    glUniform1i(texSampler, 1);
    texSampler = glGetUniformLocation(_panoProgramID, "texV");
    glUniform1i(texSampler, 2);
    texSampler = glGetUniformLocation(_panoProgramID, "texNV");
    glUniform1i(texSampler, 3);
    texSampler = glGetUniformLocation(_panoProgramID, "texRGB");
    glUniform1i(texSampler, 4);
    
    glUseProgram(_planeProgramID);
    genTextures(_texturesPlaneId);
    texSampler = glGetUniformLocation(_planeProgramID, "texY");
    glUniform1i(texSampler, 0);
    texSampler = glGetUniformLocation(_planeProgramID, "texU");
    glUniform1i(texSampler, 1);
    texSampler = glGetUniformLocation(_planeProgramID, "texV");
    glUniform1i(texSampler, 2);
    texSampler = glGetUniformLocation(_planeProgramID, "texNV");
    glUniform1i(texSampler, 3);
    texSampler = glGetUniformLocation(_panoProgramID, "texRGB");
    glUniform1i(texSampler, 4);
    
    _opened = true;
    _loadedTexture = false;
    _readyOpen = false;
    return true;
}

void Renderer::open(int texWidth, int texHeight) {
    if (_opened) {
        LOGD("Renderer::open texWidth: %d, texHeight: %d, _texWidth: %d, _texHeight: %d, already opened.", texWidth, texHeight, _texWidth, _texHeight);
        return;
    }
//    if (!_inited) {
//        LOGE("Have not call Renderer::init firstly!!!");
//        return;
//    }
//    if (_is360) {
//        glUseProgram(_panoProgramID);
//    } else {
//
//    }
    
    LOGD("Renderer::open texWidth: %d, texHeight: %d, _texWidth: %d, _texHeight: %d.", texWidth, texHeight, _texWidth, _texHeight);
    _texWidth = texWidth;
    _texHeight = texHeight;
    _readyOpen = true;
    while (_readyOpen) {
        this_thread::sleep_for(chrono::milliseconds(50));
        LOGD("wait for Renderer open..., you should assure Renderer::render be continuously called(set a breakpoint to detect this)");
    }
}

void Renderer::close() {
    if (!_opened) {
        LOGD("Renderer::close already closed.");
        return;
    }
//    _delegate = nullptr;
//    int i = 0;
//    while (_readyOpen && i < 10) {
//        this_thread::sleep_for(chrono::milliseconds(50));
//        LOGD("wait for Renderer open...");
//        i++;
//    }
//    if (!_opened) {
//        _readyOpen = false;
//        return;
//    }
    
    _readyClose = true;
    while (_readyClose) {
        this_thread::sleep_for(chrono::milliseconds(50));
        LOGD("wait for Renderer close..., you should assure Renderer::render be continuously called(set a breakpoint to detect this)");
    }
}

bool Renderer::isOpened() {
    return _opened;
}

void Renderer::safeClose() {
    if (!_readyClose) {
        return;
    }
    glUseProgram(_panoProgramID);
    glDeleteTextures(TEXTURE_NUM, _texturesPanoId);
    glUseProgram(_planeProgramID);
    glDeleteTextures(TEXTURE_NUM, _texturesPlaneId);

    glDeleteProgram(_panoProgramID);
    glDeleteProgram(_planeProgramID);
    
    glFinish();
    
    _opened = false;
    _readyClose = false;
    _initedGL = false;
}

void Renderer::set360UpDown() {
    _is360 = true;
    _uvHeight = 0.5;
    _isSingleView = false;
}

void Renderer::set360() {
    _is360 = true;
    _uvHeight = 1.0;
    _isSingleView = false;
}

void Renderer::set360SingleView() {
    _is360 = true;
    _uvHeight = 1.0;
    _isSingleView = true;
}

void Renderer::setPlane() {
    _is360 = false;
    _uWidth = 0.5;
    _uOffset = 0.5;
}

void Renderer::set3D() {
    _is360 = false;
    _uWidth = 1.0;
    _uOffset = 0.0;
}

void Renderer::set3DLeftRight() {
    setPlane();
}

void Renderer::setMode(Mode mode) {
    switch (mode) {
        case Renderer::Mode::MODE_3D:
            set3D();
            break;
        case Renderer::Mode::MODE_360:
            set360();
            break;
        case Renderer::Mode::MODE_360_UP_DOWN:
            set360UpDown();
            break;
        case Renderer::Mode::MODE_3D_LEFT_RIGHT:
            set3DLeftRight();
            break;
        case Renderer::Mode::MODE_360_SINGLE:
            set360SingleView();
            break;
        default:
            set360();
            break;
    }
}

void Renderer::setRotateDegree(int degree) {
    if (degree >= 0 && degree <= 180) {
        _rotateDegree = degree;
    }
}

void Renderer::setViewPortDegree(int degree) {
    if (degree >= 0 && degree <= 180) {
        _viewPortDegree = degree;
    }
}

void Renderer::setDelegate(RendererDelegate *delegate) {
    _delegate = delegate;
}

#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
void Renderer::genTexture(int texWidth, int texHeight, GLuint *texId, GLenum tex, GLenum texIndex, GraphicBuffer **gb, EGLImageKHR *image, const char * texSamplerName) {
#else
void Renderer::genTexture(int texWidth, int texHeight, GLuint *texId, GLenum tex, GLenum texIndex, const char * texSamplerName) {
#endif
    //注意：各个texture的像素点数一定要是偶数乘偶数的，宽和高未必相同（之前推测有误），原因待查
    glGenTextures(1, texId);
//    glActiveTexture(tex);
    glActiveTexture(GL_TEXTURE0 + texIndex);
    // "Bind" the newly created texture : all future texture functions will modify this texture
    glBindTexture(GL_TEXTURE_2D, *texId);
    
//    if (strcmp(texSamplerName, "texNV") == 0) {
//        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//    } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID_NOT_USE
    //int usage = GraphicBuffer::USAGE_HW_TEXTURE | GraphicBuffer::USAGE_SW_READ_OFTEN | GraphicBuffer::USAGE_SW_WRITE_RARELY;
    int usage = GraphicBuffer::USAGE_HW_TEXTURE | GraphicBuffer::USAGE_SW_WRITE_OFTEN | GraphicBuffer::USAGE_SW_READ_RARELY;
    *gb = new GraphicBuffer(texWidth, texHeight, PIXEL_FORMAT_RGB_565, usage);
    
    android::android_native_buffer_t *clientBuf = (android::android_native_buffer_t *)(*gb)->getNativeBuffer();
    LOGV("clientBuf, width: %d, height: %d, stride: %d, format: %d, usage: %d",
         clientBuf->width,
         clientBuf->height,
         clientBuf->stride,
         clientBuf->format,
         clientBuf->usage);
    // create the new EGLImageKHR
    const EGLint attrs[] =
    {
        EGL_IMAGE_PRESERVED_KHR, EGL_TRUE,
        EGL_NONE, EGL_NONE
    };
    
    EGLint eglImageAttributes[] = {
        //		EGL_WIDTH, 960,
        //		EGL_HEIGHT, 960,
        //EGL_MATCH_FORMAT_KHR, EGL_FORMAT_RGB_565_KHR,
        EGL_IMAGE_PRESERVED_KHR, EGL_TRUE,
        EGL_NONE, EGL_NONE};
    
    LOGV("_eglDisplay: %x, texWidth: %d, texHeight: %d\n", (unsigned int)_eglDisplay, texWidth, texHeight);
    *image = eglCreateImageKHR(
                               _eglDisplay,
                               EGL_NO_CONTEXT,
                               EGL_NATIVE_BUFFER_ANDROID,
                               (EGLClientBuffer)clientBuf,
                               eglImageAttributes);
    
    // Give the image to OpenGL
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, texWidth, texHeight, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, nullptr);
    glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, *image);
#else
    if (strcmp(texSamplerName, "texRGB") == 0) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texWidth, texHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    } else if (strcmp(texSamplerName, "texNV") == 0) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, texWidth, texHeight, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, nullptr);
    } else {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, texWidth, texHeight, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, nullptr);
    }
#endif
}

void Renderer::genTextures(GLuint texturesId[]) {
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
    _eglDisplay = eglGetCurrentDisplay();
    genTexture(_texWidth, _texHeight, &texturesId[0], GL_TEXTURE0, 0, &_graphicBufY, &_imageY, "texY");
    genTexture(_texWidth/2, _texHeight/2, &texturesId[1], GL_TEXTURE1, 1, &_graphicBufU, &_imageU, "texU");
    genTexture(_texWidth/2, _texHeight/2, &texturesId[2], GL_TEXTURE2, 2, &_graphicBufV, &_imageV, "texV");
    genTexture(_texWidth/2, _texHeight/2, &texturesId[3], GL_TEXTURE3, 3, &_graphicBufV, &_imageV, "texNV");
    genTexture(_texWidth, _texHeight, &texturesId[4], GL_TEXTURE4, 4, &_graphicBufV, &_imageV, "texRGB");
#else
    genTexture(_texWidth, _texHeight, &texturesId[0], GL_TEXTURE0, 0, "texY");
    genTexture(_texWidth/2, _texHeight/2, &texturesId[1], GL_TEXTURE1, 1, "texU");
    genTexture(_texWidth/2, _texHeight/2, &texturesId[2], GL_TEXTURE2, 2, "texV");
    genTexture(_texWidth/2, _texHeight/2, &texturesId[3], GL_TEXTURE3, 3, "texNV");
    genTexture(_texWidth, _texHeight, &texturesId[4], GL_TEXTURE4, 4, "texRGB");
#endif
}

bool Renderer::loadTexture() {
    unsigned char *dataY = nullptr;
    unsigned char *dataU = nullptr;
    unsigned char *dataV = nullptr;
    if (!_delegate) {
        return false;
    }
    bool isNewFrame = _delegate->getTexture(&dataY, &dataU, &dataV, &_texPixFmt);
    if (!isNewFrame) {
        return false;
    }
    
    _loadedTexture = true;
    
    GLuint *texturesId = nullptr;
    if (_is360) {
        texturesId = _texturesPanoId;
    } else {
        texturesId = _texturesPlaneId;
    }
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID_NOT_USE
	unsigned char * pBitmap = NULL;
    _graphicBufY->lock(GraphicBuffer::USAGE_SW_WRITE_OFTEN, (void**)&pBitmap);
    memcpy(pBitmap, dataY, _texWidth*_texHeight);
    _graphicBufY->unlock();
    
    _graphicBufU->lock(GraphicBuffer::USAGE_SW_WRITE_OFTEN, (void**)&pBitmap);
    memcpy(pBitmap, dataU, _texWidth*_texHeight/4);
    _graphicBufU->unlock();
    
    _graphicBufV->lock(GraphicBuffer::USAGE_SW_WRITE_OFTEN, (void**)&pBitmap);
    memcpy(pBitmap, dataV, _texWidth*_texHeight/4);
    _graphicBufV->unlock();
#else
    
    if (_texPixFmt == PixFmt::RGB) {
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, texturesId[4]);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, _texWidth, _texHeight, GL_RGB, GL_UNSIGNED_BYTE, dataY);
    } else {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texturesId[0]);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, _texWidth, _texHeight, GL_LUMINANCE, GL_UNSIGNED_BYTE, dataY);
        
        if (_texPixFmt == PixFmt::YUV420P) {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, texturesId[1]);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, _texWidth/2, _texHeight/2, GL_LUMINANCE, GL_UNSIGNED_BYTE, dataU);
            
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, texturesId[2]);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, _texWidth/2, _texHeight/2, GL_LUMINANCE, GL_UNSIGNED_BYTE, dataV);
        } else {
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, texturesId[3]);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, _texWidth/2, _texHeight/2, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, dataU);
        }
    }
#endif
    
    _delegate->consumeTexture();
	//eglSwapBuffers(eglGetCurrentDisplay(), t);
    return true;
}

void Renderer::computeTriangle(glm::vec3 direction, glm::vec3 headUpDir, float angle, float widthHeightRate, float vertexLeft[], float vertexRight[], glm::mat4 *mvp) {
    
    headUpDir = glm::normalize(headUpDir);
    direction = glm::normalize(direction);
    
    float distanceNear = 1.0;
    float halfVerticalLength = distanceNear * tan(angle);
    
    float halfHorizontalLength = halfVerticalLength * widthHeightRate;
    //LOGV("halfSquareLength %f", halfSquareLength);
    direction = distanceNear * glm::normalize(direction);
    //LOGV("direction(%f, %f, %f), dot: %f", direction.x, direction.y, direction.z, glm::dot(direction, headUpDir));
    
    glm::vec3 dirHalfSquareHorizontalEdge = glm::cross(direction, headUpDir);
    //LOGV("dirHalfSquareHorizontalEdge(%f, %f, %f)", dirHalfSquareHorizontalEdge.x, dirHalfSquareHorizontalEdge.y, dirHalfSquareHorizontalEdge.z);
    glm::vec3 dirHalfSquareVerticalEdge = glm::cross(dirHalfSquareHorizontalEdge, direction);
    
    dirHalfSquareHorizontalEdge = halfHorizontalLength * glm::normalize(dirHalfSquareHorizontalEdge);
    //LOGV("dirHalfSquareHorizontalEdge(%f, %f, %f)", dirHalfSquareHorizontalEdge.x, dirHalfSquareHorizontalEdge.y, dirHalfSquareHorizontalEdge.z);
    dirHalfSquareVerticalEdge = halfVerticalLength * glm::normalize(dirHalfSquareVerticalEdge);
    //LOGV("dirHalfSquareVerticalEdge(%f, %f, %f)", dirHalfSquareVerticalEdge.x, dirHalfSquareVerticalEdge.y, dirHalfSquareVerticalEdge.z);
    
    glm::vec3 dirBevelHorizontalEdge = direction + dirHalfSquareHorizontalEdge;
    //LOGV("dirBevelEdge(%f, %f, %f)", dirBevelEdge.x, dirBevelEdge.y, dirBevelEdge.z);
    glm::vec3 dirBevelVerticalEdge = direction + dirHalfSquareVerticalEdge;
    //LOGV("dirBevelEdge(%f, %f, %f)", dirBevelEdge.x, dirBevelEdge.y, dirBevelEdge.z);
    
    glm::vec3 dirRadiusEdge1 = dirBevelVerticalEdge + dirHalfSquareHorizontalEdge;
    //LOGV("dirRadiusEdge1(%f, %f, %f)", dirRadiusEdge1.x, dirRadiusEdge1.y, dirRadiusEdge1.z);
    dirRadiusEdge1 = glm::normalize(dirRadiusEdge1);
    //LOGV("dirRadiusEdge1(%f, %f, %f)", dirRadiusEdge1.x, dirRadiusEdge1.y, dirRadiusEdge1.z);
    
    dirHalfSquareHorizontalEdge = dirHalfSquareHorizontalEdge * (-1.0f);
    glm::vec3 dirRadiusEdge2 = dirBevelVerticalEdge + dirHalfSquareHorizontalEdge;
    dirRadiusEdge2 = glm::normalize(dirRadiusEdge2);
    //LOGV("dirRadiusEdge2(%f, %f, %f)", dirRadiusEdge2.x, dirRadiusEdge2.y, dirRadiusEdge2.z);
    
    dirHalfSquareVerticalEdge = dirHalfSquareVerticalEdge * (-1.0f);
    dirBevelVerticalEdge = direction + dirHalfSquareVerticalEdge;
    
    glm::vec3 dirRadiusEdge3 = dirBevelVerticalEdge + dirHalfSquareHorizontalEdge;
    dirRadiusEdge3 = glm::normalize(dirRadiusEdge3);
    //LOGV("dirRadiusEdge3(%f, %f, %f)", dirRadiusEdge3.x, dirRadiusEdge3.y, dirRadiusEdge3.z);
    
    dirHalfSquareHorizontalEdge = dirHalfSquareHorizontalEdge * (-1.0f);
    glm::vec3 dirRadiusEdge4 = dirBevelVerticalEdge + dirHalfSquareHorizontalEdge;
    dirRadiusEdge4 = glm::normalize(dirRadiusEdge4);
    //LOGV("dirRadiusEdge4(%f, %f, %f)", dirRadiusEdge4.x, dirRadiusEdge4.y, dirRadiusEdge4.z);
    
    const GLfloat cVertexLeft[] = {
        dirRadiusEdge1.x,dirRadiusEdge1.y,dirRadiusEdge1.z,
        dirRadiusEdge3.x,dirRadiusEdge3.y,dirRadiusEdge3.z,
        dirRadiusEdge4.x,dirRadiusEdge4.y,dirRadiusEdge4.z,
    };
    memcpy(vertexLeft, cVertexLeft, 9*sizeof(float));
    
    const GLfloat cVertexRight[] = {
        dirRadiusEdge1.x,dirRadiusEdge1.y,dirRadiusEdge1.z,
        dirRadiusEdge2.x,dirRadiusEdge2.y,dirRadiusEdge2.z,
        dirRadiusEdge3.x,dirRadiusEdge3.y,dirRadiusEdge3.z,
    };
    memcpy(vertexRight, cVertexRight, 9*sizeof(float));
    
    //halfSquareLength = glm::distance(dirRadiusEdge1, dirRadiusEdge2)/2 + 0.1;
    halfHorizontalLength = glm::distance(dirRadiusEdge1, dirRadiusEdge2)/2;
    halfVerticalLength = glm::distance(dirRadiusEdge2, dirRadiusEdge3)/2;
    glm::mat4 matProjection = glm::ortho(-halfHorizontalLength,
                                         halfHorizontalLength,
                                         -halfVerticalLength,
                                         halfVerticalLength,
                                         0.0f,
                                         1.0f); // In world coordinates
    glm::mat4 matView = glm::lookAt(
                                    glm::vec3(0,0,0), // Camera is at (0,0,0), in World Space
                                    glm::normalize(direction),
                                    headUpDir
                                    );
    
    *mvp = matProjection * matView;
}

void Renderer::render(float viewMatrix[]) {
    //assure all OpenGL functions execute in one and the same GLthread
    initGL();
    if (!_inited || !open()) {
        //LOGD("Have not call init and open firstly");
        return;
    }
    
    if (_is360 == true) {
        //glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
//        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(_panoProgramID);
        loadTexture();
        if (_loadedTexture) {
            //avoid green screen
            GLuint texWidthID = glGetUniformLocation(_panoProgramID, "texWidth");
            glUniform1iv(texWidthID, 1, &_texWidth);
            
            GLuint texPixFmtID = glGetUniformLocation(_panoProgramID, "texPixFmt");
            glUniform1iv(texPixFmtID, 1, reinterpret_cast<int *>(&_texPixFmt));
            
            render360(viewMatrix);
        }
    } else {
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
        //            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(_planeProgramID);
        loadTexture();
        if (_loadedTexture) {
#else
        glUseProgram(_planeProgramID);
        if (loadTexture() && _loadedTexture) {
#endif
            //avoid green screen
            GLuint texWidthID = glGetUniformLocation(_planeProgramID, "texWidth");
            glUniform1iv(texWidthID, 1, &_texWidth);
            
            GLuint texPixFmtID = glGetUniformLocation(_planeProgramID, "texPixFmt");
            glUniform1iv(texPixFmtID, 1, reinterpret_cast<int *>(&_texPixFmt));
            
            renderPlane();
        }
    }
    glFlush();
    safeClose();
    
    //LOGV("%s", "Renderer::render end");
    //    LOGV("Renderer::render end, time:%ld", std::time(nullptr));
}

void Renderer::render360(float viewMatrix[]) {
    glm::vec3 xAxis = -glm::vec3(viewMatrix[0], viewMatrix[4], viewMatrix[8]);
    glm::vec3 headUpDir = glm::vec3(viewMatrix[1], viewMatrix[5], viewMatrix[9]);
    glm::vec3 direction = -glm::vec3(viewMatrix[2], viewMatrix[6], viewMatrix[10]);
    
//     LOGV("Render::render viewMatrix \n(%f, %f, %f, %f)\n(%f, %f, %f, %f)\n(%f, %f, %f, %f)\n(%f, %f, %f, %f)",
//         viewMatrix[0], viewMatrix[1], viewMatrix[2], viewMatrix[3],
//         viewMatrix[4], viewMatrix[5], viewMatrix[6], viewMatrix[7],
//         viewMatrix[8], viewMatrix[9], viewMatrix[10], viewMatrix[11],
//         viewMatrix[12], viewMatrix[13], viewMatrix[14], viewMatrix[15]
//     );
    
    //	LOGV("direction(%f, %f, %f), dot: %f", direction.x, direction.y, direction.z, glm::dot(direction, headUpDir));
    
    float angle = _viewPortDegree/2.0/180.0*glm::pi<float>();
    float rate = _w/2.0/_h;
    if (_isSingleView) {
        rate = _w/1.0/_h;
    }
    
    GLuint uvHeightID = glGetUniformLocation(_panoProgramID, "uvHeight");
    glUniform1fv(uvHeightID, 1, &_uvHeight);
    
    glm::vec4 uvOffset = glm::vec4(0, 1.0-_uvHeight, 0, 0);
    GLuint uvOffsetID = glGetUniformLocation(_panoProgramID, "uvOffset");
    glUniform4fv(uvOffsetID, 1, &uvOffset[0]);
    
//    if (_w/2 > _h) {
//        glViewport((_w/2-_h)/2, 0, _h, _h);
//    } else {
//        glViewport(0, (_h-_w/2)/2, _w/2, _w/2);
//    }
    if (_isSingleView) {
        glViewport(0, 0, _w, _h);
    } else {
        glViewport(0, 0, _w/2, _h);
    }
    
    float vertexLeft[9] = {0};
    float vertexRight[9] = {0};
    glm::mat4 mvp;
    computeTriangle(direction, headUpDir, angle, rate, vertexLeft, vertexRight, &mvp);
    
    GLuint matrixID = glGetUniformLocation(_panoProgramID, "MVP");
    glUniformMatrix4fv(matrixID, 1, GL_FALSE, &mvp[0][0]);
    drawTriangle(_panoProgramID, vertexLeft);
    drawTriangle(_panoProgramID, vertexRight);

    if (!_isSingleView) {
        uvOffset = glm::vec4(0, 0, 0, 0);
        uvOffsetID = glGetUniformLocation(_panoProgramID, "uvOffset");
        glUniform4fv(uvOffsetID, 1, &uvOffset[0]);
        
    //    if (_w/2 > _h) {
    //        glViewport(_w/2 + (_w/2-_h)/2, 0, _h, _h);
    //    } else {
    //        glViewport(_w/2, (_h-_w/2)/2, _w/2, _w/2);
    //    }
        glViewport(_w/2, 0, _w/2, _h);
        
        float rotateAngle = -_rotateDegree/180.0*glm::pi<float>();
        direction = glm::rotate(direction, rotateAngle, headUpDir);
        computeTriangle(direction, headUpDir, angle, rate, vertexLeft, vertexRight, &mvp);
        
        matrixID = glGetUniformLocation(_panoProgramID, "MVP");
        glUniformMatrix4fv(matrixID, 1, GL_FALSE, &mvp[0][0]);
        drawTriangle(_panoProgramID, vertexLeft);
        drawTriangle(_panoProgramID, vertexRight);
    }
    
    static int count = 0;
    static time_t ts = time(nullptr);
    if (time(nullptr) != ts) {
        LOGV("frameRate render360: %d, width: %d, height: %d", count, _texWidth, _texHeight);
        count = 0;
        ts = time(nullptr);
    } else {
        count++;
    }
}

void Renderer::setLandscape(bool isLeft) {
    _isLandscapeLeft = isLeft;
}
    
void Renderer::renderPlane() {
    const GLfloat cVertexBufferDataLeft[] = {
        -1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        -1.0f, -1.0, 1.0,
    };
    const GLfloat cVertexBufferDataRight[] = {
        1.0f, -1.0f, 1.0f,
        -1.0f, -1.0, 1.0,
        1.0f, 1.0f, 1.0f,
    };
    glm::mat4 matProjection = glm::ortho(-1.0f,
                                         1.0f,
                                         -1.0f,
                                         1.0f,
                                         0.0f,
                                         1.0f); // In world coordinates
    glm::vec3 headUpDir = glm::vec3(0.0, 1.0, 0.0);
    float uOffsetLeft = _uOffset;
    float uOffsetRight = 0.0;
    if (!_isLandscapeLeft) {
        headUpDir = glm::vec3(0.0, -1.0, 0.0);
        swap(uOffsetLeft, uOffsetRight);
    }
    glm::mat4 matView = glm::lookAt(
                                    glm::vec3(0,0,0), // Camera is at (4,3,3), in World Space
                                    glm::vec3(0,0,1.0f), // and looks at the origin
                                    headUpDir  // Head is up (set to 0,-1,0 to look upside-down)
                                    );

    glm::mat4 mvp = matProjection * matView;
    
    GLuint matrixID = glGetUniformLocation(_planeProgramID, "MVP");
    glUniformMatrix4fv(matrixID, 1, GL_FALSE, &mvp[0][0]);
    
    GLuint uWidthID = glGetUniformLocation(_planeProgramID, "uWidth");
    glUniform1fv(uWidthID, 1, &_uWidth);
    
//    if (_w/2 > _h) {
//        glViewport((_w/2-_h)/2, 0, _h, _h);
//    } else {
//        glViewport(0, (_h-_w/2)/2, _w/2, _w/2);
//    }
    glViewport(0, 0, _w/2, _h);
    GLuint uOffsetID = glGetUniformLocation(_planeProgramID, "uOffset");
    glUniform1fv(uOffsetID, 1, &uOffsetLeft);
    drawTriangle(_planeProgramID, cVertexBufferDataLeft);
    drawTriangle(_planeProgramID, cVertexBufferDataRight);
    
//    if (_w/2 > _h) {
//        glViewport(_w/2 + (_w/2-_h)/2, 0, _h, _h);
//    } else {
//        glViewport(_w/2, (_h-_w/2)/2, _w/2, _w/2);
//    }
    glViewport(_w/2, 0, _w/2, _h);
    uOffsetID = glGetUniformLocation(_planeProgramID, "uOffset");
    glUniform1fv(uOffsetID, 1, &uOffsetRight);
    drawTriangle(_planeProgramID, cVertexBufferDataLeft);
    drawTriangle(_planeProgramID, cVertexBufferDataRight);
    
    static int count = 0;
    static time_t ts = time(nullptr);
    if (time(nullptr) != ts) {
        LOGV("frameRate renderPlane: %d, width: %d, height: %d", count, _texWidth, _texHeight);
        count = 0;
        ts = time(nullptr);
    } else {
        count++;
    }
}

void Renderer::drawTriangle(GLuint programID, const GLfloat* vertexBufferData) {
    // This will identify our vertex buffer
    GLuint vertexbuffer;
    // Generate 1 buffer, put the resulting identifier in vertexbuffer
    glGenBuffers(1, &vertexbuffer);
    // The following commands will talk about our 'vertexbuffer' buffer
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    // Give our vertices to OpenGL.
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 9, vertexBufferData, GL_STATIC_DRAW);

    // 1rst attribute buffer : vertices
    glEnableVertexAttribArray(0);
    //glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    GLuint location = glGetAttribLocation(programID, "vertexPosition_modelspace");
    glVertexAttribPointer(
       location,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
       3,                  // size
       GL_FLOAT,           // type
       GL_FALSE,           // normalized?
       0,                  // stride
       (void*)0            // array buffer offset
    );
    // Draw the triangle !
    glDrawArrays(GL_TRIANGLES, 0, 3); // Starting from vertex 0; 3 vertices total -> 1 triangle
    glDisableVertexAttribArray(0);

	glDeleteBuffers(1, &vertexbuffer);
}

GLuint Renderer::LoadShaders(const char * VertexSourcePointer,const char * FragmentSourcePointer) {
	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Compile Vertex Shader
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if ( InfoLogLength > 0 ){
		std::vector<char> VertexShaderErrorMessage(InfoLogLength+1);
		glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
		LOGV("VertexShaderErrorMessage: %s\n", &VertexShaderErrorMessage[0]);
	}

	// Compile Fragment Shader
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if ( InfoLogLength > 0 ){
		std::vector<char> FragmentShaderErrorMessage(InfoLogLength+1);
		glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
		LOGV("FragmentShaderErrorMessage: %s\n", &FragmentShaderErrorMessage[0]);
	}

	// Link the program
	LOGV("Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if ( InfoLogLength > 0 ){
		std::vector<char> ProgramErrorMessage(InfoLogLength+1);
		glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
		LOGV("ProgramErrorMessage: %s\n", &ProgramErrorMessage[0]);
	}

	glDetachShader(ProgramID, VertexShaderID);
	glDetachShader(ProgramID, FragmentShaderID);

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}
