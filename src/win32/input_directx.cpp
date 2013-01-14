#include "input_directx.h"


#include "..\win32\ini.h"  // Feather-ini-parser http://code.google.com/p/feather-ini-parser/
#include "..\win32\input.h"

#include "..\core\memory.h"

#define KEYDOWN(name, key) (name[key] & 0x80)

unsigned char * inputByte = &memInputState;
char m_buffer[256];
LPDIRECTINPUT8 m_diObject;          // DirectInput object
LPDIRECTINPUTDEVICE8 m_diDevice;    // DirectInput device

int defaultKeys[7]; // U,L,D,R,A,B,S
bool useKeyboard = true; // use the keyboard default
bool useJoy = false; // use the joystick? which one?
static std::string joyName;
int whichJoy = 0;    // which joystick should we use?

BOOL CALLBACK enumCallback(const DIDEVICEINSTANCE* instance, VOID* context)
{
    HRESULT hr;

    // Obtain an interface to the enumerated joystick.
    hr = m_diObject->CreateDevice(instance->guidInstance, &m_diDevice, NULL);

    // If it failed, then we can't use this joystick. (Maybe the user unplugged
    // it while we were in the middle of enumerating it.)
    if (FAILED(hr)) { 
        return DIENUM_CONTINUE;
    }

    // Stop enumeration. Note: we're just taking the first joystick we get. You
    // could store all the enumerated joysticks and let the user pick.
    return DIENUM_STOP;
}

void dx9_input_init(HINSTANCE hInst, HWND hWnd)
{
	if ( DirectInput8Create(hInst, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&m_diObject, NULL) == S_OK )
	{
		// Lets see what the INI says!
		std::string device;
		INI<std::string, std::string, std::string> ini("NeoGPC.ini", true);
		if ( ini.sections.size() != 0 ) // read the saved values from INI
		{
			device = ini["Controls"]["device"];
			joyName = ini["Controls"]["name"];
			defaultKeys[INPUT_UP] = atoi(ini["Controls"]["up"].c_str());
			defaultKeys[INPUT_LEFT] = atoi(ini["Controls"]["left"].c_str());
			defaultKeys[INPUT_DOWN] = atoi(ini["Controls"]["down"].c_str());
			defaultKeys[INPUT_RIGHT] = atoi(ini["Controls"]["right"].c_str());
			defaultKeys[INPUT_A] = atoi(ini["Controls"]["a"].c_str());
			defaultKeys[INPUT_B] = atoi(ini["Controls"]["b"].c_str());
			defaultKeys[INPUT_START] = atoi(ini["Controls"]["start"].c_str());
		}

		// Look for the first simple joystick we can find.
		if ( device == "joystick" )
		{
			if (m_diObject->EnumDevices(DI8DEVCLASS_GAMECTRL, enumCallback, NULL, DIEDFL_ATTACHEDONLY) != S_OK) {
				return;
			}
			
			// Make sure we got a joystick
			if (m_diDevice == NULL) {
			//	printf("Joystick not found.\n");
				return;
			}
		}
		else if ( device == "keyboard" )
		{
			if ( m_diObject->CreateDevice(GUID_SysKeyboard, &m_diDevice, NULL ) == S_OK )
			{
				if ( m_diDevice->SetDataFormat(&c_dfDIKeyboard) == S_OK )
				{
					if ( m_diDevice->SetCooperativeLevel(hWnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE ) == S_OK )
					{
						m_diDevice->Acquire();
					}
				}
			}
		}
   }
}

void dx9_input_reset()
{
}

void dx9_input_update()
{
   HRESULT hr;
   DWORD bufLen = sizeof(m_buffer);
   if ( m_diObject && m_diDevice )
   {
      m_diDevice->Poll();
      hr = m_diDevice->GetDeviceState(bufLen,(LPVOID)m_buffer);
      if( hr != S_OK )
      {
         // Reacquire if we lost the device (it happens..)
         m_diDevice->Acquire();
         return;
      }

	  if ( useKeyboard )
	  {
		  if ( KEYDOWN(m_buffer, defaultKeys[INPUT_UP]) )
		  {
			 *inputByte = 0x01;
		  }
		  else if ( KEYDOWN(m_buffer, defaultKeys[INPUT_DOWN]) )
		  {
			 *inputByte = 0x02;
		  }
		  else
		  {
			 *inputByte = 0x00;
		  }
		  if ( KEYDOWN(m_buffer, defaultKeys[INPUT_LEFT]) )
		  {
			 *inputByte |= 0x04;
		  }
		  else if ( KEYDOWN(m_buffer, defaultKeys[INPUT_RIGHT]) )
		  {
			 *inputByte |= 0x08;
		  }
		  if ( KEYDOWN(m_buffer, defaultKeys[INPUT_A]) )
		  {
			 *inputByte |= 0x10;
		  }
		  if ( KEYDOWN(m_buffer, defaultKeys[INPUT_B]) )
		  {
			 *inputByte |= 0x20;
		  }
		  if ( KEYDOWN(m_buffer, defaultKeys[INPUT_START]) )
		  {
			 *inputByte |= 0x40;
		  }
	  }
	  if ( useJoy )
	  {
		  
	  }
   }
}

void dx9_input_config(int i)
{
	// Detect any input in the form of joystick or keyboard
}

void dx9_input_shutdown()
{
   if ( m_diObject )
   {
      if ( m_diDevice )
      {
         // Unacquire the device
         m_diDevice->Unacquire();

         // Release the DirectInput device
         m_diDevice->Release();

         // Set the device to null
         m_diDevice = NULL;
      }

      // Release the DirectInput object
      m_diObject->Release();

      // Set the DirectInput object to NULL
      m_diObject = NULL;
   }
}
