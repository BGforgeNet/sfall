/*
 *    sfall
 *    Copyright (C) 2008, 2009, 2010, 2012  The sfall team
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "main.h"
#include "FalloutEngine.h"
#include "InputFuncs.h"
#include "LoadGameHook.h"

#include "Graphics.h"

#define UNUSEDFUNCTION { return DDERR_GENERIC; }
#define SAFERELEASE(a) { if (a) { a->Release(); a = nullptr; } }

typedef HRESULT (__stdcall *DDrawCreateProc)(void*, IDirectDraw**, void*);
//typedef IDirect3D9* (__stdcall *D3DCreateProc)(UINT version);

static const char* gpuEffectA8 =
	"texture image;"
	"texture palette;"
	"texture head;"
	"sampler s0 = sampler_state { texture=<image>; };"
	"sampler s1 = sampler_state { texture=<palette>; minFilter=none; magFilter=none; addressU=clamp; addressV=clamp; };"
	"sampler s2 = sampler_state { texture=<head>; minFilter=linear; magFilter=linear; addressU=clamp; addressV=clamp; };"
	"float2 size;"
	"float2 corner;"
	// shader for displaying head textures
	"float4 P1( in float2 Tex : TEXCOORD0 ) : COLOR0 {"
	  "float backdrop = tex2D(s0, Tex).a;"
	  "float3 result;"
	  "if (abs(backdrop - 1.0) < 0.001) {" // (48.0 / 255.0) // 48 - key index color
	    "result = tex2D(s2, saturate((Tex - corner) / size));"
	  "} else {"
	    "result = tex1D(s1, backdrop).bgr;" // get color in palette and swap R <> B
	  "}"
	  "return float4(result, 1);"
	"}"
	"technique T1"
	"{"
	  "pass p1 { PixelShader = compile ps_2_0 P1(); }"
	"}"

	// main shader
	"float4 P0( in float2 Tex : TEXCOORD0 ) : COLOR0 {"
	  "float3 result = tex1D(s1, tex2D(s0, Tex).a);" // get color in palette
	  "return float4(result.bgr, 1);"                // swap R <> B
	"}"
	"technique T0"
	"{"
	  "pass p0 { PixelShader = compile ps_2_0 P0(); }"
	"}";

static const char* gpuEffectL8 =
	"texture image;"
	"texture palette;"
	"texture head;"
	"sampler s0 = sampler_state { texture=<image>; };"
	"sampler s1 = sampler_state { texture=<palette>; minFilter=none; magFilter=none; addressU=clamp; addressV=clamp; };"
	"sampler s2 = sampler_state { texture=<head>; minFilter=linear; magFilter=linear; addressU=clamp; addressV=clamp; };"
	"float2 size;"
	"float2 corner;"
	// shader for displaying head textures
	"float4 P1( in float2 Tex : TEXCOORD0 ) : COLOR0 {"
	  "float backdrop = tex2D(s0, Tex).r;"
	  "float3 result;"
	  "if (abs(backdrop - 1.0) < 0.001) {"
	    "result = tex2D(s2, saturate((Tex - corner) / size));"
	  "} else {"
	    "result = tex1D(s1, backdrop).bgr;"
	  "}"
	  "return float4(result, 1);"
	"}"
	"technique T1"
	"{"
	  "pass p1 { PixelShader = compile ps_2_0 P1(); }"
	"}"

	// main shader
	"float4 P0( in float2 Tex : TEXCOORD0 ) : COLOR0 {"
	  "float3 result = tex1D(s1, tex2D(s0, Tex).r);"
	  "return float4(result.bgr, 1);"
	"}"
	"technique T0"
	"{"
	  "pass p0 { PixelShader = compile ps_2_0 P0(); }"
	"}";

static IDirectDrawSurface* primaryDDSurface = nullptr; // aka _GNW95_DDPrimarySurface

static DWORD ResWidth;
static DWORD ResHeight;

DWORD GPUBlt;
DWORD GraphicsMode;

bool Gfx_PlayAviMovie = false;
bool Gfx_AviMovieWidthFit = false;

static DWORD yoffset;
//static DWORD xoffset;

static bool DeviceLost = false;
static bool mainTexLock = false;
static char textureFilter; // 1 - auto, 2 - force

static DDSURFACEDESC surfaceDesc;
static DDSURFACEDESC mveDesc;
static D3DSURFACE_DESC movieDesc;

#pragma pack(push, 1)
static struct PALCOLOR {
	union {
		DWORD xRGB;
		struct {
			BYTE B;
			BYTE G;
			BYTE R;
		};
	};
} palette[256];
#pragma pack(pop)

//static bool paletteInit = false;

static DWORD gWidth;
static DWORD gHeight;

static long moveWindowKey[2];

static bool windowInit = false;
static DWORD windowLeft = 0;
static DWORD windowTop = 0;
static HWND window;
static DWORD windowStyle = WS_CAPTION | WS_BORDER | WS_MINIMIZEBOX;

static int windowData;

static DWORD ShaderVersion;

IDirect3D9* d3d9 = 0;
IDirect3DDevice9* d3d9Device = 0;

static IDirect3DTexture9* mainTex = 0;
static IDirect3DTexture9* sTex1 = 0;
static IDirect3DTexture9* sTex2 = 0;
static IDirect3DTexture9* movieTex;

static IDirect3DSurface9* sSurf1 = 0;
static IDirect3DSurface9* sSurf2 = 0;
static IDirect3DSurface9* backBuffer = 0;

static IDirect3DVertexBuffer9* vertexOrigRes;
static IDirect3DVertexBuffer9* vertexSfallRes;
static IDirect3DVertexBuffer9* vertexMovie;

static IDirect3DTexture9* gpuPalette;
static ID3DXEffect* gpuBltEffect;

static D3DXHANDLE gpuBltMainTex;
static D3DXHANDLE gpuBltPalette;
static D3DXHANDLE gpuBltHead;
static D3DXHANDLE gpuBltHeadSize;
static D3DXHANDLE gpuBltHeadCorner;

static float rcpres[2];

static size_t shadersSize;
static bool globalShadersActive = false;

struct sGlobalShader {
	std::string gShaderFile;
	bool badShader;

	sGlobalShader(std::string file) {
		gShaderFile = std::move(file);
		badShader = false;
	}
};

struct sShader {
	ID3DXEffect* Effect;
	D3DXHANDLE ehTicks;
	DWORD mode;
	DWORD mode2;
	bool Active;

	sShader() : Effect(0), ehTicks(0), mode(0), mode2(0), Active(false) {}
};

static std::vector<sGlobalShader> gShaderFiles;
static std::vector<sShader> shaders;
static std::vector<IDirect3DTexture9*> shaderTextures;

#define _VERTEXFORMAT D3DFVF_XYZRHW|D3DFVF_TEX1

struct VertexFormat {
	float x, y, z, w, u, v;
};

static VertexFormat ShaderVertices[] = {
	// x      y    z rhw u  v
	{-0.5,  -0.5,  0, 1, 0, 0}, // 0 - top left
	{-0.5,  479.5, 0, 1, 0, 1}, // 1 - bottom left
	{639.5, -0.5,  0, 1, 1, 0}, // 2 - top right
	{639.5, 479.5, 0, 1, 1, 1}  // 3 - bottom right
};

HWND Gfx_GetFalloutWindowInfo(RECT* rect) {
	if (rect) {
		rect->left = windowLeft;
		rect->top = windowTop;
		rect->right = gWidth;
		rect->bottom = gHeight;
	}
	return window;
}

long Gfx_GetGameWidthRes() {
	return (ptr_scr_size->offx - ptr_scr_size->x) + 1;
}

long Gfx_GetGameHeightRes() {
	return (ptr_scr_size->offy - ptr_scr_size->y) + 1;
}

int __stdcall GetShaderVersion() {
	return ShaderVersion;
}

//////////////////////////////// SCRIPT SHADERS ////////////////////////////////

void __stdcall SetShaderMode(DWORD d, DWORD mode) {
	if (d >= shadersSize || !shaders[d].Effect) return;
	if (mode & 0x80000000) {
		shaders[d].mode2 = mode ^ 0x80000000;
	} else {
		shaders[d].mode = mode;
	}
}

int __stdcall LoadShader(const char* file) {
	if (!GraphicsMode || strstr(file, "..") || strstr(file, ":")) return -1;
	char buf[MAX_PATH];
	sprintf_s(buf, "%s\\shaders\\%s", (*ptr_master_db_handle)->path, file); // *ptr_patches
	for (DWORD d = 0; d < shadersSize; d++) {
		if (!shaders[d].Effect) {
			if (FAILED(D3DXCreateEffectFromFile(d3d9Device, buf, 0, 0, 0, 0, &shaders[d].Effect, 0))) {
				return -1;
			} else {
				return d;
			}
		}
	}
	sShader shader = sShader();
	if (FAILED(D3DXCreateEffectFromFile(d3d9Device, buf, 0, 0, 0, 0, &shader.Effect, 0))) return -1;

	shader.Effect->SetFloatArray("rcpres", rcpres, 2);

	for (int i = 1; i < 128; i++) {
		const char* name;
		IDirect3DTexture9* tex;

		sprintf(buf, "texname%d", i);
		if (FAILED(shader.Effect->GetString(buf, &name))) break;
		sprintf_s(buf, "%s\\art\\stex\\%s", (*ptr_master_db_handle)->path, name); // *ptr_patches
		if (FAILED(D3DXCreateTextureFromFileA(d3d9Device, buf, &tex))) continue;

		sprintf(buf, "tex%d", i);
		shader.Effect->SetTexture(buf, tex);
		shaderTextures.push_back(tex);
	}

	shader.ehTicks = shader.Effect->GetParameterByName(0, "tickcount");
	shaders.push_back(shader);
	shadersSize = shaders.size();
	return shadersSize - 1;
}

static void LoadGlobalShader() {
	if (!globalShadersActive) return;
	for (std::vector<sGlobalShader>::iterator it = gShaderFiles.begin(); it != gShaderFiles.end(); ++it) {
		if (it->badShader) continue;
		long index = LoadShader(it->gShaderFile.c_str());
		if (index != -1) {
			shaders[index].Effect->SetInt("w", Gfx_GetGameWidthRes());
			shaders[index].Effect->SetInt("h", Gfx_GetGameHeightRes());
			shaders[index].Active = true;
			dlog_f("Global shader file %s is loaded.\n", DL_INIT, it->gShaderFile.c_str());
		} else {
			it->badShader = true;
		}
	}
}

void __stdcall ActivateShader(DWORD d) {
	if (d < shadersSize && shaders[d].Effect) shaders[d].Active = true;
}

void __stdcall DeactivateShader(DWORD d) {
	if (d < shadersSize) shaders[d].Active = false;
}

int __stdcall GetShaderTexture(DWORD d, DWORD id) {
	if (id < 1 || id > 128 || d >= shadersSize || !shaders[d].Effect) return -1;
	IDirect3DBaseTexture9* tex = 0;
	char buf[8] = "tex";
	_itoa_s(id, &buf[3], 4, 10);
	if (FAILED(shaders[d].Effect->GetTexture(buf, &tex)) || !tex) return -1;
	tex->Release();
	for (DWORD i = 0; i < shaderTextures.size(); i++) {
		if (shaderTextures[i] == tex) return i;
	}
	return -1;
}

void __stdcall FreeShader(DWORD d) {
	if (d < shadersSize) {
		SAFERELEASE(shaders[d].Effect);
		shaders[d].Active = false;
	}
}

void __stdcall SetShaderInt(DWORD d, const char* param, int value) {
	if (d >= shadersSize || !shaders[d].Effect) return;
	shaders[d].Effect->SetInt(param, value);
}

void __stdcall SetShaderFloat(DWORD d, const char* param, float value) {
	if (d >= shadersSize || !shaders[d].Effect) return;
	shaders[d].Effect->SetFloat(param, value);
}

void __stdcall SetShaderVector(DWORD d, const char* param, float f1, float f2, float f3, float f4) {
	if (d >= shadersSize || !shaders[d].Effect) return;
	shaders[d].Effect->SetFloatArray(param, &f1, 4);
}

void __stdcall SetShaderTexture(DWORD d, const char* param, DWORD value) {
	if (d >= shadersSize || !shaders[d].Effect || value >= shaderTextures.size()) return;
	shaders[d].Effect->SetTexture(param, shaderTextures[value]);
}

////////////////////////////////////////////////////////////////////////////////

static void WindowInit() {
	windowInit = true;
	rcpres[0] = 1.0f / (float)Gfx_GetGameWidthRes();
	rcpres[1] = 1.0f / (float)Gfx_GetGameHeightRes();
	LoadGlobalShader();
}

static void GetDisplayMode(D3DDISPLAYMODE &ddm) {
	ZeroMemory(&ddm, sizeof(ddm));
	d3d9->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &ddm);
	dlog_f("Display mode format ID: %d\n", DL_INIT, ddm.Format);
}

static void ResetDevice(bool createNew) {
	D3DPRESENT_PARAMETERS params;
	ZeroMemory(&params, sizeof(params));

	D3DDISPLAYMODE dispMode;
	GetDisplayMode(dispMode);

	params.BackBufferCount = 1;
	params.BackBufferFormat = dispMode.Format; // (GraphicsMode != 4) ? D3DFMT_UNKNOWN : D3DFMT_X8R8G8B8;
	params.BackBufferWidth = gWidth;
	params.BackBufferHeight = gHeight;
	params.EnableAutoDepthStencil = false;
	//params.MultiSampleQuality = 0;
	//params.MultiSampleType = D3DMULTISAMPLE_NONE;
	params.Windowed = (GraphicsMode != 4);
	params.SwapEffect = D3DSWAPEFFECT_DISCARD;
	params.hDeviceWindow = window;
	params.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
	if (!params.Windowed) params.FullScreen_RefreshRateInHz = dispMode.RefreshRate;

	static bool software = false;
	static D3DFORMAT textureFormat = D3DFMT_X8R8G8B8;
	bool A8_IsSupport = false;

	if (createNew) {
		dlog("Creating D3D9 Device...", DL_MAIN);
		if (FAILED(d3d9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window, D3DCREATE_PUREDEVICE | D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED | D3DCREATE_FPU_PRESERVE, &params, &d3d9Device))) {
			MessageBoxA(window, "Failed to create hardware vertex processing device.\nUsing software vertex processing instead.",
			                    "D3D9 Device", MB_TASKMODAL | MB_ICONWARNING);
			software = true;
			d3d9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window, D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED | D3DCREATE_FPU_PRESERVE, &params, &d3d9Device);
		}

		D3DCAPS9 caps;
		d3d9Device->GetDeviceCaps(&caps);
		ShaderVersion = ((caps.PixelShaderVersion & 0x0000FF00) >> 8) * 10 + (caps.PixelShaderVersion & 0xFF);

		// Use: 0 - only CPU, 1 - force GPU, 2 - Auto Mode (GPU or switch to CPU)
		if (GPUBlt == 2 && ShaderVersion < 20) GPUBlt = 0;

		if (GPUBlt) {
			A8_IsSupport = (d3d9Device->CreateTexture(ResWidth, ResHeight, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8, D3DPOOL_DEFAULT, &mainTex, 0) == D3D_OK);
			textureFormat = (A8_IsSupport) ? D3DFMT_A8 : D3DFMT_L8; // D3DFMT_A8 - not supported on some older video cards

			const char* shader = (A8_IsSupport) ? gpuEffectA8 : gpuEffectL8;
			D3DXCreateEffect(d3d9Device, shader, strlen(shader), 0, 0, 0, 0, &gpuBltEffect, 0);
			gpuBltMainTex = gpuBltEffect->GetParameterByName(0, "image");
			gpuBltPalette = gpuBltEffect->GetParameterByName(0, "palette");
			// for head textures
			gpuBltHead = gpuBltEffect->GetParameterByName(0, "head");
			gpuBltHeadSize = gpuBltEffect->GetParameterByName(0, "size");
			gpuBltHeadCorner = gpuBltEffect->GetParameterByName(0, "corner");

			Gfx_SetDefaultTechnique();
		}
	} else {
		dlog("Resetting D3D9 Device...", DL_MAIN);
		d3d9Device->Reset(&params);
		if (gpuBltEffect) gpuBltEffect->OnResetDevice();
		for (DWORD d = 0; d < shadersSize; d++) {
			if (shaders[d].Effect) shaders[d].Effect->OnResetDevice();
		}
		mainTexLock = false;
	}

	if (!A8_IsSupport && d3d9Device->CreateTexture(ResWidth, ResHeight, 1, D3DUSAGE_DYNAMIC, textureFormat, D3DPOOL_DEFAULT, &mainTex, 0) != D3D_OK) {
		d3d9Device->CreateTexture(ResWidth, ResHeight, 1, D3DUSAGE_DYNAMIC, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &mainTex, 0);
		GPUBlt = 0;
		textureFormat = D3DFMT_X8R8G8B8;
		MessageBoxA(window, "GPU does not support the D3DFMT_L8 texture format.\nNow CPU is used to convert the palette.",
		                    "Texture format error", MB_TASKMODAL | MB_ICONWARNING);
	}

	d3d9Device->CreateTexture(ResWidth, ResHeight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &sTex1, 0);
	d3d9Device->CreateTexture(ResWidth, ResHeight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &sTex2, 0);
	sTex1->GetSurfaceLevel(0, &sSurf1);
	sTex2->GetSurfaceLevel(0, &sSurf2);

	if (GPUBlt) {
		d3d9Device->CreateTexture(256, 1, 1, D3DUSAGE_DYNAMIC, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &gpuPalette, 0);
		gpuBltEffect->SetTexture(gpuBltMainTex, mainTex);
		gpuBltEffect->SetTexture(gpuBltPalette, gpuPalette);
	}

	d3d9Device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backBuffer);

	ShaderVertices[1].y = ResHeight - 0.5f;
	ShaderVertices[2].x = ResWidth - 0.5f;
	ShaderVertices[3].y = ResHeight - 0.5f;
	ShaderVertices[3].x = ResWidth - 0.5f;

	d3d9Device->CreateVertexBuffer(4 * sizeof(VertexFormat), D3DUSAGE_WRITEONLY | (software ? D3DUSAGE_SOFTWAREPROCESSING : 0), _VERTEXFORMAT, D3DPOOL_DEFAULT, &vertexOrigRes, 0);
	void* vertexPointer;
	vertexOrigRes->Lock(0, 0, &vertexPointer, 0);
	CopyMemory(vertexPointer, ShaderVertices, sizeof(ShaderVertices));
	vertexOrigRes->Unlock();

	VertexFormat shaderVertices[4] = {
		ShaderVertices[0],
		ShaderVertices[1],
		ShaderVertices[2],
		ShaderVertices[3]
	};

	shaderVertices[1].y = (float)gHeight - 0.5f;
	shaderVertices[2].x = (float)gWidth - 0.5f;
	shaderVertices[3].y = (float)gHeight - 0.5f;
	shaderVertices[3].x = (float)gWidth - 0.5f;

	d3d9Device->CreateVertexBuffer(4 * sizeof(VertexFormat), D3DUSAGE_WRITEONLY | (software ? D3DUSAGE_SOFTWAREPROCESSING : 0), _VERTEXFORMAT, D3DPOOL_DEFAULT, &vertexSfallRes, 0);
	vertexSfallRes->Lock(0, 0, &vertexPointer, 0);
	CopyMemory(vertexPointer, shaderVertices, sizeof(shaderVertices));
	vertexSfallRes->Unlock();

	d3d9Device->CreateVertexBuffer(4 * sizeof(VertexFormat), D3DUSAGE_WRITEONLY | (software ? D3DUSAGE_SOFTWAREPROCESSING : 0), _VERTEXFORMAT, D3DPOOL_DEFAULT, &vertexMovie, 0);

	d3d9Device->SetFVF(_VERTEXFORMAT);
	d3d9Device->SetTexture(0, mainTex);
	d3d9Device->SetStreamSource(0, vertexOrigRes, 0, sizeof(VertexFormat));

	//d3d9Device->SetRenderState(D3DRS_ALPHABLENDENABLE, false); // default false
	//d3d9Device->SetRenderState(D3DRS_ALPHATESTENABLE, false);  // default false
	d3d9Device->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
	d3d9Device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	d3d9Device->SetRenderState(D3DRS_LIGHTING, false);

	if (textureFilter) {
		d3d9Device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		d3d9Device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	}
	//d3d9Device->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(255, 0, 255), 1.0f, 0); // for debbuging
	dlogr(" Done", DL_MAIN);
}

static void Present() {
	if ((moveWindowKey[0] != 0 && KeyDown(moveWindowKey[0])) ||
	    (moveWindowKey[1] != 0 && KeyDown(moveWindowKey[1])))
	{
		int winX, winY;
		GetMouse(&winX, &winY);
		windowLeft += winX;
		windowTop += winY;

		RECT r, r2;
		r.left = windowLeft;
		r.right = windowLeft + gWidth;
		r.top = windowTop;
		r.bottom = windowTop + gHeight;
		AdjustWindowRect(&r, WS_CAPTION | WS_BORDER, false);

		r.right -= (r.left - windowLeft);
		r.left = windowLeft;
		r.bottom -= (r.top - windowTop);
		r.top = windowTop;

		if (GetWindowRect(GetShellWindow(), &r2)) {
			if (r.right > r2.right) {
				DWORD move = r.right - r2.right;
				windowLeft -= move;
				r.right -= move;
			} else if (r.left < r2.left) {
				DWORD move = r2.left - r.left;
				windowLeft += move;
				r.right += move;
			}
			if (r.bottom > r2.bottom) {
				DWORD move = r.bottom - r2.bottom;
				windowTop -= move;
				r.bottom -= move;
			} else if (r.top < r2.top) {
				DWORD move = r2.top - r.top;
				windowTop += move;
				r.bottom += move;
			}
		}
		MoveWindow(window, windowLeft, windowTop, r.right - windowLeft, r.bottom - windowTop, true);
	}

	if (d3d9Device->Present(0, 0, 0, 0) == D3DERR_DEVICELOST) {
		#ifndef NDEBUG
		dlogr("\nPresent: D3DERR_DEVICELOST", DL_MAIN);
		#endif
		DeviceLost = true;
		d3d9Device->SetTexture(0, 0);
		SAFERELEASE(mainTex)
		SAFERELEASE(backBuffer);
		SAFERELEASE(sSurf1);
		SAFERELEASE(sSurf2);
		SAFERELEASE(sTex1);
		SAFERELEASE(sTex2);
		SAFERELEASE(vertexOrigRes);
		SAFERELEASE(vertexSfallRes);
		SAFERELEASE(vertexMovie);
		SAFERELEASE(gpuPalette);
		Gfx_ReleaseMovieTexture();
		if (gpuBltEffect) gpuBltEffect->OnLostDevice();
		for (DWORD d = 0; d < shadersSize; d++) {
			if (shaders[d].Effect) shaders[d].Effect->OnLostDevice();
		}
	}
}

static void Refresh() {
	if (DeviceLost) return;

	d3d9Device->BeginScene();
	d3d9Device->SetStreamSource(0, vertexOrigRes, 0, sizeof(VertexFormat));
	d3d9Device->SetRenderTarget(0, sSurf1);
	d3d9Device->SetTexture(0, mainTex);

	UINT passes;
	if (GPUBlt) {
		// converts the palette in mainTex to RGB colors on the target surface (sSurf1/sTex1)
		gpuBltEffect->Begin(&passes, 0);
		gpuBltEffect->BeginPass(0);
		d3d9Device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
		gpuBltEffect->EndPass();
		gpuBltEffect->End();

		if (shadersSize) {
			d3d9Device->StretchRect(sSurf1, 0, sSurf2, 0, D3DTEXF_NONE); // copy sSurf1 to sSurf2
			d3d9Device->SetTexture(0, sTex2);
		} else {
			d3d9Device->SetTexture(0, sTex1);
		}
	}
	for (int i = shadersSize - 1; i >= 0; i--) {
		if (!shaders[i].Effect || !shaders[i].Active) continue;
		if (shaders[i].mode2 && !(shaders[i].mode2 & GetLoopFlags())) continue;
		if (shaders[i].mode & GetLoopFlags()) continue;

		if (shaders[i].ehTicks) shaders[i].Effect->SetInt(shaders[i].ehTicks, GetTickCount());

		UINT passes;
		shaders[i].Effect->Begin(&passes, 0);
		for (DWORD pass = 0; pass < passes; pass++) {
			shaders[i].Effect->BeginPass(pass);
			d3d9Device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
			shaders[i].Effect->EndPass();

			d3d9Device->StretchRect(sSurf1, 0, sSurf2, 0, D3DTEXF_NONE); // copy sSurf1 to sSurf2
			d3d9Device->SetTexture(0, sTex2);
		}
		shaders[i].Effect->End();
		d3d9Device->SetTexture(0, sTex2);
	}

	d3d9Device->SetStreamSource(0, vertexSfallRes, 0, sizeof(VertexFormat));
	d3d9Device->SetRenderTarget(0, backBuffer);

	d3d9Device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2); // render square with a texture image (mainTex/sTex1/sTex2) to the backBuffer
	d3d9Device->EndScene();
	Present();
}

void Gfx_RefreshGraphics() {
	if (!Gfx_PlayAviMovie) Refresh();
}

HRESULT Gfx_CreateMovieTexture(D3DSURFACE_DESC &desc) {
	HRESULT hr = d3d9Device->CreateTexture(desc.Width, desc.Height, 1, 0, desc.Format, D3DPOOL_DEFAULT, &movieTex, nullptr);
	if (movieTex) movieTex->GetLevelDesc(0, &movieDesc);
	return hr;
}

void Gfx_ReleaseMovieTexture() {
	SAFERELEASE(movieTex);
}

void Gfx_SetMovieTexture(bool state) {
	dlog("\nSet movie texture.", DL_INIT);
	if (!state) {
		Gfx_PlayAviMovie = false;
		return;
	} else if (Gfx_PlayAviMovie) return;

	if (!movieTex) Gfx_CreateMovieTexture(movieDesc);
	D3DSURFACE_DESC &desc = movieDesc;

	float aviAspect = (float)desc.Width / (float)desc.Height;
	float winAspect = (float)gWidth / (float)gHeight;

	VertexFormat shaderVertices[4] = {
		ShaderVertices[0],
		ShaderVertices[1],
		ShaderVertices[2],
		ShaderVertices[3]
	};

	shaderVertices[1].y = (float)gHeight - 0.5f;
	shaderVertices[2].x = (float)gWidth - 0.5f;
	shaderVertices[3].y = (float)gHeight - 0.5f;
	shaderVertices[3].x = (float)gWidth - 0.5f;

	bool subtitleShow = (*ptr_subtitleList != nullptr);

	long offset;
	if (aviAspect > winAspect) {
		// scales height proportionally and places the movie surface at the center of the window along the Y-axis
		aviAspect = (float)desc.Width / (float)gWidth;
		desc.Height = (int)(desc.Height / aviAspect);

		offset = (gHeight - desc.Height) / 2;

		shaderVertices[0].y += offset;
		shaderVertices[2].y += offset;
		shaderVertices[1].y -= offset;
		shaderVertices[3].y -= offset;

		subtitleShow = false;
	} else if (aviAspect < winAspect) {
		if (Gfx_AviMovieWidthFit || (hrpIsEnabled && *(DWORD*)HRPAddress(0x1006EC10) == 2)) {
			//desc.Width = gWidth; // scales the movie surface to screen width
		} else {
			// scales width proportionally and places the movie surface at the center of the window along the X-axis
			aviAspect = (float)desc.Height / (float)gHeight;
			desc.Width = (int)(desc.Width / aviAspect);

			offset = (gWidth - desc.Width) / 2;

			shaderVertices[0].x += offset;
			shaderVertices[2].x -= offset;
			shaderVertices[3].x -= offset;
			shaderVertices[1].x += offset;
		}
	}
	if (subtitleShow) { // decrease the surface size to display the subtitle text on the lower layer of surfaces
		int offset = (int)(15.0f * ((float)gHeight / (float)ResHeight));
		shaderVertices[1].y -= offset;
		shaderVertices[3].y -= offset;
	}

	void* vertexPointer;
	vertexMovie->Lock(0, 0, &vertexPointer, 0);
	CopyMemory(vertexPointer, shaderVertices, sizeof(shaderVertices));
	vertexMovie->Unlock();

	Gfx_PlayAviMovie = true;
}

void Gfx_ShowMovieFrame(IDirect3DTexture9* tex) {
	if (!tex || DeviceLost || !movieTex) return;

	d3d9Device->UpdateTexture(tex, movieTex);
	d3d9Device->SetRenderTarget(0, backBuffer);

	d3d9Device->BeginScene();
	if (!mainTexLock) {
		if (shadersSize && GPUBlt) {
			d3d9Device->SetTexture(0, sTex2);
		} else {
			d3d9Device->SetTexture(0, mainTex);
		}
		d3d9Device->SetStreamSource(0, vertexSfallRes, 0, sizeof(VertexFormat));

		// for showing subtitles
		if (GPUBlt) {
			UINT passes;
			gpuBltEffect->Begin(&passes, 0);
			gpuBltEffect->BeginPass(0);
		}
		d3d9Device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
		if (GPUBlt) {
			gpuBltEffect->EndPass();
			gpuBltEffect->End();
		}
	}
	// for avi movie
	d3d9Device->SetTexture(0, movieTex);
	d3d9Device->SetStreamSource(0, vertexMovie, 0, sizeof(VertexFormat));
	d3d9Device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);

	d3d9Device->EndScene();
	Present();
}

void Gfx_SetHeadTex(IDirect3DTexture9* tex, int width, int height, int xoff, int yoff) {
	gpuBltEffect->SetTexture(gpuBltHead, tex);

	float size[2];
	size[0] = (float)width * rcpres[0];
	size[1] = (float)height * rcpres[1];
	gpuBltEffect->SetFloatArray(gpuBltHeadSize, size, 2);

	size[0] = (126.0f + xoff + ((388 - width) / 2)) * rcpres[0];
	size[1] = (14.0f + yoff + ((200 - height) / 2)) * rcpres[1];
	gpuBltEffect->SetFloatArray(gpuBltHeadCorner, size, 2);

	Gfx_SetHeadTechnique();
}

void Gfx_SetHeadTechnique() {
	gpuBltEffect->SetTechnique("T1");
}

void Gfx_SetDefaultTechnique() {
	gpuBltEffect->SetTechnique("T0");
}

static void SetGPUPalette() {
	D3DLOCKED_RECT rect;
	if (gpuPalette && !FAILED(gpuPalette->LockRect(0, &rect, 0, D3DLOCK_DISCARD))) {
		CopyMemory(rect.pBits, palette, 256 * 4);
		gpuPalette->UnlockRect(0);
	}
}

class FakeDirectDrawSurface : IDirectDrawSurface {
private:
	ULONG Refs;
	bool isPrimary;
	BYTE* lockTarget;
	RECT* lockRect;

public:
	static bool IsPlayMovie;

	FakeDirectDrawSurface(bool primary) {
		Refs = 1;
		isPrimary = primary;
		lockTarget = nullptr;
		if (primary && GPUBlt) {
			// use the mainTex texture as a buffer
		} else {
			lockTarget = new BYTE[ResWidth * ResHeight];
		}
	}

	/* IUnknown methods */

	HRESULT __stdcall QueryInterface(REFIID, LPVOID *) { return E_NOINTERFACE; }

	ULONG __stdcall AddRef() { return ++Refs; }

	ULONG __stdcall Release() {
		if (!--Refs) {
			delete[] lockTarget;
			delete this;
			return 0;
		} else return Refs;
	}

	/* IDirectDrawSurface methods */

	HRESULT __stdcall AddAttachedSurface(LPDIRECTDRAWSURFACE) { UNUSEDFUNCTION; }
	HRESULT __stdcall AddOverlayDirtyRect(LPRECT) { UNUSEDFUNCTION; }

	HRESULT __stdcall Blt(LPRECT dsc, LPDIRECTDRAWSURFACE b, LPRECT scr, DWORD d, LPDDBLTFX e) { // called 0x4868DA movie_MVE_ShowFrame_ (used for game movies, only for w/o HRP)
		mveDesc.dwHeight = (dsc->bottom - dsc->top);
		yoffset = (ResHeight - mveDesc.dwHeight) / 2;
		mveDesc.lPitch = (dsc->right - dsc->left);
		//xoffset = (ResWidth - mveDesc.lPitch) / 2;

		//dlog_f("\nBlt: [mveDesc: w:%d, h:%d]", DL_INIT, mveDesc.lPitch, mveDesc.dwHeight);

		IsPlayMovie = true;
		//mainTexLock = true;

		BYTE* lockTarget = ((FakeDirectDrawSurface*)b)->lockTarget;

		D3DLOCKED_RECT dRect;
		mainTex->LockRect(0, &dRect, dsc, 0);

		DWORD width = mveDesc.lPitch; // the current size of the width of the mve movie

		if (GPUBlt) {
			BufToBuf(lockTarget, width, mveDesc.dwHeight, width, (BYTE*)dRect.pBits, dRect.Pitch);
			//char* pBits = (char*)dRect.pBits;
			//for (DWORD y = 0; y < mveDesc.dwHeight; y++) {
			//	CopyMemory(&pBits[y * pitch], &lockTarget[y * width], width);
			//}
		} else {
			int pitch = dRect.Pitch / 4;
			DWORD* pBits = (DWORD*)dRect.pBits;
			BYTE* target = lockTarget;

			int height = mveDesc.dwHeight;
			while (height--) {
				int x = width;
				while (x--) pBits[x] = palette[target[x]].xRGB;
				target += width;
				pBits += pitch;
			}
		}
		mainTex->UnlockRect(0);
		//mainTexLock = false;
		//if (Gfx_PlayAviMovie) return DD_OK; // Blt method is not executed during avi playback because the sfShowFrame_ function is blocked

		Refresh();
		return DD_OK;
	}

	HRESULT __stdcall BltBatch(LPDDBLTBATCH, DWORD, DWORD) { UNUSEDFUNCTION; }
	HRESULT __stdcall BltFast(DWORD,DWORD,LPDIRECTDRAWSURFACE, LPRECT,DWORD) { UNUSEDFUNCTION; }
	HRESULT __stdcall DeleteAttachedSurface(DWORD,LPDIRECTDRAWSURFACE) { UNUSEDFUNCTION; }
	HRESULT __stdcall EnumAttachedSurfaces(LPVOID,LPDDENUMSURFACESCALLBACK) { UNUSEDFUNCTION; }
	HRESULT __stdcall EnumOverlayZOrders(DWORD,LPVOID,LPDDENUMSURFACESCALLBACK) { UNUSEDFUNCTION; }
	HRESULT __stdcall Flip(LPDIRECTDRAWSURFACE, DWORD) { UNUSEDFUNCTION; }
	HRESULT __stdcall GetAttachedSurface(LPDDSCAPS, LPDIRECTDRAWSURFACE *) { UNUSEDFUNCTION; }
	HRESULT __stdcall GetBltStatus(DWORD) { UNUSEDFUNCTION; }
	HRESULT __stdcall GetCaps(LPDDSCAPS) { UNUSEDFUNCTION; }
	HRESULT __stdcall GetClipper(LPDIRECTDRAWCLIPPER *) { UNUSEDFUNCTION; }
	HRESULT __stdcall GetColorKey(DWORD, LPDDCOLORKEY) { UNUSEDFUNCTION; }
	HRESULT __stdcall GetDC(HDC *) { UNUSEDFUNCTION; }
	HRESULT __stdcall GetFlipStatus(DWORD) { UNUSEDFUNCTION; }
	HRESULT __stdcall GetOverlayPosition(LPLONG, LPLONG) { UNUSEDFUNCTION; }
	HRESULT __stdcall GetPalette(LPDIRECTDRAWPALETTE *) { UNUSEDFUNCTION; }
	HRESULT __stdcall GetPixelFormat(LPDDPIXELFORMAT) { UNUSEDFUNCTION; }
	HRESULT __stdcall GetSurfaceDesc(LPDDSURFACEDESC) { UNUSEDFUNCTION; }
	HRESULT __stdcall Initialize(LPDIRECTDRAW, LPDDSURFACEDESC) { UNUSEDFUNCTION; }
	HRESULT __stdcall IsLost() { UNUSEDFUNCTION; }

	/* Called from:
		0x4CB887 GNW95_ShowRect_      [c=1]
		0x48699D movieShowFrame_      [c=1]
		0x4CBBFA GNW95_zero_vid_mem_  [c=1] (clear surface)
		0x4F5E91/0x4F5EBB sub_4F5E60  [c=0] (from MVE_rmStepMovie_)
		0x486861 movie_MVE_ShowFrame_ [c=1] (capture, never called)
	*/
	HRESULT __stdcall Lock(LPRECT a, LPDDSURFACEDESC b, DWORD c, HANDLE d) {
		if (DeviceLost && Restore() == DD_FALSE) return DDERR_SURFACELOST; // DDERR_SURFACELOST 0x887601C2
		if (isPrimary) {
			lockRect = a;
			if (GPUBlt) {
				D3DLOCKED_RECT buf;
				if (SUCCEEDED(mainTex->LockRect(0, &buf, lockRect, 0))) {
					mainTexLock = true;
					b->lpSurface = buf.pBits;
					b->lPitch = buf.Pitch;
				}
			} else {
				*b = surfaceDesc;
				b->lpSurface = lockTarget;
			}
		} else {
			mveDesc.lPitch = *(DWORD*)_lastMovieW;
			mveDesc.dwHeight = *(DWORD*)_lastMovieH;
			//dlog_f("\nLock: [mveDesc: w:%d, h:%d]", DL_INIT, mveDesc.lPitch, mveDesc.dwHeight);
			*b = mveDesc;
			b->lpSurface = lockTarget;
		}
		return DD_OK;
	}

	HRESULT __stdcall ReleaseDC(HDC) { UNUSEDFUNCTION; }

	HRESULT __stdcall Restore() { // called 0x4CB907 GNW95_ShowRect_
		if (!d3d9Device) return DD_FALSE;
		if (d3d9Device->TestCooperativeLevel() == D3DERR_DEVICENOTRESET) {
			ResetDevice(false);
			DeviceLost = false;
			if (GPUBlt) SetGPUPalette(); // restore palette
			dlogr("\nD3D9 Device restored.", DL_MAIN);
		}
		return DeviceLost;
	}

	HRESULT __stdcall SetClipper(LPDIRECTDRAWCLIPPER) { UNUSEDFUNCTION; }
	HRESULT __stdcall SetColorKey(DWORD, LPDDCOLORKEY) { UNUSEDFUNCTION; }
	HRESULT __stdcall SetOverlayPosition(LONG, LONG) { UNUSEDFUNCTION; }

	HRESULT __stdcall SetPalette(LPDIRECTDRAWPALETTE a) { // called 0x4CB198 GNW95_init_DirectDraw_
		if (DeviceLost || a) return DD_OK;                // prevents executing the function when called from outside of sfall
		//dlog("\nSetPalette", DL_INIT);
		mainTexLock = true;

		D3DLOCKED_RECT dRect;
		mainTex->LockRect(0, &dRect, 0, D3DLOCK_DISCARD);

		DWORD* pBits = (DWORD*)dRect.pBits;
		int pitch = (dRect.Pitch / 4) - ResWidth;
		BYTE* target = lockTarget;

		int height = ResHeight;
		while (height--) {
			int width = ResWidth;
			while (width--) {
				pBits[0] = palette[target[0]].xRGB; // takes the color index in the palette and copies the color value to the texture surface
				target++;
				pBits++;
			}
			pBits += pitch;
		}

		mainTex->UnlockRect(0);
		mainTexLock = false;
		return DD_OK;
	}

	/* Called from:
		0x4CB8F0 GNW95_ShowRect_      (common game, primary)
		0x486A87 movieShowFrame_
		0x4CBC5A GNW95_zero_vid_mem_  (clear surface)
		0x4F5ECC sub_4F5E60           (from MVE_rmStepMovie_)
		0x4868BA movie_MVE_ShowFrame_ (capture never call)
	*/
	HRESULT __stdcall Unlock(LPVOID lockSurface) {
		//dlog("\nUnlock", DL_INIT);
		if (!isPrimary) return DD_OK;
		//dlog("\nUnlock -> primary", DL_INIT);

		if (GPUBlt == 0) {
			mainTexLock = true;

			D3DLOCKED_RECT dRect;
			mainTex->LockRect(0, &dRect, lockRect, (lockRect) ? 0 : D3DLOCK_DISCARD);

			DWORD* pBits = (DWORD*)dRect.pBits;
			int pitch = dRect.Pitch / 4;

			if (lockRect) {
				BYTE* target = (BYTE*)lockSurface;
				int width = (lockRect->right - lockRect->left);
				int height = (lockRect->bottom - lockRect->top);

				while (height--) {
					int w = width;
					while (w--) pBits[w] = palette[target[w]].xRGB;
					pBits += pitch;
					target += ResWidth;
				}

				lockRect = 0;
			} else {
				pitch -= ResWidth;
				BYTE* target = lockTarget;
				int height = ResHeight;

				// slow copy method
				while (height--) {
					int width = ResWidth;
					while (width--) {
						pBits[0] = palette[target[0]].xRGB;
						target++;
						pBits++;
					}
					pBits += pitch;
				}
			}
		}
		if (mainTexLock) mainTex->UnlockRect(0);
		mainTexLock = false;

		if (!IsPlayMovie && !Gfx_PlayAviMovie) {
			//dlog("\nUnlock -> RefreshGraphics", DL_INIT);
			Refresh();
		}
		IsPlayMovie = false;
		return DD_OK;
	}

	HRESULT __stdcall UpdateOverlay(LPRECT, LPDIRECTDRAWSURFACE,LPRECT,DWORD, LPDDOVERLAYFX) { UNUSEDFUNCTION; }
	HRESULT __stdcall UpdateOverlayDisplay(DWORD) { UNUSEDFUNCTION; }
	HRESULT __stdcall UpdateOverlayZOrder(DWORD, LPDIRECTDRAWSURFACE) { UNUSEDFUNCTION; }
};

