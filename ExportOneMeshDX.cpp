
#include "ExpDX.h"
#include "modstack.h"
#include "d3dx9.h"
#include <simpobj.h>
#include <simpmod.h>
#include "../../../Libs/Math/mPlane.h"


FILE				*streamBin;
char				fileName[256]="";
int					vx[3],flatMapChan;
float				eul[3];
INode				*node;
TCHAR				*nodeName;
BOOL				needDel,bFlatten = FALSE;
DWORD				alreadyReaded,numVerts,numFaces,ind[3],t_ind[3],
					NumNewVert,indVertArrPtr,indEdgeArrPtr, 
					*ptrToInds,*indexes,*edgeTypesArray;
__int64				*indexedEdgeArray;
IUnwrapMod			*FlattenMod;
Mesh*				mesh;
Point3				scPv(1.0f, 1.0f, 1.0f), *indexedVertArray;
AffineParts			ap;
VERTS_T2F_N3F_V3F   *old_tv1_verts;
FACES_NORMAL_AND_SMOOTH_GROUPE	
					*old_faces;

/*static VOID DeleteEdge(WORD e)
{
	for(WORD i=e; i<indEdgeArrPtr-1; i++)
	{	WORD *arrPre  = (WORD*)&indexedEdgeArray[i  ];
		WORD *arrPost = (WORD*)&indexedEdgeArray[i+1];
		*arrPre     = *arrPost;
		*(arrPre+1) = *(arrPost+1);
		*(arrPre+2) = *(arrPost+2);
		*(arrPre+3) = *(arrPost+3);
	}
	indEdgeArrPtr--;
	return;
}*/

static BOOL FindEdgeSecFace(WORD e)
{
	WORD *arr = (WORD*)&indexedEdgeArray[e];
	WORD V0 = *arr;
	WORD V1 = *(arr+1);
	WORD F0 = *(arr+2);
	for(DWORD i=0; i<numFaces; i++)// 3*numFaces
	{	if(i != F0)
		{	if( ( (ptrToInds[i*3  ] == V0) && (ptrToInds[i*3+1] == V1) ) ||
			    ( (ptrToInds[i*3  ] == V1) && (ptrToInds[i*3+1] == V0) ) ||
				( (ptrToInds[i*3+1] == V0) && (ptrToInds[i*3+2] == V1) ) ||
			    ( (ptrToInds[i*3+1] == V1) && (ptrToInds[i*3+2] == V0) ) ||
				( (ptrToInds[i*3+2] == V0) && (ptrToInds[i*3  ] == V1) ) ||
			    ( (ptrToInds[i*3+2] == V1) && (ptrToInds[i*3  ] == V0) ) )
			{	*(arr+3) = (WORD)i;
				return TRUE;
	}	}	}
	*(arr+3) = 0xffff;
	return FALSE;
}

//Vertex bo'yicha:
static BOOL IsEdgeAlreadyInArray(WORD v0, WORD v1)
{
	for(DWORD i=0; i<indEdgeArrPtr; i++)
	{	WORD *arr = (WORD*)&indexedEdgeArray[i];
		if( (*arr == v0) && (*(arr+1) == v1) )
			return TRUE;
		if( (*arr == v1) && (*(arr+1) == v0) )
			return TRUE;
	}
	return FALSE;
}

//Vertex bo'yicha:
static VOID AddEdgeToArray(WORD v0, WORD v1, WORD f)
{
WORD *arr = (WORD*)&indexedEdgeArray[indEdgeArrPtr++];
	 *arr     = v0;//first vertex ;
	 *(arr+1) = v1;//second vertex;
	 *(arr+2) = f ;//first face   ;
	return;
}

static VOID AddVertToArray(DWORD v)
{
	indexedVertArray[indVertArrPtr] = old_tv1_verts[v].p;
	indexes[indVertArrPtr] = v;
	ptrToInds[v] = indVertArrPtr++;
	return;
}

static BOOL IsVertAlreadyInArray(DWORD v)
{
	for(DWORD i=0; i<indVertArrPtr; i++)
	{	if(indexedVertArray[i].x == old_tv1_verts[v].p.x)
		if(indexedVertArray[i].y == old_tv1_verts[v].p.y)
		if(indexedVertArray[i].z == old_tv1_verts[v].p.z)
			return TRUE;
	}
	return FALSE;
}

static DWORD FindVertex(DWORD v)
{
DWORD ptr = 0;
	for(DWORD i=0; i<indVertArrPtr; i++)
	{	if(old_tv1_verts[v].p.x == old_tv1_verts[indexes[i]].p.x)
		if(old_tv1_verts[v].p.y == old_tv1_verts[indexes[i]].p.y)
		if(old_tv1_verts[v].p.z == old_tv1_verts[indexes[i]].p.z)
			{ ptr = i; break; }
	}
	return ptr;
}

//old_tv1_verts dagi Vertexlarni indexlash:
static DWORD IndexVertexes()
{
DWORD i;

	indexedVertArray = new Point3 [NumNewVert];
	ptrToInds        = new DWORD  [NumNewVert];
	indexes          = new DWORD  [NumNewVert];//Aslida bundan kam, indVertArrPtr cha;
	indVertArrPtr    = 0;                      //lekin indVertArrPtr hali hisoblanmag'on;

	for(i=0; i<NumNewVert; i++)// 3*numFaces
	{	if(IsVertAlreadyInArray(i))
			ptrToInds[i] = FindVertex(i);
		else
			AddVertToArray(i);	
	}

	return indVertArrPtr; 
}

