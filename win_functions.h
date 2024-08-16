/*
	VirtualAlloc(0, global_temp_arena.size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);



   WIN32_FIND_DATA find_data;
	HWNDLE file_handle = FindFirstFileA(filename_to_search_for, &find_data);

	do
	{
		// scan the directory for all the possible files 
		// that match the filepath set when calling FindFirstFileA
		// if the filepath ends with /* it will scan all the files in the directory

		
		if(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			// file_handle is a directory
		}else{
			// file_handle is not a directory
		}
	}
	while(FindNextFileA(file_handle, &find_data) != 0);

   FindClose(file_handle);



	if(!PathFileExistsA(filepath))
   {
		ASSERT(!"path does not exists");
	}
	
*/

internal String
win_get_current_directory(Memory_arena* arena)
{
	String result;
	char temp_buffer [MAX_PATH] = {0}; 
	
	result.length = GetCurrentDirectoryA(MAX_PATH, temp_buffer);
	ASSERT(result.length);
	
	// converting \\ to /
	UNTIL(char_i, result.length)
	{
		if(temp_buffer[char_i] == '\\') temp_buffer[char_i] = '/';
	}

	temp_buffer[result.length] = '/';
	result.length++;

	result.text = (char*)arena_push_data(arena, temp_buffer, result.length);

	return result;
}
internal bool
win_list_all_files(String filename_to_search_for, LIST(Filename, filenames_list), Memory_arena* arena)
{
	ASSERT(filename_to_search_for.length < MAX_PATH);
	char temp_buffer [MAX_PATH] = {0};
	copy_mem(filename_to_search_for.text, temp_buffer, filename_to_search_for.length);
	temp_buffer[filename_to_search_for.length] = '*';

   WIN32_FIND_DATA find_data;
	HANDLE file_handle = FindFirstFileA(temp_buffer, &find_data);
	if(file_handle == INVALID_HANDLE_VALUE)
	{
		DWORD error = GetLastError();
		ASSERT(
			error == ERROR_FILE_NOT_FOUND 
		|| error == ERROR_PATH_NOT_FOUND
		|| error == ERROR_INVALID_NAME
		);
		return false;
	}

	DWORD error = GetLastError();
	while(true)
	{
		Filename* current_filename;
		PUSH_BACK(filenames_list, arena, current_filename);

		for(u32 char_i = 0; char_i < MAX_PATH && find_data.cFileName[char_i]; char_i++)
		{
			current_filename->name.length++;
		}

		current_filename->name.text = (char*)arena_push_size(arena, 0);
		arena_push_data(arena, find_data.cFileName, current_filename->name.length);
		
		if(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			current_filename->is_folder = true;
		}

		if(!FindNextFileA(file_handle, &find_data))
		{
			error = GetLastError();
			ASSERT(error == ERROR_NO_MORE_FILES);
			break;
		}
	}

   FindClose(file_handle);

	return true;
}


internal bool
win_file_exists(char* filename)
{
  DWORD file_attributes = GetFileAttributes(filename);

  return (file_attributes != INVALID_FILE_ATTRIBUTES && 
         !(file_attributes & FILE_ATTRIBUTE_DIRECTORY));
}

global_variable File_data packed_data = {0};

