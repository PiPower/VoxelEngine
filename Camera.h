#pragma once

#include <d3d12.h>
#include <wrl.h>
#include "DeviceResources.h"

using namespace  Microsoft::WRL;
struct Camera
{
	void* cbufferMap;
	ComPtr<ID3D12Resource> CBuffer;

	XMFLOAT3 startPos;
	XMFLOAT3 lookDir;
	XMFLOAT3 upDir;
};

Camera* CreateCamera(DeviceResources* device, XMFLOAT3 startPos, XMFLOAT3 lookDir, XMFLOAT3 upDir, XMFLOAT2 n_f_planes = {0.5, 80});
