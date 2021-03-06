﻿#include "Pch.h"
#include "EngineCore.h"
#include "Engine.h"
#include "ResourceManager.h"
#include "SoundManager.h"
#include "StartupOptions.h"
#include "DebugDrawer.h"
#include "DirectX.h"
#include "Physics.h"

//-----------------------------------------------------------------------------
const Int2 Engine::MIN_WINDOW_SIZE = Int2(800, 600);
const Int2 Engine::DEFAULT_WINDOW_SIZE = Int2(1024, 768);

//-----------------------------------------------------------------------------
Engine* Engine::engine;
KeyStates Key;
extern string g_system_dir;
#ifdef _DEBUG
HRESULT _d_hr;
#endif

//=================================================================================================
Engine::Engine() : engine_shutdown(false), timer(false), hwnd(nullptr), d3d(nullptr), device(nullptr), sprite(nullptr), cursor_visible(true),
replace_cursor(false), locked_cursor(true), lost_device(false), clear_color(Color::Black), res_freed(false), vsync(true), active(false),
activation_point(-1, -1), phy_world(nullptr)
{
	engine = this;
}

//=================================================================================================
Engine::~Engine()
{
}

//=================================================================================================
// Adjust window size to take exact value
void Engine::AdjustWindowSize()
{
	if(!fullscreen)
	{
		Rect rect = Rect::Create(Int2(0, 0), wnd_size);
		AdjustWindowRect((RECT*)&rect, WS_OVERLAPPEDWINDOW, false);
		real_size = rect.Size();
	}
	else
		real_size = wnd_size;
}

//=================================================================================================
// Called after changing mode
void Engine::ChangeMode()
{
	AdjustWindowSize();

	if(!fullscreen)
	{
		// windowed
		SetWindowLong(hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);
		SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOZORDER | SWP_NOSIZE);

		Reset(true);

		SetWindowPos(hwnd, HWND_NOTOPMOST, (GetSystemMetrics(SM_CXSCREEN) - real_size.x) / 2, (GetSystemMetrics(SM_CYSCREEN) - real_size.y) / 2,
			real_size.x, real_size.y, SWP_SHOWWINDOW | SWP_DRAWFRAME);
	}
	else
	{
		// fullscreen
		SetWindowLong(hwnd, GWL_STYLE, WS_POPUPWINDOW);
		SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOZORDER | SWP_NOSIZE);

		Reset(true);

		SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, real_size.x, real_size.y, SWP_NOMOVE | SWP_SHOWWINDOW);
	}

	// reset cursor
	replace_cursor = true;
	Key.UpdateMouseDif(Int2::Zero);
	unlock_point = real_size / 2;
}

//=================================================================================================
// Change display mode
bool Engine::ChangeMode(bool new_fullscreen)
{
	if(fullscreen == new_fullscreen)
		return false;

	Info(new_fullscreen ? "Engine: Changing mode to fullscreen." : "Engine: Changing mode to windowed.");

	fullscreen = new_fullscreen;
	ChangeMode();

	return true;
}

//=================================================================================================
// Change resolution and display mode
bool Engine::ChangeMode(const Int2& size, bool new_fullscreen, int hz)
{
	assert(size.x > 0 && size.y > 0 && hz >= 0);

	if(!CheckDisplay(size, hz))
	{
		Error("Engine: Can't change display mode to %dx%d (%d Hz, %s).", size.x, size.y, hz, new_fullscreen ? "fullscreen" : "windowed");
		return false;
	}

	if(wnd_size == size && fullscreen == new_fullscreen && wnd_hz == hz)
		return false;

	Info("Engine: Resolution changed to %dx%d (%d Hz, %s).", size.x, size.y, hz, new_fullscreen ? "fullscreen" : "windowed");

	bool size_changed = (wnd_size != size);

	fullscreen = new_fullscreen;
	wnd_size = size;
	wnd_hz = hz;
	ChangeMode();

	if(size_changed)
		OnResize();

	return true;
}

//=================================================================================================
// Change multisampling
int Engine::ChangeMultisampling(int type, int level)
{
	if(type == multisampling && (level == -1 || level == multisampling_quality))
		return 1;

	DWORD levels, levels2;
	if(SUCCEEDED(d3d->CheckDeviceMultiSampleType(0, D3DDEVTYPE_HAL, D3DFMT_A8R8G8B8, fullscreen ? FALSE : TRUE, (D3DMULTISAMPLE_TYPE)type, &levels)) &&
		SUCCEEDED(d3d->CheckDeviceMultiSampleType(0, D3DDEVTYPE_HAL, D3DFMT_D24S8, fullscreen ? FALSE : TRUE, (D3DMULTISAMPLE_TYPE)type, &levels2)))
	{
		levels = min(levels, levels2);
		if(level < 0)
			level = levels - 1;
		else if(level >= (int)levels)
			return 0;

		multisampling = type;
		multisampling_quality = level;

		Reset(true);

		return 2;
	}
	else
		return 0;
}

