#include "stdafx.h"
#include "HeightMap.h"


CHeightMapImage::CHeightMapImage(const wchar_t* filePath, int nWidth, int nLength, XMFLOAT3 xmf3Scale)
{
    m_nWidth = nWidth;
    m_nLength = nLength;
    m_xmf3Scale = xmf3Scale;

    std::ifstream inFile{ filePath, std::ios::binary };
    std::vector<WORD> v(m_nWidth * m_nLength);
    inFile.read((char*)v.data(), sizeof(WORD) * m_nLength * m_nWidth);

    m_pHeightMapPixels = std::make_unique<WORD[]>(m_nWidth * m_nLength);
    for (int z = 0; z < m_nLength; ++z) {
        for (int x = 0; x < m_nWidth; ++x) {
            //m_pHeightMapPixels[x + ((m_nLength - 1 - z) * m_nWidth)] = v[x + (z * m_nWidth)];//pHeightMapPixels[x + (z * m_nWidth)];
            m_pHeightMapPixels[x + (z * m_nWidth)] = v[x + (z * m_nWidth)];
        }
    }
}

float CHeightMapImage::GetHeight(int x, int z)
{
    return m_pHeightMapPixels[x + (z * m_nWidth)] * m_xmf3Scale.y;
}

float CHeightMapImage::GetHeightinWorldSpace(float x, float z)
{
    // need to check
    float lx = x / m_xmf3Scale.x;
    float lz = z / m_xmf3Scale.z;

    if (lx < 0 || lx >= m_nWidth || lz < 0 || lz >= m_nLength)
        return 0.0f;

    int ix = static_cast<int>(lx);
    int iz = static_cast<int>(lz);

    float xRatio = lx - ix;
    float zRatio = lz - iz;

    auto myLerp = [](float& f1, float& f2, float ratio) {
        return ((1.0f - ratio) * f1) + ((ratio)*f2);
        };

    float x0z0 = GetHeight(ix, iz); float x1z0 = GetHeight(ix + 1, iz);
    float x0z1 = GetHeight(ix, iz + 1); float x1z1 = GetHeight(ix + 1, iz + 1);

    float lerp1 = myLerp(x0z0, x0z1, zRatio);
    float lerp2 = myLerp(x1z0, x1z1, zRatio);

    return myLerp(lerp1, lerp2, xRatio);
}