bool FakeDirectDrawSurface::IsPlayMovie;

class FakeDirectDrawPalette : IDirectDrawPalette {
private:
	ULONG Refs;
public:
	FakeDirectDrawPalette() {
		Refs = 1;
	}

	/* IUnknown methods */

	HRESULT __stdcall QueryInterface(REFIID, LPVOID*) { return E_NOINTERFACE; }

	ULONG __stdcall AddRef() { return ++Refs; }

	ULONG __stdcall Release() {
		if (!--Refs) {
			delete this;
			return 0;
		} else return Refs;
	}

	/* IDirectDrawPalette methods */

	HRESULT __stdcall GetCaps(LPDWORD) { UNUSEDFUNCTION; }
	HRESULT __stdcall GetEntries(DWORD, DWORD, DWORD, LPPALETTEENTRY) { UNUSEDFUNCTION; }
	HRESULT __stdcall Initialize(LPDIRECTDRAW, DWORD, LPPALETTEENTRY) { UNUSEDFUNCTION; }

	/* Called from:
		0x4CB5C7 GNW95_SetPalette_
		0x4CB36B GNW95_SetPaletteEntries_
	*/
	HRESULT __stdcall SetEntries(DWORD a, DWORD b, DWORD c, LPPALETTEENTRY destPal) { // used to set palette for splash screen, fades, subtitles
		if (!windowInit || c == 0 || b + c > 256) return DDERR_INVALIDPARAMS;

		__movsd((DWORD*)&palette[b], (DWORD*)destPal, c);

		if (GPUBlt) {
			SetGPUPalette();
		} else {
			// X8B8G8R8 format
			for (size_t i = b; i < b + c; i++) { // swap color B <> R
				BYTE clr = palette[i].B;
				palette[i].B = palette[i].R;
				palette[i].R = clr;
			}
			primaryDDSurface->SetPalette(0); // update texture
			if (FakeDirectDrawSurface::IsPlayMovie) return DD_OK; // prevents flickering at the beginning of playback (w/o HRP & GPUBlt=2)
		}
		if (!Gfx_PlayAviMovie) {
			//dlog("\nSetEntries -> RefreshGraphics", DL_INIT);
			Refresh();
		}
		return DD_OK;
	}
};

