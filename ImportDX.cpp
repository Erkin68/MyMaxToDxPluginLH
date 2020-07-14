#include "windows.h"
#include <stdio.h>
#include "ExpDX.h"
#include "../../../Libs/Math/mMathFn.h"

#define MAX_BUF   10000


#pragma warning(disable:4244)


//Weld qilishda 1ta vertni olib tashlash uchun, faqat pos geom uchun.
template<class T> VOID DeleteVert(T* inds, float* verts, T vCh, DWORD v, DWORD totVerts, DWORD totFaces)
{
	//Avval surib chiqamiz:
	for(DWORD vp=v; vp<totVerts-1; vp++)
	{	verts[vp*3  ] = verts[(vp+1)*3  ];
		verts[vp*3+1] = verts[(vp+1)*3+1];
		verts[vp*3+2] = verts[(vp+1)*3+2];
	}
	
	//Endi indexlarni to'g'rilaymiz:
	for(DWORD f=0; f<totFaces; f++)
	{	for(DWORD ind=0; ind<3; ind++)
		{	if(inds[f*3+ind] == v)
				inds[f*3+ind] = vCh;
			else if(inds[f*3+ind] > v)
				inds[f*3+ind]--;
	}	}
	return;
}

template<class T> VOID DeleteTVVert(T* inds, float* verts, T vCh, DWORD v, DWORD totVerts, DWORD totFaces)
{
	//Avval surib chiqamiz:
	for(DWORD vp=v; vp<totVerts-1; vp++)
	{	verts[vp*2  ] = verts[(vp+1)*2  ];
		verts[vp*2+1] = verts[(vp+1)*2+1];
	}

	//Endi indexlarni to'g'rilaymiz:
	for(DWORD f=0; f<totFaces; f++)
	{	for(DWORD ind=0; ind<3; ind++)
		{	if(inds[f*3+ind] == v)
				inds[f*3+ind] = vCh;
			else if(inds[f*3+ind] > v)
				inds[f*3+ind]--;
	}	}
	return;
}

template<class T> VOID DeleteCVVert(T* inds, DWORD* verts, T vCh, DWORD v, DWORD totVerts, DWORD totFaces)
{
	//Avval surib chiqamiz:
	for(DWORD vp=v; vp<totVerts-1; vp++)
		verts[vp] = verts[vp+1];

	//Endi indexlarni to'g'rilaymiz:
	for(DWORD f=0; f<totFaces; f++)
	{	for(DWORD ind=0; ind<3; ind++)
		{	if(inds[f*3+ind] == v)
				inds[f*3+ind] = vCh;
			else if(inds[f*3+ind] > v)
				inds[f*3+ind]--;
	}	}
	return;
}

//Import qilganda vertexlar bir-biridan ajralib qolg'on. Ularni payvandlash kerak:
template<class T> DWORD WeldVertices(T* inds, float *verts, DWORD totFaces, DWORD totVerts)
{
	for(DWORD v=0; v<totVerts; v++)
	{	Point3 PV(verts[3*v], verts[3*v+1], verts[3*v+2]);
		for(T vp=0; vp<v; vp++)
		{	Point3 PVP(verts[3*vp], verts[3*vp+1], verts[3*vp+2]);
			if(PVP == PV)
			{	DeleteVert(inds, verts, vp, v, totVerts, totFaces);
				totVerts--;
				v--;
				break;
	}	}	}

	return totVerts;
}

template<class T> DWORD WeldTVVertices(T* inds, float *verts, DWORD totFaces, DWORD totTVVerts)
{
	for(DWORD v=0; v<totTVVerts; v++)
	{	Point2 PV(verts[2*v], verts[2*v+1]);
		for(T vp=0; vp<v; vp++)
		{	Point2 PVP(verts[2*vp], verts[2*vp+1]);
			if(PVP == PV)
			{	DeleteTVVert(inds, verts, vp, v, totTVVerts, totFaces);
				totTVVerts--;
				v--;
				break;
	}	}	}

	return totTVVerts;
}

template<class T> DWORD WeldCVVertices(T* inds, DWORD*verts, DWORD totFaces, DWORD totCVVerts)
{
	for(DWORD v=0; v<totCVVerts; v++)
	{	DWORD PV = verts[v];
		for(T vp=0; vp<v; vp++)
		{	T PVP = verts[vp];
			if(PVP == PV)
			{	DeleteCVVert(inds, verts, vp, v, totCVVerts, totFaces);
				totCVVerts--;
				v--;
				break;
	}	}	}

	return totCVVerts;
}