//=================================================================================================
// Verify display mode
bool Engine::CheckDisplay(const Int2& size, int& hz)
{
	assert(size.x >= MIN_WINDOW_SIZE.x && size.x >= MIN_WINDOW_SIZE.y);

	// check minimum resolution
	if(size.x < MIN_WINDOW_SIZE.x || size.y < MIN_WINDOW_SIZE.y)
		return false;

	uint display_modes = d3d->GetAdapterModeCount(used_adapter, DISPLAY_FORMAT);

	if(hz == 0)
	{
		bool valid = false;

		for(uint i = 0; i < display_modes; ++i)
		{
			D3DDISPLAYMODE d_mode;
			V(d3d->EnumAdapterModes(used_adapter, DISPLAY_FORMAT, i, &d_mode));
			if(size.x == d_mode.Width && size.y == d_mode.Height)
			{
				valid = true;
				if(hz < (int)d_mode.RefreshRate)
					hz = d_mode.RefreshRate;
			}
		}

		return valid;
	}
	else
	{
		for(uint i = 0; i < display_modes; ++i)
		{
			D3DDISPLAYMODE d_mode;
			V(d3d->EnumAdapterModes(used_adapter, DISPLAY_FORMAT, i, &d_mode));
			if(size.x == d_mode.Width && size.y == d_mode.Height && hz == d_mode.RefreshRate)
				return true;
		}

		return false;
	}
}

//=================================================================================================
// Cleanup engine
void Engine::Cleanup()
{
	Info("Engine: Cleanup.");

	OnCleanup();

	ResourceManager::Get().Cleanup();

	// directx
	if(device)
	{
		device->SetStreamSource(0, nullptr, 0, 0);
		device->SetIndices(nullptr);
	}
	SafeRelease(sprite);
	SafeRelease(device);
	SafeRelease(d3d);

	CustomCollisionWorld::Cleanup(phy_world);
}

//=================================================================================================
// Compile shader
ID3DXEffect* Engine::CompileShader(cstring name)
{
	assert(name);

	CompileShaderParams params = { name };

	// add c to extension
	LocalString str = (shader_version == 3 ? "3_" : "2_");
	str += name;
	str += 'c';
	params.cache_name = str;

	// set shader version
	D3DXMACRO macros[3] = {
		"VS_VERSION", shader_version == 3 ? "vs_3_0" : "vs_2_0",
		"PS_VERSION", shader_version == 3 ? "ps_3_0" : "ps_2_0",
		nullptr, nullptr
	};
	params.macros = macros;

	return CompileShader(params);
}

//=================================================================================================
// Compile shader with params
ID3DXEffect* Engine::CompileShader(CompileShaderParams& params)
{
	assert(params.name && params.cache_name);

	ID3DXBuffer* errors = nullptr;
	ID3DXEffectCompiler* compiler = nullptr;
	cstring filename = Format("%s/shaders/%s", g_system_dir.c_str(), params.name);
	cstring cache_path = Format("cache/%s", params.cache_name);
	HRESULT hr;

	const DWORD flags =
#ifdef _DEBUG
		D3DXSHADER_DEBUG | D3DXSHADER_OPTIMIZATION_LEVEL1;
#else
		D3DXSHADER_OPTIMIZATION_LEVEL3;
#endif

	// open file and get date if not from string
	FileReader file;
	if(!params.input)
	{
		if(!file.Open(filename))
			throw Format("Engine: Failed to load shader '%s' (%d).", params.name, GetLastError());
		params.file_time = file.GetTime();
	}

	// check if in cache
	{
		FileReader cache_file(cache_path);
		if(cache_file && params.file_time == cache_file.GetTime())
		{
			// same last modify time, use cache
			cache_file.ReadToString(g_tmp_string);
			ID3DXEffect* effect = nullptr;
			hr = D3DXCreateEffect(device, g_tmp_string.c_str(), g_tmp_string.size(), params.macros, nullptr, flags, params.pool, &effect, &errors);
			if(FAILED(hr))
			{
				Error("Engine: Failed to create effect from cache '%s' (%d).\n%s", params.cache_name, hr,
					errors ? (cstring)errors->GetBufferPointer() : "No errors information.");
				SafeRelease(errors);
				SafeRelease(effect);
			}
			else
			{
				SafeRelease(errors);
				return effect;
			}
		}
	}

	// load from file
	if(!params.input)
	{
		file.ReadToString(g_tmp_string);
		params.input = &g_tmp_string;
	}
	hr = D3DXCreateEffectCompiler(params.input->c_str(), params.input->size(), params.macros, nullptr, flags, &compiler, &errors);
	if(FAILED(hr))
	{
		cstring str;
		if(errors)
			str = (cstring)errors->GetBufferPointer();
		else
		{
			switch(hr)
			{
			case D3DXERR_INVALIDDATA:
				str = "Invalid data.";
				break;
			case D3DERR_INVALIDCALL:
				str = "Invalid call.";
				break;
			case E_OUTOFMEMORY:
				str = "Out of memory.";
				break;
			case ERROR_MOD_NOT_FOUND:
			case 0x8007007e:
				str = "Can't find module (missing d3dcompiler_43.dll?).";
				break;
			default:
				str = "Unknown error.";
				break;
			}
		}

		cstring msg = Format("Engine: Failed to compile shader '%s' (%d).\n%s", params.name, hr, str);

		SafeRelease(errors);

		throw msg;
	}
	SafeRelease(errors);

	// compile shader
	ID3DXBuffer* effect_buffer = nullptr;
	hr = compiler->CompileEffect(flags, &effect_buffer, &errors);
	if(FAILED(hr))
	{
		cstring msg = Format("Engine: Failed to compile effect '%s' (%d).\n%s", params.name, hr,
			errors ? (cstring)errors->GetBufferPointer() : "No errors information.");

		SafeRelease(errors);
		SafeRelease(effect_buffer);
		SafeRelease(compiler);

		throw msg;
	}
	SafeRelease(errors);

	// save to cache
	CreateDirectory("cache", nullptr);
	FileWriter f(cache_path);
	if(f)
	{
		f.Write(effect_buffer->GetBufferPointer(), effect_buffer->GetBufferSize());
		f.SetTime(params.file_time);
	}
	else
		Warn("Engine: Failed to save effect '%s' to cache (%d).", params.cache_name, GetLastError());

	// create effect from effect buffer
	ID3DXEffect* effect = nullptr;
	hr = D3DXCreateEffect(device, effect_buffer->GetBufferPointer(), effect_buffer->GetBufferSize(),
		params.macros, nullptr, flags, params.pool, &effect, &errors);
	if(FAILED(hr))
	{
		cstring msg = Format("Engine: Failed to create effect '%s' (%d).\n%s", params.name, hr,
			errors ? (cstring)errors->GetBufferPointer() : "No errors information.");

		SafeRelease(errors);
		SafeRelease(effect_buffer);
		SafeRelease(compiler);

		throw msg;
	}

	// free directx stuff
	SafeRelease(errors);
	SafeRelease(effect_buffer);
	SafeRelease(compiler);

	return effect;
}