class FakeDirectDraw : IDirectDraw {
private:
	ULONG Refs;
public:
	FakeDirectDraw() {
		Refs = 1;
	}

	/* IUnknown methods */

	HRESULT __stdcall QueryInterface(REFIID, LPVOID*) { return E_NOINTERFACE; }

	ULONG __stdcall AddRef()  { return ++Refs; }

	ULONG __stdcall Release() { // called from game on exit
		if (!--Refs) {
			globalShadersActive = false;
			Graphics_OnGameLoad();
			for (DWORD d = 0; d < shaderTextures.size(); d++) shaderTextures[d]->Release();
			shaderTextures.clear();

			SAFERELEASE(backBuffer);
			SAFERELEASE(sSurf1);
			SAFERELEASE(sSurf2);
			SAFERELEASE(mainTex);
			SAFERELEASE(sTex1);
			SAFERELEASE(sTex2);
			SAFERELEASE(vertexOrigRes);
			SAFERELEASE(vertexSfallRes);
			SAFERELEASE(d3d9Device);
			SAFERELEASE(d3d9);
			SAFERELEASE(gpuPalette);
			SAFERELEASE(gpuBltEffect);
			SAFERELEASE(vertexMovie);

			delete this;
			return 0;
		} else return Refs;
	}

	/* IDirectDraw methods */

