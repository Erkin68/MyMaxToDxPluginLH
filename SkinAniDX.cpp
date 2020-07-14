#include "ExpDX.h"
#include "modstack.h"
#include "iunwrap.h"
#include "animtbl.h"
#include "decomp.h"
#include "iskin.h"
#include "d3dx9.h"
#include "../../../Libs/Math/mMathFn.h"
#include "../../../Game SDK/Main/typesWin32.h"
#include "C:/Program Files (x86)/Autodesk/3ds Max 2010 SDK/maxsdk/samples/controllers/reactor/reactapi.h"

extern Matrix3 TM;


Quat setLH(Matrix3*);
VOID ExportOneBpQuats(Interface*);
BOOL bFootSteps ;


int GetBipParentBoneNum(INode* node)
{
int		i;
	for(i = 0; i < numBones; i++)
	{	boneName = pBonNodes[i]->GetName();//node ni nomini olamiz
		if(node == pBonNodes[i])
			return i;
	}
	return -1;
}


VOID RecGetNumBone(INode *chb)
{
 	int nch = chb->NumberOfChildren();
	for(int j = 0; j < nch; j ++)
	{
		boneName = chb->GetChildNode(j)->GetName();
		if(strstr(boneName,"Nub") || strstr(boneName,"nub"))
			continue;
		if(strstr(boneName,"Footsteps"))
		{	bFootSteps       = TRUE;
			continue;
		}
		numBones ++;
		RecGetNumBone(chb->GetChildNode(j));
	}
	return ;
}

VOID RecZapGetNumBone(INode *chb)
{
	int nch = chb->NumberOfChildren();
	for(int j = 0; j < nch; j ++)
	{
		boneName = chb->GetChildNode(j)->GetName();
		if(strstr(boneName,"Nub") || strstr(boneName,"nub"))
			continue;
		if(strstr(boneName,"Footsteps"))
		{
			bFootSteps       = TRUE;
			continue;
		}
		pBonNodes[numBones++] = chb->GetChildNode(j);
		RecZapGetNumBone(chb->GetChildNode(j));
	}
	return ;
}