//old_tv1_verts dagi Vertexlarni indexlash:
static DWORD IndexEdges()
{
DWORD i;

	indexedEdgeArray = new __int64 [NumNewVert];
	edgeTypesArray   = new DWORD   [NumNewVert];
	indEdgeArrPtr    = 0;                      //lekin indVertArrPtr hali hisoblanmag'on;

	for(i=0; i<numFaces; i++)// 3*numFaces
	{	WORD v0 = (WORD)ptrToInds[i*3  ];
		WORD v1 = (WORD)ptrToInds[i*3+1];
		WORD v2 = (WORD)ptrToInds[i*3+2];
		if(!IsEdgeAlreadyInArray(v0, v1))
			AddEdgeToArray(v0, v1, (WORD)i);
		if(!IsEdgeAlreadyInArray(v1, v2))
			AddEdgeToArray(v1, v2, (WORD)i);
		if(!IsEdgeAlreadyInArray(v0, v2))
			AddEdgeToArray(v0, v2, (WORD)i);
	}

	//Vertexlar bo'yincha edge larni indexladik, endi feyslarini tartiblaymiz;
	for(DWORD i=0; i<indEdgeArrPtr; i++)
	{	if(!FindEdgeSecFace((WORD)i))
		{	//char s[8]; itoa(i, s, 8);
			//MessageBox(NULL, s, "- edge 2-face not founded", MB_OK);
	}	}

	//Ana endi agarda ikkita pleyni 1 xil normallik edge topilsa, uni yo'qotish:
	for(DWORD i=0; i<indEdgeArrPtr; i++)
	{	WORD *e = (WORD*)&indexedEdgeArray[i];
		WORD F0 = *(e+2);
		WORD F1 = *(e+3);
		if(F1 == 0xffff)
		{	edgeTypesArray[i] = 0x0000;//One faced.
			continue;
		}
		Point3 *V0_0 = &old_tv1_verts[F0*3  ].p;
		Point3 *V0_1 = &old_tv1_verts[F0*3+1].p;
		Point3 *V0_2 = &old_tv1_verts[F0*3+2].p;
		Point3 *V1_0 = &old_tv1_verts[F1*3  ].p;
		Point3 *V1_1 = &old_tv1_verts[F1*3+1].p;
		Point3 *V1_2 = &old_tv1_verts[F1*3+2].p;
		Point3 V0_0_1 = (*V0_1) - (*V0_0);
		Point3 V0_0_2 = (*V0_2) - (*V0_0);
		Point3 V1_0_1 = (*V1_1) - (*V1_0);
		Point3 V1_0_2 = (*V1_2) - (*V1_0);
		V0_0_1 = V0_0_1.Normalize();
		V0_0_2 = V0_0_2.Normalize();
		V1_0_1 = V1_0_1.Normalize();
		V1_0_2 = V1_0_2.Normalize();
		Point3 n0 = CrossProd(V0_0_2, V0_0_1);
		Point3 n1 = CrossProd(V1_0_2, V1_0_1);
		n0 = n0.Normalize();
		n1 = n1.Normalize();
		float dp = DotProd(n0, n1);
		if(dp > 0.999f)
			edgeTypesArray[i] = 0x0001;//Hidden,   DeleteEdge(i) edi.
		else
		{	DWORD v_F1_0 = ptrToInds[F1*3  ];
			DWORD v_F1_1 = ptrToInds[F1*3+1];
			DWORD v_F1_2 = ptrToInds[F1*3+2];
			DWORD F1othVert=0;
			if( ( (*e == v_F1_0) && (*(e+1) == v_F1_1) ) || (*e == v_F1_1) && (*(e+1) == v_F1_0) )
				F1othVert = F1*3+2;
			else if( ( (*e == v_F1_1) && (*(e+1) == v_F1_2) ) || (*e == v_F1_2) && (*(e+1) == v_F1_1) )
				F1othVert = F1*3  ;
			else if( ( (*e == v_F1_0) && (*(e+1) == v_F1_2) ) || (*e == v_F1_2) && (*(e+1) == v_F1_0) )
				F1othVert = F1*3+1;
			else
				edgeTypesArray[i] = 0x0000;//One faced.
			Point3 toF2Edge = old_tv1_verts[F1othVert].p - old_tv1_verts[F0*3].p;
			float dp = DotProd(n0, toF2Edge);
			if(dp > 0.0f)
				edgeTypesArray[i] = 0x0002;//Convolve.
			else
				edgeTypesArray[i] = 0x0003;//Convex.
	}	}

	return indEdgeArrPtr; 
}

BOOL FindFlatten()
{
int		numMods;
Object	*obj;
    
	obj = node->GetObjectRef(); 
    IDerivedObject *dObj = NULL;
    if( obj->SuperClassID() == GEN_DERIVOB_CLASS_ID )
    {   dObj =  (IDerivedObject *)(obj) ;
        numMods = dObj->NumModifiers();
    }
    else
    {   //MaxMsgBox(NULL, _T("Cannot detach this kind of object"), _T("Flatten"), MB_OK );
        return FALSE;
    }
	//char s[20];
	//wsprintf(s,"%s%d","NumCount ",numMods);
    //MaxMsgBox(NULL, s, _T("Flatten"), MB_OK );
    BOOL res   = FALSE;
	FlattenMod = NULL;
    for( int modCount =0; modCount < numMods; ++modCount)
    {   Modifier *mod = dObj->GetModifier(modCount);
        if( mod->ClassID() == Class_ID(0x02df2e3a,0x72ba4e1f) )//Unwrap
		{	FlattenMod = (IUnwrapMod *)mod;
			res = TRUE;
	}   }
	
	flatMapChan = 3;

	if(!mesh->mapSupport(flatMapChan)) 
	{	//MaxMsgBox(NULL, _T("map channel ni 3 ga qo'ying, chunki Flatteningniki 3 da"),"Xato !!!", MB_OK );
		return FALSE;
	}

	return res;
}

