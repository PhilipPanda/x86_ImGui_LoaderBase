// Made by PhilipPanda | https://github.com/PhilipPanda
#include "gui.h"
#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_dx9.h"
#include "../imgui/imgui_impl_win32.h"
#include "../imgui/imgui_internal.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
	HWND window,
	UINT message,
	WPARAM wideParameter,
	LPARAM longParameter
);

long __stdcall WindowProcess(
	HWND window,
	UINT message,
	WPARAM wideParameter,
	LPARAM longParameter)
{
	if (ImGui_ImplWin32_WndProcHandler(window, message, wideParameter, longParameter))
		return true;


	switch (message)
	{
	case WM_SIZE: {
		if (gui::device && wideParameter != SIZE_MINIMIZED)
		{
			gui::presentParameters.BackBufferWidth = LOWORD(longParameter);
			gui::presentParameters.BackBufferHeight = HIWORD(longParameter);
			gui::ResetDevice();
		}
	}return 0;

	case WM_SYSCOMMAND: {
		if ((wideParameter & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
	}break;

	case WM_DESTROY: {
		PostQuitMessage(0);
	}return 0;

	case WM_LBUTTONDOWN: {
		gui::position = MAKEPOINTS(longParameter); // set click points
	}return 0;

	case WM_MOUSEMOVE: {
		if (wideParameter == MK_LBUTTON)
		{
			const auto points = MAKEPOINTS(longParameter);
			auto rect = ::RECT{ };

			GetWindowRect(gui::window, &rect);

			rect.left += points.x - gui::position.x;
			rect.top += points.y - gui::position.y;

			if (gui::position.x >= 0 &&
				gui::position.x <= gui::WIDTH &&
				gui::position.y >= gui::position.y <= 19)
				SetWindowPos(
					gui::window,
					HWND_TOPMOST,
					rect.left,
					rect.top,
					0, 0,
					SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOZORDER
				);
		}

	}return 0;





	}

	return DefWindowProcW(window, message, wideParameter, longParameter);
}

void gui::CreateHWindow(
	const char* windowName,
	const char* className) noexcept
{
	windowClass.cbSize = sizeof(WNDCLASSEXA);
	windowClass.style = CS_CLASSDC;
	windowClass.lpfnWndProc = WindowProcess;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = GetModuleHandleA(0);
	windowClass.hIcon = 0;
	windowClass.hCursor = 0;
	windowClass.hbrBackground = 0;
	windowClass.lpszMenuName = 0;
	windowClass.lpszClassName = className;
	windowClass.hIconSm = 0;

	RegisterClassExA(&windowClass);

	window = CreateWindowA(
		className,
		windowName,
		WS_POPUP,
		100,
		100,
		WIDTH,
		HEIGHT,
		0,
		0,
		windowClass.hInstance,
		0
	);

	ShowWindow(window, SW_SHOWDEFAULT);
	UpdateWindow(window);

}

void gui::DestroyHWindow() noexcept
{
	DestroyWindow(window);
	UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);

}

bool gui::CreateDevice() noexcept
{
	d3d = Direct3DCreate9(D3D_SDK_VERSION);

	if (!d3d)
		return false;

	ZeroMemory(&presentParameters, sizeof(presentParameters));

	presentParameters.Windowed = TRUE;
	presentParameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
	presentParameters.BackBufferFormat = D3DFMT_UNKNOWN;
	presentParameters.EnableAutoDepthStencil = TRUE;
	presentParameters.AutoDepthStencilFormat = D3DFMT_D16;
	presentParameters.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

	if (d3d->CreateDevice(
		D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		window,
		D3DCREATE_HARDWARE_VERTEXPROCESSING,
		&presentParameters,
		&device) < 0)
		return false;

	return true;
}

void gui::ResetDevice() noexcept
{
	ImGui_ImplDX9_InvalidateDeviceObjects();

	const auto result = device->Reset(&presentParameters);

	if (result == D3DERR_INVALIDCALL)
		IM_ASSERT(0);

	ImGui_ImplDX9_CreateDeviceObjects();

}
void gui::DestroyDevice() noexcept
{
	if (device)
	{
		device->Release();
		device = nullptr;
	}

	if (d3d)
	{
		d3d->Release();
		d3d = nullptr;
	}
}

void gui::CreateImGui() noexcept
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ::ImGui::GetIO(); (void)io;

	// Load Font
	io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 15.f);


	io.IniFilename = NULL;

	ImGui::StyleColorsDark();

	// Set Theme Color

	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX9_Init(device);

}

