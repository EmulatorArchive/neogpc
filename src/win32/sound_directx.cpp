#include "sound_directx.h"

#include <Windows.h>

#include "..\core\memory.h"
#include "..\core\sound.h"
#include "..\core\neopopsound.h"

LPDIRECTSOUND8 m_dsObject;
LPDIRECTSOUNDBUFFER m_dsBuffer;
LPDIRECTSOUNDBUFFER m_chipBuffer;
LPDIRECTSOUNDBUFFER m_dacBuffer;

#define CHIPFREQ 44100
#define CHIPBUFLEN 35280

#define DACFREQ 8000   // hz From NeoPop?
#define DACBUFLEN 128000 // 16 * 8000

void dx9_sound_init(HWND hWnd)
{
   HRESULT hr = DirectSoundCreate8(NULL, &m_dsObject, NULL);
   if ( hr == S_OK )
   {
      // Set DirectSound cooperative level
      hr = m_dsObject->SetCooperativeLevel( hWnd, DSSCL_PRIORITY );
      if ( hr == S_OK )
         hr = m_dsObject->SetCooperativeLevel( hWnd, DSSCL_NORMAL );
      if ( hr == S_OK )
      {
         // Define a WAVEFORMATEX structure
         WAVEFORMATEX wfx;
         DSBUFFERDESC dsbdesc;

         //!! Create a primary buffer

         // Set the DS BUffer DESC
         ZeroMemory(&dsbdesc, sizeof(DSBUFFERDESC));
         dsbdesc.dwSize = sizeof(DSBUFFERDESC);
         dsbdesc.dwFlags = DSBCAPS_PRIMARYBUFFER;
         dsbdesc.dwBufferBytes = 0;
         dsbdesc.lpwfxFormat = NULL;

         // Create a sound buffer
         m_dsBuffer = NULL;
         hr = m_dsObject->CreateSoundBuffer(&dsbdesc, &m_dsBuffer, NULL);
         if ( hr != S_OK )
            return;

         // Set Primary buffer format
         memset(&wfx, 0, sizeof(WAVEFORMATEX));
         wfx.wFormatTag = WAVE_FORMAT_PCM;
         wfx.nChannels = 2;
         wfx.nSamplesPerSec = CHIPFREQ;
         wfx.wBitsPerSample = 16;
         wfx.nBlockAlign = (wfx.wBitsPerSample / 8) * wfx.nChannels;
         wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

         hr = m_dsBuffer->SetFormat(&wfx);

         // Create the Chip buffer (3-tones mixed to one)
         ZeroMemory(&dsbdesc, sizeof(DSBUFFERDESC));
         dsbdesc.dwSize = sizeof(DSBUFFERDESC);
         dsbdesc.dwFlags = DSBCAPS_LOCSOFTWARE | DSBCAPS_GETCURRENTPOSITION2;
         dsbdesc.lpwfxFormat = &wfx;
         dsbdesc.dwBufferBytes = CHIPBUFLEN;
         
         hr = m_dsObject->CreateSoundBuffer(&dsbdesc, &m_chipBuffer, NULL);
         if ( hr != S_OK )
            return;

         // Create a DAC buffer
         // Set DAC buffer format
         memset(&wfx, 0, sizeof(WAVEFORMATEX)); 
         wfx.wFormatTag = WAVE_FORMAT_PCM; 
         wfx.nChannels = 1;
         wfx.nSamplesPerSec = DACFREQ;	
         wfx.wBitsPerSample = 8;
         wfx.nBlockAlign = (wfx.wBitsPerSample / 8) * wfx.nChannels;
         wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

         memset(&dsbdesc, 0, sizeof(DSBUFFERDESC));
	      dsbdesc.dwSize = sizeof(DSBUFFERDESC);
	      dsbdesc.dwFlags = DSBCAPS_LOCSOFTWARE | DSBCAPS_GETCURRENTPOSITION2;
	      dsbdesc.lpwfxFormat = &wfx;
	      dsbdesc.dwBufferBytes = DACBUFLEN;

         hr = m_dsObject->CreateSoundBuffer(&dsbdesc, &m_dacBuffer, NULL);
         if ( hr != S_OK )
            return;

		 system_sound_chipreset();	//Resets chips
//	      system_sound_silence();	// Clear buffers and starts playing them.
         // Sound Silence
         BYTE	*ppvAudioPtr1, *ppvAudioPtr2;
	      DWORD	pdwAudioBytes1, pdwAudioBytes2;

	      chipWrite = UNDEFINED;
	      dacWrite = UNDEFINED; 

	      if (m_chipBuffer)
	      {
		      m_chipBuffer->Stop();

		      // Fill the sound buffer
            hr = m_chipBuffer->Lock(0, 0, (LPVOID*)&ppvAudioPtr1, &pdwAudioBytes1, (LPVOID*)&ppvAudioPtr2, &pdwAudioBytes2, DSBLOCK_ENTIREBUFFER);
		      if (hr == S_OK)
		      {
			      if (ppvAudioPtr1 && pdwAudioBytes1)
				      memset(ppvAudioPtr1, 0, pdwAudioBytes1);

			      if (ppvAudioPtr2 && pdwAudioBytes2)
				      memset(ppvAudioPtr2, 0, pdwAudioBytes2);
			
               m_chipBuffer->Unlock(ppvAudioPtr1, pdwAudioBytes1, ppvAudioPtr2, pdwAudioBytes2);
		      }

		      //Start playing
		      if (mute == FALSE)
               m_chipBuffer->Play(0, 0, DSBPLAY_LOOPING );
	      }

	      if (m_dacBuffer)
	      {
		      m_dacBuffer->Stop();

		      // Fill the sound buffer
            hr = m_dacBuffer->Lock(0, 0, (LPVOID*)&ppvAudioPtr1, &pdwAudioBytes1, (LPVOID*)&ppvAudioPtr2, &pdwAudioBytes2, DSBLOCK_ENTIREBUFFER);
		      if ( hr == S_OK )
		      {
			      if (ppvAudioPtr1 && pdwAudioBytes1)
				      memset(ppvAudioPtr1, 0x80, pdwAudioBytes1);

			      if (ppvAudioPtr2 && pdwAudioBytes2)
				      memset(ppvAudioPtr2, 0x80, pdwAudioBytes2);
			
               m_dacBuffer->Unlock(ppvAudioPtr1, pdwAudioBytes1, ppvAudioPtr2, pdwAudioBytes2);
		      }

		      //Start playing
		      if (mute == FALSE)
               m_dacBuffer->Play(0,0, DSBPLAY_LOOPING );
	      }

      }
   }
}