static BOOL IsAncestorSelected(INode *node)
{
	if (node->GetParentNode()->Selected()) 
		return TRUE;
	else return FALSE;
}

int GetSelID(Interface *ip)
{

int		id = -1,selCount = -1;
BOOL	gotLOD=FALSE, gotNonLOD=FALSE; 

	selCount = ip->GetSelNodeCount();
	if (!selCount) 
		return NODESEL_NONE;

	for (int i=0; i<selCount; i++) 
	{	if (IsAncestorSelected(ip->GetSelNode(i))) 
			continue;
		id ++;
	}
	return id;
}

// Dummy function for progress bar
DWORD WINAPI fn(LPVOID arg)
{
	return(0);
}

LRESULT CALLBACK ProgressWinProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{	case WM_INITDIALOG:
			SendMessage(GetDlgItem(hDlg,IDC_PROGRESS_ONE_MESH),PBM_SETRANGE, 0, MAKELPARAM(0, 100)); 
			SendMessage(GetDlgItem(hDlg,IDC_PROGRESS_ALL_MESHS),PBM_SETRANGE, 0, MAKELPARAM(0, 100)); 
		break;
	}
    return DefWindowProc(hDlg, message, wParam, lParam);
}

TriObject* GetTriObjectFromNode(INode *node, TimeValue t, int *deleteIt)
{
	*deleteIt = FALSE;
	Object *obj = node->EvalWorldState(t).obj;
	if (obj->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID, 0))) 
	{	TriObject *tri = (TriObject *) obj->ConvertToType(t,Class_ID(TRIOBJ_CLASS_ID, 0));
		// Note that the TriObject should only be deleted
		// if the pointer to it is not equal to the object
		// pointer that called ConvertToType()
		if(obj != tri) *deleteIt = TRUE;
		return tri;
	}
	else 
		return NULL;
}

BOOL TMNegParity(Matrix3 &m)
{
	return (DotProd(CrossProd(m.GetRow(0),m.GetRow(1)),m.GetRow(2))<0.0)?1:0;
}

