#pragma once

#include "window.h"
#include <dxgi1_4.h>
#include <d3d12.h>
#include <wrl.h>
#include <DirectXMath.h>
#include <vector>

void not_succeded(HRESULT hr, int line, const char* path);
#define NOT_SUCCEEDED(hr) if(hr != S_OK) {not_succeded(hr, __LINE__, __FILE__ ) ;}

using namespace  Microsoft::WRL;
using namespace DirectX;
using namespace std;

class DeviceResources
{
public:
	DeviceResources(HWND hwnd, int frame_count = 3);
	void static Resize(HWND window, void* object);
	D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView();
	D3D12_CPU_DESCRIPTOR_HANDLE CurrentDepthBufferView();
	void CreateTextureDH(ID3D12DescriptorHeap** heap, int count, ID3D12Resource** array_of_resources);
	void UploadTexture(int width, int height, float* data, ID3D12Resource* texture, D3D12_RESOURCE_STATES original_state = D3D12_RESOURCE_STATE_GENERIC_READ);
	void CreateDefaultBuffer(ID3D12Resource** Resource, int size, 
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATES initState = D3D12_RESOURCE_STATE_COMMON);
	void CreateUploadBuffer(ID3D12Resource** Resource, int size, 
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATES initState = D3D12_RESOURCE_STATE_GENERIC_READ);
	void CreateTexture2D(ID3D12Resource** Resource, int width, int height, int mipLevels = 1,
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE, DXGI_FORMAT format = DXGI_FORMAT_R32G32B32_FLOAT);
	ID3D12Resource* CreateDefaultBuffer(int size, 
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATES initState = D3D12_RESOURCE_STATE_COMMON);
	ID3D12Resource* CreateUploadBuffer(int size, 
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATES initState = D3D12_RESOURCE_STATE_GENERIC_READ);
	ID3D12Resource* CreateTexture2D(int width, int height, int mipLevels = 1, 
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE, DXGI_FORMAT format = DXGI_FORMAT_R32G32B32_FLOAT);
	ID3D12DescriptorHeap* CreateTextureDH(int count, ID3D12Resource** array_of_resources);
	void Synchronize();
	virtual ~DeviceResources(){}
protected:
	void CreateCommandAllocatorListQueue();
	void CreateDescriptorHeaps();
	virtual void Resize(HWND hwnd);
	void CreateSwapChain(HWND hwnd);
	float AspectRatio();
protected:
	ComPtr<IDXGIFactory4> factory;
	ComPtr<IDXGISwapChain3> SwapChain;
	ComPtr<ID3D12Device5> Device;
	ComPtr<ID3D12DescriptorHeap> rtvHeap;
	ComPtr<ID3D12DescriptorHeap> depthHeap;
	vector<ComPtr<ID3D12Resource> > renderTargets;
	ComPtr<ID3D12CommandQueue> CommandQueue;
	ComPtr<ID3D12CommandAllocator> CommandAllocator;
	ComPtr<ID3D12GraphicsCommandList4> CommandList;
	ComPtr<ID3D12Resource> DepthStencilBuffer;

	D3D12_VIEWPORT ViewPort;
	D3D12_RECT ScissorRect;

	HANDLE FenceEvent;
	ComPtr<ID3D12Fence> Fence;
	UINT64 FenceValue;

	int width, height;
	int frame_count, current_frame;
	int rtv_heap_size, cbv_srv_uav_heap_size;
	int dsv_heap_size,sampler_heap_size;

	DXGI_FORMAT backBufferFormat;
	DXGI_FORMAT DsvFormat;
	UINT MsaaQualityLevel;

};


