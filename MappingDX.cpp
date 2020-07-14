//Hozirgi plaginda bu fayl ishlatilmaydur.



#include "ExpDX.h"
#include "..\MapCreator\resource.h"

static ThrPar	*thrPar;
static char		MAPFILENAME[1024];
static int		MAPWIDTH  = 100;//sm
static int		MAPHEIGHT = 100;//sm
static int		MAXFACES  = -32767;
static int		MINFACES  = 32767;
static int      MAXR;


static VOID OnInitMappingDlg(HWND hWnd);
static VOID CalcMapping(HWND hWnd);
static VOID GetMeshsMaxMin(int k, Interface *ip, float *XMin, float *XMax, float *YMin, float *YMax);
static int  GetMeshsFacesInRect(FILE *stream, int k, Interface *ip, float *XFrom, float *YFrom, float *width, float *height);
static BOOL IsMeshFaceInRect(int k, Interface *ip, float *XFrom, float *YFrom, float *width, float *height);
int			IsPointInwardTriangle(float *pt, float *tra);
int			AreVectorsCrosses(float *v1, float *v2);


LRESULT CALLBACK MappingDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
			thrPar = (ThrPar *)lParam;
			OnInitMappingDlg(hDlg);
		return TRUE;

		case WM_COMMAND:
			if(LOWORD(wParam) == IDCANCEL) 
			{
				EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
			}
			if(LOWORD(wParam) == IDC_BUTTON_CALC_MAP) 
			{
				__try
				{
					CalcMapping(hDlg);
				}
				__except(1,1)
				{
					MessageBox(NULL, "Kutilmagan gen. xato", "Diqqat!!!", MB_OK);
				}
				return TRUE;
			}
		break;
	}
    return FALSE;
}


static VOID OnInitMappingDlg(HWND hWnd)
{
char str[128];

	// Sentrovka 
	SetWindowPos(hWnd, HWND_TOP, GetSystemMetrics(SM_CXSCREEN)/2 - 204, GetSystemMetrics(SM_CYSCREEN)/2 - 190, 408, 380, SWP_SHOWWINDOW);

	//MAPFILENAME
	sprintf(MAPFILENAME, sMyData.szExport);
	lstrcat (MAPFILENAME, "\\Scene.mp");
	SetDlgItemText(hWnd, IDC_EDIT_EXPORT_MAPFILE_NAME, MAPFILENAME);

	//WIDTH
	sprintf(str, "%d", MAPWIDTH);
	SetDlgItemText(hWnd, IDC_EDIT_EXPORT_MAP_WIDTH, str);
	
	//HEIGHT
	sprintf(str, "%d", MAPHEIGHT);
	SetDlgItemText(hWnd, IDC_EDIT_EXPORT_MAP_HEIGHT, str);

	//Output
	SetDlgItemText(hWnd, IDC_EDIT_EXPORT_MAP_WIDTH, "********************");
	SetDlgItemText(hWnd, IDC_EDIT_EXPORT_MAP_HEIGHT, "********************");
	SetDlgItemText(hWnd, IDC_EDIT_EXPORT_MAPFILE_FACE_MAX, "********************");
	SetDlgItemText(hWnd, IDC_EDIT_EXPORT_MAPFILE_FACE_MIN, "********************");
	SetDlgItemText(hWnd, IDC_EDIT_EXPORT_MAPFILE_SIZE, "********************");
	SetDlgItemText(hWnd, IDC_EDIT_EXPORT_MAP_XMIN, "********************");
	SetDlgItemText(hWnd, IDC_EDIT_EXPORT_MAP_XMAX, "********************");
	SetDlgItemText(hWnd, IDC_EDIT_EXPORT_MAP_YMIN, "********************");
	SetDlgItemText(hWnd, IDC_EDIT_EXPORT_MAP_YMAX, "********************");
	SetDlgItemText(hWnd, IDC_EDIT_EXPORT_MAPFILE_MAXFACE_RECT, "********");
	SetDlgItemText(hWnd, IDC_EDIT_EXPORT_MAPFILE_MAXFACE_XFROM, "********");
	SetDlgItemText(hWnd, IDC_EDIT_EXPORT_MAPFILE_MAXFACE_YFROM, "********");
	SetDlgItemText(hWnd, IDC_EDIT_EXPORT_MAP_SUM_X, "100");
	SetDlgItemText(hWnd, IDC_EDIT_EXPORT_MAP_SUM_Y, "100");

	// Set the range and increment of the progress bar. 
    SendDlgItemMessage(hWnd, IDC_PROGRESS_MAP, PBM_SETRANGE, 0, MAKELPARAM(0, 100)); 
    SendDlgItemMessage(hWnd, IDC_PROGRESS_MAP, PBM_SETSTEP, (WPARAM) 1, 0);
}

