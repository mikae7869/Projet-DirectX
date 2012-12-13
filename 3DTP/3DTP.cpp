#include "stdafx.h"
#include "3DTP.h"

// global variables
HINSTANCE	hInst;					// application instance
HWND		wndHandle;				// application window handle

LPDIRECT3D9			pD3D;			// the Direct3D Object
LPDIRECT3DDEVICE9	pd3dDevice;		// the Direct3D Device

LPD3DXMESH		pSunMesh;			// holds the newly created sun mesh
LPD3DXMESH		pPlanetMesh;		// holds the planet mesh (orbits the sun)   
LPD3DXMESH		pMoonMesh;			// holds the moon mesh (orbits the planet)

D3DXMATERIAL	material;			// holds the material for the mesh

// camera variables
D3DXMATRIX		matView;			// the view matrix
D3DXMATRIX		matProj;			// the projection matrix
D3DXVECTOR3		cameraPosition;		// the position of the camera
D3DXVECTOR3		cameraLook;			// where the camera is pointing

// VERTEX STRUCTURE
typedef struct _VERTEX
{
    D3DXVECTOR3	pos;		// vertex position
    D3DXVECTOR3	norm;		// vertex normal
    float		tu;			// texture coordinates
    float		tv;
} VERTEX, *LPVERTEX;

// Screen quad vertex format
typedef struct _SCREENVERTEX
{
    D3DXVECTOR4 p; // position
    D3DXVECTOR2 t; // texture coordinate
} SCREENVERTEX;

// Z_NEAR, Z_FAR
float	ZNEAR	= 1.0f;
float	ZFAR	= 600.f;

// WINDOW HEIGHT & WIDTH
UINT	WIDTH	= 1024;
UINT	HEIGHT	= 768;

// TEXTURES
LPDIRECT3DTEXTURE9	ppTextEarth;
LPDIRECT3DTEXTURE9	ppRenderTexture = NULL;

// SURFACES
LPDIRECT3DSURFACE9	ppRenderSurface = NULL;
LPDIRECT3DSURFACE9	ppBackBuffer = NULL;

// SHADER
LPD3DXEFFECT	pEffect;

// HANDLES
D3DXHANDLE	hWorldViewProj;
D3DXHANDLE	hDiffuseMap;
D3DXHANDLE	hLightPosition;
D3DXHANDLE	hLightAmbiantColor;
D3DXHANDLE	hLightDiffuseColor;
D3DXHANDLE	hLightSpecularColor;
D3DXHANDLE	hLightDistance;
D3DXHANDLE	hCameraPos;

