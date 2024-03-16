#include "window.h"
#include "ChunkRenderer.h"

void processUserInput(Camera* cam, Window* window);

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{

#if defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	Window window(1600, 900, L"test", L"Voxel world");
	ChunkRenderer renderer(window.GetWindowHWND());
	ChunkGrid* grid = CreateChunkGrid(&renderer, 100, 100, 10, 10);

	Camera* cam = CreateCamera(&renderer, { 0,0,0 }, { 0,0,1 }, { 0,1,0 });
	renderer.BindCamera(cam);

	window.RegisterResizezable(&renderer, ChunkRenderer::Resize);
	while (window.ProcessMessages() == 0)
	{
		renderer.StartRecording();
		renderer.DrawGridOfChunks(grid, cam);
		renderer.StopRecording();

		processUserInput(cam, &window);
	}
}



void processUserInput(Camera* cam, Window* window)
{
	constexpr float rot_tempo = 0.02;
	float x = 0, y = 0, z = 0, delta_x = 0, delta_y = 0;
	float delta_sigma = 0, delta_ro = 0, delta_beta = 0;
	static bool left_pressed = false;
	static int mouse_x, mouse_y;

	if (window->IsKeyPressed('W')) z += 0.03;
	if (window->IsKeyPressed('S')) z -= 0.03;
	if (window->IsKeyPressed('D')) x += 0.03;
	if (window->IsKeyPressed('A')) x -= 0.03;
	if (window->IsKeyPressed(32)) y += 0.03;
	if (window->IsKeyPressed(17)) y -= 0.03;
	if (window->IsKeyPressed('Z')) delta_sigma = 0.04;
	if (window->IsKeyPressed('X')) delta_ro = 0.04;
	if (window->IsKeyPressed('C')) delta_beta = 0.04;
	if (window->IsLeftPressed())
	{
		if (!left_pressed)
		{
			left_pressed = true;
			mouse_x = window->GetMousePosX();
			mouse_y = window->GetMousePosY();
		}
		else
		{
			//rotation round y
			delta_y += ((mouse_x - window->GetMousePosX() > 0) - (mouse_x - window->GetMousePosX() < 0)) * rot_tempo;
			mouse_x = window->GetMousePosX();
			//rotation round 
			delta_x = ((mouse_y - window->GetMousePosY() > 0) - (mouse_y - window->GetMousePosY() < 0)) * rot_tempo;
			mouse_y = window->GetMousePosY();
		}
	}
	else left_pressed = false;

	MoveCamera(cam, x, y, z, delta_x, delta_y);
}