//=================================================================================================
// Do pseudo update tick, used to render in update loop
void Engine::DoPseudotick(bool msg_only)
{
	MSG msg = { 0 };
	if(!timer.IsStarted())
		timer.Start();

	while(msg.message != WM_QUIT && PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	if(msg_only)
		timer.Tick();
	else
		DoTick(false);
}

//=================================================================================================
// Common part for WindowLoop and DoPseudotick
void Engine::DoTick(bool update_game)
{
	const float dt = timer.Tick();
	assert(dt >= 0.f);

	// calculate fps
	frames++;
	frame_time += dt;
	if(frame_time >= 1.f)
	{
		fps = frames / frame_time;
		frames = 0;
		frame_time = 0.f;
	}

	// update activity state
	bool is_active = IsWindowActive();
	bool was_active = active;
	UpdateActivity(is_active);

	// handle cursor movement
	Int2 mouse_dif = Int2::Zero;
	if(active)
	{
		if(locked_cursor)
		{
			if(replace_cursor)
				replace_cursor = false;
			else if(was_active)
			{
				POINT pt;
				GetCursorPos(&pt);
				ScreenToClient(hwnd, &pt);
				mouse_dif = Int2(pt.x, pt.y) - real_size / 2;
			}
			PlaceCursor();
		}
	}
	else if(!locked_cursor && lock_on_focus)
		locked_cursor = true;
	Key.UpdateMouseDif(mouse_dif);

	// update keyboard shortcuts info
	Key.UpdateShortcuts();

	// update game
	if(update_game)
		OnTick(dt);
	if(engine_shutdown)
	{
		if(active && locked_cursor)
		{
			Rect rect;
			GetClientRect(hwnd, (RECT*)&rect);
			Int2 wh = rect.Size();
			POINT pt;
			pt.x = int(float(unlock_point.x)*wh.x / wnd_size.x);
			pt.y = int(float(unlock_point.y)*wh.y / wnd_size.y);
			ClientToScreen(hwnd, &pt);
			SetCursorPos(pt.x, pt.y);
		}
		return;
	}
	Key.UpdateMouseWheel(0);

	Render();
	Key.Update();
	sound_mgr->Update(dt);
}

//=================================================================================================
bool Engine::IsWindowActive()
{
	HWND foreground = GetForegroundWindow();
	if(foreground != hwnd)
		return false;
	return !IsIconic(hwnd);
}

//=================================================================================================
// Start closing engine
void Engine::EngineShutdown()
{
	if(!engine_shutdown)
	{
		engine_shutdown = true;
		Info("Engine: Started closing engine...");
	}
}

//=================================================================================================
// Show fatal error
void Engine::FatalError(cstring err)
{
	assert(err);
	ShowError(err, Logger::L_FATAL);
	EngineShutdown();
}

//=================================================================================================
// Gather directx params
void Engine::GatherParams(D3DPRESENT_PARAMETERS& d3dpp)
{
	d3dpp.Windowed = !fullscreen;
	d3dpp.BackBufferCount = 1;
	d3dpp.BackBufferFormat = BACKBUFFER_FORMAT;
	d3dpp.BackBufferWidth = wnd_size.x;
	d3dpp.BackBufferHeight = wnd_size.y;
	d3dpp.EnableAutoDepthStencil = TRUE;
	d3dpp.MultiSampleType = (D3DMULTISAMPLE_TYPE)multisampling;
	d3dpp.MultiSampleQuality = multisampling_quality;
	d3dpp.hDeviceWindow = hwnd;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.AutoDepthStencilFormat = ZBUFFER_FORMAT;
	d3dpp.Flags = 0;
	d3dpp.PresentationInterval = (vsync ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE);
	d3dpp.FullScreen_RefreshRateInHz = (fullscreen ? wnd_hz : 0);
}

//=================================================================================================
// Handle windows events
long Engine::HandleEvent(HWND in_hwnd, uint msg, uint wParam, long lParam)
{
	switch(msg)
	{
	// window closed/destroyed
	case WM_CLOSE:
	case WM_DESTROY:
		engine_shutdown = true;
		return 0;

	// handle keyboard
	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
		Key.Process((byte)wParam, true);
		return 0;
	case WM_SYSKEYUP:
	case WM_KEYUP:
		Key.Process((byte)wParam, false);
		return 0;

	// handle mouse
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_XBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MBUTTONUP:
	case WM_XBUTTONUP:
		{
			byte key;
			int result;
			bool down = MsgToKey(msg, wParam, key, result);

			if((!locked_cursor || !active) && down && lock_on_focus)
			{
				ShowCursor(false);
				Rect rect;
				GetClientRect(hwnd, (RECT*)&rect);
				Int2 wh = rect.Size();
				POINT pt;
				GetCursorPos(&pt);
				ScreenToClient(hwnd, &pt);
				activation_point = Int2(pt.x * wnd_size.x / wh.x, pt.y * wnd_size.y / wh.y);
				PlaceCursor();

				if(active)
					locked_cursor = true;

				return result;
			}

			Key.Process(key, down);
			return result;
		}

	// handle double click
	case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDBLCLK:
	case WM_MBUTTONDBLCLK:
	case WM_XBUTTONDBLCLK:
		{
			byte key;
			int result = 0;
			MsgToKey(msg, wParam, key, result);
			Key.ProcessDoubleClick(key);
			return result;
		}

	// close alt+space menu
	case WM_MENUCHAR:
		return MAKELRESULT(0, MNC_CLOSE);

	// handle text input
	case WM_CHAR:
	case WM_SYSCHAR:
		OnChar((char)wParam);
		return 0;

	// handle mouse wheel
	case WM_MOUSEWHEEL:
		Key.UpdateMouseWheel(Key.GetMouseWheel() + float(GET_WHEEL_DELTA_WPARAM(wParam)) / WHEEL_DELTA);
		return 0;
	}

	// return default message
	return DefWindowProc(in_hwnd, msg, wParam, lParam);
}

//=================================================================================================
// Convert message to virtual key
bool Engine::MsgToKey(uint msg, uint wParam, byte& key, int& result)
{
	bool down = false;

	switch(msg)
	{
	default:
		assert(0);
		break;
	case WM_LBUTTONDOWN:
		down = true;
	case WM_LBUTTONUP:
	case WM_LBUTTONDBLCLK:
		key = VK_LBUTTON;
		result = 0;
		break;
	case WM_RBUTTONDOWN:
		down = true;
	case WM_RBUTTONUP:
	case WM_RBUTTONDBLCLK:
		key = VK_RBUTTON;
		result = 0;
		break;
	case WM_MBUTTONDOWN:
		down = true;
	case WM_MBUTTONUP:
	case WM_MBUTTONDBLCLK:
		key = VK_MBUTTON;
		result = 0;
		break;
	case WM_XBUTTONDOWN:
		down = true;
	case WM_XBUTTONUP:
	case WM_XBUTTONDBLCLK:
		key = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1 ? VK_XBUTTON1 : VK_XBUTTON2);
		result = TRUE;
		break;
	}

	return down;
}