void dx9_sound_reset()
{
	// Sound Silence
	BYTE	*ppvAudioPtr1, *ppvAudioPtr2;
	DWORD	pdwAudioBytes1, pdwAudioBytes2;

	chipWrite = UNDEFINED;
	dacWrite = UNDEFINED; 

	if (m_chipBuffer)
	{
		m_chipBuffer->Stop();

		// Fill the sound buffer
		HRESULT hr = m_chipBuffer->Lock(0, 0, (LPVOID*)&ppvAudioPtr1, &pdwAudioBytes1, (LPVOID*)&ppvAudioPtr2, &pdwAudioBytes2, DSBLOCK_ENTIREBUFFER);
		if (hr == S_OK)
		{
			if (ppvAudioPtr1 && pdwAudioBytes1)
				memset(ppvAudioPtr1, 0, pdwAudioBytes1);

			if (ppvAudioPtr2 && pdwAudioBytes2)
				memset(ppvAudioPtr2, 0, pdwAudioBytes2);
			
		m_chipBuffer->Unlock(ppvAudioPtr1, pdwAudioBytes1, ppvAudioPtr2, pdwAudioBytes2);
		}

		//Start playing
		if (mute == FALSE)
		m_chipBuffer->Play(0, 0, DSBPLAY_LOOPING );
	}

	if (m_dacBuffer)
	{
		m_dacBuffer->Stop();

		// Fill the sound buffer
		HRESULT hr = m_dacBuffer->Lock(0, 0, (LPVOID*)&ppvAudioPtr1, &pdwAudioBytes1, (LPVOID*)&ppvAudioPtr2, &pdwAudioBytes2, DSBLOCK_ENTIREBUFFER);
		if ( hr == S_OK )
		{
			if (ppvAudioPtr1 && pdwAudioBytes1)
				memset(ppvAudioPtr1, 0x80, pdwAudioBytes1);

			if (ppvAudioPtr2 && pdwAudioBytes2)
				memset(ppvAudioPtr2, 0x80, pdwAudioBytes2);
			
		m_dacBuffer->Unlock(ppvAudioPtr1, pdwAudioBytes1, ppvAudioPtr2, pdwAudioBytes2);
		}

		//Start playing
		if (mute == FALSE)
		m_dacBuffer->Play(0,0, DSBPLAY_LOOPING );
	}
}

void dx9_sound_pause()
{
	m_chipBuffer->Stop();
	m_dacBuffer->Stop();
}

