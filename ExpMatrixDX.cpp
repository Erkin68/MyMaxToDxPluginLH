#include "ExpDX.h"
#include "modstack.h"
#include "iunwrap.h"
#include "animtbl.h"
#include "decomp.h"
#include "../../../Libs/Math/mMathFn.h"


int				tpf,s,e;//animation ranges
Matrix3			TM;	
Quat			newQuat; 

static VOID ExportOneMeshMatrix(Interface *ip, IUtil *iu, int sel_num)
{
float			PerCent; 
WORD			iNumKeys; 
Point3			pos;
IKeyControl		*ikcX,*ikcY,*ikcZ;
int				KeyCounter;


	node = ip->GetSelNode(sel_num);//node ni olamiz
	nodeName = node->GetName();//node ni nomini olamiz
//******* Birinchi NumAnimKeys(pos , rot , scal) ni topamiz *****************
	iNumKeys = 0;
	tpf = GetTicksPerFrame();
	s = ip->GetAnimRange().Start()/tpf; 
	e = ip->GetAnimRange().End()/tpf;

	//FPS ga bo'lib olganimizdan so'ng, s va e da sekundlarda chiqdi:

	Control* cpos = node->GetTMController()->GetPositionController();
	Control* crot = node->GetTMController()->GetRotationController();
	Control* cscl = node->GetTMController()->GetScaleController();

	Control* crotNew = (Control*)ip->CreateInstance(CTRL_ROTATION_CLASS_ID,Class_ID(EULER_CONTROL_CLASS_ID,0));
	crotNew->Copy(crot);
	node->GetTMController()->SetRotationController(crotNew);

	Control* xContr  = crotNew->GetXController();
	Control* yContr  = crotNew->GetYController();
	Control* zContr  = crotNew->GetZController();

	if( (!cpos) || (!crotNew) || (!cpos) )
		return;

	ikcX = GetKeyControlInterface(xContr);//(IKeyControl*)xContr->GetInterface(I_KEYCONTROL);
	ikcY = GetKeyControlInterface(yContr);//(IKeyControl*)yContr->GetInterface(I_KEYCONTROL);
	ikcZ = GetKeyControlInterface(zContr);//(IKeyControl*)zContr->GetInterface(I_KEYCONTROL);
	if(!ikcX) MessageBox(NULL,"Do not get Euler X rotation controlller.","Error:",MB_OK);
	if(!ikcY) MessageBox(NULL,"Do not get Euler Y rotation controlller.","Error:",MB_OK);
	if(!ikcZ) MessageBox(NULL,"Do not get Euler Z rotation controlller.","Error:",MB_OK);
	int totXK = ikcX->GetNumKeys();
	int totYK = ikcY->GetNumKeys();
	int totZK = ikcZ->GetNumKeys();


	TimeValue i;
	for (int f = s; f <= e; f++) 
	{	i = f*tpf;
		if(crotNew->IsKeyAtTime(i,KEYAT_ROTATION))
			iNumKeys ++;
	}

	if(iNumKeys < 1)
	{	MessageBox(NULL, "Anim keylar yo'q!", "Ey", MB_OK);
		return ;
	}
//***************************************************************************

	if(!openfile(sMyData.szExport, nodeName, ".TMs", &streamBin  ,"wb"))
	{	char fullname[128]; sprintf(fullname, "%s\\%s%s", sMyData.szExport, nodeName, ".ani");
		MessageBox(NULL, fullname, "Faylini ochishda xato.", MB_OK);
		return;
	}

	//Shapkasi, for future using:
	char sh[20] = {"TMs................"};//Ortiqcha tip/mip masalan, yozish kerak b-sa,
	fwrite( sh, 1, 20, streamBin );//shu yerda yoziladi;

	//Birinchi bo'lib sekundlarda animini chiqarib olay, millisekundlarda:
	if(0.0f == sMyData.dete)//Aks holda dete editdan olib hisoblangan:
		sMyData.dete = 1000.0f * 40.0f * (float)(e-s) / tpf;//25 frame / sec. dan

	fwrite( &sMyData.dete, sizeof( sMyData.dete ), 1, streamBin );

	fwrite( &iNumKeys, sizeof( iNumKeys ), 1, streamBin );
//******************  Indexlarni yozamiz *************************************
	KeyCounter = 0;
	for(int f = s; f <= e; f++) 
	{	i = f*tpf;
		if(crotNew->IsKeyAtTime(i,KEYAT_ROTATION))
		{	char s[12];	wsprintf(s,"%d",KeyCounter);
			TM = node->GetNodeTM(i) * Inverse(node->GetParentTM(i));
	//************* avval percentni yozamiz *************************************
			PerCent = (float)f / (e);

			if( (iNumKeys == 0) && (PerCent != 0.f) )
				MessageBox(NULL,"Boshi surilgan, to'g'rilang, iltimos !!!","Diqqat",MB_OK);

			fwrite(&PerCent, sizeof(PerCent), 1, streamBin);
	//************* avval positionni yozamiz *************************************
			decomp_affine(TM, &ap);
			Point3 pos = ap.t;
			//avval surilmagan poslarni yozamiz ************************
			float mposx,mposy,mposz;
			mposx = pos.x * sMyData.fMassh;
			mposy = pos.y * sMyData.fMassh;
			mposz = pos.z * sMyData.fMassh;

			//Joyini almashtirib qo'yamiz:
			fwrite(&mposx, sizeof(mposx), 1, streamBin);
			fwrite(&mposz, sizeof(mposz), 1, streamBin);
			fwrite(&mposy, sizeof(mposy), 1, streamBin);
	//*************** endi rotationni yozsak bo'lar,??? *****************************
			ILinRotKey keyX[4], keyY[4], keyZ[4];
			Point3 tRot(0.0f, 0.0f, 0.0f);

			if(ikcX != 0)
			{	for(int xk = 0; xk < totXK; xk++)
				{	ikcX->GetKey(xk, &keyX[0]);
					if(keyX[0].time == i) { tRot.x = -keyX[0].val.z; break; }
			}	}

			if(ikcY != 0)
			{	for(int yk = 0; yk < totYK; yk++)
				{	ikcY->GetKey(yk, &keyY[0]);
					if(keyY[0].time == i) { tRot.y = -keyY[0].val.z; break; }
			}	}

			if(ikcZ != 0)
			{	for(int zk = 0; zk < totZK; zk++)
				{	ikcZ->GetKey(zk, &keyZ[0]);
					if(keyZ[0].time == i) { tRot.z = -keyZ[0].val.z; break; }
			}	}

			//FlipYZAxisInEuler(&tRot.x); Programmaning o'zida YZ rot lar matr * matr almashadi;
			fwrite(&tRot.x, sizeof(tRot.x), 1, streamBin);
			fwrite(&tRot.z, sizeof(tRot.z), 1, streamBin);
			fwrite(&tRot.y, sizeof(tRot.y), 1, streamBin);

			//Har biriga, trans,rot:
			fwrite(&sMyData.interpKoef, sizeof(float), 1, streamBin);
			fwrite(&sMyData.interpKoef, sizeof(float), 1, streamBin);

			KeyCounter++;
			Progr(0,100,(float)f/(float)e,"Write animation matrix: ", ip); 
	}	}

	fclose(streamBin);
	if(sMyData.bToText)
	{	char fullname[128]; sprintf(fullname, "%s\\%s%s", sMyData.szExport, nodeName, ".TMs");
		TMToText(fullname);
	}
	return;
}

VOID AllMeshMatrix(LPVOID par)
{
int			k,i;
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
	{
		MessageBox(NULL,"1nechtasini olib tashlang!","Diqqat",MB_OK);
		return;
	}

	if( (i < 0) || (i > 65000) ) // select qilingani umuman bo'lmasa
	{
		MessageBox(NULL,"Hech qanaqa obyekt topolmadim!","Diqqat",MB_OK);
		return;
	}


	for(k=0;k<=i;k++)
	{
		__try
		{
			streamBin = 0;
			ExportOneMeshMatrix(ip,iu,k);
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
	}
	return;
}