VOID ExportOneSkinAni(Interface *ip,IUtil *iu,int sel_num)
{
	pBonNodes = 0;pBonesParentTable =0; 
	node = ip->GetSelNode(sel_num);//node ni olamiz
	nodeName = node->GetName();
    Control *c = node->GetTMController();
    //if( !((c->ClassID() == BIPSLAVE_CONTROL_CLASS_ID) ||
	//	(c->ClassID() == BIPBODY_CONTROL_CLASS_ID) ) )
    //{	MessageBox(NULL,"Bipedni topolmadim.","Kechirasiz, chiqib ketaman!!!",MB_OK);
	//	return;
	//}

	numBones = 1;
	bFootSteps = FALSE;
	RecGetNumBone(node);

	if(numBones == 1)
    {	MessageBox(NULL,"Bipening asosi yoq.","Kechirasiz, chiqib ketaman!!!",MB_OK);
		return;
	}

	pBonNodes = 0; pBonesParentTable = 0; pBonesHierarchTable = 0;
	pBonNodes         = new INode *  [numBones];
	pBonesParentTable = new int      [numBones];
	pBonesHierarchTable= new int      [numBones];
	//Footstepini hali chiqarganimcha yo'q

	pBonNodes[0] = node;

	numBones = 1;
	bFootSteps = FALSE;
	RecZapGetNumBone(node);

	//Alfavit bo'yicha to'g'rilash,Skinnikiga to'g'ri kelmagani uchun
	int   *nAlphaBet = new int [numBones];
	INode **alpNodes = new INode * [numBones];
	int engKich  = 100;
	int engKatta = -100;
	for(WORD i = 0; i < numBones; i ++)
	{	LPCTSTR lpStr1,lpStr2;
		nAlphaBet [i] = 0;
		lpStr1 = pBonNodes[i]->GetName();
		for(int ii = 0; ii < numBones; ii ++)
		{	if(i != ii)
			{	lpStr2 = pBonNodes [ii] ->GetName();
				int res = lstrcmp(lpStr1, lpStr2);
				if(res > 0)	nAlphaBet [i] ++;
				if(res < 0)	nAlphaBet [i] --;
		}	}
		if(engKich > nAlphaBet [i])	engKich = nAlphaBet [i];
		if(engKatta < nAlphaBet [i])engKatta = nAlphaBet [i];
	}
	
	int count = 0;
	while(count < numBones)
	{	for(WORD i = 0; i < numBones; i ++)
		{	if(engKich == nAlphaBet [i])
				alpNodes [count ++] = pBonNodes [i];
		}
		engKich ++;
	}
	for(WORD i = 0; i < numBones; i++)
		pBonNodes[i] = alpNodes[i];
	delete [] nAlphaBet;
	delete [] alpNodes;
	
	for(WORD i = 0; i < numBones; i++)
	{	INode* parNode        = pBonNodes[i]->GetParentNode();
		pBonesParentTable[i ] = GetBipParentBoneNum(parNode);
	}
//********** iyerarxiyasi bo'yicha tartib bilan qo'yib chiqishim kerak;
	int *pBonesParentCounts = new int [numBones];
	int stepCount = 0;
	for(WORD i = 0; i < numBones; i++)
	{	pBonesParentCounts[i] =	GetParentCounts(pBonNodes[i]);
		if(pBonesParentCounts[i] > stepCount)
			stepCount = pBonesParentCounts[i];
	}
	int countIyer = 0;
	for(int k = 0; k <= stepCount; k++)
	{	for(WORD i = 0; i < numBones; i++)
		{	if(pBonesParentCounts[i] == k)
				pBonesHierarchTable[countIyer++] = i;
	}	}

	int s = ip->GetAnimRange().Start();
	TM = pBonNodes[pBonesHierarchTable[0]]->GetNodeTM(s);
	if(TMNegParity(TM))TM.Scale(Point3(-1.0f, -1.0f, -1.0f), FALSE);

	delete [] pBonesParentCounts;

	ExportOneBpQuats(ip);

	if(pBonNodes)delete [] pBonNodes;
	if(pBonesParentTable)delete [] pBonesParentTable;
	if(pBonesHierarchTable)delete [] pBonesHierarchTable;
	if(keyArrays)delete [] keyArrays;
	return;
}