int errStep=0;
static VOID ExportOneMesh(Interface *ip,IUtil *iu,int sel_num)
{
Matrix3 mInv; 
float				XFMIN = 3.402823466e+38F, XFMAX = -3.402823466e+38F, 
					YFMIN = 3.402823466e+38F, YFMAX = -3.402823466e+38F, 
					ZFMIN = 3.402823466e+38F, ZFMAX = -3.402823466e+38F; 
#define calcMaxMin	if(XFMIN > v[0]) XFMIN = v[0];\
					if(XFMAX < v[0]) XFMAX = v[0];\
					if(YFMIN > v[1]) YFMIN = v[1];\
					if(YFMAX < v[1]) YFMAX = v[1];\
					if(ZFMIN > v[2]) ZFMIN = v[2];\
					if(ZFMAX < v[2]) ZFMAX = v[2];
BOOL                bClone;

	errStep = 0;

	char *vertSelections = NULL; DWORD nselfaces = 0, nchan = 0;
	if(sMyData.bWriteSelection)
	{	FILE *f = fopen(sMyData.vrtSelFile,"rb");
		if(!f) MessageBox(NULL,"Err.","Vertex selection file is not opened.",MB_OK);
		fread(&nselfaces,4,1,f); fread(&nchan,4,1,f);
		vertSelections = (char*)malloc(nselfaces*3);
		fread(vertSelections,1,nselfaces*3,f); fclose(f);
	}
	node = ip->GetSelNode(sel_num);//node ni olamiz
	nodeName = node->GetName();//node ni nomini olamiz
//********* Agar CLone bo'lsa ***********************************************
	if(strstr(nodeName, "Clone")) bClone = TRUE;
	else bClone = FALSE;
//***************************************************************************
	Matrix3 TM = node->GetNodeTM(ip->GetTime()) * Inverse(node->GetParentTM(ip->GetTime()));
	decomp_affine(TM, &ap);
	BOOL negScale = TMNegParity(node->GetObjTMAfterWSM(ip->GetTime()));//TM edi

    if (negScale) 
	{ vx[0] = 0;vx[1] = 1;vx[2] = 2; }
	else 
	{ vx[0] = 0;vx[1] = 2;vx[2] = 1; }
	ObjectState os = node->EvalWorldState(ip->GetTime());
	if (!os.obj || os.obj->SuperClassID()!=GEOMOBJECT_CLASS_ID) 
	{	MessageBox(NULL,"Katta ildizni ololmayman!","Diqqat",MB_OK);
		return; // Safety net. This shouldn't happen.
	}
//*******  Mesh qilib ko'ramiz **********************************************
	TriObject* tri = GetTriObjectFromNode(node, ip->GetTime(), &needDel);
	if (!tri) 
	{	MessageBox(NULL,"Obyektni editmesh qilolmayman!","Diqqat",MB_OK);
		return;
	}
	mesh = &tri->GetMesh();
	mesh->buildNormals();
//************************** Parametrlari ************************************
	numVerts = mesh->getNumVerts();
	numFaces = (DWORD)(mesh->getNumFaces());
	if(sMyData.bWriteSelection && numFaces!=nselfaces)
	{	MessageBox(NULL,"Err.","Selections faces and mesh faces number mismatch",MB_OK);
		return;
	}

    bFlatten = FindFlatten();//Flatten borligini topamiz;;;;;;;;;;;;;;;;;;
	if(!bClone)
	{if(!mesh->numTVerts)
	{	MessageBox(NULL,"Texturalik malumotlar yo'q!",nodeName,MB_OK);
		return;
	}}
	//if(numFaces>9800)//Predel
	//{	MessageBox(NULL,"Feyslar soni 9800 dan ziyod, bu predel.",nodeName,MB_OK);
	//	return;
	//}
//*********************  Fileni ochamiz **************************************
	if(!openfile(sMyData.szExport, nodeName, bClone ? ".clmsh" : ".msh", &streamBin  ,"wb"))
	{	char fullname[128]; sprintf(fullname, "%s\\%s%s", sMyData.szExport, nodeName, bClone ? ".clmsh" : ".msh");
		MessageBox(NULL, fullname, "Faylini ochishda xato.", MB_OK);
		return;
	}

	//Shapkasi, for future using:
	char sh[20] = {"Msh16.............."};//Ortiqcha tip/mip masalan, yozish kerak b-sa,
	if(sMyData.b32bit && sMyData.bInIndexed) {sh[3] = '3'; sh[4] = '2';}
	else               {sh[3] = '1'; sh[4] = '6';}
	if(bClone)         {sh[5] = 'c'; sh[6] = 'l';}
	fwrite( sh, 1, 20, streamBin );//shu yerda yoziladi;
//***** MATERIAL ****************************************
	errStep = 1;
	myMtrl.Get(node);
	errStep = 2;
	myMtrl.Write(streamBin);
//*******************************************************
	Point3 mpos; float mposx,mposy,mposz;
	mposx = ap.t.x * sMyData.fMassh;
	mposy = ap.t.y * sMyData.fMassh;
	mposz = ap.t.z * sMyData.fMassh;
	//Joyini almashtirib yozsak bo'ladi:
	fwrite( &mposx   , sizeof( mposx )   , 1, streamBin );
	fwrite( &mposz   , sizeof( mposz )   , 1, streamBin );
	fwrite( &mposy   , sizeof( mposy )   , 1, streamBin );
	//*********** endi scaleni yozsak bo'lar,??? *****************************
	if(bClone) 
	{	fwrite( &ap.k.x  , sizeof( ap.k.x ) , 1, streamBin );
		fwrite( &ap.k.z  , sizeof( ap.k.z ) , 1, streamBin );
		fwrite( &ap.k.y  , sizeof( ap.k.y ) , 1, streamBin );
	} else
	{	fwrite( &scPv.x  , sizeof( scPv.x ) , 1, streamBin );
		fwrite( &scPv.z  , sizeof( scPv.z ) , 1, streamBin );
		fwrite( &scPv.y  , sizeof( scPv.y ) , 1, streamBin );
	}
	//*********** endi rotateni yozsak bo'lar,??? *****************************
	//Buni joyini almashtirib yozib bo'lmaydur:

	Point3 fRot = GetFlippedRot(&TM);
	fwrite( &fRot.x, sizeof( fRot.x ), 1, streamBin );
	fwrite( &fRot.y, sizeof( fRot.y ), 1, streamBin );
	fwrite( &fRot.z, sizeof( fRot.z ), 1, streamBin );

	//agar clone bo'lsa:
	if(bClone) goto Tamom;
//***************   FACE lar sonini yozamiz **********************************
	fwrite(&numFaces, sizeof(DWORD), 1, streamBin    );
//******************  1 - hammasini 1 joyga to'playmiz *************************************
	char s[256];
	wsprintf(s,"%s%s",nodeName," malumotlarni yig'ish");
	old_tv1_verts	= new VERTS_T2F_N3F_V3F[numFaces * 3];
	old_faces = new FACES_NORMAL_AND_SMOOTH_GROUPE[numFaces];//Smoothing groupe uchun

	mInv = GetInvTransform(ap.t, ap.q);

	for (DWORD i=0; i<numFaces; i++) 
	{	for(int k=0; k<3; k++)
		{	ind[k] = (WORD)mesh->faces[i].v[vx[k]];
			t_ind[k] = bFlatten ?   (WORD)mesh->mapFaces(flatMapChan)[i].t[vx[k]] :
									(WORD)mesh->tvFace[i].t[vx[k]];
			Point3 V = mesh->verts[ind[k]] * node->GetObjTMAfterWSM(ip->GetTime());
			Point3 v = V * mInv;//Scale ini yo'qotish uchun avval TM ga ko'paytiramiz;
			calcMaxMin
						
			errStep=3;
			Point3 vertexNormal; if(!GetVertexNormal(mesh, i, mesh->getRVertPtr(ind[k]), &vertexNormal)){fclose(streamBin);return;}
			errStep=4;

			Point3 tv; UVVert ftv;
			if(bFlatten)
				ftv = mesh->mapVerts(flatMapChan)[t_ind[k]];
			else
				tv = mesh->tVerts[t_ind[k]];
			
			old_tv1_verts[i * 3 + k].p = v;

			old_tv1_verts[i * 3 + k].n = sMyData.bFlipNormals?(-vertexNormal):vertexNormal;

			if(bFlatten)
			{	if(sMyData.bClampTV)
					ClampTU_TV(ftv);
				old_tv1_verts[i * 3 + k].tv = (float)ftv.x;		
				old_tv1_verts[i * 3 + k].tu = (float)ftv.y;		
			}else
			{	if(sMyData.bClampTV)
					ClampTU_TV(tv);
				old_tv1_verts[i * 3 + k].tv = (float)tv.x;		
				old_tv1_verts[i * 3 + k].tu = (float)tv.y;		
		}	}
		//Feys normali va smoothing groupe i ni ham topib qo'yayluk:
		//old_faces[i].smGr = mesh->faces[i].getSmGroup();//smmothing groupe i
		//Point3 pv0(old_tv1_verts[i*3+1].p - old_tv1_verts[i*3].p);
		//Point3 pv1(old_tv1_verts[i*3+2].p - old_tv1_verts[i*3].p);
		//pv0 = Normalize(pv0); avval ham // edi;
		//pv1 = Normalize(pv1);

		//Feys normali:
		//old_faces[i].n = CrossProd(pv1, pv0);//Soat miliga qarshi yo'n. bo'yicha cross.
		//old_faces[i].n = Normalize(old_faces[i].n);

		//Vertexlarga ham shuni yozib qo'yaylik:
		//old_tv1_verts[i * 3    ].n = old_faces[i].n;
		//old_tv1_verts[i * 3 + 1].n = old_faces[i].n;
		//old_tv1_verts[i * 3 + 2].n = old_faces[i].n;
		Progr(0,60,(float)i/(int)numFaces, "Vertices geometry:", ip); 
	}

	//SmoothNormalsWithWeightingByFaceAngle();

//**************************  Endi yozamiz ****************************************************
	NumNewVert = 3 * numFaces;	

	float mXFMIN,mXFMAX,mYFMIN,mYFMAX,mZFMIN,mZFMAX;
	mXFMIN = XFMIN * sMyData.fMassh;
	mXFMAX = XFMAX * sMyData.fMassh;
	mYFMIN = YFMIN * sMyData.fMassh;
	mYFMAX = YFMAX * sMyData.fMassh;
	mZFMIN = ZFMIN * sMyData.fMassh;
	mZFMAX = ZFMAX * sMyData.fMassh;

	//Y - Z joyi almashadi:
	fwrite(&mXFMIN, sizeof(mXFMIN), 1, streamBin);// float
	fwrite(&mXFMAX, sizeof(mXFMAX), 1, streamBin);// float
	fwrite(&mZFMIN, sizeof(mZFMIN), 1, streamBin);// float
	fwrite(&mZFMAX, sizeof(mZFMAX), 1, streamBin);// float
	fwrite(&mYFMIN, sizeof(mYFMIN), 1, streamBin);// float
	fwrite(&mYFMAX, sizeof(mYFMAX), 1, streamBin);// float
//****************************************************************************
	for(DWORD i=0; i<NumNewVert; i++)//yangi vertexlar **********************************
	{	Point3 mpos;
		mpos = old_tv1_verts[i].p * sMyData.fMassh;

		if(sMyData.bWriteSelection)
		{	switch(vertSelections[i])
			{	case 0:
					SetFloatBit(&mpos.x,0,FALSE);
					SetFloatBit(&mpos.y,0,FALSE);
					SetFloatBit(&mpos.z,0,FALSE);//SetFloatBit(&old_tv1_verts[i].n.x,0,FALSE);SetFloatBit(&old_tv1_verts[i].n.y,0,FALSE);SetFloatBit(&old_tv1_verts[i].n.z,0,FALSE);
				break;
				case 1:
					SetFloatBit(&mpos.x,0,TRUE);
					SetFloatBit(&mpos.y,0,FALSE);
					SetFloatBit(&mpos.z,0,FALSE);//SetFloatBit(&old_tv1_verts[i].n.x,0,FALSE);SetFloatBit(&old_tv1_verts[i].n.y,0,FALSE);SetFloatBit(&old_tv1_verts[i].n.z,0,FALSE);
				break;
				case 2:
					SetFloatBit(&mpos.x,0,FALSE);
					SetFloatBit(&mpos.y,0,TRUE);
					SetFloatBit(&mpos.z,0,FALSE);//SetFloatBit(&old_tv1_verts[i].n.x,0,FALSE);SetFloatBit(&old_tv1_verts[i].n.y,0,FALSE);SetFloatBit(&old_tv1_verts[i].n.z,0,FALSE);
				break;
				case 3:
					SetFloatBit(&mpos.x,0,TRUE);
					SetFloatBit(&mpos.y,0,TRUE);
					SetFloatBit(&mpos.z,0,FALSE);//SetFloatBit(&old_tv1_verts[i].n.x,0,FALSE);SetFloatBit(&old_tv1_verts[i].n.y,0,FALSE);SetFloatBit(&old_tv1_verts[i].n.z,0,FALSE);
				break;
				case 4:
					SetFloatBit(&mpos.x,0,FALSE);
					SetFloatBit(&mpos.y,0,FALSE);
					SetFloatBit(&mpos.z,0,TRUE);//SetFloatBit(&old_tv1_verts[i].n.x,0,FALSE);SetFloatBit(&old_tv1_verts[i].n.y,0,FALSE);SetFloatBit(&old_tv1_verts[i].n.z,0,FALSE);
				break;
				case 5:
					SetFloatBit(&mpos.x,0,TRUE);
					SetFloatBit(&mpos.y,0,FALSE);
					SetFloatBit(&mpos.z,0,TRUE);//SetFloatBit(&old_tv1_verts[i].n.x,0,FALSE);SetFloatBit(&old_tv1_verts[i].n.y,0,FALSE);SetFloatBit(&old_tv1_verts[i].n.z,0,FALSE);
				break;
				case 6:
					SetFloatBit(&mpos.x,0,FALSE);
					SetFloatBit(&mpos.y,0,TRUE);
					SetFloatBit(&mpos.z,0,TRUE);//SetFloatBit(&old_tv1_verts[i].n.x,0,FALSE);SetFloatBit(&old_tv1_verts[i].n.y,0,FALSE);SetFloatBit(&old_tv1_verts[i].n.z,0,FALSE);
				break;
				case 7:
					SetFloatBit(&mpos.x,0,TRUE);
					SetFloatBit(&mpos.y,0,TRUE);
					SetFloatBit(&mpos.z,0,TRUE);//SetFloatBit(&old_tv1_verts[i].n.x,0,FALSE);SetFloatBit(&old_tv1_verts[i].n.y,0,FALSE);SetFloatBit(&old_tv1_verts[i].n.z,0,FALSE);
				break;
		}	}
		fwrite( &mpos.x, sizeof( mpos.x ), 1, streamBin );
		fwrite( &mpos.z, sizeof( mpos.z ), 1, streamBin );
		fwrite( &mpos.y, sizeof( mpos.y ), 1, streamBin );
		//************
		//************
		//************
		fwrite( &old_tv1_verts[i].n.x, sizeof( old_tv1_verts[i].n.x ), 1, streamBin );
		fwrite( &old_tv1_verts[i].n.z, sizeof( old_tv1_verts[i].n.z ), 1, streamBin );
		fwrite( &old_tv1_verts[i].n.y, sizeof( old_tv1_verts[i].n.y ), 1, streamBin );
		//************
		//************
		//************
		//DirectXga OpenGL dan farqini kiritamiz:
		old_tv1_verts[i].tu = 1.f - old_tv1_verts[i].tu;
		fwrite( &old_tv1_verts[i].tv, sizeof( old_tv1_verts[i].tv ), 1, streamBin );
		fwrite( &old_tv1_verts[i].tu, sizeof( old_tv1_verts[i].tu ), 1, streamBin );
		Progr(60, 100, (float)i/(int)NumNewVert,"Writing vertice geometry:", ip); 
	}
//	Vertexlarni indexlash bo'yicha ozgina o'zgartirishlar kiritamiz, 22.04.08 dan boshlab.
//  DrawIndexedPrimitive dagi indexlash ketmaydi. Sababi, collisionda bu narsa ketmag'on,
//  Chunkim, bunda tu,tv lari 1 xil bo'lgan vertexlargina indexlanar edi. Lekin tu-tv si
//  1 xil bo'lmagan, ammo v si 1 xil bo'lgan vertexlar ham borku. Bizni esa mana shunday
//  1 xil bo'lgan vertexlar qiziqtiradi, collision uchun. Shuning uchun ham xotira masa-
//  lasida yutqazsakda, ushbu o'zgartirishni qilishga majburmiz.
/*	DWORD VertIndNum;
	VertIndNum = IndexVertexes();

	//if(VertIndNum > 65535)//Predel, endi buning ahamiyati yo'q;
	//{	MessageBox(NULL,"Vertexlar soni 65535 dan ko\'p, bu WORD indexlar uchun predel.",nodeName,MB_OK);
	//	return;
	//}

	fwrite(&VertIndNum, sizeof(DWORD), 1, streamBin);

	for(DWORD i=0; i<VertIndNum; i++)//yangi vertexlar **********************************
		fwrite(&indexes[i], 2, 1, streamBin);//O'zi DWORD

	//Hozirchalik bu massivni yozishimizdan ma'no yo'q.Agarda muhtojligi sezilsa, yozamiz.
	//for(i=0; i<NumNewVert; i++)
	//	fwrite( &ptrToInds[i], sizeof(ptrToInds[i]), 1, streamBin );
	//}

	DWORD edges;
	edges = IndexEdges();
	//if(edges > 65535)//Predel. Endi buning ahamiyati yo'q;
	//{	MessageBox(NULL,"Edgelar soni 65535 dan ko\'p, bu WORD edgelar uchun predel.",nodeName,MB_OK);
	//	return;
	//}

	fwrite(&edges, sizeof(DWORD), 1, streamBin);

	for(DWORD i=0; i<edges; i++)
	{	fwrite(&edgeTypesArray[i], sizeof(WORD), 1, streamBin);//O'zi DWORD

		WORD *arr = (WORD*)&indexedEdgeArray[i];
		fwrite(arr  , sizeof(WORD), 1, streamBin);
		fwrite(arr+1, sizeof(WORD), 1, streamBin);
		fwrite(arr+2, sizeof(WORD), 1, streamBin);
		fwrite(arr+3, sizeof(WORD), 1, streamBin);
	}

	//Physics uchun alohida chiqaramiz, iloj yo'q,Physicsni ishlatish kerakka o'xshaydi.
	fwrite(&numVerts, sizeof(DWORD), 1, streamBin);
	if((!sMyData.b32bit) && numVerts > 65535)//Predel
	{	MessageBox(NULL,"Phys. vertexlar soni 65535 dan ko\'p, bu WORD indexlar uchun predel.",nodeName,MB_OK);
		return;
	}
	for(DWORD i=0; i<numFaces; i++)
	{	for(int k=0; k<3; k++)
		{	ind[k] = (WORD)mesh->faces[i].v[vx[k]];
			fwrite(&ind[k], sMyData.b32bit?4:2, 1, streamBin);
	}	}
	for(DWORD i=0; i<numVerts; i++)
	{	Point3 V = mesh->verts[i] * node->GetObjTMAfterWSM(ip->GetTime());
		Point3 v = V * mInv;//Scale ini yo'qotish uchun avval TM ga ko'paytiramiz;

		v *= sMyData.fMassh;
		fwrite(&v.x, sizeof(float), 1, streamBin);
		fwrite(&v.z, sizeof(float), 1, streamBin);
		fwrite(&v.y, sizeof(float), 1, streamBin);
	}*/

//********************  TAMOM  ****************************
Tamom:
	fclose(streamBin);
	if(!bClone)
	{	delete [] old_tv1_verts;
		delete [] old_faces;
		delete [] indexedVertArray;
		delete [] ptrToInds;
		delete [] indexes;
		delete [] indexedEdgeArray;
		delete [] edgeTypesArray;
	}
	if(needDel) tri->DeleteMe();
	char fullname[128]; sprintf(fullname, "%s\\%s%s", sMyData.szExport, nodeName, bClone ? ".clmsh" : ".msh");
	if(!bClone)
	{if(sMyData.bInIndexed)
	 {	if(-3==ConvertMeshToIndexedFormat(fullname, sMyData.bWriteSelection?0.0f:sMyData.fThreshold))
			MessageBox(NULL, "Error. 32bit inds. requered.", "Please set 32 bit check!", MB_OK);
		if(sMyData.bDeleteSource)
			DeleteFile(fullname);
		sprintf(fullname, "%s\\%s%s", sMyData.szExport, nodeName, ".msi");
		if(sMyData.bTangent)
		{	ConvertMeshToTangentAndBinormalToF16_4_UB4N_F16_2FormatTriangleStripDX(fullname);
			/*ConvertIndexedMeshToMeshTangentAndBinormalUseDX(fullname);
			if(sMyData.bDeleteSource)
				DeleteFile(fullname);
			sprintf(fullname, "%s\\%s%s", sMyData.szExport, nodeName, ".mst");
			ConvertMeshTangentAndBinormalToF16_4_UB4N_F16_2FormatTriangleStrip(fullname);*/
			//fullname[lstrlen(fullname)-1]='u';
			//RenameFile(fullname, lstrlen(fullname)-1, 't');
			if(sMyData.bDeleteSource)
				DeleteFile(fullname);
			fullname[lstrlen(fullname)-1]='t';
			if(sMyData.bToText)
				IndexedMeshTangencedBinormalToText(fullname);
		}
		else
		{	if(!sMyData.b32bit)
			{ConvertIndexedMeshF16_4_UB4N_F16_2FormatTriangleStripDX(fullname);//ConvertIndexedMeshF16_4_UB4N_F16_2FormatTriangleStrip
			 fullname[lstrlen(fullname)-1]='u';
			 RenameFile(fullname, lstrlen(fullname)-1, 'i');
			 fullname[lstrlen(fullname)-1]='i';
			}
			if(sMyData.bToText)
				IndexedMeshToText(fullname);
	}	}
	else if(sMyData.bToText) MeshToText(fullname);
	}

	if(sMyData.bWriteSelection) free(vertSelections);
	return;
#undef calcMaxMin
}

