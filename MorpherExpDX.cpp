#include "ExpDX.h"
//#include "modstack.h"
//#include "iunwrap.h"
//#include "animtbl.h"
//#include "decomp.h"
#include "../../../Libs/Math/mMathFn.h"
//#include "maxheap.h"
//#include "IGame.h"
//#include "IGameControl.h"
//#include "IGameMaterial.h"
//#include "IGameObject.h"
//#include "IGameStd.h"
//#include "IGameModifier.h" 
//#if !defined _HYBRID
//#error Compiled with no Hybrid configuration.
//#endif



Modifier			*modif;
MorphR3				*morpher;
DWORD				dwBandChan;
Point3				MrphTr,MrphRt;
VERTS_T2F_N3F_V3F	*old_tv2_verts;
ObjectState			os;
size_t				mrphSz = 0;
float				*chanPrs;
extern int			tpf,s,e;
extern Matrix3		TM;
Matrix3				mInv;				





bool GetMorpherModifier(INode *node)
{
int modifier_index;
/*	IGameScene *gs = GetIGameInterface();
	gs->InitialiseIGame();
	IGameNode   * child = gs->GetIGameNode(node);
	IGameObject * obj = child->GetIGameObject();
	IGameModifier *m = obj->GetIGameModifier(i);
	IGameMorpher *mrp = (IGameMorpher*)m;
	modif = mrp->GetMaxModifier();
	if( MR3_CLASS_ID != modif->ClassID())return;
	else morpher = (MorphR3*)modif;
	if( morpher->chanBank[i].mActive )return;
*/	Object* object = node->GetObjectRef();
	if(!object) return false;
	while(object->SuperClassID() == GEN_DERIVOB_CLASS_ID && object)
	{	IDerivedObject *derivedobject = static_cast<IDerivedObject*>(object);
		int modcount = 0;
		while(modcount < derivedobject->NumModifiers())
		{	modif = derivedobject->GetModifier(modcount);
			if (modif->ClassID() == MR3_CLASS_ID)
			{	morpher = (MorphR3*)modif;
				modifier_index = modcount;				
				__try
				{
					mrphSz = morpher->chanBank.size();
				}
				__except(1,1)
				{
					MessageBox(NULL, "morpher->chanBank.size()", "FAILED", MB_OK);
					return false;
				}
				return true;
			}	
			modcount++;
		}	
		object = derivedobject->GetObjRef();
	}
	return false;
}

static VOID ExportOneMorphAnim(Interface *ip)
{
	tpf = GetTicksPerFrame();
	s = ip->GetAnimRange().Start()/tpf; 
	e = ip->GetAnimRange().End()/tpf;

	if(!openfile(sMyData.szExport, nodeName, ".mra", &streamBin  ,"wb"))
	{	char fullname[128]; sprintf(fullname, "%s\\%s%s", sMyData.szExport, nodeName, ".mra");
		MessageBox(NULL, fullname, "Faylini ochishda xato.", MB_OK);
		return;
	}

	char sh[20] = "Mra................";
	fwrite(sh, 1, 20, streamBin );

	if(0.0f == sMyData.dete)//Aks holda dete editdan olib hisoblangan:
		sMyData.dete = 1000.0f * 40.0f * (float)(e-s) / tpf;//25 frame / sec. dan

	fwrite( &sMyData.dete, sizeof( sMyData.dete ), 1, streamBin );

	fwrite(&dwBandChan, sizeof( dwBandChan ), 1, streamBin );
//******************  Indexlarni yozamiz *************************************
 	for(unsigned i=0;i<mrphSz;i++)
	{	morphChannel *channel = &(morpher->chanBank[i]);
		if (!channel || !channel->mActive)// || !channel->mActiveOverride || !channel->cblock )
			continue;

		Control *mcont = channel->cblock->GetController(0);
			
		__int16 nk = mcont->NumKeys();

		fwrite( &nk, sizeof( __int16 ), 1, streamBin );

		for (int k = 0; k < nk; k++) 
		{	TimeValue t = mcont->GetKeyTime(k);
			float f = (float)t / tpf;
			//************* avval percentni yozamiz *************************************
			float PerCent = (float)f / (e);//shkala % ti
			float fChannelPercent;
			channel->cblock->GetValue(0, t, fChannelPercent, FOREVER);
			fChannelPercent *= 0.01f;

			fwrite( &PerCent, sizeof( float ), 1, streamBin );
			fwrite( &fChannelPercent, sizeof( float ), 1, streamBin );
		}
		Progr(90,100,(float)i/(float)100.f,"Morph anim.:", ip); 
	}
	fclose(streamBin); 
	if(sMyData.bToText)
	{	char fullname[128]; sprintf(fullname, "%s\\%s%s", sMyData.szExport, nodeName,".mra");
		MorphAnimToText(fullname);
	}
	return;
}