//=================================================================================================
// Initialize Directx 9 rendering
void Engine::InitRender()
{
	HRESULT hr;

	// create direct3d object
	d3d = Direct3DCreate9(D3D_SDK_VERSION);
	if(!d3d)
		throw "Engine: Failed to create direct3d object.";

	// get adapters count
	uint adapters = d3d->GetAdapterCount();
	Info("Engine: Adapters count: %u", adapters);

	// get adapters info
	D3DADAPTER_IDENTIFIER9 adapter;
	for(uint i = 0; i < adapters; ++i)
	{
		hr = d3d->GetAdapterIdentifier(i, 0, &adapter);
		if(FAILED(hr))
			Warn("Engine: Can't get info about adapter %d (%d).", i, hr);
		else
		{
			Info("Engine: Adapter %d: %s, version %d.%d.%d.%d", i, adapter.Description, HIWORD(adapter.DriverVersion.HighPart),
				LOWORD(adapter.DriverVersion.HighPart), HIWORD(adapter.DriverVersion.LowPart), LOWORD(adapter.DriverVersion.LowPart));
		}
	}
	if(used_adapter > (int)adapters)
	{
		Warn("Engine: Invalid adapter %d, defaulting to 0.", used_adapter);
		used_adapter = 0;
	}

	// check shaders version
	D3DCAPS9 caps;
	d3d->GetDeviceCaps(used_adapter, D3DDEVTYPE_HAL, &caps);
	if(D3DVS_VERSION(2, 0) > caps.VertexShaderVersion || D3DPS_VERSION(2, 0) > caps.PixelShaderVersion)
	{
		throw Format("Engine: Too old graphic card! This game require vertex and pixel shader in version 2.0+. "
			"Your card support:\nVertex shader: %d.%d\nPixel shader: %d.%d",
			D3DSHADER_VERSION_MAJOR(caps.VertexShaderVersion), D3DSHADER_VERSION_MINOR(caps.VertexShaderVersion),
			D3DSHADER_VERSION_MAJOR(caps.PixelShaderVersion), D3DSHADER_VERSION_MINOR(caps.PixelShaderVersion));
	}
	else
	{
		Info("Supported shader version vertex: %d.%d, pixel: %d.%d.",
			D3DSHADER_VERSION_MAJOR(caps.VertexShaderVersion), D3DSHADER_VERSION_MINOR(caps.VertexShaderVersion),
			D3DSHADER_VERSION_MAJOR(caps.PixelShaderVersion), D3DSHADER_VERSION_MINOR(caps.PixelShaderVersion));

		int version = min(D3DSHADER_VERSION_MAJOR(caps.VertexShaderVersion), D3DSHADER_VERSION_MAJOR(caps.PixelShaderVersion));
		if(shader_version == -1 || shader_version > version)
			shader_version = version;

		Info("Using shader version %d.", shader_version);
	}

	// check texture types
	hr = d3d->CheckDeviceType(used_adapter, D3DDEVTYPE_HAL, DISPLAY_FORMAT, BACKBUFFER_FORMAT, fullscreen ? FALSE : TRUE);
	if(FAILED(hr))
		throw Format("Engine: Unsupported backbuffer type %s for display %s! (%d)", STRING(BACKBUFFER_FORMAT), STRING(DISPLAY_FORMAT), hr);

	hr = d3d->CheckDeviceFormat(used_adapter, D3DDEVTYPE_HAL, DISPLAY_FORMAT, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, ZBUFFER_FORMAT);
	if(FAILED(hr))
		throw Format("Engine: Unsupported depth buffer type %s for display %s! (%d)", STRING(ZBUFFER_FORMAT), STRING(DISPLAY_FORMAT), hr);

	hr = d3d->CheckDepthStencilMatch(used_adapter, D3DDEVTYPE_HAL, DISPLAY_FORMAT, D3DFMT_A8R8G8B8, ZBUFFER_FORMAT);
	if(FAILED(hr))
		throw Format("Engine: Unsupported render target D3DFMT_A8R8G8B8 with display %s and depth buffer %s! (%d)",
			STRING(DISPLAY_FORMAT), STRING(BACKBUFFER_FORMAT), hr);

	// check multisampling
	DWORD levels, levels2;
	if(SUCCEEDED(d3d->CheckDeviceMultiSampleType(used_adapter, D3DDEVTYPE_HAL, D3DFMT_A8R8G8B8, fullscreen ? FALSE : TRUE,
		(D3DMULTISAMPLE_TYPE)multisampling, &levels))
		&& SUCCEEDED(d3d->CheckDeviceMultiSampleType(used_adapter, D3DDEVTYPE_HAL, D3DFMT_D24S8, fullscreen ? FALSE : TRUE,
		(D3DMULTISAMPLE_TYPE)multisampling, &levels2)))
	{
		levels = min(levels, levels2);
		if(multisampling_quality < 0 || multisampling_quality >= (int)levels)
		{
			Warn("Engine: Unavailable multisampling quality, changed to 0.");
			multisampling_quality = 0;
		}
	}
	else
	{
		Warn("Engine: Your graphic card don't support multisampling x%d. Maybe it's only available in fullscreen mode. "
			"Multisampling was turned off.", multisampling);
		multisampling = 0;
		multisampling_quality = 0;
	}
	LogMultisampling();

	// select resolution
	SelectResolution();

	// gather params
	D3DPRESENT_PARAMETERS d3dpp = { 0 };
	GatherParams(d3dpp);

	// available modes
	const DWORD mode[] = {
		D3DCREATE_HARDWARE_VERTEXPROCESSING,
		D3DCREATE_MIXED_VERTEXPROCESSING,
		D3DCREATE_SOFTWARE_VERTEXPROCESSING
	};
	const cstring mode_str[] = {
		"hardware",
		"mixed",
		"software"
	};

	// try to create device in one of modes
	for(uint i = 0; i < 3; ++i)
	{
		DWORD sel_mode = mode[i];
		hr = d3d->CreateDevice(used_adapter, D3DDEVTYPE_HAL, d3dpp.hDeviceWindow, sel_mode, &d3dpp, &device);

		if(SUCCEEDED(hr))
		{
			Info("Engine: Created direct3d device in %s mode.", mode_str[i]);
			break;
		}
	}

	// failed to create device
	if(FAILED(hr))
		throw Format("Engine: Failed to create direct3d device (%d).", hr);

	// create sprite
	hr = D3DXCreateSprite(device, &sprite);
	if(FAILED(hr))
		throw Format("Engine: Failed to create direct3dx sprite (%d).", hr);

	SetDefaultRenderState();

	Info("Engine: Directx device created.");
}