static VOID ExportOneMeshTextCoord(Interface *ip,IUtil *iu,int sel_num)
{
	node = ip->GetSelNode(sel_num);//node ni olamiz
	nodeName = node->GetName();//node ni nomini olamiz
//***************************************************************************
    Matrix3 TM = node->GetNodeTM(ip->GetTime()) * Inverse(node->GetParentTM(ip->GetTime()));
	decomp_affine(TM, &ap);
	BOOL negScale = TMNegParity(node->GetObjTMAfterWSM(ip->GetTime()));

    if (negScale) 
	{	vx[0] = 0;vx[1] = 1;vx[2] = 2;	}
	else 
	{	vx[0] = 0;vx[1] = 2;vx[2] = 1;	}
	ObjectState os = node->EvalWorldState(ip->GetTime());
	if (!os.obj || os.obj->SuperClassID()!=GEOMOBJECT_CLASS_ID) 
	{	MessageBox(NULL,"Katta ildizni ololmayman!","Diqqat",MB_OK);
		return; // Safety net. This shouldn't happen.
	}
//*******  Mesh qilib ko'ramiz **********************************************
	TriObject* tri = GetTriObjectFromNode(node, ip->GetTime(), &needDel);
	if (!tri) 
	{	MessageBox(NULL,"Obyektni editmesh qilolmayman!","Diqqat",MB_OK);
		return;
	}
	mesh = &tri->GetMesh();
	mesh->buildNormals();
//************************** Parametrlari ************************************
	numVerts = mesh->getNumVerts();
	numFaces = (DWORD)(mesh->getNumFaces());
    bFlatten = FindFlatten();//Flatten borligini topamiz;;;;;;;;;;;;;;;;;;
	if(!mesh->numTVerts)
	{	MessageBox(NULL,"Texturalik malumotlar yo'q!",nodeName,MB_OK);
		return;
	}
//*********************  Fileni ochamiz **************************************
	if(!openfile(sMyData.szExport, nodeName, ".crd", &streamBin  ,"wb"))
	{	char fullname[128]; sprintf(fullname, "%s\\%s%s", sMyData.szExport, nodeName, ".crd");
		MessageBox(NULL, fullname, "Faylini ochishda xato.", MB_OK);
		return;
	}
//******************  1 - hammasini 1 joyga to'playmiz *************************************
	char s[256];
	wsprintf(s,"%s%s",nodeName," malumotlarni yig'ish");
	old_tv1_verts	= new VERTS_T2F_N3F_V3F[numFaces * 3];

	for (DWORD i=0; i<numFaces; i++) 
	{	for (int k=0; k<3; k++) 
		{	ind[k] = (WORD)mesh->faces[i].v[vx[k]];
			t_ind[k] = bFlatten ?	(WORD)mesh->mapFaces(flatMapChan)[i].t[vx[k]] :
									(WORD)mesh->tvFace[i].t[vx[k]];
			errStep=3;
			Point3 v;
			v = /*TM   **/ mesh->verts[ind[k]];
			Point3 vertexNormal; if(!GetVertexNormal(mesh, i, mesh->getRVertPtr(ind[k]), &vertexNormal)){fclose(streamBin);return;}
			errStep=4;

			Point3 tv;UVVert ftv;
			if(bFlatten)
				ftv = mesh->mapVerts(flatMapChan)[t_ind[k]];
			else
				tv = mesh->tVerts[t_ind[k]];

			old_tv1_verts[i * 3 + k].p = v;

			old_tv1_verts[i * 3].n = vertexNormal;

			if(bFlatten)
			{	old_tv1_verts[i * 3 + k].tv = (float)ftv.x;		
				old_tv1_verts[i * 3 + k].tu = (float)ftv.y;		
			}else
			{	old_tv1_verts[i * 3 + k].tv = (float)tv.x;		
				old_tv1_verts[i * 3 + k].tu = (float)tv.y;		
		}	}
		Progr(0,20,(float)i/(int)numFaces,"TU-TV:", ip); 
	}
//********* Index uchun sortirovkalash *******************************************************
    // Taxminan massivi shuncha beraman;
	NumNewVert = 3 * numFaces;	

	fwrite(&NumNewVert, sizeof(DWORD), 1, streamBin    );// O'zi DWORD
//****************************************************************************
	for(DWORD i=0;i<NumNewVert;i++)//yangi vertexlar **********************************
	{	//DirectXga OpenGL dan farqini kiritamiz:
		old_tv1_verts[i].tu = 1.f - old_tv1_verts[i].tu;
		fwrite( &old_tv1_verts[i].tv, sizeof( old_tv1_verts[i].tv ), 1, streamBin );
		fwrite( &old_tv1_verts[i].tu, sizeof( old_tv1_verts[i].tu ), 1, streamBin );
		Progr(40,100,(float)i/(int)NumNewVert,"Writing TU-TV:", ip); 
	}
//********************  TAMOM  ****************************
fclose(streamBin); 
	delete [] old_tv1_verts;
	if(needDel) tri->DeleteMe();

	char fullname[128]; sprintf(fullname, "%s\\%s%s", sMyData.szExport, nodeName, ".crd");
	if(sMyData.bInIndexed)
	{	if(-2 == TextureCoordsToIndexed(fullname))
			MessageBox(NULL,"To cnvrt. ind. need mesh file(*.msh)","Err.",MB_OK);
	}
	if(sMyData.bToText) 
		TextureCoordsToText(fullname);

	return;
}