internal File_data 
win_read_file(String filename, Memory_arena* arena)
{
	File_data result = {0};
	char temp_buffer [MAX_PATH]={0};
	copy_mem(filename.text, temp_buffer, filename.length);

	HANDLE file_handle = CreateFileA(temp_buffer, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if(file_handle == INVALID_HANDLE_VALUE)
	{
		DWORD error = GetLastError();
		ASSERT(!error);
	}
	LARGE_INTEGER file_size;
	if( GetFileSizeEx(file_handle, &file_size) )
	{
		u32 file_size_32 = (u32)file_size.QuadPart;
		result.data = arena_push_size(arena, file_size_32);
		if(result.data)
		{
			DWORD bytes_read;
			if(ReadFile(file_handle, result.data, file_size_32, &bytes_read, 0))
				if((file_size_32 == bytes_read))
				result.size = file_size_32;

			
			else
				arena_pop_back_size(arena, file_size_32);
		}
	}
	ASSERT(result.size);
	CloseHandle(file_handle);

	return result;
}

internal bool
win_write_file(String filename, void* data, u32 file_size)
{
	b32 result = false;

	UNTIL(char_i, filename.length)
	{
		switch(filename.text[char_i])
		{
			// INVALID CHARACTERS FOR FILENAME
			// case ':':
			// case '/':
			// case '\\':
			case '>':
			case '<':
			case '\"':
			case '|':
			case '?':
			case '*':
			{
				ASSERT(false);
				return false;
			}break;

			default: break;
		}
	}

	HANDLE file_handle = CreateFileA(filename.text, GENERIC_WRITE, FILE_SHARE_WRITE, 0, CREATE_ALWAYS, 0,0);
	if(file_handle != INVALID_HANDLE_VALUE)
	{
		DWORD bytes_written;
		if(WriteFile(file_handle, data, file_size, &bytes_written, 0))
		{
			result = (bytes_written == file_size);
		}else{
			DWORD error = GetLastError();
			ASSERT(!error);
		}

		CloseHandle(file_handle);
	}
	else
	{
		DWORD error = GetLastError();
		ASSERT(!error);
	}

	return result;
}

internal bool
win_delete_file(String filename)
{
	char filename_buffer [MAX_PATH] = {0};
	copy_mem(filename.text, filename_buffer, filename.length);

	return DeleteFileA(filename_buffer);
}

internal bool
win_copy_file(String filename, String new_filename)
{
	char filename_buffer [MAX_PATH] = {0};
	copy_mem(filename.text, filename_buffer, filename.length);

	char new_filename_buffer [MAX_PATH] = {0};
	copy_mem(new_filename.text, new_filename_buffer, new_filename.length);

	b32 overwrite_if_exists = 0;
	return CopyFile(filename_buffer, new_filename_buffer, overwrite_if_exists);
}

internal Int2
win_get_client_sizes(HWND window)
{
	Int2 result = {0};
	RECT rect;
	GetClientRect(window, &rect);
	result.x = rect.right - rect.left;
	result.y = rect.bottom - rect.top;
	
	return result;
}

internal FILETIME
win_get_last_write_time(char* filename)
{
	FILETIME result = {0};

	WIN32_FILE_ATTRIBUTE_DATA file_data;
	if(GetFileAttributesExA(filename, GetFileExInfoStandard, (LPVOID)&file_data))
	{
		result = file_data.ftLastWriteTime;
	}
	else
	{
		ASSERT(false);
	}

	return result;
}

// TIME FUNCTIONS

#if OLD_TIME_FUNCTIONS
internal Win_filetime 
win_systemtime_to_filetime(Win_systemtime systemtime)
{
	Win_filetime result;
	FILETIME temp_result;
	SYSTEMTIME* temp_systemtime = (SYSTEMTIME*)&systemtime;
	SystemTimeToFileTime(temp_systemtime, &temp_result);
	result.low_part = temp_result.dwLowDateTime;
	result.high_part = temp_result.dwHighDateTime;

	return result;
}

internal Win_systemtime
win_filetime_to_systemtime(Win_filetime filetime)
{
	Win_systemtime result = {0};
	SYSTEMTIME temp_result;
	
	FILETIME* temp_filetime = (FILETIME*)&filetime;
	FileTimeToSystemTime(temp_filetime, &temp_result);

	result.year = temp_result.wYear;
	result.month = temp_result.wMonth;
	result.weekday = temp_result.wDayOfWeek;
	result.monthday = temp_result.wDay;
	result.hour = temp_result.wHour;
	result.min = temp_result.wMinute;
	result.sec = temp_result.wSecond;
	result.ms = temp_result.wMilliseconds;

	return result;
}
#endif

internal Datetime
win_systemtime_to_date(SYSTEMTIME* win_systemtime)
{
	Datetime result;
	result.year = win_systemtime->wYear;
	result.month = win_systemtime->wMonth;
	result.weekday = win_systemtime->wDayOfWeek;
	result.monthday = win_systemtime->wDay;
	result.hour = win_systemtime->wHour;
	result.min = win_systemtime->wMinute;
	result.sec = win_systemtime->wSecond;
	result.ms = win_systemtime->wMilliseconds;

	return result;
}
internal SYSTEMTIME
win_date_to_systemtime(Datetime* date)
{
	SYSTEMTIME result;
	result.wYear = date->year;
	result.wMonth = date->month;
	result.wDayOfWeek = date->weekday;
	result.wDay = date->monthday;
	result.wHour = date->hour;
	result.wMinute = date->min;
	result.wSecond = date->sec;
	result.wMilliseconds = date->ms;

	return result;
}

internal Datetime
win_get_current_date()
{
	SYSTEMTIME systemtime;
	GetLocalTime(&systemtime);
	return win_systemtime_to_date(&systemtime);
}

internal Datetime
win_offset_date_by_days(Datetime* date, s32 days)
{
	if(days)
	{
		//                       		  h   m   s   ms    100ns
		s64 days_in_100ns = ((s64)days)*24L*60L*60L*1000L*10000L;

		SYSTEMTIME date_systemtime = win_date_to_systemtime(date);

		FILETIME date_filetime;
		SystemTimeToFileTime(&date_systemtime, &date_filetime);

		ULARGE_INTEGER date_ularge;
		date_ularge.LowPart = date_filetime.dwLowDateTime;
		date_ularge.HighPart = date_filetime.dwHighDateTime;

		ULARGE_INTEGER ularge_result;
		ularge_result.QuadPart = date_ularge.QuadPart + days_in_100ns;
		// ASSERT( (-((s64)date_ularge.QuadPart)) < days_in_100ns);

		FILETIME filetime_result;
		filetime_result.dwHighDateTime = ularge_result.HighPart;
		filetime_result.dwLowDateTime = ularge_result.LowPart;

		SYSTEMTIME systemtime_result;
		FileTimeToSystemTime(&filetime_result, &systemtime_result);

		Datetime result = win_systemtime_to_date(&systemtime_result);

		return result;	
	}
	else
	{
		return *date;
	}
}

// if this fails return value is NULL
internal HMODULE
win_load_game_dll(char* dll_name, FILETIME* out_dll_last_write_time)
{

	WIN32_FILE_ATTRIBUTE_DATA ignore;
	// if i am not in the middle of compilation
	if(!GetFileAttributesEx("lock.tmp", GetFileExInfoStandard, &ignore))
	{
		*out_dll_last_write_time = win_get_last_write_time(dll_name);
	
		char temp_dll_name [MAX_PATH] = {0};
		concat_char_strings(dll_name, "_temp.dll", temp_dll_name);
		
		#if DEBUGMODE	
			CopyFile(dll_name, temp_dll_name, FALSE);
			HMODULE result = LoadLibraryA(temp_dll_name);
		#else
			HMODULE result = LoadLibraryA(dll_name);

		#endif


		ASSERT(result);
		return result;
	}
	else
	{
		return 0;
	}
}


#define ASSERMSG(msg) case msg:ASSERT(false); break;
LRESULT CALLBACK
win_main_window_proc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;

	switch(message)
	{
		case WM_DESTROY:
		case WM_QUIT:
		case WM_CLOSE:
		{
			b32* close_app = (b32*)GetPropA(window, "close_app");
			*close_app = true;
		}
		break;
		case WM_SIZING:
		{
			RECT* result_rect = (RECT*)lParam;
			Int2 new_size = {MAX(result_rect->right - result_rect->left, 200), MAX(result_rect->bottom - result_rect->top, 200)};

			b32* enforce_aspect_ratio = (b32*)GetPropA(window,"enforce_aspect_ratio");

			if(*enforce_aspect_ratio)
			{

				Int2* client_size = (Int2*)GetPropA(window, "client_size");

				RECT winrect = {0,0,client_size->x, client_size->y};
				AdjustWindowRectEx(&winrect, WS_OVERLAPPEDWINDOW,0,0);
				Int2 win_size = {winrect.right-winrect.left, winrect.bottom-winrect.top};
				GetWindowRect(window, &winrect);
				Int2 winclient_diff = win_size - (*client_size);
				
				s32 new_height = MAX(90,new_size.y - winclient_diff.y);
				s32 new_width = MAX(160,new_size.x - winclient_diff.x);


				s32 new_bottom = 0;
				switch (wParam){
					case WMSZ_TOP:
						result_rect->top = MIN(result_rect->top,winrect.bottom - (winclient_diff.y+new_height));
					case WMSZ_BOTTOMRIGHT:
					case WMSZ_BOTTOM:
						new_width = (s32)(0.5f+(16*new_height/9));
						new_bottom =  result_rect->top + (new_height+winclient_diff.y);
					break;
					
					case WMSZ_LEFT:
					case WMSZ_TOPLEFT:
					case WMSZ_BOTTOMLEFT:
						result_rect->left = MIN(result_rect->left,winrect.right - (winclient_diff.x+new_width));
					case WMSZ_RIGHT:
						new_height = (s32)(0.5f+(9*new_width/16));
						new_bottom =  result_rect->top + (new_height+winclient_diff.y);
					break;
					case WMSZ_TOPRIGHT:
						
						new_bottom =  winrect.bottom;
						new_height = (s32)(0.5f+(9*new_width/16));
						result_rect->top = winrect.bottom - (winclient_diff.y+new_height);
					break;
					default:
						new_height = MAX(new_height,(s32)(0.5f+(9*new_width/16)));
						new_width = MAX(new_width, (s32)(0.5f+(16*new_height/9)));
						new_bottom =  result_rect->top + (new_height+winclient_diff.y);
					break;
				}
				result_rect->right = result_rect->left + (new_width+winclient_diff.x);
				result_rect->bottom = new_bottom;
				ASSERT(true);	
			}
			else
			{
				result_rect->right = result_rect->left + new_size.x;
				result_rect->bottom = result_rect->top + new_size.y;
			}
		}break;
		// ASSERMSG(WM_MOUSEWHEEL)
		// ASSERMSG(WM_LBUTTONDBLCLK)
		// ASSERMSG(WM_LBUTTONDOWN)
		// ASSERMSG(WM_LBUTTONUP) 
		// ASSERMSG(WM_RBUTTONDOWN) 
		// ASSERMSG(WM_RBUTTONUP) 
		// ASSERMSG(WM_SYSKEYDOWN) 
		// ASSERMSG(WM_SYSKEYUP) 
		// ASSERMSG(WM_KEYDOWN) 
		// ASSERMSG(WM_KEYUP) 

		// UNPROCESSED MESSAGES
		case WM_NCMOUSELEAVE:
		case WM_NCMOUSEMOVE:

		
		case WM_IME_SETCONTEXT:
		case WM_IME_NOTIFY:

		case WM_WINDOWPOSCHANGED:

		// RESIZING WINDOW MESSAGES
		case WM_GETMINMAXINFO:
		case WM_ENTERSIZEMOVE:
		case WM_EXITSIZEMOVE:
		case WM_MOVING:
		// break;

		default:
			result = DefWindowProc(window, message, wParam, lParam);
	}

	return result;
}