#include "ExpDX.h"
#include "modstack.h"
#include "iunwrap.h"
#include "animtbl.h"
#include "decomp.h"
#include "../../../Libs/Math/mMathFn.h"
#include "C:\Program Files (x86)/Autodesk/3ds Max 2010 SDK/maxsdk/samples/controllers/reactor/reactapi.h"

ISkin					*skin; 
ISkinContextData		*skincontext; 
int						numBones,*pBonesParentTable,*pBonesHierarchTable,
						*VerOldTabl;//agar 1 ta to'liq weight 
BOOL					*keyArrays; 
INode					**pBonNodes;                //bo'lsa, bone raqami, aks holda -1 ga teng; 
Modifier				*SkinModif; 					
WORD					wNumBones;//footstep ni hisobga olganda
TCHAR					*boneName;
Control					*c;
Animatable				*vertAn,*horAn,*rotAn;
VERTS_SKIN_T2F_N3F_V3F  *sk_verts;

bool GetSkinModifier(INode *node)
{
	Object* object = node->GetObjectRef();
	if(!object) return false;
	while(object->SuperClassID() == GEN_DERIVOB_CLASS_ID && object)
	{	IDerivedObject* derivedobject = static_cast<IDerivedObject *>(object);
		int modcount = 0;
		while(modcount < derivedobject->NumModifiers())
		{	SkinModif = derivedobject->GetModifier(modcount);
			if (SkinModif->ClassID() == SKIN_CLASSID)
			{	skin = (ISkin*)SkinModif->GetInterface(I_SKIN);
				return true;
			}
			modcount++;
		}	
		object = derivedobject->GetObjRef();
	}
	return false;
}

int GetParentBoneNum(char *parName)
{
int		i,ii;

	ii = 0;
	for(i = 0; i < numBones; i++)
	{	boneName = skin->GetBone(i)->GetName();//node ni nomini olamiz
		if(!strcmp(boneName, parName))
			return ii;
		ii++;
	}
	return -1;
}

int GetParentCounts(INode* node)
{
int		i;
INode	*n;
	
	n = node;	
	i = 0;
	
	while(1)
	{	if(n->IsRootNode())
			return i;
		n = n->GetParentNode();
		i++;
	}
	return 0;
}

VOID RebuildMatrix(Matrix3 &M)
{
AffineParts	AP;
	decomp_affine(M, &AP);
	M.SetRotate(AP.q);
	M.Translate(AP.t);
	return;
}

VOID RebuildMakeClosestQuat(Matrix3 &m, Matrix3 *mCompare)
{
AffineParts	AP, APCMP;
	decomp_affine(m, &AP);
	decomp_affine(*mCompare, &APCMP);
	if(mCompare)//Birinchisiga tegishli emas;
		AP.q.MakeClosest(APCMP.q);
	m.SetRotate(AP.q);
	m.Translate(AP.t);
	return;
}

//1 ta vertexga qarab facening qolgan 2 ta vert.ni topish
VOID FindOtherTwoVert(int *v1, int *v2, int *v3)
{
	for (DWORD i=0; i<numFaces; i++) 
	{	ind[0] = (DWORD)mesh->faces[i].v[0];
		ind[1] = (DWORD)mesh->faces[i].v[1];
		ind[2] = (DWORD)mesh->faces[i].v[2];
		if(ind[0] == *v1)
		{ *v2 = ind[1]; *v3 = ind[2]; return; }
		if(ind[1] == *v1)
		{ *v2 = ind[0]; *v3 = ind[2]; return; }
		if(ind[2] == *v1)
		{ *v2 = ind[0]; *v3 = ind[1]; return; }
	}
	return;
}

//Eski tablening oldPos o'rinining yangi table dagi o'rni:
int GetBonPosInNewHierarchTable(int oldPos)
{
	if(oldPos == -1) return -1;
	int ii = 0;
	char *bnIT; bnIT = skin->GetBone(pBonesHierarchTable[oldPos])->GetName();//node ni nomini olamiz
	for(int i = 0; i < numBones; i++)
	{	boneName = skin->GetBone(i)->GetName();//node ni nomini olamiz
		if(strstr(boneName,"Footsteps") || strstr(boneName,"Nub") || strstr(boneName,"nub"))
			continue;
		if(pBonesHierarchTable[ii] == oldPos)
			return ii;
		ii++;
	}
	MessageBox(NULL,"Diqqat","Bon Hier. jadv.dagi yangi o'rinni topolmadim",MB_OK);
	return -1;
}