VOID AllMesh(LPVOID par)
{
int		    k,i;
ThrPar		*thrPar;
Interface	*ip;
IUtil		*iu;
int			type;

	thrPar  = (ThrPar*)par;
	ip		= thrPar->ip;
	iu		= thrPar->iu;
	type	= thrPar->type;

	
	i = GetSelID(ip);


	if( (i < 0) || (i > 65000) ) // select qilingani umuman bo'lmasa
	{	MessageBox(NULL,"1nechasini exp (belgilang)-- ni belgilamang","1 nechtasini belgilabsiz!",MB_OK);
		return;
	}


	for(k=0;k<=i;k++)
	{	switch(type)
		{	case 0:
			break;
			case 1:
			break;
			case 2:
			break;
			case 3:
			break;
			case 4:
			break;
			case 5:
				__try
				{
					streamBin = 0;
					ExportOneMesh(ip, iu, k);
					streamBin = 0;
				}
				__except(1,1)
				{
					switch(errStep)
					{	case 0:
							MessageBox(NULL, "Kutilmagan gen. xato", "Materialgacha !!!", MB_OK);
						break;
						case 1:
							MessageBox(NULL, "Kutilmagan gen. xato", "Material !!!", MB_OK);
						break;
						case 3:
							MessageBox(NULL, "Kutilmagan gen. xato", "'GetNormal'da,AuthoSmooth qiling.", MB_OK);
						break;
						default:
							MessageBox(NULL, "Kutilmagan gen. xato", "Diqqat !!!", MB_OK);
						break;
					}
					if(streamBin) fclose(streamBin);
				}
			break;
			default:
				;
	}	}
}

