//////////////////////////////////////////////
//
//  NeoGPC :
//   NeoGeo Pocket Color emulator
//    Author(s): Luke Arntson
//                
//    Created: 10/13/2012
//    Last Edited: 11/5/2012
//
//////////////////////////////////////////////

// System includes
#include <windows.h>
#include <cstdio>

// Win32 local includes
#include "video.h"
#include "winsound.h"
#include "input.h"
#include "resource.h"

#include "ini.h"  // Feather-ini-parser http://code.google.com/p/feather-ini-parser/

// Win32 depends
#include <string>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h> // Used for INI defines

// NeoGPC includes
#include "../core/neogpc.h"
#include "../core/log.h"
#include "../core/neopopsound.h"
#include "../core/flash.h"

// NeoGPC Win32 includes
#include "../win32/windebugger.h"

bool isRunning = false;
bool isPaused = false;

// Settings INI read values
int g_scale;
int g_fullscreen;

// Debugger Variables
HWND g_tlcs900hDebugHwnd;
BOOL g_tlcs900hActive;

// Current time, previous time
INT64 m_ticksPerSecond;
INT64 m_currentTime;
INT64 m_lastTime;
INT64 m_lastFPSUpdate;
INT64 m_FPSUpdateInterval;
INT64 tmp;
UINT m_numFrames;
float m_runningTime;
float m_timeElapsed;
float m_fps;
BOOL m_timerStopped;

typedef enum {
	EMU_NO_ROM = 0,
	EMU_LOAD_ROM,
	EMU_ROM_RUNNING,
	EMU_QUIT
};

unsigned int m_emuState;

// Open this file
char szFile[100];
HMENU hMenu;

void ResetTimers()
{
   QueryPerformanceFrequency( (LARGE_INTEGER *)&m_ticksPerSecond );
   m_currentTime = m_lastTime = m_lastFPSUpdate = 0; 
   m_numFrames = 0; 
   m_runningTime = m_timeElapsed = m_fps = 0.0f; 
   m_FPSUpdateInterval = m_ticksPerSecond / HOST_FPS; 
   m_timerStopped = TRUE;
   
   QueryPerformanceCounter( (LARGE_INTEGER *)&m_lastTime ); 
   m_timerStopped = FALSE;
}

// Initialize our INI file
void InitIni()
{
	// Settings ini
	INI <std::string, std::string, std::string> ini("NeoGPC.ini", true);
	if ( ini.sections.size() == 0 || strcmp(ini["System"]["version"].c_str(), NEOGPC_VERSION) !=0 )
	{
		// Clear it if anything exists
		ini.Clear();

		// Initialize our fresh INI to default values
		ini.Create("System");		// Information about the system
		ini.Create("Controls");		// Input controls
		ini.Create("Video");		// Video section
		ini.Create("Audio");		// Audio section
		
		// Setup our system
		ini.Select("System");
		ini.Set("version", NEOGPC_VERSION);

		// Select our controls
		ini.Select("Controls");
		ini.Set("device","keyboard");
		ini.Set("name","keyboard");
		ini.Set("up", DIK_UP);
		ini.Set("left", DIK_LEFT);
		ini.Set("down", DIK_DOWN);
		ini.Set("right", DIK_RIGHT);
		ini.Set("a", DIK_Z);
		ini.Set("b", DIK_X);
		ini.Set("start", DIK_RETURN);

		// Select our video
		ini.Select("Video");
		ini.Set("scale", "3");
		ini.Set("fullscreen","0");

		ini.Save();  //Saves contents to file, does not Clear() afterwards
	}

	g_scale = atoi(ini["Video"]["scale"].c_str());
	g_fullscreen = atoi(ini["Video"]["fullscreen"].c_str());

}

INT_PTR CALLBACK TLCS900hProc(
  HWND hwndDlg,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam
)
{
    switch(uMsg)
    {
        case WM_COMMAND:
			switch(LOWORD(wParam))
            {
				case IDC_TLCS900HD_CLOSE:
					DestroyWindow(hwndDlg);
					return TRUE;
				break;
			}
			break;
		case WM_DESTROY:
			g_tlcs900hActive = false;
			break;
		default:
			int debug = 0;
			break;
    }
    return FALSE;
}