static VOID CalcMapping(HWND hWnd)
{
int			k,i;
Interface	*ip;
char		str[128];



	ip = thrPar->ip;
	
	i = GetSelID(ip);

	if( (i < 0) || (i > 65000) ) // select qilingani umuman bo'lmasa
	{
		MessageBox(NULL,"Hech qanaqa obyekt select qilinmagan!","Diqqat",MB_OK);
		return;
	}

	//Output results
	SetDlgItemText(hWnd, IDC_EDIT_EXPORT_MAP_WIDTH, "********************");
	SetDlgItemText(hWnd, IDC_EDIT_EXPORT_MAP_HEIGHT, "********************");
	SetDlgItemText(hWnd, IDC_EDIT_EXPORT_MAPFILE_FACE_MAX, "********************");
	SetDlgItemText(hWnd, IDC_EDIT_EXPORT_MAPFILE_FACE_MIN, "********************");
	SetDlgItemText(hWnd, IDC_EDIT_EXPORT_MAPFILE_SIZE, "********************");
	SetDlgItemText(hWnd, IDC_EDIT_EXPORT_MAP_XMIN, "********************");
	SetDlgItemText(hWnd, IDC_EDIT_EXPORT_MAP_XMAX, "********************");
	SetDlgItemText(hWnd, IDC_EDIT_EXPORT_MAP_YMIN, "********************");
	SetDlgItemText(hWnd, IDC_EDIT_EXPORT_MAP_YMAX, "********************");
	SetDlgItemText(hWnd, IDC_EDIT_EXPORT_MAPFILE_MAXFACE_RECT, "********");
	SetDlgItemText(hWnd, IDC_EDIT_EXPORT_MAPFILE_MAXFACE_XFROM, "********");
	SetDlgItemText(hWnd, IDC_EDIT_EXPORT_MAPFILE_MAXFACE_YFROM, "********");

	UpdateWindow(hWnd);

	float XMin = FLT_MAX, XMax = -FLT_MAX, YMin = FLT_MAX, YMax = -FLT_MAX; 
	MAXFACES  = -32767;
	MINFACES  =  32767;
	vx1 = 0; vx2 = 1; vx3 = 2;
	for(k = 0; k <= i; k++)
	{
		GetMeshsMaxMin(k, ip, &XMin, &XMax, &YMin, &YMax);
		//Progress
		SendDlgItemMessage(hWnd, IDC_PROGRESS_MAP, PBM_SETPOS, (WPARAM)(100 * k / (i+1)), 0);
	}
	sprintf(str, "%.4f", XMin);
	SetDlgItemText(hWnd, IDC_EDIT_EXPORT_MAP_XMIN, str);
	sprintf(str, "%.4f", XMax);
	SetDlgItemText(hWnd, IDC_EDIT_EXPORT_MAP_XMAX, str);
	sprintf(str, "%.4f", YMin);
	SetDlgItemText(hWnd, IDC_EDIT_EXPORT_MAP_YMIN, str);
	sprintf(str, "%.4f", YMax);
	SetDlgItemText(hWnd, IDC_EDIT_EXPORT_MAP_YMAX, str);

	int widthCount = 0, heigtCount = 0;
	GetDlgItemText(hWnd, IDC_EDIT_EXPORT_MAP_SUM_X, str, 128);
	widthCount = atoi(str);
	GetDlgItemText(hWnd, IDC_EDIT_EXPORT_MAP_SUM_Y, str, 128);
	heigtCount = atoi(str);
	if( (!widthCount) || (!heigtCount) )
	{
		MessageBox(NULL, "Diqqat, xatolik!!!", "X/Y b-cha sonini to'gri kiriting.", MB_OK);
		return;
	}

	float depth, height;
	depth  = (XMax - XMin) / widthCount;
	height = (YMax - YMin) / heigtCount;
	sprintf(str, "%.4f", depth);
	SetDlgItemText(hWnd, IDC_EDIT_EXPORT_MAP_WIDTH, str);
	sprintf(str, "%.4f", height);
	SetDlgItemText(hWnd, IDC_EDIT_EXPORT_MAP_HEIGHT, str);

	//faylni ochamiz:
	FILE *stream;
	GetDlgItemText(hWnd, IDC_EDIT_EXPORT_MAPFILE_NAME, str, 128);
	if( (stream = fopen( str, "w+" )) == NULL )
	{
		MessageBox(NULL, str, "faylini ocholmadim", MB_OK);
		return;
	}

	fprintf(stream, "%d\n%s\n\t%s\n\t\t%d\t%d\t%f\t%f\t%f\t%f\t%f\t%f\n", 
					widthCount*heigtCount, 
					"MAXF",
					"RREC",
					widthCount,
					heigtCount,
					XMin, 
					XMax, 
					YMin, 
					YMax, 
					depth, 
					height);

	//to'rtburchaklarga bo'lib, ma'lumotlarni yig'amiz;
	int numRect, numRealRect;
	numRect = numRealRect = 0;
	float MaxXFrom, MaxYFrom;

	float h = YMin;

	for(int hc = 0; hc < heigtCount; hc++, h += height)
	{
		float w = XMin;

		for(int wc = 0; wc < widthCount; wc++, w += depth)
		{
			int sumFaceInRect;
			BOOL bNumRect;

			numRect++;
			bNumRect = FALSE;
			sumFaceInRect = 0;
			for(k = 0; k <= i; k++)
			{
				if(IsMeshFaceInRect(k, ip, &w, &h, &depth, &height))
				{
					if(!bNumRect)
					{
						bNumRect = TRUE;
						fprintf(stream, "\n%d\t%s\t%d\t%s", ++numRealRect, " R:", numRect, "NUMMESH");						
					}
					fprintf(stream, "\n%s\t", "  Mesh");
					int facecount = GetMeshsFacesInRect(stream, k, ip, &w, &h, &depth, &height);
					sumFaceInRect += facecount;
					if(MAXFACES < sumFaceInRect)
					{
						MAXFACES = sumFaceInRect; MAXR = numRect;
						MaxXFrom = w; MaxYFrom = h;
					}
					if(MINFACES > sumFaceInRect)
						MINFACES = sumFaceInRect;
				}
			}
		}
		//Progress
		SendDlgItemMessage(hWnd, IDC_PROGRESS_MAP, PBM_SETPOS, (WPARAM)(100 * h / fabs(YMax - YMin)), 0); 
	}

	SendDlgItemMessage(hWnd, IDC_PROGRESS_MAP, PBM_SETPOS, (WPARAM)(100), 0); 

	// Max-Min feyslar ***
	sprintf(str, "%d", MAXFACES);
	SetDlgItemText(hWnd, IDC_EDIT_EXPORT_MAPFILE_FACE_MAX, str);
	sprintf(str, "%d", MINFACES);
	SetDlgItemText(hWnd, IDC_EDIT_EXPORT_MAPFILE_FACE_MIN, str);
	sprintf(str, "%d", MAXR);
	SetDlgItemText(hWnd, IDC_EDIT_EXPORT_MAPFILE_MAXFACE_RECT, str);
	sprintf(str, "%.4f", MaxXFrom);
	SetDlgItemText(hWnd, IDC_EDIT_EXPORT_MAPFILE_MAXFACE_XFROM, str);
	sprintf(str, "%.4f", MaxYFrom);
	SetDlgItemText(hWnd, IDC_EDIT_EXPORT_MAPFILE_MAXFACE_YFROM, str);


	// fayl hajmi ***
	fpos_t pos;
	fgetpos(stream, &pos);	
	sprintf(str, "%d", pos);
	SetDlgItemText(hWnd, IDC_EDIT_EXPORT_MAPFILE_SIZE, str);


	//MAX face ni yozamiz
	fseek(stream, SEEK_SET, 0);
	while(!feof(stream))
	{
		fpos_t pos;
		fgetpos(stream, &pos);

		fscanf(stream, "%s", str);
		if(!Strcmp("MAXF", str))
		{
			fsetpos(stream, &pos);
			sprintf(str, "%d", MAXFACES);
			fprintf(stream, "%s%s%s", " ", str, "   ");
			break;
		}
	}

	//numRealRect
	fseek(stream, SEEK_SET, 0);
	while(!feof(stream))
	{
		fpos_t pos;
		fgetpos(stream, &pos);

		fscanf(stream, "%s", str);
		if(!Strcmp("RREC", str))
		{
			fsetpos(stream, &pos);
			sprintf(str, "%d", numRealRect);
			fprintf(stream, "%s%s%s", " ", str, "        ");
			break;
		}
	}

	fclose(stream);
}


