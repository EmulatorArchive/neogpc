#include "debugger.h"

#ifdef NEOGPC_DEBUGGER

// Debugger Variables
HWND g_tlcs900hDebugHwnd;
BOOL g_tlcs900hActive;
BOOL g_tlcs900hUpdateDebug;

HWND g_Z80DebugHwnd;
BOOL g_Z80Active;
BOOL g_Z80UpdateDebug;

static char debug_str[35000] = {0};

// Uses Tokenize
inline bool IsAllHex(char * must_be_hex)
{
	char copy_of_param [1024];
	return (strtok (strcpy(copy_of_param, must_be_hex),"0123456789ABCDEFabcdef") == NULL);
}

// Update the registers for the TLCS900h and the Z80
void UpdateRegs(HWND hwndDlg)
{
	char regStr[16];

	sprintf(regStr, "%06x", gen_regsXWA0);
	SetDlgItemText(hwndDlg, IDC_XWA0, (LPCSTR)regStr);

	sprintf(regStr, "%06x", gen_regsXBC0);
	SetDlgItemText(hwndDlg, IDC_XBC0, (LPCSTR)regStr);

	sprintf(regStr, "%06x", gen_regsXDE0);
	SetDlgItemText(hwndDlg, IDC_XDE0, (LPCSTR)regStr);

	sprintf(regStr, "%06x", gen_regsXHL0);
	SetDlgItemText(hwndDlg, IDC_XHL0, (LPCSTR)regStr);

	sprintf(regStr, "%06x", gen_regsXWA1);
	SetDlgItemText(hwndDlg, IDC_XWA1, (LPCSTR)regStr);

	sprintf(regStr, "%06x", gen_regsXBC1);
	SetDlgItemText(hwndDlg, IDC_XBC1, (LPCSTR)regStr);

	sprintf(regStr, "%06x", gen_regsXDE1);
	SetDlgItemText(hwndDlg, IDC_XDE1, (LPCSTR)regStr);

	sprintf(regStr, "%06x", gen_regsXHL1);
	SetDlgItemText(hwndDlg, IDC_XHL1, (LPCSTR)regStr);

	sprintf(regStr, "%06x", gen_regsXWA2);
	SetDlgItemText(hwndDlg, IDC_XWA2, (LPCSTR)regStr);

	sprintf(regStr, "%06x", gen_regsXBC2);
	SetDlgItemText(hwndDlg, IDC_XBC2, (LPCSTR)regStr);

	sprintf(regStr, "%06x", gen_regsXDE2);
	SetDlgItemText(hwndDlg, IDC_XDE2, (LPCSTR)regStr);

	sprintf(regStr, "%06x", gen_regsXHL2);
	SetDlgItemText(hwndDlg, IDC_XHL2, (LPCSTR)regStr);

	sprintf(regStr, "%06x", gen_regsXWA3);
	SetDlgItemText(hwndDlg, IDC_XWA3, (LPCSTR)regStr);

	sprintf(regStr, "%06x", gen_regsXBC3);
	SetDlgItemText(hwndDlg, IDC_XBC3, (LPCSTR)regStr);

	sprintf(regStr, "%06x", gen_regsXDE3);
	SetDlgItemText(hwndDlg, IDC_XDE3, (LPCSTR)regStr);

	sprintf(regStr, "%06x", gen_regsXHL3);
	SetDlgItemText(hwndDlg, IDC_XHL3, (LPCSTR)regStr);

	sprintf(regStr, "%06x", gen_regsXIX);
	SetDlgItemText(hwndDlg, IDC_XIX, (LPCSTR)regStr);

	sprintf(regStr, "%06x", gen_regsXIY);
	SetDlgItemText(hwndDlg, IDC_XIY, (LPCSTR)regStr);

	sprintf(regStr, "%06x", gen_regsXIZ);
	SetDlgItemText(hwndDlg, IDC_XIZ, (LPCSTR)regStr);

	sprintf(regStr, "%06x", gen_regsXSP);
	SetDlgItemText(hwndDlg, IDC_XSP, (LPCSTR)regStr);

	sprintf(regStr, "%06x", gen_regsPC);
	SetDlgItemText(hwndDlg, IDC_PC, (LPCSTR)regStr);

	sprintf(regStr, "%06x", gen_regsSR);
	SetDlgItemText(hwndDlg, IDC_SR, (LPCSTR)regStr);

	sprintf(regStr, "%06x", gen_regsXSSP);
	SetDlgItemText(hwndDlg, IDC_XSSP, (LPCSTR)regStr);

	sprintf(regStr, "%06x", gen_regsXNSP);
	SetDlgItemText(hwndDlg, IDC_XNSP, (LPCSTR)regStr);

	if ( gen_regsSR & 0x01 )
		SendMessage( GetDlgItem( hwndDlg, IDC_CF ), BM_SETCHECK, BST_CHECKED, 0);
	else
		SendMessage( GetDlgItem( hwndDlg, IDC_CF ), BM_SETCHECK, BST_UNCHECKED, 0);
	if ( gen_regsSR & 0x02 )
		SendMessage( GetDlgItem( hwndDlg, IDC_NF ), BM_SETCHECK, BST_CHECKED, 0);
	else
		SendMessage( GetDlgItem( hwndDlg, IDC_NF ), BM_SETCHECK, BST_UNCHECKED, 0);
	if ( gen_regsSR & 0x04 )
		SendMessage( GetDlgItem( hwndDlg, IDC_VF ), BM_SETCHECK, BST_CHECKED, 0);
	else
		SendMessage( GetDlgItem( hwndDlg, IDC_VF ), BM_SETCHECK, BST_UNCHECKED, 0);
	if ( gen_regsSR & 0x10 )
		SendMessage( GetDlgItem( hwndDlg, IDC_HF ), BM_SETCHECK, BST_CHECKED, 0);
	else
		SendMessage( GetDlgItem( hwndDlg, IDC_HF ), BM_SETCHECK, BST_UNCHECKED, 0);
	if ( gen_regsSR & 0x40 )
		SendMessage( GetDlgItem( hwndDlg, IDC_ZF ), BM_SETCHECK, BST_CHECKED, 0);
	else
		SendMessage( GetDlgItem( hwndDlg, IDC_ZF ), BM_SETCHECK, BST_UNCHECKED, 0);
	if ( gen_regsSR & 0x80 )
		SendMessage( GetDlgItem( hwndDlg, IDC_SF ), BM_SETCHECK, BST_CHECKED, 0);
	else
		SendMessage( GetDlgItem( hwndDlg, IDC_SF ), BM_SETCHECK, BST_UNCHECKED, 0);

	// Update the Z80 regs
	sprintf(regStr, "%04x", z80Regs[Z80AF]);
	SetDlgItemText(hwndDlg, IDC_Z80_AF, (LPCSTR)regStr);

	sprintf(regStr, "%04x", z80Regs[Z80BC]);
	SetDlgItemText(hwndDlg, IDC_Z80_BC, (LPCSTR)regStr);

	sprintf(regStr, "%04x", z80Regs[Z80DE]);
	SetDlgItemText(hwndDlg, IDC_Z80_DE, (LPCSTR)regStr);

	sprintf(regStr, "%04x", z80Regs[Z80HL]);
	SetDlgItemText(hwndDlg, IDC_Z80_HL, (LPCSTR)regStr);

	sprintf(regStr, "%04x", z80Regs[Z80AF2]);
	SetDlgItemText(hwndDlg, IDC_Z80_AF2, (LPCSTR)regStr);

	sprintf(regStr, "%04x", z80Regs[Z80BC2]);
	SetDlgItemText(hwndDlg, IDC_Z80_BC2, (LPCSTR)regStr);

	sprintf(regStr, "%04x", z80Regs[Z80DE2]);
	SetDlgItemText(hwndDlg, IDC_Z80_DE2, (LPCSTR)regStr);

	sprintf(regStr, "%04x", z80Regs[Z80HL2]);
	SetDlgItemText(hwndDlg, IDC_Z80_HL2, (LPCSTR)regStr);

	sprintf(regStr, "%04x", z80Regs[Z80IX]);
	SetDlgItemText(hwndDlg, IDC_Z80_IX, (LPCSTR)regStr);

	sprintf(regStr, "%04x", z80Regs[Z80IY]);
	SetDlgItemText(hwndDlg, IDC_Z80_IY, (LPCSTR)regStr);

	sprintf(regStr, "%04x", Z80IFF);
	SetDlgItemText(hwndDlg, IDC_Z80_IFF, (LPCSTR)regStr);

	sprintf(regStr, "%04x", Z80IM);
	SetDlgItemText(hwndDlg, IDC_Z80_IM, (LPCSTR)regStr);

	sprintf(regStr, "%04x", z80Regs[Z80PC]);
	SetDlgItemText(hwndDlg, IDC_Z80_PC, (LPCSTR)regStr);

	sprintf(regStr, "%04x", z80Regs[Z80SP]);
	SetDlgItemText(hwndDlg, IDC_Z80_SP, (LPCSTR)regStr);
}

