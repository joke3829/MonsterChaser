#pragma once
#include "stdafx.h"

class CHeightMapImage {
public:
    CHeightMapImage(const wchar_t* filePath, int nWidth, int nLength, XMFLOAT3 xmf3Scale);
    float GetHeight(int x, int z);
    float GetHeightinWorldSpace(float x, float z);

    std::unique_ptr<WORD[]> m_pHeightMapPixels;

    int m_nWidth;
    int m_nLength;

    XMFLOAT3 m_xmf3Scale;
};