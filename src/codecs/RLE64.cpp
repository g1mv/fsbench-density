// ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>



// ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned long long RLE64_Compress64(const void *pacSource, void *pacDest, unsigned long long plSize)
{
	unsigned long long *alSource;
	unsigned long long *alDest;
	unsigned long long lPrevious;
	unsigned long long lActual;
	unsigned long long lRep;
	unsigned long long lCont;

	alSource = (unsigned long long *) pacSource;
	alDest = (unsigned long long *) pacDest;
	lPrevious = *alSource;
	alSource++;
	lRep = 0;
	do
	{
		lActual = *alSource;
		alSource++;
		if ((lActual == lPrevious) && (lRep < ULLONG_MAX))
		{
			lRep++;
		}
		//Optimize the most common case
		else if ((lRep == 0) && (lPrevious != 0xAAAAAAAAAAAAAAAA))
		{
			*alDest = lPrevious;
			alDest++;
			lPrevious = lActual;
		}
		else if ((lRep < 3) && (lPrevious != 0xAAAAAAAAAAAAAAAA))
		{
			for (lCont = 0; lCont <= lRep; lCont++)
			{
				*alDest = lPrevious;
				alDest++;
			}
			lRep = 0;
			lPrevious = lActual;
		}
		else
		{
			*alDest = 0xAAAAAAAAAAAAAAAA;
			alDest++;
			*alDest = lRep;
			alDest++;
			*alDest = lPrevious;
			alDest++;
			lRep = 0;
			lPrevious = lActual;
		}
	}
	while ((alSource - (unsigned long long *) pacSource) <= plSize / sizeof(unsigned long long));
	//Write any pending
	for (lCont = 0; lCont <= lRep; lCont++)
	{
		*alDest = lActual;
		alDest++;
	}
	//Write rest of non qword bytes
	memcpy(alDest, alSource, plSize & (sizeof(unsigned long long) - 1));
	return((alDest - (unsigned long long *) pacDest - 1) * sizeof(unsigned long long) + (plSize & (sizeof(unsigned long long) - 1)));

}


// ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned long long RLE64_Uncompress64(const void *pacSource, void *pacDest, unsigned long long plSize)
{
	unsigned long long *alSource;
	unsigned long long *alDest;
	unsigned long long lActual;
	unsigned long long lRep;
	unsigned long long lCont;

	alSource = (unsigned long long *) pacSource;
	alDest = (unsigned long long *) pacDest;
	do
	{
		lActual = *alSource;
		alSource++;
		if (lActual == 0xAAAAAAAAAAAAAAAA)
		{
			lRep = *alSource;
			alSource++;
			lActual = *alSource;
			alSource++;
			for (lCont = 0; lCont <= lRep; lCont++)
			{
				*alDest = lActual;
				alDest++;
			}
		}
		else
		{
			*alDest = lActual;
			alDest++;
		}
	}
	while ((alSource - (unsigned long long *) pacSource) <= plSize / sizeof(unsigned long long));
	//Write rest of non qword bytes
	memcpy(alDest, alSource, plSize & (sizeof(unsigned long long) - 1));
	return((alDest - (unsigned long long *) pacDest - 1) * sizeof(unsigned long long)  + (plSize & (sizeof(unsigned long long) - 1)));
}


// ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned int RLE64_Compress32(const void *paiSource, void *paiDest, unsigned int piSize)
{
	unsigned int *aiSource;
	unsigned int *aiDest;
	unsigned int iPrevious;
	unsigned int iActual;
	unsigned int iRep;
	unsigned int iCont;

	aiSource = (unsigned int *) paiSource;
	aiDest = (unsigned int *) paiDest;
	iPrevious = *aiSource;
	aiSource++;
	iRep = 0;
	do
	{
		iActual = *aiSource;
		aiSource++;
		if ((iActual == iPrevious) && (iRep < UINT_MAX))
		{
			iRep++;
		}
		//Optimize the most common case
		else if ((iRep == 0) && (iPrevious != 0xAAAAAAAA))
		{
			*aiDest = iPrevious;
			aiDest++;
			iPrevious = iActual;
		}
		else if ((iRep < 3) && (iPrevious != 0xAAAAAAAA))
		{
			for (iCont = 0; iCont <= iRep; iCont++)
			{
				*aiDest = iPrevious;
				aiDest++;
			}
			iRep = 0;
			iPrevious = iActual;
		}
		else
		{
			*aiDest = 0xAAAAAAAA;
			aiDest++;
			*aiDest = iRep;
			aiDest++;
			*aiDest = iPrevious;
			aiDest++;
			iRep = 0;
			iPrevious = iActual;
		}
	}
	while ((aiSource - (unsigned int *) paiSource) <= piSize / sizeof(unsigned int));
	//Write any pending
	for (iCont = 0; iCont <= iRep; iCont++)
	{
		*aiDest = iActual;
		aiDest++;
	}
	//Write rest of non qword bytes
	memcpy(aiDest, aiSource, piSize & (sizeof(unsigned int) - 1));
	return((aiDest - (unsigned int *) paiDest - 1) * sizeof(unsigned int) + (piSize & (sizeof(unsigned int) - 1)));

}


// ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned int RLE64_Uncompress32(const void *paiSource, void *paiDest, unsigned int piSize)
{
	unsigned int *aiSource;
	unsigned int *aiDest;
	unsigned int iActual;
	unsigned int iRep;
	unsigned int iCont;

	aiSource = (unsigned int *) paiSource;
	aiDest = (unsigned int *) paiDest;
	do
	{
		iActual = *aiSource;
		aiSource++;
		if (iActual == 0xAAAAAAAA)
		{
			iRep = *aiSource;
			aiSource++;
			iActual = *aiSource;
			aiSource++;
			for (iCont = 0; iCont <= iRep; iCont++)
			{
				*aiDest = iActual;
				aiDest++;
			}
		}
		else
		{
			*aiDest = iActual;
			aiDest++;
		}
	}
	while ((aiSource - (unsigned int *) paiSource) <= piSize / sizeof(unsigned int));
	//Write rest of non qword bytes
	memcpy(aiDest, aiSource, piSize & (sizeof(unsigned int) - 1));
	return((aiDest - (unsigned int *) paiDest - 1) * sizeof(unsigned int)  + (piSize & (sizeof(unsigned int) - 1)));
}


// ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned int RLE64_Compress16(const void *paiSource, void *paiDest, unsigned int piSize)
{
	unsigned short int *aiSource;
	unsigned short int *aiDest;
	unsigned int iPrevious;
	unsigned int iActual;
	unsigned int iRep;
	unsigned int iCont;

	aiSource = (unsigned short int *) paiSource;
	aiDest = (unsigned short int *) paiDest;
	iPrevious = *aiSource;
	aiSource++;
	iRep = 0;
	do
	{
		iActual = *aiSource;
		aiSource++;
		if ((iActual == iPrevious) && (iRep < USHRT_MAX))
		{
			iRep++;
		}
		//Optimize the most common case
		else if ((iRep == 0) && (iPrevious != 0xAAAA))
		{
			*aiDest = iPrevious;
			aiDest++;
			iPrevious = iActual;
		}
		else if ((iRep < 3) && (iPrevious != 0xAAAA))
		{
			for (iCont = 0; iCont <= iRep; iCont++)
			{
				*aiDest = iPrevious;
				aiDest++;
			}
			iRep = 0;
			iPrevious = iActual;
		}
		else
		{
			*aiDest = 0xAAAA;
			aiDest++;
			*aiDest = iRep;
			aiDest++;
			*aiDest = iPrevious;
			aiDest++;
			iRep = 0;
			iPrevious = iActual;
		}
	}
	while ((aiSource - (unsigned short int *) paiSource) <= piSize / sizeof(unsigned short int));
	//Write any pending
	for (iCont = 0; iCont <= iRep; iCont++)
	{
		*aiDest = iActual;
		aiDest++;
	}
	//Write rest of non qword bytes
	memcpy(aiDest, aiSource, piSize & (sizeof(unsigned short int) - 1));
	return((aiDest - (unsigned short int *) paiDest - 1) * sizeof(unsigned short int) + (piSize & (sizeof(unsigned short int) - 1)));

}


// ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned int RLE64_Uncompress16(const void *paiSource, void *paiDest, unsigned int piSize)
{
	unsigned short int *aiSource;
	unsigned short int *aiDest;
	unsigned int iActual;
	unsigned int iRep;
	unsigned int iCont;

	aiSource = (unsigned short int *) paiSource;
	aiDest = (unsigned short int *) paiDest;
	do
	{
		iActual = *aiSource;
		aiSource++;
		if (iActual == 0xAAAA)
		{
			iRep = *aiSource;
			aiSource++;
			iActual = *aiSource;
			aiSource++;
			for (iCont = 0; iCont <= iRep; iCont++)
			{
				*aiDest = iActual;
				aiDest++;
			}
		}
		else
		{
			*aiDest = iActual;
			aiDest++;
		}
	}
	while ((aiSource - (unsigned short int *) paiSource) <= piSize / sizeof(unsigned short int));
	//Write rest of non qword bytes
	memcpy(aiDest, aiSource, piSize & (sizeof(unsigned short int) - 1));
	return((aiDest - (unsigned short int *) paiDest - 1) * sizeof(unsigned short int)  + (piSize & (sizeof(unsigned short int) - 1)));
}




// ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned int RLE64_Compress8(const void *pacSource, void *pacDest, unsigned int piSize)
{
	unsigned char *acSource;
	unsigned char *acDest;
	unsigned int iPrevious;
	unsigned int iActual;
	unsigned int iRep;
	unsigned int iCont;

	acSource = (unsigned char *) pacSource;
	acDest = (unsigned char *) pacDest;
	iPrevious = *acSource;
	acSource++;
	iRep = 0;
	do
	{
		iActual = *acSource;
		acSource++;
		if ((iActual == iPrevious) && (iRep < UCHAR_MAX))
		{
			iRep++;
		}
		//Optimize the most common case
		else if ((iRep == 0) && (iPrevious != 0xAA))
		{
			*acDest = iPrevious;
			acDest++;
			iPrevious = iActual;
		}
		else if ((iRep < 3) && (iPrevious != 0xAA))
		{
			for (iCont = 0; iCont <= iRep; iCont++)
			{
				*acDest = iPrevious;
				acDest++;
			}
			iRep = 0;
			iPrevious = iActual;
		}
		else
		{
			*acDest = 0xAA;
			acDest++;
			*acDest = iRep;
			acDest++;
			*acDest = iPrevious;
			acDest++;
			iRep = 0;
			iPrevious = iActual;
		}
	}
	while ((acSource - (unsigned char *) pacSource) <= piSize);
	//Write any pending
	for (iCont = 0; iCont <= iRep; iCont++)
	{
		*acDest = iActual;
		acDest++;
	}
	return((acDest - (unsigned char *) pacDest - 1));

}


// ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned int RLE64_Uncompress8(const void *pacSource, void *pacDest, unsigned int piSize)
{
	unsigned char *acSource;
	unsigned char *acDest;
	unsigned int iActual;
	unsigned int iRep;
	unsigned int iCont;

	acSource = (unsigned char *) pacSource;
	acDest = (unsigned char *) pacDest;
	do
	{
		iActual = *acSource;
		acSource++;
		if (iActual == 0xAA)
		{
			iRep = *acSource;
			acSource++;
			iActual = *acSource;
			acSource++;
			for (iCont = 0; iCont <= iRep; iCont++)
			{
				*acDest = iActual;
				acDest++;
			}
		}
		else
		{
			*acDest = iActual;
			acDest++;
		}
	}
	while ((acSource - (unsigned char *) pacSource) <= piSize);
	return((acDest - (unsigned char *) pacDest - 1));
}

