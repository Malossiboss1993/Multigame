#ifndef TEXTURE_H_
#define TEXTURE_H_

#include <iostream>
#include <string>
#include <mutex>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <memory>
#include <array>

#include <debug/Utils.h>

class Texture
{
public:
    /**
     * @brief The EFlipFormat enum - specifies how the texture will be flipped
     * when rendered.
     */
    enum class EFlipFormat
    {
        eFlipHorizontal,
        eFlipVertical,
        eFlipBoth,
        eFlipNone
    };

    /**
     * @brief The ETexCoord enum - describes the four points of clip-rectangle,
     * used to sample a texture.
     */
    enum class ETexCoords
    {
        /* The top-left corner of the rectangle */
        eTopLeft,
        /* The top-right corner of the rectangle */
        eTopRight,
        /* The bottom-right corner of the rectangle */
        eBottomRight,
        /* The bottom-left corner of the rectangle */
        eBottomLeft,
        /* Total number of corners of this texture */
        eTotalCount
    };

    using ArrTexCoords = std::array<glm::vec2, Utils::ToNum(ETexCoords::eTotalCount)>;

    Texture(const std::string &strPath = "");
    /**
     * @brief ~Texture - virtual class destructor. Unbinds and deletes this texture.
     */
    virtual ~Texture();

    /**
     * @brief CreateTexture - creates a texture, based on the file extension of the path.
     * Note: Creating a new texture, will unbind and delete the old one!
     * @param strPath - path, used to initialize m_strPath.
     * @return shared pointer of the newly created Texture Instance.
     */
    static std::shared_ptr<Texture> CreateTexture(const std::string &strPath);
    /**
     * @brief Load - load an image file from the specified path. Its implementation
     * is child-specific.
     * @param strPath - the location of the image file.
     * @return if the loading was successful.
     */
    virtual bool LoadSurface();
    /**
     * DON'T FORGET TO CALL LoadTextureFromSurface(), due to free pixel data !!!
     * @brief LoadSurface - load an surface from the path, specied in m_strPath, thread safe
     * @return true if the load was successful.
     */
    virtual bool LoadTextureFromSurface();
    /**
     * @brief LoadTextureFromSurface - load a texture from already loaded surface specied in m_strPath
     * @return true if the load was successful.
     */
    virtual bool Load(const std::string &strPath);
    /**
     * @brief Load - load an image from the path, specied in m_strPath
     *  [calls Load(m_strPath)].
     * @return true if the load was successful.
     */
    virtual bool Load();
    /**
     * @brief GetTextureID - returns the current texture ID.
     * @return m_unTextureID.
     */
    /**
     * @brief GetPath - returns the file path of the current texture.
     * @return m_strPath
     */
    const std::string &GetPath() const { return m_strPath; }
    GLuint GetTextureID() const { return m_unTextureID; }
    /**
     * @brief GetWidth - returns the width of the current texture.
     * @return m_nWidth.
     */
    int GetWidth() const { return m_nWidth; }
    /**
     * @brief GetHeight - returns the height of the current texture.
     * @return m_nHeight.
     */
    int GetHeight() const { return m_nHeight; }

    /**
     * @brief GetOriginalWidth - returns the width of the current texture, never changes.
     * @return m_nOriginalWidth.
     */
    int GetOriginalWidth() const { return m_nOriginalWidth; }
    /**
     * @brief GetOriginalHeight - returns the height of the current texture, never changes.
     * @return m_nOriginalHeight.
     */
    int GetOriginalHeight() const { return m_nOriginalHeight; }

    /**
     * @brief GetFlipFormat - return the current texture flip format.
     * @return m_eFlipFormat.
     */
    EFlipFormat GetFlipFormat() const { return m_eFlipFormat; }
    /**
     * @brief SetFlipFormat - set a value to m_eFlipFormat
     * @param eFlipFormat - the current flip format to be used
     */
    void SetFlipFormat(EFlipFormat eFlipFormat);

    /**
     * @brief SetWrapModeS - sets the horizontal texture wrap mode.
     * @param eWrapMode - OpenGL wrap mode, for example GL_REPEAT.
     */
    void SetWrapModeS(GLenum eWrapMode);

    /**
     * @brief GetTextureCoords - return the texture coordinates, used to sample
     * from this texture.
     * @return m_arrTexCoords.
     */
    const ArrTexCoords &GetTexCoords() const { return m_arrTexCoords; }
    /**
     * @brief GetTexCoord - return a texture coordinate from m_arrTexCoords.
     * @param eTexCoord - index of the coordinate to be returned.
     * @return the value of the texture coordinate, specified by eTexCoord.
     */
    const glm::vec2 &GetTexCoord(ETexCoords eTexCoord) const;

