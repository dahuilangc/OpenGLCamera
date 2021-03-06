//
// Created by yongfali on 2016/2/25.
//

#ifndef OPENGLCAMERA_GLTWOPASSFILTER_H
#define OPENGLCAMERA_GLTWOPASSFILTER_H

namespace e {

class GLTwoPassFilter
    : public GLShaderFilter{
public:
    //@initialize
    virtual bool Initialize(const char* firstStageVertexShaderString
        , const char* firstStageFragmentShaderString
        , const char* secondStageVertexShaderString
        , const char* secondStageFragmentShaderString);

    virtual bool Initialize(const char* firstStageFragmentShaderString
        ,const char* secondStageFragmentShaderString);

    void InitializeSecondaryAttributes(void);
    GLFramebuffer* FramebufferForOutput(void);
    void RemoveOutputFramebuffer(void);
    virtual void SetUniforms(int programIndex);
protected:
    GLProgram* _secondFilterProgram;
    GLFramebuffer* _secondOutputFramebuffer;
    GLint _secondFilterPositionAttribute;
    GLint _secondFilterTextureCoordinateAttribute;
    GLint _secondFilterInputTextureUniform;
    GLint _secondFilterInputTextureUniform2;
};

}

#endif //OPENGLCAMERA_GLTWOPASSFILTER_H
