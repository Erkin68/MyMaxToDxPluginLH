//BOOL DeleteWeightsBelovePercFrSkin2Morph(char *pathAndName, int percent)
//BOOL DetachOneWeightFacesFrSkin2Morph(char *pathAndName)
//BOOL ConvertSkin2MorphToSHFormat(char *pathAndName)
//BOOL ConvertSoftSkin2MorphToIndexedFormat(char *pathAndName, F32 delta)
//BOOL ConvertIndexedSkin2MorphToSHFormat(char *pathAndName)
//BOOL SoftSkin2MorphToText(char *pathAndName)
//BOOL SHSkin2MorphToText(char *pathAndName)
//BOOL SoftSkin2MorphIndexedToText(char *pathAndName)
//BOOL SHSkin2MorphIndexedToText(char *pathAndName)
//BOOL SortForTextrIndexedSkin2Morph(char *pathAndName)
//BOOL Skin2MorphShadowToText(char *pathAndName)
//BOOL SHSkin2MorphTBIndexedToText(char *pathAndName)
//int  ConvertIndexedShaderSkin2MorphToF3_UB4N_F16_2Format(char *NAME)
//int  ConvertIndexedShaderSkin2MorphTBToF3_UB4N_F16_2Format(char *NAME)
//BOOL RenameFile(char *name, S32 posInName, char c)
//BOOL ConvertSHKin2MorphToSHSkin2MorphTangentAndBinormal(char *NAME)
//BOOL ConvertSHKin2MorphToSHSkin2MorphTangentAndBinormalUseDX(char *NAME)

#define MAX_SKIN2MORPH_CHANNEL 11
#define NEW(type, name, sz) type *name = NULL; if(sz) name = (type*)malloc(sz);
#define RENEW(type, name, sz) name = NULL; if(sz) name = (type*)malloc(sz);
#define DEL(name) if(name) free(name); name = NULL;

#include <stdio.h>
#include <math.h>
#include "windows.h"
#include "D3DX10Math.h"
#include "d3dx9mesh.h"
#include "mPoint.h"
#include "..\..\game SDK\Main\typeswin32.h"

typedef FLOAT* (__stdcall *D3DXFloat16To32Array_t)(FLOAT*,CONST D3DXFLOAT16*,UINT);
extern D3DXFloat16To32Array_t myD3DXFloat16To32Array;
typedef D3DXFLOAT16* (__stdcall *D3DXFloat32To16Array_t)(D3DXFLOAT16*,CONST FLOAT*,UINT);
extern D3DXFloat32To16Array_t myD3DXFloat32To16Array;
typedef HRESULT (__stdcall *D3DXComputeTangentFrameEx_t)(ID3DXMesh*,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,CONST DWORD*,FLOAT,FLOAT,FLOAT,ID3DXMesh**,ID3DXBuffer**);
extern D3DXComputeTangentFrameEx_t myD3DXComputeTangentFrameEx;
typedef HRESULT (__stdcall *D3DXCreateMesh_t)(DWORD,DWORD,DWORD,CONST LPD3DVERTEXELEMENT9*,LPDIRECT3DDEVICE9,LPD3DXMESH*);
extern D3DXCreateMesh_t myD3DXCreateMesh;
typedef IDirect3D9* (__stdcall *Direct3DCreate9_t)(UINT);
extern Direct3DCreate9_t myDirect3DCreate9;

BOOL DetachMrpFacesInSoftSkin2MorphIndexed(char*);
BOOL SortForTextrIndexedSkin2Morph(char*);
int  ConvertMeshToIndexedFormat(char *pathAndName, float delta);

#pragma warning(disable : 4244)
#pragma warning(disable : 4996)

static char	Name[128];

//Weightlarni bonelar aro taqsimlashda percent dan kichik weightlarni yo'qotib, 
//qolganlariga taqsimlash. Bu Index *.smi emas, s2m. smi ga aylantirguncha bajarish kerak; 
//Test pasted successfully;
BOOL DeleteWeightsBelovePercFrSkin2Morph(char *pathAndName, int percent)
{
FILE	*fIn;
fpos_t	posWeights, posMrp, posEnd;
DWORD   dwNumFaces, szWeights, szMrp;
char 	maxAssignBone = 0;
float   fPercent = (float)percent * 0.01f;

	if(fPercent < 0.01f) return TRUE;

	lstrcpy(Name, pathAndName);
	fIn = fopen(Name, "r+b");
	if(!fIn) return FALSE;

	char sh[20];
	fread(sh, 1, 20, fIn);
	if(!(sh[0]=='S'&&sh[1]=='k'&&sh[2]=='n'&&sh[3]=='2'&&sh[4]=='m'&&sh[5]=='r'&&sh[6]=='p'))
	{	fclose(fIn);
		return FALSE;
	}
	BOOL b32bit = (sh[7] == '3' && sh[8] == '2');

	fseek(fIn, 3*sizeof(float), SEEK_CUR);//float pos[3];
	fseek(fIn, 3*sizeof(float), SEEK_CUR);//float eul[3];
	fread(&dwNumFaces, sizeof(DWORD), 1, fIn);
	fseek(fIn, 6*sizeof(float), SEEK_CUR);//fMinMax[6];
	fseek(fIn, dwNumFaces*3*32 , SEEK_CUR);//pVertices;
	WORD wNumBones;
	fread(&wNumBones, sizeof(WORD), 1, fIn);
	for(WORD b=0; b<wNumBones; b++)//pBoneNames
	{	char n[4]; n[0] = 32;
		while(n[0]) fread(n, 1, 1, fIn);
	}
	fseek(fIn, 16*4, SEEK_CUR);//MeshTM
	fseek(fIn, wNumBones*16*4, SEEK_CUR);//pmBoneInitTMs;
	fgetpos(fIn, &posWeights);
	fread(&szWeights, sizeof(szWeights), 1, fIn);
	NEW(unsigned char, pSz, szWeights)
	unsigned char *pSzC = pSz;

	//Deallocating weights:
	for(DWORD v=0; v<3*dwNumFaces; v++)
	{	//if(v==44)//89148)
		//	Beep(100,100);
		WORD numBones; fread(&numBones, sizeof(WORD), 1, fIn);
		WORD *whichBone = new WORD [numBones];
		BOOL *detBits = new BOOL [numBones];
		float *weights  = new float [numBones];
		float distrWeight = 0;
		int iDistr = 0;
        for(WORD b=0; b<numBones; b++)
		{	fread(&whichBone[b], sizeof(WORD), 1, fIn);
			fread(&weights[b], sizeof(float), 1, fIn);
			if((weights[b] < fPercent) &&(numBones > 4))//>4 ni yangi kiritdim, 21.08.2010 yil;
			{	detBits[b] = TRUE;
				distrWeight += weights[b];
				iDistr++;
			} else detBits[b] = FALSE;
		}
		//Redistribute:
		if(iDistr>0)
		{	float deweight = distrWeight / (numBones - iDistr);
			for(WORD b=0; b<numBones; b++)
			{	if(!detBits[b]) weights[b] += deweight;
		}	}
		//Writing to mem. in pSz:
		numBones -= iDistr;
		*((WORD*)pSzC) = numBones; pSzC += 2;//fwrite(&numBones, sizeof(WORD), 1, fIn);
		if(numBones > maxAssignBone) maxAssignBone = numBones;
		for(WORD b=0; b<numBones+iDistr; b++)
		{	if(!detBits[b])
			{	*((WORD*)pSzC) = whichBone[b]; pSzC += 2;//fwrite(&whichBone[b], sizeof(WORD), 1, fIn);
				*((float*)pSzC) = weights[b];  pSzC += 4;//fread(&weights[b], sizeof(float), 1, fIn);
		}	}
		delete [] whichBone;
		delete [] detBits;
		delete [] weights;
	}
	char mAsBn;	fread(&mAsBn, sizeof(mAsBn), 1, fIn);

	//Saving other part of file fr. this pos. to end:
	fgetpos(fIn, &posMrp);
	fseek(fIn, 0, SEEK_END);
	fgetpos(fIn, &posEnd);
	szMrp = posEnd - posMrp;
	fseek(fIn, posMrp, SEEK_SET);
	NEW(VOID, pMrp, szMrp)
	fread(pMrp, 1, szMrp, fIn);

	//Writing correct weights from mem in pSz:
	fseek(fIn, posWeights, SEEK_SET);
	szWeights = pSzC - pSz;
	fwrite(&szWeights, 4, 1, fIn);
	fwrite(pSz, 1, szWeights, fIn);

	//Saving end of file for the defining how bytes we must write:
	fgetpos(fIn, &posWeights);

	//Reallocate mem. buf. for all the corrected file size:
	DEL(pSz)
	RENEW(unsigned char, pSz, posWeights)
	fseek(fIn, 0, SEEK_SET);

	//Reading corrected data:
	fread (pSz, 1, posWeights, fIn);
	fclose(fIn);

	//Open new file:
	fIn = fopen(Name, "wb");
	fwrite(pSz, 1, posWeights, fIn);
	fwrite(&maxAssignBone, 1, 1, fIn);

	//Writing other morph part:
	fwrite(pMrp, 1, szMrp, fIn);

	fclose(fIn);

	DEL(pSz)
	DEL(pMrp)
	return TRUE;
}

//Skin dan 1 ta feys uchun 3ta vertiga ham weighti 1.0f bo'lgan bone berkitilgan bo'lsa
//ularni *.msh qilib detach qilish. Jami MAX(wNumBone) ta *.msh detach qilinadi. Bu ham
//DrawPrim orqali chizib tez ishlatish uchun.
//Test pasted successfully;  oxirida to indexed bo'ldi, ConvertMeshToIndexedFormat b-n;
//Hali UB4F16_2 ga o'tganicha yoq';
BOOL DetachOneWeightFacesFrSkin2Morph(char *pathAndName)
{
BYTE	*pCompVerts, *pv, *pBones, *pCompWeights, *pMrp, *pw;
fpos_t	posVerts, posBones, posWeights, posMrp, posEnd;
char 	maxAssignBone = 0;
DWORD   dwNumFaces;
FILE	*fIn;
DWORD   sz, szMrp;


	lstrcpy(Name, pathAndName);
	fIn = fopen(Name, "r+b");
	if(!fIn) return FALSE;

	char sh[20];
	fread(sh, 1, 20, fIn);
	if(!(sh[0]=='S'&&sh[1]=='k'&&sh[2]=='n'&&sh[3]=='2'&&sh[4]=='m'&&sh[5]=='r'&&sh[6]=='p'))
	{	fclose(fIn);
		return FALSE;
	}
	BOOL b32bit = (sh[7] == '3' && sh[8] == '2');

	fseek(fIn, 3*sizeof(float), SEEK_CUR);//float pos[3];
	fseek(fIn, 3*sizeof(float), SEEK_CUR);//float eul[3];
	fread(&dwNumFaces, sizeof(DWORD), 1, fIn);
	fseek(fIn, 6*sizeof(float), SEEK_CUR);//fMinMax[6];
	fgetpos(fIn, &posVerts);
	fseek(fIn, dwNumFaces*3*32 , SEEK_CUR);//pVertices;

	fgetpos(fIn, &posBones);
	WORD wNumBones;
	fread(&wNumBones, sizeof(WORD), 1, fIn);
	NEW(WORD, totDetFacesPerBone, wNumBones*2)
	for(WORD b=0; b<wNumBones; b++)//pBoneNames
	{	char n[4]; n[0] = 32;
		while(n[0]) fread(n, 1, 1, fIn);
		totDetFacesPerBone[b] = 0;
	}
	fseek(fIn, 16*4, SEEK_CUR);//MeshTM
	fseek(fIn, wNumBones*16*4, SEEK_CUR);//pmBoneInitTMs;

	fgetpos(fIn, &posWeights);

	//Read bones info to mem:
	DWORD szBones = posWeights - posBones;
	RENEW(BYTE, pBones, szBones)
	fseek(fIn, posBones, SEEK_SET);
	fread(pBones, 1, szBones, fIn);

	fseek(fIn, posWeights, SEEK_SET);
	fread(&sz, sizeof(sz), 1, fIn);

	NEW(int, eachFaces, dwNumFaces*sizeof(int))
	NEW(int, facePtrForMrp, dwNumFaces*sizeof(int))//For the correction of morph faces;

	//Marking the 1weight vertices:
	for(DWORD f=0; f<dwNumFaces; f++)
	{	WORD whichBone, numBones[3], numFirstBones[3];
		for(WORD v=0; v<3; v++)
		{	fread(&numBones[v], sizeof(WORD), 1, fIn);
			for(WORD b=0; b<numBones[v]; b++)
			{	fread(&whichBone, sizeof(WORD), 1, fIn);
				if(!b) numFirstBones[v] = whichBone;
				fseek(fIn, 4, SEEK_CUR);//weights
		}	}
		eachFaces[f] = -1;
		if(numBones[0]==1)
		{	if(numBones[1]==1)
			{	if(numBones[2]==1)
				{	if(numFirstBones[0]==numFirstBones[1])
					{	if(numFirstBones[0]==numFirstBones[2])
						{	eachFaces[f] = numFirstBones[0]; 
							totDetFacesPerBone[numFirstBones[0]]++;
	}	}	}	}	}	}
	fread(&maxAssignBone, sizeof(maxAssignBone), 1, fIn);

	//Read morph part:
	fgetpos(fIn, &posMrp);
	fseek(fIn, 0, SEEK_END);
	fgetpos(fIn, &posEnd);
	szMrp = posEnd - posMrp;
	fseek(fIn, posMrp, SEEK_SET);
	RENEW(BYTE, pMrp, szMrp)
	fread(pMrp, 1, szMrp, fIn);
	fseek(fIn, posMrp, SEEK_SET);

	//Attach morph vertices back to main block, if this detached.
	DWORD dwUsedMrpChans; fread(&dwUsedMrpChans, 4, 1, fIn);
	for(DWORD ch=0; ch<dwUsedMrpChans; ch++)
	{	DWORD pdwJamDefFeysl; fread(&pdwJamDefFeysl, 4, 1, fIn);
		for(DWORD f=0; f<pdwJamDefFeysl; f++)
		{	DWORD fNum = 0; fread(&fNum, b32bit?4:2, 1, fIn);
			if(eachFaces[fNum] != -1)
			{	totDetFacesPerBone[eachFaces[fNum]]--;
				eachFaces[fNum] = -1;//Morphnikini, agar detach qilingan bo'lsa, qaytib joyiga qo'yamiz;
			}
			fseek(fIn, 4*18, SEEK_CUR);
	}	}

	//Save correction of the morph verts:
	int cnt = 0;
	for(DWORD f=0; f<dwNumFaces; f++)
	{	if(eachFaces[f] == -1) facePtrForMrp[f] = cnt++;
		else facePtrForMrp[f] = -1;
	}

	//Detach earch bone mesh:
	for(WORD b=0; b<wNumBones; b++)
	{	if(!totDetFacesPerBone[b]) continue;
		char name[128], N[16]; sprintf(name, Name); sprintf(N, "%d", b);
		*(strrchr(name, '\\')) = 0; lstrcat(name, "\\"); lstrcat(name, N); lstrcat(name, ".msh");
		FILE *mf = fopen(name, "wb");
		char sh[20] = "Msh16.............."; sh[10] = 'U';
		fwrite(sh, 1, 20, mf);
		float fzero[24] = {0.0f};
		fwrite(fzero, 4,18, mf);//Material
		fwrite(fzero, 4, 9, mf);//pos,scale,eul
		DWORD wntfaces = totDetFacesPerBone[b];
		fwrite(&wntfaces, sizeof(DWORD), 1, mf);
		fwrite(fzero, 4, 6, mf);//fxminmax
		for(DWORD f=0; f<dwNumFaces; f++)
		{	if(eachFaces[f]==b)//-1 == static;
			{	//Writing mesh:
				fseek(fIn, posVerts+f*32*3, SEEK_SET);//pVertices
				fread(fzero, 4, 8*3, fIn);
				fwrite(fzero, 4, 8*3, mf);//Bu yerda UB3 ga o'tkazmaymiz;
/*	
				float norms[3]; D3DXFLOAT16 uv16[2];
				fzero[3]=(1.0f+fzero[3])*127.5f;fzero[4]=(1.0f+fzero[4])*127.5;fzero[5]=(1.0f+fzero[5])*127.5f;
				norms[0] = (BYTE)fzero[3]; norms[1] = (BYTE)fzero[4]; norms[2] = (BYTE)fzero[5];
				fwrite(norms, 1, 4, mf);//norms;
				myD3DXFloat32To16Array(uv16,&fzero[6],2);
				fwrite(uv16, 2, 2, mf);//uvs;

				fwrite(&fzero[8], 4, 3, mf);//2-vert:
				fzero[11]=(1.0f+fzero[11])*127.5f;fzero[12]=(1.0f+fzero[12])*127.5;fzero[13]=(1.0f+fzero[13])*127.5f;
				norms[0] = (BYTE)fzero[11]; norms[1] = (BYTE)fzero[12]; norms[2] = (BYTE)fzero[13];
				fwrite(norms, 1, 4, mf);//norms;
				myD3DXFloat32To16Array(uv16,&fzero[14],2);
				fwrite(uv16, 2, 2, mf);//uvs;

				fwrite(&fzero[16], 4, 3, mf);//3-vert:
				fzero[19]=(1.0f+fzero[19])*127.5f;fzero[20]=(1.0f+fzero[20])*127.5;fzero[21]=(1.0f+fzero[21])*127.5f;
				norms[0] = (BYTE)fzero[19]; norms[1] = (BYTE)fzero[20]; norms[2] = (BYTE)fzero[21];
				fwrite(norms, 1, 4, mf);//norms;
				myD3DXFloat32To16Array(uv16,&fzero[22],2);
				fwrite(uv16, 2, 2, mf);//uvs;
*/
		}	}
		fclose(mf);
		if(-3==ConvertMeshToIndexedFormat(name))
			MessageBox(NULL, "Error. 32bit inds. requered.", "Please set 32 bit check!", MB_OK);
		DeleteFile(name);
		name[lstrlen(name)-1]='i';
		ConvertIndexedMeshToF3_UB4N_F16_2Format(name);
		RenameFile(name, lstrlen(name)-1,'i');
	}

	//Compact pVertices:
	fseek(fIn, posVerts, SEEK_SET);//pVertices
	RENEW(BYTE, pCompVerts, 32*3*dwNumFaces)
	pv = pCompVerts;
	for(DWORD f=0; f<dwNumFaces; f++)
	{	float vrts[8*3];fread(vrts, 4, 8*3, fIn);
		if(eachFaces[f]==-1)
		{	for(int vnt=0; vnt<8*3; vnt++)
			{ *((float*)pv) = vrts[vnt]; pv += 4; }
	}	}

	//Compact weights:
	fseek(fIn, posWeights+4, SEEK_SET);//weights
	RENEW(BYTE, pCompWeights, sz)
	pw = pCompWeights;
	for(DWORD f=0; f<dwNumFaces; f++)
	{	WORD whichBone, numBones;
		for(WORD v=0; v<3; v++)
		{	fread(&numBones, sizeof(WORD), 1, fIn);
			if(eachFaces[f] == -1)
			{ *((WORD*)pw) = numBones; pw += 2; 
			}
			for(WORD b=0; b<numBones; b++)
			{	fread(&whichBone, sizeof(WORD), 1, fIn);
				float weight; fread(&weight, sizeof(float), 1, fIn);
				if(eachFaces[f] == -1)
				{	*((WORD*)pw) = whichBone; pw += 2;
					*((float*)pw) = weight; pw += 4;
	}	}	}	}

	//Pos of wNumFaces:
	fseek(fIn, (int)(posVerts-sizeof(DWORD)-6*sizeof(float)), SEEK_SET);//wNumFaces,fMinMax[6];
	dwNumFaces = (pv-pCompVerts) / (32*3);
	fwrite(&dwNumFaces, 1, 4, fIn);

	//Write compressed vertices:
	fseek(fIn, posVerts, SEEK_SET);//pVertices
	fwrite(pCompVerts, 1, pv-pCompVerts, fIn);
	DEL(pCompVerts)

	//Write bones info:
	fwrite(pBones, 1, szBones, fIn);
	DEL(pBones)

	//Write size of weights:
	sz = pw-pCompWeights;
	fwrite(&sz, 4, 1, fIn);

	//Write weights:
	fwrite(pCompWeights, 1, sz, fIn);
	DEL(pCompWeights)

	fwrite(&maxAssignBone, sizeof(maxAssignBone), 1, fIn);

	//And morph part:
	BYTE* pbMrp = pMrp;
	fwrite(&dwUsedMrpChans, 4, 1, fIn); pbMrp += 4;
	for(DWORD ch=0; ch<dwUsedMrpChans; ch++)
	{	DWORD pdwJamDefFeysl = *((DWORD*)pbMrp); pbMrp += 4; 
		fwrite(&pdwJamDefFeysl, 4, 1, fIn);
		for(DWORD f=0; f<pdwJamDefFeysl; f++)
		{	DWORD fNum = b32bit?(*((DWORD*)pbMrp)):(*((WORD*)pbMrp));
			pbMrp += b32bit?4:2;
			fNum = facePtrForMrp[fNum]; fwrite(&fNum, b32bit?4:2, 1, fIn);
			fwrite(pbMrp, 4, 18, fIn); pbMrp += 4*18;
	}	}
	DEL(pMrp)

	fgetpos(fIn, &posEnd);
	DEL(totDetFacesPerBone);
	DEL(eachFaces)

	//Reading to the mem. for cutting total length, because CRT not have fcut metod.
	NEW(VOID, pbf, posEnd)
	fseek(fIn, 0, SEEK_SET);//pVertices
	fread(pbf, 1, posEnd, fIn);
	fclose(fIn);

	//Cut lenght with writing as new file:
	fIn = fopen(Name, "wb");
	fwrite(pbf, 1, posEnd, fIn);
	fclose(fIn);

	DEL(pbf)
	DEL(facePtrForMrp)
	return TRUE;
}

