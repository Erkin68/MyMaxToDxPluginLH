#ifndef __ExpDx__H
#define __ExpDx__H

#include "Max.h"
#include "resource.h"
#include "istdplug.h"
#include "iparamb2.h"
#include "iparamm2.h"
#include "decomp.h"
#include "iskin.h"
#include "iunwrap.h"
#include "d3dx9.h"
#include "../../../Game SDK/Main/typesWin32.h"
#include "utilapi.h"
#include "C:/Program Files (x86)/Autodesk/3ds Max 2010 SDK/maxsdk/samples/modifiers/morpher/wm3.h"


typedef struct _MYDATA
{	char	szExport    [128];
	char	szImport    [128];
	char	szReadToText[128];
	char	szWriteText [128];
	char	szGetFromDlg[128];
	char	vrtSelFile  [128];
	float   fMassh;
	float   fThreshold;
	float	dete;
	float	interpKoef;
	BOOL    bToText;
	BOOL    bInIndexed;
	BOOL    bTangent;
	BOOL    b32bit;
	BOOL    bDetachSknMesh;
	BOOL    bDeleteSource;
	BOOL	bFlipNormals;
	BOOL	bClampTV;
	BOOL	bWriteSelection;
} MYDATA, FAR * LPMYDATA;

typedef struct _VERTS_T2F_N3F_V3F
{	float  tv;
	float  tu;
	Point3 n;
	Point3 p;
} VERTS_T2F_N3F_V3F;

typedef struct _VERTS_T2F_N3F_V3F_CDW
{	float tv;
	float tu;
	Point3 n;
	Point3 p;
	DWORD color;
} VERTS_T2F_N3F_V3F_CDW;

typedef struct _weightTableForSkVrtx
{	int   whichBone;
	float weightForBone;
}weightTableForSkVrtx;

typedef struct _VERTS_SKIN_T2F_N3F_V3F
{	float tv;
	float tu;
	Point3 n;
	Point3 p;
	int   numAssigBones;	
	weightTableForSkVrtx *pWeightTable;
} VERTS_SKIN_T2F_N3F_V3F;

typedef struct _FACES_NORMAL_AND_SMOOTH_GROUPE
{	Point3 n;
	DWORD smGr;
} FACES_NORMAL_AND_SMOOTH_GROUPE;

typedef struct ThreadParameter
{	Interface	*ip;
	IUtil		*iu;
	int			type;
} ThrPar;// funksiyalarni chaqirish parameterlari

class MyMtrl
{
public:
	MyMtrl();
	Point4 vMatAmb;
	Point4 vMatDif;
	Point4 vMatSpe;
	Point4 vMatEmi;
	float  fMatPow;
	float  fMatk_r;
	void Nill();
	void Read(FILE*);
	void Write(FILE*);
	void Get(INode*);
	void Set(INode*);
};
extern MyMtrl myMtrl;




#define MAXSTREAM 4096
#define RAD     57.2957795130823208767f
#define IKKIPI  6.28318530717958647693f


extern HINSTANCE	hInstance;
extern MYDATA		sMyData;		
extern INode		*node;
extern TCHAR		*nodeName;
extern HANDLE		hf;
extern AffineParts	ap;
extern Mesh			*mesh;
extern char			fileName[256];
extern int			vx[3],flatMapChan;
extern float		eul[3];
extern double		widthX,heightY,FromX,FromY,ToX,ToY,DELTA;
extern BOOL			needDel,bFlatten;
extern DWORD		numVerts,numFaces,ind[3],t_ind[3],
					dwNumVerts,NumNewVert, alreadyReaded;
extern FILE			*streamBin;
extern VERTS_T2F_N3F_V3F
					*old_tv1_verts, *old_tv2_verts;
extern VERTS_T2F_N3F_V3F_CDW 
					*old_tcv1_verts;
extern FACES_NORMAL_AND_SMOOTH_GROUPE	
					*old_faces;
extern IUnwrapMod	*FlattenMod;
extern UINT			cdmsgShareViolation,cdmsgFileOK,cdmsgHelp;
extern int			numBones,*pBonesParentTable,*pBonesHierarchTable;
extern BOOL			*keyArrays; 
extern INode		**pBonNodes;                //bo'lsa, bone raqami, aks holda -1 ga teng; 
extern WORD			wNumBones;//footstep ni hisobga olganda
extern TCHAR		*boneName;
extern Point3		scPv;
extern FACES_NORMAL_AND_SMOOTH_GROUPE	
					*old_faces;