//Albatta indexed meshda;
static VOID ImportOneIndexedMesh(Interface *ip, LPCTSTR buf)
{
Matrix3 TM;
Mesh	*mesh; 
FILE    *stream;
DWORD	dwNumFaces,dwNumVerts;
DWORD	*pdwIndicaces = NULL,*pdwTVIndicaces = NULL;
WORD	*pwIndicaces = NULL,*pwTVIndicaces = NULL;
float   pos[3],scs[3],angles[3],*pfNormals = NULL,
		*pfVerts = NULL,*pfTextCoords = NULL;

	if(!openfile((char*)buf, &stream  ,"rb"))
	{	MessageBox(NULL, buf, "Faylini ochishda xato.", MB_OK);
		return;
	}
//*********************************************
	char sh[20]; if(!READFILE(sh, 1, 20, stream)){ fclose(stream); return; }
	BOOL b32bit = (sh[3]=='3' && sh[4]=='2');
	BOOL bcl = (sh[5]=='c' && sh[6]=='l');
	if(bcl){MessageBox(NULL, "This is clone only file.", "Can'n!", MB_OK); fclose(stream); return;}
	myMtrl.Read(stream);
//************************************************************************************
	if(!READFILE(&pos[0]   , sizeof(pos[0]), 3, stream)) goto End;
	if(sMyData.fMassh != 0.f){pos[0] /= sMyData.fMassh;pos[1] /= sMyData.fMassh;pos[2] /= sMyData.fMassh;}
	if(!READFILE(&scs[0]   , sizeof(scs[0]), 3, stream)) goto End;
	if(!READFILE(&angles[0], sizeof(angles[0]), 3, stream))goto End;
//************************************************************************************
	if(!READFILE(&dwNumFaces, sizeof(DWORD), 1, stream)) goto End;
	if(!READFILE(&dwNumVerts, sizeof(DWORD), 1, stream)) goto End;
//************************************************************************************
	float MinMax[6];
	if(!READFILE(&MinMax[0], sizeof(float), 6, stream)) return;
	if(sMyData.fMassh != 0.f){for(int i=0; i<6; i++) MinMax[i] /= sMyData.fMassh;}
//************************************************************************************
	//yangi heap uchun
	if(b32bit)
	{	pdwIndicaces   = (DWORD*)malloc(4 * dwNumFaces * 3);
		pdwTVIndicaces = (DWORD*)malloc(4 * dwNumFaces * 3);
	}
	else
	{	pwIndicaces   = (WORD*)malloc(2 * dwNumFaces * 3);
		pwTVIndicaces = (WORD*)malloc(2 * dwNumFaces * 3);
	}
	pfNormals     = (float*)malloc(sizeof(float) * dwNumVerts * 3);
	pfVerts       = (float*)malloc(sizeof(float) * dwNumVerts * 3);
	pfTextCoords  = (float*)malloc(sizeof(float) * dwNumVerts * 2);
//************************************************************************************
	for(DWORD i = 0; i < dwNumVerts; i++)
	{	if(!READFILE(pfVerts    + i * 3, sizeof(float), 3, stream)) goto End;
		if(!READFILE(pfNormals  + i * 3, sizeof(float), 3, stream)) goto End;
		if(!READFILE(pfTextCoords+i * 2, sizeof(float), 2, stream)) goto End;
		//DirectX ning OpenGL dan farqini kiritamiz:
		//1.Textura tu,tv larini to'g'rilab chiqamiz:
		*(pfTextCoords+i * 2 + 1) = 1.f - *(pfTextCoords+i * 2 + 1);
	}
	for(DWORD i = 0; i < dwNumFaces; i++)
	{	if(b32bit)
		{	if(!READFILE(pdwIndicaces + i * 3, sizeof(DWORD), 3, stream)) goto End;
			*(pdwTVIndicaces + i*3  ) = *(pdwIndicaces + i*3  );
			*(pdwTVIndicaces + i*3+1) = *(pdwIndicaces + i*3+1);
			*(pdwTVIndicaces + i*3+2) = *(pdwIndicaces + i*3+2);
		}
		else
		{	if(!READFILE(pwIndicaces + i * 3, sizeof(WORD), 3, stream)) goto End;
			*(pwTVIndicaces + i*3  ) = *(pwIndicaces + i*3  );
			*(pwTVIndicaces + i*3+1) = *(pwIndicaces + i*3+1);
			*(pwTVIndicaces + i*3+2) = *(pwIndicaces + i*3+2);
	}	}
	
	DWORD newVerts=0, newTVVerts=0;
	if(b32bit)
	{	newVerts = WeldVertices(pdwIndicaces, pfVerts, dwNumFaces, dwNumVerts);
		newTVVerts = WeldTVVertices(pdwTVIndicaces, pfTextCoords, dwNumFaces, dwNumVerts);
	}
	else
	{	newVerts = WeldVertices(pwIndicaces, pfVerts, dwNumFaces, dwNumVerts);
		newTVVerts = WeldTVVertices(pwTVIndicaces, pfTextCoords, dwNumFaces, dwNumVerts);
	}

//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************
//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************
//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************
//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************
//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************
//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************
//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************
//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************
//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************
//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************
//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************

	TriObject *tri; tri = CreateNewTriObject();
	if(!tri) goto End;

	mesh = 0; mesh = &tri->GetMesh();
	if(!mesh->setNumFaces(dwNumFaces)) goto End;
	if(!mesh->setNumTVFaces(dwNumFaces))	goto End;
	if(!mesh->setNumVerts(newVerts)) goto End;
	if(!mesh->setNumTVerts(newTVVerts))	goto End;

	//indicase larni joylashtiraman;
	//DirectXda farqi bor, qara:
	for(DWORD i=0; i<dwNumFaces; i++) 
	{// Maxda feys yo'nalishi teskaridur, shuning uchun avval +2, so'ng +1.
		if(b32bit)
		{	mesh-> faces[i].v[0]  = *(pdwIndicaces + i*3    );
			mesh-> faces[i].v[1]  = *(pdwIndicaces + i*3 + 2);
			mesh-> faces[i].v[2]  = *(pdwIndicaces + i*3 + 1);

			mesh->tvFace[i].t[0]  = *(pdwTVIndicaces + i*3    );
			mesh->tvFace[i].t[1]  = *(pdwTVIndicaces + i*3 + 2);
			mesh->tvFace[i].t[2]  = *(pdwTVIndicaces + i*3 + 1);
		}
		else
		{	mesh-> faces[i].v[0]  = *(pwIndicaces + i*3    );
			mesh-> faces[i].v[1]  = *(pwIndicaces + i*3 + 2);
			mesh-> faces[i].v[2]  = *(pwIndicaces + i*3 + 1);

			mesh->tvFace[i].t[0]  = *(pwTVIndicaces + i*3    );
			mesh->tvFace[i].t[1]  = *(pwTVIndicaces + i*3 + 2);
			mesh->tvFace[i].t[2]  = *(pwTVIndicaces + i*3 + 1);
	}	}
	for(DWORD i=0; i<newVerts; i++) 
	{	mesh->verts[i].x = *(pfVerts + i*3    );
		mesh->verts[i].z = *(pfVerts + i*3 + 1);
		mesh->verts[i].y = *(pfVerts + i*3 + 2);
		if(sMyData.fMassh != 0.f)
		{	mesh->verts[i].x /= sMyData.fMassh;
			mesh->verts[i].y /= sMyData.fMassh;
			mesh->verts[i].z /= sMyData.fMassh;
	}	}
	for(DWORD i=0; i<newTVVerts; i++) 
	{	mesh->tVerts[i].x = *(pfTextCoords + i*2    );
		mesh->tVerts[i].y = *(pfTextCoords + i*2 + 1);
	}

 	INode *node; node = ip->CreateObjectNode(tri);
	myMtrl.Set(node);

	int onlyNamePos; onlyNamePos = strrchr(buf, '\\') - buf + 1;
	char *name; name = (char*)(buf + onlyNamePos);
	*(strrchr(name, '.')) = 0; // Chop off the name
	node->SetName((char *)name);

	TM.IdentityMatrix();
	TM.RotateX(angles[0]);
	TM.RotateY(angles[2]);
	TM.RotateZ(angles[1]);
	TM.Translate(Point3(pos[0],pos[1],pos[2]));

	node->SetNodeTM(0, TM);
	node->NotifyDependents(FOREVER,PART_TM,REFMSG_CHANGE);

	ip->RedrawViews(ip->GetTime());

End:

//	if(!mesh)
//		delete mesh;
//	if(!tri)
//		delete tri;

	if(!pdwIndicaces) free(pdwIndicaces);
	if(!pdwTVIndicaces) free(pdwTVIndicaces);
	if(!pwIndicaces) free(pwIndicaces);
	if(!pwTVIndicaces) free(pwTVIndicaces);
	if(!pfNormals) free(pfNormals);
	if(!pfVerts) free(pfVerts);
	if(!pfTextCoords) free(pfTextCoords);

	fclose(stream);
	return;
}

