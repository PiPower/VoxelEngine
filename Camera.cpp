#include "Camera.h"
#include <DirectXMath.h>

enum class CbufferOffsets
{
    viewTransform = 0,
    perspectiveProjection = 4*4*sizeof(float)
};


Camera* CreateCamera(DeviceResources* device, XMFLOAT3 startPos, XMFLOAT3 lookDir, XMFLOAT3 upDir, XMFLOAT2 n_f_planes)
{
    Camera* newCamera = new Camera();

    newCamera->lookDir = startPos;
    newCamera->lookDir = lookDir;
    newCamera->lookDir = upDir;

    int size = ceil(sizeof(XMFLOAT4X4) * 2.0/256) * 256;

    device->CreateUploadBuffer(&newCamera->CBuffer, size);
    D3D12_RANGE readRange = { 0,0 };
    newCamera->CBuffer->Map(0, &readRange, &newCamera->cbufferMap);
    XMVECTOR startPosVec = XMLoadFloat3(&startPos);
    XMVECTOR lookDirVec = XMLoadFloat3(&lookDir);
    XMVECTOR upDirVec = XMLoadFloat3(&upDir);

    XMMATRIX cameraTransform = XMMatrixLookToLH(startPosVec, lookDirVec, upDirVec);
    cameraTransform = XMMatrixTranspose(cameraTransform);
    XMFLOAT4X4 matrixBuff;
    XMStoreFloat4x4(&matrixBuff, cameraTransform);
    memcpy(newCamera->cbufferMap, &matrixBuff, 4 * 4 * sizeof(float));


    XMMATRIX perspectiveTransform = XMMatrixPerspectiveFovLH(3.14 / 2, device->AspectRatio(), n_f_planes.x, n_f_planes.y);
    perspectiveTransform = XMMatrixTranspose(perspectiveTransform);
    XMStoreFloat4x4(&matrixBuff, perspectiveTransform);
    memcpy((char*)newCamera->cbufferMap + (int)CbufferOffsets::perspectiveProjection, &matrixBuff, 4 * 4 * sizeof(float));
    return newCamera;
}