//Test pasted successfully;
BOOL ConvertSkin2MorphToSHFormat(char *pathAndName)
{
FILE *fIn, *fOut;

	lstrcpy(Name, pathAndName);
	fIn = fopen(Name, "rb");
	if(!fIn) return FALSE;
	Name[lstrlen(Name)-2] = 'b';
	Name[lstrlen(Name)-1] = '5';
	fOut = fopen(Name, "wb");
//************************************************************************************
	char sh[20];
	fread(sh, 1, 20, fIn);
	if(!(sh[0]=='S'&&sh[1]=='k'&&sh[2]=='n'&&sh[3]=='2'&&sh[4]=='m'&&sh[5]=='r'&&sh[6]=='p'))
	{	fclose(fIn);
		fclose(fOut);
		return FALSE;
	}
	BOOL b32bit = (sh[7] == '3' && sh[8] == '2');
	sh[9]='S';sh[10]='H'; fwrite(sh, 1, 20, fOut);
//************************************************************************************
	float buf[64];
	fread (buf, 4, 6, fIn);	fwrite(buf, 4, 6, fOut);//pos, EUL;
	DWORD dwNumFaces; fread(&dwNumFaces, 4, 1, fIn); fwrite(&dwNumFaces, 4, 1, fOut);
	DWORD dwNumFaceVerts = dwNumFaces * 3;

	fread(buf, 6, 4, fIn);//fXMin fXMax;
	fwrite(buf, 6, 4, fOut);
//************************************************************************************
	NEW(float, pVertices, 32 * dwNumFaceVerts)
	fread(pVertices, 32, dwNumFaceVerts, fIn);
//************************************************************************************
	WORD wNumBones; fread(&wNumBones, 2, 1, fIn);//wNumBones;
	fwrite(&wNumBones, 2, 1, fOut);
	for(WORD b=0; b<wNumBones; b++)
	{	char n[4]; n[0] = 32;
		while(n[0]) { fread(n, 1, 1, fIn); fwrite(n, 1, 1, fOut); }
	}
//************************************************************************************
	fread (buf, 4, 16, fIn );//meshTM;
	fwrite(buf, 4, 16, fOut);
//************************************************************************************
	for(WORD inTm=0; inTm<wNumBones; inTm++)
	{	fread(buf, 4, 16, fIn);//bone InitTMs;
		fwrite(buf, 4, 16, fOut);
	}
//************************************************************************************
	DWORD sz; fread(&sz, 1, sizeof(sz), fIn);
	NEW(BYTE, pvVertBoneIndsAndWeights, sz)
	fread(pvVertBoneIndsAndWeights, 1, sz, fIn);

	char maxAssignBone; fread(&maxAssignBone, 1, 1, fIn);
	if(maxAssignBone > 4)
		MessageBox(NULL, "Max blend matrice or bones for each vertex above the norm(4) for the bind verts.", "Canceling converting.", MB_OK);

	DWORD pdwJamDefFeysl, dwUsedMrpChans;
	fread(&dwUsedMrpChans, 4, 1, fIn);

	WORD numBones; DWORD v, indices;
//************************************************************************************
	BYTE *pcWeightTable = pvVertBoneIndsAndWeights;
	for(v=0; v<dwNumFaceVerts; v++)
	{	fwrite(&pVertices[v*8], 4, 3, fOut);//pos
		float blend[3] = {0.0f};
		indices = 0x00000000;
		numBones = *((WORD*)pcWeightTable); 
		pcWeightTable += sizeof(WORD);
		if(numBones == 1)
		{	indices = *((WORD*)pcWeightTable);
			indices &= 0x000000ff;
			blend[0] = 1.0f;
			pcWeightTable += sizeof(WORD) + sizeof(float);
		}
		else
		{	WORD ind[4] = {0};
			for (int j=0;j<numBones;j++)
			{	if(j<4) ind[j] = *((WORD*)pcWeightTable);//4 dan ortig'ini shunday tashlab yuboramiz;
					pcWeightTable += sizeof(WORD);
				if(j<3) blend[j]  = *((float*)pcWeightTable);//4 dan ortig'ini shunday tashlab yuboramiz;
					pcWeightTable += sizeof(float);
			}
			indices =	(ind[0] & 0x000000ff) | 
						((ind[1] & 0x000000ff) << 8 ) |
						((ind[2] & 0x000000ff) << 16) |
						((ind[3] & 0x000000ff) << 24) ;
		}
		fwrite(blend, 4, 3, fOut);//blend
		fwrite(&indices, 4, 1, fOut);//indicases
		fwrite(&pVertices[v*8+3], 4, 3, fOut);//normal
		fwrite(&pVertices[v*8+6], 4, 2, fOut);//tu-tv
	}


	//Morph part:
	//Morph part:
	//Morph part:
	fwrite(&dwUsedMrpChans, 4, 1, fOut);

	NEW(float, mrpDeltas, dwNumFaces*3*6*4)//VN 4-sizeof(float), 3*6=18 ta float

	//Writing the morph part:
	for(DWORD c = 0; c < dwUsedMrpChans; c++)
	{	ZeroMemory(mrpDeltas, dwNumFaces*3*6*4);//Zero deltas;
		fread(&pdwJamDefFeysl, 4, 1, fIn);
		for(DWORD f=0; f<pdwJamDefFeysl; f++)
		{	DWORD fNum=0; fread(&fNum, b32bit?4:2, 1, fIn);
			float d[18]; fread(d, 4, 18, fIn);
			for(int id=0; id<18; id++)
				mrpDeltas[fNum*18+id] += d[id];
		}
		for(DWORD f=0; f<dwNumFaces; f++)
		{	//5 kanallikdan 11 kanallikka o'tgandan so'ng quyidagiga o'zgardi.
			if(c < 6) fwrite(&mrpDeltas[f*18], 4, 18, fOut);
			else
			{	if(c < (dwUsedMrpChans < MAX_SKIN2MORPH_CHANNEL ? MAX_SKIN2MORPH_CHANNEL-dwUsedMrpChans+1 : 1))
					 fwrite(&mrpDeltas[f*18], 4, 18, fOut);
				else
				{	 fwrite(&mrpDeltas[f*18   ], 4, 3, fOut);
					 fwrite(&mrpDeltas[f*18+6 ], 4, 3, fOut);
					 fwrite(&mrpDeltas[f*18+12], 4, 3, fOut);
	}	}	}	}
	fclose(fOut);
	fclose(fIn);
	DEL(pVertices)
	DEL(pvVertBoneIndsAndWeights)
	DEL(mrpDeltas)
	return TRUE;
}

//Test pasted successfully;
BOOL ConvertSoftSkin2MorphToIndexedFormat(char *pathAndName, float delta)
{
FILE *fIn, *fOut;

	lstrcpy(Name, pathAndName);
	fIn = fopen(Name, "rb");
	if(!fIn) return FALSE;
	Name[lstrlen(Name)-2] = 'm';
	Name[lstrlen(Name)-1] = 'i';
	fOut = fopen(Name, "wb");
//************************************************************************************
	char sh[20];
	fread(sh, 1, 20, fIn);
	if(!(sh[0]=='S'&&sh[1]=='k'&&sh[2]=='n'&&sh[3]=='2'&&sh[4]=='m'&&sh[5]=='r'&&sh[6]=='p'))
	{	fclose(fIn);
		fclose(fOut);
		return FALSE;
	}
	BOOL b32bit = (sh[7] == '3' && sh[8] == '2');
	sh[9]='I';sh[10]='.';sh[11]='.'; fwrite(sh, 1, 20, fOut);
//************************************************************************************
	float buf[64];
	fread (buf, 4, 6, fIn);//pos, EUL;
	fwrite(buf, 4, 6, fOut);

	fpos_t posNumFaces; fgetpos(fOut, &posNumFaces);
	DWORD tNumFaces, dwNumFaces[2] = {0}, dwNumVerts[2] = {0};
	fread(&tNumFaces, 4, 1, fIn); 
	fwrite(dwNumFaces, 4, 2, fOut);
	fwrite(dwNumVerts, 4, 2, fOut);

	fread (buf, 6, 4, fIn );//fXMin fXMax;
	fwrite(buf, 6, 4, fOut);
//************************************************************************************
	NEW(float, pVertices, 32 * 3 * tNumFaces)
	fread(pVertices, 32, 3 * tNumFaces, fIn);
//************************************************************************************
	WORD wNumBones; fread(&wNumBones, 2, 1, fIn);//wNumBones;
	fwrite(&wNumBones, 2, 1, fOut);
	for(WORD b=0; b<wNumBones; b++)
	{	char n[4]; n[0] = 32;
		while(n[0]) { fread(n, 1, 1, fIn); fwrite(n, 1, 1, fOut); }
	}
//** Har 1 bone uchun unused faces, used faces lar uchun shu yerda joy 
//tashlab ketsok, kelgusi metodlar uchun judayam qulay bo'lg'ay ekan:
	for(WORD b=0; b<wNumBones*2; b++) fwrite(&wNumBones, 2, 1, fOut);
//************************************************************************************
	fread (buf, 4, 16, fIn );//meshTM;
	fwrite(buf, 4, 16, fOut);
//************************************************************************************
	for(WORD inTm=0; inTm<wNumBones; inTm++)
	{	fread(buf, 4, 16, fIn);//bone InitTMs;
		fwrite(buf, 4, 16, fOut);
	}
//************************************************************************************
	DWORD szWghts; fread(&szWghts, 1, sizeof(szWghts), fIn);
	fpos_t posWgts; fgetpos(fIn, &posWgts);
	fseek(fIn, szWghts, SEEK_CUR);
	char maxAssignBone; fread(&maxAssignBone, 1, 1, fIn);
	fwrite(&maxAssignBone, 1, 1, fOut);

	//Morph part:
	NEW(BYTE, bMrpFaces, tNumFaces)
	ZeroMemory(bMrpFaces,tNumFaces);
	DWORD dwUsedMrpChans; fread(&dwUsedMrpChans, 4, 1, fIn);
	for(DWORD c = 0; c < dwUsedMrpChans; c++)
	{	DWORD pdwJamDefFeysl; fread (&pdwJamDefFeysl, 4, 1, fIn);
		for(DWORD f=0; f<pdwJamDefFeysl; f++)
		{	DWORD fNum=0; fread(&fNum, b32bit?4:2, 1, fIn);
			bMrpFaces[fNum] = 1;
			fseek(fIn, 3*4*6, SEEK_CUR);//face deltas;
	}	}

	//Compact unused morph vertices:
	NEW(DWORD , pInds0 ,  4 * 3 * tNumFaces)//har 1 yoyilgan vertning compga strelkasi;
	NEW(float, pVerts0, 32 * 3 * tNumFaces)
	for(DWORD v=0; v<3*tNumFaces; v++)
	{	DWORD f = v/3; if(bMrpFaces[f]) continue;
		BOOL findCopy = FALSE;
		for(DWORD cv=0; cv<dwNumVerts[0]; cv++)
		{	//if(cv==986 || cv==987 || cv==1006)
			//	Beep(1000, 1000);
			if (fabs(pVertices[8*v  ] - pVerts0[8*cv  ]) < delta)//x
			{if(fabs(pVertices[8*v+1] - pVerts0[8*cv+1]) < delta)//y
			{if(fabs(pVertices[8*v+2] - pVerts0[8*cv+2]) < delta)//z
			{if(fabs(pVertices[8*v+3] - pVerts0[8*cv+3]) < delta)//nx
			{if(fabs(pVertices[8*v+4] - pVerts0[8*cv+4]) < delta)//ny
			{if(fabs(pVertices[8*v+5] - pVerts0[8*cv+5]) < delta)//nz
			{if(fabs(pVertices[8*v+6] - pVerts0[8*cv+6]) < delta)//tv
			{if(fabs(pVertices[8*v+7] - pVerts0[8*cv+7]) < delta)//tu
			{ findCopy = TRUE; pInds0[v] = cv; break; }
		}}}}}}}}
		if(findCopy) continue;
		for(int j=0; j<8; j++) pVerts0[8*dwNumVerts[0]+j] = pVertices[8*v+j];
		fwrite(&pVerts0[8*dwNumVerts[0]], 4, 8, fOut);
		//if(6377==dwNumVerts[0])
		//	Beep(100,100);
		pInds0[v] = dwNumVerts[0]++;
	}

	//Compact used morph vertices:
	NEW(DWORD , pInds1 , 4 * 3 * tNumFaces)//har 1 yoyilgan vertning compga strelkasi;
	NEW(float, pVerts1, 32 * 3 * tNumFaces)
	for(DWORD v=0; v<3*tNumFaces; v++)
	{	DWORD f = v/3; if(!bMrpFaces[f]) continue;
		BOOL findCopy = FALSE;
		for(DWORD cv=0; cv<dwNumVerts[1]; cv++)
		{	if (pVertices[8*v  ] == pVerts1[8*cv  ])//x
			{if(pVertices[8*v+1] == pVerts1[8*cv+1])//y
			{if(pVertices[8*v+2] == pVerts1[8*cv+2])//z
			{if(pVertices[8*v+3] == pVerts1[8*cv+3])//nx
			{if(pVertices[8*v+4] == pVerts1[8*cv+4])//ny
			{if(pVertices[8*v+5] == pVerts1[8*cv+5])//nz
			{if(pVertices[8*v+6] == pVerts1[8*cv+6])//tv
			{if(pVertices[8*v+7] == pVerts1[8*cv+7])//tu
			{ findCopy = TRUE; pInds1[v] = cv; break; }
		}}}}}}}}
		if(findCopy) continue;
		for(int j=0; j<8; j++) pVerts1[8*dwNumVerts[1]+j] = pVertices[8*v+j];
		fwrite(&pVerts1[8*dwNumVerts[1]], 4, 8, fOut);
		pInds1[v] = dwNumVerts[1]++;
	}

	//Compact unused morph faces:
	for(DWORD f=0; f<tNumFaces; f++)
	{	if(bMrpFaces[f]) continue;
		fwrite(&pInds0[3*f  ], b32bit?4:2, 1, fOut);
		fwrite(&pInds0[3*f+1], b32bit?4:2, 1, fOut);
		fwrite(&pInds0[3*f+2], b32bit?4:2, 1, fOut);
		dwNumFaces[0]++;
	}
	//Compact used morph faces:
	for(DWORD f=0; f<tNumFaces; f++)
	{	if(!bMrpFaces[f]) continue;
		fwrite(&pInds1[3*f  ], b32bit?4:2, 1, fOut);
		fwrite(&pInds1[3*f+1], b32bit?4:2, 1, fOut);
		fwrite(&pInds1[3*f+2], b32bit?4:2, 1, fOut);
		dwNumFaces[1]++;
	}
	fseek(fOut, posNumFaces, SEEK_SET);
	fwrite(dwNumFaces, 4, 2, fOut);
	fwrite(dwNumVerts, 4, 2, fOut);
	fseek(fOut, 0, SEEK_END);

	//Vertex bone weights:
	fseek(fIn, posWgts, SEEK_SET);
	NEW(__int16, pVertBonNums, 2 * dwNumVerts[0])
	NEW(__int16, pVertBons   , 2 * dwNumVerts[0] * maxAssignBone)
	NEW(float, pVertWeights  , 4 * dwNumVerts[0] * maxAssignBone)
	//Unused morph vertices:
	for(DWORD v=0; v<dwNumVerts[0]; v++) pVertBonNums[v] = 0;
	NEW(WORD , which , 2*maxAssignBone)
	NEW(float, weight, 4*maxAssignBone)
	for(DWORD v=0; v<tNumFaces*3; v++)
	{	WORD bones; fread(&bones, 2, 1, fIn);
		for(WORD b=0; b<bones; b++)
		{	fread(&which [b], 2, 1, fIn);
			fread(&weight[b], 4, 1, fIn);
		}
		DWORD f = v/3; if(bMrpFaces[f]) continue;
		if(pVertBonNums[pInds0[v]] < bones)
		{	pVertBonNums[pInds0[v]] = bones;
			for(int j=0;j<bones;j++)
			{	pVertBons[pInds0[v]*maxAssignBone+j] = which[j];
				pVertWeights[pInds0[v]*maxAssignBone+j] = weight[j];
	}	}	}
	fpos_t posWghtOut; DWORD szWghtOut = 0; fgetpos(fOut, &posWghtOut);
	fwrite(&szWghtOut, 4, 1, fOut);
	//Save weights for unused morph vertices:
	for(DWORD v=0; v<dwNumVerts[0]; v++)
	{	if(pVertBonNums[v] != 0)
		{	fwrite(&pVertBonNums[v], 2, 1, fOut); szWghtOut += 2;
			for(int j=0; j<pVertBonNums[v]; j++)
			{	fwrite(&pVertBons[v*maxAssignBone+j], 2, 1, fOut); szWghtOut += 2;
				fwrite(&pVertWeights[v*maxAssignBone+j], 4, 1, fOut); szWghtOut += 4;
	}	}	}

	DEL(pVertBonNums)
	DEL(pVertBons)
	DEL(pVertWeights)
	RENEW(__int16, pVertBonNums, 2 * dwNumVerts[1])
	RENEW(__int16, pVertBons   , 2 * dwNumVerts[1] * maxAssignBone)
	RENEW(float, pVertWeights  , 4 * dwNumVerts[1] * maxAssignBone)

	//Used morph vertices:
	fseek(fIn, posWgts, SEEK_SET);
	for(DWORD v=0; v<dwNumVerts[1]; v++) pVertBonNums[v] = 0;
	for(DWORD v=0; v<tNumFaces*3; v++)
	{	WORD bones; fread(&bones, 2, 1, fIn);
		for(WORD b=0; b<bones; b++)
		{	fread(&which [b], 2, 1, fIn);
			fread(&weight[b], 4, 1, fIn);
		}
		DWORD f = v/3; if(!bMrpFaces[f]) continue;
		if(pVertBonNums[pInds1[v]] < bones)
		{	pVertBonNums[pInds1[v]] = bones;
			for(int j=0;j<bones;j++)
			{	pVertBons[pInds1[v]*maxAssignBone+j] = which[j];
				pVertWeights[pInds1[v]*maxAssignBone+j] = weight[j];
	}	}	}

	NEW(float, mrpDeltas, dwNumVerts[1]*6*4)

	//Save weights for used morph vertices:
	for(DWORD v=0; v<dwNumVerts[1]; v++)
	{	if(pVertBonNums[v] != 0)
		{	fwrite(&pVertBonNums[v], 2, 1, fOut); szWghtOut += 2;
			for(int j=0; j<pVertBonNums[v]; j++)
			{	fwrite(&pVertBons[v*maxAssignBone+j], 2, 1, fOut); szWghtOut += 2;
				fwrite(&pVertWeights[v*maxAssignBone+j], 4, 1, fOut); szWghtOut += 4;
	}	}	}
	fseek(fOut, posWghtOut, SEEK_SET);
	fwrite(&szWghtOut, 4, 1, fOut);
	fseek(fOut, 0, SEEK_END);
	DEL(which )
	DEL(weight)

	//Morph part:
	fseek(fIn, posWgts+szWghts+1+4, SEEK_SET);//weights , maxAssignBone and dwUsedMrpChans;
	fwrite(&dwUsedMrpChans, 4, 1, fOut);
	//NEW(float, mrpDeltas, wNumVerts[1]*6*4)
	for(DWORD c = 0; c < dwUsedMrpChans; c++)
	{	ZeroMemory(mrpDeltas, dwNumVerts[1]*6*4);//Zero deltas;
		DWORD pdwJamDefFeysl; fread (&pdwJamDefFeysl, 4, 1, fIn);
		for(DWORD f=0; f<pdwJamDefFeysl; f++)
		{	DWORD fNum=0; fread(&fNum, b32bit?4:2, 1, fIn);
			DWORD V0 = pInds1[fNum*3  ];
 			DWORD V1 = pInds1[fNum*3+1];
			DWORD V2 = pInds1[fNum*3+2];
			float d[18]; fread(d, 4, 18, fIn);
			for(int j=0; j<6; j++) mrpDeltas[V0*6+j] = d[j   ];
			for(int j=0; j<6; j++) mrpDeltas[V1*6+j] = d[6+j ];
			for(int j=0; j<6; j++) mrpDeltas[V2*6+j] = d[12+j];
		}DWORD pdwJamDefVertl = 0;
		for(DWORD v=0; v<dwNumVerts[1]; v++)
		{	if(mrpDeltas[v*6] != 0.0f || mrpDeltas[v*6+1] != 0.0f || mrpDeltas[v*6+2] != 0.0f)// ||
			   //normali kerakmas:mrpDeltas[v*6+3] != 0.0f || mrpDeltas[v*6+4] != 0.0f || mrpDeltas[v*6+5] != 0.0f)
			pdwJamDefVertl++;
		}
		fwrite(&pdwJamDefVertl, sizeof(DWORD), 1, fOut);
		for(DWORD v=0; v<dwNumVerts[1]; v++)
		{	if(mrpDeltas[v*6] != 0.0f || mrpDeltas[v*6+1] != 0.0f || mrpDeltas[v*6+2] != 0.0f)
			{	fwrite(&v, b32bit?4:2, 1, fOut);
				fwrite(&mrpDeltas[v*6], 4, 6, fOut);
	}	}	}

	fclose(fOut);
	fclose(fIn);
	DEL(pVertices   )
	DEL(bMrpFaces   )
	DEL(pInds0		)
	DEL(pVerts0		)
	DEL(pInds1		)
	DEL(pVerts1		)
	DEL(pVertBonNums)
	DEL(pVertBons   )
	DEL(pVertWeights)
	DEL(mrpDeltas   )
	return SortForTextrIndexedSkin2Morph(Name);
}

