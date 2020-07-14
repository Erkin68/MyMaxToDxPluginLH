/**********************************************************************
 *<
	FILE: ExpDX.cpp

	DESCRIPTION:	Appwizard generated plugin

	CREATED BY: Erkin

	HISTORY: 

 	Copyright (c) 2006, All Rights Reserved.

	FILE: ExpDX.cpp

	DESCRIPTION:	Appwizard generated plugin

	CREATED BY: Erkin

	HISTORY: 

// Conversion from 3ds max coords to Direct3D coords:
//
// 3ds max:  (Up, Front, Right) == (+Z, +Y, +X)
//
// Direct3D: (Up, Front, Right) == (+Z, -Y, +X)
//
// Conversion from 3ds max to Direct3D coords:
//
// 3ds max * conversion matrix = Direct3D
//
// [ x y z w ] * | +1  0  0  0 | = [ X Y Z W ]
//               |  0  0 +1  0 |
//               |  0 +1  0  0 |
//               |  0  0  0 +1 |
// degan iddaolar no'to'gri bo'lib chiqdi. Sababi, bunday hisoblashda Matrix ning Z tashkil etuvchisi
// teskari bo'lib qoladur. Hisoblashlarda buning oqibati yomon bo'ladur. Masalan distansiyani hisoblasok,
// bu qiymat haqiqiysidan ishirasi teskariligi b-n ajralib turodir. Quat larni hisoblashlarda ham bunday
// xatolik juda katta oqibatlarga olib keldi. Skin animatsiyasi Maxnikidan umuman notog'ri ishlay
// boshladi. To'g'ri yechimga faqatgina 27 fevral, 2009 yilda keldim. Bu men uchun eng katta yutuq va 
// ahamiyati kattadur. Chunki bu engine ning eng asosiy poydevoridur. Yechimni FlipMatrix va FlipQuat
// metodlarda keltirdim. 
*/
#include "ExpDX.h"

#pragma warning(disable:4311)

#define MYPLUGIN_CLASS_ID	Class_ID(0x5fd9931e, 0x2e4db251)


class MyPlugin : public UtilityObj 
{
	public:


		HWND			hPanel;
		IUtil			*iu;
		Interface		*ip;
		
		VOID BeginEditParams(Interface *ip,IUtil *iu);
		VOID EndEditParams(Interface *ip,IUtil *iu);

		VOID Init(HWND hWnd);
		VOID Destroy(HWND hWnd);
		

		VOID DeleteThis() { }		
		//Constructor/Destructor

		MyPlugin();
		~MyPlugin();		

};

static MyPlugin theMyPlugin;

class MyPluginClassDesc:public ClassDesc 
{
	public:
	int 			IsPublic() { return TRUE; }
	VOID *			Create(BOOL loading = FALSE) { return &theMyPlugin; }
	const TCHAR *	ClassName() { return GetString(IDS_CLASS_NAME); }
	SClass_ID		SuperClassID() { return UTILITY_CLASS_ID; }
	Class_ID		ClassID() { return MYPLUGIN_CLASS_ID; }
	const TCHAR* 	Category() { return GetString(IDS_CATEGORY); }