static VOID GetMeshsMaxMin(int k, Interface *ip, float *XMin, float *XMax, float *YMin, float *YMax)
{
INode		*node;
Matrix3		tm;     
BOOL		needDel;
Mesh*		mesh;
Point3		v;
DWORD		numFaces, numVerts, ind[3];


	node = ip->GetSelNode(k);
    tm   = node->GetObjTMAfterWSM(ip->GetTime());

	ObjectState os = node->EvalWorldState(ip->GetTime());
	if (!os.obj || os.obj->SuperClassID()!=GEOMOBJECT_CLASS_ID) 
		return; 

//*******  Mesh qilib ko'ramiz **********************************************
	TriObject* tri = GetTriObjectFromNode(node, ip->GetTime(), needDel);
	if (!tri) 
	{
		MessageBox(NULL,"Obyektni editmesh qilolmayman!","Diqqat",MB_OK);
		return;
	}
	mesh = &tri->GetMesh();

	numVerts = mesh->getNumVerts();
	numFaces = (DWORD)(mesh->getNumFaces());

	for (i=0; i<numFaces; i++) 
	{
		ind0   = (WORD)mesh->faces[i].v[vx1];
		ind1   = (WORD)mesh->faces[i].v[vx2];
		ind2   = (WORD)mesh->faces[i].v[vx3];

		v = tm * mesh->verts[ind0];
		if(*XMin  > v[0])
			*XMin = v[0];
		if(*XMax  < v[0])
			*XMax = v[0];
		if(*YMin  > v[1])
			*YMin = v[1];
		if(*YMax  < v[1])
			*YMax = v[1];
		v = tm * mesh->verts[ind1];
		if(*XMin  > v[0])
			*XMin = v[0];
		if(*XMax  < v[0])
			*XMax = v[0];
		if(*YMin  > v[1])
			*YMin = v[1];
		if(*YMax  < v[1])
			*YMax = v[1];
		v = tm * mesh->verts[ind2];
		if(*XMin  > v[0])
			*XMin = v[0];
		if(*XMax  < v[0])
			*XMax = v[0];
		if(*YMin  > v[1])
			*YMin = v[1];
		if(*YMax  < v[1])
			*YMax = v[1];
	}

	return;
}

