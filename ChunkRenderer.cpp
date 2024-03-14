#include "ChunkRenderer.h"
#include "d3dx12.h"
#include <d3dcompiler.h>
#include "ImageFile.h"

ChunkRenderer::ChunkRenderer(HWND hwnd)
	:
	DeviceResources(hwnd)
{
	ComPtr<ID3D12Resource> uploadBuffer;

	NOT_SUCCEEDED(CommandAllocator->Reset());
	NOT_SUCCEEDED(CommandList->Reset(CommandAllocator.Get(), nullptr));
	CompileShaders();
	CreateRootSignature();
	CreateLocalPipeline();
	CreateTexture(&uploadBuffer);
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
	float color[4] = {0.3, 0.3, 0.7, 1 };
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

	CommandList->SetDescriptorHeaps(1, textureHeap.GetAddressOf());
	CommandList->SetGraphicsRootDescriptorTable(2, textureHeap->GetGPUDescriptorHandleForHeapStart());
}


void ChunkRenderer::DrawChunk(Chunk* chunk)
{
	CommandList->IASetVertexBuffers(0, 1, &chunk->vertexView);
	CommandList->IASetIndexBuffer(&chunk->indexView);
	CommandList->SetGraphicsRootConstantBufferView(1, chunk->cbuffer->GetGPUVirtualAddress());
	CommandList->DrawIndexedInstanced(chunk->indexCount, 1, 0, 0, 0);
}

void ChunkRenderer::DrawGridOfChunks(ChunkGrid* grid)
{
	CommandList->IASetVertexBuffers(0, 1, &grid->gridOfChunks[0]->vertexView);
	for (int i = 0; i < grid->totalChunks; i++)
	{
		if (!IsInFrustum(&grid->gridOfChunks[i]->boundingVolume, this->camera)) { continue; }

		CommandList->IASetIndexBuffer(&grid->gridOfChunks[i]->indexView);
		CommandList->SetGraphicsRootConstantBufferView(1, grid->gridOfChunks[i]->cbuffer->GetGPUVirtualAddress());
		CommandList->DrawIndexedInstanced(grid->gridOfChunks[i]->indexCount, 1, 0, 0, 0);
	}
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

void ChunkRenderer::Resize(HWND hwnd, void* renderer)
{
	ChunkRenderer* chunkRenderer = (ChunkRenderer*)renderer;
	chunkRenderer->DeviceResources::Resize(hwnd);
}

void ChunkRenderer::CompileShaders()
{
#if defined(_DEBUG)
	// Enable better shader debugging with the graphics debugging tools.
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	CompileShader(&vs_shaderBlob,L"shaders.hlsl", NULL, NULL, "VS", "vs_5_0", compileFlags, 0);

	CompileShader(&ps_shaderBlob, L"shaders.hlsl", NULL, NULL, "PS", "ps_5_0", compileFlags, 0);

	CompileShader(&gs_shaderBlob, L"shaders.hlsl", NULL, NULL, "GS", "gs_5_0", compileFlags, 0);
}

void ChunkRenderer::CreateRootSignature()
{

	CD3DX12_DESCRIPTOR_RANGE1 texTable;
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	CD3DX12_ROOT_PARAMETER1 rootSlots[3];
	rootSlots[0].InitAsConstantBufferView(0);
	rootSlots[1].InitAsConstantBufferView(1);
	rootSlots[2].InitAsDescriptorTable(1, &texTable);

	CD3DX12_STATIC_SAMPLER_DESC pointWrap(
		0, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init_1_1(_countof(rootSlots), rootSlots, 1, &pointWrap,
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
	psoDesc.GS = { gs_shaderBlob->GetBufferPointer(), gs_shaderBlob->GetBufferSize() };
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

void ChunkRenderer::CreateTexture(ID3D12Resource** uploadBuffer)
{
	ImageFile image(L"./texture.png");
	CreateTexture2D(&texture, image.GetWidth(), image.GetHeight(), 1, D3D12_RESOURCE_FLAG_NONE, DXGI_FORMAT_R8G8B8A8_UNORM);
	CreateTextureDH(&textureHeap,1, texture.GetAddressOf());

	UINT64 bufferSize;

	textureLocation.pResource = texture.Get();
	textureLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	textureLocation.SubresourceIndex = 0;

	D3D12_RESOURCE_DESC textureDesc = texture->GetDesc();
	Device->GetCopyableFootprints(&textureDesc, 0, 1, 0, &bufferLocation.PlacedFootprint, nullptr, nullptr, &bufferSize);

	TextureData = new unsigned char[bufferSize]; // dim, dim, channel, faces
	for (int y = 0; y < image.GetHeight(); y++)
	{
		for (int x = 0; x < image.GetWidth(); x++)
		{
			int index = y * bufferLocation.PlacedFootprint.Footprint.RowPitch + x * 4;
			unsigned int colorData = image.GetColorAt(x, y);
			memcpy(TextureData + index, &colorData, sizeof(unsigned int));
			//data format BGRA
			//TextureData[index + 0] = (unsigned char) ((colorData >> 16 ) & 0xFF); 
			//TextureData[index + 1] = (unsigned char)((colorData >> 8) & 0xFF);
			//TextureData[index + 2] = (unsigned char)((colorData >> 0) & 0xFF);
			//TextureData[index + 3] = (unsigned char)((colorData >> 24) & 0xFF);
		}
	}


	UINT* mapPtr;
	CreateUploadBuffer(uploadBuffer, bufferSize);
	(*uploadBuffer)->Map(0, nullptr, (void**)&mapPtr);
	memcpy(mapPtr, TextureData, bufferSize);
	
	bufferLocation.pResource = (*uploadBuffer);
	bufferLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;

	// copy buffer to texture
	transitionTable[0] = CD3DX12_RESOURCE_BARRIER::Transition(texture.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST);
	transitionTable[1] = CD3DX12_RESOURCE_BARRIER::Transition((*uploadBuffer),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_SOURCE);

	//reverse ubove transition
	transitionTable[2] = CD3DX12_RESOURCE_BARRIER::Transition(texture.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
	transitionTable[3] = CD3DX12_RESOURCE_BARRIER::Transition((*uploadBuffer),
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_GENERIC_READ);

	CommandList->ResourceBarrier(2, transitionTable);
	CommandList->CopyTextureRegion(&textureLocation, 0, 0, 0, &bufferLocation, nullptr);
	CommandList->ResourceBarrier(2, transitionTable + 2);

	//delete[] TextureData;
}
