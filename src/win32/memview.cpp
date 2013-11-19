#include "memview.h"

// Debugger Variables
HWND g_memViewHwnd;
BOOL g_memViewActive;

static char debug_str[4096] = {0};
static int currentPos;
static HDC memDC;
static HDC dataDC;
static HFONT memFont;
static int fixedFontHeight;
static int fixedFontWidth;

static int HexBackColorR = 240;	// Grey
static int HexBackColorG = 240;
static int HexBackColorB = 240;
static int HexForeColorR = 0;		// Black
static int HexForeColorG = 0;
static int HexForeColorB = 0;

// Uses Tokenize
inline bool IsAllHex(char * must_be_hex)
{
	char copy_of_param [1024];
	return (strtok (strcpy(copy_of_param, must_be_hex),"0123456789ABCDEFabcdef") == NULL);
}

void DisplayMemoryEditor()
{
	// Print the hex
	char tmpLine[2048];
	dataDC = GetDC(GetDlgItem(g_memViewHwnd,IDC_MEMVIEW_RAM));
	SelectObject (dataDC, memFont);
	SetTextAlign(dataDC, TA_UPDATECP | TA_TOP | TA_LEFT);

	for(int i = 0; i < 30; i++)
	{
		sprintf(tmpLine, "[0x%06x]  ", currentPos+0x4000+i*16);
		strcpy(debug_str, tmpLine);
		for(int j = 0; j < 16; j++)
		{
			if ( currentPos+i*16+j >= 0x4000 )
			{
				i = 30; // break out early
				break;
			}
			sprintf(tmpLine, "  %02x", memRAM[currentPos+i*16+j]);
			strcat(debug_str, tmpLine);
		}
		//strcat(debug_str, "\r\n");
		MoveToEx(dataDC,4, i*fixedFontHeight, (LPPOINT)NULL);
		SetTextColor(dataDC,RGB(HexForeColorR,HexForeColorG,HexForeColorB));	//addresses text color			000 = black, 255255255 = white
		SetBkColor(dataDC,RGB(HexBackColorR,HexBackColorG,HexBackColorB));		//addresses back color
		TextOut(dataDC, 0, 0, debug_str, strlen(debug_str));
	}
	SetTextColor(dataDC,RGB(0,0,0));
	SetBkColor(dataDC,RGB(0,0,0));
	MoveToEx(dataDC,0,0,NULL);	
	ReleaseDC(g_memViewHwnd, dataDC);

	dataDC = GetDC(GetDlgItem(g_memViewHwnd,IDC_MEMVIEW_CPURAM));
	SelectObject (dataDC, memFont);
	SetTextAlign(dataDC, TA_UPDATECP | TA_TOP | TA_LEFT);

	//SetDlgItemText(g_memViewHwnd, IDC_MEMVIEW_RAM, debug_str);
	for(int i = 0; i < 32; i++)
	{
		sprintf(tmpLine, "[0x%02x] ", i*8);
		//strcat(debug_str, tmpLine);
		strcpy(debug_str, tmpLine);
		for(int j = 0; j < 8; j++)
		{
			sprintf(tmpLine, "%02x", memCPURAM[i*8+j]);
			strcat(debug_str, tmpLine);
		}
		//strcat(debug_str, "\r\n");
		MoveToEx(dataDC, 4, i*fixedFontHeight, (LPPOINT)NULL);
		SetTextColor(dataDC,RGB(HexForeColorR,HexForeColorG,HexForeColorB));	//addresses text color			000 = black, 255255255 = white
		SetBkColor(dataDC,RGB(HexBackColorR,HexBackColorG,HexBackColorB));		//addresses back color
		TextOut(dataDC, 0, 0, debug_str, strlen(debug_str));
	}
	//SetDlgItemText(g_memViewHwnd, IDC_MEMVIEW_CPURAM, debug_str);
	SetTextColor(dataDC,RGB(0,0,0));
	SetBkColor(dataDC,RGB(0,0,0));
	MoveToEx(dataDC,0,0,NULL);	
	ReleaseDC(g_memViewHwnd, dataDC);
}

// 000000 -> 0000FF	CPU Internal RAM (Timers / DMA / RTC / IO / etc.)
// 004000 -> 006BFF	Work RAM
// 006C00 -> 006FFF	Bios work RAM
// 007000 -> 007FFF	Z80 Shared RAM
// 008000 -> 008FFF	Video registers
// 009000 -> 00BFFF	Video RAM