//pBonesHierarchTable[i] bo'lsa, i-yangi tablitsadagi orin, pBonesHierarchTable[i] esa eski tabledagi o'rin;
int BuildBonesHierarchy(int NumBones, BOOL *bFootSteps, int *iFootStepsBonNum)
{
*iFootStepsBonNum = -1;
*bFootSteps = FALSE;

//*****************************************************************************
	pBonNodes = 0;	pBonesParentTable = 0;	pBonesHierarchTable= 0;
	pBonNodes         = new INode *  [NumBones];
	pBonesParentTable = new int      [NumBones];
	pBonesHierarchTable= new int      [NumBones];

	int ii; ii = 0;
	for(WORD i = 0; i < NumBones; i++)
	{	boneName = skin->GetBone(i)->GetName();//node ni nomini olamiz
		if(strstr(boneName,"Footsteps"))
		{	*bFootSteps = TRUE; *iFootStepsBonNum =i;
			continue;
		}
		if(strstr(boneName,"Nub") || strstr(boneName,"nub"))
		{	MessageBox(NULL, "Skin bonelaridan Nub-Pubini olib tashlash kerak.", "Aks holda noto'g'ri ishlaydi, o'yinda", MB_OK);
Fin:		if(pBonNodes         ) delete [] pBonNodes;
			if(pBonesParentTable ) delete [] pBonesParentTable;
			if(pBonesHierarchTable) delete [] pBonesHierarchTable;
			return -1;
		}	
		pBonNodes[ii] = skin->GetBone(i);
		if(!pBonNodes[ii])
		{	char s[128];
			wsprintf(s,"%d%s",i," chi bone ni topolmadim.");
			MessageBox(NULL,s,"Kechirasiz, chiqib ketaman!!!",MB_OK);
			goto Fin;
		}
		INode *parNode = pBonNodes[ii]->GetParentNode();
		boneName = parNode->GetName();//parent node ni nomini olamiz
		pBonesParentTable[ii] = GetParentBoneNum(boneName);
		ii++;
	}
//********** iyerarxiyasi bo'yicha tartib bilan qo'yib chiqishim kerak;
	int *pBonesParentCounts; pBonesParentCounts = new int [numBones];
	int stepCount; stepCount = 0;
	ii = 0;
	for(WORD i = 0; i < NumBones; i++)
	{	boneName = skin->GetBone(i)->GetName();//node ni nomini olamiz
		if(strstr(boneName,"Footsteps")) continue;
		pBonesParentCounts[ii] = GetParentCounts(pBonNodes[ii]);
		if(pBonesParentCounts[ii] > stepCount) stepCount = pBonesParentCounts[ii];
		ii++;
	}
	int countHier; countHier = 0;
	for(int k = 0; k <= stepCount; k++)
	{	ii = 0;
		for(WORD i = 0; i < NumBones; i++)
		{	boneName = skin->GetBone(i)->GetName();//node ni nomini olamiz
			if(strstr(boneName,"Footsteps")) continue;
			if(pBonesParentCounts[ii] == k)
			{	pBonesHierarchTable[countHier] = ii;
				++countHier;
			}
			ii++;
	}	}
	delete [] pBonesParentCounts;
	//1 xil nomlarni topsin:
	for(WORD i = 0; i < NumBones; i++)
	{	boneName = skin->GetBone(i)->GetName();//node ni nomini olamiz
		int l1,l2 = lstrlen(boneName);
		for(ii = i+1; ii < NumBones; ii++)
		{	char *nxtName = skin->GetBone(ii)->GetName();
			l1 =  lstrlen(nxtName);
			if(l1==l2 && (!strcmp(boneName, nxtName)))
				MessageBox(NULL, boneName, "2 bones with same name.", MB_OK); 
	}	}
	return pBonesHierarchTable[0];
}