//Test pasted successfully;
BOOL SoftSkin2MorphToText(char *NAME)
{
FILE	*fIn, *fOut;
char	pathAndName[128];

	lstrcpy(pathAndName, NAME);

	lstrcpy(Name, pathAndName);
	fIn = fopen(Name, "rb");
	if(!fIn) return FALSE;
	lstrcat(strrchr(Name, '.'), "txt");
	fOut = fopen(Name, "w");

	char sh[20];
	fread(sh, 1, 20, fIn);
	if(!(sh[0]=='S'&&sh[1]=='k'&&sh[2]=='n'&&sh[3]=='2'&&sh[4]=='m'&&sh[5]=='r'&&sh[6]=='p'))
	{	fclose(fIn);
		fclose(fOut);
		return FALSE;
	}
	BOOL b32bit = (sh[7] == '3' && sh[8] == '2');

	float buf[24];
	fread(buf, 4, 6, fIn);
	fprintf(fOut, "\n Position: %.4f, %.4f, %.4f;\n Rotation in EulerXYZ: %.4f, %.4f, %.4f;",
					buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);

	DWORD dwNumFaces; fread(&dwNumFaces, 4, 1, fIn);
	fprintf(fOut, "\n Number of faces: %#05.5d", dwNumFaces);

	fread(buf, 4, 6, fIn);
	fprintf(fOut, "\n Bound box sizes: %.4f, %.4f, %.4f, %.4f, %.4f, %.4f;", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);

	fprintf(fOut, "\n\n                     Vertice positions:               Normals:   Texture coordinates:");
	for(DWORD f=0; f<dwNumFaces; f++)
	{	fprintf(fOut, "\nF %#05.5d:", f);
		fread(buf, 4, 24, fIn);
		fprintf(fOut, " V %#05.5d: %.4f, %.4f, %4f, %4f, %.4f, %4f, %4f, %.4f;",
		 f*3, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]); 
		fprintf(fOut, "\n         V %#05.5d: %.4f, %.4f, %4f, %4f, %.4f, %4f, %4f, %.4f;",
		 f*3+1, buf[8], buf[9], buf[10], buf[11], buf[12], buf[13], buf[14], buf[15]);
		fprintf(fOut, "\n         V %#05.5d: %.4f, %.4f, %4f, %4f, %.4f, %4f, %4f, %.4f;",
		 f*3+2, buf[16], buf[17], buf[18], buf[19], buf[20], buf[21], buf[22], buf[23]);
	}

	WORD wNumBones;	fread(&wNumBones, 2, 1, fIn);
	fprintf(fOut, "\n\n Number of bones: %#05.5d\nBones: ", wNumBones);
	for(WORD b=0; b<wNumBones; b++)
	{	char n[4]; n[0] = 32; fprintf(fOut, "\n%#05.5d ", b);
		while(n[0]) { fread(n, 1, 1, fIn); fwrite(n, 1, 1, fOut); }
	}

	fread(buf, 4, 16, fIn);
	fprintf(fOut, "\n\n MeshTM: %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f;", 
			buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], 
			buf[9], buf[10], buf[11], buf[12], buf[13], buf[14], buf[15]);

	fprintf(fOut, "\n\n Bones InitTMs:"); 
	for(WORD b=0; b<wNumBones; b++)
	{	fprintf(fOut, "\n%#05.5d ", b);
		fread(buf, 4, 16, fIn);
		fprintf(fOut, "\n %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f;", 
			buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], 
			buf[9], buf[10], buf[11], buf[12], buf[13], buf[14], buf[15]);
	}

	DWORD sz; fpos_t posSz; fread(&sz, 4, 1, fIn);
	fgetpos(fIn, &posSz);
	fprintf(fOut, "\n\n               Vertice weights:");
	for(DWORD v=0; v<dwNumFaces*3; v++)
	{	//if(v==44)//89148)
		//	Beep(100,100);
		fprintf(fOut, "\nV %#05.5d: ", v);
		WORD bones; fread(&bones, 2, 1, fIn);
		fprintf(fOut, "Total bones: %#05.5d ", bones);
		for(WORD b=0; b<bones; b++)
		{	WORD which;  fread(&which, 2, 1, fIn);
						 fread(buf, 4, 1, fIn);
			fprintf(fOut, " bone: %#05.5d, weight: %.4f", which, buf[0]);
	}	}
	fseek(fIn, posSz+sz, SEEK_SET);//weights

	BYTE maxAsBn; fread(&maxAsBn, 1, 1, fIn);
	fprintf(fOut, "\n Maximum assigned bones: %#05.5d", maxAsBn);

	DWORD dwUsedMrpChans; fread(&dwUsedMrpChans, 4, 1, fIn);
	fprintf(fOut, "\n\n Total used morpher channels: %#05.5d", dwUsedMrpChans);
	for(DWORD ch=0; ch<dwUsedMrpChans; ch++)
	{	DWORD pdwJamDefFeysl; fread(&pdwJamDefFeysl, 4, 1, fIn);
		fprintf(fOut, "\n\n Channel number: %#05.5d\n Total used faces: %#05.5d", ch, pdwJamDefFeysl);
		for(DWORD f=0; f<pdwJamDefFeysl; f++)
		{	DWORD fNum=0; fread(&fNum, b32bit?4:2, 1, fIn);
			fread(buf, 4, 18, fIn);
			fprintf(fOut, "\nF %#05.5d: V0: %.4f, %.4f, %.4f; N0: %.4f, %.4f, %.4f; V1: %.4f, %.4f, %.4f; N1: %.4f, %.4f, %.4f; V2: %.4f, %.4f, %.4f; N2: %.4f, %.4f, %.4f;",
					fNum, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], buf[9], buf[10], buf[11], buf[12], buf[13], buf[14], buf[15], buf[16], buf[17]);
	}	}

	fclose(fIn);
	fclose(fOut);
	return TRUE;
}

static VOID Cross(float *res, float *a, float *b)
{
	res[0] = a[1] * b[2] - a[2] * b[1];
	res[1] = a[2] * b[0] - a[0] * b[2];
	res[2] = a[0] * b[1] - a[1] * b[0];
	return;
}