static BOOL IsMeshFaceInRect(int k, Interface *ip, float *XFrom, float *YFrom, float *width, float *height)
{
INode		*node;
Matrix3		tm, tmRot;     
BOOL		needDel;
Mesh*		mesh;
Point3		v0, v1, v2;
DWORD		numFaces, numVerts, ind[3];


	node = ip->GetSelNode(k);
    tm   = node->GetObjTMAfterWSM(ip->GetTime());
	tmRot= tm;
	tmRot.NoTrans();
	tmRot.NoScale();

	ObjectState os = node->EvalWorldState(ip->GetTime());
	if (!os.obj || os.obj->SuperClassID()!=GEOMOBJECT_CLASS_ID) 
		return FALSE; 

//*******  Mesh qilib ko'ramiz **********************************************
	TriObject* tri = GetTriObjectFromNode(node, ip->GetTime(), needDel);
	if (!tri) 
	{
		MessageBox(NULL,"Obyektni editmesh qilolmayman!","Diqqat",MB_OK);
		return FALSE;
	}
	mesh = &tri->GetMesh();

	numVerts = mesh->getNumVerts();
	numFaces = (DWORD)(mesh->getNumFaces());

	for (i=0; i<numFaces; i++) 
	{
		//agar normali yoqoriga qaramagan bo'lsa, chiq:
		Point3 faceNormal;
 		faceNormal  = tmRot * mesh->getFaceNormal(i);
		if(faceNormal.z < -0.1f)
			continue;
		
		ind0   = (WORD)mesh->faces[i].v[vx1];
		ind1   = (WORD)mesh->faces[i].v[vx2];
		ind2   = (WORD)mesh->faces[i].v[vx3];

//		float pt[2]; float tra[6];
		v0 = tm * mesh->verts[ind0];
		v1 = tm * mesh->verts[ind1];
		v2 = tm * mesh->verts[ind2];

		//uchburchak umuman rectdan chetda bo'lsa hisoblab o'tirma
		if(v0[0] < *XFrom) 
			if(v1[0] < *XFrom) 
				if(v2[0] < *XFrom) 
					continue;
		if(v0[0] > *XFrom + *width) 
			if(v1[0] > *XFrom + *width) 
				if(v2[0] > *XFrom + *width) 
					continue;
		if(v0[1] < *YFrom) 
			if(v1[1] < *YFrom) 
				if(v2[1] < *YFrom) 
					continue;
		if(v0[1] > *YFrom + *height)
			if(v1[1] > *YFrom + *height) 
				if(v2[1] > *YFrom + *height) 
					continue;

//ayqash-payqashi b-n birga:

/*
		//uchburchak uchidan birortasi rect ichida bo'lsa
		if( (*XFrom <= v0[0]) && (*XFrom + *width > v0[0]) && (*YFrom <= v0[1]) && (*YFrom + *height > v0[1]) )
			return TRUE;
		tra[0] = v0[0]; tra[1] = v0[1];

		if( (*XFrom <= v1[0]) && (*XFrom + *width > v1[0]) && (*YFrom <= v1[1]) && (*YFrom + *height > v1[1]) )
			return TRUE;
		tra[2] = v1[0]; tra[3] = v1[1];

		if( (*XFrom <= v2[0]) && (*XFrom + *width > v2[0]) && (*YFrom <= v2[1]) && (*YFrom + *height > v2[1]) )
			return TRUE;
		tra[4] = v2[0]; tra[5] = v2[1];



		//endi uchburchak uchlari rect dan tashqaridayu, rect uning ichida bo'lsachi
		pt[0] = *XFrom; pt[1] = *YFrom;
		if(1 == IsPointInwardTriangle(pt, tra))
			return TRUE;

		pt[1] = (*YFrom) + (*height);
		if(1 == IsPointInwardTriangle(pt, tra))
			return TRUE;
	
		pt[0] = (*XFrom) + (*width); pt[1] = (*YFrom);
		if(1 == IsPointInwardTriangle(pt, tra))
			return TRUE;

		pt[1] = (*YFrom) + (*height);
		if(1 == IsPointInwardTriangle(pt, tra))
			return TRUE;

	
	//yana shunday bo'lishi ham mumkinki, to'rtburchakning ham birorta uchi feys ichida
	//yotmaydi, feysning ham birorta uchi to'rtburchak ichida yotmaydiku, lekin ular 
	//kesishgan:
	float v1[4], v2[4];
	//1. Rect ni bir tomoni:
	v1[0] = *XFrom; v1[1] = *YFrom; v1[2] = *XFrom + *width; v1[3] = *YFrom;
		//1.1. Feysni bir tomoni:
		v2[0] = tra[0]; v2[1] = tra[1]; v2[2] = tra[2]; v2[3] = tra[3]; 
		int ret; ret = AreVectorsCrosses(v1, v2);
		if(ret != -1)
			return TRUE;
		//1.2. Feysni ikkinchi tomoni:
		v2[0] = tra[0]; v2[1] = tra[1]; v2[2] = tra[4]; v2[3] = tra[5]; 
		ret = AreVectorsCrosses(v1, v2);
		if(ret != -1)
			return TRUE;
		//1.3. Feysni uchinci tomoni:
		v2[0] = tra[2]; v2[1] = tra[3]; v2[2] = tra[4]; v2[3] = tra[5]; 
		ret = AreVectorsCrosses(v1, v2);
		if(ret != -1)
			return TRUE;
	//2. Rect ni ikkinchi tomoni:
	v1[0] = *XFrom; v1[1] = *YFrom; v1[2] = *XFrom; v1[3] = *YFrom + *height;
		//2.1. Feysni bir tomoni:
		v2[0] = tra[0]; v2[1] = tra[1]; v2[2] = tra[2]; v2[3] = tra[3]; 
		ret = AreVectorsCrosses(v1, v2);
		if(ret != -1)
			return TRUE;
		//2.2. Feysni ikkinchi tomoni:
		v2[0] = tra[0]; v2[1] = tra[1]; v2[2] = tra[4]; v2[3] = tra[5]; 
		ret = AreVectorsCrosses(v1, v2);
		if(ret != -1)
			return TRUE;
		//2.3. Feysni uchinci tomoni:
		v2[0] = tra[2]; v2[1] = tra[3]; v2[2] = tra[4]; v2[3] = tra[5]; 
		ret = AreVectorsCrosses(v1, v2);
		if(ret != -1)
			return TRUE;
	//3. Rect ni uchinchi tomoni:
	v1[0] = *XFrom + *width; v1[1] = *YFrom; v1[2] = *XFrom + *width; v1[3] = *YFrom + *height;
		//3.1. Feysni bir tomoni:
		v2[0] = tra[0]; v2[1] = tra[1]; v2[2] = tra[2]; v2[3] = tra[3]; 
		ret = AreVectorsCrosses(v1, v2);
		if(ret != -1)
			return TRUE;
		//3.2. Feysni ikkinchi tomoni:
		v2[0] = tra[0]; v2[1] = tra[1]; v2[2] = tra[4]; v2[3] = tra[5]; 
		ret = AreVectorsCrosses(v1, v2);
		if(ret != -1)
			return TRUE;
		//3.3. Feysni uchinci tomoni:
		v2[0] = tra[2]; v2[1] = tra[3]; v2[2] = tra[4]; v2[3] = tra[5]; 
		ret = AreVectorsCrosses(v1, v2);
		if(ret != -1)
			return TRUE;
	//4. Rect ni to'rtinchi tomoni:
	v1[0] = *XFrom; v1[1] = *YFrom + *height; v1[2] = *XFrom + *width; v1[3] = *YFrom + *height;
		//4.1. Feysni bir tomoni:
		v2[0] = tra[0]; v2[1] = tra[1]; v2[2] = tra[2]; v2[3] = tra[3]; 
		ret = AreVectorsCrosses(v1, v2);
		if(ret != -1)
			return TRUE;
		//4.2. Feysni ikkinchi tomoni:
		v2[0] = tra[0]; v2[1] = tra[1]; v2[2] = tra[4]; v2[3] = tra[5]; 
		ret = AreVectorsCrosses(v1, v2);
		if(ret != -1)
			return TRUE;
		//4.3. Feysni uchinci tomoni:
		v2[0] = tra[2]; v2[1] = tra[3]; v2[2] = tra[4]; v2[3] = tra[5]; 
		ret = AreVectorsCrosses(v1, v2);
		if(ret != -1)
*/			return TRUE;

	}

	return FALSE;
}