//=================================================================================================
// Create window
void Engine::InitWindow(StartupOptions& options)
{
	assert(options.title);

	// register window class
	WNDCLASSEX wc = {
		sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS,
		[](HWND hwnd, uint msg, WPARAM wParam, LPARAM lParam) -> LRESULT { return Engine::Get().HandleEvent(hwnd, msg, wParam, lParam); },
		0, 0, GetModuleHandle(nullptr), LoadIcon(GetModuleHandle(nullptr), "Icon"), LoadCursor(nullptr, IDC_ARROW), (HBRUSH)GetStockObject(BLACK_BRUSH),
		nullptr, "Krystal", nullptr
	};
	if(!RegisterClassEx(&wc))
		throw Format("Failed to register window class (%d).", GetLastError());

	// create window
	AdjustWindowSize();
	hwnd = CreateWindowEx(0, "Krystal", options.title, fullscreen ? WS_POPUPWINDOW : WS_OVERLAPPEDWINDOW, 0, 0, real_size.x, real_size.y,
		nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
	if(!hwnd)
		throw Format("Failed to create window (%d).", GetLastError());

	// position window
	if(!fullscreen)
	{
		if(options.force_pos != Int2(-1, -1) || options.force_size != Int2(-1, -1))
		{
			// set window position from config file
			Rect rect;
			GetWindowRect(hwnd, (RECT*)&rect);
			if(options.force_pos.x != -1)
				rect.Left() = options.force_pos.x;
			if(options.force_pos.y != -1)
				rect.Top() = options.force_pos.y;
			Int2 size = real_size;
			if(options.force_size.x != -1)
				size.x = options.force_size.x;
			if(options.force_size.y != -1)
				size.y = options.force_size.y;
			SetWindowPos(hwnd, 0, rect.Left(), rect.Top(), size.x, size.y, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
		}
		else
		{
			// set window at center of screen
			MoveWindow(hwnd,
				(GetSystemMetrics(SM_CXSCREEN) - real_size.x) / 2,
				(GetSystemMetrics(SM_CYSCREEN) - real_size.y) / 2,
				real_size.x, real_size.y, false);
		}
	}

	// show window
	ShowWindow(hwnd, options.hidden_window ? SW_HIDE : SW_SHOWNORMAL);

	// reset cursor
	replace_cursor = true;
	Key.UpdateMouseDif(Int2::Zero);
	unlock_point = real_size / 2;

	Info("Engine: Window created.");
}

//=================================================================================================
// Log available multisampling levels
void Engine::LogMultisampling()
{
	LocalString s = "Engine: Available multisampling: ";

	for(int j = 2; j <= 16; ++j)
	{
		DWORD levels, levels2;
		if(SUCCEEDED(d3d->CheckDeviceMultiSampleType(used_adapter, D3DDEVTYPE_HAL, BACKBUFFER_FORMAT, FALSE, (D3DMULTISAMPLE_TYPE)j, &levels)) &&
			SUCCEEDED(d3d->CheckDeviceMultiSampleType(used_adapter, D3DDEVTYPE_HAL, ZBUFFER_FORMAT, FALSE, (D3DMULTISAMPLE_TYPE)j, &levels2)))
		{
			s += Format("x%d(%d), ", j, min(levels, levels2));
		}
	}

	if(s.at_back(1) == ':')
		s += "none";
	else
		s.pop(2);

	Info(s);
}

//=================================================================================================
// Place cursor on window center
void Engine::PlaceCursor()
{
	POINT p;
	p.x = real_size.x / 2;
	p.y = real_size.y / 2;
	ClientToScreen(hwnd, &p);
	SetCursorPos(p.x, p.y);
}

//=================================================================================================
// Rendering
void Engine::Render(bool dont_call_present)
{
	HRESULT hr = device->TestCooperativeLevel();
	if(hr != D3D_OK)
	{
		lost_device = true;
		if(hr == D3DERR_DEVICELOST)
		{
			// device lost, can't reset yet
			Sleep(1);
			return;
		}
		else if(hr == D3DERR_DEVICENOTRESET)
		{
			// try reset
			if(!Reset(false))
			{
				Sleep(1);
				return;
			}
		}
		else
			throw Format("Engine: Lost directx device (%d).", hr);
	}

	assert(cam_base);
	OnDraw();

	if(!dont_call_present)
	{
		hr = device->Present(nullptr, nullptr, hwnd, nullptr);
		if(FAILED(hr))
		{
			if(hr == D3DERR_DEVICELOST)
				lost_device = true;
			else
				throw Format("Engine: Failed to present screen (%d).", hr);
		}
	}
}

//=================================================================================================
// Reset directx device
bool Engine::Reset(bool force)
{
	Info("Engine: Reseting device.");

	// free resources
	if(!res_freed)
	{
		res_freed = true;
		V(sprite->OnLostDevice());
		for(ShaderHandler* shader : shaders)
			shader->OnReset();
		OnReset();
	}

	// gather params
	D3DPRESENT_PARAMETERS d3dpp = { 0 };
	GatherParams(d3dpp);

	// reset
	HRESULT hr = device->Reset(&d3dpp);
	if(FAILED(hr))
	{
		if(force || hr != D3DERR_DEVICELOST)
		{
			if(hr == D3DERR_INVALIDCALL)
				throw "Engine: Device reset returned D3DERR_INVALIDCALL, not all resources was released.";
			else
				throw Format("Engine: Failed to reset directx device (%d).", hr);
		}
		else
		{
			Warn("Engine: Failed to reset device.");
			return false;
		}
	}

	// reload resources
	SetDefaultRenderState();
	for(ShaderHandler* shader : shaders)
		shader->OnReload();
	OnReload();
	V(sprite->OnResetDevice());
	lost_device = false;
	res_freed = false;

	return true;
}

//=================================================================================================
void Engine::WaitReset()
{
	Info("Engine: Device lost at loading. Waiting for reset.");

	// free resources
	if(!res_freed)
	{
		res_freed = true;
		V(sprite->OnLostDevice());
		OnReset();
	}
	UpdateActivity(false);

	// gather params
	D3DPRESENT_PARAMETERS d3dpp = { 0 };
	GatherParams(d3dpp);

	// wait for reset
	while(true)
	{
		DoPseudotick(true);

		HRESULT hr = device->TestCooperativeLevel();
		if(hr == D3DERR_DEVICELOST)
		{
			Info("Engine: Device lost, waiting...");
		}
		else if(hr == D3DERR_DEVICENOTRESET)
		{
			Info("Engine: Device can be reseted, trying...");

			// reset
			hr = device->Reset(&d3dpp);
			if(FAILED(hr))
			{
				if(hr == D3DERR_DEVICELOST)
					Warn("Engine: Can't reset, device is lost.");
				else if(hr == D3DERR_INVALIDCALL)
					throw "Engine: Device reset returned D3DERR_INVALIDCALL, not all resources was released.";
				else
					throw Format("Engine: Device reset returned error (%u).", hr);
			}
			else
				break;
		}
		else
			throw Format("Engine: Device lost and cannot reset (%u).", hr);
		Sleep(500);
	}

	// reload resources
	Info("Engine: Device reset successful. Reloading resources.");
	SetDefaultRenderState();
	OnReload();
	V(sprite->OnResetDevice());
	lost_device = false;
	res_freed = false;
}

namespace E
{
	struct Res
	{
		int w, h, hz;

		Res(int w, int h, int hz) : w(w), h(h), hz(hz) {}
	};

	bool ResPred(const Res& r1, const Res& r2)
	{
		if(r1.w > r2.w)
			return false;
		else if(r1.w < r2.w)
			return true;
		else if(r1.h > r2.h)
			return false;
		else if(r1.h < r2.h)
			return true;
		else if(r1.hz > r2.hz)
			return false;
		else if(r1.hz < r2.hz)
			return true;
		else
			return false;
	}
}

//=================================================================================================
// Log avaiable resolutions and select valid
void Engine::SelectResolution()
{
	vector<E::Res> ress;
	LocalString str = "Engine: Available display modes:";
	uint display_modes = d3d->GetAdapterModeCount(used_adapter, DISPLAY_FORMAT);
	int best_hz = 0, best_valid_hz = 0;
	bool res_valid = false, hz_valid = false;
	for(uint i = 0; i < display_modes; ++i)
	{
		D3DDISPLAYMODE d_mode;
		V(d3d->EnumAdapterModes(used_adapter, DISPLAY_FORMAT, i, &d_mode));
		if(d_mode.Width < (uint)MIN_WINDOW_SIZE.x || d_mode.Height < (uint)MIN_WINDOW_SIZE.y)
			continue;
		ress.push_back(E::Res(d_mode.Width, d_mode.Height, d_mode.RefreshRate));
		if(d_mode.Width == (uint)DEFAULT_WINDOW_SIZE.x && d_mode.Height == (uint)DEFAULT_WINDOW_SIZE.y)
		{
			if(d_mode.RefreshRate > (uint)best_hz)
				best_hz = d_mode.RefreshRate;
		}
		if(d_mode.Width == wnd_size.x && d_mode.Height == wnd_size.y)
		{
			res_valid = true;
			if(d_mode.RefreshRate == wnd_hz)
				hz_valid = true;
			if((int)d_mode.RefreshRate > best_valid_hz)
				best_valid_hz = d_mode.RefreshRate;
		}
	}
	std::sort(ress.begin(), ress.end(), E::ResPred);
	int cw = 0, ch = 0;
	for(vector<E::Res>::iterator it = ress.begin(), end = ress.end(); it != end; ++it)
	{
		E::Res& r = *it;
		if(r.w != cw || r.h != ch)
		{
			if(it != ress.begin())
				str += " Hz)";
			str += Format("\n\t%dx%d (%d", r.w, r.h, r.hz);
			cw = r.w;
			ch = r.h;
		}
		else
			str += Format(", %d", r.hz);
	}
	str += " Hz)";
	Info(str->c_str());

	// adjust selected resolution
	if(!res_valid)
	{
		const Int2 defaul_res(1024, 768);
		if(wnd_size.x != 0)
			Warn("Engine: Resolution %dx%d is not valid, defaulting to %dx%d (%d Hz).", wnd_size.x, wnd_size.y, defaul_res.x, defaul_res.y, best_hz);
		else
			Info("Engine: Defaulting resolution to %dx%dx (%d Hz).", defaul_res.x, defaul_res.y, best_hz);
		wnd_size = defaul_res;
		wnd_hz = best_hz;
		AdjustWindowSize();
		SetWindowPos(hwnd, HWND_NOTOPMOST, (GetSystemMetrics(SM_CXSCREEN) - real_size.x) / 2, (GetSystemMetrics(SM_CYSCREEN) - real_size.y) / 2,
			real_size.x, real_size.y, SWP_SHOWWINDOW | SWP_DRAWFRAME);
	}
	else if(!hz_valid)
	{
		if(wnd_hz != 0)
			Warn("Engine: Refresh rate %d Hz is not valid, defaulting to %d Hz.", wnd_hz, best_valid_hz);
		else
			Info("Engine: Defaulting refresh rate to %d Hz.", best_valid_hz);
		wnd_hz = best_valid_hz;
	}
}

//=================================================================================================
// Set starting multisampling
void Engine::SetStartingMultisampling(int _multisampling, int _multisampling_quality)
{
	if(_multisampling < 0 || _multisampling == 1 || _multisampling > D3DMULTISAMPLE_16_SAMPLES)
	{
		multisampling = 0;
		Warn("Engine: Unsupported multisampling: %d.", _multisampling);
	}
	else
	{
		multisampling = _multisampling;
		multisampling_quality = _multisampling_quality;
	}
}

//=================================================================================================
// Change window title
void Engine::SetTitle(cstring title)
{
	assert(title);
	SetWindowTextA(hwnd, title);
}

//=================================================================================================
// Show/hide cursor
void Engine::ShowCursor(bool _show)
{
	if (IsCursorVisible() != _show)
	{
		::ShowCursor(_show);
		cursor_visible = _show;
	}
}

//=================================================================================================
// Show error
void Engine::ShowError(cstring msg, Logger::Level level)
{
	assert(msg);

	ShowWindow(hwnd, SW_HIDE);
	ShowCursor(true);
	Logger::global->Log(level, msg);
	Logger::global->Flush();
	MessageBox(nullptr, msg, nullptr, MB_OK | MB_ICONERROR | MB_APPLMODAL);
}

//=================================================================================================
// Initialize and start engine
bool Engine::Start(StartupOptions& options)
{
	// set parameters
	fullscreen = options.fullscreen;
	wnd_size = Int2::Max(options.size, MIN_WINDOW_SIZE);
	vsync = options.vsync;

	// initialize engine
	try
	{
		Init(options);
	}
	catch(cstring e)
	{
		ShowError(Format("Engine: Failed to initialize CaRpg engine!\n%s", e), Logger::L_FATAL);
		Cleanup();
		return false;
	}

	// initialize game
	if(!InitGame())
	{
		Cleanup();
		return false;
	}

	// loop game
	try
	{
		if(locked_cursor && active)
			PlaceCursor();
		WindowLoop();
	}
	catch(cstring e)
	{
		ShowError(Format("Engine: Game error!\n%s", e));
		Cleanup();
		return false;
	}

	// cleanup
	Cleanup();
	return true;
}

//=================================================================================================
void Engine::Init(StartupOptions& options)
{
	InitWindow(options);
	InitRender();
	sound_mgr.reset(new SoundManager);
	sound_mgr->Init(options);
	phy_world = CustomCollisionWorld::Init();
	ResourceManager::Get().Init(device, sound_mgr.get());
	debug_drawer.reset(new DebugDrawer);
	debug_drawer->InitOnce();
}

//=================================================================================================
// Unlock cursor - show system cursor and allow to move outside of window
void Engine::UnlockCursor(bool _lock_on_focus)
{
	lock_on_focus = _lock_on_focus;
	if(!locked_cursor)
		return;
	locked_cursor = false;

	if(!IsCursorVisible())
	{
		Rect rect;
		GetClientRect(hwnd, (RECT*)&rect);
		Int2 wh = rect.Size();
		POINT pt;
		pt.x = int(float(unlock_point.x)*wh.x / wnd_size.x);
		pt.y = int(float(unlock_point.y)*wh.y / wnd_size.y);
		ClientToScreen(hwnd, &pt);
		SetCursorPos(pt.x, pt.y);
	}

	ShowCursor(true);
}

//=================================================================================================
// Lock cursor when window gets activated
void Engine::LockCursor()
{
	if(locked_cursor)
		return;
	lock_on_focus = true;
	locked_cursor = true;
	if(active)
	{
		ShowCursor(false);
		PlaceCursor();
	}
}

//=================================================================================================
// Update window activity
void Engine::UpdateActivity(bool is_active)
{
	if(is_active == active)
		return;
	active = is_active;
	if(active)
	{
		if (locked_cursor)
		{
			ShowCursor(false);
			PlaceCursor();
		}
	}
	else
	{
		ShowCursor(true);
		Key.ReleaseKeys();
	}
	OnFocus(active, activation_point);
	activation_point = Int2(-1, -1);
}

//=================================================================================================
// Main window loop
void Engine::WindowLoop()
{
	MSG msg = { 0 };

	// start timer
	timer.Start();
	frames = 0;
	frame_time = 0.f;
	fps = 0.f;

	while(msg.message != WM_QUIT)
	{
		// handle winapi messages
		if(PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
			DoTick(true);

		if(engine_shutdown)
			break;
	}
}

//=================================================================================================
void Engine::SetDefaultRenderState()
{
	V(device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD));
	V(device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA));
	V(device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA));
	V(device->SetRenderState(D3DRS_ALPHAREF, 200));
	V(device->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL));

	r_alphatest = false;
	r_alphablend = false;
	r_nocull = false;
	r_nozwrite = false;
}

