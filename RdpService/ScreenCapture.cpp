#include "ScreenCapture.h"

#include "base.h"


ScreenCapture::ScreenCapture() : nGridX(4), nGridY(4)
{
}


ScreenCapture::~ScreenCapture()
{
}

struct GdiList *ScreenCapture::Add_Gdi(struct GdiList *pNode, struct GdiDS Gdi) {
	if (pNode->pNext = (struct GdiList *)malloc(sizeof(struct GdiList)))
	{
		// Move to the Created Node
		pNode = pNode->pNext;

		// Set the Grid Coordinates
		pNode->Gdi.iGridX = Gdi.iGridX;
		pNode->Gdi.iGridY = Gdi.iGridY;

		// Set the Rectangular Coordinates of the Region
		pNode->Gdi.iWidth1 = Gdi.iWidth1;
		pNode->Gdi.iWidth2 = Gdi.iWidth2;
		pNode->Gdi.iHeight1 = Gdi.iHeight1;
		pNode->Gdi.iHeight2 = Gdi.iHeight2;

		// Set the Number of Colors in the DIB Color Table
		pNode->Gdi.nColors = Gdi.nColors;

		// Set the Number of Bytes in the DIB Information Header
		pNode->Gdi.dwBitMapHeader = Gdi.dwBitMapHeader;

		// Bitmap Lengths and Starting Coordinate
		pNode->Gdi.dwLen = Gdi.dwLen;
		pNode->Gdi.dwCompress = Gdi.dwCompress;
		pNode->Gdi.iStartPos = Gdi.iStartPos;

		// Set the Device Independant Bitmap
		pNode->Gdi.DIBitmap = Gdi.DIBitmap;

		// Set the Device Independent Bitmap Information Header
		pNode->Gdi.BMIH = Gdi.BMIH;

		// Set the Pointer to the Device Independant Bitmap Information Header
		pNode->Gdi.lpBMIH = Gdi.lpBMIH;

		// DC Handle for the Region
		pNode->Gdi.hMemDC = Gdi.hMemDC;

		// Bitmap Handle to the Region
		pNode->Gdi.hDIBitmap = Gdi.hDIBitmap;

		// Pointer to the Region Uncompressed DIB
		pNode->Gdi.pDIB = Gdi.pDIB;

		// Pointer to the Changes in the Region DIB
		pNode->Gdi.pDIBChange = Gdi.pDIBChange;

		// Pointer to the Compressed Region DIB
		pNode->Gdi.pDIBCompress = Gdi.pDIBCompress;

		// Pointer to the Global Region Bitmap
		pNode->Gdi.pDIBitmap = Gdi.pDIBitmap;

		// Region Bitmap Flag
		pNode->Gdi.fDIBitmap = Gdi.fDIBitmap;
		pNode->Gdi.fChange = Gdi.fChange;

		pNode->pNext = NULL;
		return pNode;
	}
	return NULL;
}

void ScreenCapture::Clear_Gdi(struct GdiList *pStart) {

	struct	GdiList	*pPrev;
	struct	GdiList	*pNode;
	while (pNode = pStart->pNext)
	{
		// Point to the Start of the List
		pPrev = pStart;

		// Link with the Node after the Current Node
		pPrev->pNext = pNode->pNext;

		// Delete the Memory DC
		DeleteDC(pNode->Gdi.hMemDC);

		// Delete the Bitmap Object
		DeleteObject(pNode->Gdi.hDIBitmap);

		// Free the Memory Associated w/ the Bitmap
		if (pNode->Gdi.fDIBitmap)
		{
			free(pNode->Gdi.pDIBitmap);
			free(pNode->Gdi.pDIB);
			free(pNode->Gdi.pDIBChangeStart);
		}

		// Delete the Current Node
		free(pNode);
	}
}

