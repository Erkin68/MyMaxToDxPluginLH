#include "ExpDX.h"
#include "modstack.h"
#include "iunwrap.h"
#include "stdmat.h"


/*VOID WriteMtrl(INode* node, FILE* f)
{
Mtl			*m = node->GetMtl();
BYTE		txtrNameLen = 0x00;
BitmapTex	*bitmap;
Texmap		*tmap; 
int			len;
TCHAR		*n, *nShort;

	if(!m)//Material yo'q:
	{	fwrite(&txtrNameLen, 1, 1, f); 
		return;
	}

	if(m->ClassID() == Class_ID(DMTL_CLASS_ID, 0)) 
	{	// Access the Diffuse map and see if it's a Bitmap texture
		tmap = m->GetSubTexmap(ID_DI);
		if(tmap)
		{	if(tmap->ClassID() == Class_ID(BMTEX_CLASS_ID, 0)) 
			{	bitmap = (BitmapTex*)tmap;
				n = bitmap->GetMapName();
		}	}
		else
		{	fwrite(&txtrNameLen, 1, 1, f); 
			return;
	}	}
	else if(m->ClassID() == Class_ID(MULTI_CLASS_ID, 0))
	{	Mtl* subm = m->GetSubMtl(mesh->faces[0].getMatID() );
		if(subm->ClassID() == Class_ID(DMTL_CLASS_ID, 0)) 
		{	// Access the Diffuse map and see if it's a Bitmap texture
			tmap = subm->GetSubTexmap(ID_DI);
			if(tmap)
			{	if(tmap->ClassID() == Class_ID(BMTEX_CLASS_ID, 0)) 
				{	bitmap = (BitmapTex*)tmap;
					n = bitmap->GetMapName();
			}	}	
			else
			{	fwrite(&txtrNameLen, 1, 1, f); 
				return;
	}	}	}
	//else if(m->ClassID() == Class_ID(MTL_COMPOSITE, 0))
	//{	fwrite(&txtrNameLen, 1, 1, f); 
	//	MessageBox(NULL, "Composite mtrl not supported.", "Err.", MB_OK);
	//	return;	
	//}

	nShort = strrchr(n, '\\')+1;
	len = lstrlen(nShort);
	txtrNameLen = (BYTE)len;
	fwrite(&txtrNameLen, 1, 1, f);
	fwrite(nShort, 1, len, f);
	
	return;
}*/