BOOL Skin2MorphShadowToText(char *NAME)
{
FILE	*fIn, *fOut;
char	pathAndName[128];

	lstrcpy(pathAndName, NAME);

	fIn = fopen(pathAndName, "rb");
	if(!fIn) return FALSE;
	lstrcat(strrchr(pathAndName, '.'), "txt");
	fOut = fopen(pathAndName, "w");

	char sh[20];
	fread(sh, 1, 20, fIn);
	if(!(sh[0]=='S'&&sh[1]=='k'&&sh[2]=='n'&&sh[3]=='2'&&sh[4]=='m'&&sh[5]=='r'&&sh[6]=='p'&&
		sh[9]=='s'&&sh[10]=='h'&&sh[11]=='a'&&sh[12]=='d'&&sh[13]=='o'&&sh[14]=='w'))
	{	fclose(fIn);
		fclose(fOut);
		return FALSE;
	}
	BOOL b32bit = (sh[7] == '3' && sh[8] == '2');
	BOOL bStrip = (sh[15] == 'S');
	if(bStrip) fprintf(fOut, "\nTRIANGLE STRIP:");

	DWORD dwNumFaces; fread(&dwNumFaces, 4, 1, fIn);
	fprintf(fOut, "\nNumber of faces: %#05.5d", dwNumFaces);

	DWORD dwNumVerts; fread(&dwNumVerts, 4, 1, fIn);
	fprintf(fOut, "\nNumber of vertices: %#05.5d", dwNumVerts);

	float buf[6];
	fprintf(fOut, "\n\n           Vertice positions[3]:       Weights[3];    Indicases[DW];    Pos1[3];        Pos2[3];    Weights1:   Weights2: Inds1: Inds2: Calculated normal for checking:");
	for(DWORD v=0; v<dwNumVerts; v++)
	{	fread(buf, 4, 3, fIn);
		fprintf(fOut, "\nV %#05.5d:  %.4f, %.4f, %.4f;",
		 v, buf[0], buf[1], buf[2]);
		float pos[3] = {buf[0],buf[1],buf[2]};

		//weights:
		BYTE norms[4]; fread(norms, 1, 4, fIn);
		fprintf(fOut, " %d, %d, %d, %d", norms[0], norms[1], norms[2], norms[3]);

		DWORD inds; fread(&inds, 4, 1, fIn); fprintf(fOut, " %x: ", inds);

		fread(buf, 4, 6, fIn);
		fprintf(fOut, " %.4f, %.4f, %.4f     %.4f, %.4f, %.4f", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
		float v0[3]={buf[0]-pos[0],buf[1]-pos[1],buf[2]-pos[2]};
		float v1[3]={buf[3]-pos[0],buf[4]-pos[1],buf[5]-pos[2]};
		Cross(pos, v0, v1);
		float ln = pos[0]*pos[0]+pos[1]*pos[1]+pos[2]*pos[2];
		ln = sqrtf(ln);if(ln>0.0f){pos[0]/=ln;pos[1]/=ln;pos[2]/=ln;}

		//weights1:
		fread(norms, 1, 4, fIn);
		fprintf(fOut, " %d, %d, %d, %d", norms[0], norms[1], norms[2], norms[3]);

		//weights1:
		fread(norms, 1, 4, fIn);
		fprintf(fOut, " %d, %d, %d, %d", norms[0], norms[1], norms[2], norms[3]);

		fread(&inds, 4, 1, fIn); fprintf(fOut, " %x: ", inds);
		fread(&inds, 4, 1, fIn); fprintf(fOut, " %x: ", inds);

		fprintf(fOut, "    %.4f, %.4f, %.4f", pos[0], pos[1], pos[2]);
	}

	fprintf(fOut, "\n\n geometry vertices, total: %#05.5d\n Indicase to the upper vertice table:", dwNumVerts);
	DWORD inds2[3];	WORD inds1[3];

	if(bStrip)
	{	if(b32bit)
		{	S32 fst=0; fread(inds2, 4, 3, fIn);
			fprintf(fOut, "\nF %#05.5d: %#05.5d, %#05.5d,  %#05.5d", fst++, inds2[0], inds2[1], inds2[2]);
			for(DWORD f=0; f<dwNumFaces-1; f++)
			{	fread(&inds2[0], 4, 1, fIn);
				if((f%2)==0)
				{	fprintf(fOut, "\nF %#05.5d: %#05.5d, %#05.5d,  %#05.5d", fst++,
									inds2[1], inds2[0], inds2[2]);
					inds2[1] = inds2[0];
				}
				else
				{
					fprintf(fOut, "\nF %#05.5d: %#05.5d, %#05.5d,  %#05.5d", fst++,
									inds2[2], inds2[1], inds2[0]); 
					inds2[2] = inds2[0];
		}	}	}
		else //b16bit
		{	S32 fst=0; fread(inds1, 2, 3, fIn);
			fprintf(fOut, "\nF %#05.5d: %#05.5d, %#05.5d,  %#05.5d", fst++, inds1[0], inds1[1], inds1[2]);
			for(DWORD f=0; f<dwNumFaces-1; f++)
			{	fread(&inds1[0], 2, 1, fIn);
				if((f%2)==0)
				{	fprintf(fOut, "\nF %#05.5d: %#05.5d, %#05.5d,  %#05.5d", fst++,
									inds1[1], inds1[0], inds1[2]);
					inds1[1] = inds1[0];
				}
				else
				{
					fprintf(fOut, "\nF %#05.5d: %#05.5d, %#05.5d,  %#05.5d", fst++,
									inds1[2], inds1[1], inds1[0]); 
					inds1[2] = inds1[0];
	}	}	}	}
	else
	{	for(DWORD f=0; f<dwNumFaces; f++)
		{	if(b32bit)
			{	fread(inds2, 4, 3, fIn);
				fprintf(fOut, "\nF %#05.5d: %#05.5d %#05.5d %#05.5d", f, inds2[0], inds2[1], inds2[2]);
			} else
			{	fread(inds1, 2, 3, fIn);
				fprintf(fOut, "\nF %#05.5d: %#05.5d %#05.5d %#05.5d", f, inds1[0], inds1[1], inds1[2]);
	}	}	}

	fclose(fIn);
	fclose(fOut);
	return TRUE;
}

//Test pasted successfully;
BOOL SHSkin2MorphToText(char *pathAndName)
{
FILE	*fIn, *fOut;

	lstrcpy(Name, pathAndName);
	fIn = fopen(Name, "rb");
	if(!fIn) return FALSE;
	lstrcat(strrchr(Name, '.'), "txt");
	fOut = fopen(Name, "w");

	char sh[20];
	fread(sh, 1, 20, fIn);
	if(!(sh[0]=='S'&&sh[1]=='k'&&sh[2]=='n'&&sh[3]=='2'&&sh[4]=='m'&&sh[5]=='r'&&sh[6]=='p'&&sh[9]=='S'&&sh[10]=='H'))
	{	fclose(fIn);
		fclose(fOut);
		return FALSE;
	}
	BOOL b32bit = (sh[3] == '7' && sh[4] == '8');

	float buf[24];
	fread(buf, 4, 6, fIn);
	fprintf(fOut, "\n Position: %.4f, %.4f, %.4f;\n Rotation in EulerXYZ: %.4f, %.4f, %.4f;",
					buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);

	DWORD dwNumFaces; fread(&dwNumFaces, 4, 1, fIn);
	fprintf(fOut, "\n Number of faces: %#05.5d", dwNumFaces);

	fread(buf, 4, 6, fIn);
	fprintf(fOut, "\n Bound box sizes: %.4f, %.4f, %.4f, %.4f, %.4f, %.4f;", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);

	WORD wNumBones;	fread(&wNumBones, 2, 1, fIn);
	fprintf(fOut, "\n Number of bones: %#05.5d\n\n Bones: ", wNumBones);
	for(WORD b=0; b<wNumBones; b++)
	{	char n[4]; n[0] = 32; fprintf(fOut, "\n%#05.5d ", b);
		while(n[0]) { fread(n, 1, 1, fIn); fwrite(n, 1, 1, fOut); }
	}

	fread(buf, 4, 16, fIn);
	fprintf(fOut, "\n\n MeshTM: %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f;", 
			buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], 
			buf[9], buf[10], buf[11], buf[12], buf[13], buf[14], buf[15]);

	fprintf(fOut, "\n\n Bones InitTMs:"); 
	for(WORD b=0; b<wNumBones; b++)
	{	fprintf(fOut, "\n%#05.5d ", b);
		fread(buf, 4, 16, fIn);
		fprintf(fOut, "\n %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f;", 
			buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], 
			buf[9], buf[10], buf[11], buf[12], buf[13], buf[14], buf[15]);
	}

	fprintf(fOut, "\n\n           Vertice positions:   Blend0;Blend1;Blend2; Bone indicase:  Normals:  Texture coordinates:");
	for(DWORD f=0; f<dwNumFaces; f++)
	{	fprintf(fOut, "\nF %#05.5d: ", f);
		for(WORD v=0; v<3; v++)
		{	DWORD indicase; fread(buf, 4, 6, fIn); fread(&indicase, 4, 1, fIn);
			fprintf(fOut, v ? "\n         V %#05.5d: %.4f, %.4f, %.4f; %.4f, %.4f, %.4f; %#08.8x;" : 
				"V %#05.5d: %.4f, %.4f, %.4f; %.4f, %.4f, %.4f; %#08.8x; "   ,
				f*3+v, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], indicase);
			fread(buf, 4, 5, fIn); 
			fprintf(fOut, "%.4f, %.4f, %.4f; %.4f, %.4f;", buf[0], buf[1], buf[2], buf[3], buf[4]);
	}	}

	DWORD dwUsedMrpChans; fread(&dwUsedMrpChans, 4, 1, fIn);
	fprintf(fOut, "\n Total used morpher channels: %#05.5d", dwUsedMrpChans);

	if(dwUsedMrpChans < 6)
	{	for(DWORD ch=0; ch<dwUsedMrpChans; ch++)
		{	fprintf(fOut, "\n\n Chan: %#05.5d", ch);
			for(DWORD f=0; f<dwNumFaces; f++)
			{	fprintf(fOut, "\nF %#05.5d: ", f);
				fread(buf, 4, 18, fIn);
				fprintf(fOut, "V %#05.5d: %.4f, %.4f, %4f, %4f, %.4f, %4f",
				 f*3, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]); 
				fprintf(fOut, "\n         V %#05.5d: %.4f, %.4f, %4f, %4f, %.4f, %4f",
				 f*3+1, buf[6], buf[7], buf[8], buf[9], buf[10], buf[11]);
				fprintf(fOut, "\n         V %#05.5d: %.4f, %.4f, %4f, %4f, %.4f, %4f",
				 f*3+2, buf[12], buf[13], buf[14], buf[15], buf[16], buf[17]);
	}	}	}
	else
	{	for(DWORD ch=0; ch<(DWORD)11-dwUsedMrpChans+1; ch++)
		{	fprintf(fOut, "\n Chan: %#05.5d", ch);
			for(DWORD f=0; f<dwNumFaces; f++)
			{	fprintf(fOut, "\nF %#05.5d: ", f);
				fread(buf, 4, 18, fIn);
				fprintf(fOut, "V %#05.5d: %.4f, %.4f, %4f, %4f, %.4f, %4f",
				 f*3, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]); 
				fprintf(fOut, "\n         V %#05.5d: %.4f, %.4f, %4f, %4f, %.4f, %4f",
				 f*3+1, buf[6], buf[7], buf[8], buf[9], buf[10], buf[11]);
				fprintf(fOut, "\n         V %#05.5d: %.4f, %.4f, %4f, %4f, %.4f, %4f",
				 f*3+2, buf[12], buf[13], buf[14], buf[15], buf[16], buf[17]);
		}	}
		for(DWORD ch=11-dwUsedMrpChans+1; ch<dwUsedMrpChans; ch++)
		{	fprintf(fOut, "\n\n Chan: %#05.5d", ch);
			for(DWORD f=0; f<dwNumFaces; f++)
			{	fprintf(fOut, "\nF %#05.5d: ", f);
				fread(buf, 4, 9, fIn);
				fprintf(fOut, "V %#05.5d: %.4f, %.4f, %4f;",
				 f*3, buf[0], buf[1], buf[2]); 
				fprintf(fOut, "\n         V %#05.5d: %.4f, %.4f, %4f;",
				 f*3+1, buf[3], buf[4], buf[5]);
				fprintf(fOut, "\n         V %#05.5d: %.4f, %.4f, %4f;",
				 f*3+2, buf[6], buf[7], buf[8]);
	}	}	}

	fclose(fIn);
	fclose(fOut);
	return TRUE;
}

//Test pasted successfully;
BOOL SoftSkin2MorphIndexedToText(char *pathAndName)
{
FILE	*fIn, *fOut;

	lstrcpy(Name, pathAndName);
	fIn = fopen(Name, "rb");
	if(!fIn) return FALSE;
	lstrcat(strrchr(Name, '.'), "txt");
	fOut = fopen(Name, "w");

	char sh[20];
	fread(sh, 1, 20, fIn);
	if(!(sh[0]=='S'&&sh[1]=='k'&&sh[2]=='n'&&sh[3]=='2'&&sh[4]=='m'&&sh[5]=='r'&&sh[6]=='p'&&sh[9]=='I'))
	{	fclose(fIn);
		fclose(fOut);
		return FALSE;
	}
	BOOL b32bit = (sh[7] == '3' && sh[8] == '2');

	float buf[24];
	fread(buf, 4, 6, fIn);
	fprintf(fOut, "\n Position: %.4f, %.4f, %.4f;\n Rotation in EulerXYZ: %.4f, %.4f, %.4f;",
					buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);

	DWORD dwNumFaces[2]; fread(dwNumFaces, 4, 2, fIn);
	fprintf(fOut, "\n Number of unused morph faces: %#05.5d, Number of used morph faces: %#05.5d", dwNumFaces[0], dwNumFaces[1]);
	DWORD numCompVerts[2]; fread(numCompVerts, 4, 2, fIn);
	fprintf(fOut, "\n Number of unused morph vertices: %#05.5d, Number of used morph vertices: %#05.5d", numCompVerts[0], numCompVerts[1]);

	fread(buf, 4, 6, fIn);
	fprintf(fOut, "\n Bound box sizes: %.4f, %.4f, %.4f, %.4f, %.4f, %.4f;", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);

	WORD wNumBones;	fread(&wNumBones, 2, 1, fIn);
	fprintf(fOut, "\n\n Number of bones: %#05.5d\nBones: ", wNumBones);
	for(WORD b=0; b<wNumBones; b++)
	{	char n[4]; n[0] = 32; fprintf(fOut, "\n%#05.5d ", b);
		while(n[0]) { fread(n, 1, 1, fIn); fwrite(n, 1, 1, fOut); }
	}

	fprintf(fOut, "\n\n Number of unused faces for each bone:");
	for(WORD b=0; b<wNumBones; b++)
	{ WORD nf; fread(&nf, 2, 1, fIn); fprintf(fOut, "\n%#05.5d %#05.5d", b, nf); }

	fprintf(fOut, "\n\n Number of used faces for each bone:");
	for(WORD b=0; b<wNumBones; b++)
	{ WORD nf; fread(&nf, 2, 1, fIn); fprintf(fOut, "\n%#05.5d %#05.5d", b, nf); }

	fread(buf, 4, 16, fIn);
	fprintf(fOut, "\n\n MeshTM: %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f;", 
			buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], 
			buf[9], buf[10], buf[11], buf[12], buf[13], buf[14], buf[15]);

	fprintf(fOut, "\n\n Bones InitTMs:"); 
	for(WORD b=0; b<wNumBones; b++)
	{	fprintf(fOut, "\n%#05.5d ", b);
		fread(buf, 4, 16, fIn);
		fprintf(fOut, "\n %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f;", 
			buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], 
			buf[9], buf[10], buf[11], buf[12], buf[13], buf[14], buf[15]);
	}

	BYTE maxAsBn; fread(&maxAsBn, 1, 1, fIn);
	fprintf(fOut, "\n\n Maximum assigned bones: %#05.5d", maxAsBn);

	fprintf(fOut, "\n\n Unused morph vertice positions:               Normals:   Texture coordinates:");
	for(WORD v=0; v<numCompVerts[0]; v++)
	{	fread(buf, 4, 8, fIn);
		fprintf(fOut, "\nV %#05.5d: %.4f, %.4f, %4f, %4f, %.4f, %4f, %4f, %.4f;",
		 v, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);
	}
	fprintf(fOut, "\n\n Used morph vertice positions:               Normals:   Texture coordinates:");
	for(WORD v=0; v<numCompVerts[1]; v++)
	{	fread(buf, 4, 8, fIn);
		fprintf(fOut, "\nV %#05.5d: %.4f, %.4f, %4f, %4f, %.4f, %4f, %4f, %.4f;",
		 v, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);
	}

	DWORD ind[3]; fprintf(fOut, "\n\n Unused morph faces, in indexes:");
	for(DWORD f=0; f<dwNumFaces[0]; f++)
	{	ind[0] = ind[1] = ind[2] = 0;
		fread(&ind[0], b32bit?4:2, 1, fIn);
		fread(&ind[1], b32bit?4:2, 1, fIn);
		fread(&ind[2], b32bit?4:2, 1, fIn);
		fprintf(fOut, "\nF %#05.5d: %#05.5d, %#05.5d, %#05.5d", f, ind[0], ind[1], ind[2]);
	}
	fprintf(fOut, "\n\n Used morph faces, in indexes:");
	for(DWORD f=0; f<dwNumFaces[1]; f++)
	{	ind[0] = ind[1] = ind[2] = 0;
		fread(&ind[0], b32bit?4:2, 1, fIn);
		fread(&ind[1], b32bit?4:2, 1, fIn);
		fread(&ind[2], b32bit?4:2, 1, fIn);
		fprintf(fOut, "\nF %#05.5d: %#05.5d, %#05.5d, %#05.5d", f, ind[0], ind[1], ind[2]);
	}

	DWORD sz; fpos_t posSz; fread(&sz, 4, 1, fIn);
	fgetpos(fIn, &posSz);
	fprintf(fOut, "\n\n Unused morph vertice weights:");
	for(DWORD v=0; v<numCompVerts[0]; v++)
	{	fprintf(fOut, "\nV %#05.5d: ", v);
		WORD bones; fread(&bones, 2, 1, fIn);
		fprintf(fOut, "Total bones - %#05.5d; ", bones);
		for(WORD b=0; b<bones; b++)
		{	WORD which;  fread(&which, 2, 1, fIn);
						 fread(buf, 4, 1, fIn);
			fprintf(fOut, " bone %#05.5d:, weight: %.4f", which, buf[0]);
	}	}
	fprintf(fOut, "\n\n Used morph vertice weights:");
	for(DWORD v=0; v<numCompVerts[1]; v++)
	{	fprintf(fOut, "\nV %#05.5d: ", v);
		WORD bones; fread(&bones, 2, 1, fIn);
		fprintf(fOut, "Total bones - %#05.5d; ", bones);
		for(WORD b=0; b<bones; b++)
		{	WORD which;  fread(&which, 2, 1, fIn);
						 fread(buf, 4, 1, fIn);
			fprintf(fOut, " bone %#05.5d:, weight: %.4f", which, buf[0]);
	}	}
	fseek(fIn, posSz+sz, SEEK_SET);//weights

	DWORD dwUsedMrpChans; fread(&dwUsedMrpChans, 4, 1, fIn);
	fprintf(fOut, "\n\n Total used morpher channels: %#05.5d", dwUsedMrpChans);
	for(DWORD ch=0; ch<dwUsedMrpChans; ch++)
	{	DWORD pdwJamDefVertl; fread(&pdwJamDefVertl, 4, 1, fIn);
		fprintf(fOut, "\n\n Channel number: %#05.5d\n Total used vertices: %#05.5d", ch, pdwJamDefVertl);
		for(DWORD v=0; v<pdwJamDefVertl; v++)
		{	DWORD vNum=0; fread(&vNum, b32bit?4:2, 1, fIn);
			fread(buf, 4, 6, fIn);
			fprintf(fOut, "\n V %#05.5d: %.4f, %.4f, %.4f; N: %.4f, %.4f, %.4f;",
					vNum, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
	}	}

	fclose(fIn);
	fclose(fOut);
	return TRUE;
}