static VOID ImportOneMesh(Interface *ip, LPCTSTR buf)
{
Matrix3 TM;
Mesh	*mesh; 
FILE    *stream;
DWORD	dwNumFaces,dwNumVerts;
DWORD	*pdwIndicaces = NULL,*pdwTVIndicaces = NULL;
WORD	*pwIndicaces = NULL,*pwTVIndicaces = NULL;
float   pos[3],scs[3],angles[3],*pfNormals = NULL,
		*pfVerts = NULL,*pfTextCoords = NULL;

	if(!openfile((char*)buf, &stream  ,"rb"))
	{	MessageBox(NULL, buf, "Faylini ochishda xato.", MB_OK);
		return;
	}
//*********************************************
	char sh[20]; if(!READFILE(sh, 1, 20, stream)){ fclose(stream); return; }
	BOOL b32bit = (sh[3]=='3' && sh[4]=='2');
	BOOL bcl = (sh[5]=='c' && sh[6]=='l');
	if(bcl){MessageBox(NULL, "This is clone only file.", "Can'n!", MB_OK); fclose(stream); return;}
	myMtrl.Read(stream);
//************************************************************************************
	if(!READFILE(&pos[0]   , sizeof(pos[0]), 3, stream)) goto End;
	if(sMyData.fMassh != 0.f){pos[0] /= sMyData.fMassh;pos[1] /= sMyData.fMassh;pos[2] /= sMyData.fMassh;}
	if(!READFILE(&scs[0]   , sizeof(scs[0]), 3, stream)) goto End;
	if(!READFILE(&angles[0], sizeof(angles[0]), 3, stream))goto End;
//************************************************************************************
	if(!READFILE(&dwNumFaces, sizeof(DWORD), 1, stream)) goto End;
	if(!READFILE(&dwNumVerts, sizeof(DWORD), 1, stream)) goto End;//hozircha;
	dwNumVerts = dwNumFaces*3;
//************************************************************************************
	float MinMax[6];
	if(!READFILE(&MinMax[0], sizeof(float), 6, stream)) return;
	if(sMyData.fMassh != 0.f){for(int i=0; i<6; i++) MinMax[i] /= sMyData.fMassh;}
//************************************************************************************
	//yangi heap uchun
	if(b32bit)
	{	pdwIndicaces   = (DWORD*)malloc(4 * dwNumFaces * 3);
		pdwTVIndicaces = (DWORD*)malloc(4 * dwNumFaces * 3);
	}
	else
	{	pwIndicaces   = (WORD*)malloc(2 * dwNumFaces * 3);
		pwTVIndicaces = (WORD*)malloc(2 * dwNumFaces * 3);
	}
	pfNormals     = (float*)malloc(sizeof(float) * dwNumVerts * 3);
	pfVerts       = (float*)malloc(sizeof(float) * dwNumVerts * 3);
	pfTextCoords  = (float*)malloc(sizeof(float) * dwNumVerts * 2);
//************************************************************************************
	for(DWORD i = 0; i < dwNumFaces*3; i++)
	{	if(!READFILE(pfVerts    + i * 3, sizeof(float), 3, stream)) goto End;
		if(!READFILE(pfNormals  + i * 3, sizeof(float), 3, stream)) goto End;
		if(!READFILE(pfTextCoords+i * 2, sizeof(float), 2, stream)) goto End;
		//DirectX ning OpenGL dan farqini kiritamiz:
		//1.Textura tu,tv larini to'g'rilab chiqamiz:
		*(pfTextCoords+i * 2 + 1) = 1.f - *(pfTextCoords+i * 2 + 1);
	}
	for(DWORD i = 0; i < dwNumFaces; i++)
	{	if(b32bit)
		{	*(pdwIndicaces   + i*3  ) = i*3;
			*(pdwIndicaces   + i*3+1) = i*3+1;
			*(pdwIndicaces   + i*3+2) = i*3+2;
			*(pdwTVIndicaces + i*3  ) = i*3;
			*(pdwTVIndicaces + i*3+1) = i*3+1;
			*(pdwTVIndicaces + i*3+2) = i*3+2;
		}
		else
		{	*(pwIndicaces   + i*3  ) = i*3  ;
			*(pwIndicaces   + i*3+1) = i*3+1;
			*(pwIndicaces   + i*3+2) = i*3+2;
			*(pwTVIndicaces + i*3  ) = i*3  ;
			*(pwTVIndicaces + i*3+1) = i*3+1;
			*(pwTVIndicaces + i*3+2) = i*3+2;
	}	}

	//if(!READFILE(&dwNumVerts, sizeof(DWORD), 1, stream)) goto End;
	
	DWORD newVerts=0, newTVVerts=0;// 0 edi;
	if(b32bit)
	{	newVerts = WeldVertices(pdwIndicaces, pfVerts, dwNumFaces, dwNumVerts);
		newTVVerts = WeldTVVertices(pdwTVIndicaces, pfTextCoords, dwNumFaces, dwNumVerts);
	}
	else
	{	newVerts = WeldVertices(pwIndicaces, pfVerts, dwNumFaces, dwNumVerts);
		newTVVerts = WeldTVVertices(pwTVIndicaces, pfTextCoords, dwNumFaces, dwNumVerts);
	}

//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************
//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************
//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************
//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************
//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************
//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************
//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************
//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************
//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************
//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************
//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************

	TriObject *tri; tri = CreateNewTriObject();
	if(!tri) goto End;

	mesh = &tri->GetMesh();
	if(!mesh->setNumFaces(dwNumFaces)) goto End;
	if(!mesh->setNumTVFaces(dwNumFaces)) goto End;
	if(!mesh->setNumVerts(newVerts)) goto End;
	if(!mesh->setNumTVerts(newTVVerts)) goto End;

	//indicase larni joylashtiraman;
	//DirectXda farqi bor, qara:
	for(DWORD i=0; i<dwNumFaces; i++) 
	{// Maxda feys yo'nalishi teskaridur, shuning uchun avval +2, so'ng +1.
		if(b32bit)
		{	mesh-> faces[i].v[0]  = *(pdwIndicaces + i*3    );
			mesh-> faces[i].v[1]  = *(pdwIndicaces + i*3 + 2);
			mesh-> faces[i].v[2]  = *(pdwIndicaces + i*3 + 1);

			mesh->tvFace[i].t[0]  = *(pdwTVIndicaces + i*3    );
			mesh->tvFace[i].t[1]  = *(pdwTVIndicaces + i*3 + 2);
			mesh->tvFace[i].t[2]  = *(pdwTVIndicaces + i*3 + 1);
		}
		else
		{	mesh-> faces[i].v[0]  = *(pwIndicaces + i*3    );
			mesh-> faces[i].v[1]  = *(pwIndicaces + i*3 + 2);
			mesh-> faces[i].v[2]  = *(pwIndicaces + i*3 + 1);

			mesh->tvFace[i].t[0]  = *(pwTVIndicaces + i*3    );
			mesh->tvFace[i].t[1]  = *(pwTVIndicaces + i*3 + 2);
			mesh->tvFace[i].t[2]  = *(pwTVIndicaces + i*3 + 1);
	}	}
	for(DWORD i=0; i<newVerts; i++) 
	{	mesh->verts[i].x = *(pfVerts + i*3    );
		mesh->verts[i].z = *(pfVerts + i*3 + 1);
		mesh->verts[i].y = *(pfVerts + i*3 + 2);
		if(sMyData.fMassh != 0.f)
		{	mesh->verts[i].x /= sMyData.fMassh;
			mesh->verts[i].y /= sMyData.fMassh;
			mesh->verts[i].z /= sMyData.fMassh;
	}	}
	for(DWORD i=0; i<newTVVerts; i++) 
	{	mesh->tVerts[i].x = *(pfTextCoords + i*2    );
		mesh->tVerts[i].y = *(pfTextCoords + i*2 + 1);
	}

 	INode *node; node = ip->CreateObjectNode(tri);
	myMtrl.Set(node);

	int onlyNamePos; onlyNamePos = strrchr(buf, '\\') - buf + 1;
	char *name; name = (char*)(buf + onlyNamePos);
	*(strrchr(name, '.')) = 0; // Chop off the name
	node->SetName((char *)name);

	TM.IdentityMatrix();
	TM.RotateX(angles[0]);
	TM.RotateY(angles[2]);
	TM.RotateZ(angles[1]);
	TM.Translate(Point3(pos[0],pos[1],pos[2]));

	node->SetNodeTM(0, TM);
	node->NotifyDependents(FOREVER,PART_TM,REFMSG_CHANGE);

	ip->RedrawViews(ip->GetTime());

End:

//	if(!mesh)
//		delete mesh;
//	if(!tri)
//		delete tri;

	if(!pdwIndicaces) free(pdwIndicaces);
	if(!pdwTVIndicaces) free(pdwTVIndicaces);
	if(!pwIndicaces) free(pwIndicaces);
	if(!pwTVIndicaces) free(pwTVIndicaces);
	if(!pfNormals) free(pfNormals);
	if(!pfVerts) free(pfVerts);
	if(!pfTextCoords) free(pfTextCoords);

	fclose(stream);
	return;
}