VOID ExportOneSkin(Interface *ip,IUtil *iu,int sel_num)
{
TSTR		indent;				TriObject	*tri;				
ObjectState	os;					BOOL		negScale;
Point3		rots,scs;			Matrix3		TM, mMeshTM, mInv; 
BYTE        maxAssignedBone;	int			k, baseBone, iFootStepsBonNum;
BOOL        bFootSteps; D3DXMATRIX  r;

float				XFMIN = 3.402823466e+38F, XFMAX = -3.402823466e+38F, 
					YFMIN = 3.402823466e+38F, YFMAX = -3.402823466e+38F, 
					ZFMIN = 3.402823466e+38F, ZFMAX = -3.402823466e+38F; 
#define calcMaxMin	if(XFMIN > v[0]) XFMIN = v[0];\
					if(XFMAX < v[0]) XFMAX = v[0];\
					if(YFMIN > v[1]) YFMIN = v[1];\
					if(YFMAX < v[1]) YFMAX = v[1];\
					if(ZFMIN > v[2]) ZFMIN = v[2];\
					if(ZFMAX < v[2]) ZFMAX = v[2];

	old_tv1_verts = 0; VerOldTabl  = 0; pBonNodes = 0;
	pBonesParentTable =0; sk_verts = 0; keyArrays = 0;

	node = ip->GetSelNode(sel_num);//node ni olamiz
	nodeName = node->GetName();//node ni nomini olamiz

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
	if(!openfile(sMyData.szExport, nodeName, ".skn", &streamBin  ,"wb"))
	{	char fullname[128]; sprintf(fullname, "%s\\%s%s", sMyData.szExport, nodeName, ".skn");
		MessageBox(NULL, fullname, "Faylini ochishda xato.", MB_OK);
		return;
	}
	char sh[20] = "Skn16sft...........";
	if(sMyData.b32bit){sh[3]='3'; sh[4]='2';}
	fwrite(sh, 1, 20, streamBin );
//***** MATERIAL ****************************************
	errStep = 1;
	myMtrl.Get(node);
	errStep = 2;
	myMtrl.Write(streamBin);
//*********************************************************************************
	// if trans to be own matrix *********************************************
	float mposx,mposy,mposz;
	mposx = ap.t.x * sMyData.fMassh;
	mposy = ap.t.y * sMyData.fMassh;
	mposz = ap.t.z * sMyData.fMassh;

	fwrite( &mposx   , sizeof( float )   , 1, streamBin );
	fwrite( &mposz   , sizeof( float )   , 1, streamBin );
	fwrite( &mposy   , sizeof( float )   , 1, streamBin );
	//*********** endi rotateni yozsak bo'lar,??? *****************************
	//Buni joyini almashtirib yozib bo'lmaydur:
	Point3 fRot = GetFlippedRot(&TM);
	//Programmaning o'zida almashtiramiz:
	//agar FlipAxisInRot(fRot); ni ishlatsang, tepadagi minuslarni olib tashla!!!!
	fwrite( &fRot.x, sizeof( fRot.x ), 1, streamBin );
	fwrite( &fRot.y, sizeof( fRot.y ), 1, streamBin );
	fwrite( &fRot.z, sizeof( fRot.z ), 1, streamBin );
//***************   FACE lar sonini yozamiz **********************************
	fwrite(&numFaces, sizeof(DWORD), 1, streamBin);
//******************  1 - hammasini 1 joyga to'playmiz *************************************
	char s[256];
	wsprintf(s,"%s%s",nodeName," malumotlarni yig'ish");
	old_tv1_verts = new VERTS_T2F_N3F_V3F [numFaces * 3];
	old_faces = new FACES_NORMAL_AND_SMOOTH_GROUPE[numFaces];//Smoothing groupe uchun

	for (DWORD i=0; i<numFaces; i++) 
	{	for (k=0; k<3; k++) 
		{	ind[k] = (WORD)mesh->faces[i].v[vx[k]];
			t_ind[k] = bFlatten ? (WORD)mesh->mapFaces(flatMapChan)[i].t[vx[k]] :
								(WORD)mesh->tvFace[i].t[vx[k]];
			Point3 v;
			v = mesh->verts[ind[k]];//mInv ga ko'paytirsang umuman xatodur;
		
			errStep=3;
			Point3 vertexNormal; if(!GetVertexNormal(mesh, i, mesh->getRVertPtr(ind[k]), &vertexNormal)){fclose(streamBin);return;}
			errStep=4;

			calcMaxMin
			Point3 tv;UVVert ftv;
			if(bFlatten)
				ftv = mesh->mapVerts(flatMapChan)[t_ind[k]];
			else
				tv = mesh->tVerts[t_ind[k]];

			old_tv1_verts[i * 3 + k].p = v;

			old_tv1_verts[i * 3 + k].n = vertexNormal;

			if(bFlatten)
			{	old_tv1_verts[i * 3 + k].tv = (float)ftv.x;		
				old_tv1_verts[i * 3 + k].tu = (float)ftv.y;		
			}else
			{	old_tv1_verts[i * 3 + k].tv = (float)tv.x;		
				old_tv1_verts[i * 3 + k].tu = (float)tv.y;		
		}	}
		//Feys normali va smoothing groupe i ni ham topib qo'yayluk:
		//old_faces[i].smGr = mesh->faces[i].getSmGroup();//smmothing groupe i
		//Point3 pv0(old_tv1_verts[i*3+1].p - old_tv1_verts[i*3].p);
		//Point3 pv1(old_tv1_verts[i*3+2].p - old_tv1_verts[i*3].p);

		//Feys normali:
		//old_faces[i].n = CrossProd(pv1, pv0);//Soat miliga qarshi yo'n. bo'yicha cross.
		//old_faces[i].n = Normalize(old_faces[i].n);

		//Vertexlarga ham shuni yozib qo'ylik:
		//old_tv1_verts[i * 3    ].n = old_faces[i].n;
		//old_tv1_verts[i * 3 + 1].n = old_faces[i].n;
		//old_tv1_verts[i * 3 + 2].n = old_faces[i].n;

		Progr(0,10,(float)i/(int)numFaces,"skin", ip); 
	}

	//SmoothNormalsWithWeightingByFaceAngle();

	float mXFMIN,mXFMAX,mYFMIN,mYFMAX,mZFMIN,mZFMAX;
	mXFMIN = XFMIN * sMyData.fMassh;
	mXFMAX = XFMAX * sMyData.fMassh;
	mZFMIN = ZFMIN * sMyData.fMassh;
	mZFMAX = ZFMAX * sMyData.fMassh;
	mYFMIN = YFMIN * sMyData.fMassh;
	mYFMAX = YFMAX * sMyData.fMassh;

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
	for(DWORD i=0;i<NumNewVert;i++)//yangi vertexlar **********************************
	{	Point3 mpos;
		mpos = old_tv1_verts[i].p * sMyData.fMassh;

		fwrite( &mpos.x   , sizeof( mpos.x )   , 1, streamBin );
		fwrite( &mpos.z   , sizeof( mpos.z )   , 1, streamBin );
		fwrite( &mpos.y   , sizeof( mpos.y )   , 1, streamBin );
		fwrite( &old_tv1_verts[i].n.x, sizeof( old_tv1_verts[i].n.x ), 1, streamBin );
		fwrite( &old_tv1_verts[i].n.z, sizeof( old_tv1_verts[i].n.z ), 1, streamBin );
		fwrite( &old_tv1_verts[i].n.y, sizeof( old_tv1_verts[i].n.y ), 1, streamBin );

		//********************
		//********************
		//********************
		//********************
		
		//DirectXga OpenGL dan farqini kiritamiz:
		old_tv1_verts[i].tu = 1.f - old_tv1_verts[i].tu;
		fwrite( &old_tv1_verts[i].tv, sizeof( old_tv1_verts[i].tv ), 1, streamBin );
		fwrite( &old_tv1_verts[i].tu, sizeof( old_tv1_verts[i].tu ), 1, streamBin );
		Progr(30,60,(float)i/(int)NumNewVert,"skin", ip); 
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

	//meshni olib bo'ldik, endi skinni enable qilib qo'ysak ham bo'laveradi

	wNumBones = bFootSteps ? (WORD)(numBones-1) : (WORD)numBones;
	fwrite( &wNumBones, sizeof( wNumBones ), 1, streamBin    );
 	for(WORD i = 0; i < wNumBones; i++)
	{	boneName = pBonNodes[pBonesHierarchTable[i]]->GetName();//node ni nomini olamiz
		fwrite(boneName, lstrlen(boneName)+1, 1, streamBin);
	}

	skin->GetSkinInitTM(node, mMeshTM, TRUE);//	Matrix3 mMeshTM1;skin->GetSkinInitTM(node, mMeshTM1,FALSE); Node TM xato bo'lg'ay;
	/*Point3 TMSC = GetMatrixScale(&mMeshTM);
	if(fabs(TMSC.x - 1.0f) > 0.002f)
		{MessageBox(NULL,"Please, check skin mesh X scale.","Err.",MB_OK);return;}
	else if(fabs(TMSC.y - 1.0f) > 0.002f)
		{MessageBox(NULL,"Please, check skin mesh Y scale.","Err.",MB_OK);return;}
	if(fabs(TMSC.z - 1.0f) > 0.002f)
		{MessageBox(NULL,"Please, check skin mesh Z scale.","Err.",MB_OK);return;}
*/
	r = FlipYZAxisInMatrix(mMeshTM);
	r._41 *= sMyData.fMassh; r._42 *= sMyData.fMassh; r._43 *= sMyData.fMassh;
	fwrite(&r, 16*sizeof(float), 1, streamBin);

 	for(WORD i = 0; i < wNumBones; i++)
	{	Matrix3 initBone;
		skin->GetBoneInitTM(pBonNodes[pBonesHierarchTable[i]], initBone, FALSE);
		Point3 TMSC = GetMatrixScale(&initBone);
		if( (fabs(TMSC.x - 1.0f) > 0.002f) || (fabs(TMSC.y - 1.0f) > 0.002f) || (fabs(TMSC.z - 1.0f) > 0.002f) )
		{	//Matrix3 boneStretchTM = pBonNodes[pBonesHierarchTable[i]]->GetStretchTM(0);
			//initBone = boneStretchTM * initBone;
			TMSC = GetMatrixScale(&initBone);
			TMSC.x = 1.0f / TMSC.x; TMSC.y = 1.0f / TMSC.y; TMSC.z = 1.0f / TMSC.z;
			initBone.PreScale(TMSC, FALSE);
		}
		RebuildMatrix(initBone);//1 marta rebuild qilingan TM quat b-n 1 necha marta aylantirilsayam to'g'ri chiqadi;
		//if(TMNegParity(initBone))//Mirror qilingan bo'lsaya:
		//	RebuildMatrix(initBone);
		r = FlipYZAxisInMatrix(initBone);//ConvToD3DXMATRIX(&initBone);
		r._41 *= sMyData.fMassh; r._42 *= sMyData.fMassh; r._43 *= sMyData.fMassh;
		fwrite(&r, 16*sizeof(float), 1, streamBin);
		Progr(60,70,(float)i/(int)numBones,"skin", ip);
	}

 	for(WORD i = 0; i < wNumBones; i++)//parents:
	{	__int16 iPrntIer=-1;
		INode *parNode = pBonNodes[pBonesHierarchTable[i]]->GetParentNode();
		if(parNode)
		{	boneName = parNode->GetName();
			if(strcmp(boneName,"Scene Root"))
			{	for(WORD ii = 0; ii < wNumBones; ii++)
				{	if(ii!=i)
					{	if(!strcmp(boneName,pBonNodes[pBonesHierarchTable[ii]]->GetName()))
						{	iPrntIer= ii;
							break;
		}	}	}	}	}
		fwrite(&iPrntIer, 2, 1, streamBin);
	}
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
    fpos_t posPre; char st;
	fgetpos( streamBin, &posPre ); 
	fwrite( &st, sizeof( st ), 1, streamBin    );// bone pointerga ko'rsatish uchun 
	fwrite( &st, sizeof( st ), 1, streamBin    );// WORD ptr joy tashlab ketaman. 
	fwrite( &st, sizeof( st ), 1, streamBin    );// bone pointerga ko'rsatish uchun 
	fwrite( &st, sizeof( st ), 1, streamBin    );// WORD ptr joy tashlab ketaman. 
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
		fwrite( &wNumAssigBones,sizeof(wNumAssigBones),1,streamBin);
		
		float sumWeight; sumWeight = 0.f;
		for(k = 0; k < sk_verts[i].numAssigBones; k++)
		{	sk_verts[i].pWeightTable[k].whichBone = 
						skincontext->GetAssignedBone(VerOldTabl[i], k);
			//Footsteps uchun o'zgartirish ****************************
			if(bFootSteps)
			{	if(iFootStepsBonNum < sk_verts[i].pWeightTable[k].whichBone)
					sk_verts[i].pWeightTable[k].whichBone --;
			}

			sk_verts[i].pWeightTable[k].weightForBone = 
						skincontext->GetBoneWeight(VerOldTabl[i], k);
			sumWeight += sk_verts[i].pWeightTable[k].weightForBone;

			WORD wWhichBone;
 			wWhichBone = GetBonPosInNewHierarchTable(sk_verts[i].pWeightTable[k].whichBone);
			
			fwrite( &wWhichBone,sizeof(WORD),1,streamBin);
			fwrite( &sk_verts[i].pWeightTable[k].weightForBone,sizeof(sk_verts[i].pWeightTable[k].weightForBone),1,streamBin);
		}
		if(sk_verts[i].numAssigBones < 1) 
		{	char s[12];wsprintf(s,"%d",i);
			MessageBox(NULL," chi vertex bonesiz qolib ketgan!",s,MB_OK);
		}
		if( (k > 0) && (sumWeight < 0.9999f) )
		{	char s[12];wsprintf(s,"%d",i);
			MessageBox(NULL," chi vertex weighti 1dan kichik!",s,MB_OK);
		}
		Progr(70,80,(float)i/(int)NumNewVert,"skin", ip); 
		if(maxAssignedBone < (BYTE)sk_verts[i].numAssigBones)
			maxAssignedBone = (BYTE)sk_verts[i].numAssigBones;
	}
//***********************************************************
//***********************************************************
//***********************************************************
//***********************************************************
    fpos_t posSle;
	fgetpos( streamBin, &posSle ); 

	fwrite( &maxAssignedBone, sizeof(BYTE), 1, streamBin );

	fseek( streamBin, (long)posPre, SEEK_SET );
	DWORD sz; sz = (DWORD)(posSle - posPre - 4);
 	fwrite( &sz, sizeof( DWORD ), 1, streamBin );

	fclose(streamBin); 
	if(sk_verts)
	{	for (DWORD i=0; i<NumNewVert; i++)//sekin ishlaydur;
			delete sk_verts[i].pWeightTable;
		delete sk_verts;	
	}

NotSkinContext:
	if(old_tv1_verts) delete old_tv1_verts;	
	if(VerOldTabl) delete VerOldTabl;
	if(pBonNodes) delete pBonNodes;
	if(pBonesParentTable) delete pBonesParentTable;
	if(pBonesHierarchTable) delete pBonesHierarchTable;
	if(keyArrays) delete keyArrays;

	if(needDel)tri->DeleteMe();
	char fullname[128]; sprintf(fullname, "%s\\%s%s", sMyData.szExport, nodeName, ".skn");
	//if(sMyData.bToText)
	// SoftSkinToText(fullname);
	ConvertSoftSkinToIndexedFormat(fullname, sMyData.fThreshold);
	if(sMyData.bDeleteSource)DeleteFile(fullname);
	sprintf(fullname, "%s\\%s%s", sMyData.szExport, nodeName, ".ski");
	//if(sMyData.bToText)
	//	IndexedSkinToText(fullname);
	IDirect3D9 *pD3D;LPDIRECT3DDEVICE9 pd3dDevice;
	ConvertIndexedSkinToSHFormat(fullname,sMyData.bTangent?(&pD3D):0,sMyData.bTangent?(&pd3dDevice):0);
	if(sMyData.bDeleteSource)DeleteFile(fullname);
	sprintf(fullname, "%s\\%s%s", sMyData.szExport, nodeName, ".sb4");
	if(sMyData.bToText && (!sMyData.bTangent))
		IndexedSHSkinToText(fullname,FALSE);
		
	if(sMyData.bTangent)
	{	ConvertIndexedShaderSkinToTBF3_UB4N_F16_2FormatUseDX(fullname,pD3D,pd3dDevice);
		if(sMyData.bDeleteSource)DeleteFile(fullname);
		sprintf(fullname, "%s\\%s%s", sMyData.szExport, nodeName, ".sb4t");
		if(sMyData.bToText)
			IndexedSHSkinToText(fullname,TRUE);
	}

	SkinModif->EnableMod();//meshni olayotganda skinni disable qilib qo'yamiz
	return;
}

