#include "ExpDX.h"
#include <shape.h> 
#include "../../../Libs/Math/mMathFn.h"



static VOID ExportOneShape(Interface *ip,IUtil *iu,int sel_num)
{
BOOL	bClone;
DWORD	p, polys;
float				XFMIN = 3.402823466e+38F, XFMAX = -3.402823466e+38F, 
					YFMIN = 3.402823466e+38F, YFMAX = -3.402823466e+38F, 
					ZFMIN = 3.402823466e+38F, ZFMAX = -3.402823466e+38F; 
#define calcMaxMin	if(XFMIN > mkx) XFMIN = mkx;\
					if(XFMAX < mkx) XFMAX = mkx;\
					if(YFMIN > mky) YFMIN = mky;\
					if(YFMAX < mky) YFMAX = mky;\
					if(ZFMIN > mkz) ZFMIN = mkz;\
					if(ZFMAX < mkz) ZFMAX = mkz;

	node = ip->GetSelNode(sel_num);//node ni olamiz
	nodeName = node->GetName();//node ni nomini olamiz
//********* Agar CLone bo'lsa ***********************************************
	if(strstr(nodeName, "Clone")) bClone = TRUE;
	else bClone = FALSE;
//***************************************************************************
    Matrix3 TM = node->GetNodeTM(ip->GetTime()) * Inverse(node->GetParentTM(ip->GetTime()));
	decomp_affine(TM, &ap);
	BOOL negScale = TMNegParity(node->GetObjTMAfterWSM(ip->GetTime()));

//*******  Shape qilib ko'ramiz **********************************************
	Object *obj = node->EvalWorldState(ip->GetTime()).obj;
	ShapeObject *so;

	if(obj->SuperClassID() == SHAPE_CLASS_ID) 
		so = (ShapeObject *)obj; 
	else 
	{
		MessageBox(NULL,"Obyektni editmesh qilolmayman!","Diqqat",MB_OK);
		return;
	}

	BezierShape shape;

	if(so->CanMakeBezier()) 
		so->MakeBezier(ip->GetTime(), shape);
	else
	{	MessageBox(NULL,"Obyektni Besierga aylantira olmayapman!","Diqqat",MB_OK);
		return;
	}
//*********************  Fileni ochamiz **************************************

	if(!openfile(sMyData.szExport, nodeName, bClone ? ".clshp" : ".shp", &streamBin  ,"wb"))
	{	char fullname[128]; sprintf(fullname, "%s\\%s%s", sMyData.szExport, nodeName, bClone ? ".clshp" : ".shp");
		MessageBox(NULL, fullname, "Faylini ochishda xato.", MB_OK);
		return;
	}

	// if trans to be own matrix *********************************************
	float mposx,mposy,mposz;
	mposx = ap.t.x * sMyData.fMassh;
	mposy = ap.t.y * sMyData.fMassh;
	mposz = ap.t.z * sMyData.fMassh;
	fwrite( &mposx   , sizeof( mposx )   , 1, streamBin );
	fwrite( &mposz   , sizeof( mposz )   , 1, streamBin );
	fwrite( &mposy   , sizeof( mposy )   , 1, streamBin );
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
	Point3 fRot = GetFlippedRot(&TM);
	//Programmaning o'zida almashtiramiz:
	//agar FlipAxisInRot(fRot); ni ishlatsang, tepadagi minuslarni olib tashla!!!!
	fwrite( &fRot.x, sizeof( fRot.x ), 1, streamBin );
	fwrite( &fRot.y, sizeof( fRot.y ), 1, streamBin );
	fwrite( &fRot.z, sizeof( fRot.z ), 1, streamBin );
	//agar clone bo'lsa:
	if(bClone) goto Tamom;
//***************   FACE lar sonini yozamiz **********************************
	polys = shape.SplineCount();

	fwrite( &polys, sizeof(DWORD), 1, streamBin    );

	for (p=0; p<polys; p++) 
	{   Spline3D *spline = shape.GetSpline(p);
		WORD	knCount = spline->KnotCount();//alohida elementlar soni
		if(spline->Closed())// agar yopiq bo'lsa oxiriga boshini taqab qo'yamiz
		{   knCount ++;
			fwrite(&knCount, sizeof( knCount ), 1, streamBin);
			knCount --;
		}
		else fwrite(&knCount, sizeof( knCount ), 1, streamBin);

		for (int i=0; i<knCount; i++) 
		{   Point3 k = spline->GetKnotPoint(i);
			k = k * TM;
			float mkx,mky,mkz;
			mkx = k.x * sMyData.fMassh;
			mky = k.y * sMyData.fMassh;
			mkz = k.z * sMyData.fMassh;

			calcMaxMin

			fwrite(&mkx, sizeof( float ), 1, streamBin);
			fwrite(&mkz, sizeof( float ), 1, streamBin);
			fwrite(&mky, sizeof( float ), 1, streamBin);
		}
		if(spline->Closed())
		{   Point3 k = spline->GetKnotPoint(0);
			k = k * TM;
			float mkx,mky,mkz;
			mkx = k.x * sMyData.fMassh;
			mky = k.y * sMyData.fMassh;
			mkz = k.z * sMyData.fMassh;

			calcMaxMin

			fwrite(&mkx, sizeof( float ), 1, streamBin);
			fwrite(&mkz, sizeof( float ), 1, streamBin);
			fwrite(&mky, sizeof( float ), 1, streamBin);
		}
		Progr(0,100,(float)p/(float)polys,"Write shape geometry: ", ip); 
	}

	fwrite(&XFMIN, sizeof(XFMIN), 1, streamBin);// float
	fwrite(&XFMAX, sizeof(XFMAX), 1, streamBin);// float
	fwrite(&ZFMIN, sizeof(ZFMIN), 1, streamBin);// float
	fwrite(&ZFMAX, sizeof(ZFMAX), 1, streamBin);// float
	fwrite(&YFMIN, sizeof(YFMIN), 1, streamBin);// float
	fwrite(&YFMAX, sizeof(YFMAX), 1, streamBin);// float
Tamom:
	fclose(streamBin); 
	if(sMyData.bToText)
	{	char fullname[128]; sprintf(fullname, "%s\\%s%s", sMyData.szExport, nodeName, bClone ? ".clshp" : ".shp");
		ShapeToText(fullname);
	}
	return;
}