void gui::DestroyImGui() noexcept
{
	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

}
void gui::BeginRender() noexcept
{
	MSG message;
	while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&message);
		DispatchMessage(&message);
	}

	// Start the Dear ImGui frame
	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

}

void gui::EndRender() noexcept
{
	ImGui::EndFrame();

	device->SetRenderState(D3DRS_ZENABLE, FALSE);
	device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);

	device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_RGBA(0, 0, 0, 255), 1.0f, 0);

	if (device->BeginScene() >= 0)
	{
		ImGui::Render();
		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
		device->EndScene();
	}

	const auto result = device->Present(0, 0, 0, 0);

	// Handle less of D3D9 device
	if (result == D3DERR_DEVICELOST && device->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
		ResetDevice();

}

// Main Gui Code
void gui::Render() noexcept
{
	// Color scheme for cheat menu
	ImVec4 bgColor = ImVec4(0.07f, 0.07f, 0.07f, 1.0f);
	ImVec4 textColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	ImVec4 frameColor = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
	ImVec4 tabColor = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
	ImVec4 tabHoverColor = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
	ImVec4 tabActiveColor = ImVec4(0.18f, 0.18f, 0.18f, 1.0f);
	ImVec4 headerColor = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
	ImVec4 headerHoverColor = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
	ImVec4 headerActiveColor = ImVec4(0.18f, 0.18f, 0.18f, 1.0f);

	ImGuiStyle& style = ImGui::GetStyle();
	style.Colors[ImGuiCol_WindowBg] = bgColor;
	style.Colors[ImGuiCol_Text] = textColor;
	style.Colors[ImGuiCol_FrameBg] = frameColor;
	style.Colors[ImGuiCol_Tab] = tabColor;
	style.Colors[ImGuiCol_TabHovered] = tabHoverColor;
	style.Colors[ImGuiCol_TabActive] = tabActiveColor;
	style.Colors[ImGuiCol_Header] = headerColor;
	style.Colors[ImGuiCol_HeaderHovered] = headerHoverColor;
	style.Colors[ImGuiCol_HeaderActive] = headerActiveColor;
	style.TabRounding = 0.0f; // set tab bar to square shaped

	// Define variables for cheat menu options
	static float fov = 0.0f; // Define a static variable within the function scope
	static bool checkbox1 = false;
	static bool checkbox2 = false;
	static bool checkbox3 = false;
	static bool checkbox4 = false;
	static bool checkbox5 = false;

	// Create the cheat menu window
	ImGui::SetNextWindowPos({ 0, 0 });
	ImGui::SetNextWindowSize({ WIDTH, HEIGHT });
	ImGui::Begin(
		"x86 Loader Base | PhilipPanda",
		&exit,
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoMove
	);


	// Begin Tab Bar
	if (ImGui::BeginTabBar("TabBar"))
	{
		// Create the first tab
		if (ImGui::BeginTabItem("Tab1"))
		{
			ImGui::Spacing();
			ImGui::Checkbox("Checkbox1", &checkbox3);
			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

		

			ImGui::EndTabItem();
		}



		// Create the second tab
		if (ImGui::BeginTabItem("Tab2"))
		{
			ImGui::Spacing();
			ImGui::Checkbox("Checkbox2", &checkbox1);
			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			// Create a child window for the FOV slider
			ImGui::BeginChild("FOVChild", ImVec2(0, 0), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

			ImGui::Text("Slider");
			ImGui::SliderFloat("##FOV", &fov, 0.0f, 100.0f, "%.0f");

			ImGui::EndChild();

			ImGui::EndTabItem();
		}


		// Create the third tab
		if (ImGui::BeginTabItem("Tab3"))
		{
			ImGui::Text("Text");
			ImGui::Checkbox("Checkbox3", &checkbox4);
			ImGui::Checkbox("Checkbox4", &checkbox5);

		}

		ImGui::EndTabItem();
	}


	// Set the cursor position to near the bottom of the window
	float cursorPosY = ImGui::GetWindowHeight() - ImGui::GetStyle().ItemSpacing.y - 20.0f;
	ImGui::SetCursorPosY(cursorPosY);

	// Display the text
	ImGui::Text("DEVELOPED BY: PHILIPPANDA");

	// End the cheat menu window
	ImGui::End();

}

