// VERTEX DECLARATION
D3DVERTEXELEMENT9 dwDecl3[] =
{
    { 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
    { 0, 12, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 }, 
    { 0, 20, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0 },
	D3DDECL_END()
};

D3DVERTEXELEMENT9 screenDecl2[] =
{
    { 0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
    { 0, 16, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
	D3DDECL_END()
};

// VERTEX DECLARATION
IDirect3DVertexDeclaration9*	ppDecl;
IDirect3DVertexDeclaration9*	ppScreenDecl;

////////////////////////////////////////////// forward declarations
bool initWindow(HINSTANCE hInstance);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LPD3DXMESH CreateMappedSphere(LPDIRECT3DDEVICE9, float, UINT, UINT);

void DrawFullScreenQuad( float fLeftU, float fTopV, float fRightU, float fBottomV );

// DirectX functions
bool initDirect3D();
void render(void);
void Release(void);

//camera functions
void createCamera(float, float);
void moveCamera(D3DXVECTOR3);
void pointCamera(D3DXVECTOR3);


/*********************************************************************
* WinMain
*********************************************************************/
int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, char *lpCmdLine, int nCmdShow)
{
    if (!initWindow(hInstance))
    {
        MessageBox(NULL, L"Unable to create window", L"ERROR", MB_OK);
        return false;
    }

    if (!initDirect3D())
    {
        MessageBox(NULL, L"Unable to init Direct3D", L"ERROR", MB_OK);
        return false;
    }

    // Main message loop:
    MSG msg;
    ZeroMemory( &msg, sizeof(msg) );

	pd3dDevice->CreateVertexDeclaration(dwDecl3, &ppDecl);
	pd3dDevice->CreateVertexDeclaration(screenDecl2, &ppScreenDecl);

	// Get Back Buffer
	pd3dDevice->GetRenderTarget(0, &ppBackBuffer);

	// Create Render Texture
	pd3dDevice->CreateTexture(WIDTH, HEIGHT, 1, D3DUSAGE_RENDERTARGET, D3DFMT_R5G6B5, D3DPOOL_DEFAULT, &ppRenderTexture, NULL);

	// Get Surface Texture
	ppRenderTexture->GetSurfaceLevel(0, &ppRenderSurface);

	D3DXCreateTextureFromFile(pd3dDevice, L"./earth.jpg", &ppTextEarth);

	LPD3DXBUFFER compilationErrors;
	LPCWSTR pSrcFile = L"./effect.fx";

	if (D3D_OK != D3DXCreateEffectFromFile(pd3dDevice, pSrcFile, NULL, NULL, 0, NULL, &pEffect, &compilationErrors))
	{
		MessageBoxA (NULL, (char *) compilationErrors->GetBufferPointer(), "Error", 0);
	}
	
	hWorldViewProj = pEffect->GetParameterByName(NULL, "WorldViewProj");
	hDiffuseMap = pEffect->GetParameterByName(NULL, "DiffuseMap");
	hCameraPos = pEffect->GetParameterByName(NULL, "CameraPos");
	hLightPosition = pEffect->GetParameterByName(NULL, "LightPosition");
	hLightAmbiantColor = pEffect->GetParameterByName(NULL, "LightAmbiantColor");
	hLightDiffuseColor = pEffect->GetParameterByName(NULL, "LightDiffuseColor");
	hLightSpecularColor = pEffect->GetParameterByName(NULL, "LightSpecularColor");
	hLightDistance = pEffect->GetParameterByName(NULL, "LightDistance");
	
	pEffect->SetVector(hLightSpecularColor, &D3DXVECTOR4(1.0f, 1.0f, 1.0f, 1.0f)); // Set specular light color
	pEffect->SetVector(hLightAmbiantColor, &D3DXVECTOR4(0.15f, 0.15f, 0.15f, 1.0f)); // Set ambiant light color
	pEffect->SetVector(hLightDiffuseColor, &D3DXVECTOR4(1.0f, 1.0f, 1.0f, 1.0f)); // Set diffuse light color
	pEffect->SetVector(hLightPosition, &D3DXVECTOR4(0.0f, 0.0f, 100.0f, 1.0f)); // Set light position
	pEffect->SetFloat(hLightDistance, 600.0f); // Set light propagation distance

	// create the three sphere meshes
	pSunMesh	=	CreateMappedSphere(pd3dDevice, 40, 20, 20);
    pPlanetMesh	=	CreateMappedSphere(pd3dDevice, 20, 20, 20);
    pMoonMesh	=	CreateMappedSphere(pd3dDevice, 10, 20, 20);
    
	while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }       
        else
        {
            render();
        }
    }
	Release();
       
    return (int) msg.wParam;
}