BOOL SHSkin2MorphIndexedToText(char *pathAndName)
{
FILE	*fIn, *fOut;

	lstrcpy(Name, pathAndName);
	fIn = fopen(Name, "rb");
	if(!fIn) return FALSE;
	lstrcat(strrchr(Name, '.'), "txt");
	fOut = fopen(Name, "w");

	char sh[20];
	fread(sh, 1, 20, fIn);
	if(!(sh[0]=='S'&&sh[1]=='k'&&sh[2]=='n'&&sh[3]=='2'&&sh[4]=='m'&&sh[5]=='r'&&sh[6]=='p'&&sh[9]=='S'&&sh[10]=='H'&&sh[11]=='I'))
	{	fclose(fIn);
		fclose(fOut);
		return FALSE;
	}
	BOOL b32bit = (sh[7] == '3' && sh[8] == '2');
	BOOL bF3UN4F16_2 = (sh[12] == 'U');

	float buf[24];
	fread(buf, 4, 6, fIn);
	fprintf(fOut, "\n Position: %.4f, %.4f, %.4f;\n Rotation in EulerXYZ: %.4f, %.4f, %.4f;",
					buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);

	DWORD dwNumFaces[2]; fread(dwNumFaces, 4, 2, fIn);
	fprintf(fOut, "\n Number of unused morph faces: %#05.5d,  Number of used morph faces: %#05.5d", dwNumFaces[0], dwNumFaces[1]);
	DWORD numVerts[2]; fread(numVerts, 4, 2, fIn); fprintf(fOut, "\n Number of unused morph vertices: %#05.5d, Number of used morph vertices: %#05.5d", numVerts[0], numVerts[1]);

	fread(buf, 4, 6, fIn);
	fprintf(fOut, "\n Bound box sizes: %.4f, %.4f, %.4f, %.4f, %.4f, %.4f;", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);

	WORD wNumBones;	fread(&wNumBones, 2, 1, fIn);
	fprintf(fOut, "\n\n Number of bones: %#05.5d\nBones: ", wNumBones);
	for(WORD b=0; b<wNumBones; b++)
	{	char n[4]; n[0] = 32; fprintf(fOut, "\n%#05.5d ", b);
		while(n[0]) { fread(n, 1, 1, fIn); fwrite(n, 1, 1, fOut); }
 	}

	fprintf(fOut, "\n\n Unused morph faces for each bones:");
	for(WORD b=0; b<wNumBones; b++)
	{	WORD boneFaces; fread(&boneFaces, 2, 1, fIn);
		fprintf(fOut, "\n%#05.5d - %#05.5d", b, boneFaces);
	}

	fprintf(fOut, "\n\n Used morph faces for each bones:");
	for(WORD b=0; b<wNumBones; b++)
	{	WORD boneFaces; fread(&boneFaces, 2, 1, fIn);
		fprintf(fOut, "\n%#05.5d - %#05.5d", b, boneFaces);
	}

	fread(buf, 4, 16, fIn);
	fprintf(fOut, "\n\n MeshTM: %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f;", 
			buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], 
			buf[9], buf[10], buf[11], buf[12], buf[13], buf[14], buf[15]);

	fprintf(fOut, "\n\n Bones InitTMs:"); 
	for(WORD b=0; b<wNumBones; b++)
	{	fprintf(fOut, "\n%#05.5d ", b);
		fread(buf, 4, 16, fIn);
		fprintf(fOut, "\n %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f;", 
			buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], 
			buf[9], buf[10], buf[11], buf[12], buf[13], buf[14], buf[15]);
	}

	fprintf(fOut, "\n\n Unused morph vertices positions:  Blend0: Blend1: Blend2: Indicase: Normals: Texture coordinates:");
	for(DWORD v=0; v<numVerts[0]; v++)
	{	fread(buf, 4, 3, fIn);
		fprintf(fOut, "\nV %#05.5d: %.4f, %.4f, %4f;", v, buf[0], buf[1], buf[2]);
		
		BYTE norms[4];//as weights;
		if(bF3UN4F16_2)
		{	fread(norms, 1, 4, fIn);
			fprintf(fOut, " %d, %d, %d, %d", norms[0], norms[1], norms[2], norms[3]);
		}
		else
		{	fread(buf, 4, 3, fIn);
			fprintf(fOut, " %.4f, %.4f, %4f;", buf[0], buf[1], buf[2]);
		}
		
		DWORD inds; fread(&inds, 4, 1, fIn);
		fprintf(fOut, " %#05.5x; ",	inds);
		if(bF3UN4F16_2)
		{	fread(norms, 1, 4, fIn);
			fprintf(fOut, " %d, %d, %d, %d", norms[0], norms[1], norms[2], norms[3]);
			D3DXFLOAT16 tutv[2]; fread(tutv, 2, 2, fIn);
			myD3DXFloat16To32Array(buf,tutv,2);
			fprintf(fOut, " %.4f, %.4f", buf[0], buf[1]);
		}
		else
		{	fread(buf, 4, 5, fIn);
			fprintf(fOut, "%.4f, %.4f, %4f, %4f, %4f;", buf[0], buf[1], buf[2], buf[3], buf[4]);
	}	}

	fprintf(fOut, "\n\n Used morph vertices positions:  Blend0: Blend1: Blend2: Indicase: Normals: Texture coordinates:");
	for(DWORD v=0; v<numVerts[1]; v++)
	{	fread(buf, 4, 3, fIn);
		fprintf(fOut, "\nV %#05.5d: %.4f, %.4f, %4f",
		 v, buf[0], buf[1], buf[2]);

		BYTE norms[4];
		if(bF3UN4F16_2)
		{	fread(norms, 1, 4, fIn);
			fprintf(fOut, " %d, %d, %d, %d", norms[0], norms[1], norms[2], norms[3]);
		}
		else
		{	fread(buf, 4, 3, fIn);
			fprintf(fOut, " %.4f, %.4f, %4f;", buf[0], buf[1], buf[2]);
		}
		
		DWORD inds; fread(&inds, 4, 1, fIn);
		fprintf(fOut, " %#05.5x; ",	inds);
		if(bF3UN4F16_2)
		{	fread(norms, 1, 4, fIn);
			fprintf(fOut, " %d, %d, %d, %d", norms[0], norms[1], norms[2], norms[3]);
			D3DXFLOAT16 tutv[2]; fread(tutv, 2, 2, fIn);
			myD3DXFloat16To32Array(buf,tutv,2);
			fprintf(fOut, " %.4f, %.4f", buf[0], buf[1]);
		}
		else
		{	fread(buf, 4, 5, fIn);
			fprintf(fOut, "%.4f, %.4f, %4f, %4f, %4f;", buf[0], buf[1], buf[2], buf[3], buf[4]);
	}	}

	WORD ind[3]; fprintf(fOut, "\n\n\n Unused morph faces, in indexes:");
	for(DWORD f=0; f<dwNumFaces[0]; f++)
	{	fread(ind, 2, 3, fIn);
		fprintf(fOut, "\nF %#05.5d: %#05.5d, %#05.5d, %#05.5d", f, ind[0], ind[1], ind[2]);
	}

	fprintf(fOut, "\n\n Used morph faces, in indexes:");
	for(DWORD f=0; f<dwNumFaces[1]; f++)
	{	fread(ind, 2, 3, fIn);
		fprintf(fOut, "\nF %#05.5d: %#05.5d, %#05.5d, %#05.5d", f, ind[0], ind[1], ind[2]);
	}

	DWORD dwUsedMrpChans; fread(&dwUsedMrpChans, 4, 1, fIn);
	fprintf(fOut, "\n\n Total used morpher channels: %#05.5d", dwUsedMrpChans);

	for(DWORD ch=0; ch<dwUsedMrpChans; ch++)
	{	fprintf(fOut, "\n\n Channel number: %#05.5d", ch);
		for(WORD v=0; v<numVerts[1]; v++)
		{	fread(buf, 4, 3, fIn);//V(pos)
			fprintf(fOut, "\nV %#05.5d: %.4f, %.4f, %.4f;",
			v, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
			/*if(ch < 6)
			{	fread(buf, 4, 3, fIn);//V(pos)
				fprintf(fOut, "\nV %#05.5d: %.4f, %.4f, %.4f;",
					v, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
				if(bF3UN4F16_2)
				{	BYTE norms[4]; fread(norms, 1, 4, fIn);
					fprintf(fOut, " %d, %d, %d, %d", norms[0], norms[1], norms[2], norms[3]);
				}
				else
				{	fread(buf, 4, 3, fIn);
					fprintf(fOut, " N: %.4f, %.4f, %.4f;",
						buf[0], buf[1], buf[2]);
			}	}
			else
			{	if(ch < MAX_SKIN2MORPH_CHANNEL-dwUsedMrpChans+1)
				{	fread(buf, 4, 3, fIn);//V(pos)
					fprintf(fOut, "\nV %#05.5d: %.4f, %.4f, %.4f;",
						v, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
					if(bF3UN4F16_2)
					{	BYTE norms[4]; fread(norms, 1, 4, fIn);
						fprintf(fOut, " %d, %d, %d, %d", norms[0], norms[1], norms[2], norms[3]);
					}
					else
					{	fread(buf, 4, 3, fIn);
						fprintf(fOut, " N: %.4f, %.4f, %.4f;",
							buf[0], buf[1], buf[2]);
				}	}
				else
				{	fread(buf, 4, 3, fIn);
					fprintf(fOut, "\nV %#05.5d: %.4f, %.4f, %.4f;",
							v, buf[0], buf[1], buf[2]);
	}	}*/	}	}

	fclose(fIn);
	fclose(fOut);
	return TRUE;
}

//Har bir bone uchun alohida indexlar yozamiz, qaysiki har bir bone ni texture b-n
//alohida chizish imkoniyati bo'lsin:
BOOL SortForTextrIndexedSkin2Morph(char *pathAndName)
{
FILE	*fIn;

	lstrcpy(Name, pathAndName);
	fIn = fopen(Name, "r+b");
	if(!fIn) return FALSE;

	char sh[20];
	fread(sh, 1, 20, fIn);
	if(!(sh[0]=='S'&&sh[1]=='k'&&sh[2]=='n'&&sh[3]=='2'&&sh[4]=='m'&&sh[5]=='r'&&sh[6]=='p'&&sh[9]=='I'))
	{	fclose(fIn);
		return FALSE;
	}
	BOOL b32bit = (sh[7] == '3' && sh[8] == '2');

	fseek(fIn, 4*6, SEEK_CUR);//Position,Rotation; frite dn so'ng 1 ta surildiku;
	DWORD dwNumFaces[2]; fread(dwNumFaces, 4, 2, fIn);
	DWORD dwNumVerts[2]; fread(dwNumVerts, 4, 2, fIn);

	fseek(fIn, 4*6, SEEK_CUR);//Bound box;
	WORD wNumBones;	fread(&wNumBones, 2, 1, fIn);

	NEW(char*, pBoneNames, wNumBones*4)

	for(WORD b=0; b<wNumBones; b++)
	{	char n[64], *pn = &n[0]; n[0] = 32;
		do { fread(pn, 1, 1, fIn); } while(*pn++);
		*pn = '\0'; pBoneNames[b] = (char*)malloc(lstrlen(n)+4);
		lstrcpy(pBoneNames[b], n);
	}
	fpos_t posBoneFaces; fgetpos(fIn, &posBoneFaces);
	fseek(fIn, 4*wNumBones, SEEK_CUR);//bones unused and used faces, 2 tadan 2 ta=4ta;
	fseek(fIn, 4*16, SEEK_CUR);//MeshTM
	fseek(fIn, 4*16*wNumBones, SEEK_CUR);//BoneInitTMs
	char maxAssgBone; fread(&maxAssgBone, 1, 1, fIn);

	fseek(fIn, 4*8*(dwNumVerts[0]+dwNumVerts[1]), SEEK_CUR);//Vertices

	fpos_t posInds; fgetpos(fIn, &posInds);
	WORD *inds0=NULL, *inds1=NULL;
	DWORD *inds20=NULL, *inds21=NULL;
	if(b32bit)
	{	RENEW(DWORD, inds20, dwNumFaces[0]*4*3)
		fread(inds20, 4*3, dwNumFaces[0], fIn);
		RENEW(DWORD, inds21, dwNumFaces[1]*4*3)
		fread(inds21, 4*3, dwNumFaces[1], fIn);
	}
	else
	{	RENEW(WORD, inds0, dwNumFaces[0]*2*3)
		fread(inds0, 2*3, dwNumFaces[0], fIn);
		RENEW(WORD, inds1, dwNumFaces[1]*2*3)
		fread(inds1, 2*3, dwNumFaces[1], fIn);
	}

	DWORD sz; fread(&sz, 4, 1, fIn);
	NEW(WORD, wBoneOfVerts0, dwNumVerts[0]*2)
	NEW(WORD, which, 2*maxAssgBone);
	NEW(float,weight, 4*maxAssgBone);
	for(DWORD v=0; v<dwNumVerts[0]; v++)
	{	//if(v==466)
		//	Beep(100,100);
		
		WORD bones; fread(&bones, 2, 1, fIn);
		for(WORD b=0; b<bones; b++)
		{	fread(&which [b], 2, 1, fIn);
			fread(&weight[b], 4, 1, fIn);
		}
		float blMax = 0.0f; wBoneOfVerts0[v] = 0xffff;
		for(DWORD b=0; b<bones; b++)
		{	if(blMax < weight[b])
			{ blMax = weight[b]; wBoneOfVerts0[v] = which[b]; }
	}	}
	NEW(WORD, wBoneOfVerts1, dwNumVerts[1]*2)
	for(DWORD v=0; v<dwNumVerts[1]; v++)
	{	WORD bones; fread(&bones, 2, 1, fIn);
		//if(v==4920)
		//	Beep(100,100);
		for(WORD b=0; b<bones; b++)
		{	fread(&which [b], 2, 1, fIn);
			fread(&weight[b], 4, 1, fIn);
		}
		float blMax = 0.0f; wBoneOfVerts1[v] = 0xffff;
		for(DWORD b=0; b<bones; b++)
		{	if(blMax < weight[b])
			{ blMax = weight[b]; wBoneOfVerts1[v] = which[b]; }
	}	}
	DEL(which )
	DEL(weight)

	NEW(BYTE, Overr0, dwNumFaces[0])//hammasi 1 talik b-sa hamma bone gayam yozmaslik uchun;
	NEW(WORD, wBoneFaceNums0, wNumBones*2)
	ZeroMemory(Overr0, dwNumFaces[0]);
	ZeroMemory(wBoneFaceNums0, wNumBones*2);

	NEW(BYTE, Overr1, dwNumFaces[1])//hammasi 1 talik b-sa hamma bone gayam yozmaslik uchun;
	NEW(WORD, wBoneFaceNums1, wNumBones*2)
	ZeroMemory(Overr1, dwNumFaces[1]);
	ZeroMemory(wBoneFaceNums1, wNumBones*2);

	fseek(fIn, posInds, SEEK_SET);
	DWORD totFaces0 = 0;
	for(WORD b=0; b<wNumBones; b++)
	{	for(DWORD f=0; f<dwNumFaces[0]; f++)
		{	BOOL Wr = FALSE; DWORD B[3];
			if(b32bit)
			{	B[0] = wBoneOfVerts0[inds20[3*f  ]];
				B[1] = wBoneOfVerts0[inds20[3*f+1]];
				B[2] = wBoneOfVerts0[inds20[3*f+2]];
			} else
			{	B[0] = wBoneOfVerts0[inds0[3*f  ]];
				B[1] = wBoneOfVerts0[inds0[3*f+1]];
				B[2] = wBoneOfVerts0[inds0[3*f+2]];
			}
			if((B[0]==b && B[1]==b) || (B[1]==b && B[2]==b) || (B[0]==b && B[2]==b)) Wr = TRUE;
			else if(B[0]==b && B[1]!=B[2]) Wr = TRUE;
			else if(B[1]==b && B[0]!=B[2]) Wr = TRUE;
			else if(B[2]==b && B[0]!=B[1]) Wr = TRUE;
			if(Wr)
			{	if((strstr(pBoneNames[b], "Head") || strstr(pBoneNames[b], "Finger") || strstr(pBoneNames[b], "Hand"))
					&& !(B[0]==b && B[1]==b && B[2]==b))
					continue;					
			}
			else
			{	if(strstr(pBoneNames[B[0]], "Head") || strstr(pBoneNames[B[0]], "Finger") || strstr(pBoneNames[B[0]], "Hand"))
				if(B[1] != B[2]) Wr = TRUE;
				if(strstr(pBoneNames[B[1]], "Head") || strstr(pBoneNames[B[1]], "Finger") || strstr(pBoneNames[B[1]], "Hand"))
				if(B[0] != B[2]) Wr = TRUE;			
				if(strstr(pBoneNames[B[2]], "Head") || strstr(pBoneNames[B[2]], "Finger") || strstr(pBoneNames[B[2]], "Hand"))
				if(B[0] != B[1]) Wr = TRUE;			
			}
			if(Wr && (!Overr0[f]))
			{	if(b32bit)
					fwrite(&inds20[f*3], 2, 3, fIn);
				else
					fwrite(&inds0[f*3], 2, 3, fIn);
				Overr0[f] = 1;
				totFaces0++;
				wBoneFaceNums0[b]++;
	}	}	}
	if(totFaces0 != dwNumFaces[0]) 
	{ MessageBox(NULL, "totFaces0 != wNumFaces[0]", "SortForTextrIndexedSkin2Morph", MB_OK); goto End; }

	DWORD totFaces1 = 0;
	for(WORD b=0; b<wNumBones; b++)
	{	for(DWORD f=0; f<dwNumFaces[1]; f++)
		{	BOOL Wr = FALSE; DWORD B[3];
			if(b32bit)
			{	B[0] = wBoneOfVerts1[inds21[3*f  ]];
				B[1] = wBoneOfVerts1[inds21[3*f+1]];
				B[2] = wBoneOfVerts1[inds21[3*f+2]];
			} else
			{	B[0] = wBoneOfVerts1[inds1[3*f  ]];
				B[1] = wBoneOfVerts1[inds1[3*f+1]];
				B[2] = wBoneOfVerts1[inds1[3*f+2]];
			}
			if((B[0]==b && B[1]==b) || (B[1]==b && B[2]==b) || (B[0]==b && B[2]==b)) Wr = TRUE;
			else if(B[0]==b && B[1]!=B[2]) Wr = TRUE;
			else if(B[1]==b && B[0]!=B[2]) Wr = TRUE;
			else if(B[2]==b && B[0]!=B[1]) Wr = TRUE;
			if(Wr)
			{	if((strstr(pBoneNames[b], "Head") || strstr(pBoneNames[b], "Finger") || strstr(pBoneNames[b], "Hand"))
					&& !(B[0]==b && B[1]==b && B[2]==b))
					continue;					
			}
			else
			{	if(strstr(pBoneNames[B[0]], "Head") || strstr(pBoneNames[B[0]], "Finger") || strstr(pBoneNames[B[0]], "Hand"))
				if(B[1] != B[2]) Wr = TRUE;
				if(strstr(pBoneNames[B[1]], "Head") || strstr(pBoneNames[B[1]], "Finger") || strstr(pBoneNames[B[1]], "Hand"))
				if(B[0] != B[2]) Wr = TRUE;			
				if(strstr(pBoneNames[B[2]], "Head") || strstr(pBoneNames[B[2]], "Finger") || strstr(pBoneNames[B[2]], "Hand"))
				if(B[0] != B[1]) Wr = TRUE;			
			}
			if(Wr && (!Overr1[f]))
			{	if(b32bit)
					fwrite(&inds21[f*3], 2, 3, fIn);
				else
					fwrite(&inds1[f*3], 2, 3, fIn);
				Overr1[f] = 1;
				totFaces1++;
				wBoneFaceNums1[b]++;
	}	}	}
	if(totFaces1 != dwNumFaces[1]) 
	{ MessageBox(NULL, "totFaces1 != wNumFaces[1]", "SortForTextrIndexedSkin2Morph", MB_OK); goto End; }

	fseek(fIn, posBoneFaces, SEEK_SET);
	for(WORD b=0; b<wNumBones; b++) fwrite(&wBoneFaceNums0[b], 2, 1, fIn);
	for(WORD b=0; b<wNumBones; b++) fwrite(&wBoneFaceNums1[b], 2, 1, fIn);

End:
	DEL(Overr0		  )
	DEL(Overr1        )
	DEL(wBoneFaceNums0)
	DEL(wBoneFaceNums1)
	DEL(wBoneOfVerts0 )
	DEL(wBoneOfVerts1 )
	DEL(inds20        )
	DEL(inds21        )
	DEL(inds0         )
	DEL(inds1         )
	for(WORD b=0; b<wNumBones; b++) free(pBoneNames[b]);
	DEL(pBoneNames    )
	fclose(fIn);
	return TRUE;
}