int GetKeysArray(Interface *ip)
{
int t,KeyNum;

	int tpf = GetTicksPerFrame();
	int s   = ip->GetAnimRange().Start()/tpf; 
	int e   = ip->GetAnimRange().End()/tpf;

	t   = e - s + 1;

	keyArrays = new BOOL [t];
	for(int k = 0; k < t; k++) keyArrays[k] = FALSE;


	for(int b = 0; b < wNumBones; b++)
	{	c = pBonNodes[pBonesHierarchTable[b]]->GetTMController();

		vertAn = c->SubAnim(0); // vertical
		horAn  = c->SubAnim(1); // horizontal
		rotAn  = c->SubAnim(2); // rotation

		if(!c)
		{	fclose(streamBin); 
			return -1;
		}

		int keys = 0;
		KeyNum = 0;
		TimeValue i;
		for (int f = s; f <= e; f++) 
		{	i = f*tpf;
			if( (c->IsKeyAtTime(i,KEYAT_ROTATION)) ||
				(c->IsKeyAtTime(i,KEYAT_POSITION)) ||
				(vertAn && ( (vertAn->IsKeyAtTime(i,KEYAT_ROTATION)) || (vertAn->IsKeyAtTime(i,KEYAT_POSITION)))) ||
				(horAn && ( (horAn->IsKeyAtTime(i,KEYAT_ROTATION)) || (horAn->IsKeyAtTime(i,KEYAT_POSITION))) ) ||
				(rotAn && ( (rotAn->IsKeyAtTime(i,KEYAT_ROTATION)) || (rotAn->IsKeyAtTime(i,KEYAT_POSITION))) ) )
			{	keyArrays[keys] = TRUE;
				KeyNum ++;
			}
			keys++;
	}	}
	KeyNum = 0;
	for(int k = 0; k < t; k++)
	{	if(keyArrays[k])
			KeyNum++;
	}
	return KeyNum;//bu proc arrayni new qilib yaratdi,delete qilishni unutma;
}