static VOID WriteChangedVerts()
{
DWORD	faceModNum = 0, i, ii;
fpos_t	posFaceModNum;

	fgetpos(streamBin, &posFaceModNum);
	fwrite(&faceModNum, sizeof(faceModNum), 1, streamBin);

	//1- qancha vertlar o'zgarganligini hisoblaymiz
	//feysga qarab to'g'rilayman:
	//for(i=0; i<NumNewVert; i++)
	for(i=0; i<numFaces; i++)
	{	if( old_tv2_verts[i*3  ].p.x  != old_tv1_verts[i*3  ].p.x ||
			old_tv2_verts[i*3  ].p.y  != old_tv1_verts[i*3  ].p.y ||
			old_tv2_verts[i*3  ].p.z  != old_tv1_verts[i*3  ].p.z ||

			old_tv2_verts[i*3+1].p.x  != old_tv1_verts[i*3+1].p.x ||
			old_tv2_verts[i*3+1].p.y  != old_tv1_verts[i*3+1].p.y ||
			old_tv2_verts[i*3+1].p.z  != old_tv1_verts[i*3+1].p.z ||
			
			old_tv2_verts[i*3+2].p.x  != old_tv1_verts[i*3+2].p.x ||
			old_tv2_verts[i*3+2].p.y  != old_tv1_verts[i*3+2].p.y ||
			old_tv2_verts[i*3+2].p.z  != old_tv1_verts[i*3+2].p.z )
		{
			fwrite(&i, sMyData.b32bit?4:2, 1, streamBin);
			
			float TM[6];

			for(int v=0; v<3; v++)
			{	ii = i*3+v;
				TM[0] = old_tv2_verts[ii].p.x - old_tv1_verts[ii].p.x;
				TM[1] = old_tv2_verts[ii].p.z - old_tv1_verts[ii].p.z;
				TM[2] = old_tv2_verts[ii].p.y - old_tv1_verts[ii].p.y;
				TM[3] = old_tv2_verts[ii].n.x - old_tv1_verts[ii].n.x;
				TM[4] = old_tv2_verts[ii].n.z - old_tv1_verts[ii].n.z;
				TM[5] = old_tv2_verts[ii].n.y - old_tv1_verts[ii].n.y;			
				TM[0] *= sMyData.fMassh;
				TM[1] *= sMyData.fMassh;
				TM[2] *= sMyData.fMassh;
				fwrite(TM, sizeof(float), 6, streamBin);	
			}
			faceModNum++;
	}	}

	fseek(streamBin, (long)posFaceModNum, SEEK_SET);
	fwrite(&faceModNum, 4, 1, streamBin);
	fseek(streamBin, 0, SEEK_END);
	return;
}