void ScreenCapture::InitDisplay()
{
	// Structure to Hold the Gdi Members
	struct	GdiDS	Gdi;

	// Grid Spacing
	int		iWidthX, iHeightY, nGrid;

	// Looping Variables
	int		iXGrid, iYGrid, iLoop;

	// Initialize the Gdi Linked List
	GdiStart.pNext = NULL;
	pGdiNode = &GdiStart;

	// Get the Device Context for the Entire Display
	hDDC = CreateDC(L"DISPLAY", NULL, NULL, NULL);

	// Get width and height of the Entire Display

	DISPLAY_DEVICE display;
	DEVMODE devmode;

	EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devmode);

	iWidth = devmode.dmPelsWidth;
	iHeight = devmode.dmPelsHeight;
	
	NOTIFY_MESSAGE(std::to_string(devmode.dmPelsWidth) + "\n" + std::to_string(devmode.dmPelsHeight));
	// Divide the Screen into the X Grid Regions
	if (nGridX > 0)
		iWidthX = iWidth / nGridX;
	else
		iWidthX = iWidth;

	// Divide the Screen into the Y Grid Regions
	if (nGridY > 0)
		iHeightY = iHeight / nGridY;
	else
		iHeightY = iHeight;

	// Create the Regional Bitmap and Memory DC Information for the Grids
	if ((nGridX > 0) && (nGridY > 0))
	{
		for (iXGrid = 0; iXGrid < nGridX; iXGrid++)
		{
			for (iYGrid = 0; iYGrid < nGridY; iYGrid++)
			{
				// Initialize the Bitmap Status
				Gdi.fChange = FALSE;
				Gdi.fDIBitmap = FALSE;

				// Get the Grid Coordinates
				Gdi.iGridX = iXGrid;
				Gdi.iGridY = iYGrid;

				// Get the Rectangular Coordinates of the Region
				Gdi.iWidth1 = iXGrid * iWidthX;
				Gdi.iWidth2 = iXGrid * iWidthX + iWidthX;
				Gdi.iHeight1 = iYGrid * iHeightY;
				Gdi.iHeight2 = iYGrid * iHeightY + iHeightY;

				// Create a Regional Compatible Memory DCs for the DC
				Gdi.hMemDC = CreateCompatibleDC(hDDC);

				// Create a Regional Compatible Bitmaps for the DC
				Gdi.hDIBitmap = CreateCompatibleBitmap(hDDC, iWidthX, iHeightY);

				// Select the Regional Bitmap Associated with the Device Context into the Regional Memory DC
				SelectObject(Gdi.hMemDC, Gdi.hDIBitmap);

				// Add the Gdi Information to the Linked List
				pGdiNode = Add_Gdi(pGdiNode, Gdi);
			}
		}
	}
	else // One Grid Component Not Used
	{
		nGrid = max(nGridX, nGridY);
		for (iLoop = 0; iLoop < nGrid; iLoop++)
		{
			// Initialize the Bitmap Status
			Gdi.fChange = FALSE;
			Gdi.fDIBitmap = FALSE;

			// Get the Rectangular Coordinates of the X Region
			if (nGridX > 0)
			{
				Gdi.iGridX = iLoop;
				Gdi.iWidth1 = iLoop * iWidthX;
				Gdi.iWidth2 = iLoop * iWidthX + iWidthX;
			}
			else
			{
				Gdi.iGridX = 0;
				Gdi.iWidth1 = 0;
				Gdi.iWidth2 = iWidthX;
			}

			// Get the Rectangular Coordinates of the Y Region
			if (nGridY > 0)
			{
				Gdi.iGridY = iLoop;
				Gdi.iHeight1 = iLoop * iHeightY;
				Gdi.iHeight2 = iLoop * iHeightY + iHeightY;
			}
			else
			{
				Gdi.iGridY = 0;
				Gdi.iHeight1 = 0;
				Gdi.iHeight2 = iHeightY;
			}

			// Create a Regional Compatible Memory DCs for the DC
			Gdi.hMemDC = CreateCompatibleDC(hDDC);

			// Create a Regional Compatible Bitmaps for the DC
			Gdi.hDIBitmap = CreateCompatibleBitmap(hDDC, iWidthX, iHeightY);

			// Select the Regional Bitmap Associated with the Device Context into the Regional Memory DC
			SelectObject(Gdi.hMemDC, Gdi.hDIBitmap);

			// Add the Gdi Information to the Linked List
			pGdiNode = Add_Gdi(pGdiNode, Gdi);
		}
	}

	// Get a DC to Get the DIB From and Remap the System Palette
	hNullDC = GetDC(NULL);

}