/*********************************************************************
* initWindow
*********************************************************************/
bool initWindow(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style				= CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc		= (WNDPROC)WndProc;
    wcex.cbClsExtra			= 0;
    wcex.cbWndExtra			= 0;
    wcex.hInstance			= hInstance;
    wcex.hIcon				= 0;
    wcex.hCursor			= LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground		= (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName		= NULL;
    wcex.lpszClassName		= L"DirectXExample";
    wcex.hIconSm			= 0;
    RegisterClassEx(&wcex);

    // create the window
    wndHandle = CreateWindow(L"DirectXExample",
                             L"Project DirectX ",
                             WS_OVERLAPPEDWINDOW,
                             CW_USEDEFAULT,
                             CW_USEDEFAULT,
                             WIDTH,
                             HEIGHT,
                             NULL,
                             NULL,
                             hInstance,
                             NULL);
   if (!wndHandle)
      return false;
  
   ShowWindow(wndHandle, SW_SHOW);
   UpdateWindow(wndHandle);

   return true;
}

/*********************************************************************
* WndProc
*********************************************************************/
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
       
    // handle keyboard events
        case WM_KEYDOWN:
            // switch on which key was pressed
            switch( wParam )
            {
                // MOVE UP
                case VK_NUMPAD9:
                    cameraPosition.y += 10.0f;
                    cameraLook.y += 10.0f;
                    break;

                // MOVE DOWN
                case VK_NUMPAD3:
                    cameraPosition.y -= 10.0f;
                    cameraLook.y -= 10.0f;
                    break;

                // MOVE LEFT
                case VK_NUMPAD4:
                    cameraPosition.x -= 10.0f;
                    cameraLook.x -= 10.0f;
                    break;

                // MOVE RIGHT
                case VK_NUMPAD6:
                    cameraPosition.x += 10.0f;
                    cameraLook.x += 10.0f;
                    break;

				//ZOOM IN
                case VK_NUMPAD8:
                    cameraPosition.z += 10.0f;
                    cameraLook.z += 10.0f;
                    break;
   
				//ZOOM OUT
                case VK_NUMPAD2:
                    cameraPosition.z -= 10.0f;
                    cameraLook.z -= 10.0f;
                    break;
            }
        break;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

/*********************************************************************
* initDirect3D
* initializes direct3D
*********************************************************************/
bool initDirect3D()
{
    pD3D = NULL;
    pd3dDevice = NULL;

    // create the directX object
    if (NULL == (pD3D = Direct3DCreate9(D3D_SDK_VERSION)))
        return false;

    // fill the presentation parameters structure
    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.Windowed			= TRUE;
    d3dpp.SwapEffect		= D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat	= D3DFMT_UNKNOWN;
    d3dpp.BackBufferCount	= 1;
    d3dpp.BackBufferHeight	= HEIGHT;
    d3dpp.BackBufferWidth	= WIDTH;
    d3dpp.hDeviceWindow		= wndHandle;

    // create a default directx device
	if(FAILED(pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_REF, wndHandle, D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &pd3dDevice)))
		return false;

    //set rendering state
    //pd3dDevice->SetRenderState(D3DRS_AMBIENT, D3DCOLOR_XRGB(255, 255, 255));
	//pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
	pd3dDevice->SetRenderState(D3DRS_WRAP0, D3DWRAPCOORD_0);
    //pd3dDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);

    //initialize camera variables
    moveCamera(D3DXVECTOR3(0.0f, 0.0f, -100.0f));
    pointCamera(D3DXVECTOR3(0.0f, 0.0f, 0.0f));
   
    return true;
}

/*********************************************************************
* render
*********************************************************************/
void render(void)
{
    // check to make sure we have a valid Direct3D Device
    if( NULL == pd3dDevice )
        return;

    // make the angle variable so we can change it each frame
    static int angle = 0;
    D3DXMATRIX WorldViewProj, meshMat, meshRotate, meshRotate2, meshTranslate, meshTranslate2;
	unsigned int cPasses, iPass;
	LPDIRECT3DVERTEXBUFFER9 ppVertexBuffer;
	LPDIRECT3DINDEXBUFFER9 ppIndexBuffer;

    createCamera(ZNEAR, ZFAR);  // near clip plane, far clip plane
    pointCamera(cameraLook);

	// Set Render Target the Render Scene
	pd3dDevice->SetRenderTarget(0, ppRenderSurface);

    pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);

    pd3dDevice->BeginScene();

