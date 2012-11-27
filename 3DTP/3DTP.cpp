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

// FVF_VERTEX DEFINE
#define FVF_VERTEX    D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_TEX1

// Z_NEAR, Z_FAR
float	ZNEAR	= 1.0f;
float	ZFAR	= 1000.f;

// WINDOW HEIGHT & WIDTH
UINT	WIDTH	= 1024;//640;
UINT	HEIGHT	= 768;//480;

// TEXTURES
LPDIRECT3DTEXTURE9	ppTextMoon;
LPDIRECT3DTEXTURE9	ppTextEarth;
LPDIRECT3DTEXTURE9	ppTextSun;

// SHADER
LPD3DXEFFECT	pEffect;

// HANDLES
D3DXHANDLE	hWorldViewProj;
D3DXHANDLE	hDiffuseMap;

// VERTEX DECLARATION
D3DVERTEXELEMENT9 dwDecl3[] =
{
    { 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
    { 0, 12, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 }, 
    { 0, 20, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0 },
	D3DDECL_END()
};

// VERTEX DECLARATION
IDirect3DVertexDeclaration9* ppDecl;

////////////////////////////////////////////// forward declarations
bool initWindow(HINSTANCE hInstance);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LPD3DXMESH CreateMappedSphere(LPDIRECT3DDEVICE9, float, UINT, UINT);

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

	D3DXCreateTextureFromFile(pd3dDevice, L"./moon.jpg", &ppTextMoon);
	D3DXCreateTextureFromFile(pd3dDevice, L"./earth.jpg", &ppTextEarth);
	D3DXCreateTextureFromFile(pd3dDevice, L"./sun.jpg", &ppTextSun);

	LPD3DXBUFFER compilationErrors;
	LPCWSTR pSrcFile = L"./effect.fx";

	if (D3D_OK != D3DXCreateEffectFromFile(pd3dDevice, pSrcFile, NULL, NULL, 0, NULL, &pEffect, &compilationErrors))
	{
		MessageBoxA (NULL, (char *) compilationErrors->GetBufferPointer(), "Error", 0);
	}
	
	hWorldViewProj = pEffect->GetParameterByName(NULL, "WorldViewProj");
	hDiffuseMap = pEffect->GetParameterByName(NULL, "DiffuseMap");

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
                    cameraPosition.y += 1.0f;
                    cameraLook.y += 1.0f;
                    break;

                // MOVE DOWN
                case VK_NUMPAD3:
                    cameraPosition.y -= 1.0f;
                    cameraLook.y -= 1.0f;
                    break;

                // MOVE LEFT
                case VK_NUMPAD4:
                    cameraPosition.x -= 1.0f;
                    cameraLook.x -= 1.0f;
                    break;

                // MOVE RIGHT
                case VK_NUMPAD6:
                    cameraPosition.x += 1.0f;
                    cameraLook.x += 1.0f;
                    break;

				//ZOOM IN
                case VK_NUMPAD8:
                    cameraPosition.z += 1.0f;
                    cameraLook.z += 1.0f;
                    break;
   
				//ZOOM OUT
                case VK_NUMPAD2:
                    cameraPosition.z -= 1.0f;
                    cameraLook.z -= 1.0f;
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
    if(FAILED(pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_REF, wndHandle, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pd3dDevice)))
        return false;

    //set rendering state
    //pd3dDevice->SetRenderState(D3DRS_AMBIENT, D3DCOLOR_XRGB(255, 255, 255));
	pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
	pd3dDevice->SetRenderState(D3DRS_WRAP0, D3DWRAPCOORD_0);
    //pd3dDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);

    //initialize camera variables
    moveCamera(D3DXVECTOR3(0.0f, 0.0f, -400.0f));
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

    

    pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);

    pd3dDevice->BeginScene();

//---------------Draw Sun-------------------
   
    // set the rotation for the sun
    D3DXMatrixRotationY(&meshRotate, D3DXToRadian(angle++));

    //set the rotation for the moon
    D3DXMatrixRotationY(&meshRotate2, D3DXToRadian(angle + 30));

	WorldViewProj = meshRotate * matView * matProj;
	pEffect->SetMatrix(hWorldViewProj, &WorldViewProj);
	pEffect->SetTexture(hDiffuseMap, ppTextSun);

	pSunMesh->GetVertexBuffer(&ppVertexBuffer);
	pSunMesh->GetIndexBuffer(&ppIndexBuffer);

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

