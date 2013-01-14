#include "video_directx.h"

// DirectX 9 includes
#include <d3d9.h>
#include <d3dx9.h>

// NeoGPC includes
#include "../core/graphics.h"

// include the Direct3D Library file
#pragma comment (lib, "d3d9.lib")
#pragma comment (lib, "d3dx9.lib")

// global declarations
LPDIRECT3D9 d3d;    // the pointer to our Direct3D interface
LPDIRECT3DDEVICE9 d3ddev;    // the pointer to the device class
D3DPRESENT_PARAMETERS d3dpp;
D3DDISPLAYMODE d3ddm;

// Our screen texture (pixels render directly to this)
LPD3DXSPRITE g_screenSprite = NULL;
LPDIRECT3DTEXTURE9 g_screenTex = NULL;

// Scale the sprite to match the window
D3DXMATRIX		pTransform;
D3DXVECTOR2		spritePos;
D3DXVECTOR2		spriteCenter;
RECT srcRect = {0.0f, 0.0f, 256.0f, 256.0f};
D3DXVECTOR3 vCenter( 0.0f, 0.0f, 0.0f );
D3DXVECTOR3 vPosition( 0.0f, 0.0f, 0.0f );

BOOL dx9vid_init()
{
	d3d = Direct3DCreate9(D3D_SDK_VERSION);    // create the Direct3D interface
	D3DCAPS9 d3dCaps;
	d3d->GetDeviceCaps( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &d3dCaps );
	d3d->GetAdapterDisplayMode( D3DADAPTER_DEFAULT, &d3ddm );

	ZeroMemory( &d3dpp, sizeof(d3dpp) );
	d3dpp.Windowed               = TRUE;
	d3dpp.SwapEffect             = D3DSWAPEFFECT_DISCARD;
	d3dpp.BackBufferFormat       = d3ddm.Format;
	d3dpp.EnableAutoDepthStencil = TRUE;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
	d3dpp.PresentationInterval   = D3DPRESENT_INTERVAL_IMMEDIATE;

	d3d->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, g_hwnd,
						D3DCREATE_SOFTWARE_VERTEXPROCESSING,
						&d3dpp, &d3ddev );

	// Create the screen texture
	D3DXCreateTexture( d3ddev, 256, 256, D3DX_FILTER_NONE, D3DUSAGE_DYNAMIC, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &g_screenTex);

	// Create the screen sprite (ignores 3d perspective)
	D3DXCreateSprite( d3ddev, &g_screenSprite );

	// Scale our matrix to match the screen
	D3DXMatrixIdentity( &pTransform );
	spritePos = D3DXVECTOR2( 0.f, 0.f );
	spriteCenter = D3DXVECTOR2( 128.f, 128.f);
	D3DXVECTOR2 vscale = D3DXVECTOR2( float(2.0f), float(2.0f));
	D3DXMatrixTransformation2D( &pTransform, NULL, 0.0f, &vscale, &spriteCenter, 0.f, &spritePos );
	g_screenSprite->SetTransform(&pTransform);

	return TRUE;
}

BOOL dx9vid_update()
{
   // Update from the graphics
  // lets draw a random pixel
   D3DLOCKED_RECT pLocked;
   DWORD ret;
   ret = g_screenTex->LockRect(0, &pLocked, 0, 0);
   if ( ret != D3D_OK )
   {
      return true;
   }

   // Draw some pixels
   DWORD* pDst       = (DWORD*)pLocked.pBits;

   //draw the screen to our sprite
   //for (int count=0; count < NGPC_SIZEY*NGPC_SIZEX; count++)
   //{
   for(int y = 0; y < NGPC_SIZEY; y++)
   {
      for(int x = 0; x < NGPC_SIZEX; x++)
      {
         unsigned short color = drawBuffer[y*NGPC_SIZEX+x];
         unsigned short b = ((color&0x003E)>>1)*8;
         unsigned short g = ((color&0x07C0)>>6)*8;
         unsigned short r = ((color&0xF800)>>11)*8;
         if ( color != 0 )
         {
            int debug = 0;
         }
         pDst[y*256+x] =   (r<<16) | (g<<8) | b;  //b (4-bit)
         //pDst[y*256+x] |=  g<<4;  //g (4-bit)
         //pDst[y*256+x] |=  r<<8;  //b (4-bit)1
         //pDst[y*256+x] |= 0<<12; //x
      }
   }
   g_screenTex->UnlockRect(0);

   return TRUE;
}

BOOL dx9vid_render()
{
   // clear the window to a deep blue
   d3ddev->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 40, 100), 1.0f, 0);
   d3ddev->BeginScene();    // begins the 3D scene
   g_screenSprite->Begin( 0 );

   d3ddev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
   d3ddev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
   d3ddev->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);

   g_screenSprite->Draw( g_screenTex, &srcRect, &vCenter, &vPosition,
                     D3DCOLOR_COLORVALUE(1.0f,1.0f,1.0f,1.0f) );
   g_screenSprite->End();
   d3ddev->EndScene();    // ends the 3D scene
   d3ddev->Present(NULL, NULL, NULL, NULL);   // displays the created frame on the screen
   return TRUE;
}

BOOL dx9vid_resize(int scale)
{
	D3DXVECTOR2 vscale = D3DXVECTOR2( float(2.0f), float(2.0f));
	D3DXMatrixTransformation2D( &pTransform, NULL, 0.0f, &vscale, &spriteCenter, 0.f, &spritePos );
	g_screenSprite->SetTransform(&pTransform);
	
	return TRUE;
}

void dx9vid_shutdown()
{
   g_screenSprite->Release(); // release the screen sprite
   g_screenTex->Release(); // release the screen texture
   d3ddev->Release();    // close and release the 3D device
   d3d->Release();    // close and release Direct3D
}
