#include "ExpDX.h"
#include "modstack.h"
#include "iunwrap.h"
#include "animtbl.h"
#include "decomp.h"
#include "iskin.h"
#include "../../../Libs/Math/mMathFn.h"
#include "C:\Program Files (x86)/Autodesk/3ds Max 2010 SDK/maxsdk/samples/controllers/reactor/reactapi.h"


float DeltaMin = 0.01f;

_VERTS_T2F_N3F_V3F *old_tv1_verts_sk2;

//Eski table b-n ishlaydi, yangi table ni aralashtirmaslik kerak;
BOOL FoundParentToUpHierarch(WORD wWhichBone, char*cmpBoneName)
{
WORD tot = 0;
INode *p = skin->GetBone(wWhichBone)->GetParentNode();
	while(p)
	{	char *pN = p->GetName();
		if(strstr(pN, cmpBoneName))	
			return TRUE;
		if(tot++ > wNumBones) return FALSE;
		p = p->GetParentNode();
	}
	return FALSE;
}

float GetVertAngleSkn2(int f, int v)//f-face, v-vert
{
Point3 v0, v1;
	
	if(v==0)
	{	v0 = old_tv1_verts_sk2[f*3+1].p - old_tv1_verts_sk2[f*3  ].p;
		v1 = old_tv1_verts_sk2[f*3+2].p - old_tv1_verts_sk2[f*3  ].p;
	}
	else if(v==1)
	{	v0 = old_tv1_verts_sk2[f*3  ].p - old_tv1_verts_sk2[f*3+1].p;
		v1 = old_tv1_verts_sk2[f*3+2].p - old_tv1_verts_sk2[f*3+1].p;
	}
	else//if(v==2)
	{	v0 = old_tv1_verts_sk2[f*3  ].p - old_tv1_verts_sk2[f*3+2].p;
		v1 = old_tv1_verts_sk2[f*3+1].p - old_tv1_verts_sk2[f*3+2].p;
	}

	v0 = Normalize(v0);
	v1 = Normalize(v1);

	return acosf(DotProd(v0, v1));
}

VOID SmoothNormalsWithWeightingByFaceAngleSkn2(Interface *ip)
{
//** Bir xil smoothing groupe lardagi normallarni to'g'rilab chiqaylukchi, akesh !!!! *********
	int equVerts[3][500][2],//1 xil vertlar topilsa, shularni indekslash uchun; 
		equPtr[3];          //Baribir maximum 1000 dan ortiq bo'lmasa kerak; 
	for (DWORD i=0; i<numFaces; i++) 
	{	equPtr[0] = equPtr[1] = equPtr[2] = 0;
		for(DWORD ii=i+1; ii<numFaces; ii++)
		{	if(old_faces[i].smGr == old_faces[ii].smGr)
			{	for(int fv=0; fv<3; fv++)//Birlamchi uchta vertex uchun:
				{	for(int vv=0; vv<3; vv++)//Ikkilamchi uchta vertex uchun:
					{	Point3 delta = old_tv1_verts_sk2[i*3+fv].p - old_tv1_verts_sk2[ii*3+vv].p;
						if(fabs(delta.Length()) < 0.01f)
						{	equVerts[fv][equPtr[fv]][0] = ii;//feys numuri;
							equVerts[fv][equPtr[fv]][1] = vv ;//shu feys verti numuri;
							if(++equPtr[fv] > 1000)
							{	MessageBox(NULL, nodeName, "To\'xtatdim. 1 xil posli va 1 xil smoothing gr.li vert.lar soni 1000 dan ko'p: .", MB_OK);
								return;
		}	}	}	}	}	}

#define V_nrm old_tv1_verts_sk2[ equVerts[fv][av][0]*3 + equVerts[fv][av][1] ].n

		//BITTA FEYSGA MOS 1 XIL VERTLARNI TO'DALADUK, equVerts LARGA, equPtr TADAN;
		Point3 averNorm;
		for(int fv=0; fv<3; fv++)//Birlamchi uchta vertex uchun:
		{	if(equPtr[fv] > 0)//Hech bo'lmasa 1 tadan ko'p 1 xil vertlar bor ekan, 1-vertexga:
			{	//avval o'ziniki:
				averNorm = GetVertAngleSkn2(i, fv) * old_tv1_verts_sk2[i*3+fv].n;
				for(int av=0; av<equPtr[fv]; av++)
					averNorm += GetVertAngleSkn2(equVerts[fv][av][0], equVerts[fv][av][1]) * V_nrm;
				averNorm = Normalize(averNorm);//Normalladuk;       
				//endi hammasiga yozavuz:
				//avval o'ziga:
				old_tv1_verts_sk2[i*3+fv].n = averNorm;
				//endi qoganiga:
				for(int av=0; av<equPtr[fv]; av++)
					V_nrm = averNorm;
	
		}	}
		Progr(0,100,(float)i/(int)numFaces,"smoothing normals ...", ip);
#undef V_nrm
	}

	return;
}