/*
// ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
int _tmain (int argc, TCHAR *argv[])
{
	unsigned int iSizeOriginal, iSizeCompressed, iSizeUncompressed;
	char *acOriginal, *acCompressed, *acUncompressed;
	clock_t iStart, iEnd, iDif;
	FILE *pFile;


	_tprintf(_T("\nRLE64 R3.00 - (c) 2011 by Javier Gutierrez Chamorro - http://guti.isgreat.org\n"));
	_tprintf(_T("A fast Run Length Encoding (RLE) compressor / decompressor.\n\n"));

	if (argc < 5)
	{
		_tprintf(_T("Usage: rle64.exe <8|16|32|64> <OriginalFile> <CompressedFile> <UncompressedFile>\n"));
		_tprintf(_T("Example: rle64.exe 64 girl.bmp girl.bmp.rle64 girl.unrle64.bmp\n"));
		return(-1);
	}

	pFile = _tfopen(argv[2], _T("rb"));
	if (!pFile)
	{
		_tprintf(_T("Cannot read original file %s\n"), argv[2]);
		return(-2);
	}
	iSizeOriginal = filelength(fileno(pFile));
	acOriginal = (char *) malloc(iSizeOriginal + 8);
	if (!acOriginal)
	{
		_tprintf(_T("Cannot allocate memory for original file\n"));
		return(-3);
	}
	fread(acOriginal, 1, iSizeOriginal, pFile);
	fclose(pFile);

	acCompressed =  (char *) malloc(iSizeOriginal << 1);
	if (!acCompressed)
	{
		_tprintf(_T("Cannot allocate memory for compressed file\n"));
		return(-4);
	}

	_tprintf(_T("Compressing %s (%ld bytes)..."), argv[2], iSizeOriginal);
	iStart = clock();
	if (_tcscmp(argv[1], _T("8")) == 0)
	{
		iSizeCompressed = Compress8(acOriginal, acCompressed, iSizeOriginal);
	}
	else if (_tcscmp(argv[1], _T("16")) == 0)
	{
		iSizeCompressed = Compress16(acOriginal, acCompressed, iSizeOriginal);
	}
	else if (_tcscmp(argv[1], _T("32")) == 0)
	{
		iSizeCompressed = Compress32(acOriginal, acCompressed, iSizeOriginal);
	}
	else
	{
		iSizeCompressed = Compress64(acOriginal, acCompressed, iSizeOriginal);
	}
	iEnd = clock();
	//Prevent later divisions by zero
	iDif = iEnd - iStart;
	if (iDif == 0)
	{
		iDif = 1;
	}
	_tprintf(_T("\rCompressed %s in %ld ms (%ld MiB/s).\n"), argv[2], iDif, iSizeOriginal / iDif / 1000);
	free(acOriginal);

	pFile = _tfopen(argv[3], _T("wb"));
	if (!pFile)
	{
		_tprintf(_T("Cannot write compressed file %s\n"), argv[3]);
		return(-5);
	}
	fwrite(acCompressed, 1, iSizeCompressed, pFile);
	fclose(pFile);

	acUncompressed =  (char *) malloc(iSizeOriginal + 8);
	if (!acUncompressed)
	{
		_tprintf(_T("Cannot allocate memory for uncompressed file\n"));
		return(-6);
	}

	_tprintf(_T("Uncompressing %s (%ld bytes)..."), argv[3], iSizeCompressed);
	iStart = clock();
	if (_tcscmp(argv[1],_T("8")) == 0)
	{
		iSizeUncompressed = Uncompress8(acCompressed, acUncompressed, iSizeCompressed);
	}
	else if (_tcscmp(argv[1], _T("16")) == 0)
	{
		iSizeUncompressed = Uncompress16(acCompressed, acUncompressed, iSizeCompressed);
	}
	else if (_tcscmp(argv[1], _T("32")) == 0)
	{
		iSizeUncompressed = Uncompress32(acCompressed, acUncompressed, iSizeCompressed);
	}
	else
	{
		iSizeUncompressed = Uncompress64(acCompressed, acUncompressed, iSizeCompressed);
	}
	iEnd = clock();
	//Prevent later divisions by zero
	iDif = iEnd - iStart;
	if (iDif == 0)
	{
		iDif = 1;
	}
	_tprintf(_T("\rUncompressed %s in %ld ms (%ld MiB/s).\n"), argv[3], iDif, iSizeOriginal / iDif / 1000);


	pFile = _tfopen(argv[4], _T("wb"));
	if (!pFile)
	{
		_tprintf(_T("Cannot write uncompressed file %s\n"), argv[4]);
		return(-7);
	}
	fwrite(acUncompressed, 1, iSizeUncompressed, pFile);
	fclose(pFile);

	free(acUncompressed);
	free(acCompressed);

	return(0);
}*/