	HRESULT __stdcall Compact() { UNUSEDFUNCTION; }
	HRESULT __stdcall CreateClipper(DWORD, LPDIRECTDRAWCLIPPER*, IUnknown*) { UNUSEDFUNCTION; }

	HRESULT __stdcall CreatePalette(DWORD, LPPALETTEENTRY, LPDIRECTDRAWPALETTE* c, IUnknown*) { // called 0x4CB182 GNW95_init_DirectDraw_
		*c = (IDirectDrawPalette*)new FakeDirectDrawPalette();
		return DD_OK;
	}

	/*
		0x4CB094 GNW95_init_DirectDraw_ (primary surface)
		0x4F5DD4/0x4F5DF9 nfConfig_     (mve surface)
	*/
	HRESULT __stdcall CreateSurface(LPDDSURFACEDESC a, LPDIRECTDRAWSURFACE* b, IUnknown* c) {
		if (a->ddsCaps.dwCaps == DDSCAPS_PRIMARYSURFACE && a->dwFlags == DDSD_CAPS) {
			*b = primaryDDSurface = (IDirectDrawSurface*)new FakeDirectDrawSurface(true);
		} else {
			*b = (IDirectDrawSurface*)new FakeDirectDrawSurface(false);
		}
		return DD_OK;
	}