INT_PTR CALLBACK MemViewProc(
  HWND hwndDlg,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam
)
{
	HDC hdc;
	PAINTSTRUCT ps;
	HGDIOBJ old;
	TEXTMETRIC tm;
	char lpszPassword[1024]; // overflow, but you can't automate it
	WORD cchPassword;
	char * lpszCheck;
	SCROLLINFO si;
	int addr;
	bool updateMem = false;
    switch(uMsg)
    {
		case WM_INITDIALOG:
			g_memViewActive = true;
			memFont = CreateFont(14, 8, /*Height,Width*/
				0,0, /*escapement,orientation*/
				FW_REGULAR,FALSE,FALSE,FALSE, /*weight, italic, underline, strikeout*/
				ANSI_CHARSET,OUT_DEVICE_PRECIS,CLIP_MASK, /*charset, precision, clipping*/
				DEFAULT_QUALITY, DEFAULT_PITCH, /*quality, and pitch*/
				"Courier New"); /*font name*/
			hdc = GetDC(GetDesktopWindow());
			old = SelectObject(hdc,memFont);
			GetTextMetrics(hdc,&tm);
			fixedFontHeight = tm.tmHeight;
			fixedFontWidth = tm.tmAveCharWidth;
			SelectObject(hdc,old);
			DeleteDC(hdc);
			si.cbSize = sizeof(SCROLLINFO);
			si.fMask = SIF_ALL;
			si.nMin = 0;
			si.nMax = 0x800; // RAM 0x8000 / 16
			si.nPos = 0;
			si.nPage = 0x20;
			SetScrollInfo(GetDlgItem(hwndDlg,IDC_MEMVIEW_VSCR),SB_CTL,&si,TRUE);
			currentPos = si.nPos*16;
			return TRUE;
		break;
        case WM_COMMAND:
			switch(LOWORD(wParam))
            {
			case WM_DESTROY:
				DestroyWindow(hwndDlg);
				return TRUE;
			break;
			case IDC_MEMVIEW_GOTO:
				// Get number of characters. 
                cchPassword = (WORD) SendDlgItemMessage(hwndDlg, 
                                                        IDC_MEMVIEW_GOTO_EDIT, 
                                                        EM_LINELENGTH, 
                                                        (WPARAM) 0, 
                                                        (LPARAM) 0);

                // Put the number of characters into first word of buffer.
				if ( cchPassword > 1024 )
					cchPassword = (DWORD)1024;

                *((LPWORD)lpszPassword) = cchPassword;

                // Get the characters. 
                SendDlgItemMessage(hwndDlg, 
                                    IDC_MEMVIEW_GOTO_EDIT,
                                    EM_GETLINE,
                                    (WPARAM) 0,       // line 0 
                                    (LPARAM) lpszPassword); 

                // Nullo-terminate the string. 
                lpszPassword[cchPassword] = 0;

				// Format 0x0020016c
				if ( strstr(lpszPassword, "0x") == lpszPassword )
				{
					lpszCheck = &(lpszPassword[2]);
				}	 
				else
				{
					lpszCheck = lpszPassword;
				}

				if ( IsAllHex(lpszCheck) && strlen(lpszCheck) <= 8 )
				{
					addr = strtol(lpszCheck, NULL, 16);
					currentPos = addr-0x4000;
					DisplayMemoryEditor();
				}
				else
				{
					MessageBox(hwndDlg, "Invalid Address.. Format: 0x40016C or 40016C", "Alert", MB_OK);
				}
			break;
			}
			break;
		case WM_VSCROLL:
			if ( lParam )
			{
				si.fMask = SIF_ALL;
				si.cbSize = sizeof(SCROLLINFO);
				GetScrollInfo((HWND)lParam,SB_CTL,&si);
				switch(LOWORD(wParam)) {
					case SB_ENDSCROLL:
					case SB_TOP:
					case SB_BOTTOM: break;
					case SB_LINEUP: si.nPos--; if ( si.nPos < 0 ) si.nPos = 0; break;
					case SB_LINEDOWN: si.nPos++; break;
					case SB_PAGEUP: si.nPos-=si.nPage; if ( si.nPos < 0 ) si.nPos = 0; break;
					case SB_PAGEDOWN: si.nPos+=si.nPage; break;
					case SB_THUMBPOSITION: //break;
					case SB_THUMBTRACK: si.nPos = si.nTrackPos; if ( si.nPos < 0 ) si.nPos = 0; break;
				}
				SetScrollInfo((HWND)lParam,SB_CTL,&si,TRUE);
				currentPos = si.nPos*16;
			}
			// Disassemble
			break;
		case WM_PAINT:
			hdc = BeginPaint(g_memViewHwnd, &ps);
			EndPaint(g_memViewHwnd, &ps);
			DisplayMemoryEditor();
			break;
		case WM_DESTROY:
			ReleaseDC(g_memViewHwnd, memDC);
			DeleteObject(memFont);
			g_memViewActive = false;
			break;
		default:
			break;
    }

    return FALSE;
}

void openMemoryEditor()
{
	if ( g_memViewActive == false )
	{
		g_memViewHwnd = CreateDialog(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_MEMVIEW), 0/*hwnd*/, MemViewProc);
		if ( g_memViewHwnd )
		{
			ShowWindow(g_memViewHwnd, SW_SHOWNORMAL);
			SetForegroundWindow(g_memViewHwnd);
		}
	}
}