static int GetMeshsFacesInRect(FILE *stream, int k, Interface *ip, float *XFrom, float *YFrom, float *width, float *height)
{
INode		*node;
Matrix3		tm, tmRot;     
BOOL		needDel;
Mesh*		mesh;
Point3		v0, v1, v2;
DWORD		numFaces, numVerts, ind0, ind1, ind2;


	node = ip->GetSelNode(k);
	nodeName = node->GetName();//node ni nomini olamiz
	fprintf(stream, "%s\t%s\t%d\t", nodeName, "T:", 999);

    tm = node->GetObjTMAfterWSM(ip->GetTime());
	tmRot= tm;
	tmRot.NoTrans();
	tmRot.NoScale();

	ObjectState os = node->EvalWorldState(ip->GetTime());
	if (!os.obj || os.obj->SuperClassID()!=GEOMOBJECT_CLASS_ID) 
		return 0;  

//*******  Mesh qilib ko'ramiz **********************************************
	TriObject* tri = GetTriObjectFromNode(node, ip->GetTime(), needDel);
	if (!tri) 
	{
		MessageBox(NULL,"Obyektni editmesh qilolmayman!","Diqqat",MB_OK);
		return 0;
	}
	mesh = &tri->GetMesh();

	numVerts = mesh->getNumVerts();
	numFaces = (DWORD)(mesh->getNumFaces());

	int facecount;
	facecount = 0;
	for (i=0; i<numFaces; i++) 
	{

		//agar normali yoqoriga qaramagan bo'lsa, chiq:
		Point3 faceNormal;
		faceNormal  = tmRot * mesh->getFaceNormal(i);
		if(faceNormal.z < -0.1f)
			continue;


//		float pt[2]; float tra[6];

		ind0   = (WORD)mesh->faces[i].v[vx1];
		ind1   = (WORD)mesh->faces[i].v[vx2];
		ind2   = (WORD)mesh->faces[i].v[vx3];

		v0 = tm * mesh->verts[ind0];
		v1 = tm * mesh->verts[ind1];
		v2 = tm * mesh->verts[ind2];

		//uchburchak umuman rectdan chetda bo'lsa hisoblab o'tirma
		if(v0[0] < *XFrom) 
			if(v1[0] < *XFrom) 
				if(v2[0] < *XFrom) 
					continue;
		if(v0[0] > *XFrom + *width) 
			if(v1[0] > *XFrom + *width) 
				if(v2[0] > *XFrom + *width) 
					continue;
		if(v0[1] < *YFrom) 
			if(v1[1] < *YFrom) 
				if(v2[1] < *YFrom) 
					continue;
		if(v0[1] > *YFrom + *height)
			if(v1[1] > *YFrom + *height) 
				if(v2[1] > *YFrom + *height) 
					continue;

//ayqash-payqashi b-n birga:

/*
		//uchburchak ichlaridan biri rect ichida bo'lsa:
		if( (*XFrom <= v0[0]) && (*XFrom + *width > v0[0]) && (*YFrom <= v0[1]) && (*YFrom + *height > v0[1]) )
			goto Force;
		tra[0] = v0[0]; tra[1] = v0[1];

		if( (*XFrom <= v1[0]) && (*XFrom + *width > v1[0]) && (*YFrom <= v1[1]) && (*YFrom + *height > v1[1]) )
			goto Force;
		tra[2] = v1[0]; tra[3] = v1[1];

		if( (*XFrom <= v2[0]) && (*XFrom + *width > v2[0]) && (*YFrom <= v2[1]) && (*YFrom + *height > v2[1]) )
			goto Force;
		tra[4] = v2[0]; tra[5] = v2[1];





		//endi uchburchak uchlari rect dan tashqaridayu, rect uning ichida bo'lsachi
		pt[0] = *XFrom; pt[1] = *YFrom;
		if(1 == IsPointInwardTriangle(pt, tra))
			goto Force;

		pt[1] = (*YFrom) + (*height);
		if(1 == IsPointInwardTriangle(pt, tra))
			goto Force;
	
		pt[0] = (*XFrom) + (*width); pt[1] = *YFrom;
		if(1 == IsPointInwardTriangle(pt, tra))
			goto Force;

		pt[1] = (*YFrom) + (*height);
		if(1 == IsPointInwardTriangle(pt, tra))
			goto Force;





		//yana shunday bo'lishi ham mumkinki, to'rtburchakning ham birorta uchi 
		//feys ichida yotmaydi, feysning ham birorta uchi to'rtburchak ichida 
		//yotmaydiku, lekin ular kesishgan:
		float v1[4], v2[4];
		//1. Rect ni bir tomoni:
		v1[0] = *XFrom; v1[1] = *YFrom; v1[2] = *XFrom + *width; v1[3] = *YFrom;
			//1.1. Feysni bir tomoni:
			v2[0] = tra[0]; v2[1] = tra[1]; v2[2] = tra[2]; v2[3] = tra[3]; 
			int ret; ret = AreVectorsCrosses(v1, v2);
			if(ret != -1)
				goto Force;
			//1.2. Feysni ikkinchi tomoni:
			v2[0] = tra[0]; v2[1] = tra[1]; v2[2] = tra[4]; v2[3] = tra[5]; 
			ret; ret = AreVectorsCrosses(v1, v2);
			if(ret != -1)
				goto Force;
			//1.3. Feysni uchinci tomoni:
			v2[0] = tra[2]; v2[1] = tra[3]; v2[2] = tra[4]; v2[3] = tra[5]; 
			ret = AreVectorsCrosses(v1, v2);
			if(ret != -1)
				goto Force;
		//2. Rect ni ikkinchi tomoni:
		v1[0] = *XFrom; v1[1] = *YFrom; v1[2] = *XFrom; v1[3] = *YFrom + *height;
			//2.1. Feysni bir tomoni:
			v2[0] = tra[0]; v2[1] = tra[1]; v2[2] = tra[2]; v2[3] = tra[3]; 
			ret = AreVectorsCrosses(v1, v2);
			if(ret != -1)
				goto Force;
			//2.2. Feysni ikkinchi tomoni:
			v2[0] = tra[0]; v2[1] = tra[1]; v2[2] = tra[4]; v2[3] = tra[5]; 
			ret = AreVectorsCrosses(v1, v2);
			if(ret != -1)
				goto Force;
			//2.3. Feysni uchinci tomoni:
			v2[0] = tra[2]; v2[1] = tra[3]; v2[2] = tra[4]; v2[3] = tra[5]; 
			ret = AreVectorsCrosses(v1, v2);
			if(ret != -1)
				goto Force;
		//3. Rect ni uchinchi tomoni:
		v1[0] = *XFrom + *width; v1[1] = *YFrom; v1[2] = *XFrom + *width; v1[3] = *YFrom + *height;
			//3.1. Feysni bir tomoni:
			v2[0] = tra[0]; v2[1] = tra[1]; v2[2] = tra[2]; v2[3] = tra[3]; 
			ret = AreVectorsCrosses(v1, v2);
			if(ret != -1)
				goto Force;
			//3.2. Feysni ikkinchi tomoni:
			v2[0] = tra[0]; v2[1] = tra[1]; v2[2] = tra[4]; v2[3] = tra[5]; 
			ret = AreVectorsCrosses(v1, v2);
			if(ret != -1)
				goto Force;
			//3.3. Feysni uchinci tomoni:
			v2[0] = tra[2]; v2[1] = tra[3]; v2[2] = tra[4]; v2[3] = tra[5]; 
			ret = AreVectorsCrosses(v1, v2);
			if(ret != -1)
				goto Force;
		//4. Rect ni to'rtinchi tomoni:
		v1[0] = *XFrom; v1[1] = *YFrom + *height; v1[2] = *XFrom + *width; v1[3] = *YFrom + *height;
			//4.1. Feysni bir tomoni:
			v2[0] = tra[0]; v2[1] = tra[1]; v2[2] = tra[2]; v2[3] = tra[3]; 
			ret = AreVectorsCrosses(v1, v2);
			if(ret != -1)
				goto Force;
			//4.2. Feysni ikkinchi tomoni:
			v2[0] = tra[0]; v2[1] = tra[1]; v2[2] = tra[4]; v2[3] = tra[5]; 
			ret = AreVectorsCrosses(v1, v2);
			if(ret != -1)
				goto Force;
			//4.3. Feysni uchinci tomoni:
			v2[0] = tra[2]; v2[1] = tra[3]; v2[2] = tra[4]; v2[3] = tra[5]; 
			ret = AreVectorsCrosses(v1, v2);
			if(ret != -1)
*/			{
/*	Force:*/	fprintf(stream, "%d\t", i); 
				facecount++;
			}
	}

	return facecount;
}


