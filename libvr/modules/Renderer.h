#ifndef __RENDERER_H__
#define __RENDERER_H__

#include "platform/GL.h"

#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
#include "GraphicBuffer.h"
#endif

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>

class RendererDelegate;
class Renderer {
public:
    enum PixFmt {YUV420P, NV12, NV21, RGB};
private:
    static const int TEXTURE_NUM = 5;
    int _w, _h;
    GLuint _panoProgramID;
    GLuint _planeProgramID;
    GLuint _texturesPanoId[TEXTURE_NUM];
    GLuint _texturesPlaneId[TEXTURE_NUM];
    int _texWidth, _texHeight;
    PixFmt _texPixFmt;
    RendererDelegate * _delegate;
    volatile bool _inited;
    volatile bool _initedGL;
    volatile bool _readyOpen;
    volatile bool _opened;
    volatile bool _readyClose;
    volatile bool _loadedTexture;
    float _uvHeight;
    float _uWidth;
    float _uOffset;
    bool _is360;
    bool _isSingleView;
    int _rotateDegree;
    int _viewPortDegree;
    bool _isLandscapeLeft;
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
    EGLDisplay _eglDisplay;
    EGLImageKHR _imageY;
    EGLImageKHR _imageU;
    EGLImageKHR _imageV;
    GraphicBuffer *_graphicBufY;
    GraphicBuffer *_graphicBufU;
    GraphicBuffer *_graphicBufV;
#endif
    
public:
    Renderer();
    void init(int winWidth, int winHeight);
    void setDelegate(RendererDelegate *delegate);
    void open(int texWidth, int texHeight);
    bool isOpened();
    void render(float viewMatrix[]);
    void close();
    void set360UpDown();
    void set360();
    void set360SingleView();
    void setPlane();
    void set3D();
    void set3DLeftRight();
    void setRotateDegree(int degree);
    void setViewPortDegree(int degree);
    void setLandscape(bool isLeft);
    
private:
    void safeClose();
    bool open();
    void initGL();
    void drawTriangle(GLuint programID, const GLfloat* vertexBufferData);
    GLuint LoadShaders(const char * VertexSourcePointer,const char * FragmentSourcePointer);
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
    void genTexture(int texWidth, int texHeight, GLuint *texId, GLenum tex, GLenum texIndex, GraphicBuffer **gb, EGLImageKHR *image, const char * texSamplerName);
#else
    void genTexture(int texWidth, int texHeight, GLuint *texId, GLenum tex, GLenum texIndex, const char * texSamplerName);
#endif
    bool loadTexture();
    void computeTriangle(glm::vec3 direction, glm::vec3 headUpDir, float angle, float widthHeightRate, float vertexLeft[], float vertexRight[], glm::mat4 *mvp);
    void render360(float viewMatrix[]);
    void renderPlane();
    void genTextures(GLuint texturesId[]);
};

class RendererDelegate {
    public:
    virtual void consumeTexture() = 0;
    virtual bool getTexture(unsigned char **dataY, unsigned char **dataU, unsigned char **dataV, Renderer::PixFmt *pixFmt) = 0;
};

#endif  // __RENDERER_H__