	const TCHAR*	InternalName() { return _T("MyPlugin"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }				// returns owning module handle

};

static MyPluginClassDesc MyPluginDesc;
ClassDesc* GetMyPluginDesc() { return &MyPluginDesc; }
static ISpinnerControl *ScaleSpin = 0;
static ISpinnerControl *ThresholdSpin = 0;


D3DXMATRIX ConvToD3DXMATRIX(Matrix3 *m)
{
D3DXMATRIX M;
MRow *mc = m->GetAddr();
	M._11 = mc[0][0]; M._12 = mc[0][1]; M._13 = mc[0][2]; M._14 = 0.0f;
	M._21 = mc[1][0]; M._22 = mc[1][1]; M._23 = mc[1][2]; M._24 = 0.0f;
	M._31 = mc[2][0]; M._32 = mc[2][1]; M._33 = mc[2][2]; M._34 = 0.0f;
	M._41 = mc[3][0]; M._42 = mc[3][1]; M._43 = mc[3][2]; M._44 = 1.0f;
	return M;
}

Matrix3 ConvToMatrix3(D3DXMATRIX *M)
{
Matrix3 m;
MRow *mc = m.GetAddr();
	mc[0][0] = M->_11; mc[0][1] = M->_12; mc[0][2] = M->_13; mc[0][3] = M->_14; 
	mc[1][0] = M->_21; mc[1][1] = M->_22; mc[1][2] = M->_23; mc[1][3] = M->_24; 
	mc[2][0] = M->_31; mc[2][1] = M->_32; mc[2][2] = M->_33; mc[2][3] = M->_34; 
	mc[3][0] = M->_41; mc[3][1] = M->_42; mc[3][2] = M->_43; mc[3][3] = M->_44; 
	return m;
}

VOID InverseInDX(Matrix3 *m)
{
D3DXMATRIX MINV, M = ConvToD3DXMATRIX(m);
	myD3DXMatrixInverse(&MINV, NULL, &M);
	*m = ConvToMatrix3(&MINV);
	return;
}

//Qaytarilgan Quat faqatgina LH da ishlaydi, xolos. Maxda ishlatmaslik shart!!!;
D3DXQUATERNION FlipYZAxisInQuat(Quat &q)
{
D3DXQUATERNION	Q = q;
	q.x = -Q.x;
	q.y = -Q.z;
	q.z = -Q.y;
	q.w =  Q.w;
	return Q;
}

//Qaytarilgan Matrix faqatgina LH da ishlaydi, xolos. Maxda ishlatmaslik shart!!!;
D3DXMATRIX FlipYZAxisInMatrix(Matrix3 &m)
{
D3DXQUATERNION	Q;
D3DXVECTOR3		sc, tr;
D3DXMATRIX		MC, M = ConvToD3DXMATRIX(&m);
	MC._11 = M._11; MC._12 = M._13; MC._13 = M._12; MC._14 = 0.0f;
	MC._21 = M._31; MC._22 = M._33; MC._23 = M._32; MC._24 = 0.0f;
	MC._31 = M._21; MC._32 = M._23; MC._33 = M._22; MC._34 = 0.0f;
	MC._41 = M._41; MC._42 = M._43; MC._43 = M._42; MC._44 = 1.0f;
	return MC;
}

//Qaytarilgan Matrix faqatgina LH da ishlaydi, xolos. Maxda ishlatmaslik shart!!!;
D3DXMATRIX FlipYZAxisInD3DXMATRIX(D3DXMATRIX &M)
{
D3DXQUATERNION	Q;
D3DXVECTOR3		sc, tr;
D3DXMATRIX		MC;
	MC._11 = M._11; MC._12 = M._13; MC._13 = M._12; MC._14 = 0.0f;
	MC._21 = M._31; MC._22 = M._33; MC._23 = M._32; MC._24 = 0.0f;
	MC._31 = M._21; MC._32 = M._23; MC._33 = M._22; MC._34 = 0.0f;
	MC._41 = M._41; MC._42 = M._43; MC._43 = M._42; MC._44 = 1.0f;
	return MC;
}

//Qaytarilgan Euler faqatgina LH da ishlaydi, xolos. Maxda ishlatmaslik shart!!!;
Point3 GetFlippedRot(Matrix3 *m)
{
Point3			euler;
D3DXVECTOR3		sc, tr;
D3DXQUATERNION	Q;
AffineParts		AP;
D3DXMATRIX		M = FlipYZAxisInMatrix(*m);
	myD3DXMatrixDecompose(&sc, &Q, &tr, &M);
	QuatToEuler((Quat)Q, euler, EULERTYPE_XYZ);
	euler.x = -euler.x;
	euler.y = -euler.y; 
	euler.z = -euler.z; 
	return euler;
}

//Burilish va scalelarni joyiga qo'yib, ya'ni 0 0 0 burilhgandagi matritsasi:
Matrix3 GetInvTransform(Point3 &p, Quat &q)
{
Matrix3 mt;
	mt.SetRotate(q);
	mt.Translate(p);
	mt.Invert();
	return mt;
}

Point3 GetMatrixScale(Matrix3 *m)
{
	Point3 sc;
	MRow *mc = m->GetAddr();
	sc.x = sqrtf(mc[0][0]*mc[0][0] + mc[0][1]*mc[0][1] + mc[0][2]*mc[0][2]);
	sc.y = sqrtf(mc[1][0]*mc[1][0] + mc[1][1]*mc[1][1] + mc[1][2]*mc[1][2]);
	sc.z = sqrtf(mc[2][0]*mc[2][0] + mc[2][1]*mc[2][1] + mc[2][2]*mc[2][2]);
	return sc;
}

VOID writeConfig(HWND hWnd)
{
char	ModulePath[MAX_PATH];
FILE	*strin;
char	str[128];

	GetModuleFileName(NULL, ModulePath, MAX_PATH);
	*(strrchr(ModulePath, __TEXT('\\'))) = 0;    
	lstrcat(ModulePath, "\\Plugins\\ExpPl.cfg");
	
	if(!openfile(ModulePath, &strin, "wb"))
		return;

	fwrite(&sMyData.fMassh , 4, 1, strin);
	fwrite(&sMyData.fThreshold, 4, 1, strin);
	fwrite(&sMyData.dete, 4, 1, strin);
	fwrite(&sMyData.interpKoef, 4, 1, strin);
	GetDlgItemText(hWnd,IDC_EDIT_TOLIQ_SAHNANI_EXPORT_YOLI,str,128);
	fwrite(str , 128, 1, strin);
	GetDlgItemText(hWnd,IDC_EDIT_SELECTION_FILE,sMyData.vrtSelFile,128);
	fwrite(sMyData.vrtSelFile, 128, 1, strin);
	sMyData.bToText = (BST_CHECKED == SendMessage(GetDlgItem(hWnd,IDC_CHECK_TEXT),BM_GETCHECK,BST_CHECKED,0));
	fwrite(&sMyData.bToText, 4, 1, strin);
	sMyData.bInIndexed = (BST_CHECKED == SendMessage(GetDlgItem(hWnd,IDC_CHECK_INDEXED_SEQUENCE),BM_GETCHECK,BST_CHECKED,0));
	fwrite(&sMyData.bInIndexed, 4, 1, strin);
	sMyData.bTangent = (BST_CHECKED == SendMessage(GetDlgItem(hWnd,IDC_CHECK_TANGENT_AND_BINORMAL),BM_GETCHECK,BST_CHECKED,0));
	fwrite(&sMyData.bTangent, 4, 1, strin);
	sMyData.b32bit = (BST_CHECKED == SendMessage(GetDlgItem(hWnd,IDC_CHECK_32_BIT),BM_GETCHECK,BST_CHECKED,0));
	fwrite(&sMyData.b32bit, 4, 1, strin);
	fwrite(&sMyData.bDeleteSource, 4, 1, strin);
	sMyData.bFlipNormals = (BST_CHECKED == SendMessage(GetDlgItem(hWnd,IDC_CHECK_FLIP_NORMALS),BM_GETCHECK,BST_CHECKED,0));
	fwrite(&sMyData.bFlipNormals, 4, 1, strin);
	sMyData.bClampTV = (BST_CHECKED == SendMessage(GetDlgItem(hWnd,IDC_CHECK_CLAMPTV),BM_GETCHECK,BST_CHECKED,0));
	fwrite(&sMyData.bClampTV, 4, 1, strin);
	fclose(strin);
	return;
}

VOID readConfig(HWND hWnd)
{
char	ModulePath[MAX_PATH];
FILE	*strin;
char	str[256];

	sMyData.fMassh = 0.01f;
	sMyData.dete   = 0.0f;

	GetModuleFileName(NULL,ModulePath,MAX_PATH);
	*(strrchr(ModulePath, __TEXT('\\'))) = 0;    
	lstrcat(ModulePath, "\\Plugins\\ExpPl.cfg");

	openfile(ModulePath, &strin, "rb");

	if(strin)
	{	fread(&sMyData.fMassh, 4  , 1, strin);
		fread(&sMyData.fThreshold, 4  , 1, strin);
		fread(&sMyData.dete  , 4  , 1, strin);
		fread(&sMyData.interpKoef, 4  , 1, strin);
		fread(str            , 128, 1, strin);
		fread(sMyData.vrtSelFile, 128, 1, strin);
	} else
	{	sprintf_s(str, 256, "%s", "D:\\Temp\\Meshes");
		sprintf_s(sMyData.vrtSelFile, 128, "%s", "D:\\Temp\\Meshes");
	}
	SetDlgItemText(hWnd,IDC_EDIT_SELECTION_FILE,sMyData.vrtSelFile);

	wsprintf(sMyData.szExport,"%s",str);
	wsprintf(sMyData.szImport,"%s",str);
	wsprintf(sMyData.szReadToText,"%s",str);
	wsprintf(sMyData.szWriteText,"%s",str);

	SetDlgItemText(hWnd,IDC_EDIT_TOLIQ_SAHNANI_EXPORT_YOLI,sMyData.szExport);

	//masshtab:
	ScaleSpin->SetValue(sMyData.fMassh*10000.0f, 1);
	sprintf_s(str, 256, "%.3f", sMyData.fMassh);
	SetDlgItemText(hWnd, IDC_EDIT_MASH, str);

	//threshold:
	ThresholdSpin->SetValue(sMyData.fThreshold*100000.0f, 1);
	sprintf_s(str, 256, "%.6f", sMyData.fThreshold);
	SetDlgItemText(hWnd, IDC_EDIT_THRESHOLD, str);

	//Anim time editdan olinsun:
	sprintf_s(str, 256, "%f", sMyData.dete * 1000.0f);
	SetDlgItemText(hWnd, IDC_EDIT_ANIM_TIME, str);
	
	//Interpolation koef.:
	sprintf_s(str, 256, "%f", sMyData.interpKoef);
	SetDlgItemText(hWnd, IDC_EDIT_INTERPOLATION_KOEF, str);
	
	if(strin)
	{	fread(str, 128, 1, strin);
		SendMessage(GetDlgItem(hWnd,IDC_CHECK_TEXT),BM_SETCHECK,sMyData.bToText?BST_CHECKED:BST_UNCHECKED,0);
		fread(&sMyData.bInIndexed, 4, 1, strin);
		SendMessage(GetDlgItem(hWnd,IDC_CHECK_INDEXED_SEQUENCE),BM_SETCHECK,sMyData.bInIndexed?BST_CHECKED:BST_UNCHECKED,0);
		fread(&sMyData.bTangent, 4, 1, strin);
		SendMessage(GetDlgItem(hWnd,IDC_CHECK_TANGENT_AND_BINORMAL),BM_SETCHECK,sMyData.bTangent?BST_CHECKED:BST_UNCHECKED,0);
		EnableWindow(GetDlgItem(hWnd,IDC_CHECK_TANGENT_AND_BINORMAL),sMyData.bInIndexed);
		fread(&sMyData.b32bit, 4, 1, strin);
		SendMessage(GetDlgItem(hWnd,IDC_CHECK_32_BIT),BM_SETCHECK,sMyData.b32bit?BST_CHECKED:BST_UNCHECKED,0);
		EnableWindow(GetDlgItem(hWnd,IDC_CHECK_32_BIT),sMyData.bInIndexed);
		fread(&sMyData.bDeleteSource, 4, 1, strin);
		SendMessage(GetDlgItem(hWnd,IDC_CHECK_DELETE_OLD),BM_SETCHECK,sMyData.bDeleteSource?BST_CHECKED:BST_UNCHECKED,0);
		fread(&sMyData.bFlipNormals, 4, 1, strin);
		SendMessage(GetDlgItem(hWnd,IDC_CHECK_FLIP_NORMALS),BM_SETCHECK,sMyData.bFlipNormals?BST_CHECKED:BST_UNCHECKED,0);
		fread(&sMyData.bClampTV, 4, 1, strin);
		SendMessage(GetDlgItem(hWnd,IDC_CHECK_CLAMPTV),BM_SETCHECK,sMyData.bClampTV?BST_CHECKED:BST_UNCHECKED,0);
		fclose(strin);
	}
	return;
}

VOID Progr(int s, int po, float prots, const char *txt, Interface* ip)
{
	static int sit = 0;
	int it = (int)((float)s + (float)(po - s) * prots);
	if(sit != it)
	{	ip->ProgressUpdate(	it, TRUE, TSTR(txt));
		sit = it;
	}static char chst[128] = {0};
	if(lstrcmp(chst, txt))
	{ lstrcpy(chst, txt); ip->ProgressEnd();
	  ip->ProgressStart(_T(chst), TRUE, fn, NULL);
	}
	return;
}

LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
				return TRUE;