VOID GetAngleFrVectors2D(double *v1, double *v2, double *out) 
{
double	skalyar = v1[0]*v2[0] + v1[1]*v2[1];
		
		if(skalyar == 0.f)
		{
			out[0] = 0;
			return;
		}
double  len1 = sqrt( v1[0]*v1[0] + v1[1]*v1[1] );
		if(len1 < 1e-6f)
		{
			out[0] = 0;
			return;
		}
double  len2 = sqrt( v2[0]*v2[0] + v2[1]*v2[1] );
		if(len2 < 1e-6f)
		{
			out[0] = 0;
			return;
		}
		out[0] = m_Acos(skalyar / (len1 * len2));
		return;
}

//4talik stride qilingan, tra 
int IsPointInwardTriangle(float *pt, float *tra)//tra  ga uchta pt
{
double v0[2], v1[2], v2[2];

	v0[0] = tra[0 ] - pt[0];
	v0[1] = tra[1 ] - pt[1];

	v1[0] = tra[2 ] - pt[0];
	v1[1] = tra[3 ] - pt[1];

	v2[0] = tra[4 ] - pt[0];
	v2[1] = tra[5 ] - pt[1];

	double f0, f1, f2, sum;

	GetAngleFrVectors2D(&f0, v0, v1); 
	GetAngleFrVectors2D(&f1, v0, v2); 
	GetAngleFrVectors2D(&f2, v1, v2); 


	if( (f0 == 0.f) || (f1 == 0.f) || (f2 == 0.f) || (f0 == PI) || (f1 == PI) || (f2 == PI) )
		return 0;//Uchburchak qirrasida		

	sum = f0 + f1 + f2;

	if( (sum > 2*PI - 0.002f) &&  (sum < 2*PI + 0.002f) )
		return 1;//Uchburchak ichida
	else
		return -1;//Uchburchak tashqarisida

}