extern ISkin		*skin; 
extern ISkinContextData *skincontext; 
extern int			numBones,*pBonesParentTable,*pBonesHierarchTable,
					*VerOldTabl;//agar 1 ta to'liq weight 
extern BOOL			*keyArrays; 
extern INode		**pBonNodes;                //bo'lsa, bone raqami, aks holda -1 ga teng; 
extern Modifier		*SkinModif;
extern MorphR3		*morpher;
extern WORD			wNumBones;//footstep ni hisobga olganda
extern TCHAR		*boneName;
extern Control		*c;
extern Animatable	*vertAn,*horAn,*rotAn;
extern VERTS_SKIN_T2F_N3F_V3F  *sk_verts;
extern DWORD		dwBandChan; 				
extern size_t		mrphSz;
extern float		*chanPrs;
extern int			errStep;


extern VOID				AllDiffuseMesh(LPVOID);
extern VOID				AllMesh(LPVOID);
extern VOID				AllPhysXMesh(LPVOID);
extern VOID				AllMeshMatrix(LPVOID);
extern VOID				AllMeshTextCoord(LPVOID);
extern VOID				AllMorph(LPVOID);
extern VOID				AllMorphAnim(LPVOID);
extern VOID				AllShapes(LPVOID);
extern VOID				AllSkin(LPVOID);
extern VOID				AllSkin2Morph(LPVOID);
extern VOID				AllSkinAni(LPVOID);
extern VOID				AllZet(LPVOID);
extern VOID				BinFromText();
extern int				BuildBonesHierarchy(int, BOOL*, int*);
extern BOOL CALLBACK	ComDlg32DlgProc(HWND, UINT, WPARAM, LPARAM);
extern Matrix3			ConvToMatrix3(D3DXMATRIX*);
extern D3DXMATRIX		ConvToD3DXMATRIX(Matrix3*);
//extern VOID			ExportOneSkinKeys (Interface*,BOOL);
//extern VOID			ExportOneSkinQuats(Interface*,BOOL);
extern int				ExpSelVertsFrIndsTxtFile(LPVOID,BOOL,BOOL);
extern BOOL				FindFlatten();
extern WORD				FindVert_T2F_N3F_V3F(VERTS_T2F_N3F_V3F*);
extern D3DXMATRIX		FlipYZAxisInMatrix(Matrix3&);
extern D3DXMATRIX		FlipYZAxisInD3DXMATRIX(D3DXMATRIX&);
extern D3DXQUATERNION 	FlipYZAxisInQuat(Quat&);
//extern VOID			FlipYZAxisInEuler(float*);
extern DWORD WINAPI		fn(LPVOID);
extern int				GetBonPosInNewHierarchTable(int);
extern Point3			GetFlippedRot(Matrix3*);
extern Point3			GetMatrixScale(Matrix3*);
extern bool				GetMorpherModifier(INode*);
extern int				GetParentCounts(INode*);
extern int				GetSelID(Interface*);
extern VOID				GetSelectedInfo(LPVOID);
extern bool				GetSkinModifier(INode*);
extern BOOL				GetVertexNormal(Mesh*, int, RVertex*, Point3*);
extern Matrix3			GetInvTransform(Point3&, Quat&);
extern TriObject		*GetTriObjectFromNode(INode*, TimeValue, int*);
extern TCHAR			*GetString(int);
extern VOID				ImportDiffuseMesh(Interface*);
extern VOID				ImportMesh(Interface*);
extern VOID				ImportSkin(Interface*);
extern VOID				ImportTngntMesh(Interface*);
extern VOID				InverseInDX(Matrix3*);
extern BOOL				IsAncestorSelected(INode*);
extern VOID				Progr(int, int, float, const char*, Interface*);
extern LRESULT CALLBACK	ProgressWinProc(HWND, UINT, WPARAM, LPARAM);
extern VOID				ReadToText();
extern VOID				RebuildMatrix(Matrix3&);
extern VOID				RebuildMakeClosestQuat(Matrix3&, Matrix3*);
extern VOID				RestoreAllChannels(Interface*);
extern VOID				SaveAllChannels(Interface*);
extern int				SaveVertexSelectionToFile(LPVOID);
extern VOID				SmoothNormalsWithWeightingByFaceAngle();
extern VOID				SmoothNormalsWithWeightingByFaceAngle2();
extern VOID				SmoothNormalsWithWeightingByFaceAngleTCV();
extern BOOL				TMNegParity(Matrix3&);
extern BOOL				openfile(char*, char*, char*, FILE**, char*);
extern BOOL				openfile(char*, FILE**, char*);
extern VOID				ZeroAllChannels(Interface*);
extern void				ClampTU_TV(float*);