VOID SmoothNormalsWithWeightingByFaceAngle2Skn2()
{
	Interface *ip = GetCOREInterface();
//** Bir xil smoothing groupe lardagi normallarni to'g'rilab chiqaylukchi, akesh !!!! *********
	int equVerts[3][500][2],//1 xil vertlar topilsa, shularni indexlash uchun; 
		equPtr[3];           //Baribir maximum 1000 dan ortiq bo'lmasa kerak; 
	for (DWORD i=0; i<numFaces; i++) 
	{ if( fabs(old_tv2_verts[i*3  ].p.x - old_tv1_verts_sk2[i*3  ].p.x) > DeltaMin ||
		  fabs(old_tv2_verts[i*3  ].p.y - old_tv1_verts_sk2[i*3  ].p.y) > DeltaMin ||
		  fabs(old_tv2_verts[i*3  ].p.z - old_tv1_verts_sk2[i*3  ].p.z) > DeltaMin ||

		  fabs(old_tv2_verts[i*3+1].p.x - old_tv1_verts_sk2[i*3+1].p.x) > DeltaMin ||
		  fabs(old_tv2_verts[i*3+1].p.y - old_tv1_verts_sk2[i*3+1].p.y) > DeltaMin ||
		  fabs(old_tv2_verts[i*3+1].p.z - old_tv1_verts_sk2[i*3+1].p.z) > DeltaMin ||
			
		  fabs(old_tv2_verts[i*3+2].p.x - old_tv1_verts_sk2[i*3+2].p.x) > DeltaMin ||
		  fabs(old_tv2_verts[i*3+2].p.y - old_tv1_verts_sk2[i*3+2].p.y) > DeltaMin ||
		  fabs(old_tv2_verts[i*3+2].p.z - old_tv1_verts_sk2[i*3+2].p.z) > DeltaMin  )
	  {	equPtr[0] = equPtr[1] = equPtr[2] = 0;
		for(DWORD ii=i+1; ii<numFaces; ii++)
		{	if(old_faces[i].smGr == old_faces[ii].smGr)
			{	for(int fv=0; fv<3; fv++)//Birlamchi uchta vertex uchun:
				{	for(int vv=0; vv<3; vv++)//Ikkilamchi uchta vertex uchun:
					{	Point3 delta = old_tv2_verts[i*3+fv].p - old_tv2_verts[ii*3+vv].p;
						if(fabs(delta.Length()) < 0.01f)
						{	equVerts[fv][equPtr[fv]][0] = ii;//feys numuri;
							equVerts[fv][equPtr[fv]][1] = vv ;//shu feys verti numuri;
							if(++equPtr[fv] > 1000)
							{	MessageBox(NULL, nodeName, "To\'xtatdim. 1 xil posli va 1 xil smoothing gr.li vert.lar soni 1000 dan ko'p: .", MB_OK);
								return;
		}	}	}	}	}	}

#define V_nrm old_tv2_verts[ equVerts[fv][av][0]*3 + equVerts[fv][av][1] ].n

		//BITTA FEYSGA MOS 1 XIL VERTLARNI TO'DALADUK, equVerts LARGA, equPtr TADAN;
		Point3 averNorm;
		for(int fv=0; fv<3; fv++)//Birlamchi uchta vertex uchun:
		{	if(equPtr[fv] > 0)//Hech bo'lmasa 1 tadan ko'p 1 xil vertlar bor ekan, 1-vertexga:
			{	//avval o'ziniki:
				averNorm = GetVertAngleSkn2(i, fv) * old_tv2_verts[i*3+fv].n;
				for(int av=0; av<equPtr[fv]; av++)
					averNorm += GetVertAngleSkn2(equVerts[fv][av][0], equVerts[fv][av][1]) * V_nrm;
				averNorm = Normalize(averNorm);//Normalladuk;       
				//endi hammasiga yozavuz:
				//avval o'ziga:
				old_tv2_verts[i*3+fv].n = averNorm;
				//endi qoganiga:
				for(int av=0; av<equPtr[fv]; av++)
					V_nrm = averNorm;
	 }	}	}
	else//Agar morph ishlatilmagan bo'lsa:
	{	old_tv2_verts[i*3  ].n = old_tv1_verts_sk2[i*3  ].n;
		old_tv2_verts[i*3+1].n = old_tv1_verts_sk2[i*3+1].n;
		old_tv2_verts[i*3+2].n = old_tv1_verts_sk2[i*3+2].n;
	}
#undef V_nrm
		Progr(0,60,(float)i/(int)numFaces, "Smoothing normals for morpher one channel ...", ip);
	}

	return;
}

static VOID WriteChangedMorphVertsSkn2()
{
DWORD   i, ii, mrpFaces = 0;
float	TM[6];
fpos_t  sumMrpFaces;

	fgetpos(streamBin, &sumMrpFaces);
	fwrite(&mrpFaces, sizeof(mrpFaces), 1, streamBin);

	for(i=0; i<numFaces; i++)
	{	if( fabs(old_tv2_verts[i*3  ].p.x - old_tv1_verts_sk2[i*3  ].p.x) > DeltaMin ||
			fabs(old_tv2_verts[i*3  ].p.y - old_tv1_verts_sk2[i*3  ].p.y) > DeltaMin ||
			fabs(old_tv2_verts[i*3  ].p.z - old_tv1_verts_sk2[i*3  ].p.z) > DeltaMin ||

			fabs(old_tv2_verts[i*3+1].p.x - old_tv1_verts_sk2[i*3+1].p.x) > DeltaMin ||
			fabs(old_tv2_verts[i*3+1].p.y - old_tv1_verts_sk2[i*3+1].p.y) > DeltaMin ||
			fabs(old_tv2_verts[i*3+1].p.z - old_tv1_verts_sk2[i*3+1].p.z) > DeltaMin ||
			
			fabs(old_tv2_verts[i*3+2].p.x - old_tv1_verts_sk2[i*3+2].p.x) > DeltaMin ||
			fabs(old_tv2_verts[i*3+2].p.y - old_tv1_verts_sk2[i*3+2].p.y) > DeltaMin ||
			fabs(old_tv2_verts[i*3+2].p.z - old_tv1_verts_sk2[i*3+2].p.z) > DeltaMin  )
		{	fwrite(&i, sMyData.b32bit?4:2, 1, streamBin);//Face num;
			for(int v=0; v<3; v++)
			{	ii = i*3+v;
				TM[0] = old_tv2_verts[ii].p.x  - old_tv1_verts_sk2[ii].p.x;
				TM[1] = old_tv2_verts[ii].p.z  - old_tv1_verts_sk2[ii].p.z;
				TM[2] = old_tv2_verts[ii].p.y  - old_tv1_verts_sk2[ii].p.y;
				TM[3] = old_tv2_verts[ii].n.x  - old_tv1_verts_sk2[ii].n.x;
				TM[4] = old_tv2_verts[ii].n.z  - old_tv1_verts_sk2[ii].n.z;
				TM[5] = old_tv2_verts[ii].n.y  - old_tv1_verts_sk2[ii].n.y;			
				TM[0] *= sMyData.fMassh;
				TM[1] *= sMyData.fMassh;
				TM[2] *= sMyData.fMassh;
				fwrite(TM, sizeof(float), 6, streamBin);	
			}	
			mrpFaces++;
	}	}
	fseek(streamBin, (long)sumMrpFaces, SEEK_SET);
	fwrite(&mrpFaces, 4, 1, streamBin);
	fseek(streamBin, 0, SEEK_END);
	return;
}