		case WM_COMMAND:
			if(LOWORD(wParam) == IDOK) 
			{
				EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
			}
			break;
	}
    return FALSE;
}

static BOOL CALLBACK MyPluginDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
OPENFILENAME	OpenFileName;
TCHAR			szFile[MAX_PATH]      = "\0";
static ThrPar   thrPar;
char			str[128];
int				ch;

	switch (msg) 
	{	case WM_INITDIALOG:
			theMyPlugin.Init(hWnd);
			SendMessage(GetDlgItem(hWnd,IDC_CHECK_NOTUTV),BM_SETCHECK,BST_CHECKED,0);
			SendMessage(GetDlgItem(hWnd,IDC_CHECK_NO_NORMALS),BM_SETCHECK,BST_CHECKED,0);
			readConfig(hWnd);
		break;
		case WM_DESTROY:
			writeConfig(hWnd);
			theMyPlugin.Destroy(hWnd);
		break;
		case WM_COMMAND:
			switch(HIWORD(wParam))
			{	case EN_SETFOCUS: 
					DisableAccelerators();
				return TRUE;
				case EN_KILLFOCUS: 
					EnableAccelerators();
					if(LOWORD(wParam) == IDC_EDIT_ANIM_TIME)
					{	GetDlgItemText(hWnd,IDC_EDIT_ANIM_TIME, str, 128);
						sMyData.dete = 0.001f * (float)atof(str);
					} else
					if(LOWORD(wParam) == IDC_EDIT_MASH)
					{	GetDlgItemText(hWnd,IDC_EDIT_MASH, str, 128);
						sMyData.fMassh = (float)atof(str);
						ScaleSpin->SetValue(sMyData.fMassh*10000.0f, 1);
					}else
					if(LOWORD(wParam) == IDC_EDIT_THRESHOLD)
					{	GetDlgItemText(hWnd,IDC_EDIT_THRESHOLD, str, 128);
						sMyData.fThreshold = (float)atof(str);
						ThresholdSpin->SetValue(sMyData.fThreshold*100000.0f, 1);
					}else
					if(LOWORD(wParam) == IDC_EDIT_TOLIQ_SAHNANI_EXPORT_YOLI)
					{	GetDlgItemText(hWnd,IDC_EDIT_TOLIQ_SAHNANI_EXPORT_YOLI, str, 128);
						wsprintf(sMyData.szExport,"%s",str);
					} else
					if(LOWORD(wParam) == IDC_EDIT_INTERPOLATION_KOEF)
					{	GetDlgItemText(hWnd,IDC_EDIT_INTERPOLATION_KOEF, str, 128);
						sMyData.interpKoef = (float)atof(str);
					}else					
				return TRUE;
				case BN_CLICKED://   Bosilgan, knopkami, yoki boshqa narsa 
					switch(LOWORD(wParam))
					{	case IDC_CHECK_TEXT:
							sMyData.bToText = (BST_CHECKED == SendMessage(GetDlgItem(hWnd,IDC_CHECK_TEXT),BM_GETCHECK,BST_CHECKED,0));
						break;
						case IDC_CHECK_INDEXED_SEQUENCE:
							sMyData.bInIndexed = (BST_CHECKED == SendMessage(GetDlgItem(hWnd,IDC_CHECK_INDEXED_SEQUENCE),BM_GETCHECK,BST_CHECKED,0));
							EnableWindow(GetDlgItem(hWnd,IDC_CHECK_TANGENT_AND_BINORMAL),sMyData.bInIndexed);
							EnableWindow(GetDlgItem(hWnd,IDC_CHECK_32_BIT),sMyData.bInIndexed);
							if(!sMyData.bInIndexed)
							{	SendMessage(GetDlgItem(hWnd,IDC_CHECK_TANGENT_AND_BINORMAL),BM_SETCHECK,BST_UNCHECKED,0);
								SendMessage(GetDlgItem(hWnd,IDC_CHECK_32_BIT),BM_SETCHECK,BST_UNCHECKED,0);
							}
						break;
						case IDC_CHECK_TANGENT_AND_BINORMAL:
							sMyData.bTangent = (BST_CHECKED == SendMessage(GetDlgItem(hWnd,IDC_CHECK_TANGENT_AND_BINORMAL),BM_GETCHECK,BST_CHECKED,0));
						break;
						case IDC_CHECK_32_BIT:
							sMyData.b32bit = (BST_CHECKED == SendMessage(GetDlgItem(hWnd,IDC_CHECK_32_BIT),BM_GETCHECK,BST_CHECKED,0));
						break;
						case IDC_CHECK_DETACH_MESH:
							sMyData.bDetachSknMesh =  (BST_CHECKED == SendMessage(GetDlgItem(hWnd,IDC_CHECK_DETACH_MESH),BM_GETCHECK,BST_CHECKED,0));
						break;
						case IDC_CHECK_DELETE_OLD:
							sMyData.bDeleteSource =  (BST_CHECKED == SendMessage(GetDlgItem(hWnd,IDC_CHECK_DELETE_OLD),BM_GETCHECK,BST_CHECKED,0));
						break;			
						case IDC_CHECK_FLIP_NORMALS:
							sMyData.bFlipNormals =  (BST_CHECKED == SendMessage(GetDlgItem(hWnd,IDC_CHECK_FLIP_NORMALS),BM_GETCHECK,BST_CHECKED,0));
						break;			
						case IDC_CHECK_CLAMPTV:
							sMyData.bClampTV =  (BST_CHECKED == SendMessage(GetDlgItem(hWnd,IDC_CHECK_CLAMPTV),BM_GETCHECK,BST_CHECKED,0));
						break;			
//******************** TO"LIQ SAHNANI EXPORT QILISHNING YO"LI (EDITINI)*****
						case IDC_BTN_ALL_EXPORT_PATH:
							lstrcpy( szFile, "");
							memset(&OpenFileName, 0, sizeof (OPENFILENAME));
							OpenFileName.lStructSize       = sizeof(OPENFILENAME);
							OpenFileName.hwndOwner         = hWnd;
							OpenFileName.hInstance         = hInstance;
							OpenFileName.lpstrFilter       = NULL;
							OpenFileName.lpstrCustomFilter = NULL;
							OpenFileName.nMaxCustFilter    = 0;
							OpenFileName.nFilterIndex      = 0;
							OpenFileName.lpstrFile         = szFile;
							OpenFileName.nMaxFile          = sizeof(szFile);
							OpenFileName.lpstrFileTitle    = NULL;
							OpenFileName.nMaxFileTitle     = 0;
							OpenFileName.lpstrInitialDir   = NULL;
							OpenFileName.lpstrTitle        = "Export qilish uchun direktoriyani tanlang va OK ni bosing";
							OpenFileName.nFileOffset       = 0;
							OpenFileName.nFileExtension    = 0;
							OpenFileName.lpstrDefExt       = NULL;
							OpenFileName.lCustData         = (LPARAM)&sMyData;
							OpenFileName.lpfnHook 		   = (LPOFNHOOKPROC)ComDlg32DlgProc;
							OpenFileName.lpTemplateName    = MAKEINTRESOURCE(IDD_DIALOG_COMDLG32);
							OpenFileName.Flags             = OFN_SHOWHELP | OFN_EXPLORER | OFN_ENABLEHOOK | OFN_ENABLETEMPLATE;
							GetOpenFileName(&OpenFileName);
							SetDlgItemText(hWnd,IDC_EDIT_TOLIQ_SAHNANI_EXPORT_YOLI,sMyData.szGetFromDlg);
							wsprintf(sMyData.szExport,"%s",sMyData.szGetFromDlg);
						break;
						case IDC_BUTTON_SELECTION_FILE:
							memset(&OpenFileName, 0, sizeof (OPENFILENAME));
							memset(sMyData.vrtSelFile,0,128);
							OpenFileName.lStructSize       = sizeof(OPENFILENAME);
							OpenFileName.lpstrFilter       = TEXT("All vertice selections files (*.vsl)\0*.vsl\0All \0*.*\0");
							OpenFileName.lpstrFile         = sMyData.vrtSelFile;
							OpenFileName.lpstrFileTitle    = TEXT("Save vertice selection file.");
							OpenFileName.lpstrTitle        = "Vertice differences.";
							OpenFileName.Flags             = OFN_EXPLORER;
							OpenFileName.nMaxFile		   = MAX_PATH;
							if(GetSaveFileName(&OpenFileName))
								SetDlgItemText(hWnd,IDC_EDIT_SELECTION_FILE,sMyData.vrtSelFile);
						break;
						case IDC_BUTTON_SAVE_SELECTION_CHANNEL_TO_FILE:
							GetDlgItemText(hWnd,IDC_EDIT_SELECTION_FILE, sMyData.vrtSelFile, 128);
							thrPar.ip = theMyPlugin.ip;
							thrPar.iu = theMyPlugin.iu;
							ch = SaveVertexSelectionToFile((LPVOID)&thrPar);
							if(ch>0)
							{itoa(ch,str,10);
							 SetDlgItemText(hWnd,IDC_EDIT_SAVE_SEL_CHANNEL, str);
							}
						break;
//***********************  EXPORT ***********************************************
						case IDC_BTN_EXP_MESH:
							thrPar.ip = theMyPlugin.ip;
							thrPar.iu = theMyPlugin.iu;
							sMyData.bWriteSelection = (BST_CHECKED == SendMessage(GetDlgItem(hWnd,IDC_CHECK_SAVE_SELECTION),BM_GETCHECK,BST_CHECKED,0));
							GetDlgItemText(hWnd,IDC_EDIT_SELECTION_FILE,sMyData.szImport,128);
							thrPar.type = 5;
							theMyPlugin.ip->ProgressStart(_T("Mesh exporting:"), TRUE, fn, NULL);
							AllMesh((LPVOID)&thrPar);
							theMyPlugin.ip->ProgressEnd();
						break;
						case IDC_BTN_EXP_PHYSX_MESH:
							thrPar.ip = theMyPlugin.ip;
							thrPar.iu = theMyPlugin.iu;
							sMyData.bWriteSelection = (BST_CHECKED == SendMessage(GetDlgItem(hWnd,IDC_CHECK_SAVE_SELECTION),BM_GETCHECK,BST_CHECKED,0));
							GetDlgItemText(hWnd,IDC_EDIT_SELECTION_FILE,sMyData.szImport,128);
							thrPar.type = 5;
							theMyPlugin.ip->ProgressStart(_T("Mesh exporting:"), TRUE, fn, NULL);
							AllPhysXMesh((LPVOID)&thrPar);
							theMyPlugin.ip->ProgressEnd();
						break;
						case IDC_BTN_EXP_ANIM :
							thrPar.ip = theMyPlugin.ip;
							thrPar.iu = theMyPlugin.iu;
							//theMyPlugin.ip->ProgressStart(_T("Animation matrix:"), TRUE, fn, NULL);
							AllMeshMatrix((LPVOID)&thrPar);
							theMyPlugin.ip->ProgressEnd();
						break;
						case IDC_BTN_EXP_TU_TV:
							thrPar.ip = theMyPlugin.ip;
							thrPar.iu = theMyPlugin.iu;
							thrPar.type = 5;
							theMyPlugin.ip->ProgressStart(_T("TU-TV exporting:"), TRUE, fn, NULL);
							AllMeshTextCoord((LPVOID)&thrPar);
							theMyPlugin.ip->ProgressEnd();
						break;
						case IDC_BTN_EXP_SHAPE:
							thrPar.ip = theMyPlugin.ip;
							thrPar.iu = theMyPlugin.iu;
							thrPar.type = 1;
							theMyPlugin.ip->ProgressStart(_T("Shape exporting:"), TRUE, fn, NULL);
							AllShapes((LPVOID)&thrPar);// T2F_N3F_V3F ni
							theMyPlugin.ip->ProgressEnd();
						break;
						case IDC_BTN_EXP_DIFFUSE:
							thrPar.ip = theMyPlugin.ip;
							thrPar.iu = theMyPlugin.iu;
							thrPar.type = 5;
							theMyPlugin.ip->ProgressStart(_T("Diffuse exporting:"), TRUE, fn, NULL);
							AllDiffuseMesh((LPVOID)&thrPar);	
							theMyPlugin.ip->ProgressEnd();
						break;
						case IDC_BTN_EXP_SKIN:
							thrPar.ip = theMyPlugin.ip;
							thrPar.iu = theMyPlugin.iu;
							theMyPlugin.ip->ProgressStart(_T("Skin exporting:"), TRUE, fn, NULL);
							AllSkin((LPVOID)&thrPar);
							theMyPlugin.ip->ProgressEnd();
						break;
						case IDC_BTN_EXP_SKIN2_MORPH:
							thrPar.ip = theMyPlugin.ip;
							thrPar.iu = theMyPlugin.iu;
							theMyPlugin.ip->ProgressStart(_T("Skin2Morph exporting:"), TRUE, fn, NULL);
							AllSkin2Morph((LPVOID)&thrPar);
							theMyPlugin.ip->ProgressEnd();
						break;
						case IDC_BTN_EXP_MORPH:
							thrPar.ip = theMyPlugin.ip;
							thrPar.iu = theMyPlugin.iu;
							theMyPlugin.ip->ProgressStart(_T("Morph exporting:"), TRUE, fn, NULL);
							AllMorph((LPVOID)&thrPar);
							theMyPlugin.ip->ProgressEnd();
						break;						
						case IDC_BTN_EXP_MORPH_ANIM:
							thrPar.ip = theMyPlugin.ip;
							thrPar.iu = theMyPlugin.iu;
							theMyPlugin.ip->ProgressStart(_T("Morph animation exporting:"), TRUE, fn, NULL);
							AllMorphAnim((LPVOID)&thrPar);
							theMyPlugin.ip->ProgressEnd();
						break;
						case IDC_BTN_EXP_BIPE_ANIM:
							thrPar.ip = theMyPlugin.ip;
							thrPar.iu = theMyPlugin.iu;
							theMyPlugin.ip->ProgressStart(_T("Bipe animation exporting:"), TRUE, fn, NULL);
							AllSkinAni((LPVOID)&thrPar);
							theMyPlugin.ip->ProgressEnd();
						break;
						case IDC_BTN_IMPORT_MESH:
							ImportMesh(theMyPlugin.ip);
						break;						
						case IDC_BTN_IMPORT_TNGNT_MESH:
							ImportTngntMesh(theMyPlugin.ip);
						break;						
						case IDC_BTN_IMPORT_DIFFUSE_MESH:
							ImportDiffuseMesh(theMyPlugin.ip);
						break;							
						case IDC_BTN_IMPORT_SKIN:
							ImportSkin(theMyPlugin.ip);
						break;
						case IDC_BTN_ABOUT:
							DialogBox(hInstance, (LPCTSTR)IDD_DIALOG_HELP, hWnd, (DLGPROC)About);
						break;
						case IDC_BTN_GET_SELECTED_INFO:
							thrPar.ip = theMyPlugin.ip;
							thrPar.iu = theMyPlugin.iu;
							GetSelectedInfo((LPVOID)&thrPar);
						break;
						case IDC_BUTTON_EXP_FR_SEL_IND_FILE:
							lstrcpy( szFile, "");
							memset(&OpenFileName, 0, sizeof (OPENFILENAME));
							OpenFileName.lStructSize       = sizeof(OPENFILENAME);
							OpenFileName.hwndOwner         = hWnd;
							OpenFileName.hInstance         = hInstance;
							OpenFileName.lpstrFilter       = TEXT("All text Files (*.txt)\0*.txt\0All \0*.*\0");
							OpenFileName.lpstrCustomFilter = NULL;
							OpenFileName.nMaxCustFilter    = 0;
							OpenFileName.nFilterIndex      = 0;
							OpenFileName.lpstrFile         = szFile;
							OpenFileName.nMaxFile          = sizeof(szFile);
							OpenFileName.lpstrFileTitle    = NULL;
							OpenFileName.nMaxFileTitle     = 0;
							OpenFileName.lpstrInitialDir   = NULL;
							OpenFileName.lpstrTitle        = "Verteks indexlari ko'rsatilgan text faylini ko'rsating:";
							OpenFileName.nFileOffset       = 0;
							OpenFileName.nFileExtension    = 0;
							OpenFileName.lpstrDefExt       = NULL;
							OpenFileName.lCustData         = (LPARAM)&sMyData;
							OpenFileName.lpfnHook 		   = (LPOFNHOOKPROC)ComDlg32DlgProc;
							OpenFileName.lpTemplateName    = MAKEINTRESOURCE(IDD_DIALOG_COMDLG32);
							OpenFileName.Flags             = OFN_SHOWHELP | OFN_EXPLORER | OFN_ENABLEHOOK | OFN_ENABLETEMPLATE;
							GetOpenFileName(&OpenFileName);
							wsprintf(sMyData.vrtSelFile,"%s",sMyData.szGetFromDlg);
							BOOL bFlX = (BST_CHECKED == SendMessage(GetDlgItem(hWnd,IDC_CHECK_FLIPX_EXP_SEL_VERTS),BM_GETCHECK,BST_CHECKED,0));
							BOOL bFlY = (BST_CHECKED == SendMessage(GetDlgItem(hWnd,IDC_CHECK_FLIPY_EXP_SEL_VERTS),BM_GETCHECK,BST_CHECKED,0));
							thrPar.ip = theMyPlugin.ip;
							thrPar.iu = theMyPlugin.iu;
							ExpSelVertsFrIndsTxtFile((LPVOID)&thrPar,bFlX,bFlY);
						break;
						//case IDC_BTN_INIT_SKIN2_BONE_TMS:
							//InitBoneTMs();
						//break;
					}
				break;
			}
		break;
		case CC_SPINNER_CHANGE:
			if(LOWORD(wParam) == IDC_SPIN_SCALE)
			{	sMyData.fMassh = ((ISpinnerControl*)lParam)->GetIVal() / 10000.0f;
				sprintf(str, "%.6f", sMyData.fMassh);
				SetDlgItemText(hWnd, IDC_EDIT_MASH, str);
			}
			else if(LOWORD(wParam) == IDC_SPIN_THRESHOLD)
			{	sMyData.fThreshold = ((ISpinnerControl*)lParam)->GetIVal() / 100000.0f;
				sprintf(str, "%.6f", sMyData.fThreshold);
				SetDlgItemText(hWnd, IDC_EDIT_THRESHOLD, str);
			}
			return FALSE;
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_MOUSEMOVE:
			theMyPlugin.ip->RollupMouseMessage(hWnd,msg,wParam,lParam); 
		break;
		default:
			return FALSE; 
	}
	return TRUE;
}