// Decode X number of lines
void DisassembleTlcs900h(HWND hwndDlg, unsigned int addr)
{
	char tmpBuf[1024];
	strcpy(debug_str, ""); // null it out
	
	bool inBios = false;
	if ( addr >= 0xFF0000 )
		inBios = true;
	int incStep = 0;
	for(int i = 0; i < 28; i++)
	{
		if ( addr == gen_regsPC )
			sprintf(tmpBuf, "> %06x: %s", addr, neogpc_asmprint(addr));
		else
			sprintf(tmpBuf, "%06x: %s", addr, neogpc_asmprint(addr));
		strcat(debug_str, tmpBuf);
		strcat(debug_str, "\r\n");
		incStep = neogpc_asminc(addr);
		if ( incStep == 0 )
			incStep = 1;
		addr += incStep;
		if ( inBios == false && addr >= 0x400000 )
		{
			inBios = true;
			addr = 0xFF0000;
		} else if ( inBios == true && addr > 0xFFFFFF )
		{
			// Can't go past here!
			break;
		}
	}
	SetDlgItemText(hwndDlg, IDC_TLCS900HD_OPCODE_LIST, debug_str);
	UpdateRegs(hwndDlg);
}

INT_PTR CALLBACK TLCS900hProc(
  HWND hwndDlg,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam
)
{
	char lpszPassword[1024]; // overflow, but you can't automate it
	WORD cchPassword;
	char * lpszCheck;
	char listStr[16];
	int addr, bpIdx;
	SCROLLINFO si;
	bool updateDisas = false;
	int incStep;
    switch(uMsg)
    {
		case WM_INITDIALOG:
			g_tlcs900hActive = true;
			g_tlcs900hUpdateDebug = true;
			//setupTLCS900hDebugger(g_tlcs900hDebugHwnd);	// Setup our TLCS900h debugger	
			si.cbSize = sizeof(SCROLLINFO);
			si.fMask = SIF_ALL;
			si.nMin = 0;
			si.nMax = 0x200000+0xFFFF;
			si.nPos = gen_regsPC-0x200000;
			si.nPage = 0x20;
			SetScrollInfo(GetDlgItem(hwndDlg,IDC_DEBUGGER_DISASSEMBLY_VSCR),SB_CTL,&si,TRUE);
			SetDlgItemText(hwndDlg, IDC_TLCS900h_DEBUGGER_STATUS, "Paused");
			//Disassemble(hwndDlg, gen_regsPC, si.nPos+0x200000);

			return TRUE;
		break;
        case WM_COMMAND:
			switch(LOWORD(wParam))
            {
				case WM_DESTROY: // close?
					DestroyWindow(hwndDlg);
					return TRUE;
				break;
				case IDC_TLCS900HD_ADD_BREAKPOINT:
					// Get number of characters. 
                    cchPassword = (WORD) SendDlgItemMessage(hwndDlg, 
                                                            IDE_TLCS900HD_ADD_ADDRESS, 
                                                            EM_LINELENGTH, 
                                                            (WPARAM) 0, 
                                                            (LPARAM) 0);

                    // Put the number of characters into first word of buffer.
					if ( cchPassword > 1024 )
						cchPassword = (DWORD)1024;

                    *((LPWORD)lpszPassword) = cchPassword;

                    // Get the characters. 
                    SendDlgItemMessage(hwndDlg, 
                                       IDE_TLCS900HD_ADD_ADDRESS,
                                       EM_GETLINE,
                                       (WPARAM) 0,       // line 0 
                                       (LPARAM) lpszPassword); 

                    // Null-terminate the string. 
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
						bpIdx = neogpc_setbreakpoint(addr,BREAKPOINT_PERSIST);
						if ( bpIdx != -1 )
						{
							sprintf(listStr, "[%i] 0x%08X", bpIdx, addr);
							SendDlgItemMessage(hwndDlg, IDC_TLCS900HD_BREAKPOINT_LIST, LB_ADDSTRING, 0, (LPARAM)listStr);
							SendDlgItemMessage(hwndDlg, IDE_TLCS900HD_ADD_ADDRESS, WM_SETTEXT, 0, (LPARAM)"");
						}
						else
						{
							MessageBox(hwndDlg, "Invalid Breakpoint Address.. Cannot add two breakpoints at the same location", "Alert", MB_OK);
						}
					}
					else
					{
						MessageBox(hwndDlg, "Invalid Breakpoint Address.. Format: 0x20016C or 20016C", "Alert", MB_OK);
					}
				break;
				case IDC_TLCS900HD_REMOVE_BREAKPOINT:
					//neogpc_deletebreakpoint(0);
					bpIdx = SendDlgItemMessage(hwndDlg, IDC_TLCS900HD_BREAKPOINT_LIST, LB_GETCURSEL , 0, 0);
					lpszCheck = new char[1024];
					SendDlgItemMessage(hwndDlg, IDC_TLCS900HD_BREAKPOINT_LIST, LB_GETTEXT, bpIdx, (LPARAM)lpszCheck);
					bpIdx = SendDlgItemMessage(hwndDlg, IDC_TLCS900HD_BREAKPOINT_LIST, LB_DELETESTRING , bpIdx, 0);
					// Parse out the actual breakpoint index from the text
					if ( lpszCheck[2] == ']' )
					{
						lpszCheck[2] == '\0';
					}
					else if ( lpszCheck[3] == ']' )
					{
						lpszCheck[3] == '\0';
					}
					bpIdx = atoi(&(lpszCheck[1]));
					neogpc_deletebreakpoint(bpIdx);
					delete [] lpszCheck;
				break;
				case IDC_TLCS900HD_PAUSE:
					g_tlcs900hUpdateDebug = false;
					neogpc_pausedebugger();
					win_sound_pause();
					DisassembleTlcs900h(hwndDlg, gen_regsPC);
					SetDlgItemText(hwndDlg, IDC_TLCS900h_DEBUGGER_STATUS, "Paused");
					si.cbSize = sizeof(SCROLLINFO);
					si.fMask = SIF_ALL;
					GetScrollInfo(GetDlgItem(hwndDlg,IDC_DEBUGGER_DISASSEMBLY_VSCR),SB_CTL,&si);
					si.nPos = gen_regsPC-0x200000;
					SetScrollInfo(GetDlgItem(hwndDlg,IDC_DEBUGGER_DISASSEMBLY_VSCR),SB_CTL,&si,TRUE);	
				break;
				case IDC_TLCS900HD_RESUME:
					neogpc_resumedebugger();
					win_sound_reset();
					SetDlgItemText(hwndDlg, IDC_TLCS900h_DEBUGGER_STATUS, "Running");
				break;
				case IDC_TLCS900HD_STEPIN:
					g_tlcs900hUpdateDebug = true;
					neogpc_stepindebugger();
					//Disassemble(hwndDlg, gen_regsPC, gen_regsPC);
				break;
				case IDC_TLCS900HD_STEPOVER:
					g_tlcs900hUpdateDebug = true;
					neogpc_stepoverdebugger();
					//Disassemble(hwndDlg, gen_regsPC, gen_regsPC);
				break;
				/*
				case IDC_TLCS900HD_CLOSE:
					neogpc_cleardebugger();
					DestroyWindow(hwndDlg);
					return TRUE;
				break;
				*/
				case IDC_TLCS900HD_GOTO_ADDRESS:
					// Get number of characters. 
                    cchPassword = (WORD) SendDlgItemMessage(hwndDlg, 
                                                            IDE_TLCS900HD_GOTO_ADDRESS, 
                                                            EM_LINELENGTH, 
                                                            (WPARAM) 0, 
                                                            (LPARAM) 0);

                    // Put the number of characters into first word of buffer.
					if ( cchPassword > 1024 )
						cchPassword = (DWORD)1024;

                    *((LPWORD)lpszPassword) = cchPassword;

                    // Get the characters. 
                    SendDlgItemMessage(hwndDlg, 
                                       IDE_TLCS900HD_GOTO_ADDRESS,
                                       EM_GETLINE,
                                       (WPARAM) 0,       // line 0 
                                       (LPARAM) lpszPassword); 

                    // Null-terminate the string. 
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
						DisassembleTlcs900h(hwndDlg, addr);
						si.cbSize = sizeof(SCROLLINFO);
						si.fMask = SIF_ALL;
						GetScrollInfo(GetDlgItem(hwndDlg,IDC_DEBUGGER_DISASSEMBLY_VSCR),SB_CTL,&si);
						si.nPos = addr-0x200000;
						SetScrollInfo(GetDlgItem(hwndDlg,IDC_DEBUGGER_DISASSEMBLY_VSCR),SB_CTL,&si,TRUE);
						SendDlgItemMessage(hwndDlg, IDE_TLCS900HD_GOTO_ADDRESS, WM_SETTEXT, 0, (LPARAM)"");
					}
					else
					{
						MessageBox(hwndDlg, "Invalid Goto Address.. Format: 0x20016C or 20016C", "Alert", MB_OK);
					}
				break;
				case IDC_TLCS900HD_GOTO_PC:
					DisassembleTlcs900h(hwndDlg, gen_regsPC);
					si.cbSize = sizeof(SCROLLINFO);
					si.fMask = SIF_ALL;
					GetScrollInfo(GetDlgItem(hwndDlg,IDC_DEBUGGER_DISASSEMBLY_VSCR),SB_CTL,&si);
					si.nPos = gen_regsPC-0x200000;
					SetScrollInfo(GetDlgItem(hwndDlg,IDC_DEBUGGER_DISASSEMBLY_VSCR),SB_CTL,&si,TRUE);
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
					case SB_LINEUP: si.nPos--; updateDisas=true; break;
					case SB_LINEDOWN:
						incStep = neogpc_asminc(si.nPos+0x200000);
						if ( incStep == 0 )
							si.nPos++;
						else
							si.nPos += incStep;
						updateDisas=true; 
					break;
					case SB_PAGEUP:
						si.nPos-=si.nPage;
						updateDisas=true;
					break;
					case SB_PAGEDOWN:
						si.nPos+=si.nPage;
						updateDisas=true; break;
					case SB_THUMBPOSITION: //break;
					case SB_THUMBTRACK: si.nPos = si.nTrackPos; updateDisas=true; break;
				}
				SetScrollInfo((HWND)lParam,SB_CTL,&si,TRUE);
				if ( updateDisas == true )
				{
					if ( si.nPos >= 0x200000 )
						DisassembleTlcs900h(hwndDlg, 0xDF0000 + si.nPos);
					else
						DisassembleTlcs900h(hwndDlg, si.nPos+0x200000);
				}
			}
			// Disassemble
			break;
		case WM_CLOSE:
			break;
		case WM_DESTROY:
			g_tlcs900hActive = false;
			win_sound_reset();
			break;
		default:
			break;
    }

    return FALSE;
}

// Open the TLCS900h debugger
void openTlcs900hDebugger()
{
	if ( g_tlcs900hActive == false )
	{
		g_tlcs900hDebugHwnd = CreateDialog(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_TLCS900HDEBUGGER), 0/*hwnd*/, TLCS900hProc);
		if ( g_tlcs900hDebugHwnd )
		{
			ShowWindow(g_tlcs900hDebugHwnd, SW_SHOWNORMAL);
			SetForegroundWindow(g_tlcs900hDebugHwnd);
		}
	}
}

// Open the Z80 debugger
void openZ80Debugger()
{
}

#endif