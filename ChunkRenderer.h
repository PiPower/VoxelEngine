#include "DeviceResources.h"
#include "Chunk.h"
#include "Camera.h"
#pragma once
class ChunkRenderer : public DeviceResources
{
public:
	ChunkRenderer(HWND hwnd);
	void BindCamera(const Camera* camera);
	void StartRecording();
	void DrawChunk(Chunk* chunk);
	void StopRecording();
	static void Resize(HWND hwnd, void* renderer);
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
	const Camera* camera;
};