//Albatta indexed meshda;
static VOID ImportOneTngntMesh(Interface *ip, LPCTSTR buf)
{
FILE    *stream;
float   pos[3],scs[3],angles[3],*pfNormals=NULL,*pfVerts=NULL,*pfTextCoords=NULL;
DWORD	dwNumFaces,dwNumVerts;
WORD	*pwIndicaces = NULL,*pwTVIndicaces=NULL;
DWORD	*pdwIndicaces=NULL,*pdwTVIndicaces=NULL;
Mesh	*mesh; 
Matrix3 TM;

	if(!openfile((char*)buf, &stream  ,"rb"))
	{	MessageBox(NULL, buf, "Faylini ochishda xato.", MB_OK);
		return;
	}
//*********************************************
	char sh[20]; if(!READFILE(sh, 1, 20, stream)){ fclose(stream); return; }
	BOOL b32bit = (sh[3]=='3' && sh[4]=='2');
	BOOL bcl = (sh[5]=='c' && sh[6]=='l');
	if(bcl){MessageBox(NULL, "This is clone only file.", "Can'n!", MB_OK); fclose(stream); return;}
	myMtrl.Read(stream);
//************************************************************************************
	if(!READFILE(&pos[0]   , sizeof(pos[0]), 3, stream)) goto End;
	if(sMyData.fMassh != 0.f){pos[0] /= sMyData.fMassh;pos[1] /= sMyData.fMassh;pos[2] /= sMyData.fMassh;}
	if(!READFILE(&scs[0]   , sizeof(scs[0]), 3, stream)) goto End;
	if(!READFILE(&angles[0], sizeof(angles[0]), 3, stream))goto End;
//************************************************************************************
	if(!READFILE(&dwNumFaces, sizeof(DWORD), 1, stream)) goto End;
	if(!READFILE(&dwNumVerts, sizeof(DWORD), 1, stream)) goto End;
//************************************************************************************
	float MinMax[6];
	if(!READFILE(&MinMax[0], sizeof(float), 6, stream)) return;
	if(sMyData.fMassh != 0.f){for(int i=0; i<6; i++) MinMax[i] /= sMyData.fMassh;}
//************************************************************************************
	//yangi heap uchun 
	if(b32bit)
	{	pdwIndicaces   = (DWORD*)malloc(2 * dwNumFaces * 3);
		pdwTVIndicaces = (DWORD*)malloc(2 * dwNumFaces * 3);
	}
	else
	{	pwIndicaces   = (WORD*)malloc(2 * dwNumFaces * 3);
		pwTVIndicaces = (WORD*)malloc(2 * dwNumFaces * 3);
	}
	pfNormals     = (float*)malloc(sizeof(float) * dwNumVerts * 3);
	pfVerts       = (float*)malloc(sizeof(float) * dwNumVerts * 3);
	pfTextCoords  = (float*)malloc(sizeof(float) * dwNumVerts * 2);
//************************************************************************************
	for(DWORD i = 0; i < dwNumVerts; i++)
	{	if(!READFILE(pfVerts    + i * 3, sizeof(float), 3, stream)) goto End;
		if(!READFILE(pfNormals  + i * 3, sizeof(float), 3, stream)) goto End;
		if(!READFILE(pfTextCoords+i * 2, sizeof(float), 2, stream)) goto End;
		float tngntBinrmls[6]; if(!READFILE(&tngntBinrmls[0], sizeof(float), 6, stream)) goto End;
		//DirectX ning OpenGL dan farqini kiritamiz:
		//1.Textura tu,tv larini to'g'rilab chiqamiz:
		*(pfTextCoords+i * 2 + 1) = 1.f - *(pfTextCoords+i * 2 + 1);
	}
	for(DWORD i = 0; i < dwNumFaces; i++)
	{	if(b32bit)
		{	if(!READFILE(pdwIndicaces + i * 3, sizeof(DWORD), 3, stream)) goto End;
			*(pdwTVIndicaces + i*3  ) = *(pdwIndicaces + i*3  );
			*(pdwTVIndicaces + i*3+1) = *(pdwIndicaces + i*3+1);
			*(pdwTVIndicaces + i*3+2) = *(pdwIndicaces + i*3+2);
		}
		else
		{	if(!READFILE(pwIndicaces + i * 3, sizeof(WORD), 3, stream)) goto End;
			*(pwTVIndicaces + i*3  ) = *(pwIndicaces + i*3  );
			*(pwTVIndicaces + i*3+1) = *(pwIndicaces + i*3+1);
			*(pwTVIndicaces + i*3+2) = *(pwIndicaces + i*3+2);
	}	}
	
	DWORD newVerts=0, newTVVerts=0;
	if(b32bit)
	{	newVerts = WeldVertices(pdwIndicaces, pfVerts, dwNumFaces, dwNumVerts);
		newTVVerts = WeldTVVertices(pdwTVIndicaces, pfTextCoords, dwNumFaces, dwNumVerts);
	}
	else
	{	newVerts = WeldVertices(pwIndicaces, pfVerts, dwNumFaces, dwNumVerts);
		newTVVerts = WeldTVVertices(pwTVIndicaces, pfTextCoords, dwNumFaces, dwNumVerts);
	}

//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************
//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************
//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************
//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************
//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************
//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************
//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************
//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************
//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************
//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************
//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************

	TriObject *tri; tri = CreateNewTriObject();
	if(!tri) goto End;

	mesh = 0; mesh = &tri->GetMesh();
	if(!mesh->setNumFaces(dwNumFaces)) goto End;
	if(!mesh->setNumTVFaces(dwNumFaces))	goto End;
	if(!mesh->setNumVerts(newVerts)) goto End;
	if(!mesh->setNumTVerts(newTVVerts))	goto End;

	//indicase larni joylashtiraman;
	//DirectXda farqi bor, qara:
	for(DWORD i=0; i<dwNumFaces; i++) 
	{	if(b32bit)
		{// Maxda feys yo'nalishi teskaridur, shuning uchun avval +2, so'ng +1.
			mesh-> faces[i].v[0]  = *(pdwIndicaces + i*3    );
			mesh-> faces[i].v[1]  = *(pdwIndicaces + i*3 + 2);
			mesh-> faces[i].v[2]  = *(pdwIndicaces + i*3 + 1);

			mesh->tvFace[i].t[0]  = *(pdwTVIndicaces + i*3    );
			mesh->tvFace[i].t[1]  = *(pdwTVIndicaces + i*3 + 2);
			mesh->tvFace[i].t[2]  = *(pdwTVIndicaces + i*3 + 1);
		}
		else
		{// Maxda feys yo'nalishi teskaridur, shuning uchun avval +2, so'ng +1.
			mesh-> faces[i].v[0]  = *(pwIndicaces + i*3    );
			mesh-> faces[i].v[1]  = *(pwIndicaces + i*3 + 2);
			mesh-> faces[i].v[2]  = *(pwIndicaces + i*3 + 1);

			mesh->tvFace[i].t[0]  = *(pwTVIndicaces + i*3    );
			mesh->tvFace[i].t[1]  = *(pwTVIndicaces + i*3 + 2);
			mesh->tvFace[i].t[2]  = *(pwTVIndicaces + i*3 + 1);
	}	}
	for(DWORD i=0; i<newVerts; i++) 
	{	mesh->verts[i].x = *(pfVerts + i*3    );
		mesh->verts[i].z = *(pfVerts + i*3 + 1);
		mesh->verts[i].y = *(pfVerts + i*3 + 2);
		if(sMyData.fMassh != 0.f)
		{	mesh->verts[i].x /= sMyData.fMassh;
			mesh->verts[i].y /= sMyData.fMassh;
			mesh->verts[i].z /= sMyData.fMassh;
	}	}
	for(DWORD i=0; i<newTVVerts; i++) 
	{	mesh->tVerts[i].x = *(pfTextCoords + i*2    );
		mesh->tVerts[i].y = *(pfTextCoords + i*2 + 1);
	}

 	INode *node; node = ip->CreateObjectNode(tri);

	int onlyNamePos; onlyNamePos = strrchr(buf, '\\') - buf + 1;
	char *name; name = (char*)(buf + onlyNamePos);
	*(strrchr(name, '.')) = 0; // Chop off the name
	node->SetName((char *)name);

	TM.IdentityMatrix();
	TM.RotateX(angles[0]);
	TM.RotateY(angles[2]);
	TM.RotateZ(angles[1]);
	TM.Translate(Point3(pos[0],pos[1],pos[2]));

	node->SetNodeTM(0, TM);
	node->NotifyDependents(FOREVER,PART_TM,REFMSG_CHANGE);

	ip->RedrawViews(ip->GetTime());

End:

//	if(!mesh)
//		delete mesh;
//	if(!tri)
//		delete tri;

	if(!pdwIndicaces) free(pdwIndicaces);
	if(!pdwTVIndicaces) free(pdwTVIndicaces);
	if(!pwIndicaces) free(pwIndicaces);
	if(!pwTVIndicaces) free(pwTVIndicaces);
	if(!pfNormals) free(pfNormals);
	if(!pfVerts)free(pfVerts);
	if(!pfTextCoords) free(pfTextCoords);

	fclose(stream);
	return;
}