static VOID CalcNewMorphVertsSkn2(Interface *ip)
{
	for (DWORD i=0; i<numFaces; i++) 
	{	for (int k=0; k<3; k++) 
		{	ind[k] = (WORD)mesh->faces[i].v[vx[k]];
			t_ind[k] = bFlatten ?	(WORD)mesh->mapFaces(flatMapChan)[i].t[vx[k]] :
									(WORD)mesh->tvFace[i].t[vx[k]];
			Point3 v;
			v = mesh->verts[ind[k]];
		
			errStep=3;
			Point3 vertexNormal; if(!GetVertexNormal(mesh, i, mesh->getRVertPtr(ind[k]), &vertexNormal))
			{	MessageBox(NULL, "Set edit mesh for normal calculating.", "Err.", MB_OK);
				return;
			}
			errStep=4;

			Point3 tv;UVVert ftv;
			if(bFlatten)
				ftv = mesh->mapVerts(flatMapChan)[t_ind[k]];
			else
				tv = mesh->tVerts[t_ind[k]];

			old_tv2_verts[i * 3 + k].p = v;

			old_tv2_verts[i * 3 + k].n = vertexNormal;

			if(bFlatten)
			{	old_tv2_verts[i * 3 + k].tv = (float)ftv.x;		
				old_tv2_verts[i * 3 + k].tu = (float)ftv.y;		
			}else
			{	old_tv2_verts[i * 3 + k].tv = (float)tv.x;		
				old_tv2_verts[i * 3 + k].tu = (float)tv.y;		
		}	}
		//Feys normali va smoothing groupe i ni ham topib qo'yayluk:
		//old_faces[i].smGr = mesh->faces[i].getSmGroup();//smmothing groupe i
		//Point3 pv0(old_tv2_verts[i*3+1].p - old_tv2_verts[i*3].p);
		//Point3 pv1(old_tv2_verts[i*3+2].p - old_tv2_verts[i*3].p);
		//pv0 = Normalize(pv0);
		//pv1 = Normalize(pv1);

		//Feys normali:
		//old_faces[i].n = CrossProd(pv1, pv0);//Soat miliga qarshi yo'n. bo'yicha cross.
		//old_faces[i].n = Normalize(old_faces[i].n);

		//Vertexlarga ham shuni yozib qo'ydik:
		//old_tv2_verts[i * 3    ].n = old_faces[i].n;
		//old_tv2_verts[i * 3 + 1].n = old_faces[i].n;
		//old_tv2_verts[i * 3 + 2].n = old_faces[i].n;

		Progr(80, 100, (float)i/(int)numFaces,"CalcNewMorphVertsSkn2:", ip);
	}
	//SmoothNormalsWithWeightingByFaceAngle2Skn2();
	return;
}