int ScreenCapture::GetRegionDisplay()
{
	// Grid Variables
	int		iWidth1, iWidth2, iHeight1, iHeight2;

	// DIB Variables
	BOOL	bGotBits;
	DWORD	iLength;
	char	*pStartDIB;

	// Temporary Variables for Assembler Code
	DWORD	dwLen1;
	DWORD	dwBitMapHeader1;
	char	*pDIBitmap1;
	char	*pDIB1;
	int		fChange1;
	char	*pDIBChange1;

	// Get the Rectangular Coordinates of the Region
	iWidth1 = pGdiNode->Gdi.iWidth1;
	iWidth2 = pGdiNode->Gdi.iWidth2;
	iHeight1 = pGdiNode->Gdi.iHeight1;
	iHeight2 = pGdiNode->Gdi.iHeight2;

	// Blit the Screen in the Regional DC (DDB) to the Regional Memory DC
	BitBlt(pGdiNode->Gdi.hMemDC, 0, 0, iWidth2, iHeight2, hDDC, iWidth1, iHeight1, SRCCOPY);

	/* Step 2 : Convert the DDB to a DIB */

	// Get the Regional Bitmap Information
	GetObject(pGdiNode->Gdi.hDIBitmap, sizeof(BITMAP), &(pGdiNode->Gdi.DIBitmap));

	// Set the Color Mode

	pGdiNode->Gdi.DIBitmap.bmBitsPixel = 4;

	// Initialize the Bitmap Info Header
	pGdiNode->Gdi.BMIH.biSize = sizeof(BITMAPINFOHEADER);
	pGdiNode->Gdi.BMIH.biWidth = pGdiNode->Gdi.DIBitmap.bmWidth;
	pGdiNode->Gdi.BMIH.biHeight = pGdiNode->Gdi.DIBitmap.bmHeight;
	pGdiNode->Gdi.BMIH.biPlanes = 1;
	pGdiNode->Gdi.BMIH.biBitCount = (WORD)pGdiNode->Gdi.DIBitmap.bmPlanes * (WORD)pGdiNode->Gdi.DIBitmap.bmBitsPixel;
	pGdiNode->Gdi.BMIH.biCompression = BI_RGB;
	pGdiNode->Gdi.BMIH.biSizeImage = 0;
	pGdiNode->Gdi.BMIH.biXPelsPerMeter = 0;
	pGdiNode->Gdi.BMIH.biYPelsPerMeter = 0;
	pGdiNode->Gdi.BMIH.biClrUsed = 0;
	pGdiNode->Gdi.BMIH.biClrImportant = 0;

	// Set the Regional Number of Colors
	pGdiNode->Gdi.nColors = 1 << pGdiNode->Gdi.BMIH.biBitCount;
	if (pGdiNode->Gdi.nColors > 256)
		pGdiNode->Gdi.nColors = 0; // Palette Not Used

								   // Set the Size of the Allocation to Hold the Bitmap Info Header and the Color Table
	pGdiNode->Gdi.dwLen = (DWORD)(sizeof(BITMAPINFOHEADER) + pGdiNode->Gdi.nColors * sizeof(RGBQUAD));

	// Allocate Memory to hold the Regional Bitmapinfoheader and Color Table
	if (!pGdiNode->Gdi.fDIBitmap)
	{
		pGdiNode->Gdi.pDIB = (char *)malloc(pGdiNode->Gdi.dwLen);
		pStartDIB = pGdiNode->Gdi.pDIB;
	}

	// Cast the Allocated Memory to a Bitmap Info Header for the Region
	pGdiNode->Gdi.lpBMIH = (LPBMIH)pGdiNode->Gdi.pDIB;

	// Set the Regional Bitmap Info Header to the Regional DIB
	*(pGdiNode->Gdi.lpBMIH) = pGdiNode->Gdi.BMIH;

	// Get the Regional DIB Size
	GetDIBits(hNullDC, pGdiNode->Gdi.hDIBitmap, 0L, (DWORD)pGdiNode->Gdi.BMIH.biHeight, (LPBYTE)NULL, (LPBITMAPINFO)pGdiNode->Gdi.lpBMIH, DIB_RGB_COLORS);

	// Set the Regional Bitmap Info Header with the Calculated Size
	pGdiNode->Gdi.BMIH = *(pGdiNode->Gdi.lpBMIH);

	// Save the Size of the BitMap Header
	pGdiNode->Gdi.dwBitMapHeader = pGdiNode->Gdi.dwLen;

	// Calculate the New Size of the Regional DIB w/ the Size of the Image Added
	pGdiNode->Gdi.dwLen += (DWORD)(pGdiNode->Gdi.BMIH.biSizeImage);

	// ReAllocate the Memory to Hold the Entire Regional DIB
	if (!pGdiNode->Gdi.fDIBitmap)
	{
		pGdiNode->Gdi.pDIB = pStartDIB;
		pGdiNode->Gdi.pDIB = (char *)realloc(pGdiNode->Gdi.pDIB, pGdiNode->Gdi.dwLen);
	}

	// Cast the Storage for the Regional DIB Bits
	pGdiNode->Gdi.lpBMIH = (LPBMIH)pGdiNode->Gdi.pDIB;

	// Get the Regional DIB
	bGotBits = GetDIBits(hNullDC, pGdiNode->Gdi.hDIBitmap, 0L, (DWORD)pGdiNode->Gdi.BMIH.biHeight, (LPBYTE)pGdiNode->Gdi.lpBMIH + (pGdiNode->Gdi.BMIH.biSize + pGdiNode->Gdi.nColors * sizeof(RGBQUAD)), (LPBITMAPINFO)pGdiNode->Gdi.lpBMIH, DIB_RGB_COLORS);

	// Compare the Contents of the Before and After Regional DIBS for a Change
	if (pGdiNode->Gdi.fDIBitmap)
	{
		dwLen1 = pGdiNode->Gdi.dwLen;
		dwBitMapHeader1 = pGdiNode->Gdi.dwBitMapHeader;
		pDIBitmap1 = pGdiNode->Gdi.pDIBitmap;
		pDIB1 = pGdiNode->Gdi.pDIB;
		fChange1 = pGdiNode->Gdi.fChange;

		// Compare the two Bitmaps for a Difference 4 bytes at a time.  
		// Skip the BitMapHeader
		__asm
		{
			MOV		ECX, dwLen1
			SUB		ECX, dwBitMapHeader1
			SHR		ECX, 2 // Divide by 4 for 4 bytes at a time
			MOV		EDX, dwBitMapHeader1
			MOV		ESI, pDIBitmap1
			ADD		ESI, EDX
			MOV		EDI, pDIB1
			ADD		EDI, EDX
			REP		CMPSD
			JNZ		SetFlagRegion1
			MOV		fChange1, FALSE
			JMP		ExitRegion1
			SetFlagRegion1 :
			MOV		fChange1, TRUE
				ExitRegion1 :
		}

		// Set the Change Status
		pGdiNode->Gdi.fChange = fChange1;

		// Build a DIB of Differences between the two Regional DIBs
		if (pGdiNode->Gdi.fChange)
		{
			DWORD		iZeros = 0;

			// Set the Length of the Data to Allocate w/o the BitMapHeader
			iLength = (pGdiNode->Gdi.dwLen - pGdiNode->Gdi.dwBitMapHeader);

			// Allocate enough memory to hold the Regional DIB w/o the BitMapHeader
			pGdiNode->Gdi.pDIBChange = pGdiNode->Gdi.pDIBChangeStart;
			pDIBChange1 = pGdiNode->Gdi.pDIBChange;

			// Build the Delta of the Before and After Bitmaps
			__asm
			{
				MOV		ECX, iLength // Length of Loop
				SHR		ECX, 2 // Divide by 4 for 4 bytes at a time
				MOV		EDI, pDIBChange1
				MOV		ESI, pDIB1
				ADD		ESI, dwBitMapHeader1
				MOV		EDX, pDIBitmap1
				ADD		EDX, dwBitMapHeader1
				SubtractRegion :
				LODSD
					SUB		EAX, [EDX]
					ADD		EDX, 4
					STOSD
					DEC		ECX
					JNZ		SubtractRegion
			}

			// Copy to the Global Regional DIB
			memblast(pGdiNode->Gdi.pDIBitmap, pGdiNode->Gdi.pDIB, pGdiNode->Gdi.dwLen);

			// Set the Length to Compress
			pGdiNode->Gdi.dwCompress = pGdiNode->Gdi.dwLen - pGdiNode->Gdi.dwBitMapHeader;

			// Set the Starting Position of the Data
			pGdiNode->Gdi.iStartPos = pGdiNode->Gdi.dwBitMapHeader;
		}
	}
	else
	{
		// Set the Length of the Data to Allocate
		iLength = (pGdiNode->Gdi.dwLen);

		// Allocate the Memory for the Global Regional DIB
		pGdiNode->Gdi.pDIBitmap = (char *)malloc(iLength);

		// Allocate enough memory to hold the Regional DIB
		pGdiNode->Gdi.pDIBChange = (char *)malloc(iLength);
		pGdiNode->Gdi.pDIBChangeStart = pGdiNode->Gdi.pDIBChange;

		// Copy to the Global Regional DIB
		memblast(pGdiNode->Gdi.pDIBitmap, pGdiNode->Gdi.pDIB, pGdiNode->Gdi.dwLen);
		memblast(pGdiNode->Gdi.pDIBChange, pGdiNode->Gdi.pDIB, pGdiNode->Gdi.dwLen);

		// Don't ReAllocate the Memory
		pGdiNode->Gdi.fDIBitmap = TRUE;
		pGdiNode->Gdi.fChange = TRUE;

		// Set the Length to Compress
		pGdiNode->Gdi.dwCompress = pGdiNode->Gdi.dwLen;

		// Set the Starting Position of the Data
		pGdiNode->Gdi.iStartPos = 0;
	}
	//	hNullDC = GetDC(NULL);
	// Default Send a Screen for the First Time
	return pGdiNode->Gdi.fChange;
}

