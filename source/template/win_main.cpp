
// #define COBJMACROS
#include <windows.h>
#include <dsound.h>

#include "app.h"
#include APP_HEADER_FILENAME

#include "win_functions.h"

#include "d3d11_layer.h"

// STB LIBRARIES
//TODO: assets will use my own format so in the future use this just to convert image formats
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "libraries/stb_image.h"

#define STB_RECT_PACK_IMPLEMENTATION
#include "libraries/stb_rect_pack.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "libraries/stb_truetype.h"
// STB END

struct App_dll
{
	FILETIME dll_last_write_time;
	HMODULE dll;

	UPDATE_TYPE(update);
	RENDER_TYPE(render);
	INIT_TYPE(init);
	CLOSE_TYPE(close_app);
};

struct Audio_output
{
	u32 hz;
	u32 channels;
	u32 bytes_per_sample;
	u32 bytes_per_full_sample;
	u32 bits_per_sample;
	u32 buffer_size;
	LPDIRECTSOUNDBUFFER buffer;
};


#if DEBUGMODE
	global_variable u32 E_LAST_FLAG_BIT_POS;
#endif

// ENTRY POINT


int WINAPI 
wWinMain(HINSTANCE h_instance, HINSTANCE h_prev_instance, PWSTR cmd_line, int cmd_show)
{h_prev_instance; cmd_line; cmd_show; //unreferenced formal parameters
	

	RECT winrect = {0,0,1600,900};
	AdjustWindowRectEx(&winrect, WS_OVERLAPPEDWINDOW,0,0);
	Int2 win_size = {winrect.right-winrect.left, winrect.bottom-winrect.top};

	// MAIN MEMORY BLOCKS

	#if DEBUGMODE // this is so that pointers always point to the same address
		LPVOID base_address = (LPVOID)TERABYTES(2);
	#else
		LPVOID base_address = 0;
	#endif

	Memory_arena arena1 = {0};
	arena1.size = GIGABYTES(2); 
	arena1.data = (u8*)VirtualAlloc(base_address, arena1.size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	Memory_arena* permanent_arena = &arena1;
	if(!arena1.data){
		if(GetLastError() == ERROR_INVALID_PARAMETER){
			ASSERT(false);// probably compiling in x86 x32 bits and size too big
		}else{
			ASSERT(false);
		}
	}


	Memory_arena arena2 = {0};
	arena2.size = GIGABYTES(2);
	arena2.data = (u8*)VirtualAlloc(0, arena2.size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	Memory_arena* temp_arena = &arena2;
	if(!arena2.data){
		if(GetLastError() == ERROR_INVALID_PARAMETER){
			ASSERT(false);// probably compiling in x86 x32 bits and size too big
		}else{
			ASSERT(false);
		}
	}

	Memory_arena arena3 = {0};
	arena3.size = MEGABYTES(256);
	arena3.data = (u8*)VirtualAlloc(0, arena3.size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	Memory_arena* assets_arena = &arena3;
	if(!arena1.data){
		if(GetLastError() == ERROR_INVALID_PARAMETER){
			ASSERT(false);// probably compiling in x86 x32 bits and size too big
		}else{
			ASSERT(false);
		}
	}


	Platform_data memory = {0};
	b32 app_size = sizeof(memory) < 65535;
	ASSERT(app_size);
	memory.temp_arena = temp_arena;
	memory.permanent_arena = permanent_arena;

	
	App_data* app_data = (App_data*)VirtualAlloc(0, sizeof(App_data), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	// LOADING APP DLL


	char* dll_names [] = APP_DLL_NAMES;
	App_dll apps [ARRAYCOUNT(dll_names)]= {0};
	u32 apps_count = ARRAYCOUNT(dll_names);
	u32 current_app = 0;
	{
		UNTIL(dll_i, apps_count)
		{
			apps[dll_i].dll = win_load_game_dll(dll_names[dll_i], &apps[dll_i].dll_last_write_time);
			ASSERT(apps[dll_i].dll);
			apps[dll_i].update = (UPDATE_TYPE( ))GetProcAddress(apps[dll_i].dll, "update");
			apps[dll_i].render = (RENDER_TYPE( ))GetProcAddress(apps[dll_i].dll, "render");
			apps[dll_i].init = (INIT_TYPE( ))GetProcAddress(apps[dll_i].dll, "init");
			apps[dll_i].close_app = (CLOSE_TYPE( ))GetProcAddress(apps[dll_i].dll, "close_app");
			ASSERT(apps[dll_i].update);
			ASSERT(apps[dll_i].render);
			ASSERT(apps[dll_i].init);
		}
	}

	// APP INIT

	memory.file_io.read_file = &win_read_file;
	memory.file_io.write_file = &win_write_file;
	memory.file_io.file_exists = &win_file_exists;
	memory.file_io.delete_file = &win_delete_file;
	memory.file_io.copy_file = &win_copy_file;
	memory.file_io.get_current_directory = &win_get_current_directory;
	memory.file_io.list_all_files = &win_list_all_files;

	memory.win_time.get_current_date = &win_get_current_date;
	memory.win_time.offset_date_by_days = &win_offset_date_by_days;

	Init_data init_data = {0};
	{
		Asset_request* request;
		// DEFAULT TEXTURE VIEW
		{
			PUSH_BACK(init_data.asset_requests, memory.temp_arena, request)
			
			u32* white_pixel = ARENA_PUSH_STRUCT(memory.temp_arena, u32);
			*white_pixel = 0xffffffff;
			Surface whitetex ={1,1, white_pixel};
			
			request->type = ASSET_REQUEST_TEX_FROM_SURFACE;
			request->tex_surface = whitetex;

			request->p_uid = ARENA_PUSH_STRUCT(memory.temp_arena, u16);
		}
		// DEFAULT VERTEX SHADER
		{
			DEFINE_ARRAY(char*, ie_names, memory.temp_arena, 
				{
					DEFAULT_IED_NAMES
				}
			);
			DEFINE_ARRAY(IE_FORMATS, ie_formats, memory.temp_arena, 
				{
					DEFAULT_IED_FORMATS
				}
			);
			Input_element_desc default_ied = {ie_names, ie_formats, 5};

			PUSH_BACK(init_data.asset_requests, memory.temp_arena, request);
			request->type = ASSET_REQUEST_VS_FROM_FILE;
			request->filename = string("shaders/3d_vs.cso");
			ASSERT(win_file_exists(request->filename.text));
			request->p_uid = ARENA_PUSH_STRUCT(memory.temp_arena, u16);
			request->ied = default_ied;
		}
		// DEFAULT PIXEL SHADER
		{
			PUSH_BACK(init_data.asset_requests, memory.temp_arena, request);
         request->type = ASSET_REQUEST_PS_FROM_FILE;
         request->filename = string("shaders/simple_ps.cso");
			ASSERT(win_file_exists(request->filename.text));
         request->p_uid = ARENA_PUSH_STRUCT(memory.temp_arena, u16);
		}

		// DEFAULT MESH
		{
			PUSH_BACK(init_data.asset_requests, memory.temp_arena, request);
			DEFINE_ARRAY(u16, plane_indices, memory.temp_arena,{0,1,2, 2,1,3});
			DEFINE_ARRAY(Vertex, plane, memory.temp_arena, 
				{
					{{0, 0, 0}, 	{0, 0}, 	{0,0,-1.0f}, {1.0f}},
					{{1.0f, 0, 0}, 	{1, 0},		{0,0,-1.0f}, {1.0f}},
					{{0, -1.0f, 0}, 	{0, 1},		{0,0,-1.0f}, {1.0f}},
					{{1.0f, -1.0f, 0}, 	{1, 1},	{0,0,-1.0f}, {1.0f}}
				}
			);

			request->type = ASSET_REQUEST_MESH_FROM_PRIMITIVES;
			request->p_uid = ARENA_PUSH_STRUCT(memory.temp_arena, u16);
			request->mesh_primitives.vertices = plane;
			request->mesh_primitives.vertex_size = sizeof(plane[0]);
			request->mesh_primitives.vertex_count = ARRAYLEN(plane);
			request->mesh_primitives.indices = plane_indices;
			request->mesh_primitives.indices_count = ARRAYLEN(plane_indices);
			request->mesh_primitives.topology_uid = TOPOLOGY_TRIANGLE_LIST;
		}

		// DEFAULT BLEND STATE
		{
			PUSH_BACK(init_data.asset_requests, memory.temp_arena, request);
			request->type = ASSET_REQUEST_CREATE_BLEND_STATE;
			request->enable_alpha_blending = 0;
			request->p_uid = ARENA_PUSH_STRUCT(memory.temp_arena, u16);
		}

		// DEFAULT DEPTH STENCIL IS ALREADY CREATED

		// LIST(Depth_stencil, depth_stencils_list) = {0};
		// {
		// 	Depth_stencil* null_depth_stencil;
		// 	PUSH_BACK(depth_stencils_list, assets_arena, null_depth_stencil);
		// }

		// TODO: DEFAULT FONT

		// ASSERT(false);
      // NEW_ASSET_REQUEST;
      // request->type = ASSET_REQUEST_FONT_FROM_FILE;
      // request->filename = string("data/Inconsolata-Regular.ttf");
      // request->font_uid = &assets->fonts.default_font;
      // request->font_lines_height = 18.0f;
		
		
		// CREATING DEFAULT CONSTANT BUFFERS
		
		PUSH_BACK(init_data.asset_requests, memory.temp_arena, request);
      request->type = ASSET_REQUEST_CREATE_CONSTANT_BUFFER;
      request->constant_buffer.register_index = REGISTER_INDEX_VS_OBJECT_DATA;
      request->constant_buffer.size = sizeof(Object_buffer_data);

		PUSH_BACK(init_data.asset_requests, memory.temp_arena, request);
      request->type = ASSET_REQUEST_CREATE_CONSTANT_BUFFER;
      request->constant_buffer.register_index = REGISTER_INDEX_VS_PROJECTION_MATRIX;
      request->constant_buffer.size = sizeof(Matrix);

		PUSH_BACK(init_data.asset_requests, memory.temp_arena, request);
      request->type = ASSET_REQUEST_CREATE_CONSTANT_BUFFER;
      request->constant_buffer.register_index = REGISTER_INDEX_VS_WORLD_VIEW_MATRIX;
      request->constant_buffer.size = sizeof(Matrix);

		PUSH_BACK(init_data.asset_requests, memory.temp_arena, request);
      request->type = ASSET_REQUEST_CREATE_CONSTANT_BUFFER;
      request->constant_buffer.register_index = REGISTER_INDEX_VS_CAMERA_POS;
      request->constant_buffer.size = sizeof(V4);
		
		PUSH_BACK(init_data.asset_requests, memory.temp_arena, request);
      request->type = ASSET_REQUEST_CREATE_CONSTANT_BUFFER;
      request->constant_buffer.register_index = REGISTER_INDEX_PS_SCREEN_DATA;
      request->constant_buffer.size = sizeof(Int2);

		PUSH_BACK(init_data.asset_requests, memory.temp_arena, request);
      request->type = ASSET_REQUEST_CREATE_CONSTANT_BUFFER;
      request->constant_buffer.register_index = REGISTER_INDEX_PS_TIME;
      request->constant_buffer.size = sizeof(float);
		
		PUSH_BACK(init_data.asset_requests, memory.temp_arena, request);
		request->type = ASSET_REQUEST_CREATE_DEPTH_STENCIL;
		request->p_uid = ARENA_PUSH_STRUCT(memory.temp_arena, u16);
		request->enable_depth = 0;

		UNTIL(app_i, apps_count)
		{
			apps[app_i].init(&memory, app_data, &init_data);
		}
	}	



	// WINDOW CREATION


	WNDCLASSA window_class = {0};
	window_class.style = CS_VREDRAW|CS_HREDRAW;
	window_class.lpfnWndProc = win_main_window_proc;
	window_class.hInstance = h_instance;
	window_class.lpszClassName = "classname";
	window_class.hCursor = LoadCursor(0, IDC_ARROW);

	RegisterClassA(&window_class);

	DWORD exstyle = WS_OVERLAPPEDWINDOW|WS_VISIBLE;
	
	
	HWND window;
	{
		char window_title[256] = "THE window";
		if(init_data.window_title.text)
		{
			copy_mem(init_data.window_title.text, window_title, init_data.window_title.length);
		}
		window = CreateWindowExA(
			0,// WS_EX_TOPMOST,
			window_class.lpszClassName,
			window_title,
			exstyle,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			// win_size.x,
			// win_size.y,
			0,
			0,
			window_class.hInstance,
			0
		);
		ASSERT(window);
		if(!window)
		{
			MessageBoxA(0, "CreateWindowExA failed", 0, MB_OK|MB_ICONERROR);
			return 1; 
		} 
	}

	ShowWindow(window, SW_MAXIMIZE); // MAXIMIZING WINDOW
	

	// SETTING WINDOW PROPERTIES TO ACCESS THEM FROM THE FUNCTION_WINDOW_PROCEDURE


	Int2 client_size = win_get_client_sizes(window);
	SetPropA(window, "enforce_aspect_ratio", &memory.enforce_aspect_ratio);
	SetPropA(window, "client_size", &client_size);
	SetPropA(window, "close_app", &memory.close_app);
	

	// FRAME CAPPING SETUP

	
	UINT desired_scheduler_ms = 1;
	b32 sleep_is_granular = (timeBeginPeriod(desired_scheduler_ms) == TIMERR_NOERROR);

	LARGE_INTEGER pcf_result;
	ASSERT(QueryPerformanceFrequency(&pcf_result));
	s64 performance_counter_frequency = pcf_result.QuadPart;

	//TODO: maybe in the future use GetDeviceCaps() to get the monitor hz
	int monitor_refresh_hz = 60;
	memory.fixed_dt = 1.0f/60.0f;
	
	memory.update_hz = (f32)monitor_refresh_hz;
	
	

	// DIRECTX11

	
	// INITIALIZE DIRECT3D
	
	HRESULT hr;
	

	D3D d3d = {0};
	D3D* dx = &d3d;
	#if DEBUGMODE
		// Create an instance of the DXGI debug interface

		IDXGIDebug1* p_debug = 0;
		hr = DXGIGetDebugInterface1(0, IID_PPV_ARGS(&p_debug));
		ASSERTHR(hr);
		// Enable DXGI object tracking
		p_debug->EnableLeakTrackingForThread();
	#endif

	// CREATE DEVICE AND SWAPCHAIN
	{
		D3D_FEATURE_LEVEL feature_levels[] = 
		{
			D3D_FEATURE_LEVEL_11_0,
			// D3D_FEATURE_LEVEL_10_1,
			// D3D_FEATURE_LEVEL_10_0,
			// D3D_FEATURE_LEVEL_9_3,
			// D3D_FEATURE_LEVEL_9_2,
			// D3D_FEATURE_LEVEL_9_1,
		};
		D3D_FEATURE_LEVEL result_feature_level;

		#if DEBUGMODE
			u32 create_device_flags = DX11_CREATE_DEVICE_DEBUG_FLAG;
		#else
			u32 create_device_flags = 0;
		#endif

		hr = D3D11CreateDevice(0, D3D_DRIVER_TYPE_HARDWARE, 0, create_device_flags, feature_levels, 
			ARRAYCOUNT(feature_levels), D3D11_SDK_VERSION, &dx->device, &result_feature_level, &dx->context);
		
		if(!SUCCEEDED(hr))
		{
			s32 error_code = 0;
			error_code = hr;
			String error_code_string = s32_to_string(error_code, temp_arena);
			String error_string =  concat_strings(string("D3D11CreateDevice failed: "), error_code_string, temp_arena);

			MessageBoxA(window, error_string.text, 0, MB_OK|MB_ICONERROR);
			return 11;
		}


	#if DEBUGMODE
		//for debug builds enable debug break on API errors
		ID3D11InfoQueue* info;
		dx->device->QueryInterface(IID_ID3D11InfoQueue, (void**)&info);
		info->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		info->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, TRUE);
		info->Release();

		// hresult return values should not be checked anymore
		// cuz debugger will break on errors but i will check them anyway
	#endif

		IDXGIDevice1* dxgi_device;
		hr = dx->device->QueryInterface(IID_IDXGIDevice1, (void**)&dxgi_device);
		ASSERTHR(hr);
		
		
		IDXGIAdapter* dxgi_adapter;
		hr = dxgi_device->GetAdapter(&dxgi_adapter);
		ASSERTHR(hr);
		

		IDXGIFactory2* factory;
		hr = dxgi_adapter->GetParent(IID_IDXGIFactory2, (void**)&factory);
		ASSERTHR(hr);
		
		DXGI_SWAP_CHAIN_DESC1 scd = {0};
		// default 0 value for width & height means to get it from window automatically
		//.Width = 0,
		//.Height = 0,
		scd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		scd.SampleDesc = {1, 0};
		scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		scd.BufferCount = 2;
		scd.Scaling = DXGI_SCALING_NONE;
		scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		hr = factory->CreateSwapChainForHwnd(dx->device, window, &scd, 0, 0, &dx->swap_chain);
		ASSERTHR(hr);

		// disable Alt+Enter changing monitor resolution to match window size
		factory->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER);

		//TODO: this is dumb
		factory->Release();
		dxgi_adapter->Release();
		dxgi_device->Release();
	}


	// DIRECT SOUND INITIALIZATION


	// GET A DIRECT SOUND OBJECT


	LPDIRECTSOUND direct_sound;
	hr = DirectSoundCreate(0, &direct_sound, 0);
	ASSERTHR(hr);

	hr = direct_sound->SetCooperativeLevel(window, DSSCL_PRIORITY);
	ASSERTHR(hr);


	// CREATE PRIMARY BUFFER (JUST TO SET CONFIGURATION)

	
	LPDIRECTSOUNDBUFFER primary_buffer;

	{
		DSBUFFERDESC buffer_desc = {0};
		buffer_desc.dwSize = sizeof(buffer_desc);
		buffer_desc.dwFlags = DSBCAPS_PRIMARYBUFFER;

		hr = direct_sound->CreateSoundBuffer(&buffer_desc, &primary_buffer, 0);
		ASSERTHR(hr);
	}


	// SET FORMAT WITH THE PRIMARY BUFFER


	Audio_output audio;

	audio.hz = 44100;
	audio.channels = 2;
	audio.bytes_per_sample = sizeof(s16);
	audio.bytes_per_full_sample = audio.channels * audio.bytes_per_sample;
	audio.bits_per_sample = sizeof(s16)*8;

	WAVEFORMATEX wave_format = {};
	wave_format.wFormatTag = WAVE_FORMAT_PCM;
	wave_format.nChannels = (WORD)audio.channels;
	wave_format.nSamplesPerSec = audio.hz;
	wave_format.nAvgBytesPerSec = audio.bytes_per_full_sample * audio.hz;
	wave_format.nBlockAlign = (WORD)audio.bytes_per_full_sample;
	wave_format.wBitsPerSample = (WORD)audio.bits_per_sample;
	wave_format.cbSize = 0;
	hr = primary_buffer->SetFormat(&wave_format);
	ASSERTHR(hr);


	// CREATE SECONDARY BUFFER (ACTUALLY WRITE TO IT)


	audio.buffer_size = 3 * audio.hz * audio.bytes_per_sample * audio.channels;
	{
		DSBUFFERDESC buffer_desc = {0};
		buffer_desc.dwSize = sizeof(buffer_desc);
		buffer_desc.dwFlags = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS;
		buffer_desc.dwBufferBytes = audio.buffer_size;
		buffer_desc.lpwfxFormat = &wave_format; 
		hr = direct_sound->CreateSoundBuffer(&buffer_desc, &audio.buffer, 0);
		ASSERTHR(hr);
	}

	audio.buffer->Play(0, 0, DSBPLAY_LOOPING);



	// PREPARING ASSET LISTS


	// TODO: make this more fail proof
	static D3D11_PRIMITIVE_TOPOLOGY topologies_list [TOPOLOGIES_COUNT];
	topologies_list[TOPOLOGY_LINE_LIST] = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
	topologies_list[TOPOLOGY_LINE_STRIP] = D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
	topologies_list[TOPOLOGY_TRIANGLE_LIST] = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	topologies_list[TOPOLOGY_TRIANGLE_STRIP] = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
	topologies_list[TOPOLOGY_POINTS_LIST] = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;

	static DXGI_FORMAT ie_formats_list [IE_FORMATS_COUNT];
	ie_formats_list[IE_FORMAT_TYPELESS] = DXGI_FORMAT_R32_TYPELESS;
	ie_formats_list[IE_FORMAT_U16] = DXGI_FORMAT_R16_UINT;
	ie_formats_list[IE_FORMAT_U32] = DXGI_FORMAT_R32_UINT;
	ie_formats_list[IE_FORMAT_3U32] = DXGI_FORMAT_R32G32B32_UINT;
	ie_formats_list[IE_FORMAT_S32] = DXGI_FORMAT_R32_SINT;
	ie_formats_list[IE_FORMAT_2S32] = DXGI_FORMAT_R32G32_SINT;
	ie_formats_list[IE_FORMAT_F32] = DXGI_FORMAT_R32_FLOAT;
	ie_formats_list[IE_FORMAT_2F32] = DXGI_FORMAT_R32G32_FLOAT;
	ie_formats_list[IE_FORMAT_3F32] = DXGI_FORMAT_R32G32B32_FLOAT;
	ie_formats_list[IE_FORMAT_4F32] = DXGI_FORMAT_R32G32B32A32_FLOAT;
	ie_formats_list[IE_FORMAT_16F32] = DXGI_FORMAT_R32G32B32A32_FLOAT;

	static u32 ie_formats_sizes [IE_FORMATS_COUNT];
	ie_formats_sizes[IE_FORMAT_TYPELESS] = 0;
	ie_formats_sizes[IE_FORMAT_U16] = 2;
	ie_formats_sizes[IE_FORMAT_U32] = 4;
	ie_formats_sizes[IE_FORMAT_3U32] = 12;
	ie_formats_sizes[IE_FORMAT_S32] = 4;
	ie_formats_sizes[IE_FORMAT_2S32] = 8;
	ie_formats_sizes[IE_FORMAT_F32] = 4;
	ie_formats_sizes[IE_FORMAT_2F32] = 8;
	ie_formats_sizes[IE_FORMAT_3F32] = 12;
	ie_formats_sizes[IE_FORMAT_4F32] = 16;
	ie_formats_sizes[IE_FORMAT_16F32] = 64;
	
	LIST(Dx11_texture_view*, textures_list) = {0};
	LIST(Vertex_shader, vertex_shaders_list) = {0};
	LIST(Pixel_shader, pixel_shaders_list) = {0};
	LIST(Dx_mesh, meshes_list) = {0};
	LIST(Dx11_blend_state*, blend_states_list) = {0};
	LIST(Depth_stencil, depth_stencils_list) = {0};
	{
		// Depth_stencil* null_depth_stencil;
		// PUSH_BACK(depth_stencils_list, assets_arena, null_depth_stencil);
	}

	// RENDER TARGET VIEWS RTV's
	LIST(Render_target, render_targets_list) = {0};
	Render_target* screen_render_target;
	PUSH_BACK(render_targets_list, assets_arena, screen_render_target);

	// this is so that when i use the index 0 for the REQUEST_FLAG_SET_SHADER_RESOURCE_FROM_RENDER_TARGET
	// the texture_view pointer will be a 0 so that means unset the resource
	screen_render_target->texture_view = ARENA_PUSH_STRUCT(assets_arena, Dx11_texture_view*);

	D3D_constant_buffer renderer_variables [28] = {0}; // 14 constant buffers per pipeline, vertex and pixel


	Sound_sample sounds_list [100] = {0};// TODO: THIS IS KEEPING ME FROM SKIPPING win_layer COMPILATION (i forgor why)
	{
		u32 MAX_SOUNDS = ARRAYCOUNT(sounds_list);
		#ifndef SOUNDS_COUNT
		#define SOUNDS_COUNT 0
		ASSERT(MAX_SOUNDS > SOUNDS_COUNT);
		#endif
	}

	ARRAY_DECLARATION(Audio_playback, playback_array, 300, assets_arena);

	u32 assets_count = 0;

	FOREACH(Asset_request, request, init_data.asset_requests)
	{
		switch(request->type){
			case ASSET_REQUEST_TEX_FROM_FILE:{
				ASSERT(win_file_exists(request->filename.text));
				ASSERT(request->p_uid);

				int comp;
				Surface tex_surface = {0};
				char temp_buffer [MAX_PATH] = {0}; 
				copy_mem(request->filename.text, temp_buffer, request->filename.length);
				tex_surface.data = stbi_load(temp_buffer, (int*)&tex_surface.width, (int*)&tex_surface.height, &comp, STBI_rgb_alpha);
				ASSERT(tex_surface.data);
				
				*request->p_uid = (u16)LIST_SIZE(memory.tex_infos);
				Tex_info* tex_info; PUSH_BACK(memory.tex_infos, assets_arena, tex_info);
				tex_info->w = tex_surface.width;
				tex_info->h = tex_surface.height;
				tex_info->texrect.x = 0.0f;
				tex_info->texrect.y = 0.0f;
				tex_info->texrect.w = 1.0f;
				tex_info->texrect.h = 1.0f;

				tex_info->texture_uid = LIST_SIZE(textures_list);
				Dx11_texture_view** texture_view; PUSH_BACK(textures_list, assets_arena, texture_view);
				ID3D11Texture2D* texture2d = dx11_create_texture2d(dx, &tex_surface);
				dx11_create_texture_view(dx, texture2d, texture_view);
				texture2d->Release();
			}break;


			case ASSET_REQUEST_VS_FROM_FILE:{
				// COMPILING VS
					// File_data compiled_vs = dx11_get_compiled_shader(request->filename, temp_arena, "vs", VS_PROFILE);
				// CREATING VS
				File_data compiled_vs = win_read_file(request->filename, temp_arena);
				u32 current_index = LIST_SIZE(vertex_shaders_list);
				ASSERT(current_index < 0xffff);
				*request->p_uid = (u16)current_index;
				Vertex_shader* vs; PUSH_BACK(vertex_shaders_list, assets_arena, vs);
				dx11_create_vs(dx, compiled_vs, &vs->shader);

				vs->filename.length = request->filename.length;
				vs->filename.text = (char*)arena_push_data(permanent_arena, request->filename.text, request->filename.length);
				arena_push_size(permanent_arena,1);
				vs->last_write_time = win_get_last_write_time(vs->filename.text);
				

				s32 MAX_IE_SIZE = sizeof(f32)*4;
				
				u32 ie_count = 0;
				#define USE_APPEND_ALIGNED_ELEMENT 1

				#if USE_APPEND_ALIGNED_ELEMENT
				#else
					u32 aligned_byte_offsets [2] = {0};
				#endif
				D3D11_INPUT_ELEMENT_DESC* ied = (D3D11_INPUT_ELEMENT_DESC*)(temp_arena->data+temp_arena->used);
				s32 total_element_size = 0;
				UNTIL(j, ARRAYLEN(request->ied.names))
				{
					s32 current_element_size = ie_formats_sizes[request->ied.formats[j]];
					total_element_size += current_element_size;
					for(s32 semantic_index = 0; current_element_size > 0; semantic_index++)
					{
						ie_count++;
						D3D11_INPUT_ELEMENT_DESC* current_ied = ARENA_PUSH_STRUCT(temp_arena, D3D11_INPUT_ELEMENT_DESC);
						current_ied->SemanticName = request->ied.names[j];
						// this is in case the element is bigger than a float4 (a matrix for example)
						current_ied->SemanticIndex = semantic_index;
						
						current_ied->Format = ie_formats_list[request->ied.formats[j]];

						u32 ie_slot = 0;
						if(request->ied.next_slot_beginning_index && j >= request->ied.next_slot_beginning_index)
						{
							ie_slot = 1;
						}
						
						#if USE_APPEND_ALIGNED_ELEMENT
							current_ied->AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
						#else
							current_ied->AlignedByteOffset = aligned_byte_offsets[ie_slot];
							aligned_byte_offsets[ie_slot] += MIN(MAX_IE_SIZE,current_element_size); 
						#endif
						current_element_size -= MAX_IE_SIZE;
						
						current_ied->InputSlot = ie_slot;// this is for using secondary buffers (like an instance buffer)
						current_ied->InputSlotClass = (D3D11_INPUT_CLASSIFICATION)ie_slot; // 0 PER_VERTEX_DATA vs 1 PER_INSTANCE_DATA
						current_ied->InstanceDataStepRate = ie_slot; // the amount of instances to draw using the PER_INSTANCE data
					}
				}
				UNTIL(e, ie_count)
				{
					ied[e];
					ASSERT(true);
				}
				hr = dx->device->CreateInputLayout(
					ied, ie_count, 
					compiled_vs.data, compiled_vs.size, 
					&vs->input_layout
				); 
				ASSERTHR(hr);
			}break;


			case ASSET_REQUEST_PS_FROM_FILE:{
				u32 current_index = LIST_SIZE(pixel_shaders_list);
				ASSERT(current_index < 0xffff);
				*request->p_uid = (u16)current_index;
				// COMPILING PS
					// File_data compiled_ps = dx11_get_compiled_shader(request->filename, temp_arena, "ps", PS_PROFILE);
				// CREATING PS
				File_data compiled_ps = win_read_file(request->filename, temp_arena);

				Pixel_shader* ps; PUSH_BACK(pixel_shaders_list, assets_arena, ps);
				dx11_create_ps(dx, compiled_ps, &ps->shader);

				ps->filename.length = request->filename.length;
				ps->filename.text = (char*)arena_push_data(permanent_arena, request->filename.text, request->filename.length);
				arena_push_size(permanent_arena,1);
				ps->last_write_time = win_get_last_write_time(ps->filename.text);
			}break;


			case ASSET_REQUEST_MESH_FROM_FILE:
			{
				File_data glb_file = win_read_file(request->filename, temp_arena);
				GLB glb = {0};
				glb_get_chunks(glb_file.data, 
					&glb);
				#if DEBUGMODE
					{ // THIS IS JUST FOR READABILITY OF THE JSON CHUNK
						void* formated_json = arena_push_size(temp_arena,MEGABYTES(4));
						u32 new_size = format_json_more_readable(glb.json_chunk, glb.json_size, formated_json);
						win_write_file(concat_strings(request->filename, string(".json"), temp_arena), formated_json, new_size);
						arena_pop_back_size(temp_arena, MEGABYTES(4));
					}
				#endif
				u32 meshes_count = 0;
				Gltf_mesh* meshes = gltf_get_meshes(&glb, temp_arena, &meshes_count);
				
				Mesh_primitive* primitives = ARENA_PUSH_STRUCTS(assets_arena, Mesh_primitive, meshes_count);
				for(u32 m=0; m<meshes_count; m++)
				{
					Gltf_primitive* mesh_primitive = meshes[m].primitives;
					//TODO: here i am assuming this mesh has only one primitive
					V3 normals [8];
					copy_mem(mesh_primitive->normals, normals, sizeof(V3)*8);
					primitives[m] = gltf_primitives_to_mesh_primitives(assets_arena, &mesh_primitive[0]);
					// u32 primitives_count = meshes[m].primitives_count;
					// for(u32 p=0; p<primitives_count; p++)
					// {	
					// }
				}
				u32 current_index = LIST_SIZE(meshes_list);
				ASSERT(current_index < 0xffff);
				*request->p_uid = (u16)current_index;
				
				Dx_mesh* current_mesh; PUSH_BACK(meshes_list, assets_arena, current_mesh);
				*current_mesh = dx11_init_mesh(dx, 
				&primitives[0],
				D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);	
			}break;


			case ASSET_REQUEST_CREATE_BLEND_STATE:{
				u32 current_index = LIST_SIZE(blend_states_list);
				ASSERT(current_index < 0xffff);
				*request->p_uid = (u16)current_index;

				Dx11_blend_state** blend_state; PUSH_BACK(blend_states_list, assets_arena, blend_state);
				if(!dx11_create_blend_state(dx, blend_state, request->enable_alpha_blending))
				{
					MessageBoxA(window, "CreateBlendState failed", 0, MB_OK|MB_ICONERROR);
					return 10;
				}
			}break;


			case ASSET_REQUEST_CREATE_DEPTH_STENCIL:{
				u32 current_index = LIST_SIZE(depth_stencils_list);
				ASSERT(current_index < 0xffff);
				*request->p_uid = (u16)current_index;

				Depth_stencil* depth_stencil; PUSH_BACK(depth_stencils_list, assets_arena, depth_stencil);
				dx11_create_depth_stencil_state(dx, &depth_stencil->state, request->enable_depth);
			}break;


			case ASSET_REQUEST_FONT_FROM_FILE:
			{
				ASSERT(request->font_lines_height);
				// FONTS 
				File_data font_file = win_read_file(request->filename, temp_arena);
				//currently just supporting ANSI characters
				//TODO: learn how to do UNICODE

				// GETTING BITMAPS AND INFO
				u32 atlas_texview_uid = LIST_SIZE(textures_list);

				stbtt_fontinfo font;
				Color32* charbitmaps [CHARS_COUNT];
				Tex_info temp_charinfos[CHARS_COUNT];
				stbtt_InitFont(&font, (u8*)font_file.data,stbtt_GetFontOffsetForIndex((u8*)font_file.data, 0));
				f32 font_size = stbtt_ScaleForPixelHeight(&font, request->font_lines_height);

				UNTIL(c, CHARS_COUNT)
				{
					u32 codepoint = c+FIRST_CHAR;

					temp_charinfos[c].texture_uid = atlas_texview_uid;

					u8* monobitmap = stbtt_GetCodepointBitmap(
						&font, 0, 
						font_size, 
						// stbtt_ScaleForMappingEmToPixels(&font, request->font_lines_height),
						codepoint,
						&temp_charinfos[c].w, &temp_charinfos[c].h, 
						&temp_charinfos[c].xoffset, &temp_charinfos[c].yoffset
					);
					if(temp_charinfos[c].w && temp_charinfos[c].h)
					{
						u32 bitmap_size = temp_charinfos[c].w * temp_charinfos[c].h;
						charbitmaps[c] = ARENA_PUSH_STRUCTS(temp_arena, Color32, bitmap_size);
						Color32* bitmap =  charbitmaps[c];
						UNTIL(p, bitmap_size){
							bitmap[p].r = 255;
							bitmap[p].g = 255;
							bitmap[p].b = 255;
							bitmap[p].a = monobitmap[p];
						}
					}
					stbtt_FreeBitmap(monobitmap,0);
				}
				// PACKING BITMAP RECTS

				//total_atlas_size = atlas_side*atlas_side

				s32 atlas_side;
				{
					
					s32 min_value = (u32)(request->font_lines_height*(request->font_lines_height/2)*CHARS_COUNT);
					s32 current_side = 2;
					while(min_value>(current_side*current_side)){
						current_side = current_side << 1;
					}
					atlas_side = current_side;

				}


				stbrp_context pack_context = {0};
				stbrp_node* pack_nodes = ARENA_PUSH_STRUCTS(temp_arena, stbrp_node, atlas_side);
				stbrp_init_target(&pack_context, atlas_side, atlas_side, pack_nodes, atlas_side);

				stbrp_rect* rects = ARENA_PUSH_STRUCTS(temp_arena, stbrp_rect, CHARS_COUNT);
				UNTIL(i, CHARS_COUNT){
					rects[i].w = temp_charinfos[i].w;
					rects[i].h = temp_charinfos[i].h;
				}
				stbrp_pack_rects(&pack_context, rects, CHARS_COUNT);

				// CREATE TEXTURE ATLAS AND COPY EACH CHARACTER BITMAP INTO IT USING THE POSITIONS OBTAINED FROM PACK
				Color32* atlas_pixels = ARENA_PUSH_STRUCTS(temp_arena, Color32, atlas_side*atlas_side);

				Int2 atlas_size = {atlas_side, atlas_side};

				*request->font_uid = (u16)LIST_SIZE(memory.fonts_list);
				Font* new_font;
				PUSH_BACK(memory.fonts_list, memory.permanent_arena, new_font);
				new_font->texinfos_count = CHARS_COUNT;
				new_font->texinfo_uids = ARENA_PUSH_STRUCTS(memory.permanent_arena, u32, new_font->texinfos_count);
				new_font->first_char = FIRST_CHAR;
				new_font->lines_height = request->font_lines_height;

				int ascent, descent, line_gap;
				
				stbtt_GetFontVMetrics(&font, &ascent, &descent, &line_gap);
				new_font->ascent = font_size * ascent;
				new_font->descent = font_size * descent;
				int x0, y0, x1, y1;
				stbtt_GetFontBoundingBox(&font, &x0, &y0, &x1, &y1);


				UNTIL(i, CHARS_COUNT){
					ASSERT(rects[i].was_packed);
					if(rects[i].was_packed){
						temp_charinfos[i].texrect.x = (f32)rects[i].x / atlas_size.x;
						temp_charinfos[i].texrect.y = (f32)rects[i].y / atlas_size.y;
						temp_charinfos[i].texrect.w = (f32)temp_charinfos[i].w / atlas_size.x;
						temp_charinfos[i].texrect.h = (f32)temp_charinfos[i].h / atlas_size.y;
						
						new_font->texinfo_uids[i] = LIST_SIZE(memory.tex_infos);
						Tex_info* charinfo; PUSH_BACK(memory.tex_infos, assets_arena, charinfo);
						*charinfo = temp_charinfos[i]; 

						// PASTING EACH CHAR INTO THE ATLAS PIXELS
						u32 first_char_pixel = (rects[i].y*atlas_side) + rects[i].x;
						Color32* charpixels = charbitmaps[i];
						UNTIL(y, (u32)rects[i].h){
							UNTIL(x, (u32)rects[i].w){
								u32 current_pixel = first_char_pixel + (y*atlas_side) + x;
								atlas_pixels[current_pixel] = charpixels[(y*rects[i].w) + x];
							}
						}
					}
				}
				new_font->atlas_texinfo_uid = (u16)LIST_SIZE(memory.tex_infos);
				Tex_info* atlas_tex_info; PUSH_BACK(memory.tex_infos, assets_arena, atlas_tex_info);
				atlas_tex_info->texture_uid = atlas_texview_uid;
				atlas_tex_info->w = atlas_size.x;
				atlas_tex_info->h = atlas_size.y;
				atlas_tex_info->texrect.x = 0.0f;
				atlas_tex_info->texrect.y = 0.0f;
				atlas_tex_info->texrect.w = 1.0f;
				atlas_tex_info->texrect.h = 1.0f;

				Dx11_texture_view** atlas_texture; PUSH_BACK(textures_list, assets_arena, atlas_texture);
				Surface atlas_surface = {(u32)atlas_size.x, (u32)atlas_size.y, atlas_pixels};
				ID3D11Texture2D* texture2d = dx11_create_texture2d(dx, &atlas_surface);
				dx11_create_texture_view(dx, texture2d, atlas_texture);
				texture2d->Release();
			}break;


			case ASSET_REQUEST_TEX_FROM_SURFACE:{
				u32 current_index = LIST_SIZE(memory.tex_infos);
				ASSERT(current_index < 0xffff);
				*request->p_uid = (u16)current_index;

				Tex_info* tex_info; PUSH_BACK(memory.tex_infos, assets_arena, tex_info);
				tex_info->w = request->tex_surface.width;
				tex_info->h = request->tex_surface.height;

				tex_info->texrect.x = 0.0f;
				tex_info->texrect.y = 0.0f;
				tex_info->texrect.w = 1.0f;
				tex_info->texrect.h = 1.0f;
				
				ID3D11Texture2D* texture2d = dx11_create_texture2d(dx, &request->tex_surface);

				tex_info->texture_uid = LIST_SIZE(textures_list);
				Dx11_texture_view** texture_view; PUSH_BACK(textures_list, assets_arena, texture_view);
				dx11_create_texture_view(dx, texture2d, texture_view);
				texture2d->Release();
			}break;

			case ASSET_REQUEST_CREATE_DYNAMIC_TEXTURE:{
				if(request->dynamic_tex_sizes.x && !request->dynamic_tex_sizes.y && !request->dynamic_tex_sizes.z)
				{
					ASSERT(false);
				}
				else if(request->dynamic_tex_sizes.x && request->dynamic_tex_sizes.y && !request->dynamic_tex_sizes.z)
				{
					D3D11_TEXTURE2D_DESC tex2d_desc = {0};
					tex2d_desc.Width = request->dynamic_tex_sizes.x;
					tex2d_desc.Height = request->dynamic_tex_sizes.y;
					tex2d_desc.ArraySize = 1;
					tex2d_desc.MipLevels = 1;
					// this is for multi-sampling and antialiasing apparently (no idea how it works)
					tex2d_desc.SampleDesc = {1,0}; 
					tex2d_desc.Format = DXGI_FORMAT_R32_UINT;
					tex2d_desc.Usage = D3D11_USAGE_DEFAULT;
					tex2d_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
					tex2d_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

					ID3D11Texture2D* texture2d; 
					hr = dx->device->CreateTexture2D(&tex2d_desc, 0, &texture2d);
					ASSERTHR(hr);

					*request->p_uid = (u16)LIST_SIZE(textures_list);
					Dx11_texture_view** texture_view; PUSH_BACK(textures_list, assets_arena, texture_view);
					dx11_create_texture_view(dx, texture2d, texture_view);
					texture2d->Release();

				}
				else if(request->dynamic_tex_sizes.x && request->dynamic_tex_sizes.y && request->dynamic_tex_sizes.z)
				{
					
					D3D11_TEXTURE3D_DESC tex3d_desc = {0};
					tex3d_desc.Width = request->dynamic_tex_sizes.x;
					tex3d_desc.Height = request->dynamic_tex_sizes.y;
					tex3d_desc.Depth = request->dynamic_tex_sizes.z;
					tex3d_desc.MipLevels = 1;
					tex3d_desc.Format = DXGI_FORMAT_R32_UINT;
					tex3d_desc.Usage = D3D11_USAGE_DEFAULT;
					tex3d_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
					tex3d_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

					ID3D11Texture3D* texture3d; 
					hr = dx->device->CreateTexture3D(&tex3d_desc, 0, &texture3d);
					ASSERTHR(hr);

					*request->p_uid = (u16)LIST_SIZE(textures_list);
					Dx11_texture_view** texture_view; PUSH_BACK(textures_list, assets_arena, texture_view);
					dx11_create_texture_view(dx, texture3d, texture_view);
					texture3d->Release();
				}
				else
				{
					ASSERT(false);
				}
								
			}break;

			case ASSET_REQUEST_CREATE_RTV:
			{
				*request->p_uid = (u16)LIST_SIZE(render_targets_list);
				Render_target* new_render_target;
				PUSH_BACK(render_targets_list, assets_arena, new_render_target);

				PUSH_BACK(textures_list, assets_arena, new_render_target->texture_view);
			}break;


			case ASSET_REQUEST_SOUND_FROM_FILE:{
				Sound_sample* new_audio_samples =  &sounds_list[request->sound_uid];
				ASSERT(!new_audio_samples->samples); // already saved in this index


				File_data audio_file = win_read_file(request->filename, temp_arena);
				u32* scan = (u32*)audio_file.data;
				ASSERT(scan[0] == 'FFIR'); // RIFF
				u32 file_size = scan[1];
				file_size;
				ASSERT(scan[2] == 'EVAW'); // WAVE
				ASSERT(scan[3] == ' tmf'); // fmt\0 format chunk identifier
				u32 format_chunk_size = scan[4];
				format_chunk_size;

				u16* scan16 = (u16*)&scan[5];
				u16 sample_format = scan16[0]; // 1 is PCM
				ASSERT(sample_format == 1);
				u16 channels_count = scan16[1];
				ASSERT(channels_count <= 2);

				u32 sample_hz = scan[6];
				ASSERT(sample_hz == 44100);
				u32 bytes_p_second = scan[7]; // sample_hz * bits_per_sample * channels / 8

				scan16 = (u16*)&scan[8];
				u16 bytes_per_full_sample = scan16[0]; // bits_per_sample*channels / 8
				u16 bits_per_sample = scan16[1]; 
				ASSERT(bits_per_sample == 16);
				ASSERT(bytes_p_second == (sample_hz*bytes_per_full_sample));


				ASSERT(scan[9] == 'atad'); // data
				u32 data_chunk_size = scan[10];

				s16* sample_values = (s16*)&scan[11];
				u16 bytes_per_sample = bits_per_sample / 8;

				new_audio_samples->samples_count = data_chunk_size/bytes_per_sample;
				new_audio_samples->channels = channels_count;
				new_audio_samples->samples = ARENA_PUSH_STRUCTS(assets_arena, s16, new_audio_samples->samples_count);

				UNTIL(i, new_audio_samples->samples_count){
					new_audio_samples->samples[i] = sample_values[i];
				}


			}break;


			case ASSET_REQUEST_MESH_FROM_PRIMITIVES:{
				u32 current_index = LIST_SIZE(meshes_list);
				ASSERT(current_index < 0xffff);
				*request->p_uid = (u16)current_index;

				Dx_mesh* current_mesh; PUSH_BACK(meshes_list, assets_arena, current_mesh);
				*current_mesh = dx11_init_mesh(dx, 
				&request->mesh_primitives, 
				topologies_list[request->mesh_primitives.topology_uid]);
			}break;

			case ASSET_REQUEST_CREATE_CONSTANT_BUFFER:{
				D3D_constant_buffer* new_constant_buffer = &renderer_variables[request->constant_buffer.register_index];
				Shader_constant_buffer_register_index register_index = (Shader_constant_buffer_register_index)(request->constant_buffer.register_index%14);

				u16 buffer_size = 16*((request->constant_buffer.size+15)/16);

				
				// ASSUMING I NEVER ASSIGN INITIAL DATA TO THE CONSTANT BUFFER
				dx11_create_constant_buffer(dx, new_constant_buffer, buffer_size, register_index, 0);
				if(request->constant_buffer.register_index < 14)
				{
					dx->context->VSSetConstantBuffers(register_index, 1, &new_constant_buffer->buffer);
				}
				else
				{
					dx->context->PSSetConstantBuffers(register_index, 1, &new_constant_buffer->buffer);
				}
			}break;

			case ASSET_REQUEST_CREATE_DYNAMIC_MESH:{
				u32 current_index = LIST_SIZE(meshes_list);
				ASSERT(current_index < 0xffff);
				*request->p_uid = (u16)current_index;

				Dx_mesh* current_mesh; 
				PUSH_BACK(meshes_list, assets_arena, current_mesh);
				current_mesh->topology = topologies_list[request->mesh_primitives.topology_uid];
				current_mesh->vertex_size = request->mesh_primitives.vertex_size;
				current_mesh->vertices_count = request->mesh_primitives.vertex_count;
				current_mesh->indices_count = request->mesh_primitives.indices_count;

				// VERTEX BUFFER

				D3D11_BUFFER_DESC bd = {0};
				bd.ByteWidth        = current_mesh->vertices_count * current_mesh->vertex_size;
				bd.Usage            = D3D11_USAGE_DYNAMIC;
				bd.BindFlags        = D3D11_BIND_VERTEX_BUFFER;
				bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
				
				D3D11_SUBRESOURCE_DATA buffer_init_data = {0};
				buffer_init_data.pSysMem  = request->mesh_primitives.vertices;
				if(request->mesh_primitives.vertices)
				{
					ASSERTHR(dx->device->CreateBuffer( &bd, &buffer_init_data, &current_mesh->vertex_buffer));
				}
				else
				{
					ASSERTHR(dx->device->CreateBuffer( &bd, 0, &current_mesh->vertex_buffer ));
				}

				// INDEX BUFFER
				if(request->mesh_primitives.indices_count)
				{
					bd = {0};
					bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
					bd.Usage = D3D11_USAGE_DYNAMIC;
					bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
					bd.MiscFlags = 0;
					bd.ByteWidth = current_mesh->indices_count*sizeof(u16);
					bd.StructureByteStride = sizeof(u16);

					buffer_init_data = {0};
					buffer_init_data.pSysMem = request->mesh_primitives.indices;
					
					if(request->mesh_primitives.indices)
					{
						ASSERTHR(dx->device->CreateBuffer(&bd, &buffer_init_data, &current_mesh->index_buffer));
					}
					else
					{
						ASSERTHR(dx->device->CreateBuffer(&bd, 0, &current_mesh->index_buffer));
					}
				}
			}break;

			case FORGOR_TO_SET_ASSET_TYPE:
			default:
				ASSERT(false);
			break;
		}
		assets_count++;
	}

	D3D_constant_buffer* object_buffer = &renderer_variables[0];


	// CREATING  D3D PIPELINES
	dx11_create_sampler(dx, &dx->sampler);
	dx11_bind_rasterizer_state(dx, dx->rasterizer_state);
	
	//CULL

	D3D11_FILL_MODE fill_modes_list [2];
	fill_modes_list[FILL_MODE_SOLID] = D3D11_FILL_SOLID;
	fill_modes_list[FILL_MODE_WIREFRAME] = D3D11_FILL_WIREFRAME;

	D3D11_CULL_MODE cull_modes_list [3];
	cull_modes_list[CULL_MODE_NONE] = D3D11_CULL_NONE;
	cull_modes_list[CULL_MODE_FRONT] = D3D11_CULL_FRONT;
	cull_modes_list[CULL_MODE_BACK] = D3D11_CULL_BACK;

	Dx11_rasterizer_desc global_rasterizer_desc = {0};
	global_rasterizer_desc.FillMode = fill_modes_list[0];
	global_rasterizer_desc.CullMode = cull_modes_list[0];
	hr = dx->device->CreateRasterizerState(&global_rasterizer_desc, &dx->rasterizer_state);
	ASSERTHR(hr);

#if PRINT_FRAMERATE
	u64 last_cycles_count = __rdtsc();
#endif
	LARGE_INTEGER last_counter;
	QueryPerformanceCounter(&last_counter);
	
	// TODO: input backbuffer, i actually don't know why would i need an input back buffer
	User_input input = {0};
	memory.input = &input;
	User_input holding_inputs = {0};
	memory.holding_inputs = &holding_inputs;

	memory.lock_mouse = false;

	u32 sample_t = 0;
	u32 last_byte_to_lock = 0;
	u64 initial_100ns;
	{
		FILETIME initial_time;
		GetSystemTimeAsFileTime(&initial_time);
		initial_100ns = (LONGLONG)initial_time.dwLowDateTime + ((LONGLONG)(initial_time.dwHighDateTime) << 32LL);
	}
	// Int2 smaller_client_size = {1600, 900};
	memory.renderer_needs_resizing = 1;
	memory.aspect_ratio = (((f32)client_size.x) / client_size.y);
	
	// MAIN LOOP ________________________________________________________________________________________________________________________

	HDC window_dc = GetDC(window);window_dc;
	//
	memory.screen_size.x = GetDeviceCaps(window_dc, HORZRES);
	memory.screen_size.y = GetDeviceCaps(window_dc, VERTRES);
	//TODO: test this way of doing it, but based on msdn they are the same
	// GetSystemMetrics(SM_CYSCREEN);
	// GetSystemMetrics(SM_CXSCREEN);

	// this function seems to give information about pen settings so i should check it out when i do that
	// SystemParametersInfoA
	{

		int keyboard_delay; // this value ranges from 0 through 3, which translates to 0.25s to 1s
		ASSERT(SystemParametersInfoA(SPI_GETKEYBOARDDELAY,0, &keyboard_delay,0));
		memory.keyboard_repeat_delay = (keyboard_delay+1)*0.25f;

		DWORD keyboard_speed; // this value ranges from 0 to 31, which translates to 2.5 to 30 repetitions per second
		ASSERT(SystemParametersInfoA(SPI_GETKEYBOARDSPEED,0, &keyboard_speed,0));

		f32 keyboard_repeat_rate = 2.5f + ((keyboard_speed / 31) * (30.0f - 2.5f));
		memory.keyboard_repeat_cooldown = 1 / keyboard_repeat_rate;
	}
	
   // PRE-PARING FOR RENDERING THIS FRAME
   {		
		// |REQUEST_FLAG_SET_PS
		{
			Pixel_shader* pixel_shader; LIST_GET(pixel_shaders_list, 0, pixel_shader);
			dx->context->PSSetShader(pixel_shader->shader, 0, 0);
		}
		// |REQUEST_FLAG_SET_VS
		{
			Vertex_shader* vertex_shader; LIST_GET(vertex_shaders_list, 0, vertex_shader);

			dx->context->VSSetShader(vertex_shader->shader, 0, 0);
			dx->context->IASetInputLayout(vertex_shader->input_layout);
		}
		// |REQUEST_FLAG_RESIZE_DEPTH_STENCIL_VIEW
		{
			Depth_stencil* resize_ds;
			LIST_GET(depth_stencils_list, 0, resize_ds);

			dx11_create_depth_stencil_view(dx, &resize_ds->view, client_size.x, client_size.y);
			dx->context->ClearDepthStencilView(
				resize_ds->view,
				D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
				1.0f, 0
			);
		}	
		// |REQUEST_FLAG_SET_RENDER_TARGET_AND_DEPTH_STENCIL
		{
			
			create_screen_render_target_view(dx, &screen_render_target->target_view);

			Depth_stencil* depth_stencil; LIST_GET(depth_stencils_list, 0, depth_stencil);
			// state can be null to set the default depth_stencil in 
			dx->context->OMSetDepthStencilState(depth_stencil->state, 0);
			
			Dx11_render_target_view** rtviews_to_bind = ARENA_PUSH_STRUCT(temp_arena, Dx11_render_target_view*);
			
			rtviews_to_bind[0] = screen_render_target->target_view;

			dx->context->OMSetRenderTargets(1, rtviews_to_bind, depth_stencil->view); 
		}
		// |REQUEST_FLAG_RESIZE_TARGET_VIEW
		{
			
		}

		// |REQUEST_FLAG_CHANGE_VIEWPORT_SIZE
		{
			dx11_set_viewport(dx, 0, 0, client_size.x, client_size.y);

		}
		// |REQUEST_FLAG_SET_RASTERIZER_STATE
		{
			dx->rasterizer_state->Release();
			global_rasterizer_desc.FillMode = fill_modes_list[FILL_MODE_SOLID];
			global_rasterizer_desc.CullMode = cull_modes_list[CULL_MODE_NONE];
			hr = dx->device->CreateRasterizerState(&global_rasterizer_desc, &dx->rasterizer_state);
			
			dx11_bind_rasterizer_state(dx, dx->rasterizer_state);

			ASSERTHR(hr);
			
		}
		// |REQUEST_FLAG_SET_BLEND_STATE
		{
			Dx11_blend_state** blend_state; LIST_GET(blend_states_list, 0, blend_state);
			dx->context->OMSetBlendState(*blend_state, 0, ~0U);   
		}

		// |REQUEST_FLAG_MODIFY_RENDERER_VARIABLE
		{
			//WORLD VIEW MATRIX
         Matrix world_view = matrix_translation( v3(0,0,0) );
         
         // WORLD PROJECTION
			Matrix projection;
         if(memory.perspective_on){
            projection = build_perspective_matrix(memory.aspect_ratio, memory.fov, 0.1f, 500.0f, memory.depth_effect);
         }else{         
            projection = build_orthographic_matrix(memory.aspect_ratio, 2.0f, 0.001f, 100.0f);
         }		

			dx11_modify_resource(dx, 
				renderer_variables[REGISTER_INDEX_VS_WORLD_VIEW_MATRIX].buffer, 
				&world_view, 
				renderer_variables[REGISTER_INDEX_VS_WORLD_VIEW_MATRIX].size
			);
			dx11_modify_resource(dx, 
				renderer_variables[REGISTER_INDEX_VS_PROJECTION_MATRIX].buffer, 
				&projection, 
				renderer_variables[REGISTER_INDEX_VS_PROJECTION_MATRIX].size
			);
		}
   }
	

	while(!memory.close_app)
	{
		// arena_pop_back_size(temp_arena, temp_arena->used);

		// THIS IS (A LOT!!) FASTER THAN MANUALLY CLEARING THE ARENA (at least with -Od, haven't compared with -O2)
		VirtualFree(temp_arena->data, 0, MEM_RELEASE);
		temp_arena->data = (u8*)VirtualAlloc(0, temp_arena->size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		temp_arena->used = 0;
		
		u8 pressed_keys [INPUT_COUNT]= {0};
		
		
		#if DEBUGMODE
		UNTIL(app_i, apps_count)
		{
			FILETIME dll_last_write_time = win_get_last_write_time(dll_names[app_i]);
			if(CompareFileTime(&dll_last_write_time, &apps[app_i].dll_last_write_time) != 0)
			{
				FreeLibrary(apps[app_i].dll);
				apps[app_i].dll = 0;
				apps[app_i].update = 0;
				apps[app_i].render = 0;
				apps[app_i].close_app = 0 ;

				apps[app_i].dll = win_load_game_dll(dll_names[app_i], &apps[app_i].dll_last_write_time);
				
				if(apps[app_i].dll)
				{
					apps[app_i].update = (UPDATE_TYPE( ))GetProcAddress(apps[app_i].dll, "update");
					apps[app_i].render = (RENDER_TYPE( ))GetProcAddress(apps[app_i].dll, "render");
					apps[app_i].close_app = (CLOSE_TYPE( ))GetProcAddress(apps[app_i].dll, "close_app");

					ASSERT(apps[app_i].update && apps[app_i].render);
				}
			}
		}

		FOREACH(Vertex_shader, current_vs, vertex_shaders_list)
		{
			FILETIME vs_last_write_time = win_get_last_write_time(current_vs->filename.text);
			if(CompareFileTime(&vs_last_write_time, &current_vs->last_write_time) != 0)
			{
				current_vs->shader->Release();
				String temp_filename = concat_strings(current_vs->filename, string(".temp"), temp_arena);
				win_copy_file(current_vs->filename, temp_filename);
				File_data compiled_vs = win_read_file(temp_filename, temp_arena);
				
				dx11_create_vs(dx, compiled_vs, &current_vs->shader);				
				current_vs->last_write_time = win_get_last_write_time(current_vs->filename.text);
			}
		}
		FOREACH(Pixel_shader, current_ps, pixel_shaders_list)
		{
			FILETIME ps_last_write_time = win_get_last_write_time(current_ps->filename.text);
			if(CompareFileTime(&ps_last_write_time, &current_ps->last_write_time) != 0)
			{
				current_ps->shader->Release();
				String temp_filename = concat_strings(current_ps->filename, string(".temp"), temp_arena);
				win_copy_file(current_ps->filename, temp_filename);
				File_data compiled_ps = win_read_file(temp_filename, temp_arena);
				
				dx11_create_ps(dx, compiled_ps, &current_ps->shader);				
				current_ps->last_write_time = win_get_last_write_time(current_ps->filename.text);
			}
		}
		#endif

		// HANDLE WINDOW RESIZING
		Int2 current_client_size = win_get_client_sizes(window);
		if( !screen_render_target->target_view || 
			client_size.x != current_client_size.x || 
			client_size.y != current_client_size.y
		)
		{
			memory.renderer_needs_resizing = true;
			s32 new_width = current_client_size.x;
			s32 new_height = current_client_size.y;

			RECT new_client_size = {0,0,new_width,new_height};
			client_size = {new_width, new_height};

			if(screen_render_target->target_view)
			{
				screen_render_target->target_view->Release();
				screen_render_target->target_view = 0;
			}
			
			//TODO: be careful with 8k monitors
			// why did i limit it to 4k again??
			if(client_size.x > 0 && client_size.y > 0 
			// && client_size.x < 4000 && client_size.y < 4000
			){
				ASSERT(client_size.x < 4000 && client_size.y < 4000);
				memory.aspect_ratio = (f32)client_size.x / (f32) client_size.y;
				hr = dx->swap_chain->ResizeBuffers(0, client_size.x, client_size.y, DXGI_FORMAT_UNKNOWN, 0);
				ASSERTHR(hr);

				create_screen_render_target_view(dx, &screen_render_target->target_view);
				
				Depth_stencil* resize_ds;
				LIST_GET(depth_stencils_list, 0, resize_ds);
				if(resize_ds->view)
				{
					resize_ds->view->Release();
					resize_ds->view = 0;
				}
				dx11_create_depth_stencil_view(dx, &resize_ds->view, client_size.x, client_size.y);
				dx->context->ClearDepthStencilView(
					resize_ds->view,
					D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
					1.0f, 0
				);

			}
		}

		// MOUSE POSITION
		HWND foreground_window = GetForegroundWindow();
		memory.is_window_in_focus = (foreground_window == window);
		if(memory.is_window_in_focus)
		{
			POINT mousep;
			GetCursorPos(&mousep);
			ScreenToClient(window, &mousep);

			RECT client_rect;
			GetClientRect(window, &client_rect); 

			Int2 client_center_pos = {
				client_rect.left + ((client_rect.right - client_rect.left)/2),
				client_rect.top + ((client_rect.bottom - client_rect.top)/2),
			};
			
			POINT center_point = { client_center_pos.x, client_center_pos.y };

			ClientToScreen(window, &center_point);

			input.cursor_speed = {0};
			input.cursor_pixels_pos = {mousep.x, mousep.y};
			f32 px = ((f32)(mousep.x - client_center_pos.x))/client_size.x;
			f32 py = -((f32)(mousep.y - client_center_pos.y))/client_size.y;
			input.cursor_speed.x = (2*px) - input.cursor_pos.x;
			input.cursor_speed.y = (2*py) - input.cursor_pos.y;
			if(memory.lock_mouse)
			{
				SetCursorPos(center_point.x, center_point.y);
				input.cursor_pos = {0,0};
			}
			else
			{
				input.cursor_pos = {2.0f*px, 2.0f*py};
			}
				
		}else{
			holding_inputs = {0};
		}
		input.delta_wheel = 0;
		set_mem(memory.input_chars_buffer, sizeof(memory.input_chars_buffer), 0);
		memory.input_chars_buffer_current_size = 0;
		// HANDLING MESSAGES
		MSG msg;
		while(PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
		{
			switch(msg.message)
			{
				case WM_MOUSEMOVE: // WM_MOUSEFIRST
				break;
				case WM_ACTIVATE:
					ASSERT(false);
				break;
				case WM_LBUTTONDBLCLK:
				break;
				case WM_LBUTTONDOWN:// just when the buttom is pushed
					set_input(holding_inputs.keys, pressed_keys, INPUT_CURSOR_PRIMARY, 1);
				break;
				case WM_LBUTTONUP:
					set_input(holding_inputs.keys, pressed_keys, INPUT_CURSOR_PRIMARY, 0);
				break;
				case WM_RBUTTONDOWN:
					set_input(holding_inputs.keys, pressed_keys, INPUT_CURSOR_SECONDARY, 1);
				break;
				case WM_RBUTTONUP:
					set_input(holding_inputs.keys, pressed_keys, INPUT_CURSOR_SECONDARY, 0);
				break;
				case WM_MOUSEWHEEL:
				{
					s16 delta_wheel = GET_WHEEL_DELTA_WPARAM(msg.wParam); 
					input.delta_wheel += delta_wheel/WHEEL_DELTA;
				}
				break;

				case WM_SYSKEYDOWN:
				case WM_SYSKEYUP:
				case WM_KEYDOWN:
				case WM_KEYUP:
				{
					u64 vkcode = msg.wParam;
					u16 repeat_count = (u16)msg.lParam; repeat_count;
					b32 was_down = ((msg.lParam & (1 <<  30)) != 0);
					b32 is_down = ((msg.lParam & (1 << 31)) == 0 );
					ASSERT(is_down == 0 || is_down == 1);
					if(is_down != was_down)
					{	
						#define SET_INPUT(k_index) set_input(holding_inputs.keys, pressed_keys, k_index, is_down)
						switch(vkcode){
							case VK_SPACE:
								SET_INPUT(INPUT_SPACE_BAR);
							break;
							case VK_TAB:
								SET_INPUT(INPUT_TAB);
							break;
							case VK_SHIFT:
								SET_INPUT(INPUT_SHIFT);
							break;
							case VK_CONTROL:
								SET_INPUT(INPUT_CONTROL);
							break;
							
							case 'Q':
								SET_INPUT(INPUT_Q);
							break;
							case 'W':
								SET_INPUT(INPUT_W);
							break;
							case 'E':
								SET_INPUT(INPUT_E);
							break;
							case 'R':
								SET_INPUT(INPUT_R);
							break;
							case 'T':
								SET_INPUT(INPUT_T);
							break;
							case 'Y':
								SET_INPUT(INPUT_Y);
							break;
							case 'U':
								SET_INPUT(INPUT_U);
							break;
							case 'I':
								SET_INPUT(INPUT_I);
							break;
							case 'O':
								SET_INPUT(INPUT_O);
							break;
							case 'P':
								SET_INPUT(INPUT_P);
							break;
							case 'A':
								SET_INPUT(INPUT_A);
							break;
							case 'S':
								SET_INPUT(INPUT_S);
							break;
							case 'D':
								SET_INPUT(INPUT_D);
							break;
							case 'F':
								SET_INPUT(INPUT_F);
							break;
							case 'G':
								SET_INPUT(INPUT_G);
							break;
							case 'H':
								SET_INPUT(INPUT_H);
							break;
							case 'J':
								SET_INPUT(INPUT_J);
							break;
							case 'K':
								SET_INPUT(INPUT_K);
							break;
							case 'L':
								SET_INPUT(INPUT_L);
							break;
							case 'Z':
								SET_INPUT(INPUT_Z);
							break;
							case 'X':
								SET_INPUT(INPUT_X);
							break;
							case 'C':
								SET_INPUT(INPUT_C);
							break;
							case 'V':
								SET_INPUT(INPUT_V);
							break;
							case 'B':
								SET_INPUT(INPUT_B);
							break;
							case 'N':
								SET_INPUT(INPUT_N);
							break;
							case 'M':
								SET_INPUT(INPUT_M);
							break;
							case '1':
								SET_INPUT(INPUT_1);
							break;
							case '2':
								SET_INPUT(INPUT_2);
							break;
							case '3':
								SET_INPUT(INPUT_3);
							break;
							case '4':
								SET_INPUT(INPUT_4);
							break;
							case '5':
								SET_INPUT(INPUT_5);
							break;
							case '6':
								SET_INPUT(INPUT_6);
							break;
							case '7':
								SET_INPUT(INPUT_7);
							break;
							case '8':
								SET_INPUT(INPUT_8);
							break;
							case '9':
								SET_INPUT(INPUT_9);
							break;
							case '0':
								SET_INPUT(INPUT_0);
							break;

							case VK_ESCAPE:
								SET_INPUT(INPUT_ESCAPE);
							break;
							case VK_F1:
								SET_INPUT(INPUT_F1);
							break;
							case VK_F2:
								SET_INPUT(INPUT_F2);
							break;
							case VK_F3:
								SET_INPUT(INPUT_F3);
							break;
							case VK_F4:
								SET_INPUT(INPUT_F4);
							break;
							case VK_F5:
								SET_INPUT(INPUT_F5);
							break;
							case VK_F6:
								SET_INPUT(INPUT_F6);
							break;
							case VK_F7:
								SET_INPUT(INPUT_F7);
							break;
							case VK_F8:
								SET_INPUT(INPUT_F8);
							break;
							case VK_F9:
								SET_INPUT(INPUT_F9);
							break;
							case VK_F10:
								SET_INPUT(INPUT_F10);
							break;
							case VK_F11:
								SET_INPUT(INPUT_F11);
							break;
							case VK_F12:
								SET_INPUT(INPUT_F12);
							break;

							#if DEBUGMODE
							// case VK_F6:
							// 	current_app = (current_app+(apps_count-1))%apps_count;
							// break;
							// case VK_F7:
							// 	current_app = (current_app+(1))%apps_count;
							// break;
							#endif


							case VK_LEFT:
								SET_INPUT(INPUT_LEFT);
							break;
							case VK_RIGHT:
								SET_INPUT(INPUT_RIGHT);
							break;
							case VK_UP:
								SET_INPUT(INPUT_UP);
							break;
							case VK_DOWN:
								SET_INPUT(INPUT_DOWN);
							break;
							case VK_END:
								SET_INPUT(INPUT_END);
							break;
							case VK_HOME:
								SET_INPUT(INPUT_HOME);
							break;
							case VK_INSERT:
								SET_INPUT(INPUT_INSERT);
							break;
							case VK_BACK:
								SET_INPUT(INPUT_BACKSPACE);
							break;
							case VK_RETURN:
								SET_INPUT(INPUT_RETURN);
							break;
							case VK_DELETE:
								SET_INPUT(INPUT_DEL);
							break;
							case VK_PRIOR:
								SET_INPUT(INPUT_PGUP);
							break;
							case VK_NEXT:
								SET_INPUT(INPUT_PGDOWN);
							break;
							case VK_MENU:
								SET_INPUT(INPUT_ALT);
							break;

							default:
							break;

						}

					}
					b32 AltKeyWasDown = ((msg.lParam & (1 << 29)));
					if (!AltKeyWasDown || (vkcode == VK_F4))
					{
						TranslateMessage(&msg);
						DispatchMessage(&msg);
					}
				}break;
				case WM_CHAR:
				{
					u8 char_code = (u8)msg.wParam;
					
					memory.input_chars_buffer[memory.input_chars_buffer_current_size] = char_code;
					memory.input_chars_buffer_current_size++;
				}break;
				case WM_UNICHAR:
				{

				}
				default:
					//TODO: this function is used for when i want to handle text input with WM_CHAR messages
					TranslateMessage(&msg);
					DispatchMessageA(&msg);
					// DefWindowProc(msg.hwnd, msg.message, msg.wParam, msg.lParam);
			}
		}
		//TODO: shortcuts system
		UNTIL(i, INPUT_COUNT)
		{
			// input.keys[i] = holding_inputs.keys[i] + input.keys[i]*holding_inputs.keys[i];
			if(pressed_keys[i])
			{
				input.keys[i] = 1;
			}
			else if(holding_inputs.keys[i]) {
				input.keys[i]++;
			}else{
				if(input.keys[i] > 0)
					input.keys[i] = -1; // just released button
				else
					input.keys[i] = 0;
			}
		}

		// INITIALIZING SAMPLE_T BEFORE PROCESSING THE FIRST TIME 
		if(!sample_t)
		{
			DWORD play_cursor, write_cursor;
			hr = audio.buffer->GetCurrentPosition(&play_cursor, &write_cursor);
			ASSERTHR(hr);
			
			DWORD unwrapped_write_cursor = write_cursor;
			if( unwrapped_write_cursor < play_cursor)
				unwrapped_write_cursor += audio.buffer_size;
				
			DWORD bytes_latency = unwrapped_write_cursor - play_cursor;
			DWORD unwrapped_byte_to_lock = write_cursor + bytes_latency;
			DWORD byte_to_lock = (unwrapped_byte_to_lock) % audio.buffer_size;

			if(unwrapped_byte_to_lock < last_byte_to_lock)
				unwrapped_byte_to_lock += audio.buffer_size;

			sample_t += ((unwrapped_byte_to_lock-last_byte_to_lock) % audio.buffer_size)/audio.bytes_per_sample;
			last_byte_to_lock = byte_to_lock;	
		}

		// APP UPDATE
		{
			SYSTEMTIME st;
			GetSystemTime(&st);
			
			FILETIME filetime;
			SystemTimeToFileTime(&st, &filetime);

			ULARGE_INTEGER large_ns;
			large_ns.LowPart = filetime.dwLowDateTime;
			large_ns.HighPart = filetime.dwHighDateTime;

			memory.win_time_ns = large_ns.QuadPart;

			// memory.rng.last_seed = (u32)((((f64)st.wMilliseconds/999) + ((f64)st.wSecond/60000)) * 0xffffffff);
		}

		if(apps[current_app].update)
		{
			apps[current_app].update(&memory, app_data,{playback_array, sample_t}, client_size);
		}


		// SOUND RENDERING

		
		{
			DWORD play_cursor,  write_cursor; 
			hr = audio.buffer->GetCurrentPosition(&play_cursor, &write_cursor);
			ASSERTHR(hr);

			DWORD unwrapped_write_cursor = write_cursor;
			if( unwrapped_write_cursor < play_cursor)
				unwrapped_write_cursor += audio.buffer_size;

			DWORD bytes_latency = unwrapped_write_cursor - play_cursor;
			DWORD unwrapped_byte_to_lock = write_cursor + bytes_latency;
			DWORD byte_to_lock = (unwrapped_byte_to_lock) % audio.buffer_size;

			if(unwrapped_byte_to_lock < last_byte_to_lock)
				unwrapped_byte_to_lock += audio.buffer_size;


			sample_t += ((unwrapped_byte_to_lock-last_byte_to_lock) % audio.buffer_size)/audio.bytes_per_sample;
			// TODO: THIS SHOULD BE DONE BEFORE UPDATING SAMPLE_T BUT IT CAUSES A LOT OF SKIPPING IF I DO THAT
			// if i do it like this it will skip the first samples cuz sample_t will be already far from 0
			u32 last_sample_t = sample_t; 
			last_byte_to_lock = byte_to_lock;

			DWORD bytes_to_write = audio.buffer_size - (bytes_latency);

			{
				u32 samples_to_write = bytes_to_write / audio.bytes_per_sample;
				s16* audio_processing_buffer = (s16*)arena_push_size(temp_arena, bytes_to_write);
				u32 max_audio_playback_count = ARRAYLEN(playback_array);
				UNTIL(i, max_audio_playback_count)
				{
					if(!playback_array[i].initial_sample_t ) continue;

					Audio_playback* playback = &playback_array[i];
        			s16* sample_out = audio_processing_buffer;
					
					u32 playback_sample_t = last_sample_t-playback->initial_sample_t;

					Sound_sample* sound_samples = &sounds_list[playback->sound_uid];

					if(playback_sample_t >= sound_samples->samples_count)
					{
						if(playback->loop)
							playback->initial_sample_t = sample_t;
						else{
							*playback = {0};
							continue;
						}
					}
					u32 samples_per_channel = 2/sound_samples->channels;

					u32 in_i = 0;
					u32 out_i = 0;

        			while(out_i  < samples_to_write 
					&& ((in_i + playback_sample_t) < sound_samples->samples_count)
					)
					{
						UNTIL(channel, samples_per_channel)
						{
							sample_out[out_i] += sound_samples->samples[in_i+playback_sample_t];
							out_i++;
						}
						in_i++;
					}
					ASSERT(true);
				}


				void* region1 = 0;
				DWORD region1_size = 0;
				void* region2 = 0;
				DWORD region2_size = 0;

				{// LOCKING SOUND BUFFER
					hr = audio.buffer->Lock(
						byte_to_lock,
						bytes_to_write,
						&region1,
						&region1_size,
						&region2,
						&region2_size,
						0
					); 

					ASSERTHR(hr);
					
					set_mem(region1, region1_size, 0);
					set_mem(region2, region2_size, 0);

					u32 processing_buffer_i = 0;

					s16* sample_out = (s16*)region1;
					DWORD region1_sample_count = region1_size/audio.bytes_per_sample;
					for(u32 i=0; i<region1_sample_count; i++){
						*sample_out++ = audio_processing_buffer[processing_buffer_i++];
					}

					sample_out = (s16*)region2;
					DWORD region2_sample_count = region2_size/audio.bytes_per_sample;
					for(u32 i=0; i<region2_sample_count; i++){
						*sample_out++ = audio_processing_buffer[processing_buffer_i++];
					}


					audio.buffer->Unlock(region1, region1_size, region2,region2_size);
				}

			}
		}



		// APP RENDER REQUESTS/PREPARATION


		LIST(Renderer_request, render_list) = {0};
		if(apps[current_app].render)
		{
			apps[current_app].render(&memory, app_data, render_list, client_size);
		}

		// ACTUALLY RENDER

		if(screen_render_target->target_view)
		{		
			{// setting default rtv and ds
				Depth_stencil* depth_stencil; LIST_GET(depth_stencils_list, 0, depth_stencil);
				// state can be null to set the default depth_stencil in 
				dx->context->OMSetDepthStencilState(depth_stencil->state, 0);
				
				Dx11_render_target_view** rtviews_to_bind = ARENA_PUSH_STRUCT(temp_arena, Dx11_render_target_view*);
				
				rtviews_to_bind[0] = screen_render_target->target_view;

				dx->context->OMSetRenderTargets(1, rtviews_to_bind, depth_stencil->view); 
			}

			dx->context->ClearRenderTargetView(render_targets_list[0]->target_view, (float*)&memory.bg_color);
			
			u32 counter = 0;
			FOREACH(Depth_stencil, current_ds, depth_stencils_list){
				counter++;
				if(current_ds->view)
				{
					dx->context->ClearDepthStencilView(
						current_ds->view,
						D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
						1.0f, 0);
				}
			}
			dx11_bind_sampler(dx, &dx->sampler);


			// RENDER HERE

			// 3D RENDER PIPELINE
			
			u32 count = 0;

			FOREACH(Renderer_request, request, render_list)
			{
				ASSERT(request->type_flags); //assert at least one flag is set
				count++;


				if(request->type_flags & REQUEST_FLAG_MODIFY_DYNAMIC_MESH)
				{
					Dx_mesh* modify_mesh;
					LIST_GET(meshes_list, request->modified_mesh.mesh_uid, modify_mesh);
					dx11_modify_resource(dx, modify_mesh->vertex_buffer, request->modified_mesh.vertices,
						request->modified_mesh.vertex_count * modify_mesh->vertex_size
					);

					if(request->modified_mesh.indices)
					{
						dx11_modify_resource(dx, modify_mesh->index_buffer, request->modified_mesh.indices,
							request->modified_mesh.indices_count * sizeof(u16));
					}
					ASSERT(true);
				}// TODO: UNIFY THIS 2 FLAGS INTO "REQUEST_FLAG_MODIFY_DYNAMIC_RESOURCE"
				if(request->type_flags & REQUEST_FLAG_MODIFY_DYNAMIC_TEXTURE) 
				{
					ID3D11Resource* modify_texture;
					Dx11_texture_view** texture_view;
					LIST_GET(textures_list, request->modifiable_texture.source_tex_uid, texture_view);
					(*texture_view)->GetResource(&modify_texture);
					// dx11_modify_resource(dx, modify_texture, request->modifiable_texture.new_data, request->modifiable_texture.size);
					D3D11_BOX texbox = {
						request->modifiable_texture.box.left,
						request->modifiable_texture.box.top,
						request->modifiable_texture.box.front, 
						request->modifiable_texture.box.right, 
						request->modifiable_texture.box.bottom,
						request->modifiable_texture.box.back
					};

					dx->context->UpdateSubresource(modify_texture, 0,//ignore this 0 
						&texbox, 
						request->modifiable_texture.new_data, 
						request->modifiable_texture.source_row_pitch,
						request->modifiable_texture.source_depth_pitch
						);
					modify_texture->Release();
				}
				if(request->type_flags & REQUEST_FLAG_SET_SHADER_RESOURCE_FROM_TEXTURE)
				{
					Dx11_texture_view** texture_view; 
					if(request->set_shader_resource_from_texture.tex_uid != NULL_INDEX16){
						LIST_GET(textures_list, request->set_shader_resource_from_texture.tex_uid, texture_view);
					}else{
						texture_view = ARENA_PUSH_STRUCT(temp_arena, Dx11_texture_view*);
					}
					dx->context->PSSetShaderResources(request->set_shader_resource_from_texture.target_index, 1, texture_view);
				}
				if(request->type_flags & REQUEST_FLAG_RESIZE_DEPTH_STENCIL_VIEW)
				{
					Depth_stencil* resize_ds;
					LIST_GET(depth_stencils_list, request->resize_depth_stencil_view_uid, resize_ds);
					
					if(resize_ds->view)
					{
						resize_ds->view->Release();
						resize_ds->view = 0;
					}

					dx11_create_depth_stencil_view(dx, &resize_ds->view, request->new_viewport_size.x, request->new_viewport_size.y);
					dx->context->ClearDepthStencilView(
						resize_ds->view,
						D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
						1.0f, 0
					);
				}
				if(request->type_flags & REQUEST_FLAG_SET_VS)
				{
					Vertex_shader* vertex_shader; LIST_GET(vertex_shaders_list, request->vshader_uid, vertex_shader);

					dx->context->VSSetShader(vertex_shader->shader, 0, 0);
					dx->context->IASetInputLayout(vertex_shader->input_layout);
				}
				if(request->type_flags & REQUEST_FLAG_SET_PS)
				{
					Pixel_shader* pixel_shader; LIST_GET(pixel_shaders_list, request->pshader_uid, pixel_shader);
					dx->context->PSSetShader(pixel_shader->shader, 0, 0);
				}
				if(request->type_flags & REQUEST_FLAG_SET_BLEND_STATE)
				{
					Dx11_blend_state** blend_state; LIST_GET(blend_states_list, request->blend_state_uid, blend_state);
					
					// float blend_factor [4] = {0.0f,0.0f,0.0f,1.0f};
					dx->context->OMSetBlendState(*blend_state, 0, ~0U);   
				}
				if(request->type_flags & REQUEST_FLAG_RESIZE_TARGET_VIEW)
				{	
					// the screen rtv cannot be resized this way, and is already being done JUST when the client size changes
					ASSERT(request->resize_rtv_uid != 0); 

					Render_target* render_target;
					LIST_GET(render_targets_list, request->resize_rtv_uid, render_target);
					Dx11_texture_view** texture_view = render_target->texture_view;
					if(render_target->target_view)
					{
						render_target->texture->Release();
						render_target->texture = 0;
						render_target->target_view->Release();
						render_target->target_view = 0;
						(*texture_view)->Release();
						(*texture_view) = 0;
					}
					D3D11_TEXTURE2D_DESC td = {0};
					td.Width = request->new_viewport_size.x;       // Width of the texture in pixels
					td.Height = request->new_viewport_size.y;     // Height of the texture in pixels
					td.MipLevels = 1;             // Number of mip-map levels
					td.ArraySize = 1;             // Number of textures in the array
					td.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // Format of the texture (e.g., RGBA)
					td.SampleDesc.Count = 1;      // Number of multisamples per pixel (usually 1 for a render target)
					td.SampleDesc.Quality = 0;    // Quality level of multisampling
					td.Usage = D3D11_USAGE_DEFAULT; // How the texture will be used (D3D11_USAGE_DEFAULT is common for render targets)
					td.BindFlags = D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_RENDER_TARGET; // Flags specifying how the texture will be bound (as a render target in this case)
					td.CPUAccessFlags = 0;        // CPU access flags (usually 0 for GPU-only textures)
					td.MiscFlags = 0;             // Miscellaneous flags (usually 0)

					dx->device->CreateTexture2D(&td, 0, &render_target->texture);
					dx->device->CreateRenderTargetView(
						render_target->texture, 
						0, 
						&render_target->target_view);	
					
					// TEXTURE VIEW

					D3D11_SHADER_RESOURCE_VIEW_DESC srvd = {};

					// Set the format of the texture (match this with the format of your render target texture)
					srvd.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // Replace with your format if different

					// Specify the type of view (typically Texture2D)
					srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;

					// Configure the resource range
					srvd.Texture2D.MostDetailedMip = 0; // Start from the first mip level
					srvd.Texture2D.MipLevels = 1;      // Use all available mip levels

					// dx->device->CreateShaderResourceView(dx->pre_processing_render_target_texture, &srvd, &shader_resource_view);
					
					dx->device->CreateShaderResourceView(render_target->texture, 
						&srvd, texture_view);
				}
				if(request->type_flags & REQUEST_FLAG_SET_RENDER_TARGET_AND_DEPTH_STENCIL)
				{
					Depth_stencil* depth_stencil; LIST_GET(depth_stencils_list, request->set_depth_stencil_uid, depth_stencil);
					// state can be null to set the default depth_stencil in 
					dx->context->OMSetDepthStencilState(depth_stencil->state, 0);

					ASSERT(request->set_rtv_count);
					ASSERT(request->set_rtv_uids);
					
					Dx11_render_target_view** rtviews_to_bind = ARENA_PUSH_STRUCTS(temp_arena, Dx11_render_target_view*, request->set_rtv_count);

					UNTIL(i, request->set_rtv_count)
					{
						Render_target* current_render_target;
						LIST_GET(render_targets_list, request->set_rtv_uids[i], current_render_target);
						rtviews_to_bind[i] = current_render_target->target_view;
					}
					dx->context->OMSetRenderTargets(request->set_rtv_count, rtviews_to_bind, depth_stencil->view); 
				}
				if(request->type_flags & REQUEST_FLAG_MODIFY_RENDERER_VARIABLE)
				{
					dx11_modify_resource(dx, 
						renderer_variables[request->renderer_variable.register_index].buffer, 
						request->renderer_variable.new_data, 
						renderer_variables[request->renderer_variable.register_index].size
						);
				}
				if(request->type_flags & REQUEST_FLAG_CHANGE_VIEWPORT_SIZE)
				{
					dx11_set_viewport(dx, 0, 0, request->new_viewport_size.x, request->new_viewport_size.y);
				}
				if(request->type_flags & REQUEST_FLAG_SET_SHADER_RESOURCE_FROM_RENDER_TARGET)
				{
					Render_target* render_target_source;
					LIST_GET(render_targets_list, request->set_shader_resource_from_rtv.rtv_uid, render_target_source);

					dx->context->PSSetShaderResources(request->set_shader_resource_from_rtv.target_index,1, render_target_source->texture_view);
					ASSERT(true);	
				}
				if(request->type_flags & REQUEST_FLAG_SET_RASTERIZER_STATE)
				{
					dx->rasterizer_state->Release();
					global_rasterizer_desc.FillMode = fill_modes_list[request->rasterizer_state.fill_mode];
					global_rasterizer_desc.CullMode = cull_modes_list[request->rasterizer_state.cull_mode];
					hr = dx->device->CreateRasterizerState(&global_rasterizer_desc, &dx->rasterizer_state);
					
					dx11_bind_rasterizer_state(dx, dx->rasterizer_state);

					ASSERTHR(hr);
				}
				if(request->type_flags & REQUEST_FLAG_SET_SAMPLER)
				{
					ASSERT(false);
				}
				if(request->type_flags & REQUEST_FLAG_CLEAR_RTV)
				{
					Render_target* rtv_to_clear;
					LIST_GET(render_targets_list, request->clear_rtv.uid, rtv_to_clear);
					dx->context->ClearRenderTargetView(rtv_to_clear->target_view, (float*)&request->clear_rtv.color);
					ASSERT(true);
				}


				// RENDERING CALLS


				if(request->type_flags & REQUEST_FLAG_RENDER_OBJECT)
				{	
					Object3d* object = &request->object3d;
					ASSERT(object->color.a); // FORGOR TO SET THE COLOR
					ASSERT(object->scale.x && object->scale.y && object->scale.z); // FORGOR TO SET THE SCALE
					Object_buffer_data object_data = {0};
					
					object_data.color = object->color;
					object_data.transform = 
						matrix_scale(object->scale)*
						matrix_from_quaternion(object->rotation) * 
						matrix_translation(object->pos)
					;

					if(object->texinfo_uid != NULL_INDEX16)
					{
						Tex_info* texinfo; LIST_GET(memory.tex_infos, object->texinfo_uid, texinfo);
						object_data.texrect = {
							texinfo->texrect.x + (texinfo->texrect.w*(!!request->flip_h)),
							texinfo->texrect.y,
							texinfo->texrect.w * (1.0f - (2*!!request->flip_h)),
							texinfo->texrect.h
						};

						Dx11_texture_view** texture_view; LIST_GET(textures_list, texinfo->texture_uid, texture_view);
						
						dx->context->PSSetShaderResources(0,1, texture_view);
					}
					else
					{
						object_data.texrect = {0,0,1,1};
						ASSERT(true);
					}
					dx11_modify_resource(dx, object_buffer->buffer, &object_data, sizeof(object_data));
					

					Dx_mesh* object_mesh; LIST_GET(meshes_list, object->mesh_uid, object_mesh);
					
					dx11_draw_mesh(dx, object_mesh);
					ASSERT(true);
				}				
				else if(request->type_flags & REQUEST_FLAG_RENDER_INSTANCES)
				{
					ASSERT(request->instancing_data.instances_count);
					Tex_info* texinfo; 
					LIST_GET(memory.tex_infos, request->instancing_data.texinfo_uid, texinfo);
					// THIS IS THE TEXTURE THAT ALL THE INSTANCES WILL USE
					Dx11_texture_view** texture_view;
					LIST_GET(textures_list, texinfo->texture_uid, texture_view);
					//TODO: add texrect to the instance data
					dx->context->PSSetShaderResources(0, 1, texture_view);
					
					// THIS IS THE MESH THAT ALL THE INSTANCES WILL USE
					Dx_mesh* vertices_mesh; LIST_GET(meshes_list, request->instancing_data.mesh_uid, vertices_mesh);

					// THIS IS THE PER INSTANCE DATA
					Dx_mesh* instances_data_mesh; LIST_GET(meshes_list, request->instancing_data.dynamic_instances_mesh, instances_data_mesh);
					dx11_modify_resource(dx, instances_data_mesh->vertex_buffer, 
						request->instancing_data.instances, 
						sizeof(Instance_data)*request->instancing_data.instances_count
					);
					
					u32 offsets = 0;

					u32 instances_stride = sizeof(Instance_data);
					dx->context->IASetVertexBuffers(1, 1, &instances_data_mesh->vertex_buffer, &instances_stride, &offsets);

					u32 strides = sizeof(Vertex); 
					dx->context->IASetVertexBuffers(0,1, &vertices_mesh->vertex_buffer,&strides,&offsets);

					dx->context->IASetPrimitiveTopology( vertices_mesh->topology );
					dx->context->IASetIndexBuffer(vertices_mesh->index_buffer, DXGI_FORMAT_R16_UINT, 0);

					dx->context->DrawIndexedInstanced(vertices_mesh->indices_count, request->instancing_data.instances_count, 0, 0, 0);
				}
				// else if(request->type_flags & REQUEST_FLAG_POSTPROCESSING) // POST PROCESSING EFFECTS
				// {					
				// 	// dx->context->OMSetRenderTargets(1, &dx->render_target_views_list[RTV_SCREEN], 0); 
				// 	// dx11_bind_render_target_view(dx, &dx->render_[0], aux_depth_stencil->view);

				// 	dx->context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

				// 	Dx_mesh* object_mesh; LIST_GET(meshes_list, request->mesh_uid, object_mesh);
				// 	dx11_bind_vertex_buffer(dx, object_mesh->vertex_buffer, object_mesh->vertex_size);

				// 	dx->context->Draw(4, 0);
				// }
			}

			// PRESENT RENDERING
			hr = dx->swap_chain->Present(1,0);
			ASSERTHR(hr);
		}


#if PRINT_FRAMERATE 
		// this is just for logging the framerate later
		LARGE_INTEGER this_frame_counter = last_counter;
#endif
		{//FRAME CAPPING
			#if 1
			LARGE_INTEGER current_wall_clock;
			QueryPerformanceCounter(&current_wall_clock);

			f32 frame_seconds_elapsed = (f32)(current_wall_clock.QuadPart - last_counter.QuadPart) / (f32)performance_counter_frequency;

			DWORD sleep_ms = 0;

			
			f32 target_seconds_per_frame = 1.0f / memory.update_hz;

			if(frame_seconds_elapsed < target_seconds_per_frame)
			{
				if(sleep_is_granular)
				{
					sleep_ms = (DWORD)(1000.0f * (target_seconds_per_frame - frame_seconds_elapsed));
					if(sleep_ms > 0){
						SleepEx(sleep_ms, false);
					}
				}
				while(frame_seconds_elapsed < target_seconds_per_frame)
				{
					QueryPerformanceCounter(&current_wall_clock); 
					frame_seconds_elapsed = (f32)(current_wall_clock.QuadPart - last_counter.QuadPart) / (f32)performance_counter_frequency;
				}
			}else{
				//TODO: missed framerate
				// ASSERT(false);
			}
			
			QueryPerformanceCounter(&last_counter);

			#endif

			memory.old_time_s = memory.time_s;
			memory.time_s += frame_seconds_elapsed;
			
			// TODO: in the tests i made both were pretty close together there was no significant difference
			// but because this stores the initial time and calculates the difference between current time and initial
			// MAYBE this is more precise than adding the delta time, but i know shit
			#if 0
				u32 frame_ms_elapsed = (u32)(frame_seconds_elapsed*1000);
				memory.pseudo_time_ms += frame_ms_elapsed; 
				{
					FILETIME current_time;
					GetSystemTimeAsFileTime(&current_time);
					u64 current_100ns = (LONGLONG)current_time.dwLowDateTime + ((LONGLONG)(current_time.dwHighDateTime) << 32LL);
					u64 transcurred_100ns = current_100ns - initial_100ns;

					u64 transcurred_ms = transcurred_100ns / 10000;
					memory.time_s = (f32)transcurred_ms / 1000;
					ASSERT(true);
				}
			#endif
		}
#if PRINT_FRAMERATE
		{// print out framerate
			LARGE_INTEGER current_wall_clock;
			QueryPerformanceCounter(&current_wall_clock);

			f32 ms_per_frame = 1000.0f * (f32)(current_wall_clock.QuadPart - this_frame_counter.QuadPart) / (f32)performance_counter_frequency;
			s64 end_cycle_count = __rdtsc(); // clock cycles count


			u64 cycles_elapsed = end_cycle_count - last_cycles_count;
			f32 FPS = (1.0f / (ms_per_frame/1000.0f));
			s32 MegaCyclesPF = (s32)((f64)cycles_elapsed / (f64)(1000*1000));

			char text_buffer[256];
			wsprintfA(text_buffer, "%dms/f| %d f/s|  %d Mhz/f \n", (s32)ms_per_frame, (s32)FPS, MegaCyclesPF);
			OutputDebugStringA(text_buffer);   
			last_cycles_count = __rdtsc();
		}
#endif
	}
	if(apps[0].close_app)
	{
		apps[0].close_app(&memory, app_data);
	}

	//TODO: this is dumb but i don't want dumb messages each time i exit
	// not even this dumb releases will free me from the dumb messages
	// no, they actually did... fuck
	
	dx->device->Release();
	dx->context->Release();
	dx->swap_chain->Release();
	dx->rasterizer_state->Release();
	dx->sampler->Release();

	UNTIL(cb_index, ARRAYCOUNT(renderer_variables))
	{
		if(renderer_variables[cb_index].buffer)
		{
			renderer_variables[cb_index].buffer->Release();
		}
	}
	
	FOREACH(Dx_mesh, current_mesh, meshes_list)
	{
		current_mesh->vertex_buffer->Release();
		if(current_mesh->index_buffer)
			current_mesh->index_buffer->Release();
	}


	FOREACH(Render_target, current_rt, render_targets_list)
	{
		current_rt->target_view->Release();
		//i don't know if i should release the texture or the texture view
		if(current_rt->texture_view)
		{
			if(*current_rt->texture_view)
			{
				(*current_rt->texture_view)->Release();
				(*current_rt->texture_view) = 0;
			}
		}
		if(current_rt->texture)
		{
			current_rt->texture->Release();
			current_rt->texture = 0;
		}
	}
	FOREACH(Dx11_texture_view*, current_tex, textures_list)
	{
		if(*current_tex)
		{
			(*current_tex)->Release();
		}
	}

	FOREACH(Vertex_shader, current_vs, vertex_shaders_list){
		current_vs->shader->Release();
		current_vs->input_layout->Release();
		String temp_filename = concat_strings(current_vs->filename, string(".temp"), temp_arena);
		win_delete_file(temp_filename);
	}

	FOREACH(Pixel_shader, current_ps, pixel_shaders_list){
		(current_ps->shader)->Release();
		String temp_filename = concat_strings(current_ps->filename, string(".temp"), temp_arena);
		win_delete_file(temp_filename);
	}

	FOREACH(Dx11_blend_state*, current_blend, blend_states_list){
		(*current_blend)->Release();
	}

	FOREACH(Depth_stencil, current_stencil, depth_stencils_list){
		if(current_stencil->view)
			current_stencil->view->Release();
		if(current_stencil->state)
			current_stencil->state->Release();
	}

	

#if DEBUGMODE

	// Call the ReportLiveObjects() function to report live DXGI objects
	// hr = p_debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_IGNORE_INTERNAL);
	hr = p_debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
	ASSERTHR(hr);
	// Release the DXGI debug interface
	p_debug->Release();

#endif
	return 0;
}