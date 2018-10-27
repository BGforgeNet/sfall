/*
 *    sfall
 *    Copyright (C) 2009, 2010  Mash (Matt Wells, mashw at bigpond dot net dot au)
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

#include <stdio.h>
#include "FalloutEngine.h"
#include "HeroAppearance.h"
#include "SuperSave.h"

//extern
DWORD LSPageOffset = 0;

int LSButtDN = 0;

static const char* filename = "%s\\savegame\\slotdat.ini";

static const DWORD AddPageOffset01Addrs[] = {
	0x47B929, 0x47D8DB, 0x47D9B0, 0x47DA34, 0x47DABF, 0x47DB58, 0x47DBE9,
	0x47DC9C, 0x47EC77, 0x47F5AB, 0x47F694, 0x47F6EB, 0x47F7FB, 0x47F892,
	0x47FB86, 0x47FC3A, 0x47FCF2, 0x480117, 0x4801CF, 0x480234, 0x480310,
	0x4803F3, 0x48049F, 0x480512, 0x4805F2, 0x480767, 0x4807E6, 0x480839,
	0x4808D3,
};

//--------------------------------------
void SavePageOffsets() {
	char SavePath[MAX_PATH], buffer[6];

	sprintf_s(SavePath, MAX_PATH, filename, *(char**)_patches);

	_itoa_s(*(DWORD*)_slot_cursor, buffer, 10);
	WritePrivateProfileString("POSITION", "ListNum", buffer, SavePath);
	_itoa_s(LSPageOffset, buffer, 10);
	WritePrivateProfileString("POSITION", "PageOffset", buffer, SavePath);
}

//------------------------------------------
static void __declspec(naked) save_page_offsets(void) {
	__asm {
		// save last slot position values to file
		call SavePageOffsets
		// restore original code
		mov  eax, dword ptr ds:[_lsgwin]
		ret
	}
}

//--------------------------------------
void LoadPageOffsets() {
	char LoadPath[MAX_PATH];

	sprintf_s(LoadPath, MAX_PATH, filename, *(char**)_patches);

	*(DWORD*)_slot_cursor = GetPrivateProfileInt("POSITION", "ListNum", 0, LoadPath);
	if (*(DWORD*)_slot_cursor > 9) {
		*(DWORD*)_slot_cursor = 0;
	}
	LSPageOffset = GetPrivateProfileInt("POSITION", "PageOffset", 0, LoadPath);
	if (LSPageOffset > 9990) {
		LSPageOffset = 0;
	}
}

//------------------------------------------
static void __declspec(naked) load_page_offsets(void) {
	__asm {
		// load last slot position values from file
		call LoadPageOffsets
		// restore original code
		mov  edx, 0x50A480  // ASCII "SAV"
		ret
	}
}

//------------------------------------------
static void CreateButtons() {
	DWORD winRef = *(DWORD*)_lsgwin;
	// left button -10       | X | Y | W | H |HOn |HOff |BDown |BUp |PicUp |PicDown |? |ButType
	WinRegisterButton(winRef, 100, 56, 24, 20, -1, 0x500, 0x54B, 0x14B, 0, 0, 0, 32);
	// left button -100
	WinRegisterButton(winRef,  68, 56, 24, 20, -1, 0x500, 0x549, 0x149, 0, 0, 0, 32);
	// right button +10
	WinRegisterButton(winRef, 216, 56, 24, 20, -1, 0x500, 0x54D, 0x14D, 0, 0, 0, 32);
	// right button +100
	WinRegisterButton(winRef, 248, 56, 24, 20, -1, 0x500, 0x551, 0x151, 0, 0, 0, 32);
	// Set Number button
	WinRegisterButton(winRef, 140, 56, 60, 20, -1, -1, 'p', -1, 0, 0, 0, 32);
}

static void __declspec(naked) create_page_buttons(void) {
	__asm {
		call CreateButtons;
		// restore original code
		mov  eax, 0x65;
		ret;
	}
}

//------------------------------------------------------
void SetPageNum() {
	DWORD WinRef = *(DWORD*)_lsgwin; // load/save winref
	if (WinRef == NULL) {
		return;
	}
	WINinfo *SaveLoadWin = GetWinStruct(WinRef);
	if (SaveLoadWin->surface == NULL) {
		return;
	}

	BYTE ConsoleGold = *(BYTE*)_YellowColor; // palette offset stored in mem - text colour

	char TempText[32];
	unsigned int TxtMaxWidth = GetMaxCharWidth() * 8; // GetTextWidth(TempText);
	unsigned int TxtWidth = 0;

	DWORD NewTick = 0, OldTick = 0;
	int button = 0, exitFlag = 0, numpos = 0;
	char Number[5], blip = '_';

	DWORD tempPageOffset = -1;

	while (!exitFlag) {
		NewTick = GetTickCount(); // timer for redraw
		if (OldTick > NewTick) {
			OldTick = NewTick;
		}
		if (NewTick - OldTick > 166) { // time to draw
			OldTick = NewTick;

			if (blip == '_') {
				blip = ' ';
			} else {
				blip = '_';
			}

			sprintf_s(TempText, 32, "#%d%c", tempPageOffset / 10 + 1, '_');
			if (tempPageOffset == -1) {
				sprintf_s(TempText, 32, "#%c", '_');
			}
			TxtWidth = GetTextWidth(TempText);

			sprintf_s(TempText, 32, "#%d%c", tempPageOffset / 10 + 1, blip);
			if (tempPageOffset == -1) {
				sprintf_s(TempText, 32, "#%c", blip);
			}

			// fill over text area with consol black colour
			for (unsigned int y = SaveLoadWin->width * 52; y < SaveLoadWin->width * 82; y = y + SaveLoadWin->width) {
				memset(SaveLoadWin->surface + y + 170 - TxtMaxWidth / 2, 0xCF, TxtMaxWidth);
			}

			PrintText(TempText, ConsoleGold, 170 - TxtWidth / 2, 60, TxtWidth, SaveLoadWin->width, SaveLoadWin->surface);
			RedrawWin(WinRef);
		}

		button = check_buttons();
		if (button >= '0' && button <= '9') {
			if (numpos < 4) {
				Number[numpos] = button;
				Number[numpos + 1] = '\0';
				numpos++;
				if (Number[0] == '0') {
					numpos = 0;
					tempPageOffset = -1;
				} else {
					tempPageOffset = (atoi(Number) - 1) * 10;
				}
			}
			//else exitFlag=-1;
		} else if (button == 0x08 && numpos) {
			numpos--;
			Number[numpos] = '\0';
			if (!numpos) {
				tempPageOffset = -1;
			} else {
				tempPageOffset = (atoi(Number) - 1) * 10;
			}
		} else if (button == 0x0D || button == 0x20 || button == 'p' || button == 'P') {
			exitFlag = -1; // Enter, Space or P Keys
		} else if (button == 0x1B) {
			tempPageOffset = -1, exitFlag = -1; // Esc key
		}
	}

	if (tempPageOffset != -1 && tempPageOffset <= 9990) {
		LSPageOffset = tempPageOffset;
	}

	SaveLoadWin = NULL;
}

//------------------------------------------
static long __fastcall CheckPage(long button) {
	switch (button) {
		case 0x14B:                        // left button
			if (LSPageOffset >= 10) LSPageOffset -= 10;
			__asm call gsound_red_butt_press_;
			break;
		case 0x149:                        // fast left PGUP button
			if (LSPageOffset < 100) {
				LSPageOffset = 0;          // First Page
			} else {
				LSPageOffset -= 100;
			}
			__asm call gsound_red_butt_press_;
			break;
		case 0x14D:                        // right button
			if (LSPageOffset <= 9980) LSPageOffset += 10;
			__asm call gsound_red_butt_press_;
			break;
		case 0x151:                        // fast right PGDN button
			if (LSPageOffset > 9890) {
				LSPageOffset = 9990;       // Last Page
			} else {
				LSPageOffset += 100;
			}
			__asm call gsound_red_butt_press_;
			break;
		case 'p':                          // p/P button pressed - start SetPageNum func
		case 'P':
			SetPageNum();
			break;
		default:
			if (button < 0x500) return 1;  // button in down state
	}

	LSButtDN = button;
	return 0;
}

static void __declspec(naked) check_page_buttons(void) {
	__asm {
		pushad;
		mov  ecx, eax;
		call CheckPage;
		test eax, eax;
		popad;
		jnz  CheckUp;
		add  dword ptr ds:[esp], 26;        // set return to button pressed code
		jmp  GetSlotList_;    // reset page save list func
CheckUp:
		// restore original code
		cmp  eax, 0x148;                    // up button
		ret;
	}
}

//------------------------------------------
void DrawPageText() {
	int WinRef = *(DWORD*)_lsgwin; // load/save winref
	if (WinRef == NULL) {
		return;
	}
	WINinfo *SaveLoadWin = GetWinStruct(WinRef);
	if (SaveLoadWin->surface == NULL) {
		return;
	}

	// fill over text area with consol black colour
	for (unsigned int y = SaveLoadWin->width * 52; y < SaveLoadWin->width * 82; y = y + SaveLoadWin->width) {
		memset(SaveLoadWin->surface + 50 + y, 0xCF, 240);
	}

	BYTE ConsoleGreen = *(BYTE*)_GreenColor; // palette offset stored in mem - text colour
	BYTE ConsoleGold = *(BYTE*)_YellowColor; // palette offset stored in mem - text colour
	BYTE Colour = ConsoleGreen;

	char TempText[32];
	sprintf_s(TempText, 32, "[ %d ]", LSPageOffset / 10 + 1);

	unsigned int TxtWidth = GetTextWidth(TempText);
	PrintText(TempText, Colour, 170 - TxtWidth / 2, 60, TxtWidth, SaveLoadWin->width, SaveLoadWin->surface);

	if (LSButtDN == 0x549) {
		Colour = ConsoleGold;
	} else {
		Colour = ConsoleGreen;
	}
	strcpy_s(TempText, 12, "<<");
	TxtWidth = GetTextWidth(TempText);
	PrintText(TempText, Colour, 80 - TxtWidth / 2, 60, TxtWidth, SaveLoadWin->width, SaveLoadWin->surface);

	if (LSButtDN == 0x54B) {
		Colour = ConsoleGold;
	} else {
		Colour = ConsoleGreen;
	}
	strcpy_s(TempText, 12, "<");
	TxtWidth = GetTextWidth(TempText);
	PrintText(TempText, Colour, 112 - TxtWidth / 2, 60, TxtWidth, SaveLoadWin->width, SaveLoadWin->surface);

	if (LSButtDN == 0x551) {
		Colour = ConsoleGold;
	} else {
		Colour = ConsoleGreen;
	}
	strcpy_s(TempText, 12, ">>");
	TxtWidth = GetTextWidth(TempText);
	PrintText(TempText, Colour, 260 - TxtWidth / 2, 60, TxtWidth, SaveLoadWin->width, SaveLoadWin->surface);

	if (LSButtDN == 0x54D) {
		Colour = ConsoleGold;
	} else {
		Colour = ConsoleGreen;
	}
	strcpy_s(TempText, 12, ">");
	TxtWidth = GetTextWidth(TempText);
	PrintText(TempText, Colour, 228 - TxtWidth / 2, 60, TxtWidth, SaveLoadWin->width, SaveLoadWin->surface);

	SaveLoadWin = NULL;
}

//------------------------------------------
static void __declspec(naked) draw_page_text(void) {
	__asm {
		pushad
		call DrawPageText
		popad
		// restore original code
		mov  ebp, 0x57
		ret
	}
}

//------------------------------------------
// add page num offset when reading and writing various save data files
static void __declspec(naked) AddPageOffset01(void) {
	__asm {
		mov  eax, dword ptr ds:[_slot_cursor] // list position 0-9
		add  eax, LSPageOffset // add page num offset
		ret
	}
}

//------------------------------------------
// getting info for the 10 currently displayed save slots from save.dats
static void __declspec(naked) AddPageOffset02(void) {
	__asm {
		push 0x50A514;          // ASCII "SAVE.DAT"
		lea  eax, [ebx + 1];
		add  eax, LSPageOffset; // add page num offset
		mov  edx, 0x47E5E9;     // ret addr
		jmp  edx;
	}
}

//------------------------------------------
// printing current 10 slot numbers
static void __declspec(naked) AddPageOffset03(void) {
	__asm {
		inc  eax
		add  eax, LSPageOffset // add page num offset
		mov  bl, byte ptr ss:[esp+0x10] // add 4 bytes - func ret addr
		ret
	}
}

//--------------------------------------------------------------------------
void EnableSuperSaving() {

	// save/load button setup func
	MakeCall(0x47D80D, create_page_buttons);

	// Draw button text
	MakeCall(0x47E6E8, draw_page_text);

	// check save buttons
	MakeCall(0x47BD49, check_page_buttons);

	// check load buttons
	MakeCall(0x47CB1C, check_page_buttons);

	// save current page and list positions to file on load/save scrn exit
	MakeCall(0x47D828, save_page_offsets);

	// load saved page and list positions from file
	MakeCall(0x47B82B, load_page_offsets);

	// Add Load/Save page offset to Load/Save folder number/////////////////
	for (int i = 0; i < sizeof(AddPageOffset01Addrs) / 4; i++) {
		MakeCall(AddPageOffset01Addrs[i], AddPageOffset01);
	}

	MakeJump(0x47E5E1, AddPageOffset02);

	MakeCall(0x47E756, AddPageOffset03);
}