static VOID ImportOneIndexedDiffuseMesh(Interface *ip, LPCTSTR buf)
{
FILE    *stream;
float   pos[3],scs[3],angles[3],*pfNormals=NULL,*pfVerts=NULL,*pfTextCoords=NULL;
DWORD	dwNumFaces, dwNumVerts;
DWORD	*pdwIndicaces=NULL, *pdwTVIndicaces=NULL,  *pdwCVIndicaces=NULL;
WORD	*pwIndicaces=NULL, *pwTVIndicaces=NULL,  *pwCVIndicaces=NULL;
Mesh	*mesh=NULL; 
Matrix3 TM;
DWORD   *pdwColors=NULL;

	if(!openfile((char*)buf, &stream, "rb"))
	{	MessageBox(NULL, buf, "Faylini ochishda xato.", MB_OK);
		return;
	}
//*********************************************
	char sh[20]; if(!READFILE(sh, 1, 20, stream)){ fclose(stream); return; }
	BOOL b32bit = (sh[3]=='3' && sh[4]=='2');
	BOOL bcl = (sh[5]=='c' && sh[6]=='l');
	if(bcl){MessageBox(NULL, "This is clone only file.", "Can'n!", MB_OK); fclose(stream); return;}
	myMtrl.Read(stream);
//************************************************************************************
	if(!READFILE(&pos[0]  , sizeof(pos[0]), 3, stream)) goto End;
	if(sMyData.fMassh != 0.f)
		{pos[0] /= sMyData.fMassh;pos[1] /= sMyData.fMassh;pos[2] /= sMyData.fMassh;}
	if(!READFILE(&scs[0]   , sizeof(scs[0]), 3 , stream)) goto End;
	if(!READFILE(&angles[0], sizeof(angles[0]), 3, stream))goto End;
//************************************************************************************
	if(!READFILE(&dwNumFaces, sizeof(DWORD), 1, stream)) goto End;
	if(!READFILE(&dwNumVerts, sizeof(DWORD), 1, stream)) goto End;
//************************************************************************************
	float MinMax[6];
	if(!READFILE(&MinMax[0], sizeof(float), 6, stream)) return;
	if(sMyData.fMassh != 0.f)
		{for(int i=0; i<6; i++) MinMax[i] /= sMyData.fMassh;}
//************************************************************************************
	//yangi heap uchun 
	if(b32bit)
	{	pdwIndicaces  = (DWORD*)malloc(2 * dwNumFaces * 3);
		pdwTVIndicaces= (DWORD*)malloc(2 * dwNumFaces * 3);
		pdwCVIndicaces= (DWORD*)malloc(2 * dwNumFaces * 3);
	}
	else
	{	pwIndicaces  = (WORD*)malloc(2 * dwNumFaces * 3);
		pwTVIndicaces= (WORD*)malloc(2 * dwNumFaces * 3);
		pwCVIndicaces= (WORD*)malloc(2 * dwNumFaces * 3);
	}
	pfVerts      = (float*)malloc(sizeof(float) * dwNumVerts * 3);
	pfNormals    = (float*)malloc(sizeof(float) * dwNumVerts * 3);
	pdwColors    = (DWORD*)malloc(sizeof(float) * dwNumVerts);
	pfTextCoords = (float*)malloc(sizeof(float) * dwNumVerts * 2);
//************************************************************************************
	for(DWORD i = 0; i < dwNumVerts; i++)
	{	if(!READFILE(pfVerts    + i * 3, sizeof(float), 3, stream)) goto End;
		if(!READFILE(pfNormals  + i * 3, sizeof(float), 3, stream)) goto End;
		if(!READFILE(pdwColors  + i    , sizeof(DWORD), 1, stream)) goto End;
		if(!READFILE(pfTextCoords+i * 2, sizeof(float), 2, stream)) goto End;
		//DirectX ning OpenGL dan farqini kiritamiz:
		//1.Textura tu,tv larini to'g'rilab chiqamiz:
		*(pfTextCoords+i * 2 + 1) = 1.f - *(pfTextCoords+i * 2 + 1);
	}
	for(DWORD i = 0; i < dwNumFaces; i++)
	{	if(b32bit)
		{	if(!READFILE(pwIndicaces + i * 3, sizeof(DWORD), 3, stream)) goto End;
			*(pdwTVIndicaces + i*3  ) = *(pdwIndicaces + i*3  );
			*(pdwTVIndicaces + i*3+1) = *(pdwIndicaces + i*3+1);
			*(pdwTVIndicaces + i*3+2) = *(pdwIndicaces + i*3+2);
			*(pdwCVIndicaces + i*3  ) = *(pdwIndicaces + i*3  );
			*(pdwCVIndicaces + i*3+1) = *(pdwIndicaces + i*3+1);
			*(pdwCVIndicaces + i*3+2) = *(pdwIndicaces + i*3+2);
		}
		else
		{	if(!READFILE(pwIndicaces + i * 3, sizeof(WORD),3, stream)) goto End;
			*(pwTVIndicaces + i*3  ) = *(pwIndicaces + i*3  );
			*(pwTVIndicaces + i*3+1) = *(pwIndicaces + i*3+1);
			*(pwTVIndicaces + i*3+2) = *(pwIndicaces + i*3+2);
			*(pwCVIndicaces + i*3  ) = *(pwIndicaces + i*3  );
			*(pwCVIndicaces + i*3+1) = *(pwIndicaces + i*3+1);
			*(pwCVIndicaces + i*3+2) = *(pwIndicaces + i*3+2);
	}	}

	DWORD newVerts=0, newTVVerts=0, newCVVerts=0;
	if(b32bit)
	{	newVerts   = WeldVertices(pdwIndicaces, pfVerts, dwNumFaces, dwNumVerts);
		newTVVerts = WeldTVVertices(pdwTVIndicaces, pfTextCoords, dwNumFaces, dwNumVerts);
		newCVVerts = WeldCVVertices(pdwCVIndicaces, pdwColors, dwNumFaces, dwNumVerts);
	}
	else
	{	newVerts   = WeldVertices(pwIndicaces, pfVerts, dwNumFaces, dwNumVerts);
		newTVVerts = WeldTVVertices(pwTVIndicaces, pfTextCoords, dwNumFaces, dwNumVerts);
		newCVVerts = WeldCVVertices(pwCVIndicaces, pdwColors, dwNumFaces, dwNumVerts);
	}
//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************
//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************
//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************
//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************
//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************
//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************
//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************
//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************
//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************
//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************
//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************

	TriObject *tri; tri = CreateNewTriObject();
	if(!tri) goto End;
	mesh = 0; mesh = &tri->GetMesh();
	if(!mesh->setNumFaces(dwNumFaces)) goto End;
	if(!mesh->setNumTVFaces(dwNumFaces))	goto End;
	if(!mesh->setNumVCFaces(dwNumFaces))	goto End;
	if(!mesh->setNumVerts(newVerts)) goto End;
	if(!mesh->setNumTVerts(newTVVerts))	goto End;
	if(!mesh->setNumVertCol(newCVVerts)) goto End;

	mesh->setMapSupport(0, TRUE);
	mesh->setNumMapVerts(0, newCVVerts); 

	//indicase larni joylashtiraman;
	//DirectXda farqi bor, qara:
	for(DWORD i=0; i<dwNumFaces; i++)
	{	if(b32bit)
		{	mesh-> faces[i].v[0]  = *(pdwIndicaces + i*3    );
			mesh-> faces[i].v[1]  = *(pdwIndicaces + i*3 + 2);
			mesh-> faces[i].v[2]  = *(pdwIndicaces + i*3 + 1);

			mesh->tvFace[i].t[0]  = *(pdwTVIndicaces + i*3    );
			mesh->tvFace[i].t[1]  = *(pdwTVIndicaces + i*3 + 2);
			mesh->tvFace[i].t[2]  = *(pdwTVIndicaces + i*3 + 1);

			mesh->vcFace[i].t[0]  = *(pdwCVIndicaces + i*3    );
			mesh->vcFace[i].t[1]  = *(pdwCVIndicaces + i*3 + 2);
			mesh->vcFace[i].t[2]  = *(pdwCVIndicaces + i*3 + 1);		
		}
		else
		{	mesh-> faces[i].v[0]  = *(pwIndicaces + i*3    );
			mesh-> faces[i].v[1]  = *(pwIndicaces + i*3 + 2);
			mesh-> faces[i].v[2]  = *(pwIndicaces + i*3 + 1);

			mesh->tvFace[i].t[0]  = *(pwTVIndicaces + i*3    );
			mesh->tvFace[i].t[1]  = *(pwTVIndicaces + i*3 + 2);
			mesh->tvFace[i].t[2]  = *(pwTVIndicaces + i*3 + 1);

			mesh->vcFace[i].t[0]  = *(pwCVIndicaces + i*3    );
			mesh->vcFace[i].t[1]  = *(pwCVIndicaces + i*3 + 2);
			mesh->vcFace[i].t[2]  = *(pwCVIndicaces + i*3 + 1);		
	}	}
	for(DWORD i=0; i<newVerts; i++) 
	{	mesh->verts[i].x = *(pfVerts + i*3    );
		mesh->verts[i].z = *(pfVerts + i*3 + 1);
		mesh->verts[i].y = *(pfVerts + i*3 + 2);
		if(sMyData.fMassh != 0.f)
		{
			mesh->verts[i].x /= sMyData.fMassh;
			mesh->verts[i].y /= sMyData.fMassh;
			mesh->verts[i].z /= sMyData.fMassh;
	}	}
	for(DWORD i=0; i<newTVVerts; i++) 
	{	mesh->tVerts[i].x = *(pfTextCoords + i*2    );
		mesh->tVerts[i].y = *(pfTextCoords + i*2 + 1);
	}
	for(DWORD i=0; i<newCVVerts; i++) 
	{	//Dword dan 3ta chiqaramiza:
		mesh->vertCol[i].x = (float)(((*(pdwColors + i*3)) & 0x00ff0000) >> 16) / 255.0f;//r
		mesh->vertCol[i].y = (float)(((*(pdwColors + i*3)) & 0x0000ff00) >> 8 ) / 255.0f;
		mesh->vertCol[i].z = (float)(((*(pdwColors + i*3)) & 0x000000ff)      ) / 255.0f;
	}

 	INode *node; node = ip->CreateObjectNode(tri);

	int onlyNamePos; onlyNamePos = strrchr(buf, '\\') - buf + 1;
	char *name; name = (char*)(buf + onlyNamePos);
	*(strrchr(name, '.')) = 0; // Chop off the name
	node->SetName((char *)name);

	TM.IdentityMatrix();
	TM.RotateX(angles[0]);
	TM.RotateY(angles[2]);
	TM.RotateZ(angles[1]);
	TM.Translate(Point3(pos[0],pos[1],pos[2]));

	node->SetNodeTM(0, TM);
	node->NotifyDependents(Interval(0,0), PART_VERTCOLOR & PART_EXCLUDE_RADIOSITY, REFMSG_CHANGE);
	node->NotifyDependents(Interval(0,0), PART_TOPO & PART_EXCLUDE_RADIOSITY, REFMSG_CHANGE);
	node->SetCVertMode(1);
	tri->UpdateValidity(VERT_COLOR_CHAN_NUM, Interval(0,0));

	ip->RedrawViews(ip->GetTime());

End:

//	if(!mesh)
//		delete mesh;
//	if(!tri)
//		delete tri;

	if(!pdwIndicaces) free(pdwIndicaces);
	if(!pdwTVIndicaces) free(pdwTVIndicaces);
	if(!pdwCVIndicaces) free(pdwCVIndicaces);
	if(!pwIndicaces) free(pwIndicaces);
	if(!pwTVIndicaces) free(pwTVIndicaces);
	if(!pwCVIndicaces) free(pwCVIndicaces);
	if(!pfNormals) free(pfNormals);
	if(!pfVerts) free(pfVerts);
	if(!pfTextCoords) free(pfTextCoords);
	if(!pdwColors) free(pdwColors);

	fclose(stream);
	return;
}