// Various window messages (handled by Main?)
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
         case WM_CREATE:
            HMENU hSubMenu;
            HICON hIcon, hIconSm;

			hMenu = GetMenu(hwnd);

			hSubMenu = GetSubMenu(hMenu,2);
			switch ( g_scale )
			{
			case 1:
				CheckMenuItem(hSubMenu, ID_SCALE1X_NGPC, MF_CHECKED);
				break;
			case 2:
				CheckMenuItem(hSubMenu, ID_SCALE2X_NGPC, MF_CHECKED);
				break;
			case 3:
				CheckMenuItem(hSubMenu, ID_SCALE3X_NGPC, MF_CHECKED);
				break;
			case 4:
				CheckMenuItem(hSubMenu, ID_SCALE4X_NGPC, MF_CHECKED);
				break;
			case 5:
				CheckMenuItem(hSubMenu, ID_SCALE5X_NGPC, MF_CHECKED);
				break;
			default:
				break;
			};

            break;
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case ID_FILE_EXIT:
                   PostMessage(hwnd, WM_CLOSE, 0, 0);
                break;
                case ID_LOAD_ROM:
					if (isRunning && !isPaused)
						win_sound_pause();
					// do not let rom loading change our current path
					char curPath[MAX_PATH];
					GetCurrentDirectory(MAX_PATH, curPath);
					// open a file name
					OPENFILENAME ofn ;
					ZeroMemory( &ofn , sizeof( ofn));
					ofn.lStructSize = sizeof ( ofn );
					ofn.hwndOwner = NULL ;
					ofn.lpstrFile = szFile;
					ofn.lpstrFile[0] = '\0';
					ofn.nMaxFile = sizeof( szFile );
					ofn.lpstrFilter = "NGPC Rom\0*.ngc\0";
					ofn.nFilterIndex =1;
					ofn.lpstrFileTitle = NULL ;
					ofn.nMaxFileTitle = 0 ;
					ofn.lpstrInitialDir=NULL ;
					ofn.Flags = OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST ;
					if ( GetOpenFileName( &ofn ) )
					{
						ResetTimers();
					}
					SetCurrentDirectory(curPath);
					if ( strcmp(ofn.lpstrFile, "") != 0 )
					{
						// For now, load "rom.ngc"
						FILE * rom = fopen(szFile, "r");
						long lSize;
						// obtain file size:
						fseek (rom , 0 , SEEK_END);
						lSize = ftell (rom);
						rewind (rom);
						char * buffer = (char*) malloc (sizeof(char)*lSize);
						fread(buffer, 1, lSize, rom);
						fclose(rom);

						// Find rom name
						int szLen = strlen(szFile);
						int bsFind = szLen-1;
						do
						{
							if ( szFile[bsFind] == '\\' )
							{
								break;
							}
						} while ( bsFind-- > 0 );
						char * romName = (char*)malloc(szLen - bsFind);
						strncpy(romName, &szFile[bsFind+1], szLen-bsFind);
						romName[szLen-bsFind-1] = '\0';

						// Load a rom into memory
						if (!neogpc_loadrom(buffer, lSize, romName))
						{
							LOG("Could not load the rom, exiting!\n");
							m_emuState = EMU_NO_ROM;
						}
						else
						{
							m_emuState = EMU_LOAD_ROM;

							// Grey out Load Rom, Ungrey Unload Rom
							HMENU hSubMenu = GetSubMenu(hMenu, 0);
							EnableMenuItem(hSubMenu, ID_LOAD_ROM, MF_BYCOMMAND | MF_DISABLED);
							EnableMenuItem(hSubMenu, ID_UNLOAD_ROM, MF_BYCOMMAND | MF_ENABLED);
						}

						free(romName);
					}
					if ( isRunning && !isPaused )
						win_sound_reset();
                break;
				case ID_UNLOAD_ROM:
					// Is a rom already running? shut it down and reload
					if ( isRunning )
					{
						isRunning = false;		// Kill our main process
						neogpc_shutdown();		// Shut down emulator
						win_sound_pause();
						
						// Ungrey Load Rom, Grey Unload Rom
						HMENU hSubMenu = GetSubMenu(hMenu, 0);
						EnableMenuItem(hSubMenu, ID_LOAD_ROM, MF_BYCOMMAND | MF_ENABLED);
						EnableMenuItem(hSubMenu, ID_UNLOAD_ROM, MF_BYCOMMAND | MF_DISABLED);
						
						// Unpause if it was paused
						if ( GetMenuState(hSubMenu, ID_PAUSE_ROM, MF_BYCOMMAND) == MF_CHECKED )
						{
							CheckMenuItem(hSubMenu, ID_PAUSE_ROM, MF_UNCHECKED);
							isPaused = false;
						}
					}
				break;
				case ID_RESET_ROM:
					neogpc_shutdown();		// Shut down emulator
					neogpc_init();			// Initialize the emulator
				break;
				case ID_PAUSE_ROM:
					if ( isRunning )
					{
						HMENU hSubMenu = GetSubMenu(hMenu,1);
						if ( GetMenuState(hSubMenu, ID_PAUSE_ROM, MF_BYCOMMAND) == MF_CHECKED )
						{
							CheckMenuItem(hSubMenu, ID_PAUSE_ROM, MF_UNCHECKED);
							win_sound_reset();
							isPaused = false;
						}
						else
						{
							CheckMenuItem(hSubMenu, ID_PAUSE_ROM, MF_CHECKED);
							win_sound_pause();
							isPaused = true;
						}
					}
				break;
				case ID_SCALE1X_NGPC:
					if ( g_scale != 1 )
					{
						HMENU hSubMenu = GetSubMenu(hMenu,2);
						CheckMenuItem(hSubMenu, ID_SCALE1X_NGPC+(g_scale-1), MF_UNCHECKED);
						g_scale = 1;
						video_resize(g_scale);
						CheckMenuItem(hSubMenu, ID_SCALE1X_NGPC, MF_CHECKED);
					}
				break;
				case ID_SCALE2X_NGPC:
					if ( g_scale != 2 )
					{
						HMENU hSubMenu = GetSubMenu(hMenu,2);
						CheckMenuItem(hSubMenu, ID_SCALE1X_NGPC+(g_scale-1), MF_UNCHECKED);
						g_scale = 2;
						video_resize(g_scale);
						CheckMenuItem(hSubMenu, ID_SCALE2X_NGPC, MF_CHECKED);
					}
				break;
				case ID_SCALE3X_NGPC:
					if ( g_scale != 3 )
					{
						HMENU hSubMenu = GetSubMenu(hMenu,2);
						CheckMenuItem(hSubMenu, ID_SCALE1X_NGPC+(g_scale-1), MF_UNCHECKED);
						g_scale = 3;
						video_resize(g_scale);
						CheckMenuItem(hSubMenu, ID_SCALE3X_NGPC, MF_CHECKED);
					}
				break;
				case ID_SCALE4X_NGPC:
					if ( g_scale != 4 )
					{
						HMENU hSubMenu = GetSubMenu(hMenu,2);
						CheckMenuItem(hSubMenu, ID_SCALE1X_NGPC+(g_scale-1), MF_UNCHECKED);
						g_scale = 4;
						video_resize(g_scale);
						CheckMenuItem(hSubMenu, ID_SCALE4X_NGPC, MF_CHECKED);
					}
				break;
				case ID_SCALE5X_NGPC:
					if ( g_scale != 5 )
					{
						HMENU hSubMenu = GetSubMenu(hMenu,2);
						CheckMenuItem(hSubMenu, ID_SCALE1X_NGPC+(g_scale-1), MF_UNCHECKED);
						g_scale = 5;
						video_resize(g_scale);
						CheckMenuItem(hSubMenu, ID_SCALE5X_NGPC, MF_CHECKED);
					}
				break;
				case ID_DEBUG_TLCS900H:
					if ( g_tlcs900hActive == false )
					{
						g_tlcs900hDebugHwnd = CreateDialog(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_TLCS900HDEBUGGER), hwnd, TLCS900hProc);
						if ( g_tlcs900hDebugHwnd != NULL )
						{
							g_tlcs900hActive = true;
							setupTLCS900hDebugger(g_tlcs900hDebugHwnd);	// Setup our TLCS900h debugger
							ShowWindow(g_tlcs900hDebugHwnd, SW_SHOW);
							return TRUE;
						}
					}
				break;
            }
            break;
		case WM_ENTERMENULOOP:
			isPaused = true;
			win_sound_pause();
		break;
		case WM_EXITMENULOOP:
		{
			HMENU hSubMenu = GetSubMenu(hMenu, 1);
			if ( GetMenuState(hSubMenu, ID_PAUSE_ROM, MF_BYCOMMAND) == MF_UNCHECKED )
			{
				isPaused = false;
				win_sound_reset();
			}
		}
		break;
        case WM_CLOSE:
            DestroyWindow(hwnd);
        break;
        case WM_DESTROY:
            PostQuitMessage(0);
        break;
        case WM_PAINT:
            // Render when the window wants to
			video_render();
        break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow)
{
	// Keep our window open
	MSG Msg;

	// Initialize INI system
	InitIni();

	// Initialize our video driver
	if ( !video_init(hInstance, hPrevInstance, lpCmdLine, nCmdShow, WndProc, g_fullscreen, g_scale) )
	{
		// Log video error, exit
		return -1;
	}

	// Initialize the sound engine
	win_sound_init(g_hwnd);

	// Initialize our input driver
	input_init(hInstance, g_hwnd);

	// Debugger Variables
	g_tlcs900hActive = false;

	// We haven't quit yet
	m_emuState = EMU_NO_ROM;

	while(m_emuState != EMU_QUIT)
	{
		//Message Pump
		if (PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE))
		{
			if (!TranslateAccelerator(g_hwnd, NULL, &Msg))
			{
				TranslateMessage(&Msg);
				DispatchMessage(&Msg);
			}
			/*
			if (!IsDialogMessage (g_tlcs900hDebugHwnd, &Msg)) // When should we do this??
			{
				TranslateMessage ( &Msg );
				DispatchMessage ( &Msg );
			}
			*/
		}
		// Is the rom currently running?
		if ( isRunning && !isPaused)
		{
			// Get the current time 
			QueryPerformanceCounter( (LARGE_INTEGER *)&m_currentTime );

			m_timeElapsed = float(m_currentTime - m_lastTime) / (float)m_ticksPerSecond; 
			m_runningTime += m_timeElapsed;

			// Update FPS 
			m_numFrames++;
			tmp = m_currentTime - m_lastFPSUpdate;
			if ( tmp >= m_FPSUpdateInterval ) 
			{
				float currentTime = (float)m_currentTime / (float)m_ticksPerSecond; 
				float lastTime = (float)m_lastFPSUpdate / (float)m_ticksPerSecond;
				tmp = currentTime - lastTime;
				m_fps = (float)m_numFrames / (float)tmp;
				m_lastFPSUpdate = m_currentTime; 
               
				// Get the input if we're about to emulate
				input_update();

				// Emulate the system
				neogpc_emulate(1); // pretend 60 fps (17ms)

				// Update video buffers as needed
				video_update();

				// Update the sound buffers
				win_sound_update();

				m_numFrames = 0;

				Sleep(1); // add some sleep

			}

			m_lastTime = m_currentTime;
		}
		// Is the TLCS900h Debugger Active?
		if ( g_tlcs900hActive == true )
		{
		}		
		// Should we load a rom?
		if ( EMU_LOAD_ROM == m_emuState )
		{
			// Setup our flash folder
			if (GetFileAttributes(SAVEGAME_DIR) == INVALID_FILE_ATTRIBUTES) {
				CreateDirectory(SAVEGAME_DIR,NULL);
			}

			// Ensure sound is working
			win_sound_reset();

			// Initialize the emulator
			neogpc_init();

			LOG("Rom loaded, NGPC loaded, lets rock and roll!");

			m_emuState = EMU_ROM_RUNNING;
			isRunning = true;
		}
		// Did we get a quit message?
		if ( Msg.message == WM_QUIT )
		{
			// Shut everything down
			m_emuState = EMU_QUIT;
			
			// Is a rom already running? shut it down and reload
			if ( isRunning )
			{
				neogpc_shutdown();		// Shut down emulator
			}
		}
	}
	// Shut it all down
	input_shutdown();
	win_sound_shutdown();
	video_shutdown();
	return 0;
}
