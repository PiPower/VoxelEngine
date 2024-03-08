#include "DeviceResources.h"
#include <iostream>
#include <string>
#include <d3dcompiler.h>
#include <array>
#include <random>
#include "d3dx12.h"
#include <comdef.h>
#include "d3dx12.h"
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")
#define NOT_SUCCEEDED(hr) if(hr != S_OK) {not_succeded(hr, __LINE__, __FILE__ ) ;}
using namespace std;

void not_succeded(HRESULT hr, int line, const char* path)
{
    _com_error err(hr);
    wstring error_msg = L"INVALID OPERATION\n";
    error_msg.append(L"LINE: ");
    error_msg.append(to_wstring(line));
    error_msg.append(L"\n");
    error_msg.append(L"FILE: ");
    wchar_t error_code[200];
    size_t char_count;
    mbstowcs_s(&char_count, error_code, (200 / sizeof(WORD)) * sizeof(wchar_t), __FILE__, 200);
    error_msg.append(error_code);
    error_msg.append(L"\n");
    error_msg.append(L"ERROR DESCRIPTION:\n");
    error_msg.append(err.ErrorMessage());
    MessageBox(NULL, error_msg.c_str(), NULL, MB_OK);
    exit(0);
}

DeviceResources::DeviceResources(HWND hwnd,int frame_count)
    :
    frame_count(frame_count), current_frame(0), renderTargets(frame_count),
    backBufferFormat(DXGI_FORMAT_R8G8B8A8_UNORM), DsvFormat(DXGI_FORMAT_D24_UNORM_S8_UINT)
{
#if defined(_DEBUG)
    // Enable the D3D12 debug layer.
    {
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();
        }
    }
#endif
    CreateDXGIFactory1(IID_PPV_ARGS(&factory));

    ComPtr<IDXGIAdapter> gpuAdapter;
    if (SUCCEEDED(factory->EnumAdapters(0, &gpuAdapter)))
    {
        DXGI_ADAPTER_DESC adapterDesc;
        gpuAdapter->GetDesc(&adapterDesc);
        OutputDebugString(L"CHOSEN ADAPTER IS: ");
        OutputDebugString(adapterDesc.Description);
        OutputDebugString(L"\n ");
    }
    else
    {
        OutputDebugString(L"INAVLID CALL TO EnumAdapters \n");
        exit(0);
    }

    NOT_SUCCEEDED(D3D12CreateDevice(gpuAdapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&Device)))

    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
    msQualityLevels.Format = backBufferFormat;
    msQualityLevels.SampleCount = 4;
    msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    msQualityLevels.NumQualityLevels = 0;
    NOT_SUCCEEDED(Device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msQualityLevels, sizeof(msQualityLevels)));
    MsaaQualityLevel = msQualityLevels.NumQualityLevels;

    NOT_SUCCEEDED(Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence)));
    FenceValue = 1;
    // Create an event handle to use for frame synchronization.
    FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (FenceEvent == nullptr)
    {
        NOT_SUCCEEDED(HRESULT_FROM_WIN32(GetLastError()));
    }

    rtv_heap_size = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    cbv_srv_uav_heap_size = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    dsv_heap_size = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    sampler_heap_size = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

    CreateCommandAllocatorListQueue();
    CreateDescriptorHeaps();
    CreateSwapChain(hwnd);
    Resize(hwnd);
}

void DeviceResources::Synchronize()
{
    // Signal and increment the fence value.
    const UINT64 fence = FenceValue;
    NOT_SUCCEEDED(CommandQueue->Signal(Fence.Get(), fence));
    FenceValue++;

    // Wait until the previous frame is finished.
    if (Fence->GetCompletedValue() < fence)
    {
        NOT_SUCCEEDED(Fence->SetEventOnCompletion(fence, FenceEvent));
        WaitForSingleObject(FenceEvent, INFINITE);
    }
}

void DeviceResources::CreateCommandAllocatorListQueue()
{
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    NOT_SUCCEEDED(Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&CommandQueue)));

    NOT_SUCCEEDED(Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&CommandAllocator)));

    NOT_SUCCEEDED(Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, CommandAllocator.Get(),
        nullptr, IID_PPV_ARGS(&CommandList)));
    CommandList->Close();
}

