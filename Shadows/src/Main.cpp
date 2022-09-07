#include "Mesh.h"
#include <stdexcept>
#include <functional>

using namespace std;

extern HWND hWnd = NULL;
extern LPDIRECT3D9 pD3D = NULL;
extern LPDIRECT3DDEVICE9 pd3dDevice = NULL;
extern LPD3DXEFFECT pLightingEffect = NULL;
extern LPD3DXFONT pFont = NULL;

bool animate;
bool showPenumbraCone;
const D3DXCOLOR fontColor = D3DXCOLOR(1.0f, 1.0f, 0.0f, 1.0f);

int nLights;
Camera camera;
Mesh lightMesh;
vector<Mesh> meshes;
vector<Light> lights;

// FPS
int framesLeft;
float fps;
float lastTime;

static const int DYNAMIC_OBJ = 0;
static const int STATIC_OBJ = 1;
static const int ROOM  = 2;

static const int width = 800;
static const int height = 800;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void Init(void);
void InitScene(void);
void InitEffects(void);
void ShutDown(void);
void Render(void);
void Update(void);

// Misc functions
inline DWORD F2DW(float f) {
	return *((DWORD*)&f);
}

float GetTime() {
	// var
	LARGE_INTEGER frequency;
	LARGE_INTEGER performanceCount;
	
	// Get time
	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&performanceCount);
	 
	return (float)(performanceCount.QuadPart) / frequency.QuadPart;
}

D3DXMATRIX GetCameraTransform() {
    D3DXMATRIX   transform;
    D3DXVECTOR3  position;

    position.x = cosf(camera.yaw)*cosf(camera.pitch)*camera.radius;
    position.z = sinf(camera.yaw)*cosf(camera.pitch)*camera.radius;
    position.y = sinf(camera.pitch)*camera.radius;

    D3DXMatrixLookAtLH(&transform, &position, &camera.eyePt, &camera.up);
    return transform;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	WNDCLASSEX winClass;
	MSG uMsg;

    memset(&uMsg,0,sizeof(uMsg));

	winClass.lpszClassName = "MY_WINDOWS_CLASS";
	winClass.cbSize = sizeof(WNDCLASSEX);
	winClass.style = CS_HREDRAW | CS_VREDRAW;
	winClass.lpfnWndProc = WindowProc;
	winClass.hInstance = hInstance;
	winClass.hIcon = NULL;
    winClass.hIconSm = NULL;
	winClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	winClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	winClass.lpszMenuName = NULL;
	winClass.cbClsExtra = 0;
	winClass.cbWndExtra = 0;

	if(!RegisterClassEx(&winClass))
		return E_FAIL;

	hWnd = CreateWindowEx(NULL, "MY_WINDOWS_CLASS", "Soft shadows - Charan", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, 0, width, height, NULL, NULL, hInstance, NULL);

	if(hWnd == NULL)
		return E_FAIL;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    ShowCursor(false);

    try
    {
        // Try init
	    Init();
        InitScene();
        InitEffects();

        while(uMsg.message != WM_QUIT) {
		    if(PeekMessage(&uMsg, NULL, 0, 0, PM_REMOVE)) { 
			    TranslateMessage(&uMsg);
			    DispatchMessage(&uMsg);
		    }
            else {
		        Render();
                Update();
            }
	    }
    }
    catch(std::runtime_error& error)
    {
        // Oops
        MessageBoxA(hWnd, error.what(), "Error", MB_OK | MB_ICONEXCLAMATION);
	    ShutDown();
        UnregisterClassA( "MY_WINDOWS_CLASS", winClass.hInstance );
        
        return 1;
    }

	ShutDown();
    UnregisterClassA( "MY_WINDOWS_CLASS", winClass.hInstance );

	return uMsg.wParam;
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch(msg)
	{
        case WM_KEYDOWN:
		{
            D3DXMATRIX   transform;

			switch( wParam )
			{
                // pause/contunie animation
                case 0x50: // P-key
                    animate = !animate;
                    break;

                
                // show/hide penumbra cone
                case 0x52: // R-key
                    showPenumbraCone = !showPenumbraCone;
                    break;

                
                // show/hide second light
                case 0x4C: // L-key
                    nLights = 3 - nLights;
                    break;

                // increase light radius
                case VK_OEM_PLUS:
                    lights[0].radius += 0.02f;
                    break;

				// Move 2nd light source
				case VK_LEFT:
					lights[0].position.z -= 0.2f;
					break;

				case VK_RIGHT:
					lights[0].position.z += 0.2f;
					break;

				case VK_UP:
					lights[0].position.x -= 0.2f;
					break;

				case VK_DOWN:
					lights[0].position.x += 0.2f;
					break;

				case 0x44: // D key
					lights[0].position.y -= 0.2f;
					break;

				case 0x55: // U key
					lights[0].position.y += 0.2f;
					break;

                // decrease light radius
                case VK_OEM_MINUS:
                    if (lights[0].radius > 0.02) lights[0].radius -= 0.02f;
                    break;

				case VK_ESCAPE:
					PostQuitMessage(0);
					break;
			}

            break;
        }

        // Move camera around
        case WM_MOUSEMOVE:
        {
            POINT pos;
            GetCursorPos(&pos);
            if (pos.x != 400 && pos.y != 300)
            {
                camera.yaw += (pos.x - 400.0f) * 0.001f;
                camera.pitch += (pos.y - 300.0f) * 0.001f;
                SetCursorPos(400, 300);
            }
            break;
        }
            

        // Move camera forward or backward
        case WM_MOUSEWHEEL:
            camera.radius -= static_cast<float>( GET_WHEEL_DELTA_WPARAM(wParam) ) / 50.0f; // wheel rotation 
            break;

		case WM_CLOSE:
        case WM_DESTROY:
            PostQuitMessage(0);
            break;

		default:
			return DefWindowProc( hWnd, msg, wParam, lParam );
		    break;
	}

	return 0;
}