static VOID ImportOneDiffuseMesh(Interface *ip, LPCTSTR buf)
{
Matrix3 TM;
Mesh	*mesh; 
FILE    *stream;
DWORD	dwNumFaces,dwNumVerts;
DWORD	*pdwIndicaces = NULL,*pdwTVIndicaces = NULL;
WORD	*pwIndicaces = NULL,*pwTVIndicaces = NULL;
float   pos[3],scs[3],angles[3],*pfNormals = NULL,
		*pfVerts = NULL,*pfTextCoords = NULL;

	if(!openfile((char*)buf, &stream  ,"rb"))
	{	MessageBox(NULL, buf, "Faylini ochishda xato.", MB_OK);
		return;
	}
//*********************************************
	char sh[20]; if(!READFILE(sh, 1, 20, stream)){ fclose(stream); return; }
	BOOL b32bit = (sh[3]=='3' && sh[4]=='2');
	BOOL bcl = (sh[5]=='c' && sh[6]=='l');
	if(bcl){MessageBox(NULL, "This is clone only file.", "Can'n!", MB_OK); fclose(stream); return;}
	myMtrl.Read(stream);
//************************************************************************************
	if(!READFILE(&pos[0]   , sizeof(pos[0]), 3, stream)) goto End;
	if(sMyData.fMassh != 0.f){pos[0] /= sMyData.fMassh;pos[1] /= sMyData.fMassh;pos[2] /= sMyData.fMassh;}
	if(!READFILE(&scs[0]   , sizeof(scs[0]), 3, stream)) goto End;
	if(!READFILE(&angles[0], sizeof(angles[0]), 3, stream))goto End;
//************************************************************************************
	if(!READFILE(&dwNumFaces, sizeof(DWORD), 1, stream)) goto End;
	dwNumVerts = dwNumFaces*3;
//************************************************************************************
	float MinMax[6];
	if(!READFILE(&MinMax[0], sizeof(float), 6, stream)) return;
	if(sMyData.fMassh != 0.f){for(int i=0; i<6; i++) MinMax[i] /= sMyData.fMassh;}
//************************************************************************************
	//yangi heap uchun
	if(b32bit)
	{	pdwIndicaces   = (DWORD*)malloc(4 * dwNumFaces * 3);
		pdwTVIndicaces = (DWORD*)malloc(4 * dwNumFaces * 3);
	}
	else
	{	pwIndicaces   = (WORD*)malloc(2 * dwNumFaces * 3);
		pwTVIndicaces = (WORD*)malloc(2 * dwNumFaces * 3);
	}
	pfNormals     = (float*)malloc(sizeof(float) * dwNumVerts * 3);
	pfVerts       = (float*)malloc(sizeof(float) * dwNumVerts * 3);
	pfTextCoords  = (float*)malloc(sizeof(float) * dwNumVerts * 2);
	DWORD color;//1 ta color, ya'ni biz uni hisobga olmaymiz.Hozircha albatta,31.03.2011;
//************************************************************************************
	for(DWORD i = 0; i < dwNumFaces*3; i++)
	{	if(!READFILE(pfVerts    + i * 3, sizeof(float), 3, stream)) goto End;
		if(!READFILE(pfNormals  + i * 3, sizeof(float), 3, stream)) goto End;
		if(!READFILE(&color			   , sizeof(DWORD), 1, stream)) goto End;
		if(!READFILE(pfTextCoords+i * 2, sizeof(float), 2, stream)) goto End;
		//DirectX ning OpenGL dan farqini kiritamiz:
		//1.Textura tu,tv larini to'g'rilab chiqamiz:
		*(pfTextCoords+i * 2 + 1) = 1.f - *(pfTextCoords+i * 2 + 1);
	}
	for(DWORD i = 0; i < dwNumFaces; i++)
	{	if(b32bit)
		{	*(pdwIndicaces   + i*3  ) = i*3;
			*(pdwIndicaces   + i*3+1) = i*3+1;
			*(pdwIndicaces   + i*3+2) = i*3+2;
			*(pdwTVIndicaces + i*3  ) = i*3;
			*(pdwTVIndicaces + i*3+1) = i*3+1;
			*(pdwTVIndicaces + i*3+2) = i*3+2;
		}
		else
		{	*(pwIndicaces   + i*3  ) = i*3  ;
			*(pwIndicaces   + i*3+1) = i*3+1;
			*(pwIndicaces   + i*3+2) = i*3+2;
			*(pwTVIndicaces + i*3  ) = i*3  ;
			*(pwTVIndicaces + i*3+1) = i*3+1;
			*(pwTVIndicaces + i*3+2) = i*3+2;
	}	}

	//if(!READFILE(&dwNumVerts, sizeof(DWORD), 1, stream)) goto End;
	
	DWORD newVerts=0, newTVVerts=0;// 0 edi;
	if(b32bit)
	{	newVerts = WeldVertices(pdwIndicaces, pfVerts, dwNumFaces, dwNumVerts);
		newTVVerts = WeldTVVertices(pdwTVIndicaces, pfTextCoords, dwNumFaces, dwNumVerts);
	}
	else
	{	newVerts = WeldVertices(pwIndicaces, pfVerts, dwNumFaces, dwNumVerts);
		newTVVerts = WeldTVVertices(pwTVIndicaces, pfTextCoords, dwNumFaces, dwNumVerts);
	}

//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************
//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************
//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************
//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************
//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************
//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************
//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************
//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************
//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************
//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************
//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************

	TriObject *tri; tri = CreateNewTriObject();
	if(!tri) goto End;

	mesh = &tri->GetMesh();
	if(!mesh->setNumFaces(dwNumFaces)) goto End;
	if(!mesh->setNumTVFaces(dwNumFaces)) goto End;
	if(!mesh->setNumVerts(newVerts)) goto End;
	if(!mesh->setNumTVerts(newTVVerts)) goto End;

	//indicase larni joylashtiraman;
	//DirectXda farqi bor, qara:
	for(DWORD i=0; i<dwNumFaces; i++) 
	{// Maxda feys yo'nalishi teskaridur, shuning uchun avval +2, so'ng +1.
		if(b32bit)
		{	mesh-> faces[i].v[0]  = *(pdwIndicaces + i*3    );
			mesh-> faces[i].v[1]  = *(pdwIndicaces + i*3 + 2);
			mesh-> faces[i].v[2]  = *(pdwIndicaces + i*3 + 1);

			mesh->tvFace[i].t[0]  = *(pdwTVIndicaces + i*3    );
			mesh->tvFace[i].t[1]  = *(pdwTVIndicaces + i*3 + 2);
			mesh->tvFace[i].t[2]  = *(pdwTVIndicaces + i*3 + 1);
		}
		else
		{	mesh-> faces[i].v[0]  = *(pwIndicaces + i*3    );
			mesh-> faces[i].v[1]  = *(pwIndicaces + i*3 + 2);
			mesh-> faces[i].v[2]  = *(pwIndicaces + i*3 + 1);

			mesh->tvFace[i].t[0]  = *(pwTVIndicaces + i*3    );
			mesh->tvFace[i].t[1]  = *(pwTVIndicaces + i*3 + 2);
			mesh->tvFace[i].t[2]  = *(pwTVIndicaces + i*3 + 1);
	}	}
	for(DWORD i=0; i<newVerts; i++) 
	{	mesh->verts[i].x = *(pfVerts + i*3    );
		mesh->verts[i].z = *(pfVerts + i*3 + 1);
		mesh->verts[i].y = *(pfVerts + i*3 + 2);
		if(sMyData.fMassh != 0.f)
		{	mesh->verts[i].x /= sMyData.fMassh;
			mesh->verts[i].y /= sMyData.fMassh;
			mesh->verts[i].z /= sMyData.fMassh;
	}	}
	for(DWORD i=0; i<newTVVerts; i++) 
	{	mesh->tVerts[i].x = *(pfTextCoords + i*2    );
		mesh->tVerts[i].y = *(pfTextCoords + i*2 + 1);
	}

 	INode *node; node = ip->CreateObjectNode(tri);
	myMtrl.Set(node);

	int onlyNamePos; onlyNamePos = strrchr(buf, '\\') - buf + 1;
	char *name; name = (char*)(buf + onlyNamePos);
	*(strrchr(name, '.')) = 0; // Chop off the name
	node->SetName((char *)name);

	TM.IdentityMatrix();
	TM.RotateX(angles[0]);
	TM.RotateY(angles[2]);
	TM.RotateZ(angles[1]);
	TM.Translate(Point3(pos[0],pos[1],pos[2]));

	node->SetNodeTM(0, TM);
	node->NotifyDependents(FOREVER,PART_TM,REFMSG_CHANGE);

	ip->RedrawViews(ip->GetTime());

End:

//	if(!mesh)
//		delete mesh;
//	if(!tri)
//		delete tri;

	if(!pdwIndicaces) free(pdwIndicaces);
	if(!pdwTVIndicaces) free(pdwTVIndicaces);
	if(!pwIndicaces) free(pwIndicaces);
	if(!pwTVIndicaces) free(pwTVIndicaces);
	if(!pfNormals) free(pfNormals);
	if(!pfVerts) free(pfVerts);
	if(!pfTextCoords) free(pfTextCoords);

	fclose(stream);
	return;
}