//=================================================================================================
void Engine::SetAlphaBlend(bool use_alphablend)
{
	if(use_alphablend != r_alphablend)
	{
		r_alphablend = use_alphablend;
		V(device->SetRenderState(D3DRS_ALPHABLENDENABLE, r_alphablend ? TRUE : FALSE));
	}
}

//=================================================================================================
void Engine::SetAlphaTest(bool use_alphatest)
{
	if(use_alphatest != r_alphatest)
	{
		r_alphatest = use_alphatest;
		V(device->SetRenderState(D3DRS_ALPHATESTENABLE, r_alphatest ? TRUE : FALSE));
	}
}

//=================================================================================================
void Engine::SetNoCulling(bool use_nocull)
{
	if(use_nocull != r_nocull)
	{
		r_nocull = use_nocull;
		V(device->SetRenderState(D3DRS_CULLMODE, r_nocull ? D3DCULL_NONE : D3DCULL_CCW));
	}
}

//=================================================================================================
void Engine::SetNoZWrite(bool use_nozwrite)
{
	if(use_nozwrite != r_nozwrite)
	{
		r_nozwrite = use_nozwrite;
		V(device->SetRenderState(D3DRS_ZWRITEENABLE, r_nozwrite ? FALSE : TRUE));
	}
}

//=================================================================================================
void Engine::SetVsync(bool new_vsync)
{
	if(new_vsync == vsync)
		return;

	vsync = new_vsync;
	Reset(true);
}

