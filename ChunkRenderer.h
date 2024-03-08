#include "DeviceResources.h"
#include "Chunk.h"
#pragma once
class ChunkRenderer : public DeviceResources
{
public:
	ChunkRenderer(HWND hwnd);
	void StartRecording();
	void testDraw();
	void StopRecording();
protected:
	void CompileShaders();
	void CreateRootSignature();
	void CreateLocalPipeline();
private:
	ComPtr<ID3D12PipelineState> graphicsPipeline;
	ComPtr<ID3D12RootSignature> rootSignature;
	ComPtr<ID3DBlob> vs_shaderBlob;
	ComPtr<ID3DBlob> ps_shaderBlob;
	ComPtr<ID3D12Resource> test_vb;
};

