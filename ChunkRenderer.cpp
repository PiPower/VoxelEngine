#include "ChunkRenderer.h"
#include <d3dcompiler.h>
#include "d3dx12.h"

ChunkRenderer::ChunkRenderer(HWND hwnd)
	:
	DeviceResources(hwnd)
{
	NOT_SUCCEEDED(CommandAllocator->Reset());
	NOT_SUCCEEDED(CommandList->Reset(CommandAllocator.Get(), nullptr));
	CompileShaders();
	CreateRootSignature();
	CreateLocalPipeline();

	NOT_SUCCEEDED(CommandList->Close());
	ID3D12CommandList* ppCommandLists[] = { CommandList.Get() };
	CommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	this->Synchronize();
}

void ChunkRenderer::BindCamera(const Camera* camera)
{
	this->camera = camera;
}

void ChunkRenderer::StartRecording()
{
	float color[4] = {0, 0, 0, 1 };
	D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = CurrentBackBufferView();
	D3D12_CPU_DESCRIPTOR_HANDLE depth_hanlde = CurrentDepthBufferView();
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = renderTargets[current_frame].Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	NOT_SUCCEEDED(CommandAllocator->Reset());
	NOT_SUCCEEDED(CommandList->Reset(CommandAllocator.Get(), graphicsPipeline.Get()));

	CommandList->RSSetViewports(1, &ViewPort);
	CommandList->RSSetScissorRects(1, &ScissorRect);

	CommandList->ResourceBarrier(1, &barrier);
	CommandList->ClearRenderTargetView(rtv_handle, color, 0, nullptr);
	CommandList->ClearDepthStencilView(depth_hanlde, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1, 0, 0, nullptr);
	CommandList->OMSetRenderTargets(1, &rtv_handle, true, &depth_hanlde);
	CommandList->SetGraphicsRootSignature(rootSignature.Get());
	CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	CommandList->SetGraphicsRootConstantBufferView(0, camera->CBuffer->GetGPUVirtualAddress());
}


void ChunkRenderer::DrawChunk(Chunk* chunk)
{
	CommandList->IASetVertexBuffers(0, 1, &chunk->vertexView);
	CommandList->IASetIndexBuffer(&chunk->indexView);
	CommandList->DrawIndexedInstanced(chunk->indexCount, 1, 0, 0, 0);
}

void ChunkRenderer::StopRecording()
{
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = renderTargets[current_frame].Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	CommandList->ResourceBarrier(1, &barrier);
	CommandList->Close();

	ID3D12CommandList* lists[] = { CommandList.Get() };
	CommandQueue->ExecuteCommandLists(1, lists);

	SwapChain->Present(0, 0);
	current_frame = SwapChain->GetCurrentBackBufferIndex();
	Synchronize();
}

void ChunkRenderer::CompileShaders()
{
	ComPtr<ID3DBlob> error;
#if defined(_DEBUG)
	// Enable better shader debugging with the graphics debugging tools.
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	NOT_SUCCEEDED(D3DCompileFromFile(L"shaders.hlsl", NULL, NULL, "VS", "vs_5_0", compileFlags, 0, &vs_shaderBlob, &error));
	if (error.Get() != nullptr)
	{
		wchar_t error_code[400];
		size_t char_count;
		mbstowcs_s(&char_count, error_code, (400 / sizeof(WORD)) * sizeof(wchar_t), (char*)error->GetBufferPointer(), 400);
		if (vs_shaderBlob.Get() == nullptr)
		{
			MessageBox(NULL, error_code, NULL, MB_OK);
			exit(-1);
		}
		else
		{
			OutputDebugString(error_code);
		}
	}

	NOT_SUCCEEDED(D3DCompileFromFile(L"shaders.hlsl", NULL, NULL, "PS", "ps_5_0", compileFlags, 0, &ps_shaderBlob, &error));
	if (error.Get() != nullptr)
	{
		wchar_t error_code[400];
		size_t char_count;
		mbstowcs_s(&char_count, error_code, (400 / sizeof(WORD)) * sizeof(wchar_t), (char*)error->GetBufferPointer(), 400);
		if (ps_shaderBlob.Get() == nullptr)
		{
			MessageBox(NULL, error_code, NULL, MB_OK);
			exit(-1);
		}
		else
		{
			OutputDebugString(error_code);
		}
	}
}

void ChunkRenderer::CreateRootSignature()
{

	CD3DX12_ROOT_PARAMETER1 rootSlots[1];
	rootSlots[0].InitAsConstantBufferView(0);//placeholder for later 

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init_1_1(_countof(rootSlots), rootSlots, 0, nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;

	NOT_SUCCEEDED(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_1, &signature, &error));
	if (error.Get() != nullptr)
	{
		MessageBox(NULL, L"ROOT SIGNATURE ERROR", NULL, MB_OK);
		exit(-1);
	}
	NOT_SUCCEEDED(Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));
}

void ChunkRenderer::CreateLocalPipeline()
{
	// Define the vertex input layout.
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
	psoDesc.pRootSignature = rootSignature.Get();
	psoDesc.VS = { vs_shaderBlob->GetBufferPointer(), vs_shaderBlob->GetBufferSize() };
	psoDesc.PS = { ps_shaderBlob->GetBufferPointer(), ps_shaderBlob->GetBufferSize() };
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.RasterizerState.AntialiasedLineEnable = TRUE;
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;
	psoDesc.DSVFormat = DsvFormat;
	NOT_SUCCEEDED(Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&graphicsPipeline)));
}