//---------------Draw Earth-------------------
   
    // set the rotation for the sun
	D3DXMatrixRotationX(&meshRotate, D3DXToRadian(-90.0f));
    D3DXMatrixRotationY(&meshRotate2, D3DXToRadian(angle++));

    //set the rotation for the moon
    //D3DXMatrixRotationY(&meshRotate2, D3DXToRadian(angle + 30));

	WorldViewProj = meshRotate * meshRotate2 * matView * matProj;

	pEffect->SetMatrix(hWorldViewProj, &WorldViewProj);
	pEffect->SetTexture(hDiffuseMap, ppTextEarth);
	pEffect->SetVector(hCameraPos , &D3DXVECTOR4(cameraPosition, 1.0f));

	pSunMesh->GetVertexBuffer(&ppVertexBuffer);
	pSunMesh->GetIndexBuffer(&ppIndexBuffer);

	pEffect->SetTechnique("diffuse");
	pEffect->Begin(&cPasses, 0);
	for (iPass = 0; iPass < cPasses; ++iPass)
	{
		pEffect->BeginPass(iPass);
		pEffect->CommitChanges();
		pd3dDevice->SetStreamSource(0, ppVertexBuffer, 0, sizeof(VERTEX));
		pd3dDevice->SetIndices(ppIndexBuffer);
		pd3dDevice->SetVertexDeclaration(ppDecl);
		pd3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, pSunMesh->GetNumVertices(), 0, pSunMesh->GetNumFaces());
		pEffect->EndPass();
	}
	pEffect->End();

    pd3dDevice->EndScene();

	pd3dDevice->SetRenderTarget(0, ppBackBuffer);
	pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);

	pd3dDevice->BeginScene();
	
	pEffect->SetTechnique("final");
	pEffect->SetTexture(hDiffuseMap, ppTextEarth);
	//pEffect->SetTexture(hDiffuseMap, ppRenderTexture);
	pEffect->Begin(&cPasses, 0);
	for (iPass = 0; iPass < cPasses; ++iPass)
	{
		pEffect->BeginPass(iPass);
		pEffect->CommitChanges();
		pd3dDevice->SetVertexDeclaration(ppScreenDecl);
		DrawFullScreenQuad(0.0f, 0.0f, 1.0f, 1.0f);
		pEffect->EndPass();
	}
	pEffect->End();
	pd3dDevice->EndScene();

    // Present the backbuffer contents to the display
    pd3dDevice->Present(NULL, NULL, NULL, NULL);

    // Free up or release the resources allocated by CreateSphere
	ppVertexBuffer->Release();
	ppIndexBuffer->Release();
}

/*************************************************************************
* createCamera
* creates a virtual camera
*************************************************************************/
void createCamera(float nearClip, float farClip)
{
    //Here we specify the field of view, aspect ration and near and far clipping planes.
    D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI/4, float(WIDTH)/HEIGHT, nearClip, farClip);
}

/*************************************************************************
* moveCamera
* moves the camera to a position specified by the vector passed as a
* parameter
*************************************************************************/
void moveCamera(D3DXVECTOR3 vec)
{
   cameraPosition = vec;
}

/*************************************************************************
* pointCamera
* points the camera a location specified by the passed vector
*************************************************************************/
void pointCamera(D3DXVECTOR3 vec)
{
    cameraLook = vec;
    D3DXMatrixLookAtLH(&matView, &cameraPosition, &cameraLook, &D3DXVECTOR3(0.0f, 1.0f, 0.0f));
}

/*************************************************************************
* CreateMappedSphere
* Create a Mapped Sphere (i.e. Precalculate coordinate for further texture application)
*************************************************************************/
LPD3DXMESH CreateMappedSphere(LPDIRECT3DDEVICE9 pDev, float fRad, UINT slices, UINT stacks)
{
    // create the sphere
    LPD3DXMESH mesh;
    if (FAILED(D3DXCreateSphere(pDev, fRad, slices, stacks, &mesh, NULL)))
        return NULL;

    // create a copy of the mesh with texture coordinates,
    // since the D3DX function doesn't include them
    LPD3DXMESH texMesh;
	//if (FAILED(mesh->CloneMeshFVF(D3DXMESH_SYSTEMMEM, FVF_VERTEX, pDev, &texMesh)))
	if (FAILED(mesh->CloneMesh(D3DXMESH_SYSTEMMEM, dwDecl3, pDev, &texMesh)))
        // failed, return un-textured mesh
        return mesh;

    // finished with the original mesh, release it
    mesh->Release();

	// Calculating normals for each vertex
	D3DXComputeNormals(texMesh, NULL);

    // return pointer to caller
    return texMesh;
}