VOID ExportOneSkin2Morph(Interface *ip,IUtil *iu,int sel_num)
{
TSTR		indent;				TriObject	*tri;				
ObjectState	os;					BOOL		negScale;
Point3		rots,scs;			Matrix3		TM, mMeshTM;
BYTE        maxAssignedBone;	int			k, baseBone, iFootStepsBonNum;
BOOL        bClone, bFootSteps; D3DXMATRIX  r;

float				XFMIN = 3.402823466e+38F, XFMAX = -3.402823466e+38F, 
					YFMIN = 3.402823466e+38F, YFMAX = -3.402823466e+38F, 
					ZFMIN = 3.402823466e+38F, ZFMAX = -3.402823466e+38F; 
#define calcMaxMin	if(XFMIN > v[0]) XFMIN = v[0];\
					if(XFMAX < v[0]) XFMAX = v[0];\
					if(YFMIN > v[1]) YFMIN = v[1];\
					if(YFMAX < v[1]) YFMAX = v[1];\
					if(ZFMIN > v[2]) ZFMIN = v[2];\
					if(ZFMAX < v[2]) ZFMAX = v[2];

	old_tv1_verts_sk2 = 0; VerOldTabl  = 0; pBonNodes = 0;
	pBonesParentTable = 0; sk_verts = 0; keyArrays = 0;

	node = ip->GetSelNode(sel_num);//node ni olamiz
	nodeName = node->GetName();//node ni nomini olamiz

//********* Agar CLone bo'lsa ***********************************************
	if(strstr(nodeName, "Clone"))bClone = TRUE;
	else bClone = FALSE;
//******* Morpher modifierni save qilib qo'yayluk: **************************
	morpher = 0;
	if(!GetMorpherModifier(node))
		return;
	if(!morpher)
	{	MessageBox(NULL,"Morpherni topolmadim.","Kechirasiz, chiqib ketaman!!!",MB_OK);
		return;
	}
	SaveAllChannels(ip);
	ZeroAllChannels(ip);
//******* Birinchi NumAnimKeys(pos , rot , scal) ni topamiz *****************
	skin = 0;
	GetSkinModifier(node);
	if(!skin)
	{	MessageBox(NULL,"Skinni topolmadim.","Kechirasiz, chiqib ketaman!!!",MB_OK);
		return;
	}
	numBones = skin->GetNumBones();
	if(!numBones)
	{	MessageBox(NULL,"Boneslarni topolmadim.","Kechirasiz, chiqib ketaman!!!",MB_OK);
		return;
	}
	baseBone = BuildBonesHierarchy(numBones, &bFootSteps, &iFootStepsBonNum);
	if(baseBone == -1)
	{	MessageBox(NULL,"Boneslarni iyerarxisini qurolmadim.","Kechirasiz, chiqib ketaman!!!",MB_OK);
		return;
	}
	SkinModif->DisableMod();
//***************************************************************************
	boneName = skin->GetBone(baseBone)->GetName();
	INode *bpNd = skin->GetBone(baseBone);
    TM = node->GetNodeTM(ip->GetTime()) * Inverse(node->GetParentTM(ip->GetTime()));//tm = bpNd->GetNodeTM(0);

	decomp_affine(TM, &ap);
	negScale = TMNegParity(node->GetObjTMAfterWSM(ip->GetTime()));
	if(negScale) 
	{	RebuildMatrix(TM);
		vx[0] = 0;vx[1] = 1;vx[2] = 2;	
	}
	else 
	{	vx[0] = 0;vx[1] = 2;vx[2] = 1;	}
	os = node->EvalWorldState(ip->GetTime());
	if (!os.obj || os.obj->SuperClassID()!=GEOMOBJECT_CLASS_ID) 
	{	MessageBox(NULL,"Katta ildizni ololmayman!","Diqqat",MB_OK);
		return; // Safety net. This shouldn't happen.
	}
//*******  Mesh qilib ko'ramiz **********************************************
	tri = GetTriObjectFromNode(node, ip->GetTime(), &needDel);
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
	if(!openfile(sMyData.szExport, nodeName, bClone ? ".cls2m" : ".s2m", &streamBin  ,"wb"))
	{	char fullname[128]; sprintf(fullname, "%s\\%s%s", sMyData.szExport, nodeName,  bClone ? ".cls2m" : ".s2m");
		MessageBox(NULL, fullname, "Faylini ochishda xato.", MB_OK);
		return;
	}
	char sh[20] = "Skn2mrp16sft.......";
	if(sMyData.b32bit){sh[7]='3'; sh[8]='2';}
	fwrite(sh, 1, 20, streamBin );
//*********************************************************************************
	// if trans to be own matrix *********************************************
	float mposx,mposy,mposz;
	mposx = ap.t.x * sMyData.fMassh;
	mposy = ap.t.y * sMyData.fMassh;
	mposz = ap.t.z * sMyData.fMassh;

	fwrite( &mposx, sizeof(float), 1, streamBin );
	fwrite( &mposz, sizeof(float), 1, streamBin );
	fwrite( &mposy, sizeof(float), 1, streamBin );
	//*********** endi rotateni yozsak bo'lar,??? *****************************
	//Buni joyini almashtirib yozib bo'lmaydur:
	Point3 fRot = GetFlippedRot(&TM);
	//Programmaning o'zida almashtiramiz:
	//agar FlipAxisInRot(fRot); ni ishlatsang, tepadagi minuslarni olib tashla!!!!
	fwrite( &fRot.x, sizeof( fRot.x ), 1, streamBin );
	fwrite( &fRot.y, sizeof( fRot.y ), 1, streamBin );
	fwrite( &fRot.z, sizeof( fRot.z ), 1, streamBin );
	//agar clone bo'lsa:
	if(bClone) goto Tamom;
//***************   FACE lar sonini yozamiz **********************************
	fwrite(&numFaces, sizeof(DWORD), 1, streamBin);
//******************  1 - hammasini 1 joyga to'playmiz *************************************
	char s[256];
	wsprintf(s,"%s%s",nodeName," malumotlarni yig'ish");
	old_tv1_verts_sk2 = new _VERTS_T2F_N3F_V3F [numFaces * 3];
	old_faces = new FACES_NORMAL_AND_SMOOTH_GROUPE[numFaces];//Smoothing groupe uchun

	for (DWORD i=0; i<numFaces; i++) 
	{	for (k=0; k<3; k++) 
		{	ind[k] = (WORD)mesh->faces[i].v[vx[k]];
			t_ind[k] = bFlatten ? (WORD)mesh->mapFaces(flatMapChan)[i].t[vx[k]] :
								(WORD)mesh->tvFace[i].t[vx[k]];
			Point3 v;
			v = mesh->verts[ind[k]];
			
			errStep=3;
			Point3 vertexNormal; if(!GetVertexNormal(mesh, i, mesh->getRVertPtr(ind[k]), &vertexNormal)){fclose(streamBin);return;}
			errStep=4;

			calcMaxMin
			Point3 tv;UVVert ftv;
			if(bFlatten)
				ftv = mesh->mapVerts(flatMapChan)[t_ind[k]];
			else
				tv = mesh->tVerts[t_ind[k]];

			old_tv1_verts_sk2[i * 3 + k].p = v;

			old_tv1_verts_sk2[i * 3 + k].n = vertexNormal;

			if(bFlatten)
			{	old_tv1_verts_sk2[i * 3 + k].tv = (float)ftv.x;		
				old_tv1_verts_sk2[i * 3 + k].tu = (float)ftv.y;		
			}else
			{	old_tv1_verts_sk2[i * 3 + k].tv = (float)tv.x;		
				old_tv1_verts_sk2[i * 3 + k].tu = (float)tv.y;		
		}	}
		//Feys normali va smoothing groupe i ni ham topib qo'yayluk:
		//old_faces[i].smGr = mesh->faces[i].getSmGroup();//smmothing groupe i
		//Point3 pv0(old_tv1_verts_sk2[i*3+1].p - old_tv1_verts_sk2[i*3].p);
		//Point3 pv1(old_tv1_verts_sk2[i*3+2].p - old_tv1_verts_sk2[i*3].p);

		//Feys normali:
		//old_faces[i].n = CrossProd(pv1, pv0);//Soat miliga qarshi yo'n. bo'yicha cross.
		//old_faces[i].n = Normalize(old_faces[i].n);

		//Vertexlarga ham shuni yozib qo'ylik:
		//old_tv1_verts_sk2[i * 3    ].n = old_faces[i].n;
		//old_tv1_verts_sk2[i * 3 + 1].n = old_faces[i].n;
		//old_tv1_verts_sk2[i * 3 + 2].n = old_faces[i].n;

		Progr(0,10,(float)i/(int)numFaces,"skin", ip);
	}

	//SmoothNormalsWithWeightingByFaceAngleSkn2(ip);

	float mXFMIN,mXFMAX,mYFMIN,mYFMAX,mZFMIN,mZFMAX;
	mXFMIN = XFMIN * sMyData.fMassh;
	mXFMAX = XFMAX * sMyData.fMassh;
	mZFMIN = ZFMIN * sMyData.fMassh;
	mZFMAX = ZFMAX * sMyData.fMassh;
	mYFMIN = YFMIN * sMyData.fMassh;
	mYFMAX = YFMAX * sMyData.fMassh;
	DeltaMin = 0.0001f* sqrtf((mXFMAX-mXFMIN)*(mXFMAX-mXFMIN) + (mYFMAX-mYFMIN)*(mYFMAX-mYFMIN) + 
							 (mZFMAX-mZFMIN)*(mZFMAX-mZFMIN) );
	if(DeltaMin > 0.0001f) DeltaMin = 0.0001f;
	fwrite(&mXFMIN, sizeof(mXFMIN), 1, streamBin);// float
	fwrite(&mXFMAX, sizeof(mXFMAX), 1, streamBin);// float
	fwrite(&mZFMIN, sizeof(mZFMIN), 1, streamBin);// float
	fwrite(&mZFMAX, sizeof(mZFMAX), 1, streamBin);// float
	fwrite(&mYFMIN, sizeof(mYFMIN), 1, streamBin);// float
	fwrite(&mYFMAX, sizeof(mYFMAX), 1, streamBin);// float

	NumNewVert = numFaces * 3;	
//********* tekshirish uchun eski vertexlarni bir yozib ko'raylik *****************************
//********* tekshirish uchun eski vertexlarni bir yozib ko'raylik *****************************
//********* tekshirish uchun eski vertexlarni bir yozib ko'raylik *****************************
//********* tekshirish uchun eski vertexlarni bir yozib ko'raylik *****************************
//********* tekshirish uchun eski vertexlarni bir yozib ko'raylik *****************************
//********* tekshirish uchun eski vertexlarni bir yozib ko'raylik *****************************
//********* tekshirish uchun eski vertexlarni bir yozib ko'raylik *****************************
//********* tekshirish uchun eski vertexlarni bir yozib ko'raylik *****************************
//********* tekshirish uchun eski vertexlarni bir yozib ko'raylik *****************************
//********* tekshirish uchun eski vertexlarni bir yozib ko'raylik *****************************
	int vts; vts = (int)numVerts;
//****************************************************************************
	VerOldTabl = new int [NumNewVert];
	for (DWORD i=0; i<numFaces; i++) 
	{	VerOldTabl[i*3    ] = mesh->faces[i].v[vx[0]];
		VerOldTabl[i*3 + 1] = mesh->faces[i].v[vx[1]];
		VerOldTabl[i*3 + 2] = mesh->faces[i].v[vx[2]];
	}
//********* tekshirish uchun eski vertexlarni bir yozib ko'raylik *****************************
//********* tekshirish uchun eski vertexlarni bir yozib ko'raylik *****************************
//********* tekshirish uchun eski vertexlarni bir yozib ko'raylik *****************************
//********* tekshirish uchun eski vertexlarni bir yozib ko'raylik *****************************
//********* tekshirish uchun eski vertexlarni bir yozib ko'raylik *****************************
//********* tekshirish uchun eski vertexlarni bir yozib ko'raylik *****************************
//********* tekshirish uchun eski vertexlarni bir yozib ko'raylik *****************************
//********* tekshirish uchun eski vertexlarni bir yozib ko'raylik *****************************
//********* tekshirish uchun eski vertexlarni bir yozib ko'raylik *****************************
//********* tekshirish uchun eski vertexlarni bir yozib ko'raylik *****************************
//**************************  Endi yozamiz ****************************************************
//****************************************************************************
    fpos_t posVerts;
	fgetpos(streamBin, &posVerts);
	for(DWORD i=0;i<NumNewVert;i++)//yangi vertexlar, keyinchalik qaytadan yozamiz, pozini eslab qoldik;
	{	fwrite( &old_tv1_verts_sk2[i].p.x, sizeof( old_tv1_verts_sk2[i].p.x ), 1, streamBin );
		fwrite( &old_tv1_verts_sk2[i].p.z, sizeof( old_tv1_verts_sk2[i].p.z ), 1, streamBin );
		fwrite( &old_tv1_verts_sk2[i].p.y, sizeof( old_tv1_verts_sk2[i].p.y ), 1, streamBin );
		fwrite( &old_tv1_verts_sk2[i].n.x, sizeof( old_tv1_verts_sk2[i].n.x ), 1, streamBin );
		fwrite( &old_tv1_verts_sk2[i].n.z, sizeof( old_tv1_verts_sk2[i].n.z ), 1, streamBin );
		fwrite( &old_tv1_verts_sk2[i].n.y, sizeof( old_tv1_verts_sk2[i].n.y ), 1, streamBin );
		fwrite( &old_tv1_verts_sk2[i].tv , sizeof( old_tv1_verts_sk2[i].tv  ), 1, streamBin );
		fwrite( &old_tv1_verts_sk2[i].tu , sizeof( old_tv1_verts_sk2[i].tu  ), 1, streamBin );
		Progr(10,30,(float)i/(int)NumNewVert,"skin", ip); 
	}
//********************  SKIN   ****************************
//********************  SKIN   ****************************
//********************  SKIN   ****************************
//********************  SKIN   ****************************
//********************  SKIN   ****************************
//********************  SKIN   ****************************
//********************  SKIN   ****************************
//********************  SKIN   ****************************
//********************  SKIN   ****************************
//********************  SKIN   ****************************
//********************  SKIN   ****************************
//********************  SKIN   ****************************
//********************  SKIN   ****************************
//********************  SKIN   ****************************
//********************  SKIN   ****************************
//********************  SKIN   ****************************
//********************  SKIN   ****************************
//********************  SKIN   ****************************
//********************  SKIN   ****************************
//********************  SKIN   ****************************
//********************  SKIN   ****************************
//********************  SKIN   ****************************
//********************  SKIN   ****************************
//********************  SKIN   ****************************
//********************  SKIN   ****************************
//********************  SKIN   ****************************
//********************  SKIN   ****************************
//********************  SKIN   ****************************
//********************  SKIN   ****************************
//********************  SKIN   ****************************
//********************  SKIN   ****************************
//********************  SKIN   ****************************
//********************  SKIN   ****************************

	wNumBones = bFootSteps ? (WORD)(numBones-1) : (WORD)numBones;
	fwrite( &wNumBones, sizeof( wNumBones ), 1, streamBin    );
 	for(WORD i = 0; i < wNumBones; i++)
	{	boneName = pBonNodes[pBonesHierarchTable[i]]->GetName();//node ni nomini olamiz
		fwrite(boneName, lstrlen(boneName)+1, 1, streamBin);
	}

	skin->GetSkinInitTM(node, mMeshTM, TRUE);//	Matrix3 mMeshTM1;skin->GetSkinInitTM(node, mMeshTM1,FALSE); Node TM xato bo'lg'ay;

	//FILE *f=fopen("ExpBonesTMs.txt", "w");
	//MRow *mr = mMeshTM.GetAddr();
	//fprintf(f, "\n     MeshTM: %.3f %.3f %.3f     %.3f %.3f %.3f     %.3f %.3f %.3f     %.3f %.3f %.3f", mr[0][0], mr[0][1], mr[0][2], mr[1][0], mr[1][1], mr[1][2], mr[2][0], mr[2][1], mr[2][2], mr[3][0], mr[3][1], mr[3][2]); 

	/*Point3 TMSC = GetMatrixScale(&mMeshTM);
	if(fabs(TMSC.x - 1.0f) > 0.002f)
		{MessageBox(NULL,"Please, check skin mesh X scale.","Err.",MB_OK);fclose(streamBin);return;}
	else if(fabs(TMSC.y - 1.0f) > 0.002f)
		{MessageBox(NULL,"Please, check skin mesh Y scale.","Err.",MB_OK);fclose(streamBin);return;}
	if(fabs(TMSC.z - 1.0f) > 0.002f)
		{MessageBox(NULL,"Please, check skin mesh Z scale.","Err.",MB_OK);fclose(streamBin);return;}
*/
	r = FlipYZAxisInMatrix(mMeshTM);
	r._41 *= sMyData.fMassh; r._42 *= sMyData.fMassh; r._43 *= sMyData.fMassh;
	fwrite(&r, 16*sizeof(float), 1, streamBin);

 	for(WORD i = 0; i < wNumBones; i++)
	{	Matrix3 initBone;//, boneStretchTM;
		skin->GetBoneInitTM(pBonNodes[pBonesHierarchTable[i]], initBone, TRUE);//FALSE bersak xato bo'lg'ay;
		Point3 TMSC = GetMatrixScale(&initBone);
		if( (fabs(TMSC.x - 1.0f) > 0.002f) || (fabs(TMSC.y - 1.0f) > 0.002f) || (fabs(TMSC.z - 1.0f) > 0.002f) )
		{	//Matrix3 boneStretchTM = pBonNodes[pBonesHierarchTable[i]]->GetStretchTM(0);
			//initBone = boneStretchTM * initBone;
			TMSC = GetMatrixScale(&initBone);
			TMSC.x = 1.0f / TMSC.x; TMSC.y = 1.0f / TMSC.y; TMSC.z = 1.0f / TMSC.z;
			initBone.PreScale(TMSC, FALSE);
		}
		RebuildMatrix(initBone);//1 marta rebuild qilingan TM quat b-n 1 necha marta aylantirilsayam to'g'ri chiqadi;
		//mr = initBone.GetAddr();
		//fprintf(f, "\n     m_InitTM: %.3f %.3f %.3f     %.3f %.3f %.3f     %.3f %.3f %.3f     %.3f %.3f %.3f", mr[0][0], mr[0][1], mr[0][2], mr[1][0], mr[1][1], mr[1][2], mr[2][0], mr[2][1], mr[2][2], mr[3][0], mr[3][1], mr[3][2]); 

		//if(TMNegParity(initBone))//Mirror qilingan bo'lsaya:
		//	RebuildMatrix(initBone);
		r = FlipYZAxisInMatrix(initBone);//ConvToD3DXMATRIX(&initBone);
		r._41 *= sMyData.fMassh; r._42 *= sMyData.fMassh; r._43 *= sMyData.fMassh;
		fwrite(&r, 16*sizeof(float), 1, streamBin);
		Progr(30,40,(float)i/(int)numBones,"Bones:", ip);
	}//fclose(f);
//*********** ISkinContextData ni olamiz **************************************
	skincontext = skin->GetContextInterface(node);
	if(!skincontext) 
	{	MessageBox(NULL, "Skin Context was not founded.", "err.", MB_OK);
		goto NotSkinContext;
	}
//*** meshni vertexlari va facelariga qarab tesselate qilib chiqish ***********
//***   SKIN ALBATTA TNV BO'LADI **********************************************
//********* vertex weight ptr ***************
//********* vertex weight ptr ***************
//********* vertex weight ptr ***************
//********* vertex weight ptr ***************
//********* vertex weight ptr ***************
//********* vertex weight ptr ***************
//********* vertex weight ptr ***************
//********* vertex weight ptr ***************
//********* vertex weight ptr ***************
//********* vertex weight ptr ***************
    fpos_t posForSzWeights;char st;
	fgetpos(streamBin, &posForSzWeights); 
	fwrite(&st, sizeof(st), 1, streamBin);// bone pointerga ko'rsatish uchun 
	fwrite(&st, sizeof(st), 1, streamBin);// DWORD ptr joy tashlab ketaman. 
	fwrite(&st, sizeof(st), 1, streamBin);
	fwrite(&st, sizeof(st), 1, streamBin);
//********* vertex weight ptr ***************
//********* vertex weight ptr ***************
//********* vertex weight ptr ***************
//********* vertex weight ptr ***************
//********* vertex weight ptr ***************
//********* vertex weight ptr ***************
//********* vertex weight ptr ***************
//********* vertex weight ptr ***************
//********* vertex weight ptr ***************
//********* vertex weight ptr ***************
//*****************  1 - hammasini 1 joyga to'playmiz *************************************
	maxAssignedBone = 0;//VERTEX BLEND ni qo'llsah uchun o'zgartirish:
	sk_verts = new VERTS_SKIN_T2F_N3F_V3F[NumNewVert];
	for (DWORD i=0; i<NumNewVert; i++) 
	{	sk_verts[i].numAssigBones = skincontext->GetNumAssignedBones(VerOldTabl[i]);
		sk_verts[i].pWeightTable = new weightTableForSkVrtx[sk_verts[i].numAssigBones];
		WORD wNumAssigBones;wNumAssigBones = (WORD)sk_verts[i].numAssigBones;
		if(!wNumAssigBones)
		{	char st[64]; sprintf(st, "%d - face", i/3);
			if(IDCANCEL == MessageBox(NULL, "Vertex is not binded to any bone!!!", st, MB_OKCANCEL))
				goto ErrWeight;
		} /*else if(wNumAssigBones > 4)
		{	char st[64]; sprintf(st, "Face numb: %d, Pos: %.2f %.2f %.2f", i/3, old_tv1_verts_sk2[i].p.x, old_tv1_verts_sk2[i].p.y, old_tv1_verts_sk2[i].p.z);
			if(IDCANCEL == MessageBox(NULL, st, "Vertex is binded to more then 4 bone!!!", MB_OKCANCEL))
				goto ErrWeight;
		}*/
		fwrite(&wNumAssigBones,sizeof(wNumAssigBones),1,streamBin);	

//		if(i==1661 || i==1705 || i==1722 || i==1740 || i==1745 || i==1851 || i==1910)
//			Beep(10,10);
		for(k = 0; k < sk_verts[i].numAssigBones; k++)
		{	sk_verts[i].pWeightTable[k].whichBone = skincontext->GetAssignedBone(VerOldTabl[i], k);
			if(bFootSteps)
			{	if(iFootStepsBonNum < sk_verts[i].pWeightTable[k].whichBone)
					sk_verts[i].pWeightTable[k].whichBone --;
			}
			sk_verts[i].pWeightTable[k].weightForBone = skincontext->GetBoneWeight(VerOldTabl[i], k);
			WORD wWhichBone; wWhichBone = GetBonPosInNewHierarchTable(sk_verts[i].pWeightTable[k].whichBone);
			fwrite(&wWhichBone,sizeof(WORD),1,streamBin);
			fwrite(&sk_verts[i].pWeightTable[k].weightForBone,sizeof(float),1,streamBin);

			if(IsErrFloatIndValue(sk_verts[i].pWeightTable[k].weightForBone))
			{	char errI[64]; sprintf(errI, "%d - vert, %d - face;", i, i/3);
				if(IDOK!=MessageBox(NULL, "Vertex weightida xatolik mavjud, davom etaymi?", errI, MB_YESNO))
					goto ErrWeight;				
		}	}
		Progr(40,60,(float)i/(int)NumNewVert, "Skin2morph", ip); 
		if(maxAssignedBone < (BYTE)sk_verts[i].numAssigBones)
			maxAssignedBone = (BYTE)sk_verts[i].numAssigBones;
	}
//***********************************************************
//***********************************************************
//***********************************************************
//***********************************************************
    fpos_t posSle;
	fgetpos(streamBin, &posSle);
	fwrite(&maxAssignedBone, sizeof(BYTE), 1, streamBin);
	DWORD sz = (DWORD)(posSle - posForSzWeights - 4);
	fseek(streamBin, (long)posForSzWeights, SEEK_SET);
	fwrite(&sz, sizeof(sz), 1, streamBin);
	
	fseek(streamBin, (long)posVerts, SEEK_SET);
	for(DWORD i=0; i<numFaces; i++)
	{	for(int fv=0; fv<3; fv++)
		{	Point3 mpos; mpos = old_tv1_verts_sk2[i*3+fv].p * sMyData.fMassh;
			fwrite(&mpos.x, sizeof(mpos.x), 1, streamBin);
			fwrite(&mpos.z, sizeof(mpos.z), 1, streamBin);
			fwrite(&mpos.y, sizeof(mpos.y), 1, streamBin);
			fwrite(&old_tv1_verts_sk2[i*3+fv].n.x, sizeof(old_tv1_verts_sk2[i*3+fv].n.x), 1, streamBin);
			fwrite(&old_tv1_verts_sk2[i*3+fv].n.z, sizeof(old_tv1_verts_sk2[i*3+fv].n.z), 1, streamBin);
			fwrite(&old_tv1_verts_sk2[i*3+fv].n.y, sizeof(old_tv1_verts_sk2[i*3+fv].n.y), 1, streamBin);
			//DirectXga OpenGL dan farqini kiritamiz:
			old_tv1_verts_sk2[i*3+fv].tu = 1.f - old_tv1_verts_sk2[i*3+fv].tu;
			fwrite(&old_tv1_verts_sk2[i*3+fv].tv, sizeof( old_tv1_verts_sk2[i*3+fv].tv ), 1, streamBin );
			fwrite(&old_tv1_verts_sk2[i*3+fv].tu, sizeof( old_tv1_verts_sk2[i*3+fv].tu ), 1, streamBin );
		}
		Progr(90,100,(float)i/(int)numFaces,"Writing skin vertices ...", ip);
	}
//************* Morphni shu yerga tiqamiz: **************************
//************* Morphni shu yerga tiqamiz: **************************
//************* Morphni shu yerga tiqamiz: **************************
//************* Morphni shu yerga tiqamiz: **************************
//************* Morphni shu yerga tiqamiz: **************************
//************* Morphni shu yerga tiqamiz: **************************
//************* Morphni shu yerga tiqamiz: **************************
//************* Morphni shu yerga tiqamiz: **************************
//************* Morphni shu yerga tiqamiz: **************************
//************* Morphni shu yerga tiqamiz: **************************
//************* Morphni shu yerga tiqamiz: **************************
	fseek(streamBin, 0, SEEK_END);
	if(!dwBandChan) MessageBox(NULL, " Alarm: Convertation pr-m will corrapt!!!", "No channels founded.", MB_OK);
	else
	{	fwrite(&dwBandChan ,sizeof(dwBandChan),1,streamBin);
 		old_tv2_verts = new VERTS_T2F_N3F_V3F[NumNewVert];
		for(DECL(unsigned) i=0;i<mrphSz;i++)
		{	morphChannel *channel = &(morpher->chanBank[i]);
			if(!channel || !channel->mActive || !channel->mActiveOverride || !channel->cblock)
				continue; //hech bo'lmasa birorta 0 dan farqli perc bormi
			channel->cblock->SetValue(0, ip->GetTime(), 100.0f);

			if(needDel)tri->DeleteMe();
			tri = GetTriObjectFromNode(node, ip->GetTime(), &needDel);
			if (!tri) 
			{	MessageBox(NULL,"Obyektni editmesh qilolmayman!","Diqqat",MB_OK);
				return;
			}
			mesh = &tri->GetMesh();
			mesh->buildNormals();
			CalcNewMorphVertsSkn2(ip);
			WriteChangedMorphVertsSkn2();
			channel->cblock->SetValue(0, ip->GetTime(), 0.0f);
			Progr(80,100,(float)i/(int)mrphSz,"Morph:", ip); 
	}	}
ErrWeight:
	fclose(streamBin); 
	if(sk_verts)
	{	for (DWORD i=0; i<NumNewVert; i++) 
			delete sk_verts[i].pWeightTable;
		delete sk_verts;	
	}
NotSkinContext:

	if(old_tv1_verts_sk2) delete old_tv1_verts_sk2;	
	if(old_tv2_verts    )delete [] old_tv2_verts;	
	if(VerOldTabl) delete VerOldTabl;
	if(pBonNodes) delete pBonNodes;
	if(pBonesParentTable) delete pBonesParentTable;
	if(pBonesHierarchTable) delete pBonesHierarchTable;
	if(keyArrays) delete keyArrays;

Tamom:
	if(needDel)tri->DeleteMe();
	RestoreAllChannels(ip);
	delete [] chanPrs;

	char fullname[128];	sprintf(fullname, "%s\\%s%s", sMyData.szExport, nodeName, bClone ? ".cls2m" : ".s2m");
	if(sMyData.bInIndexed && (!bClone))
	{	DeleteWeightsBelovePercFrSkin2Morph(fullname, 10);
		//if((!sMyData.bTangent) && sMyData.bDetachSknMesh)
		//	DetachOneWeightFacesFrSkin2Morph(fullname);// TB uchun kerakmas;
		ConvertSoftSkin2MorphToIndexedFormat(fullname, sMyData.fThreshold);
		sprintf(fullname, "%s\\%s%s", sMyData.szExport, nodeName, ".smi");
		ConvertIndexedSkin2MorphToSHFormat(fullname);

		sprintf(fullname, "%s\\%s%s", sMyData.szExport, nodeName, ".sb6");
		if(sMyData.bTangent)
		{	ConvertSHKin2MorphToSHSkin2MorphTangentAndBinormalUseDX(fullname);
			sprintf(fullname, "%s\\%s%s", sMyData.szExport, nodeName, ".sb7");
			ConvertIndexedShaderSkin2MorphTBToF3_UB4N_F16_2Format(fullname);
			DeleteFile(fullname);
			sprintf(fullname, "%s\\%s%s", sMyData.szExport, nodeName, ".sbu");
			RenameFile(fullname, lstrlen(fullname)-1,'7');
			if(sMyData.bToText)
			{	sprintf(fullname, "%s\\%s%s", sMyData.szExport, nodeName, ".sb7");
				SHSkin2MorphTBIndexedToText(fullname);
		}	}
		else
		{	ConvertIndexedShaderSkin2MorphToF3_UB4N_F16_2Format(fullname);
			fullname[lstrlen(fullname)-1]='u';
			RenameFile(fullname, lstrlen(fullname)-1, '6');
			fullname[lstrlen(fullname)-1]='6';

			if(sMyData.bToText)
				SHSkin2MorphIndexedToText(fullname);
		}

		sprintf(fullname, "%s\\%s%s", sMyData.szExport, nodeName, ".s2m");
		if(sMyData.bDeleteSource)
			DeleteFile(fullname);
		sprintf(fullname, "%s\\%s%s", sMyData.szExport, nodeName, ".smi");
		if(sMyData.bDeleteSource)
			DeleteFile(fullname);
		/*char p[1024], nm[128]; sprintf(p, "%s", sMyData.szExport);int nn=0, first = -1, ln=lstrlen(p); 
		if(!sMyData.bTangent)
		{	if(sMyData.bDetachSknMesh)
			{	for(int b=0; b<wNumBones; b++)
				{	sprintf(nm, "%d.msi", b); 
					sprintf(fullname, "%s\\%s", sMyData.szExport, nm);
					FILE *fm=fopen(fullname, "r"); if(fm)
					{	fclose(fm);
						p[ln++] = 0; 
						for(int i=0; i<lstrlen(nm); i++)
							p[ln++] = nm[i];
						if(-1==first) first = b;
						nn++;
				}	}
				p[ln++] = 0; CollectSkinMeshes(p, nn, ln);
				sprintf(fullname, "%s\\%d%s", sMyData.szExport, first, ".smv");
				sprintf(p, "%s\\%s%s", sMyData.szExport, nodeName, ".smv");
				CopyFile(fullname, p, FALSE);
				if(sMyData.bDeleteSource)
					DeleteFile(fullname);
				if(sMyData.bToText)
					IndexedSkinMeshesToText(p);
		}	}
		else//if(sMyData.bTangent)
		{	ConvertSkinMeshToSkinMeshTangentAndBinormal(p);
			if(sMyData.bToText)
			{	sprintf(p, "%s\\%s%s", sMyData.szExport, nodeName, ".smt");
				IndexedSkinMeshesTBToText(p);
		}	}*/
	}
	else if(sMyData.bToText) SoftSkin2MorphToText(fullname);
	SkinModif->EnableMod();//meshni olayotganda skinni disable qilib qo'yamiz
 	return;
}

VOID AllSkin2Morph(LPVOID par)
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
	{	MessageBox(NULL,"1nechtasini olib tashlang!","Diqqat",MB_OK);
		return;
	}


	for(k=0;k<=i;k++)
	{	__try
		{
			streamBin = 0;
			ExportOneSkin2Morph(ip,iu,k);
			streamBin = 0;
		}
		__except(1,1)
		{
			MessageBox(NULL, "Kutilmagan gen. xato", "Diqqat!!!", MB_OK);
			if(streamBin) fclose(streamBin);
	}	}
	return;
}