void Init(void) {
	D3DCAPS9 d3dCaps;
    D3DDISPLAYMODE d3ddm;
	D3DPRESENT_PARAMETERS d3dpp;
	DWORD dwBehaviorFlags = 0;

    pD3D = Direct3DCreate9(D3D_SDK_VERSION);
    pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm);
    pD3D->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3ddm.Format, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, D3DFMT_D24S8);
    pD3D->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &d3dCaps);
	
	if( d3dCaps.VertexProcessingCaps != 0 )
		dwBehaviorFlags |= D3DCREATE_HARDWARE_VERTEXPROCESSING;
	else
		dwBehaviorFlags |= D3DCREATE_SOFTWARE_VERTEXPROCESSING;

	memset(&d3dpp, 0, sizeof(d3dpp));

    d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferWidth = width;
    d3dpp.BackBufferHeight = height;
	d3dpp.Windowed = TRUE;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
    d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

    pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, dwBehaviorFlags, &d3dpp, &pd3dDevice);
    D3DXCreateFontA(pd3dDevice, 18, 0, FW_BOLD, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Font", &pFont);  
    ZTexture::Instance()->Init(width, height);
    ScreenQuad::Instance()->Init();
}

void InitScene(void) {
    // Set matrices
    D3DXMATRIX  matProj;
    D3DXMATRIX  transform;

    camera.yaw = 0.0f;
    camera.radius = 60.0f;
    camera.pitch = 0.5f;
    camera.eyePt = D3DXVECTOR3(0.0, 0.0, 0.0);
    camera.up = D3DXVECTOR3(0.0, 1.0, 0.0);

    D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI/4, 1.0f, static_cast<float>(width)/height, 500.0f);
    pd3dDevice->SetTransform(D3DTS_PROJECTION, &matProj);

    // States
    pd3dDevice->SetRenderState(D3DRS_LIGHTING, TRUE);
    pd3dDevice->SetRenderState(D3DRS_AMBIENT, D3DCOLOR_COLORVALUE(0.35, 0.35, 0.35, 1.0));
    pd3dDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
	pd3dDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_ANISOTROPIC);
	pd3dDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_ANISOTROPIC);

    // Load scene
    meshes.resize(3);
    meshes[DYNAMIC_OBJ].Load("E:\\sem6\\acg\\test_shadows\\Shadows\\Shadows\\data\\group.x");
    meshes[STATIC_OBJ].Load("E:\\sem6\\acg\\test_shadows\\Shadows\\Shadows\\data\\torus.x");
    meshes[ROOM].Load("E:\\sem6\\acg\\test_shadows\\Shadows\\Shadows\\data\\ground.x");
    lightMesh.Load("E:\\sem6\\acg\\test_shadows\\Shadows\\Shadows\\data\\light.x");

    D3DXMatrixTranslation(&transform, 8.0f, 3.0f, 0.0f);
    meshes[DYNAMIC_OBJ].Transform(transform);
	
	D3DXMatrixRotationX(&transform, D3DX_PI / 2);
	meshes[STATIC_OBJ].Transform(transform);

	D3DXMatrixRotationY(&transform, D3DX_PI / 2);
	meshes[STATIC_OBJ].Transform(transform);
	
	D3DXMatrixTranslation(&transform, 0.0f, 1.0f, 0.0f);
	meshes[STATIC_OBJ].Transform(transform);

	D3DXMatrixScaling(&transform, 2.0f, 2.0f, 2.0f);
	meshes[STATIC_OBJ].Transform(transform);

    D3DXMatrixScaling(&transform, 3.0f, 3.0f, 3.0f);
    meshes[ROOM].Transform(transform);

    // Lights
    lights.resize(2);

    lights[0].position = D3DXVECTOR4(-15.0, 12.0, 0.0, 1.0);
    lights[0].color = D3DXVECTOR4(0.0, 1.0, 1.0, 1.0);
    lights[0].linearAttenuation = 0.03f;
    lights[0].radius = 1.0f;
    lights[0].range = 150.0f;

    lights[1].position = D3DXVECTOR4(20.0, 13.0, 0.0, 1.0);
    lights[1].color = D3DXVECTOR4(0.0, 1.0, 1.0, 1.0);
    lights[1].linearAttenuation = 0.01f;
    lights[1].range = 150.0f;
    lights[1].radius = 2.0f;

    animate = true;
	nLights = 1;
    lastTime = GetTime();
    framesLeft = 0;
    showPenumbraCone = false;
    
}