void DeviceResources::CreateDescriptorHeaps()
{
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = frame_count;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    NOT_SUCCEEDED(Device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap)));

    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    dsvHeapDesc.NodeMask = 0;
    NOT_SUCCEEDED(Device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&depthHeap)));
}

void DeviceResources::Resize(HWND window, void* object)
{
    DeviceResources* deviceResources = (DeviceResources*)object;
    deviceResources->Resize(window);
}

D3D12_CPU_DESCRIPTOR_HANDLE DeviceResources::CurrentBackBufferView()
{
    D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
    current_frame = SwapChain->GetCurrentBackBufferIndex();
    handle.ptr += rtv_heap_size * current_frame;
    return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE DeviceResources::CurrentDepthBufferView()
{
    D3D12_CPU_DESCRIPTOR_HANDLE handle = depthHeap->GetCPUDescriptorHandleForHeapStart();
    return handle;
}

void DeviceResources::CreateTextureDH(ID3D12DescriptorHeap** heap, int count, ID3D12Resource** array_of_resources)
{
    *heap = CreateTextureDH(count, array_of_resources);
}

void DeviceResources::UploadTexture(int width, int height, float* data, ID3D12Resource* texture, D3D12_RESOURCE_STATES original_state)
{

    ID3D12Resource* upload_buffer;
    UINT* map;
    D3D12_RANGE read_range = { 0,0 };
    upload_buffer = CreateUploadBuffer(width * height * sizeof(float) * 3);
    upload_buffer->Map(0, &read_range,(void**) & map);
    memcpy(map, data, width * height * sizeof(float) * 3);

    D3D12_TEXTURE_COPY_LOCATION source, dest;
    source.pResource = upload_buffer;
    source.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    source.PlacedFootprint.Offset = 0;
    source.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R32G32B32_FLOAT;
    source.PlacedFootprint.Footprint.Width = width;
    source.PlacedFootprint.Footprint.Height = height;
    source.PlacedFootprint.Footprint.Depth = 1;
    source.PlacedFootprint.Footprint.RowPitch = width  * sizeof(float) * 3;

    dest.pResource = texture;
    dest.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dest.SubresourceIndex = 0;

    D3D12_RESOURCE_BARRIER transistion_copy_targets[2]{
        CD3DX12_RESOURCE_BARRIER::Transition(texture, original_state, D3D12_RESOURCE_STATE_COPY_DEST),
        CD3DX12_RESOURCE_BARRIER::Transition(upload_buffer, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_SOURCE)
    };
    
    D3D12_RESOURCE_BARRIER restore_original[2] = {
        CD3DX12_RESOURCE_BARRIER::Transition(texture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ),
        CD3DX12_RESOURCE_BARRIER::Transition(upload_buffer, D3D12_RESOURCE_STATE_COPY_SOURCE, original_state)
    };

    NOT_SUCCEEDED(CommandAllocator->Reset());
    NOT_SUCCEEDED(CommandList->Reset(CommandAllocator.Get(), nullptr));

    CommandList->ResourceBarrier(2, transistion_copy_targets);
    CommandList->CopyTextureRegion(&dest, 0, 0, 0, &source, nullptr);
    CommandList->ResourceBarrier(2, restore_original);

    NOT_SUCCEEDED(CommandList->Close());
    ID3D12CommandList* ppCommandLists[] = { CommandList.Get() };
    CommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
    this->Synchronize();
    upload_buffer->Release();
}

void DeviceResources::CreateDefaultBuffer(ID3D12Resource** Resource, int size, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initState)
{
    *Resource = CreateDefaultBuffer(size, flags, initState);
}

void DeviceResources::CreateUploadBuffer(ID3D12Resource** Resource, int size, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initState)
{
    *Resource = CreateUploadBuffer(size, flags, initState);
}

void DeviceResources::CreateTexture2D(ID3D12Resource** Resource, int width, int height, int mipLevels,
    D3D12_RESOURCE_FLAGS flags, DXGI_FORMAT format)
{
    *Resource = CreateTexture2D(width, height, mipLevels, flags, format);
}

ID3D12Resource* DeviceResources::CreateDefaultBuffer(int size, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initState)
{
    ID3D12Resource* createdResource;

    D3D12_RESOURCE_DESC buffDesc = {};
    buffDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    buffDesc.Alignment = 0;
    buffDesc.Width = size;
    buffDesc.Height = 1;
    buffDesc.MipLevels = 1;
    buffDesc.DepthOrArraySize = 1;
    buffDesc.SampleDesc.Count = 1;
    buffDesc.Format = DXGI_FORMAT_UNKNOWN;
    buffDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    buffDesc.Flags = flags;

    D3D12_HEAP_PROPERTIES heap_prop_buffers = {};
    heap_prop_buffers.Type = D3D12_HEAP_TYPE_DEFAULT;
    heap_prop_buffers.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heap_prop_buffers.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heap_prop_buffers.CreationNodeMask = 1;
    heap_prop_buffers.VisibleNodeMask = 1;

    NOT_SUCCEEDED(Device->CreateCommittedResource(&heap_prop_buffers, D3D12_HEAP_FLAG_NONE,
        &buffDesc, initState, nullptr, IID_PPV_ARGS(&createdResource)))
        return createdResource;
}

ID3D12Resource* DeviceResources::CreateUploadBuffer(int size, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initState)
{
    ID3D12Resource* createdResource;

    D3D12_RESOURCE_DESC buffDesc = {};
    buffDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    buffDesc.Alignment = 0;
    buffDesc.Width = size;
    buffDesc.Height = 1;
    buffDesc.MipLevels = 1;
    buffDesc.DepthOrArraySize = 1;
    buffDesc.SampleDesc.Count = 1;
    buffDesc.Format = DXGI_FORMAT_UNKNOWN;
    buffDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    buffDesc.Flags = flags;

    D3D12_HEAP_PROPERTIES heap_prop_upload_buffer = {};
    heap_prop_upload_buffer.Type = D3D12_HEAP_TYPE_UPLOAD;
    heap_prop_upload_buffer.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heap_prop_upload_buffer.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heap_prop_upload_buffer.CreationNodeMask = 1;
    heap_prop_upload_buffer.VisibleNodeMask = 1;

    NOT_SUCCEEDED(Device->CreateCommittedResource(&heap_prop_upload_buffer, D3D12_HEAP_FLAG_NONE,
        &buffDesc, initState, nullptr, IID_PPV_ARGS(&createdResource)))
    return createdResource;
}

ID3D12Resource* DeviceResources::CreateTexture2D(int width, int height, int mipLevels, D3D12_RESOURCE_FLAGS flags, DXGI_FORMAT format)
{
    ID3D12Resource* createdResource;

    D3D12_RESOURCE_DESC buffDesc = {};
    buffDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    buffDesc.Alignment = 0;
    buffDesc.Width = width;
    buffDesc.Height = height;
    buffDesc.MipLevels = mipLevels;
    buffDesc.DepthOrArraySize = 1;
    buffDesc.SampleDesc.Count = 1;
    buffDesc.Format = format;
    buffDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    buffDesc.Flags = flags;

    D3D12_HEAP_PROPERTIES heap_prop = {};
    heap_prop.Type = D3D12_HEAP_TYPE_DEFAULT;
    heap_prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heap_prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heap_prop.CreationNodeMask = 1;
    heap_prop.VisibleNodeMask = 1;

    NOT_SUCCEEDED(Device->CreateCommittedResource(&heap_prop, D3D12_HEAP_FLAG_NONE,
        &buffDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&createdResource)))
    return createdResource;
}