	HRESULT __stdcall DuplicateSurface(LPDIRECTDRAWSURFACE, LPDIRECTDRAWSURFACE *) { UNUSEDFUNCTION; }
	HRESULT __stdcall EnumDisplayModes(DWORD, LPDDSURFACEDESC, LPVOID, LPDDENUMMODESCALLBACK) { UNUSEDFUNCTION; }
	HRESULT __stdcall EnumSurfaces(DWORD, LPDDSURFACEDESC, LPVOID,LPDDENUMSURFACESCALLBACK) { UNUSEDFUNCTION; }
	HRESULT __stdcall FlipToGDISurface() { UNUSEDFUNCTION; }
	HRESULT __stdcall GetCaps(LPDDCAPS, LPDDCAPS b) { UNUSEDFUNCTION; }
	HRESULT __stdcall GetDisplayMode(LPDDSURFACEDESC) { UNUSEDFUNCTION; }
	HRESULT __stdcall GetFourCCCodes(LPDWORD,LPDWORD) { UNUSEDFUNCTION; }
	HRESULT __stdcall GetGDISurface(LPDIRECTDRAWSURFACE *) { UNUSEDFUNCTION; }
	HRESULT __stdcall GetMonitorFrequency(LPDWORD) { UNUSEDFUNCTION; }
	HRESULT __stdcall GetScanLine(LPDWORD) { UNUSEDFUNCTION; }
	HRESULT __stdcall GetVerticalBlankStatus(LPBOOL) { UNUSEDFUNCTION; }
	HRESULT __stdcall Initialize(GUID *) { UNUSEDFUNCTION; }
	HRESULT __stdcall RestoreDisplayMode() { return DD_OK; }

