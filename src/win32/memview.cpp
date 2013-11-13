#include "memview.h"

// Debugger Variables
HWND g_memViewHwnd;
BOOL g_memViewActive;

enum {
	MEM_RAM = 0,
	MEM_ROM,
	MEM_CPURAM,
	MEM_BIOS
};

static unsigned char memView;
static char debug_str[35000] = {0};
static int currentAddr;

void DisplayMemoryEditor()
{
	unsigned char * buf;
	unsigned int addrOff;
	if ( memView == MEM_RAM )
	{
		buf = memRAM;
		addrOff = 0x4000+currentAddr;
	}
	else if ( memView == MEM_ROM )
	{
		buf = memROM;
		addrOff = 0x200000+currentAddr;
	}
	else if ( memView == MEM_CPURAM )
	{
		buf = memCPURAM;
		addrOff = 0x0+currentAddr;
	}
	else if ( memView == MEM_BIOS )
	{
		buf = memBios;
		addrOff = 0xFF0000+currentAddr;
	}

	// Print the hex
	char tmpLine[2048];
	strcpy(debug_str, "");
	for(int i = 0, j; i < 30; i++)
	{
		sprintf(tmpLine, "[0x%06x]  ", addrOff+i*16);
		strcat(debug_str, tmpLine);
		for(j = 0; j < 16; j++)
		{
			sprintf(tmpLine, "  %02x", buf[currentAddr+(i*16)+j]);
			strcat(debug_str, tmpLine);
		}
		strcat(debug_str, "\r\n");
	}
	SetDlgItemText(g_memViewHwnd, IDC_MEMVIEW_DUMP, debug_str);
}

INT_PTR CALLBACK MemViewProc(
  HWND hwndDlg,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam
)
{
	SCROLLINFO si;
	bool updateMem = false;
    switch(uMsg)
    {
		case WM_INITDIALOG:
			g_memViewActive = true;
			memView = MEM_RAM;
			si.cbSize = sizeof(SCROLLINFO);
			si.fMask = SIF_ALL;
			si.nMin = 0;
			si.nMax = 0x800;
			si.nPos = 0;
			si.nPage = 0x20;
			SetScrollInfo(GetDlgItem(hwndDlg,IDC_MEMVIEW_VSCR),SB_CTL,&si,TRUE);
			currentAddr = si.nPos*16;
			return TRUE;
		break;
        case WM_COMMAND:
			switch(LOWORD(wParam))
            {
			case 0x05:
				// Update our memory
				si.cbSize = sizeof(SCROLLINFO);
				si.fMask = SIF_ALL;
				GetScrollInfo(GetDlgItem(hwndDlg,IDC_MEMVIEW_VSCR),SB_CTL,&si);
				currentAddr = si.nPos*16;
			break;
			case WM_DESTROY:
			case ID_MEMVIEW_CLOSE:
				DestroyWindow(hwndDlg);
				return TRUE;
			break;
			case ID_MEMVIEW_RAM:
				if ( memView != MEM_RAM )
				{
					memView = MEM_RAM;
					si.cbSize = sizeof(SCROLLINFO);
					si.fMask = SIF_ALL;
					GetScrollInfo(GetDlgItem(hwndDlg,IDC_MEMVIEW_VSCR),SB_CTL,&si);
					si.nPos = 0;
					SetScrollInfo(GetDlgItem(hwndDlg,IDC_MEMVIEW_VSCR),SB_CTL,&si,TRUE);
					currentAddr = 0x0;
				}
			break;
			case ID_MEMVIEW_ROM:
				if ( memView != MEM_ROM )
				{
					memView = MEM_ROM;
					si.cbSize = sizeof(SCROLLINFO);
					si.fMask = SIF_ALL;
					GetScrollInfo(GetDlgItem(hwndDlg,IDC_MEMVIEW_VSCR),SB_CTL,&si);
					si.nPos = 0;
					SetScrollInfo(GetDlgItem(hwndDlg,IDC_MEMVIEW_VSCR),SB_CTL,&si,TRUE);
					currentAddr = 0x0;
				}
			break;
			case ID_MEMVIEW_CPURAM:
				if ( memView != MEM_CPURAM )
				{
					memView = MEM_CPURAM;
					si.cbSize = sizeof(SCROLLINFO);
					si.fMask = SIF_ALL;
					GetScrollInfo(GetDlgItem(hwndDlg,IDC_MEMVIEW_VSCR),SB_CTL,&si);
					si.nPos = 0;
					SetScrollInfo(GetDlgItem(hwndDlg,IDC_MEMVIEW_VSCR),SB_CTL,&si,TRUE);
					currentAddr = 0x0;
				}
			break;
			case ID_MEMVIEW_BIOS:
				if ( memView != MEM_BIOS )
				{
					memView = MEM_BIOS;
					si.cbSize = sizeof(SCROLLINFO);
					si.fMask = SIF_ALL;
					GetScrollInfo(GetDlgItem(hwndDlg,IDC_MEMVIEW_VSCR),SB_CTL,&si);
					si.nPos = 0;
					SetScrollInfo(GetDlgItem(hwndDlg,IDC_MEMVIEW_VSCR),SB_CTL,&si,TRUE);
					currentAddr = 0x0;
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
				currentAddr = si.nPos*16;
			}
			// Disassemble
			break;
		case WM_DESTROY:
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
