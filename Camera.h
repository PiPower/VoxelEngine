#pragma once

#include <d3d12.h>
#include <wrl.h>
#include "DeviceResources.h"
#include "Chunk.h"
using namespace  Microsoft::WRL;

struct Camera
{
	void* cbufferMap;
	ComPtr<ID3D12Resource> CBuffer;

	XMFLOAT3 eyePos;
	XMFLOAT3 lookDir;
	XMFLOAT3 upDir;
	XMFLOAT2 rot;

	XMFLOAT4X4 clipSpaceTransform;
	XMMATRIX perspectiveTransform;
	BoundingFrustum frustum;
};

Camera* CreateCamera(DeviceResources* device, XMFLOAT3 startPos, XMFLOAT3 lookDir, XMFLOAT3 upDir, XMFLOAT2 n_f_planes = {0.1, 80});
void MoveCamera(Camera* cam, float dx, float dy, float dz, float rotX, float rotY);
bool IsInFrustum(const BoundingVolumeSphere* boundingSphere, const Camera* camera);