int GetBipKeysArray(Interface *ip)
{
int t,KeyNum;

	int s   = ip->GetAnimRange().Start(); 
	int e   = ip->GetAnimRange().End();

	t   = e - s + 1;

	keyArrays = new BOOL [t];
	for(int k = 0; k < t; k++) keyArrays[k] = FALSE;

	
	INode   *FSnode = GetCOREInterface()->GetINodeByName("Footsteps");
	Control *FSc=0;
   	if(FSnode)
		FSc = FSnode->GetTMController();


	for(int b = 0; b < wNumBones; b++)
	{	c = pBonNodes[pBonesHierarchTable[b]]->GetTMController();
		if(!c)
		{	fclose(streamBin); 
			return -1;
		}

		int keys = 0;
		KeyNum = 0;
		TimeValue i;
		for (int f = s; f <= e; f++) 
		{	//i = f*tpf;
			i = f;//bunga olib tashlaymiz stepini
			if(FSnode) 
			{	if( FSc->IsKeyAtTime(i,KEYAT_ROTATION) ||
				    FSc->IsKeyAtTime(i,KEYAT_POSITION) )
				keyArrays[keys] = TRUE;
			}
			if( c->IsKeyAtTime(i,KEYAT_ROTATION) ||
			    c->IsKeyAtTime(i,KEYAT_POSITION) )
				keyArrays[keys] = TRUE;
			if(keyArrays[keys])
				KeyNum ++;
			keys++;
	}	}
	KeyNum = 0;
	for(int k = 0; k < t; k++)
	{	if(keyArrays[k])
			KeyNum++;
	}
	return KeyNum;//bu proc arrayni new qilib yaratdi,delete qilishni unutma;
}

VOID AllSkin(LPVOID par)
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
			ExportOneSkin(ip,iu,k);
			streamBin = 0;
		}
		__except(1,1)
		{
			MessageBox(NULL, "Kutilmagan gen. xato", "Diqqat!!!", MB_OK);
			if(streamBin) fclose(streamBin);
	}	}
	return;
}
