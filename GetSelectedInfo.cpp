#include "ExpDX.h"
#include "modstack.h"
#include "d3dx9.h"
#include "../../../Libs/Math/mPlane.h"


static VOID GetMeshSelectedVertices(Interface *ip,IUtil *iu,int sel_num)
{
	node = ip->GetSelNode(sel_num);//node ni olamiz
	//Matrix3 t = node->GetObjTMAfterWSM(0);
	//if(TMNegParity(t))
	//{ vx[0] = 0;vx[1] = 1;vx[2] = 2; }
	//else 
	{ vx[0] = 0;vx[1] = 2;vx[2] = 1; }

	ObjectState os = node->EvalWorldState(ip->GetTime());
	if (!os.obj || os.obj->SuperClassID()!=GEOMOBJECT_CLASS_ID) 
	{	MessageBox(NULL,"Katta ildizni ololmayman!","Diqqat",MB_OK);
		return; // Safety net. This shouldn't happen.
	}
	TriObject* tri = GetTriObjectFromNode(node, ip->GetTime(), &needDel);
	if (!tri) 
	{	MessageBox(NULL,"Obyektni editmesh qilolmayman!","Diqqat",MB_OK);
		return;
	}
	mesh = &tri->GetMesh();
	mesh->buildNormals();
	if(mesh->selLevel != MESH_VERTEX)
	{	MessageBox(NULL, "Please, select the vertex selection metod.", "Err.", MB_OK);
		return;
	}
//************************** Parametrlari ************************************
	numFaces = (DWORD)(mesh->getNumFaces());
	char buf[256], b[16]; wsprintf(buf, "\0");
	for (DWORD i=0; i<numFaces; i++) 
	{	if(mesh->vertSel[ind[0]] || mesh->vertSel[ind[1]] || mesh->vertSel[ind[2]]) { sprintf(b, " F-%d: ", i); lstrcat(buf, b); }
		for (int k=0; k<3; k++) 
		{	ind[k] = (WORD)mesh->faces[i].v[vx[k]];
			if(mesh->vertSel[ind[k]]) { sprintf(b, "V-%d,", i*3+k); lstrcat(buf, b); }
			if(lstrlen(buf) > 200)  {lstrcat(buf, "Mem.is small."); break;        }
		}
		Progr(0,60,(float)i/(int)numFaces,"GetMeshSelectedVertices: ", ip); 
	}
	MessageBox(NULL, buf, "Selected vertices indicases in your mesh format are:", MB_OK);
//********************  TAMOM  ****************************
	if(needDel) tri->DeleteMe();
	return;
#undef calcMaxMin
}