/*************************************************************************
* Release
* Release all allocated ressources
*************************************************************************/
void Release(void)
{
	// release meshes
	if (pSunMesh != NULL)
		pSunMesh->Release();
    if (pPlanetMesh != NULL)
		pPlanetMesh->Release();
	if (pMoonMesh != NULL)
		pMoonMesh->Release();

	// release shader
	if (pEffect != NULL)
		pEffect->Release();

	// release textures
	if (ppTextEarth != NULL)
		ppTextEarth->Release();

    // release the device and the direct3D object
    if (pd3dDevice != NULL)
        pd3dDevice->Release();
    if (pD3D != NULL)
        pD3D->Release();
}

void DrawFullScreenQuad( float fLeftU, float fTopV, float fRightU, float fBottomV )
{
    D3DSURFACE_DESC dtdsdRT;
    PDIRECT3DSURFACE9 pSurfRT;

    // Acquire render target width and height
    pd3dDevice->GetRenderTarget(0, &pSurfRT);
    pSurfRT->GetDesc(&dtdsdRT);
    pSurfRT->Release();

    // Ensure that we're directly mapping texels to pixels by offset by 0.5
    // For more info see the doc page titled "Directly Mapping Texels to Pixels"
    FLOAT fWidth5 = (FLOAT)dtdsdRT.Width - 0.5f;
    FLOAT fHeight5 = (FLOAT)dtdsdRT.Height - 0.5f;

    // Draw the quad
    SCREENVERTEX svQuad[4];

    svQuad[0].p = D3DXVECTOR4( -0.5f, -0.5f, 0.5f, 1.0f );
	svQuad[0].p = D3DXVECTOR4( -1.0f, -1.0f, 0.5f, 1.0f );
    svQuad[0].t = D3DXVECTOR2( fLeftU, fTopV );

    svQuad[1].p = D3DXVECTOR4( fWidth5, -0.5f, 0.5f, 1.0f );
	svQuad[1].p = D3DXVECTOR4( -1.0f, 1.0f, 0.5f, 1.0f );
    svQuad[1].t = D3DXVECTOR2( fRightU, fTopV );

    svQuad[2].p = D3DXVECTOR4( -0.5f, fHeight5, 0.5f, 1.0f );
	svQuad[2].p = D3DXVECTOR4( 1.0f, -1.0f, 0.5f, 1.0f );
    svQuad[2].t = D3DXVECTOR2( fLeftU, fBottomV );

    svQuad[3].p = D3DXVECTOR4( fWidth5, fHeight5, 0.5f, 1.0f );
	svQuad[3].p = D3DXVECTOR4( 1.0f, 1.0f, 0.5f, 1.0f );
    svQuad[3].t = D3DXVECTOR2( fRightU, fBottomV );

    //pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
	
	LPDIRECT3DVERTEXBUFFER9	ppScreenVertexBuffer;
	LPDIRECT3DINDEXBUFFER9 ppIndexBuffer;
	VOID*					pVoid;

	pd3dDevice->CreateVertexBuffer(sizeof(svQuad), 0, 0, D3DPOOL_MANAGED, &ppScreenVertexBuffer, NULL);
	ppScreenVertexBuffer->Lock(0, sizeof(svQuad), (void**)&pVoid, 0);    // lock the vertex buffer
	memcpy(pVoid, svQuad, sizeof(svQuad));    // copy the vertices to the locked buffer
	ppScreenVertexBuffer->Unlock();    // unlock the vertex buffer
	short indices[] =
	{
		0,1,3, 3,2,0
	};
	pd3dDevice->CreateIndexBuffer(sizeof(indices), D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_MANAGED, &ppIndexBuffer, NULL);
	ppIndexBuffer->Lock(0, sizeof(indices), (void**)&pVoid, 0);
	memcpy(pVoid, indices, sizeof(indices));
	ppIndexBuffer->Unlock();

	pd3dDevice->SetStreamSource(0, ppScreenVertexBuffer, 0, sizeof(SCREENVERTEX));
	pd3dDevice->SetIndices(ppIndexBuffer);
	pd3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 4, 0, 2);
	ppScreenVertexBuffer->Release();
    //pd3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, svQuad, sizeof(SCREENVERTEX));
    //pd3dDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
}