void dx9_sound_update()
{
	DWORD		pdwAudioBytes1, pdwAudioBytes2, count;
	DWORD		Write, LengthBytes;
	unsigned short	*chipPtr1, *chipPtr2, *src16, *dest16;
	unsigned char		*dacPtr1, *dacPtr2, *src8, *dest8;

	m_chipBuffer->GetCurrentPosition(NULL, &Write);

	// UNDEFINED write cursors
	if (chipWrite == UNDEFINED || dacWrite == UNDEFINED)
	{
		lastChipWrite = chipWrite = Write;

		// Get DAC position too.
      m_dacBuffer->GetCurrentPosition(NULL, &Write);
		lastDacWrite = dacWrite = Write;

		return; //Wait a frame to accumulate length.
	}

	//Chip -> Direct Sound
	//====================

	if (Write < lastChipWrite)	//Wrap?
		lastChipWrite -= CHIPBUFFERLENGTH;

	LengthBytes = Write - lastChipWrite;
	lastChipWrite = Write;

	sound_update((unsigned short*)blockSound, LengthBytes>>1);	//Get sound data

	if (m_chipBuffer->Lock(chipWrite, LengthBytes, (LPVOID*)&chipPtr1, &pdwAudioBytes1, 
		(LPVOID*)&chipPtr2, &pdwAudioBytes2, 0) == S_OK)
	{
		src16 = (unsigned short*)blockSound;	//Copy from this buffer

		dest16 = chipPtr1;
		count = pdwAudioBytes1 >> 2; // mono -> stereo = count/2
		while(count)
		{ 
			*dest16++ = *src16;
			*dest16++ = *src16++; count--; 
		}

		//Buffer Wrap?
		if (chipPtr2)
		{
			dest16 = chipPtr2;
			count = pdwAudioBytes2 >> 2;	// mono -> stereo = count/2
			while(count)
			{ 
				*dest16++ = *src16;
				*dest16++ = *src16++; count--; 
			}
		}
		
		m_chipBuffer->Unlock(chipPtr1, pdwAudioBytes1, chipPtr2, pdwAudioBytes2);

		chipWrite += LengthBytes;
		if (chipWrite > CHIPBUFFERLENGTH)
			chipWrite -= CHIPBUFFERLENGTH;
	}
	else
	{
		DWORD status;
		chipWrite = UNDEFINED;
		m_chipBuffer->GetStatus(&status);
		if (status & DSBSTATUS_BUFFERLOST)
		{
         if (m_chipBuffer->Restore() != DS_OK) return;
         if (!mute) m_chipBuffer->Play(0, 0, DSBPLAY_LOOPING);
		}
	}

	//DAC -> Direct Sound
	//===================
	m_dacBuffer->GetCurrentPosition(NULL, &Write);

	if (Write < lastDacWrite)	//Wrap?
		lastDacWrite -= DACBUFLEN;

	LengthBytes = (Write - lastDacWrite); 
	lastDacWrite = Write;

	dac_update(blockDAC, LengthBytes);	//Get DAC data

	if (m_dacBuffer->Lock(dacWrite, LengthBytes, (LPVOID*)&dacPtr1, &pdwAudioBytes1, 
		(LPVOID*)&dacPtr2, &pdwAudioBytes2, 0) == S_OK)
	{
		src8 = (unsigned char*)blockDAC;	//Copy from this buffer

		dest8 = dacPtr1;
		count = pdwAudioBytes1;
		while(count)
		{ 
		   *dest8++ = *src8++; 
		   count--;
		}

		//Buffer Wrap?
		if (dacPtr2)
		{
		   dest8 = dacPtr2;
		   count = pdwAudioBytes2;
		   while(count)
		   { 
			   *dest8++ = *src8++; 
			   count--;
		   }
		}
		
		m_dacBuffer->Unlock(dacPtr1, pdwAudioBytes1, dacPtr2, pdwAudioBytes2);

		dacWrite += LengthBytes;
		if (dacWrite >= DACBUFLEN)
			dacWrite -= DACBUFLEN;
	}
	else
	{
		DWORD status;
		dacWrite = UNDEFINED;
		m_dacBuffer->GetStatus(&status);
		if (status & DSBSTATUS_BUFFERLOST)
		{
			if (m_dacBuffer->Restore() != DS_OK) return;
         if (!mute) m_dacBuffer->Play(0, 0, DSBPLAY_LOOPING);
		}
	}
}

void dx9_sound_shutdown()
{
   m_dsObject->Release();
}
