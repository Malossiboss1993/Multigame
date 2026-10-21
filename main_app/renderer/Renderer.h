#pragma once
#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <main_app/texture/Texture.h>
#include <main_app/font/Font.h>
#include <shader/shader_m.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class Renderer
{

private:

    GLFWwindow* window = nullptr;

    GLuint VBO2D;
    GLuint VAO2D;
    GLuint EBO2D;

    GLuint VBO3D;
    GLuint VAO3D;
    GLuint EBO3D;

    GLuint VBOFont;
    GLuint VAOFont;

    /*Fixed-resolution render target. The scene is rendered at the design
      resolution and then scaled to the window, preserving the aspect ratio.*/
    GLuint m_unFramebuffer = 0;
    GLuint m_unFramebufferTexture = 0;

    /*Windowed placement saved while switching to/from fullscreen*/
    int m_nWindowedX = 0;
    int m_nWindowedY = 0;
    int m_nWindowedWidth = 1920;
    int m_nWindowedHeight = 1080;

    void CreateRenderTarget();

    /* TOP LEFT->  *---------------------------------* <-TOP RIGHT
                   |                                 |
                   |                                 |
                   |                                 |
                   |                                 |
    BOTTOM LEFT -> *---------------------------------* <-BOTTOM RIGHT */

    /*Every cicle send the coords to the video card via VBO*/
    glm::vec2 m_vec2TopRight = glm::vec2(0.0f, 0.0f);
    glm::vec2 m_vec2TopLeft = glm::vec2(0.0f, 0.0f);
    glm::vec2 m_vec2BottomLeft = glm::vec2(0.0f, 0.0f);
    glm::vec2 m_vec2BottomRight = glm::vec2(0.0f, 0.0f);

    glm::vec2 m_vec2TextCoordTopLeft = glm::vec2(0.0f, 1.0f);
    glm::vec2 m_vec2TextCoordTopRight = glm::vec2(1.0f, 1.0f);
    glm::vec2 m_vec2TextCoordBottomRight = glm::vec2(1.0f, 0.0f);
    glm::vec2 m_vec2TextCoordBottomLeft = glm::vec2(0.0f, 0.0f);

    glm::vec4 m_vColor = glm::vec4(1.0, 1.0f, 1.0f, 1.0f);

    /*Shaders*/
    Shader m_shaderPicture;
    Shader m_shaderCrop;
    Shader m_currentPictureShader;
    Shader m_shaderFont;

    void SetGLFWCallbacks();

public:

    Renderer();

    ~Renderer();

    unsigned int SCREEN_WIDTH = 1920;
    unsigned int SCREEN_HEIGHT = 1080;
    unsigned int SCREEN_DEPTH = 100;

    GLFWwindow* GetWindow(){ return window; }
    bool InitSystem(const std::string& strTitle,
                    unsigned int nScreenWidth, unsigned int nScreenHeight,
                    unsigned int nScreenDepth);
    void Deinit();
    void CreateAndFill2DBuffers();
    void CreateAndFill3DBuffers();
    void BeginFrame();
    void EndFrame();
    void CallEventsAndSwapBuffers();
    void GetWindowToLogicalTransform(float &fOffsetX, float &fOffsetY, float &fScale) const;
    void ToggleFullscreen();

    void UseShaderPicture();
    void UseShaderCrop(glm::vec4 vec4RectCrop);
    
    void SetColor(float fRed = 1.0f, float fGreen = 1.0f, float fBlue = 1.0f, float fAlpha = 1.0f);

    /*2D*/
    void DrawLine(float fXInitial, float fYInitial, float fXFinal, float fYFinal, float fThickness, Shader& shaderLine);
    void DrawLine(float fXInitial, float fYInitial, float fXFinal, float fYFinal, float fThickness, Shader& shaderLine, std::shared_ptr<Texture> ptrTexture);
    void DrawRect(float fX, float fY, float fWidth, float fHeight, Shader& shaderRect);
    void DrawText(std::string strText, std::shared_ptr<Font> ptrFont, float fX, float fY, float fScaleFactor = 1.0f);
    void DrawPicture(std::shared_ptr<Texture> ptrTexture, float fX, float fY);
    void DrawPictureScaled(std::shared_ptr<Texture> ptrTexture, float fX, float fY, float fScaleFactor);
    void DrawPicturePart(std::shared_ptr<Texture> ptrTexture, float fX, float fY,
                         unsigned int unSourceX, unsigned int unSourceY,
                         unsigned int unSourceWidth, unsigned int unSourceHeight,
                         float fScaleFactor = 1.0f);
    void DrawPictureRotated(std::shared_ptr<Texture> ptrTexture, float fX, float fY, float fDegrees, float fScaleFactor = 1.0f);
    void DrawPictureAroundPoint(std::shared_ptr<Texture> ptrTexture, float fX, float fY, float fDegrees, float fVertPivotFromCenter);


    /*3D*/
    void DrawPicture3D(std::shared_ptr<Texture> ptrTexture, Shader& shaderTexture, glm::mat4& matrixModel, glm::mat4& matrixView);

private:

    void Send2DGeometry(std::shared_ptr<Texture> ptrTexture);
    void Set2DTextureCoords(float fTexCoordLeft, float fTexCoordBottom,
                            float fTexCoordRight, float fTexCoordTop);

};