//--- MyPlugin -------------------------------------------------------
MyPlugin::MyPlugin()
{
	iu = NULL;
	ip = NULL;	
	hPanel = NULL;
}

MyPlugin::~MyPlugin()
{
}

VOID MyPlugin::BeginEditParams(Interface *ip,IUtil *iu) 
{
	this->iu = iu;
	this->ip = ip;
	hPanel = ip->AddRollupPage(hInstance, MAKEINTRESOURCE(IDD_DIALOG_PANEL), (DLGPROC)MyPluginDlgProc,
		GetString(IDS_PARAMS), 0);
	return;
}
	
VOID MyPlugin::EndEditParams(Interface *ip,IUtil *iu) 
{
	this->iu = NULL;
	this->ip = NULL;
	ip->DeleteRollupPage(hPanel);
	hPanel = NULL;
	return;
}

VOID MyPlugin::Init(HWND hWnd)
{
	ScaleSpin = GetISpinner(GetDlgItem(hWnd, IDC_SPIN_SCALE));
	ScaleSpin->SetLimits(1, 100000, TRUE);
	ScaleSpin->SetValue(sMyData.fMassh*10000.0f, 1);
	ThresholdSpin = GetISpinner(GetDlgItem(hWnd, IDC_SPIN_THRESHOLD));
	ThresholdSpin->SetLimits(1, 1000000, TRUE);
	ThresholdSpin->SetValue(sMyData.fThreshold*100000.0f, 1);	
	return;
}

