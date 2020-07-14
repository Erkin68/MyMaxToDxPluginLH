#include "windows.h"
#include "resource.h"
#include <commdlg.h> 
#include <dlgs.h> 
#include "ExpDX.h"




#pragma warning(disable:4312)


MYDATA			sMyData;		



//
//   FUNCTION: TestNotify( HWND hDlg, LPOFNOTIFY pofn)
//
//  PURPOSE:  Processes the WM_NOTIFY message notifications that is sent
//    to the hook dialog procedure for the File Open common dialog.
//
//
BOOL NEAR PASCAL TestNotify(HWND hDlg, LPOFNOTIFY pofn)
{
	switch (pofn->hdr.code)
	{
		// The selection has changed. 
		case CDN_SELCHANGE:
		{
			char szFile[MAX_PATH];

			// Get the file specification from the common dialog.
			if (CommDlg_OpenSave_GetSpec(GetParent(hDlg),
				szFile, sizeof(szFile)) <= sizeof(szFile))
			{
				// Set the dialog item to reflect this.
				SetDlgItemText(hDlg, IDC_EDIT_PATH, szFile);
			}

			// Get the path of the selected file.
			if (CommDlg_OpenSave_GetFilePath(GetParent(hDlg),
				szFile, sizeof(szFile)) <= sizeof(szFile))
			{
				// Display this path in the appropriate box.
				SetDlgItemText(hDlg, IDC_EDIT_PATH, szFile);
			}
		}
		break;

		// A new folder has been opened.
		case CDN_FOLDERCHANGE:
		{
			char szFile[MAX_PATH];

			if (CommDlg_OpenSave_GetFolderPath(GetParent(hDlg),
				szFile, sizeof(szFile)) <= sizeof(szFile))
			{
				// Display this new path in the appropriate box.
				SetDlgItemText(hDlg, IDC_EDIT_PATH, szFile);
			}
		}
		break;

		// The "Help" pushbutton has been pressed.
		case CDN_HELP:
			MessageBox(hDlg, "Got the Help button notify.", "ComDlg32 Test", MB_OK);
			break;

		// The 'OK' pushbutton has been pressed.
		case CDN_FILEOK:
			// Update the appropriate box.
			SetDlgItemText(hDlg,IDC_EDIT_PATH, pofn->lpOFN->lpstrFile);
			SetWindowLongPtr(hDlg, DWLP_MSGRESULT, 1L);
			break;

		// Received a sharing violation.
		case CDN_SHAREVIOLATION:
			// Update the appropriate box.
			SetDlgItemText(hDlg, IDC_EDIT_PATH, pofn->pszFile);
			MessageBox(hDlg, "Got a sharing violation notify.", "ComDlg32 Test", MB_OK);
			break;
	}

	return(TRUE);
}










//
//   FUNCTION: ComDlg32DlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
//
//  PURPOSE:  Processes messages for the File Open common dialog box.
//
//    MESSAGES:
//
//	WM_INITDIALOG - save pointer to the OPENFILENAME structure in User data
//	WM_DESTROY - get the text entered and fill in the MyData structure
//	WM_NOTIFY - pass this message onto the TestNotify function
//	default - check for a sharing violation or the OK button and
//    	display a message box.
//
//
BOOL CALLBACK ComDlg32DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

	switch (uMsg)
	{
		case WM_INITDIALOG:
			// Save off the long pointer to the OPENFILENAME structure.
			SetWindowLongPtr(hDlg, DWLP_USER, (LONG)lParam);
		break;


		case WM_COMMAND:
			if (LOWORD(wParam) == IDC_BTN_OK_BOSHLA) 
			{
				EndDialog(GetParent(hDlg), LOWORD(wParam));
				return TRUE;
			}
		break;
		case WM_DESTROY:
			LPOPENFILENAME	lpOFN;
			LPMYDATA		psMyData;
			lpOFN = (LPOPENFILENAME)GetWindowLongPtr(hDlg, DWLP_USER);
			psMyData = (LPMYDATA)lpOFN->lCustData;
			GetDlgItemText(hDlg, IDC_EDIT_PATH, psMyData->szGetFromDlg,sizeof(psMyData->szGetFromDlg));
		break;

		case WM_NOTIFY:
			TestNotify(hDlg, (LPOFNOTIFY)lParam);

		default:
			if (uMsg == cdmsgFileOK)
			{
				SetDlgItemText(hDlg, IDC_EDIT_PATH, ((LPOPENFILENAME)lParam)->lpstrFile);
				if (MessageBox(hDlg, "Got the OK button message.\n\nShould I open it?", "ComDlg32 Test", MB_YESNO)
					== IDNO)
				{
					SetWindowLongPtr(hDlg, DWLP_MSGRESULT, 1L);
				}
				break;
			}
			else if (uMsg == cdmsgShareViolation)
			{
				SetDlgItemText(hDlg, IDC_EDIT_PATH, (LPSTR)lParam);
				MessageBox(hDlg, "Got a sharing violation message.", "ComDlg32 Test", MB_OK);
				break;
			}
			return FALSE;
	}
	return TRUE;
}

