void ScreenCapture::memblast(void* dest, void* src, DWORD count)
{
	DWORD	iCount;

	__asm
	{
		// Align Count to a DWORD Boundary
		MOV		ECX, count
		SHR		ECX, 2
		SHL		ECX, 2
		MOV		iCount, ECX

		// Copy All the DWORDs (32 bits at a Time)
		MOV		ESI, src		// Copy the Source Address to the Register
		MOV		EDI, dest	// Copy the Destination to the Register
		MOV		ECX, iCount	// Copy the Count to the Register
		SHR		ECX, 2		// Divide Count by 4 for DWORD Copy
		REP		MOVSD		// Move all the Source DWORDs to the Dest DWORDs

							// Get the Remaining Bytes to Copy
							MOV		ECX, count
							MOV		EAX, iCount
							SUB		ECX, EAX

							// Exit if All Bytes Copied
							JZ		Exit

							// Copy the Remaining BYTEs (8 bits at a Time)
							MOV		ESI, src		// Copy the Source Address to the Register
							ADD		ESI, EAX		// Set the Starting Point of the Copy
							MOV		EDI, dest	// Copy the Destination to the Register
							ADD		EDI, EAX		// Set the Destination Point of the Copy
							REP		MOVSB		// Move all the Source BYTEs to the Dest BYTEs
							Exit :
	}
}