inline float getMin(float a, float b)
{
   return a>b ? b : a;
}

inline float getMax(float a, float b)
{
   return a>b ? a : b;
}




//ret==0  ->Ustida yotadi, bir-birini chulg'ab;
//ret==-1 ->Kesishmaydi;
//ret==1  ->Kesishadi;
int AreVectorsCrosses(float *v1, float *v2)
{
//1.Kordinata markazini V1 boshiga suramiz:
	float V1S[4], V2S[4];
	V1S[0] = V1S[1] = 0.f; 
	V1S[2] = v1[2] - v1[0]; 
	V1S[3] = v1[3] - v1[1]; 

	V2S[0] = v2[0] - v1[0]; 
	V2S[1] = v2[1] - v1[1]; 
	V2S[2] = v2[2] - v1[0]; 
	V2S[3] = v2[3] - v1[1]; 


//2.Kordinatani shunday suramizki, X oqi 1-vektor bo'yicha yo'nalsin:
	float k1 = m_Atan(V1S[3] - V1S[1], V1S[2] - V1S[0]);

	float V1B[4], V2B[4];

	//V1B[0] = /*V1S[0]*m11 + V1S[1]*m21 + */ m41;//X
	//V1B[1] = /*V1S[0]*m12 + V1S[1]*m22 + */ m42;//Y
	V1B[0] = V1B[1] = 0.f;

	//V1B[2] = V1S[2]*m11 + V1S[3]*m21 + m41;//X
	//V1B[3] = V1S[2]*m12 + V1S[3]*m22 + m42;//Y
	V1B[2] =  V1S[2]*cosf(ang1) + V1S[3]*sinf(ang1);
	V1B[3] = -V1S[2]*sinf(ang1) + V1S[3]*cosf(ang1);

	//V2B[0] = V2S[0]*m11 + V2S[1]*m21 + m41;//X
	//V2B[1] = V2S[0]*m12 + V2S[1]*m22 + m42;//Y
	V2B[0] =  V2S[0]*cosf(ang1) + V2S[1]*sinf(ang1);
	V2B[1] = -V2S[0]*sinf(ang1) + V2S[1]*cosf(ang1);

	//V2B[2] = V2S[2]*m11 + V2S[3]*m21 + m41;//X
	//V2B[3] = V2S[2]*m12 + V2S[3]*m22 + m42;//Y
	V2B[2] =  V2S[2]*cosf(ang1) + V2S[3]*sinf(ang1);
	V2B[3] = -V2S[2]*sinf(ang1) + V2S[3]*cosf(ang1);


	//0 ga yaqinini 0 qilamiz:
	if( (V1B[0] < 0.00001f) && (V1B[0] > -0.00001f) )
		V1B[0] = 0.f; 
	if( (V1B[1] < 0.00001f) && (V1B[1] > -0.00001f) )
		V1B[1] = 0.f; 
	if( (V1B[2] < 0.00001f) && (V1B[2] > -0.00001f) )
		V1B[2] = 0.f; 
	if( (V1B[3] < 0.00001f) && (V1B[3] > -0.00001f) )
		V1B[3] = 0.f; 
	if( (V2B[0] < 0.00001f) && (V2B[0] > -0.00001f) )
		V2B[0] = 0.f; 
	if( (V2B[1] < 0.00001f) && (V2B[1] > -0.00001f) )
		V2B[1] = 0.f; 
	if( (V2B[2] < 0.00001f) && (V2B[2] > -0.00001f) )
		V2B[2] = 0.f; 
	if( (V2B[3] < 0.00001f) && (V2B[3] > -0.00001f) )
		V2B[3] = 0.f; 



	//agar V2 tepada bo'lsa:
	if( (V2B[1] > 0.f) && (V2B[3] > 0.f) )
		return -1;

	//agar V2 pastda bo'lsa:
	if( (V2B[1] < 0.f) && (V2B[3] < 0.f) )
		return -1;

	//agar V2 chapda bo'lsa:
	if( (V2B[0] < getMin(V1B[0], V1B[2])) && (V2B[2] < getMin(V1B[0], V1B[2])) )
		return -1;

	//agar V2 o'ngda bo'lsa:
	if( (V2B[0] > getMax(V1B[0], V1B[2])) && (V2B[2] > getMax(V1B[0], V1B[2])) )
		return -1;

	//ustida yotsa:
	if( (V2B[1] == 0.f) && (V2B[3] == 0.f) )
	{
		if( (V2B[0] >= getMin(V1B[0], V1B[2])) && (V2B[2] <= getMax(V1B[0], V1B[2])) )
			return 0;
		else
			return -1;//chapda, yoki o'ngda parallel
	}

	//ayqash ham bo'lishi mumkin:
	//burchagini topaylikchi:
	float k2;//, ang2;
	k2 = (float)(V2B[3] - V2B[1]) / (float)(V2B[2] - V2B[0]);//tangensi
	//ang2 = (float)(atan(k2));

	//x o'qi b-n kesishgan nuqtasini topamiz:
	float XKes;
	if(k2 != 0.f)
		XKes = V2B[0] - V2B[1] / k2;
	else
		XKes = 0.f;

	if( (XKes >= getMin(V1B[0], V1B[2])) && (XKes < getMax(V1B[0], V1B[2])) )
		return 1;
	else
		return -1;


	return 0;
}
