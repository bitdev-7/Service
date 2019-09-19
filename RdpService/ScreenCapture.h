#pragma once
#include <windows.h>
#include <gdiplus.h>
#include <stdio.h>
using namespace Gdiplus;

#pragma comment(lib, "gdiplus.lib")

#define LPBMIH	LPBITMAPINFOHEADER

struct GdiDS
{
	// Grid Coordinates
	int		iGridX;
	int		iGridY;

	// Rectangular Area of the Grid
	int		iWidth1;
	int		iWidth2;
	int		iHeight1;
	int		iHeight2;

	// Number of Colors in DIB Color Table
	int		nColors;

	// Number of Bytes in the DIB Information Header
	DWORD	dwBitMapHeader;

	// DIB Lengths and Starting Coordinate
	DWORD	dwLen;
	DWORD	dwCompress;
	DWORD	iStartPos;

	// DIB
	BITMAP	DIBitmap;

	// DIB Information Header
	BITMAPINFOHEADER	BMIH;

	// Pointer to the DIB Information Header
	LPBITMAPINFOHEADER	lpBMIH;

	// DC Handle for the Region
	HDC		hMemDC;

	// Bitmap Handle to the Region
	HBITMAP	hDIBitmap;

	// Pointers to the Region Uncompressed DIB
	char	*pDIB;

	// Pointer to the Changes in the Region DIB
	char	*pDIBChange;
	char	*pDIBChangeStart;

	// Pointer to the Compressed Region DIB
	char	*pDIBCompress;

	// Pointer to the Global Region DIB
	char	*pDIBitmap;

	// DIB Flags
	BOOL	fDIBitmap;
	BOOL	fChange;
};

// The Gdi Linked List
struct GdiList
{
	struct	GdiDS	Gdi;
	struct	GdiList	*pNext;
};

class ScreenCapture
{
public:
	ScreenCapture();
	virtual ~ScreenCapture();


public:

	HDC hDDC, hNullDC;
	struct	GdiList	GdiStart;
	struct	GdiList	*pGdiNode;
	int		iWidth, iHeight;
	int		nGridX;
	int		nGridY;

public:
	struct	GdiList	*Add_Gdi(struct GdiList *pNode, struct GdiDS Gdi);
	void	Clear_Gdi(struct GdiList *pStart);
	void	InitDisplay();
	int GetRegionDisplay();
	void memblast(void* dest, void* src, DWORD count);
	void ClearDisplay();

	int GetEncoderClsid(const WCHAR* format, CLSID* pClsid);
	char *CaptureAnImage(int &len);

	void Reset() { pGdiNode = GdiStart.pNext; };
	GdiList* GetCurNode() { return pGdiNode; };
	void Next() { pGdiNode = pGdiNode->pNext; };

};