VOID AllMeshTextCoord(LPVOID par)
{
int		k,i;
ThrPar		*thrPar;
Interface	*ip;
IUtil		*iu;
int			type;

	thrPar  = (ThrPar*)par;
	ip		= thrPar->ip;
	iu		= thrPar->iu;
	type	= thrPar->type;

	
	i = GetSelID(ip);

	if(i < 0) // select qilingani umuman bo'lmasa
	{	MessageBox(NULL,"Hech qanaqa obyekt topolmadim!","Diqqat",MB_OK);
		return;
	}

	if( (i < 0) || (i > 65000) ) // select qilingani umuman bo'lmasa
	{	MessageBox(NULL,"1nechasini exp (belgilang)-- ni belgilamang","1 nechtasini belgilabsiz!",MB_OK);
		return;
	}


	for(k=0;k<=i;k++)
	{	switch(type)
		{	case 0:
			break;
			case 1:
			break;
			case 2:
			break;
			case 3:
			break;
			case 4:
			break;
			case 5:
				__try
				{
					streamBin = 0;
					ExportOneMeshTextCoord(ip, iu, k);
					streamBin = 0;
				}
				__except(1,1)
				{
					switch(errStep)
					{	case 0:
							MessageBox(NULL, "Kutilmagan gen. xato", "Materialgacha !!!", MB_OK);
						break;
						case 1:
							MessageBox(NULL, "Kutilmagan gen. xato", "Material !!!", MB_OK);
						break;
						case 3:
							MessageBox(NULL, "Kutilmagan gen. xato", "'GetNormal'da,AuthoSmooth qiling.", MB_OK);
						break;
						default:
							MessageBox(NULL, "Kutilmagan gen. xato", "Diqqat !!!", MB_OK);
						break;
					}
					if(streamBin) fclose(streamBin);
				}
			break;
			default:
				;
	}	}
	return;
}