int GetBpKeysArray(Interface *ip)
{
int t,KeyNum;

	int s = ip->GetAnimRange().Start(); 
	int e = ip->GetAnimRange().End();

	t = e - s + 1;

	keyArrays = new BOOL [t];
	for(int k = 0; k < t; k++) keyArrays[k] = FALSE;

	
	INode   *Bipe01node = pBonNodes[pBonesHierarchTable[0]];
	Control *B01c=0;
   	if(Bipe01node)
		B01c = Bipe01node->GetTMController();


	for(int b = 0; b < numBones; b++)
	{	Control *c = pBonNodes[pBonesHierarchTable[b]]->GetTMController();
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
			if(Bipe01node) 
			{	//shu yerga ozgina dopolneniye, boshi va oxirini qo'shamiz:
				if( B01c->IsKeyAtTime(i,KEYAT_ROTATION) ||
				    B01c->IsKeyAtTime(i,KEYAT_POSITION) ||
					(f == s) || (f == e) )
				keyArrays[keys] = TRUE;
			}
			if( c->IsKeyAtTime(i,KEYAT_ROTATION) ||
			    c->IsKeyAtTime(i,KEYAT_POSITION) ||
				(f == s) || (f == e) )
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

/*float *pmInitBoneTMs = NULL;
VOID InitBoneTMs()
{	
static char **pBoneNames  ;
static WORD *pwBonesFaces0;
static WORD *pwBonesFaces1;

	if(pmInitBoneTMs)
	{	free(pmInitBoneTMs);
		for(U16 b=0; b<wNumBones; b++) free(pBoneNames[b]);
		free(pBoneNames);
		free(pwBonesFaces0);
		free(pwBonesFaces1);
		pmInitBoneTMs = NULL;
	}

	OPENFILENAME	ofn;
	TCHAR			sfile[256];
	memset(&ofn, 0, sizeof (OPENFILENAME));
	memset(sfile,0, 256);
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.lpstrFilter = TEXT("All skin2morph files (*.smv)\0*.smv\0");  
	ofn.lpstrTitle  = TEXT("Open skinmorph file.");
	ofn.lpstrFile   = sfile;
	ofn.Flags       = OFN_EXPLORER | OFN_ALLOWMULTISELECT;
	ofn.nMaxFile    = 256;
	if(!GetOpenFileName(&ofn))
		return;
	FILE *f = fopen(sfile, "rb");
	if(!f) return;
//************************************************************************************
	BYTE type; fread(&type, 1, 1, f);
	Point3 p ; fread(p, 3, 4, f);
	Point3 e ; fread(e, 3, 4, f);
	DWORD dwNumFaces; fread(&dwNumFaces, 4, 1, f);
	DWORD dwNumFacesM; fread(&dwNumFacesM, 4, 1, f);
	DWORD dwNumVerts; fread(&dwNumVerts, 4, 1, f);
	DWORD dwNumVertsM; fread(&dwNumVertsM, 4, 1, f);
	float Sizes[6];  fread(Sizes, 4, 6, f);
//************************************************************************************
	WORD wNumBones; fread(&wNumBones, 2, 1, f);
	pBoneNames    = (char**)malloc(4 * wNumBones);//Pointer to LPSTR
	pwBonesFaces0 = (WORD* )malloc(2 * wNumBones);//In here as sknBones;
	pwBonesFaces1 = (WORD* )malloc(2 * wNumBones);//In here as sknBones;
	char boneName[256]; int l;
	for(U16 b=0; b<wNumBones; b++)
	{	fscanf(f, boneName);
		l = lstrlen(boneName);
		pBoneNames[b] = (char*)malloc(l+1);
		lstrcpy(pBoneNames[b], boneName);
	}
	fseek(f, 2 * wNumBones, SEEK_CUR);
	fseek(f, 2 * wNumBones, SEEK_CUR);
	Point4 MeshTM[4]; fread(MeshTM, 4, 16, f);
//************************************************************************************
	pmInitBoneTMs = (float*)malloc(4*16*wNumBones);
	for(S32 b=0; b<wNumBones; b++)
		fread(&pmInitBoneTMs[b*16], 4, 16, f);
	fclose(f);
	return;
}*/

VOID ExportOneBpQuats(Interface *ip)
{
WORD			iNumAnimKeys; 
TSTR			indent;
Matrix3			tmCurr;	
float			PerCent; 
int				s,e,tpf;

//Bone key TM larni skin InitBoneTM ga ko'paytirib olish uchun;
	/*if(!pmInitBoneTMs)
	{	InitBoneTMs();//MessageBox(NULL, "Do\'nt allocated skin file for the InitBoneTMs initializing!!!", "Error", MB_OK);
		//return;
	}*/

	nodeName = pBonNodes[pBonesHierarchTable[0]]->GetName();

	iNumAnimKeys = GetBpKeysArray(ip);
	if(iNumAnimKeys < 1)
	{	MessageBox(NULL, "Anim keylar yo'q!", "Ey", MB_OK);
		return ;
	}
//***************************************************************************
	if(!openfile(sMyData.szExport, nodeName, ".bpa", &streamBin  ,"wb"))
	{	char fullname[128]; sprintf(fullname, "%s\\%s%s", sMyData.szExport, nodeName,  ".bpa");
		MessageBox(NULL, fullname, "Faylini ochishda xato.", MB_OK);
		return;
	}

	char sh[20] = "Bpa................";
	fwrite(sh, 1, 20, streamBin );

	s = ip->GetAnimRange().Start(); 
	e = ip->GetAnimRange().End();
	tpf = GetTicksPerFrame();

	//Quyidagilar hozircha kerak emas,
	//chunki anim vaqtini hozircha o'zimiz qo'yamiza:
	//millisekundlarda:

	if(0.0f == sMyData.dete)//Aks holda dete editdan olib hisoblangan:
	{	int tpf = GetTicksPerFrame();
		sMyData.dete = 1000.0f * 40.0f * (float)(e-s) / tpf;//25 frame / sec. dan
	}

	fwrite( &sMyData.dete, sizeof( sMyData.dete ), 1, streamBin );

	WORD nb ; nb = (WORD)numBones;
	fwrite( &nb, sizeof( nb ), 1, streamBin );
	fwrite( &iNumAnimKeys, sizeof( iNumAnimKeys ), 1, streamBin );

	for(int b = 0; b < numBones; b++)
	{	boneName = pBonNodes[pBonesHierarchTable[b]]->GetName();//node ni nomini olamiz
		fwrite(boneName, lstrlen(boneName)+1, 1, streamBin);
	}


	//FILE *f=fopen("ExpBonesCrntTMs.txt", "w");
int flips[100],flipbone[100],iflip=0;

Matrix3 *mBones = new Matrix3 [numBones];
ZeroMemory(mBones, numBones * sizeof(Matrix3));
int  *oldBoneKeyTimes = new int [numBones];

//NEW!!! 09.06.2015: Checking for: Root-root bone must have all animation time scale(for Skin::CalcBonaAnimeTime shorting):
float *bipePercents = new float [2*iNumAnimKeys];
int crntBipAnim = 0;
//*************************************************************************************************************************

for(DECL(int) b = 0; b < numBones; b++) mBones[b].IdentityMatrix();
for(DECL(int) b = 0; b < numBones; b++)
{	boneName = pBonNodes[pBonesHierarchTable[b]]->GetName();//node ni nomini olamiz
//******* Birinchi NumAnimKeys(pos , rot , scal) ni topamiz *****************
	for (int ff = s; ff <= e; ff++) 
	{	if(keyArrays[ff-s])//2_07_05 da to'g'rilandi
		{	tmCurr = pBonNodes[pBonesHierarchTable[b]]->GetNodeTM(ff);//GetNodeTM GetObjectTM(ff); GetObjTMAfterWSM(ff) xato;
			Point3 TMSC = GetMatrixScale(&tmCurr);
			if( (fabs(TMSC.x - 1.0f) > 0.002f) || (fabs(TMSC.y - 1.0f) > 0.002f) || (fabs(TMSC.z - 1.0f) > 0.002f) )
			{//MessageBox(NULL,"Please, check anim. bone matrix scale.", pBonNodes[pBonesHierarchTable[b]]->GetName(),MB_OK);fclose(streamBin);return;}
				//Matrix3 boneStretchTM = pBonNodes[pBonesHierarchTable[b]]->GetStretchTM(0);
				//tmCurr = boneStretchTM * tmCurr;
				TMSC.x = 1.0f / TMSC.x; TMSC.y = 1.0f / TMSC.y; TMSC.z = 1.0f / TMSC.z;
				tmCurr.PreScale(TMSC, FALSE);
				//if(ff==s)
				//{	MRow *mr = tmCurr.GetAddr();
				//	fprintf(f, "\n     mCurrentTM: %.3f %.3f %.3f     %.3f %.3f %.3f     %.3f %.3f %.3f     %.3f %.3f %.3f", mr[0][0], mr[0][1], mr[0][2], mr[1][0], mr[1][1], mr[1][2], mr[2][0], mr[2][1], mr[2][2], mr[3][0], mr[3][1], mr[3][2]); 
			}
			RebuildMakeClosestQuat(tmCurr, &mBones[b]);//1 marta rebuild qilingan TM quat b-n 1 necha marta aylantirilsayam to'g'ri chiqadi;
			//if(TMNegParity(tmCurr)) RebuildMatrix(tmCurr);
			D3DXMATRIX rsum = FlipYZAxisInMatrix(tmCurr);
			rsum._41 *= sMyData.fMassh;	rsum._42 *= sMyData.fMassh; rsum._43 *= sMyData.fMassh;
    		PerCent = (float)(ff-s) / (e-s);
			if( (iNumAnimKeys == 0) && (PerCent != 0.f) ) MessageBox(NULL,"Boshi surilgan, to'g'rilang, iltimos !!!","Diqqat",MB_OK);
			fwrite(&PerCent , sizeof(PerCent), 1, streamBin);
			fwrite(&rsum, 16*sizeof(float), 1, streamBin);
			if(!mBones[b].IsIdentity())
			{	float angles[3];GetAngleFrVectors(&angles[0], tmCurr.GetRow(0), mBones[b].GetRow(0));
								GetAngleFrVectors(&angles[1], tmCurr.GetRow(1), mBones[b].GetRow(1));
								GetAngleFrVectors(&angles[2], tmCurr.GetRow(2), mBones[b].GetRow(2));
				if( (angles[0] > HALFPI || angles[1] > HALFPI || angles[2] > HALFPI))
				{	if(iflip<100)
					{	flips[iflip]=oldBoneKeyTimes[b];
						flipbone[iflip++] = pBonesHierarchTable[b];
			}	}	}
			mBones[b] = tmCurr; oldBoneKeyTimes[b] = ff/tpf;
			Progr(0,100,(float)b/(float)numBones,"skin", ip);
			//NEW!!! 09.06.2015:***************************************************************
			if(0==b)																		//*
			{if(crntBipAnim>2*iNumAnimKeys-1)												//*
			  MessageBox(NULL,"Memory overflow error for bipePercents var.","Err.",MB_OK);	//*
			 else																			//*
			  bipePercents[crntBipAnim++]=PerCent;											//*
			}else																			//*
			{int iBipAnimFounded=-1;														//*
			 for(int i=0; i<crntBipAnim; i++)												//*
			 {if(bipePercents[i]==PerCent)													//*
			  {iBipAnimFounded=i;															//*	
			   break;																		//*
			 }}																				//*
			 if(-1==iBipAnimFounded)														//*
			 {char s[128];sprintf(s,														//*
 	    "Root bone key assigned for bone %s not founded at time: %f per cent of full scale",//*
				pBonNodes[pBonesHierarchTable[b]]->GetName(),PerCent*100.f);					//*
			  MessageBox(NULL,"Err.",s,MB_OK);												//*
			}}//*******************************************************************************
}	}	} //fclose(f);
delete [] mBones;
delete [] oldBoneKeyTimes;
delete [] bipePercents;
fclose(streamBin);

if(iflip)
{	char s[2048]="";
	for(int i=0; i<iflip; i++)
	{	char st[128];
		if(i)
			sprintf(s, "\nbone: %s after key %d", pBonNodes[flipbone[i]]->GetName(), flips[i]);
		else
			sprintf(s, "bone: %s after key %d", pBonNodes[flipbone[i]]->GetName(), flips[i]);
		strcat(s,st);
	}
	MessageBox(NULL, s, "Found sign flip!!!", MB_OK);
}
if(sMyData.bToText)
{	char fullname[128]; sprintf(fullname, "%s\\%s%s", sMyData.szExport, nodeName, ".bpa");
	SkinAnimToText(fullname);
}
return;
}

VOID AllSkinAni(LPVOID par)
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
			ExportOneSkinAni(ip,iu,k);
			streamBin = 0;
		}
		__except(1,1)
		{
			MessageBox(NULL, "Kutilmagan gen. xato", "Diqqat!!!", MB_OK);
			if(streamBin) fclose(streamBin);
	}	}
	return;
}