int ScreenCapture::GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
	ImageCodecInfo* pImageCodecInfo = NULL;
	UINT num, size;
	GetImageEncodersSize(&num, &size);
	if (size == 0) return -1;
	pImageCodecInfo = (ImageCodecInfo*)malloc(size);
	if (pImageCodecInfo == NULL) return -1;
	GetImageEncoders(num, size, pImageCodecInfo);
	for (UINT j = 0; j < num; ++j) {
		if (!wcscmp(pImageCodecInfo[j].MimeType, format)) {
			*pClsid = pImageCodecInfo[j].Clsid;
			free(pImageCodecInfo);
			return j;
		}
	}
	free(pImageCodecInfo);
	return -1;
}

char *ScreenCapture::CaptureAnImage(int &len)
{
	HDC hdcScreen = pGdiNode->Gdi.hMemDC;

	HDC hdcMemDC = NULL;
	HBITMAP hbmScreen = NULL;
	BITMAP bmpScreen;
	int Width;
	int Height;

	Width = iWidth / nGridX;
	Height = iHeight / nGridY;
	// Create a compatible DC which is used in a BitBlt from the window DC
	hdcMemDC = CreateCompatibleDC(hdcScreen);

	// Create a compatible bitmap from the Window DC
	hbmScreen = CreateCompatibleBitmap(hdcScreen, Width,
		Height);


	// Select the compatible bitmap into the compatible memory DC.
	SelectObject(hdcMemDC, hbmScreen);

	// Bit block transfer into our compatible memory DC.
	BitBlt(hdcMemDC, 0, 0, Width,
		Height, hdcScreen, 0, 0, SRCCOPY);


	// Get the BITMAP from the HBITMAP
	GetObject(hbmScreen, sizeof(BITMAP), &bmpScreen);

	BITMAPFILEHEADER   bmfHeader;
	BITMAPINFOHEADER   bi;

	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = bmpScreen.bmWidth;
	bi.biHeight = bmpScreen.bmHeight;
	bi.biPlanes = 1;
	bi.biBitCount = 32;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed = 0;
	bi.biClrImportant = 0;

	DWORD dwBmpSize = ((bmpScreen.bmWidth * bi.biBitCount + 31) / 32) * 4 * bmpScreen.bmHeight;

	// Starting with 32-bit Windows, GlobalAlloc and LocalAlloc are implemented as wrapper functions that 
	// call HeapAlloc using a handle to the process's default heap. Therefore, GlobalAlloc and LocalAlloc 
	// have greater overhead than HeapAlloc.
	HANDLE hDIB = GlobalAlloc(GHND, dwBmpSize);
	char *lpbitmap = (char *)GlobalLock(hDIB);

	// Gets the "bits" from the bitmap and copies them into a buffer 
	// which is pointed to by lpbitmap.
	GetDIBits(hdcScreen, hbmScreen, 0,
		(UINT)bmpScreen.bmHeight,
		lpbitmap,
		(BITMAPINFO *)&bi, DIB_RGB_COLORS);

	// Add the size of the headers to the size of the bitmap to get the total file size
	DWORD dwSizeofDIB = dwBmpSize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

	//Offset to where the actual bitmap bits start.
	bmfHeader.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER);

	//Size of the file
	bmfHeader.bfSize = dwSizeofDIB;

	//bfType must always be BM for Bitmaps
	bmfHeader.bfType = 0x4D42; //BM   

	DWORD dwBytesWritten = 0;

	IStream* pIStream = NULL;
	if (CreateStreamOnHGlobal(NULL, TRUE, (LPSTREAM*)&pIStream) != S_OK)
		return FALSE;
	pIStream->Write((LPSTR)&bmfHeader, sizeof(BITMAPFILEHEADER), &dwBytesWritten);
	pIStream->Write((LPSTR)&bi, sizeof(BITMAPINFOHEADER), &dwBytesWritten);
	pIStream->Write((LPSTR)lpbitmap, dwBmpSize, &dwBytesWritten);

	//Unlock and Free the DIB from the heap
	GlobalUnlock(hDIB);
	GlobalFree(hDIB);

	//Close the handle for the file that was created
	//	CloseHandle(hFile);

	//Clean up

	DeleteObject(hbmScreen);
	DeleteObject(hdcMemDC);
	ReleaseDC(NULL, hdcScreen);

	//////////////////////////////////


	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	CLSID encoderClsid;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
	Image* pDIBImage;
	char *pImgArray;
	if (GetEncoderClsid(L"image/jpeg", &encoderClsid) > 0) {

		LARGE_INTEGER lnOffset;
		lnOffset.QuadPart = 0;
		if (pIStream->Seek(lnOffset, STREAM_SEEK_SET, NULL) != S_OK)
		{
			pIStream->Release();
			return FALSE;
		}
		pDIBImage = Image::FromStream(pIStream);
		IStream* pOutIStream = NULL;
		if (CreateStreamOnHGlobal(NULL, TRUE, (LPSTREAM*)&pOutIStream) != S_OK)
		{
			delete pDIBImage;
			pIStream->Release();
			return FALSE;
		}
		EncoderParameters encoderParameters;
		encoderParameters.Count = 1;
		encoderParameters.Parameter[0].Guid = EncoderQuality;
		encoderParameters.Parameter[0].Type = EncoderParameterValueTypeLong;
		encoderParameters.Parameter[0].NumberOfValues = 1;
		// setup compression level
		ULONG quality = 100;
		encoderParameters.Parameter[0].Value = &quality; //set quality 100 and size of jpeg  increases

														 //now save the image to the stream
		Status SaveStatus = pDIBImage->Save(pOutIStream, &encoderClsid,NULL /* &encoderParameters*/);
		if (SaveStatus != S_OK)
		{
			delete pDIBImage;
			pIStream->Release();
			pOutIStream->Release();
			return FALSE;
		}

		// get the size of the output stream
		ULARGE_INTEGER ulnSize;
		lnOffset.QuadPart = 0;
		if (pOutIStream->Seek(lnOffset, STREAM_SEEK_END, &ulnSize) != S_OK)
		{
			delete pDIBImage;
			pIStream->Release();
			pOutIStream->Release();
			return FALSE;
		}

		// now move the pointer to the beginning of the stream
		//(the stream should now contain a complete JPG file, just as if it were on the hard drive)
		lnOffset.QuadPart = 0;
		if (pOutIStream->Seek(lnOffset, STREAM_SEEK_SET, NULL) != S_OK)
		{
			delete pDIBImage;
			pIStream->Release();
			pOutIStream->Release();
			return FALSE;
		}

		//copy the stream JPG to memory
		DWORD dwJpgSize = (DWORD)ulnSize.QuadPart;

		pImgArray = new char[dwJpgSize];
		len = dwJpgSize;
		if (pOutIStream->Read(pImgArray, dwJpgSize, NULL) != S_OK)
		{
			delete pDIBImage;

			pIStream->Release();
			pOutIStream->Release();
			return FALSE;
		}

		//cleanup memory

		delete pDIBImage;
		pIStream->Release();
		pOutIStream->Release();
	}
	GdiplusShutdown(gdiplusToken);

	return pImgArray;
}

void ScreenCapture::ClearDisplay()
{
	// Delete the Created Device Context
	DeleteDC(hDDC);

	// Delete the Null DC
	DeleteDC(hNullDC);

	// Clear the Gdi Linked List
	Clear_Gdi(&GdiStart);
}