VOID MyPlugin::Destroy(HWND hWnd)
{
	ReleaseISpinner(ScaleSpin); ScaleSpin = 0;
	ReleaseISpinner(ThresholdSpin); ThresholdSpin = 0;	
	return;
}

BOOL openfile(char *path, char *name, char *ext, FILE **stream, char *mode)
{
char fileName[256];

	*stream = 0;
	wsprintf(fileName, "%s%s%s%s", path, "\\", name, ext);

	fopen_s(stream, fileName, mode);

	if((*stream)==0)
	{	//MessageBox(NULL, fileName, "Err. creating file!!!", MB_OK);
		return FALSE;
	}

	return TRUE;
}

BOOL openfile(char *name, FILE **stream, char *mode)
{
	*stream = 0;
	fopen_s(stream, name, mode);
	if((*stream)==0)
	{	//MessageBox(NULL, name, "Err. creating file!!!", MB_OK);
		return FALSE;
	}
	return TRUE;
}

MyMtrl myMtrl;
MyMtrl::MyMtrl()
{	
	Nill();
};
void MyMtrl::Nill()
{	vMatAmb = Point4( 0.358824f, 0.311765f, 0.059804f, 1.f );
	vMatDif = Point4( 0.858824f, 0.811765f, 0.859804f, 1.f );
	vMatSpe = Point4( 0.9f, 0.9f, 0.9f, 0.0f );
	vMatEmi = Point4( 0.0f, 0.0f, 0.0f, 0.0f );
	fMatPow = 32.f;
	fMatk_r = 0.20f;
};
void MyMtrl::Read(FILE* st)
{	::fread(&vMatAmb, 4, 4, st);
	::fread(&vMatDif, 4, 4, st);
	::fread(&vMatSpe, 4, 4, st);
	::fread(&vMatEmi, 4, 4, st);
	::fread(&fMatPow, 4, 1, st);
	::fread(&fMatk_r, 4, 1, st);
};
void MyMtrl::Write(FILE* st)
{	::fwrite(&vMatAmb, 4, 4, st);
	::fwrite(&vMatDif, 4, 4, st);
	::fwrite(&vMatSpe, 4, 4, st);
	::fwrite(&vMatEmi, 4, 4, st);
	::fwrite(&fMatPow, 4, 1, st);
	::fwrite(&fMatk_r, 4, 1, st);
};
void MyMtrl::Get(INode* n)
{
 __try
 {
	Mtl *m = n->GetMtl();
	if(m->ClassID() != Class_ID(DMTL_CLASS_ID, 0)) {Nill(); return;}
	Color c = m->GetAmbient(); vMatAmb = Point4(c.r, c.g, c.b, 1.0f);
	c = m->GetDiffuse(); vMatDif = Point4(c.r, c.g, c.b, 1.0f);
	c = m->GetSpecular(); vMatSpe = Point4(c.r, c.g, c.b, 1.0f);
	c = m->GetSelfIllumColor(); vMatEmi = Point4(c.r, c.g, c.b, 1.0f);
	fMatPow = m->GetShininess();
	fMatk_r = m->GetShinStr();
	vMatAmb.FNormalize();
	vMatDif.FNormalize();
	vMatSpe.FNormalize();
	vMatEmi.FNormalize();
	float opac = 1.0f - m->GetXParency();
	vMatAmb.w = opac;
	vMatDif.w = opac;
 }
 __except(1,1)
 {
	 Nill();
 }
};
void MyMtrl::Set(INode* n)
{
	Mtl *m = n->GetMtl();
	if(!m) return;
	if(m->ClassID() != Class_ID(DMTL_CLASS_ID, 0))
		return;
	if(vMatAmb.x>0.0f)
		if(vMatAmb.y>0.0f)
			if(vMatAmb.z>0.0f)
				m->SetAmbient(Color(vMatAmb.x, vMatAmb.y, vMatAmb.z), 0);
	if(vMatDif.x>0.0f)
		if(vMatDif.y>0.0f)
			if(vMatDif.z>0.0f)
				m->SetDiffuse(Color(vMatDif.x, vMatDif.y, vMatDif.z), 0);
	if(vMatSpe.x>0.0f)
		if(vMatSpe.y>0.0f)
			if(vMatSpe.z>0.0f)
				m->SetSpecular(Color(vMatSpe.x, vMatSpe.y, vMatSpe.z), 0);
				//m->SetSelfIllumColor(Color(vMatEmi.x, vMatEmi.y, vMatEmi.z), 0);
	if(fMatPow>0.0f)
		m->SetShininess(fMatPow, 0);
	//m->SetShinStr(vMatk_r, 0);
};
//TU-TV si -1..+1 chegaradan tashqarida bo'lsa, uni to'g'rilash:
void ClampTU_TV(float *tutv)
{
	for(int i=0; i<2; i++)
	{	if(tutv[i]<0.0f)
		{	int ift = (int)tutv[i];
			float rem = tutv[i]-(float)ift;
				tutv[i] = 1.0f+rem;
		} else if(tutv[i]>1.0f)
		{	int ift = (int)tutv[i];
			float rem = tutv[i]-(float)ift;
			tutv[i] = rem;
	}	}
	return;
}
