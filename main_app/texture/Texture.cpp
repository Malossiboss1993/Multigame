#include <main_app/texture/Texture.h>
#include <algorithm>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <main_app/texture/Texture2D.h>
#include <main_app/texture/TextureDDS.h>
#include <debug/Logger.h>

Texture::Texture(const std::string &strPath)
    : m_strPath(strPath),
      m_unTextureID(0),
      m_nWidth(0),
      m_nHeight(0),
      m_nChannelCount(0),
      m_nSourceX(0),
      m_nSourceY(0),
      m_nOriginalWidth(0),
      m_nOriginalHeight(0),
      m_eFlipFormat(EFlipFormat::eFlipNone),
      m_arrTexCoords{
          glm::vec2(0.0f, 1.0f),
          glm::vec2(1.0f, 1.0f),
          glm::vec2(1.0f, 0.0f),
          glm::vec2(0.0f, 0.0f)}
{
}

Texture::~Texture()
{
    Unbind();
    DeleteTexture();
}

std::shared_ptr<Texture> Texture::CreateTexture(const std::string &strPath)
{
    const std::string &strFileExtension = Utils::GetFileExtension(strPath);
    if (strFileExtension.compare(".dds") == 0)
    {
        return std::make_shared<TextureDDS>(strPath);
    }
    else
    {
        return std::make_shared<Texture2D>(strPath);
    }
}

bool Texture::Load(const std::string &strPath)
{
    if (m_unTextureID != 0)
    {
        LOG_ERROR("Texture \"{0}\" needs to be unloaded before \"{1}\" can be loaded!",
                  m_strPath, strPath);
        return false;
    }

    m_strPath = strPath;

    /* Load Texture Flipped - OpenGL reads the format from bottom to top */
    stbi_set_flip_vertically_on_load(true);
    /* Load the current texture (array of pixels) */
    m_ucData = stbi_load(strPath.c_str(), &m_nWidth, &m_nHeight,
                         &m_nChannelCount, 4);
    if (!m_ucData)
    {
        LOG_ERROR("Failed to load Texture2D \"{0}\"", m_strPath);
        return false;
    }

    m_nOriginalWidth = m_nWidth;
    m_nOriginalHeight = m_nHeight;

    SetSourceW(m_nWidth);
    SetSourceH(m_nHeight);

    /* Upload the image to OpenGL (the GPU) */
    glGenTextures(1, &m_unTextureID);
    glBindTexture(GL_TEXTURE_2D, m_unTextureID);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_nWidth,
                 m_nHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_ucData);

    stbi_image_free(m_ucData);
    m_ucData = nullptr;

    /* Generate mimpaps for texture */
    glGenerateMipmap(GL_TEXTURE_2D);
    /* Enable bilinear filtering */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    /* Enable texture wrapping */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    if (Utils::CheckOpenGLError())
    {
        LOG_ERROR("Failed to load Texture2D to OpenGL: \"{0}\"", m_strPath);
        return false;
    }

    LOG_INFO("Loaded Texture2D \"{0}\"", m_strPath);
    return true;
}

bool Texture::LoadSurface()
{
    m_mutexLoadSurface.lock();

    if (m_unTextureID != 0)
    {
        LOG_ERROR("Texture \"{0}\" needs to be unloaded before can be loaded!",
                  m_strPath);
        return false;
    }

    /* Load Texture Flipped - OpenGL reads the format from bottom to top */
    stbi_set_flip_vertically_on_load(true);
    /* Load the current texture (array of pixels) */
    m_ucData = stbi_load(m_strPath.c_str(), &m_nWidth, &m_nHeight,
                         &m_nChannelCount, 4);
    if (!m_ucData)
    {
        LOG_ERROR("Failed to load Texture2D \"{0}\"", m_strPath);
        return false;
    }

    m_nOriginalWidth = m_nWidth;
    m_nOriginalHeight = m_nHeight;

    SetSourceW(m_nWidth);
    SetSourceH(m_nHeight);

    m_mutexLoadSurface.unlock();

    return true;
}

bool Texture::LoadTextureFromSurface()
{
    /* Upload the image to OpenGL (the GPU) */
    glGenTextures(1, &m_unTextureID);
    glBindTexture(GL_TEXTURE_2D, m_unTextureID);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_nWidth,
                 m_nHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_ucData);

    stbi_image_free(m_ucData);
    m_ucData = nullptr;

    /* Generate mimpaps for texture */
    glGenerateMipmap(GL_TEXTURE_2D);
    /* Enable bilinear filtering */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    /* Enable texture wrapping */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    if (Utils::CheckOpenGLError())
    {
        LOG_ERROR("Failed to load Texture2D to OpenGL: \"{0}\"", m_strPath);
        return false;
    }

    LOG_INFO("Loaded Texture2D  from surface \"{0}\"", m_strPath);
    return true;
}

