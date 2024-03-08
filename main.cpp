#include "window.h"
#include "ChunkRenderer.h"

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	Window window(1600, 900, L"test", L"NES emulator");
	ChunkRenderer renderer(window.GetWindowHWND());
	Chunk* chunk = CreateChunk(&renderer);
	Camera* cam = CreateCamera(&renderer, { 0,0,-2.5 }, { 0,0,1 }, { 0,1,0 });
	renderer.BindCamera(cam);

	UpdateGpuMemory(chunk);
	while (window.ProcessMessages() == 0)
	{
		renderer.StartRecording();
		renderer.DrawChunk(chunk);
		renderer.StopRecording();
	}
}