//Test pasted successfulli;
BOOL ConvertIndexedSkin2MorphToSHFormat(char *pathAndName)
{
FILE *fIn, *fOut;

	lstrcpy(Name, pathAndName);
	fIn = fopen(Name, "rb");
	if(!fIn) return FALSE;
	Name[lstrlen(Name)-2] = 'b';
	Name[lstrlen(Name)-1] = '6';
	fOut = fopen(Name, "wb");
//************************************************************************************
	char sh[20];
	fread(sh, 1, 20, fIn);
	if(!(sh[0]=='S'&&sh[1]=='k'&&sh[2]=='n'&&sh[3]=='2'&&sh[4]=='m'&&sh[5]=='r'&&sh[6]=='p'&&sh[9]=='I'))
	{	fclose(fIn);
		fclose(fOut);
		return FALSE;
	}
	BOOL b32bit = (sh[3] == '7' && sh[8] == '2');
	sh[9]='S';sh[10]='H';sh[11]='I'; fwrite(sh, 1, 20, fOut);
//************************************************************************************
	float buf[64];
	fread (buf, 4, 6, fIn);//pos, EUL;
	fwrite(buf, 4, 6, fOut);
	DWORD dwNumFaces[2]; fread(dwNumFaces, 4, 2, fIn); fwrite(dwNumFaces, 4, 2, fOut);
	DWORD dwNumVerts[2]; fread(dwNumVerts, 4, 2, fIn); fwrite(dwNumVerts, 4, 2, fOut);
	fread(buf, 6, 4, fIn);//fXMin fXMax;
	fwrite(buf, 6, 4, fOut);
//************************************************************************************
	WORD wNumBones; fread(&wNumBones, 2, 1, fIn);
	fwrite(&wNumBones, 2, 1, fOut);
	for(WORD b=0; b<wNumBones; b++)
	{	char n[4]; n[0] = 32;
		while(n[0]) { fread(n, 1, 1, fIn); fwrite(n, 1, 1, fOut); }
	}
	for(WORD b=0; b<2*wNumBones; b++)
	{	WORD boneFaces;
		fread(&boneFaces, 2, 1, fIn); fwrite(&boneFaces, 2, 1, fOut);
	}
//************************************************************************************
	fread (buf, 4, 16, fIn );//meshTM;
	fwrite(buf, 4, 16, fOut);
//************************************************************************************
	for(WORD inTm=0; inTm<wNumBones; inTm++)
	{	fread (buf, 4, 16, fIn);//bone InitTMs;
		fwrite(buf, 4, 16, fOut);
	}
	char maxAssignBone; fread(&maxAssignBone, 1, 1, fIn);
	if(maxAssignBone > 4)
		MessageBox(NULL, "Max blend matrice or bones for each vertex above the norm(4) for the b3ind verts.", "Canceling converting.", MB_OK);

	NEW(float, pVertices, 4*8*(dwNumVerts[0]+dwNumVerts[1]))
	fread(pVertices, 4, (dwNumVerts[0]+dwNumVerts[1])*8, fIn);
	NEW(WORD, inds, 2 * (dwNumFaces[0]+dwNumFaces[1]) * 3)
	fread(inds, 2, (dwNumFaces[0]+dwNumFaces[1]) * 3, fIn);

//************************************************************************************
	DWORD sz; fread(&sz, 4, 1, fIn);
	NEW(BYTE, pvVertBoneIndsAndWeights, sz)
	fread(pvVertBoneIndsAndWeights, 1, sz, fIn);

	WORD v, numBones; DWORD pdwJamDefVertl, dwUsedMrpChans, indices;
	fread(&dwUsedMrpChans, 4, 1, fIn);
//************************************************************************************
	BYTE *pcWeightTable = pvVertBoneIndsAndWeights;
	for(v=0; v<dwNumVerts[0]+dwNumVerts[1]; v++)
	{	fwrite(&pVertices[v*8], 4, 3, fOut);//pos
		float blend[3] = {0.0f};
		indices = 0x00000000;
		numBones = *((WORD*)pcWeightTable); 
		pcWeightTable += sizeof(WORD);
		if(numBones == 1)
		{	indices = *((WORD*)pcWeightTable);
			indices &= 0x000000ff;
			blend[0] = 1.0f;
			pcWeightTable += sizeof(WORD) + sizeof(float);
		}
		else
		{	WORD ind[4] = {0};
			for (int j=0;j<numBones;j++)
			{	if(j<4) ind[j] = *((WORD*)pcWeightTable);//4 dan ortig'ini shunday tashlab yuboramiz;
					pcWeightTable += sizeof(WORD);
				if(j<3) blend[j]  = *((float*)pcWeightTable);//4 dan ortig'ini shunday tashlab yuboramiz;
					pcWeightTable += sizeof(float);
			}
			indices =	(ind[0] & 0x000000ff) | 
						((ind[1] & 0x000000ff) << 8 ) |
						((ind[2] & 0x000000ff) << 16) |
						((ind[3] & 0x000000ff) << 24) ;
		}
		fwrite(blend, 4, 3, fOut);//blend
		fwrite(&indices, 4, 1, fOut);//indicases
		fwrite(&pVertices[v*8+3], 4, 3, fOut);//normal
		fwrite(&pVertices[v*8+6], 4, 2, fOut);//tu-tv
	}

	fwrite(inds, 2, (dwNumFaces[0]+dwNumFaces[1]) * 3, fOut);

	//Morph part:
	//Morph part:
	//Morph part:
	fwrite(&dwUsedMrpChans, 4, 1, fOut);
	NEW(float, mrpDeltas, dwNumVerts[1]*6*4)//VN 4-sizeof(float), 3*6=18 ta float
	//Writing the morph part:
	for(DWORD c = 0; c < dwUsedMrpChans; c++)
	{	ZeroMemory(mrpDeltas, dwNumVerts[1]*6*4);//Zero deltas;
		fread (&pdwJamDefVertl, 4, 1, fIn);
		for(DWORD v=0; v<pdwJamDefVertl; v++)
		{	DWORD vNum=0; fread(&vNum, b32bit?4:2, 1, fIn);
			if(vNum >= dwNumVerts[1]) { Beep(100, 100); goto End; }
			float d[6]; fread(d, 4, 6, fIn);
			for(int id=0; id<6; id++)
				mrpDeltas[vNum*6+id] += d[id];
		}
		for(DWORD v=0; v<dwNumVerts[1]; v++)
		{	fwrite(&mrpDeltas[v*6], 4, 3, fOut);
			//16.03.2011 dan o'zgartirilgan, MRP kanallari faqat V dir;
			//5 kanallikdan 11 kanallikka o'tgandan so'ng quyidagiga o'zgardi.
			/*if(c < 6) fwrite(&mrpDeltas[v*6], 4, 6, fOut);
			else
			{	if(c < (dwUsedMrpChans < MAX_SKIN2MORPH_CHANNEL ? MAX_SKIN2MORPH_CHANNEL-dwUsedMrpChans+1 : 1))
					 fwrite(&mrpDeltas[v*6], 4, 6, fOut);
				else
					 fwrite(&mrpDeltas[v*6], 4, 3, fOut);
	}*/	}	}
End:
	fclose(fOut);
	fclose(fIn);
	DEL(pVertices)
	DEL(pvVertBoneIndsAndWeights)
	DEL(mrpDeltas)
	return TRUE;
}

//Shader skin2morph indexed tangent-binormal
BOOL SHSkin2MorphTBIndexedToText(char *pathAndName)
{
FILE	*fIn, *fOut;

	lstrcpy(Name, pathAndName);
	fIn = fopen(Name, "rb");
	if(!fIn) return FALSE;
	lstrcat(strrchr(Name, '.'), "txt");
	fOut = fopen(Name, "w");

	char sh[20];
	fread(sh, 1, 20, fIn);
	if(!(sh[0]=='S'&&sh[1]=='k'&&sh[2]=='n'&&sh[3]=='2'&&sh[4]=='m'&&sh[5]=='r'&&sh[6]=='p'&&sh[9]=='S'&&sh[10]=='H'&&sh[11]=='T'&&sh[12]=='B'))
	{	fclose(fIn);
		fclose(fOut);
		return FALSE;
	}
	BOOL b32bit = (sh[7] == '3' && sh[8] == '2');
	BOOL bF3UN4F16_2 = (sh[13] == 'U');
	BOOL bStrip = (sh[14] == 'S');
	if(bStrip)fprintf(fOut, "\n TRIANGLE STRIP:");

	float buf[24];
	fread(buf, 4, 6, fIn);
	fprintf(fOut, "\n Position: %.4f, %.4f, %.4f;\n Rotation in EulerXYZ: %.4f, %.4f, %.4f;",
					buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);

	DWORD dwNumFaces[2]; fread(dwNumFaces, 4, 2, fIn);
	fprintf(fOut, "\n Number of unused morph faces: %#05.5d,  Number of used morph faces: %#05.5d", dwNumFaces[0], dwNumFaces[1]);

	DWORD dwNumStripFaces[2];
	if(bStrip)
	{	fread(dwNumStripFaces, 4, 2, fIn);
		fprintf(fOut, "\n Number of unused morph strip faces: %#05.5d,  Number of used morph faces: %#05.5d", dwNumStripFaces[0], dwNumStripFaces[1]);
	}
	
	DWORD numVerts[2]; fread(numVerts, 4, 2, fIn); fprintf(fOut, "\n Number of unused morph vertices: %#05.5d, Number of used morph vertices: %#05.5d", numVerts[0], numVerts[1]);

	fread(buf, 4, 6, fIn);
	fprintf(fOut, "\n Bound box sizes: %.4f, %.4f, %.4f, %.4f, %.4f, %.4f;", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);

	WORD wNumBones;	fread(&wNumBones, 2, 1, fIn);
	fprintf(fOut, "\n\n Number of bones: %#05.5d\nBones: ", wNumBones);
	for(WORD b=0; b<wNumBones; b++)
	{	char n[4]; n[0] = 32; fprintf(fOut, "\n%#05.5d ", b);
		while(n[0]) { fread(n, 1, 1, fIn); fwrite(n, 1, 1, fOut); }
 	}

	fprintf(fOut, "\n\n Unused morph faces for each bones:");
	for(WORD b=0; b<wNumBones; b++)
	{	WORD boneFaces; fread(&boneFaces, 2, 1, fIn);
		fprintf(fOut, "\n%#05.5d - %#05.5d", b, boneFaces);
	}

	fprintf(fOut, "\n\n Used morph faces for each bones:");
	for(WORD b=0; b<wNumBones; b++)
	{	WORD boneFaces; fread(&boneFaces, 2, 1, fIn);
		fprintf(fOut, "\n%#05.5d - %#05.5d", b, boneFaces);
	}

	fread(buf, 4, 16, fIn);
	fprintf(fOut, "\n\n MeshTM: %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f;", 
			buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], 
			buf[9], buf[10], buf[11], buf[12], buf[13], buf[14], buf[15]);

	fprintf(fOut, "\n\n Bones InitTMs:"); 
	for(WORD b=0; b<wNumBones; b++)
	{	fprintf(fOut, "\n%#05.5d ", b);
		fread(buf, 4, 16, fIn);
		fprintf(fOut, "\n %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f;", 
			buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], 
			buf[9], buf[10], buf[11], buf[12], buf[13], buf[14], buf[15]);
	}

	fprintf(fOut, "\n\n Unused morph vertices positions:  Blend0: Blend1: Blend2: Indicase: Normals: Texture coordinates: Tangents(xyz):");
	for(DWORD v=0; v<numVerts[0]; v++)
	{	fread(buf, 4, 3, fIn);
		fprintf(fOut, "\nV %#05.5d: %.4f, %.4f, %4f;", v, buf[0], buf[1], buf[2]);

		BYTE norms[4];
		if(bF3UN4F16_2)
		{	fread(norms, 1, 4, fIn);
			fprintf(fOut, "%d, %d, %d, %d", norms[0], norms[1], norms[2], norms[3]);
		}
		else
		{	fread(buf, 4, 3, fIn);
			fprintf(fOut, " %.4f, %.4f, %4f;", buf[0], buf[1], buf[2]);
		}

		DWORD inds; fread(&inds, 4, 1, fIn);
		fprintf(fOut, " %#05.5x; ",	inds);

		if(bF3UN4F16_2)
		{	fread(norms, 1, 4, fIn);
			fprintf(fOut, "%d, %d, %d, %d", norms[0], norms[1], norms[2], norms[3]);
			D3DXFLOAT16 tutv[2]; fread(tutv, 2, 2, fIn);
			myD3DXFloat16To32Array(buf,tutv,2);
			fprintf(fOut, " %.4f, %.4f", buf[0], buf[1]);

			fread(norms, 1, 4, fIn);//Tangents:
			fprintf(fOut, " %d, %d, %d, %d", norms[0], norms[1], norms[2], norms[3]);
			//fread(norms, 1, 4, fIn);//Binormals:
			//fprintf(fOut, " %d, %d, %d, %d", norms[0], norms[1], norms[2], norms[3]);
		}
		else
		{	fread(buf, 4, 8, fIn);//norm tuv tn bin
			fprintf(fOut, "%4f, %4f; %.4f %.4f, %4f, %4f, %4f, %4f;", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);
	}	}

	fprintf(fOut, "\n\n Used morph vertices positions:  Blend0: Blend1: Blend2: Indicase: Normals: Texture coordinates: Tangents(xyz):");
	for(DWORD v=0; v<numVerts[1]; v++)
	{	fread(buf, 4, 3, fIn);
		fprintf(fOut, "\nV %#05.5d: %4f, %.4f, %4f;", v, buf[0], buf[1], buf[2]);

		BYTE norms[4];
		if(bF3UN4F16_2)
		{	fread(norms, 1, 4, fIn);
			fprintf(fOut, " %d, %d, %d, %d", norms[0], norms[1], norms[2], norms[3]);
		}
		else
		{	fread(buf, 4, 3, fIn);
			fprintf(fOut, " %.4f, %.4f, %4f;", buf[0], buf[1], buf[2]);
		}

		DWORD inds; fread(&inds, 4, 1, fIn);
		fprintf(fOut, " %#05.5x; ",	inds);

		if(bF3UN4F16_2)
		{	BYTE norms[4]; fread(norms, 1, 4, fIn);
			fprintf(fOut, " %d, %d, %d, %d", norms[0], norms[1], norms[2], norms[3]);
			D3DXFLOAT16 tutv[2]; fread(tutv, 2, 2, fIn);
			myD3DXFloat16To32Array(buf,tutv,2);
			fprintf(fOut, " %.4f, %.4f", buf[0], buf[1]);

			fread(norms, 1, 4, fIn);//Tangents:
			fprintf(fOut, " %d, %d, %d, %d", norms[0], norms[1], norms[2], norms[3]);
			//fread(norms, 1, 4, fIn);//Binormals:
			//fprintf(fOut, " %d, %d, %d, %d", norms[0], norms[1], norms[2], norms[3]);
		}
		else
		{	fread(buf, 4, 8, fIn);//norm tuv tn bin
			fprintf(fOut, "%4f, %4f; %.4f %.4f, %4f, %4f, %4f, %4f;", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);
	}	}

	DWORD dind[3]; WORD ind[3]; fprintf(fOut, "\n\n\n Unused morph faces, in indexes:");

	if(bStrip)
	{	if(b32bit)
		{	S32 fst=0; fread(dind, 4, 3, fIn);
			fprintf(fOut, "\nF %#05.5d: %#05.5d, %#05.5d,  %#05.5d", fst++, dind[0], dind[1], dind[2]);
			for(DWORD f=0; f<dwNumStripFaces[0]-1; f++)
			{	fread(&dind[0], 4, 1, fIn);
				if((f%2)==0)
				{	fprintf(fOut, "\nF %#05.5d: %#05.5d, %#05.5d,  %#05.5d", fst++,
									dind[1], dind[0], dind[2]);
					dind[1] = dind[0];
				}
				else
				{
					fprintf(fOut, "\nF %#05.5d: %#05.5d, %#05.5d,  %#05.5d", fst++,
									dind[2], dind[1], dind[0]); 
					dind[2] = dind[0];
		}	}	}
		else //b16bit
		{	S32 fst=0; fread(ind, 2, 3, fIn);
			fprintf(fOut, "\nF %#05.5d: %#05.5d, %#05.5d,  %#05.5d", fst++, ind[0], ind[1], ind[2]);
			for(DWORD f=0; f<dwNumStripFaces[0]-1; f++)
			{	fread(&ind[0], 2, 1, fIn);
				if((f%2)==0)
				{	fprintf(fOut, "\nF %#05.5d: %#05.5d, %#05.5d,  %#05.5d", fst++,
									ind[1], ind[0], ind[2]);
					ind[1] = ind[0];
				}
				else
				{
					fprintf(fOut, "\nF %#05.5d: %#05.5d, %#05.5d,  %#05.5d", fst++,
									ind[2], ind[1], ind[0]); 
					ind[2] = ind[0];
	}	}	}	}
	else
	{	for(DWORD f=0; f<dwNumFaces[0]; f++)
		{	if(b32bit)
			{	fread(dind, 4, 3, fIn);
				fprintf(fOut, "\nF %#05.5d: %#05.5d, %#05.5d, %#05.5d", f, dind[0], dind[1], dind[2]);
			} else
			{	fread(ind, 2, 3, fIn);
				fprintf(fOut, "\nF %#05.5d: %#05.5d, %#05.5d, %#05.5d", f, ind[0], ind[1], ind[2]);
	}	}	}

	fprintf(fOut, "\n\n Used morph faces, in indexes:");
	if(bStrip)
	{	if(b32bit)
		{	S32 fst=0; fread(dind, 4, 3, fIn);
			fprintf(fOut, "\nF %#05.5d: %#05.5d, %#05.5d,  %#05.5d", fst++, dind[0], dind[1], dind[2]);
			for(DWORD f=0; f<dwNumStripFaces[1]-1; f++)
			{	fread(&dind[0], 4, 1, fIn);
				if((f%2)==0)
				{	fprintf(fOut, "\nF %#05.5d: %#05.5d, %#05.5d,  %#05.5d", fst++,
									dind[1], dind[0], dind[2]);
					dind[1] = dind[0];
				}
				else
				{
					fprintf(fOut, "\nF %#05.5d: %#05.5d, %#05.5d,  %#05.5d", fst++,
									dind[2], dind[1], dind[0]); 
					dind[2] = dind[0];
		}	}	}
		else //b16bit
		{	S32 fst=0; fread(ind, 2, 3, fIn);
			fprintf(fOut, "\nF %#05.5d: %#05.5d, %#05.5d,  %#05.5d", fst++, ind[0], ind[1], ind[2]);
			for(DWORD f=0; f<dwNumStripFaces[1]-1; f++)
			{	fread(&ind[0], 2, 1, fIn);
				if((f%2)==0)
				{	fprintf(fOut, "\nF %#05.5d: %#05.5d, %#05.5d,  %#05.5d", fst++,
									ind[1], ind[0], ind[2]);
					ind[1] = ind[0];
				}
				else
				{
					fprintf(fOut, "\nF %#05.5d: %#05.5d, %#05.5d,  %#05.5d", fst++,
									ind[2], ind[1], ind[0]); 
					ind[2] = ind[0];
	}	}	}	}
	else
	{	for(DWORD f=0; f<dwNumFaces[1]; f++)
		{	if(b32bit)
			{	fread(dind, 4, 3, fIn);
				fprintf(fOut, "\nF %#05.5d: %#05.5d, %#05.5d, %#05.5d", f, dind[0], dind[1], dind[2]);
			} else
			{	fread(ind, 2, 3, fIn);
				fprintf(fOut, "\nF %#05.5d: %#05.5d, %#05.5d, %#05.5d", f, ind[0], ind[1], ind[2]);
	}	}	}

	DWORD dwUsedMrpChans; fread(&dwUsedMrpChans, 4, 1, fIn);
	fprintf(fOut, "\n\n Total used morpher channels: %#05.5d", dwUsedMrpChans);

	for(DWORD ch=0; ch<dwUsedMrpChans; ch++)
	{	fprintf(fOut, "\n\n Channel number: %#05.5d", ch);
		for(WORD v=0; v<numVerts[1]; v++)
		{	fread(buf, 4, 3, fIn);//V(pos)
			fprintf(fOut, "\nV %#05.5d: %.4f, %.4f, %.4f;",
					v, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
			/*if(ch < 6)
			{	fread(buf, 4, 3, fIn);//V(pos)
				fprintf(fOut, "\nV %#05.5d: %.4f, %.4f, %.4f;",
					v, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
				if(bF3UN4F16_2)
				{	//BYTE norms[4]; fread(norms, 1, 4, fIn);
					//fprintf(fOut, " %d, %d, %d, %d", norms[0], norms[1], norms[2], norms[3]);
				}
				else
				{	//fread(buf, 4, 3, fIn);
					//fprintf(fOut, "\nV %#05.5d: N: %.4f, %.4f, %.4f;",
					//	v, buf[0], buf[1], buf[2]);
			}	}
			else
			{	if(ch < MAX_SKIN2MORPH_CHANNEL-dwUsedMrpChans+1)
				{	fread(buf, 4, 3, fIn);//V(pos)
					fprintf(fOut, "\nV %#05.5d: %.4f, %.4f, %.4f;",
						v, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
					if(bF3UN4F16_2)
					{	//BYTE norms[4]; fread(norms, 1, 4, fIn);
						//fprintf(fOut, " %d, %d, %d, %d", norms[0], norms[1], norms[2], norms[3]);
					}
					else
					{	//fread(buf, 4, 3, fIn);
						//fprintf(fOut, "\nV %#05.5d: N: %.4f, %.4f, %.4f;",
						//	v, buf[0], buf[1], buf[2]);
				}	}
				else
				{	fread(buf, 4, 3, fIn);
					fprintf(fOut, "\nV %#05.5d: %.4f, %.4f, %.4f;",
							v, buf[0], buf[1], buf[2]);
	}	}*/	}	}

	fclose(fIn);
	fclose(fOut);
	return TRUE;
}