//-----------------Draw Planet--------------------
   
    //set the translation for the planet
    D3DXMatrixTranslation(&meshTranslate, 100, 0, 0);

    // multiply the scaling and rotation matrices to create the meshMat matrix
    //D3DXMatrixMultiply(&meshMat, &meshTranslate, &meshRotate);
    meshMat = meshTranslate * meshRotate;

	WorldViewProj = meshMat * matView * matProj;
	pEffect->SetMatrix(hWorldViewProj, &WorldViewProj);
	pEffect->SetTexture(hDiffuseMap, ppTextEarth);

	pPlanetMesh->GetVertexBuffer(&ppVertexBuffer);
	pPlanetMesh->GetIndexBuffer(&ppIndexBuffer);

	pEffect->Begin(&cPasses, 0);
	for (iPass = 0; iPass < cPasses; ++iPass)
	{
		pEffect->BeginPass(iPass);
		pEffect->CommitChanges();
		pd3dDevice->SetStreamSource(0, ppVertexBuffer, 0, sizeof(VERTEX));
		pd3dDevice->SetIndices(ppIndexBuffer);
		pd3dDevice->SetVertexDeclaration(ppDecl);
		pd3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, pPlanetMesh->GetNumVertices(), 0, pPlanetMesh->GetNumFaces());
		pEffect->EndPass();
	}
	pEffect->End();

//---------------------Draw Moon----------------------

    //set moon at 30 from origin for first rotatation
    D3DXMatrixTranslation(&meshTranslate, 50, 0, 0);

    //translate moon to planet for second, faster rotation
    D3DXMatrixTranslation(&meshTranslate2, 100, 0, 0);

    //multiply moon's matrices together
    meshMat = meshTranslate * meshRotate2 * meshTranslate2 * meshRotate;

	WorldViewProj = meshMat * matView * matProj;
	pEffect->SetMatrix(hWorldViewProj, &WorldViewProj);
	pEffect->SetTexture(hDiffuseMap, ppTextMoon);

	pMoonMesh->GetVertexBuffer(&ppVertexBuffer);
	pMoonMesh->GetIndexBuffer(&ppIndexBuffer);

	pEffect->Begin(&cPasses, 0);
	for (iPass = 0; iPass < cPasses; ++iPass)
	{
		pEffect->BeginPass(iPass);
		pEffect->CommitChanges();
		pd3dDevice->SetStreamSource(0, ppVertexBuffer, 0, sizeof(VERTEX));
		pd3dDevice->SetIndices(ppIndexBuffer);
		pd3dDevice->SetVertexDeclaration(ppDecl);
		pd3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, pMoonMesh->GetNumVertices(), 0, pMoonMesh->GetNumFaces());
		pEffect->EndPass();
	}
	pEffect->End();

    pd3dDevice->EndScene();

    // Present the backbuffer contents to the display
    pd3dDevice->Present(NULL, NULL, NULL, NULL);

    // Free up or release the resources allocated by CreateSphere
    /*pSunMesh->Release();
    pPlanetMesh->Release();
    pMoonMesh->Release();*/
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

    // lock the vertex buffer
    LPVERTEX pVerts;
    if (SUCCEEDED(texMesh->LockVertexBuffer(0, (VOID **) &pVerts)))
	{
        // get vertex count
        int numVerts = texMesh->GetNumVertices();

        // loop through the vertices
        /*for (int i = 0; i < numVerts; i++)
		{
			// calculate normal
			D3DXVec3Normalize(&pVerts->norm, &pVerts->pos);
            // calculate texture coordinates
			pVerts->tu = 0.5f - (atan2(pVerts->norm.z, pVerts->norm.x)/(2 * D3DX_PI));//asinf(pVerts->norm.x) / D3DX_PI + 0.5f;
			pVerts->tv = 0.5f - (2.0f * (asin(pVerts->norm.y)/(2 * D3DX_PI)));//asinf(pVerts->norm.y) / D3DX_PI + 0.5f;
            // go to next vertex
            pVerts++;
        }*/
        // unlock the vertex buffer
        texMesh->UnlockVertexBuffer();
    }
    
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
	if (ppTextSun != NULL)
		ppTextSun->Release();
	if (ppTextMoon != NULL)
		ppTextMoon->Release();
	if (ppTextEarth != NULL)
		ppTextEarth->Release();

    // release the device and the direct3D object
    if (pd3dDevice != NULL)
        pd3dDevice->Release();
    if (pD3D != NULL)
        pD3D->Release();
}