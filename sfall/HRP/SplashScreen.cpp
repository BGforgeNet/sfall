/*
 *    sfall
 *    Copyright (C) 2008-2021  The sfall team
 *
 */

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "..\Modules\Graphics.h"

#include "Image.h"

#include "SplashScreen.h"

namespace HRP
{

namespace sf = sfall;

// 0 - image will display at its original size
// 1 - image will stretch to fit the screen while maintaining its aspect ratio
// 2 - image will stretch to fill the screen
long SplashScreen::SPLASH_SCRN_SIZE;

static void __cdecl game_splash_screen_hack_scr_blit(BYTE* srcPixels, long srcWidth, long srcHeight, long srcX, long srcY, long width, long height, long x, long y) {
	RECT rect;
	long w = sf::Graphics::GetGameWidthRes();
	long h = sf::Graphics::GetGameHeightRes();

	// TODO: Load an alternative 32-bit BMP image or DirectX texture
	// stretch texture for DirectX
	if (SplashScreen::SPLASH_SCRN_SIZE == 1) {
		x = Image::GetAspectSize(w, h, (float)srcWidth, (float)srcHeight);

		BYTE* resizeBuff = new BYTE[w * h];
		Image::Scale(srcPixels, srcWidth, srcHeight, resizeBuff, w, h);

		// extract x/y image position
		if (x >= w) {
			y = x / w;
			x -= y * w;
		}

		rect.top = y;
		rect.bottom = (y + h) - 1;
		rect.left = x;
		rect.right = (rect.left + w) - 1;
		sf::Graphics::UpdateDDSurface(resizeBuff, w, h, w, &rect);

		delete[] resizeBuff;
		return;
	} else if (SplashScreen::SPLASH_SCRN_SIZE == 2) {
		BYTE* resizeBuff = new BYTE[sf::Graphics::GetGameWidthRes() * sf::Graphics::GetGameHeightRes()];
		Image::Scale(srcPixels, srcWidth, srcHeight, resizeBuff, sf::Graphics::GetGameWidthRes(), sf::Graphics::GetGameHeightRes());

		rect.top = 0;
		rect.left = 0;
		rect.right =sf:: Graphics::GetGameWidthRes() - 1;
		rect.bottom = sf::Graphics::GetGameHeightRes() - 1;
		sf::Graphics::UpdateDDSurface(resizeBuff, sf::Graphics::GetGameWidthRes(), sf::Graphics::GetGameHeightRes(), sf::Graphics::GetGameWidthRes(), &rect);

		delete[] resizeBuff;
		return;
	}

	// original size to center screen

	rect.left = (sf::Graphics::GetGameWidthRes() / 2) - (srcWidth / 2) + x;
	rect.right = (rect.left + srcWidth) - 1 ;

	rect.top = (sf::Graphics::GetGameHeightRes() / 2) - (srcHeight / 2) + y;
	rect.bottom = (rect.top + srcHeight) - 1;

	sf::Graphics::UpdateDDSurface(srcPixels, srcWidth, srcHeight, srcWidth, &rect);
}

// Fixes colored screen border when the index 0 of the palette contains a color with a non-black (zero) value
static void __fastcall Clear(fo::PALETTE* palette) {
	long minValue = (palette->B + palette->G + palette->R);
	if (minValue == 0) return;

	long index = 0;

	// search index of the darkest color in the palette
	for (size_t i = 1; i < 256; i++)
	{
		long rgbVal = palette[i].B + palette[i].G + palette[i].R;
		if (rgbVal < minValue) {
			minValue = rgbVal;
			index = i;
		}
	}
	if (index != 0) sf::Graphics::BackgroundClearColor(index);
}

static void __declspec(naked) game_splash_screen_hook() {
	__asm {
		call fo::funcoffs::db_fclose_;
		mov  ecx, ebp; // .rix palette
		jmp  Clear;
	}
}

void SplashScreen::init() {
	sf::HookCall(0x4444FC, game_splash_screen_hook);
	sf::MakeCall(0x44451E, game_splash_screen_hack_scr_blit, 1);
}

}