bool Texture::Load()
{
    return Load(m_strPath);
}

void Texture::SetFlipFormat(Texture::EFlipFormat eFlipFormat)
{
    m_eFlipFormat = eFlipFormat;
    SetTexCoords(m_eFlipFormat);
}

void Texture::SetWrapModeS(GLenum eWrapMode)
{
    if (m_unTextureID == 0)
    {
        return;
    }

    glBindTexture(GL_TEXTURE_2D, m_unTextureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, eWrapMode);
}

const glm::vec2 &Texture::GetTexCoord(Texture::ETexCoords eTexCoord) const
{
    return m_arrTexCoords.at(Utils::ToNum(eTexCoord));
}

void Texture::SetTexCoord(ETexCoords eTexCoord, const glm::vec2 &vec2TexCoord)
{
    m_arrTexCoords.at(Utils::ToNum(eTexCoord)) = vec2TexCoord;
}

void Texture::SetTexCoords(Texture::EFlipFormat eFlipFormat)
{
    switch (eFlipFormat)
    {
    case EFlipFormat::eFlipHorizontal:
        std::swap(m_arrTexCoords.at(Utils::ToNum(ETexCoords::eTopLeft)).x,
                  m_arrTexCoords.at(Utils::ToNum(ETexCoords::eTopRight)).x);
        std::swap(m_arrTexCoords.at(Utils::ToNum(ETexCoords::eBottomLeft)).x,
                  m_arrTexCoords.at(Utils::ToNum(ETexCoords::eBottomRight)).x);
        break;
    case EFlipFormat::eFlipVertical:
        std::swap(m_arrTexCoords.at(Utils::ToNum(ETexCoords::eTopLeft)).y,
                  m_arrTexCoords.at(Utils::ToNum(ETexCoords::eBottomLeft)).y);
        std::swap(m_arrTexCoords.at(Utils::ToNum(ETexCoords::eTopRight)).y,
                  m_arrTexCoords.at(Utils::ToNum(ETexCoords::eBottomRight)).y);
        break;
    case EFlipFormat::eFlipBoth:
        std::swap(m_arrTexCoords.at(Utils::ToNum(ETexCoords::eTopLeft)).x,
                  m_arrTexCoords.at(Utils::ToNum(ETexCoords::eTopRight)).x);
        std::swap(m_arrTexCoords.at(Utils::ToNum(ETexCoords::eBottomLeft)).x,
                  m_arrTexCoords.at(Utils::ToNum(ETexCoords::eBottomRight)).x);
        std::swap(m_arrTexCoords.at(Utils::ToNum(ETexCoords::eTopLeft)).y,
                  m_arrTexCoords.at(Utils::ToNum(ETexCoords::eBottomLeft)).y);
        std::swap(m_arrTexCoords.at(Utils::ToNum(ETexCoords::eTopRight)).y,
                  m_arrTexCoords.at(Utils::ToNum(ETexCoords::eBottomRight)).y);
        break;
    default:
        m_arrTexCoords =
            {
                glm::vec2(0.0f, 1.0f),
                glm::vec2(1.0f, 1.0f),
                glm::vec2(1.0f, 0.0f),
                glm::vec2(0.0f, 0.0f)};
        break;
    }
}

void Texture::Bind()
{
    if (m_unTextureID != 0)
    {
        glBindTexture(GL_TEXTURE_2D, m_unTextureID);
    }
    else
    {
        LOG_ERROR("Failed to BIND Texture: \"{0}\"!", m_strPath);
    }
}

void Texture::Unbind()
{
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::DeleteTexture()
{
    if (m_unTextureID != 0)
    {
        glDeleteTextures(1, &m_unTextureID);
        LOG_WARN("Deleting Texture: \"{0}\"", m_strPath);
        m_nWidth = 0;
        m_nHeight = 0;
        m_unTextureID = 0;
        m_strPath = "";
    }

    if (m_ucData)
    {
        LOG_WARN("Free Surface Data: \"{0}\"", m_strPath);
        stbi_image_free(m_ucData);
        m_ucData = nullptr;
    }
}