static int SaveMeshSelectedVertices(Interface *ip,IUtil *iu)
{
	node = ip->GetSelNode(0);
	{ vx[0] = 0;vx[1] = 2;vx[2] = 1; }
	ObjectState os = node->EvalWorldState(ip->GetTime());
	if (!os.obj || os.obj->SuperClassID()!=GEOMOBJECT_CLASS_ID) 
	{	MessageBox(NULL,"Katta ildizni ololmayman!","Diqqat",MB_OK);
		return 0; // Safety net. This shouldn't happen.
	}
	TriObject* tri = GetTriObjectFromNode(node, ip->GetTime(), &needDel);
	if (!tri) 
	{	MessageBox(NULL,"Obyektni editmesh qilolmayman!","Diqqat",MB_OK);
		return 0;
	}
	mesh = &tri->GetMesh();
	if(mesh->selLevel != MESH_VERTEX)
	{	MessageBox(NULL, "Please, select the vertex selection metod.", "Err.", MB_OK);
		return 0;
	}
//************************** Parametrlari ************************************
	numFaces = (DWORD)(mesh->getNumFaces());
	numVerts = (DWORD)(mesh->getNumVerts());

	//Avval faylni ochib ko'ramiz:
	FILE *f = fopen(sMyData.vrtSelFile, "a");
	if(f)
	{	/*DWORD nf = numFaces;
		DWORD nch = 1;
		fwrite(&nf,4,1,f);
		fwrite(&nch,4,1,f);//Kanal soni;
		for (DWORD i=0; i<numFaces; i++) 
		{	for (int k=0; k<3; k++) 
			{	ind[k] = (WORD)mesh->faces[i].v[vx[k]];
				char ch = (mesh->vertSel[ind[k]]) ? 1: 0;
				fwrite(&ch,1,1,f);
		}	}*/
		S32 nsv=0;
		for(DWORD i=0; i<numVerts; i++)
			{if(1==mesh->vertSel[i]) nsv++;}
		fprintf(f,"\n%d\n",nsv);
		for(DWORD i=0; i<numVerts; i++)
			{if(1==mesh->vertSel[i]) fprintf(f," %d", i);}
		fclose(f); if(needDel) tri->DeleteMe(); return 1;
	}
/*	//else//eski faylga yozamiz, dobavka qilamiz:
	DWORD nf,nch;
	fread(&nf,4,1,f); fread(&nch,4,1,f);
	if(nf!=numFaces)
	{	MessageBox(NULL,"Err.","Faces number mismatch.",MB_OK);
		fclose(f); tri->DeleteMe();
	}
	char *pv = (char*)malloc(3*nf);fread(pv,1,3*nf,f); fclose(f);
	f = fopen(sMyData.vrtSelFile, "wb"); fwrite(&nf,4,1,f);
	nch++; fwrite(&nch,4,1,f);
	int crntv = 0;
	for (DWORD i=0; i<numFaces; i++) 
	{	for (int k=0; k<3; k++) 
		{	ind[k] = (WORD)mesh->faces[i].v[vx[k]];
			char ch = (mesh->vertSel[ind[k]]) ? 1: 0;
			char oldch = pv[crntv++];
			if((oldch>0) && (ch>0))
			{	char st[16]; itoa(i,st,10);
				MessageBox(NULL,st,"Faces vertex already in old selection area.",MB_OK);
				fclose(f); tri->DeleteMe(); free(pv); return nch-1;
			}
			fwrite(&ch,1,1,f);
	}	}
	fclose(f); tri->DeleteMe(); free(pv);
*/	return 0;//nch;
}

static int ExpNodeSelVertsFrIndsTxtFile(Interface *ip,IUtil *iu, BOOL bFlipX, BOOL bFlipY)
{
	node = ip->GetSelNode(0);
	{ vx[0] = 0;vx[1] = 2;vx[2] = 1; }
	ObjectState os = node->EvalWorldState(ip->GetTime());
	if (!os.obj || os.obj->SuperClassID()!=GEOMOBJECT_CLASS_ID) 
	{	MessageBox(NULL,"Katta ildizni ololmayman!","Diqqat",MB_OK);
		return 0; // Safety net. This shouldn't happen.
	}
	TriObject* tri = GetTriObjectFromNode(node, ip->GetTime(), &needDel);
	if (!tri) 
	{	MessageBox(NULL,"Obyektni editmesh qilolmayman!","Diqqat",MB_OK);
		return 0;
	}
	mesh = &tri->GetMesh();
	if(mesh->selLevel != MESH_VERTEX)
	{	MessageBox(NULL, "Please, select the vertex selection metod.", "Err.", MB_OK);
		return 0;
	}
//************************** Parametrlari ************************************
	numVerts = (DWORD)(mesh->getNumVerts());

	//Avval faylni ochib ko'ramiz:
	FILE *fi = fopen(sMyData.vrtSelFile, "r");
	if(fi)
	{	int w,h; fscanf(fi,"%d",&w);  fscanf(fi,"%d",&h);
		int *indsFrInp = (int*)malloc(w*h*sizeof(int));
		int *pi=indsFrInp;
		for(int y=0; y<h; y++) for(int x=0; x<w; x++)
			fscanf(fi, "%d", pi++);
		fclose(fi);

		fi = fopen(sMyData.vrtSelFile, "a");
		fprintf(fi,"\n\n    Please, delete all indicase numbers from begin of the lines.\nFor deleting from begin of verts. lines, uncheck \"to text\".\n     Vertice positions:\n");
char st[128];
		for(int y=0; y<h; y++)
		{	int yf = bFlipY ? (h-y-1) : y;
			for(int x=0; x<w; x++)
			{	int xf = bFlipX ? (w-x-1) : x;
				if(yf * w + xf > (int)numVerts-1)
				{	MessageBox(NULL, "Vertice indicase is oversize.", "Err.", MB_OK);
					fclose(fi); return 0;
				}
				pi = &indsFrInp[yf * w + xf];
				Point3 v = mesh->verts[*pi] * node->GetObjTMAfterWSM(ip->GetTime());
				if(sMyData.bToText)
				{	sprintf(st,"%d %d %f %f %f",x,y,
						v.x*sMyData.fMassh,
					    v.z*sMyData.fMassh,
						v.y*sMyData.fMassh);
				}
				else
				{	sprintf(st,"%f %f %f",
						v.x*sMyData.fMassh,
					    v.z*sMyData.fMassh,
						v.y*sMyData.fMassh);
				}
				for(int i=0; i<128; i++)
				{	if(','==st[i]) st[i]='.';
					if('\0'==st[i]) break;
				}
				fprintf(fi,"\n%s",st);
		}	}

		fprintf(fi,"\n\n   Texture coordinates (tu/tv) :\n");
		for(int y=0; y<h; y++)
		{	int yf = bFlipY ? (h-y-1) : y;
			for(int x=0; x<w; x++)
			{	int xf = bFlipX ? (w-x-1) : x;
				if(yf * w + xf > (int)numVerts-1)
				{	MessageBox(NULL, "Vertice indicase is oversize.", "Err.", MB_OK);
					fclose(fi); return 0;
				}
				pi = &indsFrInp[yf * w + xf];
				Point3 tv; UVVert ftv;
				if(bFlatten)
				{	ftv = mesh->mapVerts(flatMapChan)[*pi];
					if(sMyData.bToText)

					{	sprintf(st,"%d %d %f %f",x,y,ftv.x,ftv.y);
					}
					else sprintf(st,"%f %f",ftv.x,ftv.y);
				}
				else { tv = mesh->tVerts[*pi];
					if(sMyData.bToText)
						sprintf(st,"%d %d %.4f %.4f",x,y,
							tv.x,ftv.y);
					else sprintf(st,"%f %f",tv.x,tv.y);
				}
				for(int i=0; i<128; i++)
				{	if(','==st[i]) st[i]='.';
					if('\0'==st[i]) break;
				}
				fprintf(fi,"\n%s",st);
		}	}

		fclose(fi);
	}
	if(needDel) tri->DeleteMe();	
	return 1;
}