	HRESULT __stdcall SetCooperativeLevel(HWND a, DWORD b) { // called 0x4CB005 GNW95_init_DirectDraw_
		window = a;

		if (!d3d9Device) {
			CoInitialize(0);
			ResetDevice(true); // create
		}
		dlog("Creating D3D9 Device window...", DL_MAIN);

		if (GraphicsMode >= 5) {
			SetWindowLong(a, GWL_STYLE, windowStyle);
			RECT r;
			r.left = 0;
			r.right = gWidth;
			r.top = 0;
			r.bottom = gHeight;
			AdjustWindowRect(&r, windowStyle, false);
			r.right -= r.left;
			r.bottom -= r.top;
			SetWindowPos(a, HWND_NOTOPMOST, windowLeft, windowTop, r.right, r.bottom, SWP_DRAWFRAME | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
		}

		dlogr(" Done", DL_MAIN);
		return DD_OK;
	}

	HRESULT __stdcall SetDisplayMode(DWORD, DWORD, DWORD) { return DD_OK; } // called 0x4CB01B GNW95_init_DirectDraw_
	HRESULT __stdcall WaitForVerticalBlank(DWORD, HANDLE) { UNUSEDFUNCTION; }
};

HRESULT __stdcall FakeDirectDrawCreate2(void*, IDirectDraw** b, void*) {
	dlog("Initializing Direct3D...", DL_MAIN);

	// original resolution or HRP
	ResWidth = *(DWORD*)0x4CAD6B;  // 640
	ResHeight = *(DWORD*)0x4CAD66; // 480

	if (!d3d9) d3d9 = Direct3DCreate9(D3D_SDK_VERSION);

	ZeroMemory(&surfaceDesc, sizeof(DDSURFACEDESC));

	surfaceDesc.dwSize = sizeof(DDSURFACEDESC);
	surfaceDesc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT | DDSD_PITCH;
	surfaceDesc.dwWidth = ResWidth;
	surfaceDesc.dwHeight = ResHeight;
	surfaceDesc.ddpfPixelFormat.dwRGBBitCount = 16; // R5G6B5
	surfaceDesc.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
	surfaceDesc.ddpfPixelFormat.dwRBitMask = 0xF800;
	surfaceDesc.ddpfPixelFormat.dwGBitMask = 0x7E0;
	surfaceDesc.ddpfPixelFormat.dwBBitMask = 0x1F;
	surfaceDesc.ddpfPixelFormat.dwFlags = DDPF_RGB;
	surfaceDesc.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
	surfaceDesc.lPitch = ResWidth;

	// set params for .mve surface
	mveDesc = surfaceDesc;
	mveDesc.lPitch = 640;
	mveDesc.dwWidth = 640;
	mveDesc.dwHeight = 480;

	if (GraphicsMode == 6) {
		D3DDISPLAYMODE dispMode;
		GetDisplayMode(dispMode);
		gWidth  = dispMode.Width;
		gHeight = dispMode.Height;
	} else {
		gWidth = GetConfigInt("Graphics", "GraphicsWidth", 0);
		gHeight = GetConfigInt("Graphics", "GraphicsHeight", 0);
		if (!gWidth || !gHeight) {
			gWidth = ResWidth;
			gHeight = ResHeight;
		}
	}

	GPUBlt = GetConfigInt("Graphics", "GPUBlt", 0); // 0 - auto, 1 - GPU, 2 - CPU
	if (!GPUBlt || GPUBlt > 2)
		GPUBlt = 2; // Swap them around to keep compatibility with old ddraw.ini
	else if (GPUBlt == 2) GPUBlt = 0; // Use CPU

	if (GraphicsMode == 5) {
		moveWindowKey[0] = GetConfigInt("Input", "WindowScrollKey", 0);
		if (moveWindowKey[0] < 0) {
			switch (moveWindowKey[0]) {
			case -1:
				moveWindowKey[0] = DIK_LCONTROL;
				moveWindowKey[1] = DIK_RCONTROL;
				break;
			case -2:
				moveWindowKey[0] = DIK_LMENU;
				moveWindowKey[1] = DIK_RMENU;
				break;
			case -3:
				moveWindowKey[0] = DIK_LSHIFT;
				moveWindowKey[1] = DIK_RSHIFT;
				break;
			default:
				moveWindowKey[0] = 0;
			}
		} else {
			moveWindowKey[0] &= 0xFF;
		}
		windowData = GetConfigInt("Graphics", "WindowData", 0);
		if (windowData > 0) {
			windowLeft = windowData >> 16;
			windowTop = windowData & 0xFFFF;
		} else {
			windowData = 0;
		}
	}

	rcpres[0] = 1.0f / (float)gWidth;
	rcpres[1] = 1.0f / (float)gHeight;

	if (textureFilter) {
		float wScale = (float)gWidth / ResWidth;
		float hScale = (float)gHeight / ResHeight;
		if (wScale == 1.0f && hScale == 1.0f) textureFilter = 0; // disable texture filtering if the set resolutions are equal
		if (textureFilter == 1) {
			if (static_cast<int>(wScale) == wScale && static_cast<int>(hScale) == hScale) {
				textureFilter = 0; // disable for integer scales
			}
		}
	}

	*b = (IDirectDraw*)new FakeDirectDraw();

	dlogr(" Done", DL_MAIN);
	return DD_OK;
}

static __declspec(naked) void game_init_hook() {
	__asm {
		push ecx;
		call WindowInit;
		pop  ecx;
		jmp  palette_init_;
	}
}

///////////////////////////////// GAME RENDER //////////////////////////////////

static __forceinline void UpdateDDSurface(BYTE* surface, int width, int height, int widthFrom, RECT* rect) {
	long x = rect->left;
	long y = rect->top;
	if (GraphicsMode == 0) {
		__asm {
			xor  eax, eax;
			push y;
			push x;
			push height;
			push width;
			push eax; // yFrom
			push eax; // xFrom
			push eax; // heightFrom
			push widthFrom;
			push surface;
			call ds:[_scr_blit]; // GNW95_ShowRect_(int from, int widthFrom, int heightFrom, int xFrom, int yFrom, int width, int height, int x, int y)
			add  esp, 9*4;
		}
	} else {
		DDSURFACEDESC desc;
		RECT lockRect = { x, y, rect->right + 1, rect->bottom + 1 };

		if (primaryDDSurface->Lock(&lockRect, &desc, 0, 0)) return; // lock error

		if (GPUBlt == 0) desc.lpSurface = (BYTE*)desc.lpSurface + (desc.lPitch * y) + x;
		BufToBuf(surface, width, height, widthFrom, (BYTE*)desc.lpSurface, desc.lPitch);

		primaryDDSurface->Unlock(desc.lpSurface);
	}
}

static void __fastcall sf_GNW_win_refresh(WINinfo* win, RECT* updateRect, BYTE* toBuffer);

class OverlaySurface
{
private:
	long size;
	long surfWidth;
	long allocSize;
	BYTE* surface;

public:
	long winType;