typedef D3DXMATRIX* (__stdcall *D3DXMatrixInverse_t)(D3DXMATRIX*,FLOAT*,CONST D3DXMATRIX*);
typedef HRESULT (__stdcall *D3DXMatrixDecompose_t)(D3DXVECTOR3*,D3DXQUATERNION*,D3DXVECTOR3*,CONST D3DXMATRIX*);
typedef FLOAT* (__stdcall *D3DXFloat16To32Array_t)(FLOAT*,CONST D3DXFLOAT16*,UINT);
typedef D3DXFLOAT16* (__stdcall *D3DXFloat32To16Array_t)(D3DXFLOAT16*,CONST FLOAT*,UINT);
typedef HRESULT (__stdcall *D3DXComputeNormals_t)(LPD3DXBASEMESH,CONST DWORD*);
//typedef HRESULT (__stdcall *D3DXComputeTangent_t)(LPD3DXMESH,DWORD,DWORD,DWORD,DWORD,CONST DWORD*);
typedef HRESULT (__stdcall *D3DXComputeTangentFrameEx_t)(ID3DXMesh*,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,CONST DWORD*,FLOAT,FLOAT,FLOAT,ID3DXMesh**,ID3DXBuffer**);
typedef HRESULT (__stdcall *D3DXCreateMesh_t)(DWORD,DWORD,DWORD,CONST LPD3DVERTEXELEMENT9*,LPDIRECT3DDEVICE9,LPD3DXMESH*);
typedef IDirect3D9* (__stdcall *Direct3DCreate9_t)(UINT);
typedef HRESULT (__stdcall *D3DXConvertMeshSubsetToSingleStrip_t)(LPD3DXBASEMESH,DWORD,DWORD,LPDIRECT3DINDEXBUFFER9*,DWORD*);
typedef HRESULT (__stdcall *D3DXConvertMeshSubsetToStrips_t)(LPD3DXBASEMESH,DWORD,DWORD,LPDIRECT3DINDEXBUFFER9*,DWORD*,LPD3DXBUFFER*,DWORD*);
typedef D3DXVECTOR4* (__stdcall *D3DXVec3Transform_t)(D3DXVECTOR4*,CONST D3DXVECTOR3*,CONST D3DXMATRIX*);

extern D3DXMatrixInverse_t myD3DXMatrixInverse;
extern D3DXMatrixDecompose_t myD3DXMatrixDecompose;
extern D3DXFloat16To32Array_t myD3DXFloat16To32Array;
extern D3DXFloat32To16Array_t myD3DXFloat32To16Array;
extern D3DXComputeNormals_t myD3DXComputeNormals;
//extern D3DXComputeTangent_t myD3DXComputeTangent;
extern D3DXComputeTangentFrameEx_t myD3DXComputeTangentFrameEx;
extern D3DXCreateMesh_t myD3DXCreateMesh;
extern Direct3DCreate9_t myDirect3DCreate9;
extern D3DXConvertMeshSubsetToSingleStrip_t myD3DXConvertMeshSubsetToSingleStrip;
extern D3DXConvertMeshSubsetToStrips_t myD3DXConvertMeshSubsetToStrips;
extern D3DXVec3Transform_t myD3DXVec3Transform;


inline BOOL IsErrFloatIndValue(F32& fl)
{
	return ( (*(DWORD*)(&fl)) == 0xffc00000 ) ? TRUE : FALSE;
}

inline BOOL IsErrFloatQNanValue(F32& fl)
{
	return ( (*(DWORD*)(&fl)) == 0xffc00000 ) ? TRUE : FALSE;
}

#endif // __ExpDx__H