VOID GetSelectedInfo(LPVOID par)
{
int		    i;
ThrPar		*thrPar;
Interface	*ip;
IUtil		*iu;
int			type;

	thrPar  = (ThrPar*)par;
	ip		= thrPar->ip;
	iu		= thrPar->iu;
	type	= thrPar->type;

	
	i = GetSelID(ip);

	if(i != 0) // select qilingani umuman bo'lmasa
	{	MessageBox(NULL,"Faqat 1tasini exp (belgilang)-- ni belgilang", "1 nechtasini belgilabsiz!", MB_OK);
		return;
	}

	__try
	{
		GetMeshSelectedVertices(ip, iu, 0);
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
	}	}
	return;
}

int SaveVertexSelectionToFile(LPVOID par)
{
int		    i;
ThrPar		*thrPar;
Interface	*ip;
IUtil		*iu;
int			r=0;

	thrPar  = (ThrPar*)par;
	ip		= thrPar->ip;
	iu		= thrPar->iu;

	
	i = GetSelID(ip);

	if(i != 0) // select qilingani umuman bo'lmasa
	{	MessageBox(NULL,"Faqat 1tasini exp (belgilang)-- ni belgilang", "1 nechtasini belgilabsiz!", MB_OK);
		return 0;
	}

	__try
	{
		r = SaveMeshSelectedVertices(ip, iu);
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
		return 0;
	}
	return r;
}

int ExpSelVertsFrIndsTxtFile(LPVOID par, BOOL bFlipX, BOOL bFlipY)
{
int		    i;
ThrPar		*thrPar;
Interface	*ip;
IUtil		*iu;
int			r=0;

	thrPar  = (ThrPar*)par;
	ip		= thrPar->ip;
	iu		= thrPar->iu;

	i = GetSelID(ip);

	if(i != 0) // select qilingani umuman bo'lmasa
	{	MessageBox(NULL,"Faqat 1tasini exp (belgilang)-- ni belgilang", "1 nechtasini belgilabsiz!", MB_OK);
		return 0;
	}

	__try
	{
		r = ExpNodeSelVertsFrIndsTxtFile(ip, iu, bFlipX, bFlipY);
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
		return 0;
	}
	return r;
}