static VOID ImportOneSkin(Interface *ip, LPCTSTR buf)
{
FILE    *stream;
float   pos[3],angles[3],*pfNormals=NULL,*pfVertices=NULL,*pfTextCoords=NULL, MeshTM[16];
DWORD	dwNumFaces,dwNumVerts;
WORD	wNumBones, *pwIndicaces=NULL,*pwTVIndicaces=NULL;
DWORD	*pdwIndicaces=NULL,*pdwTVIndicaces=NULL;
Mesh	*mesh=0;
TriObject *trii=NULL,*tri=NULL;
Matrix3 TM;

	if(!openfile((char*)buf, &stream  ,"rb"))
	{	MessageBox(NULL, buf, "Faylini ochishda xato.", MB_OK);
		return;
	}
//*********************************************
	char sh[20]; if(!READFILE(sh, 1, 20, stream)){ fclose(stream); return; }
	BOOL b32bit = (sh[3]=='3' && sh[4]=='2');
	BOOL bcl = (sh[5]=='c' && sh[6]=='l');
	if(bcl){MessageBox(NULL, "This is clone only file.", "Can'n!", MB_OK); fclose(stream); return;}
//************************************************************************************
	if(!READFILE(&pos[0], sizeof(pos[0]), 3, stream)) goto End;
	if(sMyData.fMassh != 0.f)
		{pos[0] /= sMyData.fMassh;pos[1] /= sMyData.fMassh;pos[2] /= sMyData.fMassh;}
	if(!READFILE(&angles[0], sizeof(angles[0]), 3, stream))goto End;
//************************************************************************************
	if(!READFILE(&dwNumFaces, sizeof(DWORD), 1, stream)) goto End;
	if(!READFILE(&dwNumVerts, sizeof(DWORD), 1, stream)) goto End;
//************************************************************************************
	float MinMax[6];
	if(!READFILE(&MinMax[0], sizeof(float), 6, stream)) return;
	if(sMyData.fMassh != 0.f)
		{for(int i=0; i<6; i++) MinMax[i] /= sMyData.fMassh;}
//************************************************************************************
	if(!READFILE(&wNumBones, sizeof( wNumBones ), 1, stream)) return;
	char **pBoneNames = (char**)malloc(4* wNumBones);
	char boneName[256]; int l;
	for(WORD b=0; b<wNumBones; b++)
	{	fscanf(stream, boneName);
		l = lstrlen(boneName);
		pBoneNames[b] = (C8*)malloc(l+1);
		lstrcpy(pBoneNames[b], boneName);
	}
	if(!READFILE(&MeshTM[0], sizeof(F32), 16, stream)) return;
	float *pmBoneInitTMs = new float[16*wNumBones];
	for(S32 b=0; b<wNumBones; b++)
	{if(!READFILE(&pmBoneInitTMs[b*16], sizeof(F32), 16, stream)) return;}
//************************************************************************************
	//yangi heap uchun 
	if(b32bit)
	{	pdwIndicaces = (DWORD*)malloc(4 * dwNumFaces * 3);
		pdwTVIndicaces = (DWORD*)malloc(4 * dwNumFaces * 3);
	}
	else
	{	pwIndicaces = (WORD*)malloc(2 * dwNumFaces * 3);
		pwTVIndicaces = (WORD*)malloc(2 * dwNumFaces * 3);
	}
	pfNormals   = (float*)malloc(sizeof(float) * dwNumVerts * 3);
	pfVertices  = (float*)malloc(sizeof(float) * dwNumVerts * 3);
	pfTextCoords = (float*)malloc(sizeof(float)* dwNumVerts * 2);
//************************************************************************************

	for(DWORD i = 0; i < dwNumVerts; i++)
	{	if(!READFILE(pfVertices  + i * 3, sizeof(float), 3, stream)) goto End;
		if(!READFILE(pfNormals   + i * 3, sizeof(float), 3, stream)) goto End;
		if(!READFILE(pfTextCoords+ i * 2, sizeof(float), 2, stream)) goto End;
		//DirectX ning OpenGL dan farqini kiritamiz:
		*(pfTextCoords+i * 2 + 1) = 1.f - *(pfTextCoords+i * 2 + 1);
	}

	for(DWORD i = 0; i < dwNumFaces; i++)
	{	if(b32bit)
		{	if(!READFILE(pdwIndicaces + i * 3, sizeof(float), 3, stream)) goto End;
			*(pdwTVIndicaces + i*3  ) = *(pdwIndicaces + i*3  );
			*(pdwTVIndicaces + i*3+1) = *(pdwIndicaces + i*3+1);
			*(pdwTVIndicaces + i*3+2) = *(pdwIndicaces + i*3+2);
		}
		else
		{	if(!READFILE(pwIndicaces + i * 3, sizeof(float), 3, stream)) goto End;
			*(pwTVIndicaces + i*3  ) = *(pwIndicaces + i*3  );
			*(pwTVIndicaces + i*3+1) = *(pwIndicaces + i*3+1);
			*(pwTVIndicaces + i*3+2) = *(pwIndicaces + i*3+2);
	}	}
		
	DWORD newVerts=0, newTVVerts=0;
	if(b32bit)
	{	newVerts = WeldVertices(pdwIndicaces, pfVertices, dwNumFaces, dwNumVerts);
		newTVVerts = WeldTVVertices(pdwTVIndicaces, pfTextCoords, dwNumFaces, dwNumVerts);
	}
	else
	{	newVerts = WeldVertices(pwIndicaces, pfVertices, dwNumFaces, dwNumVerts);
		newTVVerts = WeldTVVertices(pwTVIndicaces, pfTextCoords, dwNumFaces, dwNumVerts);
	}

//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************
//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************
//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************
//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************
//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************
//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************
//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************
//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************
//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************
//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************
//****************** CONSTRUCT MESH TO THE MAX SCENE *********************************

	tri = CreateNewTriObject();
	if(!tri) goto End;
	mesh = 0; mesh = &tri->GetMesh();
	if(!mesh->setNumFaces(dwNumFaces)) goto End;
	if(!mesh->setNumTVFaces(dwNumFaces))	goto End;
	if(!mesh->setNumVerts(newVerts)) goto End;
	if(!mesh->setNumTVerts(newTVVerts))	goto End;

	//indicase larni joylashtiraman;
	for(DWORD i=0; i<dwNumFaces; i++)
	{	if(b32bit)
		{	mesh-> faces[i].v[0]  = *(pdwIndicaces + i*3    );
			mesh-> faces[i].v[1]  = *(pdwIndicaces + i*3 + 2);
			mesh-> faces[i].v[2]  = *(pdwIndicaces + i*3 + 1);

			mesh->tvFace[i].t[0]  = *(pdwTVIndicaces + i*3    );
			mesh->tvFace[i].t[1]  = *(pdwTVIndicaces + i*3 + 2);
			mesh->tvFace[i].t[2]  = *(pdwTVIndicaces + i*3 + 1);
		}
		else
		{	mesh-> faces[i].v[0]  = *(pwIndicaces + i*3    );
			mesh-> faces[i].v[1]  = *(pwIndicaces + i*3 + 2);
			mesh-> faces[i].v[2]  = *(pwIndicaces + i*3 + 1);

			mesh->tvFace[i].t[0]  = *(pwTVIndicaces + i*3    );
			mesh->tvFace[i].t[1]  = *(pwTVIndicaces + i*3 + 2);
			mesh->tvFace[i].t[2]  = *(pwTVIndicaces + i*3 + 1);
	}	}
	for(DWORD i=0; i<newVerts; i++) 
	{	mesh->verts[i].x = *(pfVertices + i*3    );
		mesh->verts[i].z = *(pfVertices + i*3 + 2);
		mesh->verts[i].y = *(pfVertices + i*3 + 1);
		if(sMyData.fMassh != 0.f)
		{	mesh->verts[i].x /= sMyData.fMassh;
			mesh->verts[i].y /= sMyData.fMassh;
			mesh->verts[i].z /= sMyData.fMassh;
	}	}
	for(DWORD i=0; i<newTVVerts; i++) 
	{	mesh->tVerts[i].x = *(pfTextCoords + i*2    );
		mesh->tVerts[i].y = *(pfTextCoords + i*2 + 1);
	}

//	PatchObject *pch; pch = (PatchObject *)tri->ConvertToType(ip->GetTime(),Class_ID(PATCHOBJ_CLASS_ID , 0) );
	trii = (TriObject *)tri->ConvertToType(ip->GetTime(),Class_ID(TRIOBJ_CLASS_ID, 0) );

 	INode *node; node = ip->CreateObjectNode(trii);

	int onlyNamePos; onlyNamePos = strrchr(buf, '\\') - buf + 1;
	char *name; name = (char*)(buf + onlyNamePos);
	*(strrchr(name, '.')) = 0; // Chop off the name
	node->SetName((char *)name);

	TM.IdentityMatrix();
	TM.RotateX(-angles[0]);
	TM.RotateY(-angles[1]);
	TM.RotateZ(-angles[2]);
	TM.Translate(Point3(pos[0],pos[1],pos[2]));

	node->SetNodeTM(0, TM);
	node->NotifyDependents(FOREVER,PART_TM,REFMSG_CHANGE);

	ip->RedrawViews(ip->GetTime());

End:

	if(!mesh) delete mesh;
	if(!tri) delete tri;
//	if(!pch) delete pch;
	if(!trii) delete trii;
	if(!pdwIndicaces) free(pdwIndicaces);
	if(!pdwIndicaces) free(pdwTVIndicaces);
	if(!pwIndicaces) free(pwIndicaces);
	if(!pwIndicaces) free(pwTVIndicaces);
	if(!pfNormals) free(pfNormals);
	if(!pfVertices)	free(pfVertices);
	if(!pfTextCoords) free(pfTextCoords);

	fclose(stream);
}