//SHSkin2morh ni tangent-binormallikka aylantiradi:
BOOL ConvertSHKin2MorphToSHSkin2MorphTangentAndBinormal(char *NAME)
{
FILE	*fIn, *fOut;
fpos_t	pos, posEnd;
char	pathAndName[128];

	lstrcpy(pathAndName, NAME);

	fIn = fopen(pathAndName, "rb");
	if(!fIn) return FALSE;
	pathAndName[lstrlen(pathAndName)-1] = '7';
	fOut = fopen(pathAndName, "wb");
	pathAndName[lstrlen(pathAndName)-1] = '6';

	char sh[20];
	fread(sh, 1, 20, fIn);
	if(!(sh[0]=='S'&&sh[1]=='k'&&sh[2]=='n'&&sh[3]=='2'&&sh[4]=='m'&&sh[5]=='r'&&sh[6]=='p'&&sh[9]=='S'&&sh[10]=='H'&&sh[11]=='I'))
	{	fclose(fIn);
		fclose(fOut);
		return FALSE;
	}
	BOOL b32bit = (sh[7] == '3' && sh[8] == '2');
	sh[11]='T';sh[12]='B';sh[13]='I';fwrite(sh, 1, 20, fOut);

	F32 buf[24];
	fread(buf, 4, 6, fIn); fwrite(buf, 4, 6, fOut); //Position, Rotation;

	DWORD dwNumFaces; fread(&dwNumFaces, 4, 1, fIn); fwrite(&dwNumFaces, 4, 1, fOut);
	DWORD dwNumFacesM;fread(&dwNumFacesM,4, 1, fIn); fwrite(&dwNumFacesM,4, 1, fOut);
	DWORD dwNumVerts; fread(&dwNumVerts, 4, 1, fIn); fwrite(&dwNumVerts, 4, 1, fOut);
	DWORD dwNumVertsM;fread(&dwNumVertsM,4, 1, fIn); fwrite(&dwNumVertsM,4, 1, fOut);
	fread(buf, 4, 6, fIn);  fwrite(buf, 4, 6, fOut);//Sizes

	WORD wNumBones; fread(&wNumBones, 2, 1, fIn); fwrite(&wNumBones, 2, 1, fOut);
	for(WORD b=0; b<wNumBones; b++)//Bone names:
	{	char n[4]; n[0] = 32;
		while(n[0]) { fread(n, 1, 1, fIn); fwrite(n, 1, 1, fOut); }
 	}

	WORD bf;
	for(WORD b=0; b<wNumBones*2; b++)//Bone faces1 & Bone faces2(*2):
	{ fread(&bf, 2, 1, fIn);  fwrite(&bf, 2, 1, fOut); }

	fread(buf, 4, 16, fIn);  fwrite(buf, 4, 16, fOut);//MeshTM
	for(WORD b=0; b<wNumBones; b++)
	{ fread(buf, 4, 16, fIn);  fwrite(buf, 4, 16, fOut); }//Bone InitTMs;


	F32 *pVertices = (F32*)malloc(dwNumVerts *12*4);//size - VB3INTVERTEX 12 edi
	F32 *pVerticesM= (F32*)malloc(dwNumVertsM*12*4);//12 edi
	fread(pVertices, 4, dwNumVerts*12,  fIn);
	fread(pVerticesM,4, dwNumVertsM*12, fIn);

	WORD *pIndicese = (WORD*)malloc(dwNumFaces *3*2);
	WORD *pIndiceseM= (WORD*)malloc(dwNumFacesM*3*2);
	fread(pIndicese, 2, dwNumFaces *3, fIn);
	fread(pIndiceseM,2, dwNumFacesM*3, fIn);

	Point3F *m_pTangents  = (Point3F*)malloc(3*4*dwNumVerts);
    //Point3F *m_pBinormals = (Point3F*)malloc(3*4*dwNumVerts);
	ZeroMemory(m_pTangents, 3*4*dwNumVerts);

	for(U32 i=0; i<dwNumFaces; i+=3)
    {	U16 a = pIndicese[3*i+0];
        U16 b = pIndicese[3*i+1];
        U16 c = pIndicese[3*i+2];

        // To find a tangent that heads in the direction of +tv(texcoords),
        // find the components of both vectors on the tangent surface ,
        // and add a linear combination of the two projections that head in the +tv direction
        m_pTangents[a] += ComputeTangentVector(&pVertices[a*12], &pVertices[b*12], &pVertices[c*12]);
        m_pTangents[b] += ComputeTangentVector(&pVertices[b*12], &pVertices[a*12], &pVertices[c*12]);
        m_pTangents[c] += ComputeTangentVector(&pVertices[c*12], &pVertices[a*12], &pVertices[b*12]);
    }

    for(U32 i=0; i<dwNumVerts; i++ )
    {	m_pTangents[i].normalize();
        //m_Cross(m_pBinormals[i], &pVertices[i*12+3], m_pTangents[i]);
    	//m_pBinormals[i].normalize();
		fwrite(&pVertices[12*i], 4, 12, fOut);
		fwrite(&m_pTangents[i], 4, 3, fOut);
		//fwrite(&m_pBinormals[i],4, 3, fOut);
    }
	free(m_pTangents);// free(m_pBinormals);



	Point3F *m_pTangentsM = (Point3F*)malloc(3*4*dwNumVertsM);
    //Point3F *m_pBinormalsM= (Point3F*)malloc(3*4*dwNumVertsM);
	ZeroMemory(m_pTangentsM, 3*4*dwNumVertsM);

	for(U32 i=0; i<dwNumFacesM; i+=3)
    {	U16 a = pIndiceseM[3*i+0];
        U16 b = pIndiceseM[3*i+1];
        U16 c = pIndiceseM[3*i+2];

        // To find a tangent that heads in the direction of +tv(texcoords),
        // find the components of both vectors on the tangent surface ,
        // and add a linear combination of the two projections that head in the +tv direction
        m_pTangentsM[a] += ComputeTangentVector(&pVerticesM[a*12], &pVerticesM[b*12], &pVerticesM[c*12]);
        m_pTangentsM[b] += ComputeTangentVector(&pVerticesM[b*12], &pVerticesM[a*12], &pVerticesM[c*12]);
        m_pTangentsM[c] += ComputeTangentVector(&pVerticesM[c*12], &pVerticesM[a*12], &pVerticesM[b*12]);
    }

    for(U32 i=0; i<dwNumVertsM; i++ )
    {	m_pTangentsM[i].normalize();
        //m_Cross(m_pBinormalsM[i], &pVerticesM[i*12+3], m_pTangentsM[i]);
    	//m_pBinormalsM[i].normalize();
		fwrite(&pVerticesM[12*i], 4, 12, fOut);
		fwrite(&m_pTangentsM[i], 4, 3, fOut);
		//fwrite(&m_pBinormalsM[i],4, 3, fOut);
    }
	free(m_pTangentsM);//free(m_pBinormalsM);

	fwrite(pIndicese , 2, dwNumFaces *3, fOut);
	fwrite(pIndiceseM, 2, dwNumFacesM*3, fOut);

	//Morph channels:
	fgetpos(fIn, &pos   ); 
	fseek(fIn, 0, SEEK_END); fgetpos(fIn, &posEnd);
	fseek(fIn, (long)pos, SEEK_SET);
	VOID *bytes = malloc((size_t)(posEnd-pos+1));
	fread(bytes , 1, (size_t)(posEnd-pos), fIn);
	fwrite(bytes, 1, (size_t)(posEnd-pos), fOut);
	free(bytes);

	free(pVertices ); free(pIndicese );
	free(pVerticesM); free(pIndiceseM);
	fclose(fIn);
	fclose(fOut);
	return TRUE;
}

BOOL ConvertSHKin2MorphToSHSkin2MorphTangentAndBinormalUseDX(char *NAME)
{
FILE	*fIn, *fOut;
fpos_t	pos, posEnd;
char	pathAndName[128];

	lstrcpy(pathAndName, NAME);

	fIn = fopen(pathAndName, "rb");
	if(!fIn) return FALSE;
	pathAndName[lstrlen(pathAndName)-1] = '7';
	fOut = fopen(pathAndName, "wb");
	pathAndName[lstrlen(pathAndName)-1] = '6';

	char sh[20];
	fread(sh, 1, 20, fIn);
	if(!(sh[0]=='S'&&sh[1]=='k'&&sh[2]=='n'&&sh[3]=='2'&&sh[4]=='m'&&sh[5]=='r'&&sh[6]=='p'&&sh[9]=='S'&&sh[10]=='H'&&sh[11]=='I'))
	{	fclose(fIn);
		fclose(fOut);
		return FALSE;
	}
	BOOL b32bit = (sh[7] == '3' && sh[8] == '2');
	sh[11]='T';sh[12]='B';sh[13]='I';
	BOOL bF3UN4F16_2 = (sh[13] == 'U');
	if(bF3UN4F16_2){fclose(fIn);fclose(fOut);return -3;}
	BOOL bStrip = (sh[14] == 'S');
	if(bStrip){fclose(fIn);fclose(fOut);return -4;}
	sh[15]='A';
	fwrite(sh, 1, 20, fOut);

	F32 buf[24];
	fread(buf, 4, 6, fIn); fwrite(buf, 4, 6, fOut); //Position, Rotation;

	DWORD dwNumFaces; fread(&dwNumFaces, 4, 1, fIn); fwrite(&dwNumFaces, 4, 1, fOut);
	DWORD dwNumFacesM;fread(&dwNumFacesM,4, 1, fIn); fwrite(&dwNumFacesM,4, 1, fOut);
	DWORD dwNumVerts; fread(&dwNumVerts, 4, 1, fIn); fwrite(&dwNumVerts, 4, 1, fOut);
	DWORD dwNumVertsM;fread(&dwNumVertsM,4, 1, fIn); fwrite(&dwNumVertsM,4, 1, fOut);
	fread(buf, 4, 6, fIn);  fwrite(buf, 4, 6, fOut);//Sizes

	WORD wNumBones; fread(&wNumBones, 2, 1, fIn); fwrite(&wNumBones, 2, 1, fOut);
	for(WORD b=0; b<wNumBones; b++)//Bone names:
	{	char n[4]; n[0] = 32;
		while(n[0]) { fread(n, 1, 1, fIn); fwrite(n, 1, 1, fOut); }
 	}

	WORD bf;
	for(WORD b=0; b<wNumBones*2; b++)//Bone faces1 & Bone faces2(*2):
	{ fread(&bf, 2, 1, fIn);  fwrite(&bf, 2, 1, fOut); }

	fread(buf, 4, 16, fIn);  fwrite(buf, 4, 16, fOut);//MeshTM
	for(WORD b=0; b<wNumBones; b++)
	{ fread(buf, 4, 16, fIn);  fwrite(buf, 4, 16, fOut); }//Bone InitTMs;

D3DVERTEXELEMENT9 dwBlendVNTTBShaderDecl[] =
{	{0, 0 , D3DDECLTYPE_FLOAT3  , D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION    , 0},
	{0, 12, D3DDECLTYPE_FLOAT3  , D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDWEIGHT , 0},
	{0, 24, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDINDICES, 0},
	{0, 28, D3DDECLTYPE_FLOAT3  , D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL      , 0},
	{0, 40, D3DDECLTYPE_FLOAT2  , D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD    , 0},
	{0, 48, D3DDECLTYPE_FLOAT3  , D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TANGENT     , 0},
	{0, 60, D3DDECLTYPE_FLOAT3  , D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BINORMAL    , 0},
	D3DDECL_END()
};
struct BlendVNTTBVERTEX32
{
    D3DXVECTOR3 p;      // Vertex position
    D3DXVECTOR3 w;      // Vertex weights
	D3DCOLOR    ind;    // Vertex inds
    D3DXVECTOR3 n;      // Vertex normals
    F32			tu, tv; // Vertex texture coordinates
	D3DXVECTOR3 t;		// Vertex tangents
	D3DXVECTOR3 b;		// Vertex binormals
};

    IDirect3D9* pD3D = myDirect3DCreate9(D3D_SDK_VERSION);
	if(!pD3D) return -4;
	D3DPRESENT_PARAMETERS d3dpp; LPDIRECT3DDEVICE9 pd3dDevice;
    ZeroMemory(&d3dpp,sizeof(d3dpp));d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
	if(FAILED(pD3D->CreateDevice(D3DADAPTER_DEFAULT,D3DDEVTYPE_REF,
									GetDesktopWindow(),D3DCREATE_SOFTWARE_VERTEXPROCESSING,
									&d3dpp,&pd3dDevice)))
        return -5;

	LPD3DXMESH pMesh = NULL;
	if(D3D_OK!=myD3DXCreateMesh(dwNumFaces+dwNumFacesM, dwNumVerts+dwNumVertsM, D3DXMESH_SYSTEMMEM,
		(LPD3DVERTEXELEMENT9*)dwBlendVNTTBShaderDecl, pd3dDevice, &pMesh))
		return FALSE;
	BlendVNTTBVERTEX32 *ptv;
	if(D3D_OK!=pMesh->LockVertexBuffer(0, (LPVOID*)&ptv))
		{ SAFE_RELEASE(pMesh); return FALSE; }
		for(U32 v=0; v<dwNumVerts; v++)
			fread(ptv++, 4, 12, fIn);
		for(U32 v=0; v<dwNumVertsM; v++)
			fread(ptv++, 4, 12, fIn);
	pMesh->UnlockVertexBuffer();

	if(b32bit)
	{	U32 *pti;
		if(D3D_OK!=pMesh->LockIndexBuffer(0, (LPVOID*)&pti))
			{ SAFE_RELEASE(pMesh); return FALSE; }
			for(U32 f=0; f<3*dwNumFaces; f++)
				fread(pti++,4,1,fIn);
			for(U32 f=0; f<3*dwNumFacesM; f++)
				fread(pti++,4,1,fIn);
	} else
	{	U16 *pti;
		if(D3D_OK!=pMesh->LockIndexBuffer(0, (LPVOID*)&pti))
			{ SAFE_RELEASE(pMesh); return FALSE; }
			for(U32 f=0; f<3*dwNumFaces; f++)
				fread(pti++,2,1,fIn);
			for(U32 f=0; f<3*dwNumFacesM; f++)
				fread(pti++,2,1,fIn);
	} pMesh->UnlockIndexBuffer();

	DWORD *rgdwAdjacency = NULL; ID3DXMesh* pNewMesh;
	rgdwAdjacency = (DWORD*)malloc(sizeof(DWORD) * (dwNumFaces+dwNumFacesM) * 3);
	if(rgdwAdjacency == NULL){SAFE_RELEASE(pMesh);return FALSE;}
	if(D3D_OK != pMesh->GenerateAdjacency(1e-6f, rgdwAdjacency)){SAFE_RELEASE(pMesh);free(rgdwAdjacency);return FALSE;}
	if(D3D_OK!=myD3DXComputeTangentFrameEx( pMesh, D3DDECLUSAGE_TEXCOORD,0,
										  D3DDECLUSAGE_TANGENT,0,
										  D3DX_DEFAULT/*D3DDECLUSAGE_BINORMAL*/,0,
                                          D3DDECLUSAGE_NORMAL, 0, 
										  D3DXTANGENT_WEIGHT_BY_AREA,
										  rgdwAdjacency, -1.01f,
                                          -0.01f, -1.01f, &pNewMesh, NULL))
		{SAFE_RELEASE(pMesh);free(rgdwAdjacency);return FALSE;}
	free(rgdwAdjacency);

	if(D3D_OK!=pNewMesh->LockVertexBuffer(0, (LPVOID*)&ptv))
		{ SAFE_RELEASE(pNewMesh); return FALSE; }
		for(U32 v=0; v<dwNumVerts; v++)
			fwrite(ptv++,sizeof(BlendVNTTBVERTEX32)-4*3,1,fOut);//binormal kerakmas;
		for(U32 v=0; v<dwNumVertsM; v++)
			fwrite(ptv++,sizeof(BlendVNTTBVERTEX32)-4*3,1,fOut);//binormal kerakmas;
	pNewMesh->UnlockVertexBuffer();

	DWORD *pIndicase = (DWORD*)malloc((dwNumFaces+dwNumFacesM)*3*4);
	if(b32bit)
	{	U32 *pti;
		if(D3D_OK!=pMesh->LockIndexBuffer(0, (LPVOID*)&pti))
			{ SAFE_RELEASE(pMesh); return FALSE; }
			for(U32 f=0; f<3*dwNumFaces; f++)
			{	fwrite(pti,4,1,fOut);
				pIndicase[f] = *pti++;
	   		}
			for(U32 f=0; f<3*dwNumFacesM; f++)
			{	fwrite(pti,4,1,fOut);
				pIndicase[f] = *pti++;
	}   	} else
	{	U16 *pti;
		if(D3D_OK!=pMesh->LockIndexBuffer(0, (LPVOID*)&pti))
			{ SAFE_RELEASE(pMesh); return FALSE; }
			for(U32 f=0; f<3*dwNumFaces; f++)
			{	fwrite(pti,2,1,fOut);
				pIndicase[f] = *pti++;
	        }
			for(U32 f=0; f<3*dwNumFacesM; f++)
			{	fwrite(pti,2,1,fOut);
				pIndicase[f] = *pti++;
	}       }pMesh->UnlockIndexBuffer();
	SAFE_RELEASE(pMesh);

	//Morph channels:
	fgetpos(fIn, &pos   ); 
	fseek(fIn, 0, SEEK_END); fgetpos(fIn, &posEnd);
	fseek(fIn, (long)pos, SEEK_SET);
	VOID *bytes = malloc((size_t)(posEnd-pos+1));
	fread(bytes , 1, (size_t)(posEnd-pos), fIn);
	fwrite(bytes, 1, (size_t)(posEnd-pos), fOut);
	free(bytes);

	fclose(fIn); fclose(fOut);
	pd3dDevice->Release();pD3D->Release();

	return TRUE;
}

