#include "plattis.h"

#include "bass.h"
#pragma comment(lib,"bass.lib")

#include "basswma.h"
#pragma comment(lib,"basswma.lib")

float angles[256];
unsigned int closing = 0;
HWND hwnd = NULL;
LPDIRECT3DSURFACE9 main_back;
LPDIRECT3DSURFACE9 main_depth;
LPDIRECT3DDEVICE9 g_pd3dDevice = NULL; // Our rendering device

LRESULT WINAPI WndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
  if (msg==WM_CLOSE)
  {
    closing = 1;
    return 0;
  }
  if (msg==WM_KEYDOWN)
  {
    switch( wParam ) 
    {
      case VK_ESCAPE:
        {
          closing = 1;
        } break;
    }
  } 
  return DefWindowProc( hWnd, msg, wParam, lParam );
}

HRESULT InitD3D( unsigned int xres, unsigned int yres, bool fullscreen )
{
  WNDCLASSEX wc = {
    sizeof(WNDCLASSEX), 
    CS_CLASSDC | CS_DBLCLKS, 
    WndProc, 
    0L, 
    0L,
    GetModuleHandle(NULL), 
    NULL, 
    NULL, 
    NULL, 
    NULL,
    "dsd", 
    NULL
  };
  RegisterClassEx( &wc );

  unsigned int cx = GetSystemMetrics(SM_CXSCREEN);
  unsigned int cy = GetSystemMetrics(SM_CYSCREEN);

  //DWORD dwStyle = WS_OVERLAPPEDWINDOW | WS_CAPTION|WS_VISIBLE|WS_POPUP;
  DWORD dwStyle = WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
  if (!fullscreen) dwStyle |= WS_OVERLAPPED | WS_CAPTION;

  RECT rc;
  if (fullscreen)
  {
    cx = xres;
    cy = yres;
  }
  rc.left = (cx - xres) / 2;
  rc.top = (cy - yres) / 2;
  rc.right = rc.left + xres;
  rc.bottom = rc.top + yres;
  AdjustWindowRectEx( &rc, dwStyle, FALSE, NULL);

  hwnd = CreateWindowEx( 0,"dsd", "doomsday w32 2013",
    dwStyle, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top,
    NULL, NULL, wc.hInstance, NULL );

  LPDIRECT3D9 d3d;
  if( NULL == ( d3d = Direct3DCreate9( D3D_SDK_VERSION ) ) )
    return E_FAIL;

  // Get the current desktop display mode, so we can set up a back
  // buffer of the same format

  D3DDISPLAYMODE d3ddm;
  if( FAILED( d3d->GetAdapterDisplayMode( D3DADAPTER_DEFAULT, &d3ddm ) ) )
    return E_FAIL;

  // Set up the structure used to create the D3DDevice		
  D3DPRESENT_PARAMETERS d3dpp;

  ZeroMemory( &d3dpp, sizeof(d3dpp) );
  d3dpp.Windowed = !fullscreen;
  d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
  d3dpp.BackBufferFormat = d3ddm.Format;
  d3dpp.BackBufferWidth = xres;
  d3dpp.BackBufferHeight = yres;
  d3dpp.BackBufferCount = 0;	

  d3dpp.EnableAutoDepthStencil=true;
  d3dpp.AutoDepthStencilFormat=D3DFMT_D16;
  d3dpp.hDeviceWindow=hwnd;
  d3dpp.Flags=0;//D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
  d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

  // Create the D3DDevice
  if( FAILED( d3d->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd,
    D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED ,
    &d3dpp, &g_pd3dDevice) ) )
  {
    if( FAILED( d3d->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd,
      D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED ,
      &d3dpp, &g_pd3dDevice) ) )
    {
      return E_FAIL;
    }
  }

  g_pd3dDevice->Clear( 0L, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL, 0xff000000, 1.0f, 0L );
  g_pd3dDevice->Present( NULL, NULL, NULL, NULL );
  g_pd3dDevice->Clear( 0L, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL, 0xff000000, 1.0f, 0L );

  WRAP(CreateSecondaryRenderTarget());

  SetSecondaryRenderTarget();
  g_pd3dDevice->Clear( 0L, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL, 0xff000000, 1.0f, 0L );
  g_pd3dDevice->Present( NULL, NULL, NULL, NULL );
  g_pd3dDevice->Clear( 0L, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL, 0xff000000, 1.0f, 0L );
  RenderSecondary(main_back, main_depth);

  ShowWindow(hwnd,SW_SHOW);
  SetForegroundWindow(hwnd);

  return S_OK;
}
int WINAPI WinMain( __in HINSTANCE hInstance, __in_opt HINSTANCE hPrevInstance, __in LPSTR lpCmdLine, __in int nShowCmd )
{
  InitD3D( 640, 480, false );

  BASS_Init(-1,44100,NULL,hwnd,NULL);
  HSTREAM hStream = BASS_WMA_StreamCreateFile(FALSE,"media\\demo.wma",0,0,0);

  demo_init();

  float fStartSec = 0.0;
  BASS_Start();
  BASS_ChannelSetPosition( hStream, BASS_ChannelSeconds2Bytes(hStream,fStartSec), BASS_POS_BYTE);
  BASS_ChannelPlay( hStream, FALSE );
  while( !closing )
  {
    MSG msg;        
    while (PeekMessage(&msg,hwnd,0,0,PM_REMOVE))
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg); 
    }

    float t = BASS_ChannelBytes2Seconds( hStream, BASS_ChannelGetPosition( hStream, BASS_POS_BYTE ) );
    
    g_pd3dDevice->Clear( 0L, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL, 0xff000000, 1.0f, 0L );
    demo_render( t );
    g_pd3dDevice->Present( NULL, NULL, NULL, NULL );
  }
}