void InitEffects(void) {
    LPD3DXBUFFER    pBufferErrors = NULL;

    // Load effect
	D3DXCreateEffectFromFileA( pd3dDevice, "E:\\sem6\\acg\\test_shadows\\Shadows\\Shadows\\shaders\\Lighting.fx", NULL, NULL, 0, NULL, &pLightingEffect, &pBufferErrors);
}

void ShutDown(void) {
    for_each(meshes.begin(), meshes.end(), mem_fun_ref(&Mesh::Clear));
    if (pFont) pFont->Release();
    if (pLightingEffect) pLightingEffect->Release();
    if (pd3dDevice) pd3dDevice->Release();
    if (pD3D) pD3D->Release();
    ZTexture::Free();
    ScreenQuad::Free();
}

void RenderAmbient() {
    D3DXMATRIX worldTransform = GetCameraTransform();

    // Clear states
	pd3dDevice->SetRenderState(D3DRS_LIGHTING, TRUE);
	pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	pd3dDevice->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
	pd3dDevice->SetRenderState(D3DRS_STENCILENABLE, FALSE);
	pd3dDevice->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED|D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_BLUE);

    // shift scene a little to prevent z-fighting
	pd3dDevice->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, F2DW(0.01f));
	pd3dDevice->SetRenderState(D3DRS_DEPTHBIAS, F2DW(1e-5f));

	// Render
    for(int i=0; i<meshes.size(); i++)
        meshes[i].RenderAmbient(worldTransform);

    // Draw white lights spheres
	pd3dDevice->SetRenderState( D3DRS_LIGHTING,	FALSE );
    for(int i=0; i<nLights; ++i) {
        D3DXMATRIX transform;
        D3DXMATRIX scaling;

        D3DXMatrixTranslation(&transform, lights[i].position.x, lights[i].position.y, lights[i].position.z);
        D3DXMatrixScaling(&scaling, lights[i].radius, lights[i].radius, lights[i].radius);
        D3DXMatrixMultiply(&transform, &scaling, &transform);

        lightMesh.SetTransform(transform);
        lightMesh.RenderAmbient(worldTransform);
    }

}

