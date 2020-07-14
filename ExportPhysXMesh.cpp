#include "ExpDX.h"
#include "modstack.h"
#include "d3dx9.h"
#include <simpobj.h>
#include <simpmod.h>


BOOL PhysXMeshToText(char*);

extern int errStep;
static VOID ExportOnePhysXMesh(Interface *ip,IUtil *iu,int sel_num)
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
//************************** Parametrlari ************************************
	numVerts = mesh->getNumVerts();
	numFaces = (DWORD)(mesh->getNumFaces());

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
	if(!openfile(sMyData.szExport, nodeName, bClone ? ".clmshPhysX" : ".mshPhysX", &streamBin  ,"wb"))
	{	char fullname[128]; sprintf(fullname, "%s\\%s%s", sMyData.szExport, nodeName, bClone ? ".clmshPhysX" : ".mshPhysX");
		MessageBox(NULL, fullname, "Faylini ochishda xato.", MB_OK);
		return;
	}

	//Shapkasi, for future using:
	char sh[20] = {"Msh16PhysX........."};//Ortiqcha tip/mip masalan, yozish kerak b-sa,
	if(sMyData.b32bit && sMyData.bInIndexed) {sh[3] = '3'; sh[4] = '2';}
	else               {sh[3] = '1'; sh[4] = '6';}
	if(bClone)         {sh[10]= 'c'; sh[11]= 'l';}
	fwrite( sh, 1, 20, streamBin );//shu yerda yoziladi;

	errStep = 1;
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
	mInv = GetInvTransform(ap.t, ap.q);
//***************   FACE lar sonini yozamiz **********************************
	fwrite(&numFaces, sizeof(DWORD), 1, streamBin    );
	fwrite(&numVerts, sizeof(DWORD), 1, streamBin    );
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
	}
//********************  TAMOM  ****************************
Tamom:
	fclose(streamBin);
	if(sMyData.bToText)
	{char fullname[128]; sprintf(fullname, "%s\\%s%s", sMyData.szExport, nodeName, bClone ? ".clmshPhysX" : ".mshPhysX");
	 PhysXMeshToText(fullname);
	}
	return;
#undef calcMaxMin
}

VOID AllPhysXMesh(LPVOID par)
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
					ExportOnePhysXMesh(ip, iu, k);
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

//Test pasted successfully;
BOOL PhysXMeshToText(char *NAME)
{
FILE	*fIn, *fOut;
char	pathAndName[128];

	lstrcpy(pathAndName, NAME);

	fIn = fopen(pathAndName, "rb");
	if(!fIn) return FALSE;
	lstrcat(strrchr(pathAndName, '.'), "txt");
	fOut = fopen(pathAndName, "w");

	//Shapkasi, for future using:
	char sh[20];
	fread(sh, 1, 20, fIn);
	if(!(sh[0]=='M'&&sh[1]=='s'&&sh[2]=='h'&&sh[5]=='P'&&sh[6]=='h'&&sh[7]=='y'&&sh[8]=='s'&&sh[9]=='X'))
	{	fclose(fIn);
		fclose(fOut);
		return FALSE;
	}
	BOOL b32bit = (sh[3] == '3' && sh[4] == '2');
	BOOL bcl = (sh[10]=='c' && sh[11]=='l');

	float buf[24];
	fread(buf, 4, 9, fIn);
	fprintf(fOut, "\nPosition: %.4f, %.4f, %.4f; \nScale: %.4f, %.4f, %.4f; \nRotation in EulerXYZ: %.4f, %.4f, %.4f;", 
					buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8]);
	if(bcl)	goto End;

	DWORD dwNumFaces; fread(&dwNumFaces, 4, 1, fIn);
	fprintf(fOut, "\nNumber of faces: %#05.5d", dwNumFaces);

	DWORD dwNumVerts; fread(&dwNumVerts, 4, 1, fIn);
	fprintf(fOut, "\nNumber of vertices: %#05.5d", dwNumVerts);

	fprintf(fOut, "\n\n           Indicases:");
	DWORD inds2[3];	WORD inds1[3];
	for(DWORD f=0; f<dwNumFaces; f++)
	{	if(b32bit)
		{	fread(inds2, 4, 3, fIn);
			fprintf(fOut, "\nF %#05.5d: %#05.5d, %#05.5d,  %#05.5d", f, inds2[0], inds2[1], inds2[2]); 
		}
		else
		{	fread(inds1, 2, 3, fIn);
			fprintf(fOut, "\nF %#05.5d: %#05.5d, %#05.5d,  %#05.5d", f, inds1[0], inds1[1], inds1[2]); 
	}	}

	fprintf(fOut, "\n\n           Vertice positions:");
	for(DWORD v=0; v<dwNumVerts; v++)
	{	fread(buf, 4, 3, fIn);
		fprintf(fOut, "\n%d: %05.2f %05.2f %05.2f", v, buf[0], buf[1], buf[2]);
	}

End:
	fclose(fIn);
	fclose(fOut);
	return TRUE;
}