//=================================================================================================
void Engine::GetResolutions(vector<Resolution>& v) const
{
	v.clear();
	uint display_modes = d3d->GetAdapterModeCount(used_adapter, DISPLAY_FORMAT);
	for(uint i = 0; i < display_modes; ++i)
	{
		D3DDISPLAYMODE d_mode;
		V(d3d->EnumAdapterModes(used_adapter, DISPLAY_FORMAT, i, &d_mode));
		if(d_mode.Width >= (uint)MIN_WINDOW_SIZE.x && d_mode.Height >= (uint)MIN_WINDOW_SIZE.y)
			v.push_back({ Int2(d_mode.Width, d_mode.Height), d_mode.RefreshRate });
	}
}

//=================================================================================================
void Engine::GetMultisamplingModes(vector<Int2>& v) const
{
	v.clear();
	for(int j = 2; j <= 16; ++j)
	{
		DWORD levels, levels2;
		if(SUCCEEDED(d3d->CheckDeviceMultiSampleType(used_adapter, D3DDEVTYPE_HAL, BACKBUFFER_FORMAT, FALSE, (D3DMULTISAMPLE_TYPE)j, &levels))
			&& SUCCEEDED(d3d->CheckDeviceMultiSampleType(used_adapter, D3DDEVTYPE_HAL, ZBUFFER_FORMAT, FALSE, (D3DMULTISAMPLE_TYPE)j, &levels2)))
		{
			int level = min(levels, levels2);
			for(int i = 0; i < level; ++i)
				v.push_back(Int2(j, i));
		}
	}
}

//=================================================================================================
void Engine::RegisterShader(ShaderHandler* shader)
{
	assert(shader);
	shaders.push_back(shader);
	shader->OnInit();
}