	OverlaySurface() : size(0), surface(nullptr), winType(-1) {}

	BYTE* Surface() { return surface; }

	void CreateSurface(WINinfo* win, long winType) {
		this->winType = winType;
		this->surfWidth = win->width;
		this->size = win->height * win->width;

		if (surface != nullptr) {
			if (size <= allocSize) {
				std::memset(surface, 0, size);
				return;
			}
			delete[] surface;
		}

		this->allocSize = size;
		surface = new BYTE[size]();
	}

	void ClearSurface() {
		if (surface != nullptr) std::memset(surface, 0, size);
	}

	void ClearSurface(sRectangle &rect) {
		if (surface != nullptr) {
			if (rect.width > surfWidth || rect.height > (size / surfWidth)) return; // going beyond the surface size
			BYTE* surf = surface + (surfWidth * rect.y) + rect.x;

			size_t sizeD = rect.width >> 2;
			size_t sizeB = rect.width & 3;
			size_t strideD = sizeD << 2;
			size_t stride = surfWidth - rect.width;

			long height = rect.height;
			while (height--) {
				if (sizeD) {
					__stosd((DWORD*)surf, 0, sizeD);
					surf += strideD;
				}
				if (sizeB) {
					__stosb(surf, 0, sizeB);
					surf += sizeB;
				}
				surf += stride;
			};
		}
	}

	void DestroySurface() {
		delete[] surface;
		surface = nullptr;
	}