    /**
     * @brief SetTexCoords - set the texture coordinates, used to sample from
     * this texture.
     * @param arrTexCoords - the new texture coordinates.
     */
    void SetTexCoords(const ArrTexCoords &arrTexCoords) { m_arrTexCoords = arrTexCoords; }

    /**
     * @brief SetTexCoord - set a texture coordinate from m_arrTexCoords.
     * @param eTexCoord - index of the coordinate to be set.
     * @param vec2TexCoord - the value of the texture coordinate, specified by
     * eTexCoord
     */
    void SetTexCoord(ETexCoords eTexCoord, const glm::vec2 &vec2TexCoord);

    /**
     * @brief SetTexCoords - set the current texture coordinates, based
     * on the passed flip format.
     * @param eFlipFormat - used to determine the current texture coordinates.
     */
    void SetTexCoords(EFlipFormat eFlipFormat);
    /**
     * @brief Bind - binds the current texture to be the current 2D Texture Target.
     */
    void Bind();
    /**
     * @brief Unbind - the current texture is no longer the current 2D Texture Target.
     */
    void Unbind();
    /**
     * @brief DeleteTexture - releases the resources of the current texture and
     * resets its fields.
     */
    void DeleteTexture();

    /**
     * @brief SetSourceX - set the current X coordinate of the rectangle,
     * used to sample from this texture.
     * @param nSourceX - new value of m_nSourceX.
     */
    inline void SetSourceX(int nSourceX) { m_nSourceX = nSourceX; }
    /**
     * @brief SetSourceY - set the current X coordinate of the rectangle,
     * used to sample from this texture.
     * @param nSourceY - new value of m_nSourceY.
     */
    inline void SetSourceY(int nSourceY) { m_nSourceY = nSourceY; }
    /**
     * @brief SetSourceW - set the current width of the rectangle,
     * used to sample from this texture.
     * @param nSourceW - new value of m_nSourceW.
     */
    inline void SetSourceW(int nSourceW) { m_nWidth = nSourceW; }
    /**
     * @brief SetSourceH - set the current height of the rectangle,
     * used to sample from this texture.
     * @param nSourceH - new value of m_nSourceH.
     */
    inline void SetSourceH(int nSourceH) { m_nHeight = nSourceH; }

    /**
     * @brief GetSourceX - return the current X coordinate of the rectangle,
     * used to sample from this texture.
     * @return m_nSourceX.
     */
    inline int GetSourceX() const { return m_nSourceX; }
    /**
     * @brief GetSourceX - return the current Y coordinate of the rectangle,
     * used to sample from this texture.
     * @return m_nSourceY.
     */
    inline int GetSourceY() const { return m_nSourceY; }
    /**
     * @brief GetSourceX - return the current height of the rectangle,
     * used to sample from this texture.
     * @return m_nSourceW.
     */
protected:
    /**
     * @brief m_mutexLoadSurface - prevent accessing method when already in use from another thread
     */
    std::mutex m_mutexLoadSurface;
    /**
     * @brief m_ucData - holds the physical pixels to surface
     */
    unsigned char *m_ucData = nullptr;
    /**
     * @brief m_strPath - holds the path to the physical image of the current
     * texture
     */
    std::string m_strPath;
    /**
     * @brief m_unTextureID - ID returned by the GPU for the current texture.
     */
    GLuint m_unTextureID;
    /**
     * @brief m_nWidth - width of the current texture.
     */
    int m_nWidth;
    /**
     * @brief m_nHeight - height of the current texture.
     */
    int m_nHeight;
    /**
     * @brief m_nOriginalWidth - width of the current texture, never changes.
     */
    int m_nOriginalWidth;
    /**
     * @brief m_nOriginalHeight - height of the current texture, never changes.
     */
    int m_nOriginalHeight;
    /**
     * @brief m_nChannelCount - number of channels in the current texture.
     */
    int m_nChannelCount;

    /**
     * @brief m_nSourceX - X position from which to sample from this texture.
     */
    int m_nSourceX = 0;
    /**
     * @brief m_nSourceY - Y position from which to sample from this texture.
     */
    int m_nSourceY = 0;

    /**
     * @brief m_eFlipFormat - specifies how the current texture will be flipped
     * when rendered.
     */
    EFlipFormat m_eFlipFormat;

    /**
     * @brief m_arrTexCoords - points, forming a rectangle from which
     * this texture will be sampled. They are in order of: top-left, top-right,
     * bottom-right, bottom left.
     */
    ArrTexCoords m_arrTexCoords;
};

#endif // TEXTURE_H_