VOID ImportMesh(Interface *ip)
{
OPENFILENAME	ofn;
TCHAR			sfile[MAX_BUF];

static int iSelect = 0;
	memset(&ofn, 0, sizeof (OPENFILENAME));
	memset(sfile,0, MAX_BUF);
	ofn.lStructSize = sizeof(OPENFILENAME);
	if(0==iSelect)
	 ofn.lpstrFilter = TEXT("All indexed mesh Files (*.msi)\0*.msi\0All mesh Files (*.msh)\0*.msh\0All \0*.*\0");
	else
	 ofn.lpstrFilter = TEXT("All mesh Files (*.msh)\0*.msh\0All indexed mesh Files (*.msi)\0*.msi\0All \0*.*\0");
	ofn.lpstrTitle  = TEXT("Open import mesh file.");
	ofn.lpstrFile   = sfile;
	ofn.Flags       = OFN_EXPLORER | OFN_ALLOWMULTISELECT;
	ofn.nMaxFile    = MAX_BUF;
	if(!GetOpenFileName(&ofn))
		return;

	//0 dan to max BUF gacha 0 ni 20, probel qilib chiqaman:
	char *pt = sfile;
	for(int i=0; i<MAX_BUF-1; i++)
	{	if(sfile[i] == 0)
		{	__try
			{	if(strstr(pt, ".msi"))
				{	ImportOneIndexedMesh(ip, pt);
					iSelect = 0;
				}
				else
				{	ImportOneMesh(ip, pt);
					iSelect = 1;
			}	}
			__except(1,1)
			{
				MessageBox(NULL, "Kutilmagan gen. xato", "Diqqat!!!", MB_OK);
			}
			if(sfile[i+1]==0)
				break;
			pt = &sfile[i+1];
	}	}
	return;
}

VOID ImportTngntMesh(Interface *ip)
{
OPENFILENAME	ofn;
TCHAR			sfile[MAX_BUF];

	memset(&ofn, 0, sizeof (OPENFILENAME));
	memset(sfile,0, MAX_BUF);
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.lpstrFilter = TEXT("All mesh Files (*.mst)\0*.mst\0All \0*.*\0");  
	ofn.lpstrTitle  = TEXT("Open import mesh file.");
	ofn.lpstrFile   = sfile;
	ofn.Flags       = OFN_EXPLORER | OFN_ALLOWMULTISELECT;
	ofn.nMaxFile    = MAX_BUF;
	if(!GetOpenFileName(&ofn))
		return;

	char *pt = sfile;
	for(int i=0; i<MAX_BUF-1; i++)
	{	if(sfile[i] == 0)
		{	__try
			{	ImportOneTngntMesh(ip, pt);
			}
			__except(1,1)
			{
				MessageBox(NULL, "Kutilmagan gen. xato", "Diqqat!!!", MB_OK);
			}
			if(sfile[i+1]==0)
				break;
			pt = &sfile[i+1];
	}	}
	return;
}

VOID ImportDiffuseMesh(Interface *ip)
{
OPENFILENAME	ofn;
TCHAR			sfile[MAX_BUF];
static int iSelect = 0;

	memset(&ofn, 0, sizeof (OPENFILENAME));
	memset(sfile,0, MAX_BUF);
	ofn.lStructSize = sizeof(OPENFILENAME);
	if(0==iSelect)
	 ofn.lpstrFilter = TEXT("All indexed diffuse files (*.dmsi;)\0*.dmsi;\0All diffuse files (*.dmsh;)\0*.dmsh;\0All \0*.*\0");
	else
	 ofn.lpstrFilter = TEXT("All indexed mesh Files (*.msi)\0*.msi\0All mesh Files (*.msh)\0*.msh\0All \0*.*\0");
	ofn.lpstrTitle  = TEXT("Open import diffuse file.");
	ofn.lpstrFile   = sfile;
	ofn.Flags       = OFN_EXPLORER | OFN_ALLOWMULTISELECT;
	ofn.nMaxFile    = MAX_BUF;
	if(!GetOpenFileName(&ofn))
		return;

	//0 dan to max BUF gacha 0 ni 20, probel qilib chiqaman:
	char *pt = sfile;
	for(int i=0; i<MAX_BUF-1; i++)
	{	if(sfile[i] == 0)
		{	__try
			{	if(strstr(pt, ".dmsi"))
				{	ImportOneIndexedDiffuseMesh(ip, pt);
					iSelect = 0;
				}
				else
				{	ImportOneDiffuseMesh(ip, pt);
					iSelect = 1;
			}	}
			__except(1,1)
			{
				MessageBox(NULL, "Kutilmagan gen. xato", "Diqqat!!!", MB_OK);
			}
			if(sfile[i+1]==0)
				break;
			pt = &sfile[i+1];
	}	}
	return;
}

VOID ImportSkin(Interface *ip)
{
OPENFILENAME	ofn;
TCHAR			sfile[MAX_BUF];

	memset(&ofn, 0, sizeof (OPENFILENAME));
	memset(sfile,0, MAX_BUF);
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.lpstrFilter = TEXT("All skin files (*.skn)\0*.skn\0All \0*.*\0");
	ofn.lpstrTitle  = TEXT("Open import skin file.");
	ofn.lpstrFile   = sfile;
	ofn.Flags       = OFN_EXPLORER | OFN_ALLOWMULTISELECT;
	ofn.nMaxFile    = MAX_BUF;
	if(!GetOpenFileName(&ofn))
		return;

	//0 dan to max BUF gacha 0 ni 20, probel qilib chiqaman:
	char *pt = sfile;
	for(int i=0; i<MAX_BUF-1; i++)
	{	if(sfile[i] == 0)
		{	__try
			{	ImportOneSkin(ip, pt);
			}
			__except(1,1)
			{
				MessageBox(NULL, "Kutilmagan gen. xato", "Diqqat!!!", MB_OK);
			}
			if(sfile[i+1]==0)
				break;
			pt = &sfile[i+1];
	}	}
	return;
}