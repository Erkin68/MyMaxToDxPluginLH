#include "Stdafx.h"

int main(int argc, char* argv[])
{
	// Dumb test

	udword Topology[] = {
		1,0,2,
		0,3,2,
		3,0,4,
		0,1,4
		/*0,1,2,
		1,2,3,
		2,3,4,
		3,4,5,
		4,5,6,
		5,6,7,
		6,7,8,
		7,8,9*/

/*0, 1,  2,
 1, 0,  3,
 4, 5,  6,
 5, 4,  7,
 8, 9,  10,
 9, 8,  11,
 12, 13,  14,
 13, 12,  15,
 16, 17,  18,
 17, 16,  19,
 20, 21,  22,
 21, 20,  6,
 23, 24,  25,
 24, 23,  26,
 27, 12,  28,
 12, 27,  29,
 30, 31,  32,
 31, 30,  33,
 34, 35,  36,
 35, 34,  22,
 37, 38,  39,
 38, 37,  40,
 41, 27,  42,
 27, 41,  43,
 44, 6,  20,
 6, 44,  4,
 45, 22,  46,
 22, 45,  20,
 47, 20,  45,
 20, 47,  44,
 28, 14,  48,
 14, 28,  12,
 42, 28,  49,
 28, 42,  27,
 49, 48,  50,
 48, 49,  28 */
	};

	STRIPERCREATE sc;
	sc.DFaces			= Topology;
	sc.NbFaces			= 4;
	sc.AskForWords		= true;
	sc.ConnectAllStrips	= true;
	sc.OneSided			= false;
	sc.SGIAlgorithm		= true;

	Striper Strip;
	Strip.Init(sc);

	STRIPERRESULT sr;
	Strip.Compute(sr);

	fprintf(stdout, "Number of strips: %d\n", sr.NbStrips);
	uword* Refs = (uword*)sr.StripRuns;
	for(udword i=0;i<sr.NbStrips;i++)
	{
		udword NbRefs = sr.StripLengths[i];
		fprintf(stdout, "Strip %d , numFaces: %d :\n", i, NbRefs-2);
		for(udword j=0;j<NbRefs;j++)
		{
			fprintf(stdout, "%d ", *Refs++);
		}
		fprintf(stdout, "\n");
	}

	getchar();
	return 0;
}