static VOID CalcNewMorphVerts(Interface *ip)
{
	for (DWORD i=0; i<numFaces; i++) 
	{	for (int k=0; k<3; k++) 
		{	ind[k] = (WORD)mesh->faces[i].v[vx[k]];
			t_ind[k] = bFlatten ?	(WORD)mesh->mapFaces(flatMapChan)[i].t[vx[k]] :
									(WORD)mesh->tvFace[i].t[vx[k]];
			Point3 v;
			v = mInv * mesh->verts[ind[k]];

			errStep=3;
			Point3 vertexNormal; if(!GetVertexNormal(mesh, i, mesh->getRVertPtr(ind[k]), &vertexNormal)){fclose(streamBin);return;}
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

		//Vertexlarga ham shuni yozib qo'ylik:
		//old_tv2_verts[i * 3    ].n = old_faces[i].n;
		//old_tv2_verts[i * 3 + 1].n = old_faces[i].n;
		//old_tv2_verts[i * 3 + 2].n = old_faces[i].n;

		Progr(80, 100, (float)i/(int)numFaces,"CalcNewmorphVerts: ", ip);
	}
	//SmoothNormalsWithWeightingByFaceAngle2();
	return;
}

VOID ZeroAllChannels(Interface* ip)
{
float nulPrs = 0.0f;
 	for(unsigned i=0; i<mrphSz; i++)
	{	morphChannel *channel = &(morpher->chanBank[i]);
		if(!channel || !channel->mActive || !channel->mActiveOverride || !channel->cblock)
			continue;
		channel->cblock->SetValue(0, ip->GetTime(), nulPrs);
	}
	return;
}

VOID SaveAllChannels(Interface* ip)
{
	dwBandChan = 0;
	chanPrs = new float [mrphSz];
 	for(unsigned i=0;i<mrphSz;i++)
	{	morphChannel *channel = &(morpher->chanBank[i]);
		if(!channel || !channel->mActive || !channel->mActiveOverride || !channel->cblock)
			continue;
		channel->cblock->GetValue(0, ip->GetTime(), chanPrs[i], FOREVER);
		dwBandChan++;
	}
	return;
}

VOID RestoreAllChannels(Interface* ip)
{
	dwBandChan = 0;
	chanPrs = new float [mrphSz];
 	for(unsigned i=0;i<mrphSz;i++)
	{	morphChannel *channel = &(morpher->chanBank[i]);
		if(!channel || !channel->mActive || !channel->mActiveOverride || !channel->cblock)
			continue;
		channel->cblock->SetValue(0, ip->GetTime(), chanPrs[i]);
		dwBandChan++;
	}
	return;
}

static VOID ExportOneMorph(Interface *ip,IUtil *iu,int sel_num)
{
TSTR		indent;				TriObject	*tri;				
BOOL		negScale, bClone;	Point3 rots,scs;
float				XFMIN = 3.402823466e+38F, XFMAX = -3.402823466e+38F, 
					YFMIN = 3.402823466e+38F, YFMAX = -3.402823466e+38F, 
					ZFMIN = 3.402823466e+38F, ZFMAX = -3.402823466e+38F; 
#define calcMaxMin	if(XFMIN > v[0]) XFMIN = v[0];\
					if(XFMAX < v[0]) XFMAX = v[0];\
					if(YFMIN > v[1]) YFMIN = v[1];\
					if(YFMAX < v[1]) YFMAX = v[1];\
					if(ZFMIN > v[2]) ZFMIN = v[2];\
					if(ZFMAX < v[2]) ZFMAX = v[2];

	node = ip->GetSelNode(sel_num);//node ni olamiz
	nodeName = node->GetName();//node ni nomini olamiz
//********* Agar CLone bo'lsa ***********************************************
	if(strstr(nodeName, "Clone")) bClone = TRUE;
	else bClone = FALSE;
//******* Birinchi NumAnimKeys(pos , rot , scal) ni topamiz *****************
	morpher = 0;
	if(!GetMorpherModifier(node))
		return;
	if(!morpher)
	{	MessageBox(NULL,"Morpherni topolmadim.","Kechirasiz, chiqib ketaman!!!",MB_OK);
		return;
	}
	SaveAllChannels(ip);
	ZeroAllChannels(ip);
//***************************************************************************
    Matrix3 TM = node->GetNodeTM(ip->GetTime()) * Inverse(node->GetParentTM(ip->GetTime()));
	decomp_affine(TM, &ap);
	negScale = TMNegParity(node->GetObjTMAfterWSM(ip->GetTime()));

    if (negScale) 
	{	vx[0] = 0;vx[1] = 1;vx[2] = 2;	}
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
	if(!openfile(sMyData.szExport, nodeName, bClone ? ".clmrph" : ".mrp", &streamBin  ,"wb"))
	{	char fullname[128]; sprintf(fullname, "%s\\%s%s", sMyData.szExport, nodeName, bClone ? ".clmrph" : ".mrp");
		MessageBox(NULL, fullname, "Faylini ochishda xato.", MB_OK);
		return;
	}
	char sh[20] = "Mrp16sft...........";
	if(sMyData.b32bit){sh[3]='3'; sh[4]='2';}
	fwrite(sh, 1, 20, streamBin );
//*********************************************************************************
	float mposx,mposy,mposz;
	mposx = ap.t.x * sMyData.fMassh;
	mposy = ap.t.y * sMyData.fMassh;
	mposz = ap.t.z * sMyData.fMassh;

	fwrite( &mposx   , sizeof( mposx )   , 1, streamBin );
	fwrite( &mposz   , sizeof( mposz )   , 1, streamBin );
	fwrite( &mposy   , sizeof( mposy )   , 1, streamBin );
	Point3 fRot = GetFlippedRot(&TM);
	fwrite( &fRot.x, sizeof( fRot.x ), 1, streamBin );
	fwrite( &fRot.y, sizeof( fRot.y ), 1, streamBin );
	fwrite( &fRot.z, sizeof( fRot.z ), 1, streamBin );
	MrphTr = ap.t; MrphRt = fRot;//animni exp qilishda kerak bo'ladi
	if(bClone) goto Fin;
//***************   FACE lar sonini yozamiz **********************************
	fwrite(&numFaces, sizeof(DWORD), 1, streamBin);
//******************  1 - hammasini 1 joyga to'playmiz *************************************
	char s[256];
	wsprintf(s,"%s%s",nodeName," malumotlarni yig'ish");
	old_tv1_verts = new VERTS_T2F_N3F_V3F [numFaces * 3];
	old_faces = new FACES_NORMAL_AND_SMOOTH_GROUPE[numFaces];//Smoothing groupe uchun

	mInv = node->GetObjTMAfterWSM(ip->GetTime()) * GetInvTransform(ap.t, ap.q);

	for (DWORD i=0; i<numFaces; i++) 
	{	for (int k=0; k<3; k++) 
		{	ind[k] = (WORD)mesh->faces[i].v[vx[k]];
			t_ind[k] = bFlatten ?	(WORD)mesh->mapFaces(flatMapChan)[i].t[vx[k]] :
									(WORD)mesh->tvFace[i].t[vx[k]];
			Point3 v = mesh->verts[ind[k]] * mInv;
			calcMaxMin

			errStep=3;
			Point3 vertexNormal; if(!GetVertexNormal(mesh, i, mesh->getRVertPtr(ind[k]), &vertexNormal)){fclose(streamBin);return;}
			errStep=4;
			
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
		//pv0 = Normalize(pv0);
		//pv1 = Normalize(pv1);

		//Feys normali:
		//old_faces[i].n = CrossProd(pv1, pv0);//Soat miliga qarshi yo'n. bo'yicha cross.
		//old_faces[i].n = Normalize(old_faces[i].n);

		//Vertexlarga ham shuni yozib qo'ylik:
		//old_tv1_verts[i * 3    ].n = old_faces[i].n;
		//old_tv1_verts[i * 3 + 1].n = old_faces[i].n;
		//old_tv1_verts[i * 3 + 2].n = old_faces[i].n;

		Progr(40, 60,(float)i/(int)numFaces,"Morph vertices: ", ip); 
	}

	//SmoothNormalsWithWeightingByFaceAngle();

	float mXFMIN,mXFMAX,mYFMIN,mYFMAX,mZFMIN,mZFMAX;
	mXFMIN = XFMIN * sMyData.fMassh;
	mXFMAX = XFMAX * sMyData.fMassh;
	mYFMIN = YFMIN * sMyData.fMassh;
	mYFMAX = YFMAX * sMyData.fMassh;
	mZFMIN = ZFMIN * sMyData.fMassh;
	mZFMAX = ZFMAX * sMyData.fMassh;

	fwrite(&mXFMIN, sizeof(mXFMIN), 1, streamBin);
	fwrite(&mXFMAX, sizeof(mXFMAX), 1, streamBin);
	fwrite(&mZFMIN, sizeof(mZFMIN), 1, streamBin);
	fwrite(&mZFMAX, sizeof(mZFMAX), 1, streamBin);
	fwrite(&mYFMIN, sizeof(mYFMIN), 1, streamBin);
	fwrite(&mYFMAX, sizeof(mYFMAX), 1, streamBin);
	
	// Taxminan massivi shuncha beraman;
	NumNewVert = numFaces * 3;	
	int vts; vts = (int)numVerts;
	for(DWORD i=0;i<NumNewVert;i++)//yangi vertexlar **********************************
	{	//DirectXga OpenGL dan farqini kiritamiz:
		Point3 mpos;
		mpos = old_tv1_verts[i].p * sMyData.fMassh;

		fwrite( &mpos.x   , sizeof( mpos.x )   , 1, streamBin );
		fwrite( &mpos.z   , sizeof( mpos.z )   , 1, streamBin );
		fwrite( &mpos.y   , sizeof( mpos.y )   , 1, streamBin );
		fwrite( &old_tv1_verts[i].n.x, sizeof( old_tv1_verts[i].n.x ), 1, streamBin );
		fwrite( &old_tv1_verts[i].n.z, sizeof( old_tv1_verts[i].n.z ), 1, streamBin );
		fwrite( &old_tv1_verts[i].n.y, sizeof( old_tv1_verts[i].n.y ), 1, streamBin );

		old_tv1_verts[i].tu = 1.f - old_tv1_verts[i].tu;
		fwrite( &old_tv1_verts[i].tv, sizeof( old_tv1_verts[i].tv ), 1, streamBin );
		fwrite( &old_tv1_verts[i].tu, sizeof( old_tv1_verts[i].tu ), 1, streamBin );

		Progr(60,80,(float)i/(int)NumNewVert,"Write morph vertices: ", ip); 
	}
//********************  MORPHER   ****************************
//********************  MORPHER   ****************************
//********************  MORPHER   ****************************
//********************  MORPHER   ****************************
//********************  MORPHER   ****************************
//********************  MORPHER   ****************************
//********************  MORPHER   ****************************
//********************  MORPHER   ****************************

	//meshni olib bo'ldik, endi morphni enable qilib qo'ysak ham bo'laveradi

//******************  Band channellar sonini hisoblaymiz ******************
	//morpher->EnableMod();
	if(!dwBandChan) MessageBox(NULL, "Danger: Exporting skin2morph process corrapted!!!", "No channels founded.", MB_OK);
	else
	{	wsprintf(s, "%d", dwBandChan);
		fwrite(&dwBandChan ,sizeof(dwBandChan),1,streamBin);
//************************************************************************
		old_tv2_verts = new VERTS_T2F_N3F_V3F[NumNewVert];
 		for(DECL(unsigned) i=0; i<mrphSz; i++)
		{	if(needDel)tri->DeleteMe();
			morphChannel *channel = &(morpher->chanBank[i]);
			if(!channel || !channel->mActive || !channel->mActiveOverride || !channel->cblock)
				continue; //hech bo'lmasa birorta 0 dan farqli perc bormi
			channel->cblock->SetValue(0, ip->GetTime(), 100.0f);

			tri = GetTriObjectFromNode(node, ip->GetTime(), &needDel);
			if (!tri){MessageBox(NULL,"Obyektni editmesh qilolmayman!","Diqqat",MB_OK);return;}
			mesh = &tri->GetMesh();
			mesh->buildNormals();

	 		CalcNewMorphVerts(ip);
			WriteChangedVerts();

			channel->cblock->SetValue(0, ip->GetTime(), 0.0f);
			Progr(80,100,(float)i/(int)mrphSz,"Used morph vertices: ", ip); 
		}	
		delete [] old_tv2_verts;
	}
//************************************************************************
	if(old_tv1_verts)delete [] old_tv1_verts;	
	fclose(streamBin); 
	streamBin = 0; 
	delete [] old_faces;
//************************************************************************
Fin:
	if(needDel) tri->DeleteMe();
	RestoreAllChannels(ip);
	delete [] chanPrs;

	char fullname[128]; sprintf(fullname, "%s\\%s%s", sMyData.szExport, nodeName, bClone ? ".clmrm" : ".mrp");
	if(sMyData.bInIndexed)
	{	ConvertSoftMorphToIndexedFormat(fullname, sMyData.fThreshold);
		if(sMyData.bDeleteSource)
			DeleteFile(fullname);
		if(sMyData.bToText)
		{	sprintf(fullname, "%s\\%s%s", sMyData.szExport, nodeName, bClone ? ".clmri" : ".mri");
			IndexedMorphToText(fullname);
	}	}
	else if(sMyData.bToText) SoftMorphToText(fullname);
	return;
}

VOID AllMorph(LPVOID par)
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
			ExportOneMorph(ip,iu,k);
			streamBin = 0;
		}
		__except(1,1)
		{	MessageBox(NULL, "Activate and set in 100% all channels.", "Warn. Engine doesn't work!!!!!!", MB_OK);
			if(streamBin) fclose(streamBin);
	}	}
	return;
}

VOID AllMorphAnim(LPVOID par)
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
		{	streamBin = 0;
			node = ip->GetSelNode(k);//node ni olamiz
			nodeName = node->GetName();//node ni nomini olamiz
			if(!GetMorpherModifier(node)) return;
			if(!morpher)
			{	MessageBox(NULL,"Morpherni topolmadim.","Kechirasiz, chiqib ketaman!!!",MB_OK);
				return;
			}
			SaveAllChannels(ip);//dwChanNum ni hisiblasun;
			ExportOneMorphAnim(ip);
			streamBin = 0;
		}
		__except(1,1)
		{	MessageBox(NULL, "Activate and set in 100% all channels.", "Warn. Engine doesn't work!!!!!!", MB_OK);
			if(streamBin) fclose(streamBin); 
	}	}
	return;
}