int ConvertIndexedShaderSkin2MorphToF3_UB4N_F16_2Format(char *NAME)
{
FILE	*fIn, *fOut;
char	pathAndName[128];

	lstrcpy(pathAndName, NAME);

	fIn = fopen(pathAndName, "rb");
	if(!fIn) return -1;
	pathAndName[lstrlen(pathAndName)-1] = 'u';
	fOut = fopen(pathAndName, "wb");
	pathAndName[lstrlen(pathAndName)-1] = 'i';

	//Shapkasi, for future using:
	char sh[20];
	fread(sh, 1, 20, fIn);
	if(!(sh[0]=='S'&&sh[1]=='k'&&sh[2]=='n'&&sh[3]=='2'&&sh[4]=='m'&&sh[5]=='r'&&sh[6]=='p'&&sh[9]=='S'&&sh[10]=='H'&&sh[11]=='I'))
	{	fclose(fIn);
		fclose(fOut);
		return -2;
	}
	BOOL b32bit = (sh[7]=='3' && sh[8]=='2');
	sh[12]='U';
	fwrite(sh, 1, 20, fOut);
//************************************************************************************
	float buf[24];
	fread(buf, 4, 6, fIn); fwrite(buf, 4, 6, fOut); //Position, Rotation;

	DWORD dwNumFaces[2]; fread(dwNumFaces, 4, 2, fIn); fwrite(dwNumFaces, 4, 2, fOut);
	DWORD numVerts[2]; fread(numVerts, 4, 2, fIn);fwrite(numVerts, 4, 2, fOut);

	fread(buf, 4, 6, fIn);	fwrite(buf, 4, 6, fOut);

	WORD wNumBones;	fread(&wNumBones, 2, 1, fIn); fwrite(&wNumBones, 2, 1, fOut);
	for(WORD b=0; b<wNumBones; b++)
	{	char n[4]; n[0] = 32;
		while(n[0]) { fread(n, 1, 1, fIn); fwrite(n, 1, 1, fOut); }
	}

	for(WORD b=0; b<wNumBones; b++)//Unused morph faces for each bones:");
	{ WORD nf; fread(&nf, 2, 1, fIn); fwrite(&nf, 2, 1, fOut); }

	for(WORD b=0; b<wNumBones; b++)//Used morph faces for each bones:");
	{ WORD nf; fread(&nf, 2, 1, fIn); fwrite(&nf, 2, 1, fOut); }

	fread(buf, 4, 16, fIn);	fwrite(buf, 4, 16, fOut);//MeshTM

	for(WORD b=0; b<wNumBones; b++)//Bones InitTMs
	{	fread(buf, 4, 16, fIn);
		fwrite(buf, 4, 16, fOut);
	}

	//Unused morph vertices positions:  Blend0: Blend1: Blend2: Indicase: Normals: Texture coordinates:");
	for(DWORD v=0; v<numVerts[0]; v++)
	{	fread(buf, 4, 6, fIn); fwrite(buf, 4, 3, fOut);

		BYTE norms[4];
		buf[3]*=255.0f;buf[4]*=255.0;buf[5]*=255.0f;
		norms[0] = (BYTE)buf[3]; norms[1] = (BYTE)buf[4]; norms[2] = (BYTE)buf[5];
		norms[3]=255-norms[0]-norms[1]-norms[2];
		fwrite(norms, 1, 4, fOut);//Blend;

		DWORD inds; fread(&inds, 4, 1, fIn); fwrite(&inds, 4, 1, fOut);
		
		D3DXFLOAT16  uv16[2];
		fread(buf, 4, 5, fIn);
		norms[3]=255;
		buf[0]=(1.0f+buf[0])*127.5f;buf[1]=(1.0f+buf[1])*127.5;buf[2]=(1.0f+buf[2])*127.5f;
		norms[0] = (BYTE)buf[0]; norms[1] = (BYTE)buf[1]; norms[2] = (BYTE)buf[2];
		fwrite(norms, 1, 4, fOut);//norms;
		myD3DXFloat32To16Array(uv16,&buf[3],2);
		fwrite(uv16, 2, 2, fOut);//uvs;
	}

	//Used morph vertices positions:  Blend0: Blend1: Blend2: Indicase: Normals: Texture coordinates:");
	for(DWORD v=0; v<numVerts[1]; v++)
	{	fread(buf, 4, 6, fIn); fwrite(buf, 4, 3, fOut);

		BYTE norms[4];
		buf[3]*=255.0f;buf[4]*=255.0;buf[5]*=255.0f;
		norms[0] = (BYTE)buf[3]; norms[1] = (BYTE)buf[4]; norms[2] = (BYTE)buf[5];
		norms[3]=255-norms[0]-norms[1]-norms[2];
		fwrite(norms, 1, 4, fOut);//norms;

		DWORD inds; fread(&inds, 4, 1, fIn); fwrite(&inds, 4, 1, fOut);

		D3DXFLOAT16  uv16[2];
		fread(buf, 4, 5, fIn);
		norms[3]=255;
		buf[0]=(1.0f+buf[0])*127.5f;buf[1]=(1.0f+buf[1])*127.5;buf[2]=(1.0f+buf[2])*127.5f;
		norms[0] = (BYTE)buf[0]; norms[1] = (BYTE)buf[1]; norms[2] = (BYTE)buf[2];
		fwrite(norms, 1, 4, fOut);//norms;
		myD3DXFloat32To16Array(uv16,&buf[3],2);
		fwrite(uv16, 2, 2, fOut);//uvs;
	}

	WORD ind[3]; //Unused morph faces, in indexes:");
	for(DWORD f=0; f<dwNumFaces[0]; f++)
	{	fread(ind, 2, 3, fIn);
		fwrite(ind, 2, 3, fOut);
	}

	//Used morph faces, in indexes:");
	for(DWORD f=0; f<dwNumFaces[1]; f++)
	{	fread(ind, 2, 3, fIn);
		fwrite(ind, 2, 3, fOut);
	}

	DWORD dwUsedMrpChans; fread(&dwUsedMrpChans, 4, 1, fIn);
						  fwrite(&dwUsedMrpChans, 4, 1, fOut);


	for(DWORD ch=0; ch<dwUsedMrpChans; ch++)
	{	for(WORD v=0; v<numVerts[1]; v++)
		{	D3DXFLOAT16  uv16[2];BYTE norms[4]={0,0,0,255};
			fread(buf, 4, 3, fIn); fwrite(buf, 4, 3, fOut);
			/*if(ch < 6)
			{	fread(buf, 4, 6, fIn); fwrite(buf, 4, 3, fOut);
				buf[3]=(1.0f+buf[3])*127.5f;buf[4]=(1.0f+buf[4])*127.5;buf[5]=(1.0f+buf[5])*127.5f;
				norms[0] = (BYTE)buf[3]; norms[1] = (BYTE)buf[4]; norms[2] = (BYTE)buf[5];
				fwrite(norms, 1, 4, fOut);
			} else
			{	if(ch < MAX_SKIN2MORPH_CHANNEL-dwUsedMrpChans+1)
				{	fread(buf, 4, 6, fIn); fwrite(buf, 4, 3, fOut);
					buf[3]=(1.0f+buf[3])*127.5f;buf[4]=(1.0f+buf[4])*127.5;buf[5]=(1.0f+buf[5])*127.5f;
					norms[0] = (BYTE)buf[3]; norms[1] = (BYTE)buf[4]; norms[2] = (BYTE)buf[5];
					fwrite(norms, 1, 4, fOut);
				}
				else
				{	fread(buf, 4, 3, fIn);
					fwrite(buf, 4, 3, fOut);
	}	}*/	}	}

	fclose(fIn);
	fclose(fOut);
	return TRUE;
}

int ConvertIndexedShaderSkin2MorphTBToF3_UB4N_F16_2Format(char *NAME)
{
FILE	*fIn, *fOut;
char	pathAndName[128];

	lstrcpy(pathAndName, NAME);

	fIn = fopen(pathAndName, "rb");
	if(!fIn) return -1;
	pathAndName[lstrlen(pathAndName)-1] = 'u';
	fOut = fopen(pathAndName, "wb");
	pathAndName[lstrlen(pathAndName)-1] = '7';

	//Shapkasi, for future using:
	char sh[20];
	fread(sh, 1, 20, fIn);
	if(!(sh[0]=='S'&&sh[1]=='k'&&sh[2]=='n'&&sh[3]=='2'&&sh[4]=='m'&&sh[5]=='r'&&sh[6]=='p'&&sh[9]=='S'&&sh[10]=='H'&&sh[11]=='T'&&sh[12]=='B'&&sh[13]=='I'))
	{	fclose(fIn);
		fclose(fOut);
		return -2;
	}
	BOOL b32bit = (sh[3]=='3' && sh[4]=='2');
	sh[13]='U';
	fwrite(sh, 1, 20, fOut);
//************************************************************************************
	float buf[24];
	fread(buf, 4, 6, fIn); fwrite(buf, 4, 6, fOut); //Position, Rotation;

	DWORD dwNumFaces[2]; fread(dwNumFaces, 4, 2, fIn); fwrite(dwNumFaces, 4, 2, fOut);
	DWORD numVerts[2]; fread(numVerts, 4, 2, fIn);fwrite(numVerts, 4, 2, fOut);

	fread(buf, 4, 6, fIn);	fwrite(buf, 4, 6, fOut);

	WORD wNumBones;	fread(&wNumBones, 2, 1, fIn); fwrite(&wNumBones, 2, 1, fOut);
	for(WORD b=0; b<wNumBones; b++)
	{	char n[4]; n[0] = 32;
		while(n[0]) { fread(n, 1, 1, fIn); fwrite(n, 1, 1, fOut); }
	}

	for(WORD b=0; b<wNumBones; b++)//Unused morph faces for each bones:");
	{ WORD nf; fread(&nf, 2, 1, fIn); fwrite(&nf, 2, 1, fOut); }

	for(WORD b=0; b<wNumBones; b++)//Used morph faces for each bones:");
	{ WORD nf; fread(&nf, 2, 1, fIn); fwrite(&nf, 2, 1, fOut); }

	fread(buf, 4, 16, fIn);	fwrite(buf, 4, 16, fOut);//MeshTM

	for(WORD b=0; b<wNumBones; b++)//Bones InitTMs
	{	fread(buf, 4, 16, fIn);
		fwrite(buf, 4, 16, fOut);
	}

	//Unused morph vertices positions:  Blend0: Blend1: Blend2: Indicase: Normals: Texture coordinates:");
	for(DWORD v=0; v<numVerts[0]; v++)
	{	fread(buf, 4, 6, fIn); fwrite(buf, 4, 3, fOut);

		BYTE norms[4];
		buf[3]*=255.0f;buf[4]*=255.0;buf[5]*=255.0f;
		norms[0] = (BYTE)buf[3]; norms[1] = (BYTE)buf[4]; norms[2] = (BYTE)buf[5];
		norms[3]=255-norms[0]-norms[1]-norms[2];
		fwrite(norms, 1, 4, fOut);//blends;

		DWORD inds; fread(&inds, 4, 1, fIn); fwrite(&inds, 4, 1, fOut);
		
		D3DXFLOAT16  uv16[2];
		fread(buf, 4, 8, fIn);
		norms[3]=255;
		buf[0]=(1.0f+buf[0])*127.5f;buf[1]=(1.0f+buf[1])*127.5;buf[2]=(1.0f+buf[2])*127.5f;
		norms[0] = (BYTE)buf[0]; norms[1] = (BYTE)buf[1]; norms[2] = (BYTE)buf[2];
		fwrite(norms, 1, 4, fOut);//norms;
		myD3DXFloat32To16Array(uv16,&buf[3],2);
		fwrite(uv16, 2, 2, fOut);//uvs;

		buf[5]=(1.0f+buf[5])*127.5f;buf[6]=(1.0f+buf[6])*127.5;buf[7]=(1.0f+buf[7])*127.5f;
		norms[0] = (BYTE)buf[5]; norms[1] = (BYTE)buf[6]; norms[2] = (BYTE)buf[7];
		fwrite(norms, 1, 4, fOut);//tangents;

		//buf[8]=(1.0f+buf[8])*127.5f;buf[9]=(1.0f+buf[9])*127.5;buf[10]=(1.0f+buf[10])*127.5f;
		//norms[0] = (BYTE)buf[8]; norms[1] = (BYTE)buf[9]; norms[2] = (BYTE)buf[10];
		//fwrite(norms, 1, 4, fOut);//binormals;
	}

	//Used morph vertices positions:  Blend0: Blend1: Blend2: Indicase: Normals: Texture coordinates:");
	for(DWORD v=0; v<numVerts[1]; v++)
	{	fread(buf, 4, 6, fIn); fwrite(buf, 4, 3, fOut);

		BYTE norms[4];
		buf[3]*=255.0f;buf[4]*=255.0;buf[5]*=255.0f;
		norms[0] = (BYTE)buf[3]; norms[1] = (BYTE)buf[4]; norms[2] = (BYTE)buf[5];
		norms[3]=255-norms[0]-norms[1]-norms[2];
		fwrite(norms, 1, 4, fOut);//norms;

		DWORD inds; fread(&inds, 4, 1, fIn); fwrite(&inds, 4, 1, fOut);

		D3DXFLOAT16  uv16[2];
		fread(buf, 4, 8, fIn);
		norms[3]=255;
		buf[0]=(1.0f+buf[0])*127.5f;buf[1]=(1.0f+buf[1])*127.5;buf[2]=(1.0f+buf[2])*127.5f;
		norms[0] = (BYTE)buf[0]; norms[1] = (BYTE)buf[1]; norms[2] = (BYTE)buf[2];
		fwrite(norms, 1, 4, fOut);//norms;
		myD3DXFloat32To16Array(uv16,&buf[3],2);
		fwrite(uv16, 2, 2, fOut);//uvs;

		buf[5]=(1.0f+buf[5])*127.5f;buf[6]=(1.0f+buf[6])*127.5;buf[7]=(1.0f+buf[7])*127.5f;
		norms[0] = (BYTE)buf[5]; norms[1] = (BYTE)buf[6]; norms[2] = (BYTE)buf[7];
		fwrite(norms, 1, 4, fOut);//tangents;

		//buf[8]=(1.0f+buf[8])*127.5f;buf[9]=(1.0f+buf[9])*127.5;buf[10]=(1.0f+buf[10])*127.5f;
		//norms[0] = (BYTE)buf[8]; norms[1] = (BYTE)buf[9]; norms[2] = (BYTE)buf[10];
		//fwrite(norms, 1, 4, fOut);//binormals;
	}

	WORD ind[3]; //Unused morph faces, in indexes:");
	for(DWORD f=0; f<dwNumFaces[0]; f++)
	{	fread(ind, 2, 3, fIn);
		fwrite(ind, 2, 3, fOut);
	}

	//Used morph faces, in indexes:");
	for(DWORD f=0; f<dwNumFaces[1]; f++)
	{	fread(ind, 2, 3, fIn);
		fwrite(ind, 2, 3, fOut);
	}

	DWORD dwUsedMrpChans; fread(&dwUsedMrpChans, 4, 1, fIn);
	if(dwUsedMrpChans > 9) dwUsedMrpChans = 9;
						  fwrite(&dwUsedMrpChans, 4, 1, fOut);

	//16.03.2011 y. dan boshlab 9 kanallik TBSkn2Mrp, normallari yo'q:
	for(DWORD ch=0; ch<dwUsedMrpChans; ch++)
	{	for(WORD v=0; v<numVerts[1]; v++)
		{	D3DXFLOAT16  uv16[2];BYTE norms[4]={0,0,0,255};
			fread(buf, 4, 3, fIn); fwrite(buf, 4, 3, fOut);
			/*if(ch < 6)
			{	fread(buf, 4, 6, fIn); fwrite(buf, 4, 3, fOut);
				buf[3]=(1.0f+buf[3])*127.5f;buf[4]=(1.0f+buf[4])*127.5;buf[5]=(1.0f+buf[5])*127.5f;
				norms[0] = (BYTE)buf[3]; norms[1] = (BYTE)buf[4]; norms[2] = (BYTE)buf[5];
				fwrite(norms, 1, 4, fOut);
			} else
			{	if(ch < MAX_SKIN2MORPH_CHANNEL-dwUsedMrpChans+1)
				{	fread(buf, 4, 6, fIn); fwrite(buf, 4, 3, fOut);
					buf[3]=(1.0f+buf[3])*127.5f;buf[4]=(1.0f+buf[4])*127.5;buf[5]=(1.0f+buf[5])*127.5f;
					norms[0] = (BYTE)buf[3]; norms[1] = (BYTE)buf[4]; norms[2] = (BYTE)buf[5];
					fwrite(norms, 1, 4, fOut);
				}
				else
				{	fread(buf, 4, 3, fIn);
					fwrite(buf, 4, 3, fOut);
	}	}*/	}	}

	fclose(fIn);
	fclose(fOut);
	return TRUE;
}

BOOL RenameFile(char *name, S32 posInName, char c)
{
char newName[128];
	char oldChar = name[posInName];
	lstrcpy(newName, name);
	newName[posInName] = c;
	if(!CopyFile(name, newName, FALSE))
		return FALSE;
	newName[posInName] = oldChar;
	return DeleteFile(newName);
}

#pragma warning(default : 4244)
#pragma warning(default : 4996)
#undef NEW
#undef RENEW
#undef DEL