void RenderZFill() {
    D3DXMATRIX  worldTransform = GetCameraTransform();
    UINT        uPasses;

    ZTexture::Instance()->SetAsTarget();
   
    pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_COLORVALUE(0.0f, 0.0f, 0.0f, 1.0f), 1.0f, 0);
    pd3dDevice->BeginScene();

    // Render
    pLightingEffect->SetTechnique("ZFill");
    pLightingEffect->Begin(&uPasses, 0);
    for(int i =0; i<meshes.size(); ++i)
        meshes[i].RenderZF(worldTransform);
    pLightingEffect->End();
    pd3dDevice->EndScene();
    ZTexture::Instance()->RestoreTarget();
}

void RenderLightened(const Light& light) {
    D3DXMATRIX  worldTransform = GetCameraTransform();
    UINT        uPasses;

    // shadow
    pLightingEffect->SetTechnique("Shadow");
    
    pLightingEffect->Begin(&uPasses, 0);
    for(int i = 0; i<meshes.size(); ++i) {
        if (meshes[i].IsClosed()) {
            meshes[i].ComputeShadowVolumes(light);
            meshes[i].SetShadowConstants(worldTransform, light);
            meshes[i].RenderUmbra(0);
            meshes[i].RenderPenumbra(1);
        }
    }
	pLightingEffect->End();

    // Draw penumra cone
    if (showPenumbraCone) {
        pLightingEffect->SetTechnique("ShowPenumbraCone");
        pLightingEffect->Begin(&uPasses, 0);
        for(int i = 0; i<meshes.size(); i++) {
            if (meshes[i].IsClosed()) {
                meshes[i].SetShadowConstants(worldTransform, light);
                meshes[i].RenderUmbra(0);
            }
        }
        pLightingEffect->End();
    }

    // Add lightened
    pLightingEffect->SetTechnique("Lighting");
    pLightingEffect->Begin(&uPasses, 0);
    for(int i=0; i<meshes.size(); i++)
    {
        meshes[i].Render(worldTransform, light);
        meshes[i].RenderTextured(worldTransform, light);
    }
    pLightingEffect->End();
}

void ClearStencilAlpha() {
    UINT uPasses;
    pLightingEffect->SetTechnique("ClearStencilAlpha");
    pLightingEffect->Begin(&uPasses, 0);
    ScreenQuad::Instance()->Render();
    pLightingEffect->End();
}

void Render(void) {
    RenderZFill();
    pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, D3DCOLOR_COLORVALUE(0.0f, 0.0f, 0.0f, 1.0f), 1.0f, 0);
    pd3dDevice->BeginScene();

    // Ambient part
    RenderAmbient();

    // Lightened part
    // Add lightened component
    for(int i = 0; i<nLights; i++) {
        ClearStencilAlpha();
        RenderLightened(lights[i]);
    }
    pd3dDevice->EndScene();
    pd3dDevice->Present(NULL, NULL, NULL, NULL);
}

void Update(void) {
    D3DXMATRIX  rotY;
    float       time = GetTime();

    ++framesLeft; 
    if (time - lastTime < 0.01) 
        return;

    // fps  
    fps = framesLeft / (time - lastTime);
    framesLeft = 0;

    // Animate scene
    if (animate) {
        D3DXMatrixRotationY(&rotY, 0.2f * (time - lastTime));
        meshes[DYNAMIC_OBJ].Transform(rotY);

		D3DXMatrixRotationY(&rotY,-1.0f * (time - lastTime));
		meshes[STATIC_OBJ].Transform(rotY);
    }

    lastTime = GetTime();
}