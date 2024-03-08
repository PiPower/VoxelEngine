#include "window.h"
#include "ChunkRenderer.h"

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	Window window(1600, 900, L"test", L"NES emulator");
	ChunkRenderer renderer(window.GetWindowHWND());
	Chunk* chunk = CreateChunk(&renderer);
	while (window.ProcessMessages() == 0)
	{
		renderer.StartRecording();
		renderer.testDraw();
		renderer.StopRecording();
	}
}