	~OverlaySurface() {
		delete[] surface;
	}
} overlaySurfaces[5];

static long indexPosition = 0;

void GameRender_CreateOverlaySurface(WINinfo* win, long winType) {
	if (win->randY) return;
	if (overlaySurfaces[indexPosition].winType == winType) {
		overlaySurfaces[indexPosition].ClearSurface();
	} else {
		if (++indexPosition == 5) indexPosition = 0;
		overlaySurfaces[indexPosition].CreateSurface(win, winType);
	}
	win->randY = reinterpret_cast<long*>(&overlaySurfaces[indexPosition]);
}

BYTE* GameRender_GetOverlaySurface(WINinfo* win) {
	return reinterpret_cast<OverlaySurface*>(win->randY)->Surface();
}

void GameRender_ClearOverlay(WINinfo* win) {
	if (win->randY) reinterpret_cast<OverlaySurface*>(win->randY)->ClearSurface();
}

void GameRender_ClearOverlay(WINinfo* win, sRectangle &rect) {
	if (win->randY) {
		reinterpret_cast<OverlaySurface*>(win->randY)->ClearSurface(rect);
		BoundRect updateRect = { rect.x, rect.y, rect.right(), rect.bottom() };
		updateRect.x += win->rect.x;
		updateRect.y += win->rect.y;
		updateRect.offx += win->rect.x;
		updateRect.offy += win->rect.y;
		sf_GNW_win_refresh(win, reinterpret_cast<RECT*>(&updateRect), 0);
	}
}

void GameRender_DestroyOverlaySurface(WINinfo* win) {
	if (win->randY) {
		OverlaySurface* overlay = reinterpret_cast<OverlaySurface*>(win->randY);
		win->randY = nullptr;
		overlay->winType = -1;
		overlay->DestroySurface();
		sf_GNW_win_refresh(win, &win->wRect, 0);
	}
}

static BYTE* GetBuffer() {
	return (BYTE*)*(DWORD*)_screen_buffer;
}

static void Draw(WINinfo* win, BYTE* surface, long width, long height, long widthFrom, BYTE* toBuffer, long toWidth, RECT &rect, RECT* updateRect) {
	auto drawFunc = (win->flags & WinFlags::Transparent && win->wID) ? TransBufToBuf : BufToBuf;
	if (toBuffer) {
		drawFunc(surface, width, height, widthFrom, &toBuffer[rect.left - updateRect->left] + ((rect.top - updateRect->top) * toWidth), toWidth);
	} else {
		drawFunc(surface, width, height, widthFrom, &GetBuffer()[rect.left] + (rect.top * toWidth), toWidth); // copy to buffer instead of DD surface (buffering)
	}

	if (!win->randY) return;
	surface = &GameRender_GetOverlaySurface(win)[rect.left - win->rect.x] + ((rect.top - win->rect.y) * win->width);

	if (toBuffer) {
		TransBufToBuf(surface, width, height, widthFrom, &toBuffer[rect.left - updateRect->left] + ((rect.top - updateRect->top) * toWidth), toWidth);
	} else {
		TransBufToBuf(surface, width, height, widthFrom, &GetBuffer()[rect.left] + (rect.top * toWidth), toWidth);
	}
}

static void __fastcall sf_GNW_win_refresh(WINinfo* win, RECT* updateRect, BYTE* toBuffer) {
	if (win->flags & WinFlags::Hidden) return;
	RectList* rects;

	if (win->flags & WinFlags::Transparent && !*(DWORD*)_doing_refresh_all) {
		__asm {
			mov  eax, updateRect;
			mov  edx, ds:[_screen_buffer];
			call refresh_all_;
		}
		int w = (updateRect->right - updateRect->left) + 1;

		if (*ptr_mouse_is_hidden || !MouseIn(updateRect->left, updateRect->top, updateRect->right, updateRect->bottom)) {
			/*__asm {
				mov  eax, win;
				mov  edx, updateRect;
				call GNW_button_refresh_;
			}*/
			int h = (updateRect->bottom - updateRect->top) + 1;

			UpdateDDSurface(GetBuffer(), w, h, w, updateRect); // update the entire rectangle area

		} else {
			MouseShow(); // for updating background cursor area
			RECT mouseRect;
			__asm {
				lea  eax, mouseRect;
				mov  edx, eax;
				call mouse_get_rect_;
				mov  eax, updateRect;
				call rect_clip_;
				mov  rects, eax;
			}
			while (rects) { // updates everything except the cursor area
				/*__asm {
					mov  eax, win;
					mov  edx, rects;
					call GNW_button_refresh_;
				}*/

				int wRect = (rects->wRect.right - rects->wRect.left) + 1;
				int hRect = (rects->wRect.bottom - rects->wRect.top) + 1;

				UpdateDDSurface(&GetBuffer()[rects->wRect.left - updateRect->left] + (rects->wRect.top - updateRect->top) * w, wRect, hRect, w, &rects->wRect);

				RectList* free = rects;
				rects = rects->nextRect;
				sf_rect_free(free);
			}
		}
		return;
	}

	/* Allocates memory for 10 RectList (if no memory was allocated), returns the first Rect and removes it from the list */
	__asm call rect_malloc_;
	__asm mov  rects, eax;
	if (!rects) return;

	rects->rect.x = updateRect->left;
	rects->rect.y = updateRect->top;
	rects->rect.offx = updateRect->right;
	rects->rect.offy = updateRect->bottom;
	rects->nextRect = nullptr;

	/*
		If the border of the updateRect rectangle is located outside the window, then assign to rects->rect the border of the window rectangle
		Otherwise, rects->rect contains the borders from the update rectangle (updateRect)
	*/
	if (rects->wRect.left   < win->wRect.left)   rects->wRect.left   = win->wRect.left;
	if (rects->wRect.top    < win->wRect.top)    rects->wRect.top    = win->wRect.top;
	if (rects->wRect.right  > win->wRect.right)  rects->wRect.right  = win->wRect.right;
	if (rects->wRect.bottom > win->wRect.bottom) rects->wRect.bottom = win->wRect.bottom;

	if (rects->wRect.right < rects->wRect.left || rects->wRect.bottom < rects->wRect.top) {
		sf_rect_free(rects);
		return;
	}

	int widthFrom = win->width;
	int toWidth = (toBuffer) ? (updateRect->right - updateRect->left) + 1 : Gfx_GetGameWidthRes();

	WinClip(win, &rects, toBuffer);

	RectList* currRect = rects;
	while (currRect) {
		int width = (currRect->wRect.right - currRect->wRect.left) + 1;   // for current rectangle
		int height = (currRect->wRect.bottom - currRect->wRect.top) + 1;; // for current rectangle

		BYTE* surface;
		if (win->wID > 0) {
			__asm {
				mov  eax, win;
				mov  edx, currRect;
				call GNW_button_refresh_;
			}
			surface = &win->surface[currRect->wRect.left - win->rect.x] + ((currRect->wRect.top - win->rect.y) * win->width);
		} else {
			surface = new BYTE[height * width](); // black background
			widthFrom = width; // replace with rectangle
		}

		Draw(win, surface, width, height, widthFrom, toBuffer, toWidth, currRect->wRect, updateRect);

		if (win->wID == 0) delete[] surface;

		currRect = currRect->nextRect;
	}

	while (rects) {
		// copy all rectangles from the buffer to the DD surface (buffering)
		if (!toBuffer) {
			int width = (rects->rect.offx - rects->rect.x) + 1;
			int height = (rects->rect.offy - rects->rect.y) + 1;
			int widthFrom = toWidth;

			UpdateDDSurface(&GetBuffer()[rects->rect.x] + (rects->rect.y * widthFrom), width, height, widthFrom, &rects->wRect);
		}
		RectList* next = rects->nextRect;
		sf_rect_free(rects);
		rects = next;
	}

	if (!toBuffer && !*(DWORD*)_doing_refresh_all && !*ptr_mouse_is_hidden && MouseIn(updateRect->left, updateRect->top, updateRect->right, updateRect->bottom)) {
		MouseShow();
	}
}

static __declspec(naked) void GNW_win_refresh_hack() {
	__asm {
		push ebx; // toBuffer
		mov  ecx, eax;
		call sf_GNW_win_refresh;
		pop  ecx;
		retn;
	}
}

////////////////////////////////////////////////////////////////////////////////

static double fadeMulti;

static __declspec(naked) void palette_fade_to_hook() {
	__asm {
		push ebx; // _fade_steps
		fild [esp];
		fmul fadeMulti;
		fistp [esp];
		pop  ebx;
		jmp  fadeSystemPalette_;
	}
}

void Graphics_OnGameLoad() { // ResetShaders
	for (DWORD d = 0; d < shadersSize; d++) SAFERELEASE(shaders[d].Effect);
	shaders.clear();
	shadersSize = 0;
	LoadGlobalShader();
}

void Graphics_Init() {
	GraphicsMode = GetConfigInt("Graphics", "Mode", 0);
	if (GraphicsMode == 6) {
		windowStyle = WS_OVERLAPPED;
	} else if (GraphicsMode != 4 && GraphicsMode != 5) {
		GraphicsMode = 0;
	}

	if (GraphicsMode) {
		dlog("Applying DX9 graphics patch.", DL_INIT);
#ifdef WIN2K
#define _DLL_NAME "d3dx9_42.dll"
#else
#define _DLL_NAME "d3dx9_43.dll"
#endif
		HMODULE h = LoadLibraryEx(_DLL_NAME, 0, LOAD_LIBRARY_AS_DATAFILE);
		if (!h) {
			MessageBoxA(0, "You have selected DirectX graphics mode, but " _DLL_NAME " is missing.\n"
						   "Switch back to mode 0, or install an up to date version of DirectX.", "Error", MB_TASKMODAL | MB_ICONERROR);
#undef _DLL_NAME
			ExitProcess(-1);
		} else {
			FreeLibrary(h);
		}
		SafeWrite8(0x50FB6B, '2'); // Set call DirectDrawCreate2
		HookCall(0x44260C, game_init_hook);

		textureFilter = GetConfigInt("Graphics", "TextureFilter", 1);
		dlogr(" Done", DL_INIT);

		for each (const std::string& shaderFile in GetConfigList("Graphics", "GlobalShaderFile", "", 1024)) {
			if (shaderFile.length() > 3) gShaderFiles.push_back(sGlobalShader(shaderFile));
		}
		globalShadersActive = !gShaderFiles.empty();
	}

	fadeMulti = GetConfigInt("Graphics", "FadeMultiplier", 100);
	if (fadeMulti != 100) {
		dlog("Applying fade patch.", DL_INIT);
		HookCall(0x493B16, palette_fade_to_hook);
		fadeMulti = ((double)fadeMulti) / 100.0;
		dlogr(" Done", DL_INIT);
	}

	// Replace the srcCopy_ function with a pure MMX implementation
	MakeJump(0x4D36D4, BufToBuf); // buf_to_buf_
	// Replace the transSrcCopy_ function
	MakeJump(0x4D3704, TransBufToBuf); // trans_buf_to_buf_

	// Enable support for transparent interface windows
	const DWORD winBufferAddr [] = {
		0x4D5D46, // win_init_ (create screen_buffer)
		0x4D75E6  // win_clip_ (remove _buffering checking)
	};
	SafeWriteBatch<WORD>(0x9090, winBufferAddr);

	// Custom implementation of the GNW_win_refresh function
	MakeJump(0x4D6FD9, GNW_win_refresh_hack, 1);
	// Replace _screendump_buf with _screen_buffer for creating screenshots
	const DWORD scrdumpBufAddr[] = {0x4C8FD1, 0x4C900D};
	SafeWriteBatch<DWORD>(_screen_buffer, scrdumpBufAddr);
}

void Graphics_Exit() {
	if (GraphicsMode) {
		if (GraphicsMode == 5) {
			int data = windowTop | (windowLeft << 16);
			if (data >= 0 && data != windowData) SetConfigInt("Graphics", "WindowData", data);
		}
		CoUninitialize();
	}
}