//VOID OneShape(Interface *ip,IUtil *iu,int type)//IDC_CHECK_SHAPE
/*VOID OneShape(LPVOID par)
{
int		i;
ThrPar		*thrPar;
Interface	*ip;
IUtil		*iu;
int			type;

	thrPar  = (ThrPar*)par;
	ip		= thrPar->ip;
	iu		= thrPar->iu;
	type	= thrPar->type;


	
	i = GetSelID(ip);

	if(i > 0)// select qilingani ko'p bo'lsa 
	{
		MessageBox(NULL,"1nechasini exp (belgilang)-- ni belgilamasdan","1 nechtasini belgilabsiz!",MB_OK);
		return;
	}
	if( (i < 0) || (i > 65000) ) // select qilingani umuman bo'lmasa
	{
		MessageBox(NULL,"Hech qanaqa obyekt topolmadim!","Diqqat",MB_OK);
		return;
	}

	__try
	{
		streamBin = 0;
		ExportOneShape(ip,iu,0,1);
		streamBin = 0;
	}
	__except(1,1)
	{
		MessageBox(NULL, "Kutilmagan gen. xato", "Diqqat!!!", MB_OK);
		if(streamBin) fclose(streamBin);
	}
}*/


//VOID AllShapes(Interface *ip,IUtil *iu,int type)
VOID AllShapes(LPVOID par)
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
			ExportOneShape(ip,iu,k);
			streamBin = 0;
		}
		__except(1,1)
		{
			MessageBox(NULL, "Kutilmagan gen. xato", "!!!", MB_OK);
			if(streamBin) fclose(streamBin);
		}
	}
}