ID3D12DescriptorHeap* DeviceResources::CreateTextureDH(int count, ID3D12Resource** array_of_resources)
{
    ID3D12DescriptorHeap* descriptor_heap;
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.NumDescriptors = count;
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    desc.NodeMask = 0;
    
    NOT_SUCCEEDED(Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptor_heap) ));

    D3D12_CPU_DESCRIPTOR_HANDLE handle = descriptor_heap->GetCPUDescriptorHandleForHeapStart();
    for (int i = 0; i < count; i++)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
        srv_desc.Format = array_of_resources[i]->GetDesc().Format;
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srv_desc.Texture2D.MostDetailedMip = 0;
        srv_desc.Texture2D.MipLevels = -1;
        
        Device->CreateShaderResourceView(array_of_resources[i], &srv_desc, handle) ;
        handle.ptr = handle.ptr + cbv_srv_uav_heap_size;
    }
    return descriptor_heap;
}




void DeviceResources::Resize(HWND hwnd)
{
    Synchronize();

    RECT rc;
    GetClientRect(hwnd, &rc);
    this->width = rc.right - rc.left;
    this->height = rc.bottom - rc.top;

    this->ViewPort = { 0, 0,(float)this->width, (float)this->height, 0, 1.0 };

    this->ScissorRect = { 0, 0,  this->width, this->height };

    for (UINT n = 0; n < frame_count; n++)
    {
        renderTargets[n].Reset();
    }


    NOT_SUCCEEDED(SwapChain->ResizeBuffers(frame_count, this->width, this->height, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
    for (UINT n = 0; n < frame_count; n++)
    {
        NOT_SUCCEEDED(SwapChain->GetBuffer(n, IID_PPV_ARGS(&renderTargets[n])));
        Device->CreateRenderTargetView(renderTargets[n].Get(), nullptr, rtvHandle);
        rtvHandle.ptr += rtv_heap_size;
    }

    D3D12_RESOURCE_DESC depthStencilDesc;
    depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthStencilDesc.Alignment = 0;
    depthStencilDesc.Width = this->width;
    depthStencilDesc.Height = this->height;
    depthStencilDesc.DepthOrArraySize = 1;
    depthStencilDesc.MipLevels = 1;
    depthStencilDesc.Format = DsvFormat;
    depthStencilDesc.SampleDesc.Count = 1;
    depthStencilDesc.SampleDesc.Quality =0;
    depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE optClear;
    optClear.Format = DsvFormat;
    optClear.DepthStencil.Depth = 1.0f;
    optClear.DepthStencil.Stencil = 0;

    D3D12_HEAP_PROPERTIES heap_prop_depth = {};
    heap_prop_depth.Type = D3D12_HEAP_TYPE_DEFAULT;
    heap_prop_depth.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heap_prop_depth.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heap_prop_depth.CreationNodeMask = 1;
    heap_prop_depth.VisibleNodeMask = 1;

    NOT_SUCCEEDED(Device->CreateCommittedResource(
        &heap_prop_depth,
        D3D12_HEAP_FLAG_NONE,
        &depthStencilDesc,
        D3D12_RESOURCE_STATE_COMMON,
        &optClear,
        IID_PPV_ARGS(&DepthStencilBuffer)));

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
    dsvDesc.Format = DsvFormat;
    //dsvDesc.Texture2D.MipSlice = 0;
    Device->CreateDepthStencilView(DepthStencilBuffer.Get(), &dsvDesc, depthHeap->GetCPUDescriptorHandleForHeapStart());

    CommandList->Reset(CommandAllocator.Get(), nullptr);
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        DepthStencilBuffer.Get(),
        D3D12_RESOURCE_STATE_COMMON,
        D3D12_RESOURCE_STATE_DEPTH_WRITE);
    CommandList->ResourceBarrier(1, &barrier);

    NOT_SUCCEEDED(CommandList->Close());
    ID3D12CommandList* ppCommandLists[] = { CommandList.Get() };
    CommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
    this->Synchronize();

    this->current_frame = SwapChain->GetCurrentBackBufferIndex();
}

void DeviceResources::CreateSwapChain(HWND hwnd)
{
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferCount = frame_count;
    swapChainDesc.BufferDesc.Width = this->width;
    swapChainDesc.BufferDesc.Height = this->height;
    swapChainDesc.BufferDesc.Format = backBufferFormat;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.OutputWindow = hwnd;
    swapChainDesc.Windowed = TRUE;


    ComPtr<IDXGISwapChain> swapChain_buffer;
    NOT_SUCCEEDED( factory->CreateSwapChain(CommandQueue.Get(), &swapChainDesc, &swapChain_buffer) );

    NOT_SUCCEEDED(swapChain_buffer.As(&SwapChain));

    current_frame = SwapChain->GetCurrentBackBufferIndex();
}

float DeviceResources::AspectRatio()
{
    return (float)((float)this->width / (float)this->height);
}
