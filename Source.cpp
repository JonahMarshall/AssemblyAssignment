/*******************************************************************
Purpose: Image brighten with C, assembly, MMX

Reference guide:

http://www.plantation-productions.com/Webster/www.artofasm.com/Windows/HTML/AoATOC.html

Note: above guide uses format <inst>(<source>, <dest>) while our compiler
requires <inst> <dest>, <source>

Description:	1. Create the assembly version of the C brighten function provided.
				2. Create the assembly version of the C brighten function using MMX assembly.

Marks

Assembly: 5 marks
MMX:	  5 marks

Due NOVEMBER 17 at midnight
*******************************************************************/

#include <windows.h>
#include <mmsystem.h>
#include <chrono>
#include <stdio.h>

HBITMAP hBitmap;
BITMAP Bitmap;
BITMAPFILEHEADER bmfh;
BITMAPINFO* pbmi;
BYTE* pBits;

LRESULT CALLBACK HelloWndProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	PSTR szCMLine, int iCmdShow) {
	static TCHAR szAppName[] = TEXT("HelloApplication");//name of app
	HWND	hwnd;//holds handle to the main window
	MSG		msg;//holds any message retrieved from the msg queue
	WNDCLASS wndclass;//wnd class for registration

	//defn wndclass attributes for this application
	wndclass.style = CS_HREDRAW | CS_VREDRAW;//redraw on refresh both directions
	wndclass.lpfnWndProc = HelloWndProc;//wnd proc to handle windows msgs/commands
	wndclass.cbClsExtra = 0;//class space for expansion/info carrying
	wndclass.cbWndExtra = 0;//wnd space for info carrying
	wndclass.hInstance = hInstance;//application instance handle
	wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);//set icon for window
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);//set cursor for window
	wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);//set background
	wndclass.lpszMenuName = NULL;//set menu
	wndclass.lpszClassName = szAppName;//set application name

	//register wndclass to O/S so approp. wnd msg are sent to application
	if (!RegisterClass(&wndclass)) {
		MessageBox(NULL, TEXT("This program requires Windows 95/98/NT"),
			szAppName, MB_ICONERROR);//if unable to be registered
		return 0;
	}
	//create the main window and get it's handle for future reference
	hwnd = CreateWindow(szAppName,		//window class name
		TEXT("Hello World for Windows"), // window caption
		WS_OVERLAPPEDWINDOW,	//window style
		CW_USEDEFAULT,		//initial x position
		CW_USEDEFAULT,		//initial y position
		CW_USEDEFAULT,		//initial x size
		CW_USEDEFAULT,		//initial y size
		NULL,				//parent window handle
		NULL,				//window menu handle
		hInstance,			//program instance handle
		NULL);				//creation parameters
	ShowWindow(hwnd, iCmdShow);//set window to be shown
	UpdateWindow(hwnd);//force an update so window is drawn

	//messgae loop
	while (GetMessage(&msg, NULL, 0, 0)) {//get message from queue
		TranslateMessage(&msg);//for keystroke translation
		DispatchMessage(&msg);//pass msg back to windows for processing
		//note that this is to put windows o/s in control, rather than this app
	}

	return msg.wParam;
}
void nonAsMbrighten(BITMAP* bitmap, INT brighten, BYTE* temppBits) {
	INT width = bitmap->bmWidth;
	INT height = bitmap->bmHeight;
	INT bitsperPixel = bitmap->bmBitsPixel;
	BYTE value = 0;//temp location of brighten value

	for (int i = 0; i < height * 3; i++) {
		for (int j = 0; j < width; j++) {
			value = temppBits[i * width + j];
			value = min(255, value + brighten);//deal with rollover via saturation
			pBits[i * width + j] = value;
		}
	}



}
/**
mmx registers: mm0-mm7 - general purpose
pxor - packed xor between two registers
movd - moves a double word (32 bits) between memory and reg or reg to reg
por - packed "or" between two registers
psllq - packed shift left quad (4 DWORDS)
movq - move quad (4 DWORDS or 64 bits) between memory and reg or reg to reg
paddusb - adds unsigned bytes (8 bytes) with saturation between two reg

*/
void mmx_brighten(BITMAP* bitmap, INT brighten, BYTE* buffer) {
	INT width = bitmap->bmWidth;
	INT height = bitmap->bmHeight;
	INT bitsPerPixel = bitmap->bmBitsPixel;
	__asm {
		//save all reg values to stack
		push eax
		push ebx
		push ecx
		push edx
		
		//number of pixels in image (w*h) in reg eax
		mov eax, width
		imul eax, height
		imul eax, 3

		//number of bytes in image (bitsPerPixel/8*pixels)
		mov edx, bitsPerPixel
		shr edx, 3
		imul edx, eax

		//divide eax by 8 as each mmx reg holds 8 bytes
		shr eax, 3
		//store buffer in reg ebx
		mov ebx, buffer
		//clear mm2 reg
			//mm2 never used

		//store brighten value
		pxor mm1, mm1
		movd mm1, brighten

		//brighten value needs to be in each byte of an mmx reg
		//loop and shift and load brighten value and "or"
		//until each byte in an mmx reg holds brighten value
		//use mm0 to hold value
		pxor mm0, mm0
		xor ecx, ecx
		movq mm0, mm1
	LOOP1: 
		cmp ecx, 7
		je END1
		psllq mm0, 8
		paddusb mm0, mm1
		inc ecx
		jmp LOOP1

	END1: 
		nop
		
		//note: can't use mm2 as work (calc) can only be done
		//using mmx reg. Only loading in a value can be done using
		//memory and mmx reg

		//clear ecx reg to use as counter
		xor ecx, ecx

		//start a loop
		//end loop if number of loops is greater than bytes
		//in image/8
		pxor mm1, mm1
	START: 
		cmp ecx, eax
		je END
		//load 8 bytes into mm1 
		movq mm1, [ebx+8*ecx]
		//add brighten value with saturation
		paddusb mm1, mm0
		//copy brighten value back to buffer
		movq [ebx+8*ecx], mm1
		//move the buffer pointer position by 8
		//since we are grabbing 8 bytes at once
			//not needed bc of how we loaded in
		//inc our counter (ecx)
		inc ecx
		//loop back to repeat
		jmp START
	END:
		nop
		//return reg values from stack		
		pop edx
		pop ecx
		pop ebx
		pop eax

		//end mmx (emms)
		emms
	}
}
void assembly_brighten(BITMAP* bitmap, INT brighten, BYTE* buffer) {
	INT width = bitmap->bmWidth;
	INT height = bitmap->bmHeight;
	INT bitsPerPixel = bitmap->bmBitsPixel;
	//REGISTERS

	//EAX, EBX, ECX, EDX are general purpose registers
	//ESI, EDI, EBP are also available as general purpose registers
	//AX, BX, CX, DX are the lower 16-bit of the above registers (think E as extended)
	//AH, AL, BH, BL, CH, CL, DH, DL are 8-bit high/low registers of the above (AX, BX, etc)
	//Typical use:
	//EAX accumulator for operands and results
	//EBX base pointer to data in the data segment
	//ECX counter for loops
	//EDX data pointer and I/O pointer
	//EBP frame pointer for stack frames
	//ESP stack pointer hardcoded into PUSH/POP operations
	//ESI source index for array operations
	//EDI destination index for array operations [e.g. copying arrays]
	//EIP instruction pointer
	//EFLAGS results flag hardcoded into conditional operations

	//SOME INSTRUCTIONS

	//MOV <source>, <destination>: mov reg, reg; mov reg, immediate; mov reg, memory; mov mem, reg; mov mem, imm
	//INC and DEC on registers or memory
	//ADD destination, source
	//SUB destination, source
	//CMP destination, source : sets the appropriate flag after performing (destination) - (source)
	//JMP label - jumps unconditionally ie. always to location marked by "label"
	//JE - jump if equal, JG/JL - jump if greater/less, JGE/JLE if greater or equal/less or equal, JNE - not equal, JZ - zero flag set
	//LOOP target: uses ECX to decrement and jump while ECX>0
	//logical instructions: AND, OR, XOR, NOT - performs bitwise logical operations. Note TEST is non-destructive AND instruction
	//SHL destination, count : shift left, SHR destination, count :shift right - carry flag (CF) and zero (Z) bits used, CL register often used if shift known
	//ROL - rotate left, ROR rotate right, RCL (rotate thru carry left), RCR (rotate thru carry right)
	//EQU - used to elimate hardcoding to create constants
	//MUL destination, source : multiplication
	//PUSH <source> - pushes source onto the stack
	//POP <destination> - pops off the stack into destination
	__asm
	{
		// save all registers you will be using onto stack
		push eax
		push ebx
		push ecx
		push edx
		push ebp
		push esi
		push edi

		// calculate the number of pixels
		mov ecx, width
		imul ecx, height
		imul ecx, 3


		// calculate the number of bytes in image (pixels*bitsperpixel)


		// store the address of the buffer into a register (e.g. ebx)
		mov ebx, buffer

		//setup counter register
		//not needed since assigned to ecx

		//create a loop
		//loop while still have pixels to brighten
		mov eax, brighten
	START: cmp ecx, 0
		  
		//jump out of loop if done
		je STOP

		//load a pixel into a register
		mov edx, [ebx+ecx]

		//need to work with each colour plane: A R G B
		mov ebp, edx

		//load same pixel then into 3 more registers
		mov esi, edx
		mov edi, edx

		//shift bits down for each channel
		shr ebp, 8
		shr esi, 16
		shr edi, 24

		//clear out other bits
		and edx, 0xff
		and ebp, 0xff
		and esi, 0xff
		and edi, 0xff

		//add brighten value to each pixel
		add edx, eax
		add ebp, eax
		add esi, eax
		add edi, eax

		//check each pixel to see if saturated
		//if greater than 255, set to 255
		cmp edx, 255
		jle C1
		mov edx, 255
	C1:
		cmp ebp, 255
		jle C2
		mov ebp, 255
	C2:
		cmp esi, 255
		jle C3
		mov esi, 255
	C3:
		cmp edi, 255
		jle C4
		mov edi, 255
	C4:
		nop

		//put pixel back together again
		//shift each channel amount needed
		shl ebp, 8
		shl esi, 16
		shl edi, 24
		//add each channel
		add edx, ebp
		add edx, esi
		add edx, edi
		//store back into buffer
		mov [ebx+ecx], edx

		//increment loop counter by 4
		// counter was set to max so we decrement and work backwards
		dec ecx
		dec ecx
		dec ecx
		dec ecx
		//loop back up
		jmp START
		//restore registers to original values before leaving
	STOP:pop edi
		pop esi
		pop ebp
		pop edx
		pop ecx
		pop ebx
		pop eax
		//function
	}

}

/**
Purpose: To handle windows messages for specific cases including when
the window is first created, refreshing (painting), and closing
the window.

Returns: Long - any error message (see Win32 API for details of possible error messages)
Notes:	 CALLBACK is defined as __stdcall which defines a calling
convention for assembly (stack parameter passing)
**/
LRESULT CALLBACK HelloWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
	HDC		hdc;
	PAINTSTRUCT ps;
	RECT	rect;
	BITMAP* bitmap;
	HDC hdcMem;
	BOOL bSuccess;
	DWORD dwBytesRead, dwInfoSize;
	HANDLE hFile;
	int error = 0;
	BYTE* temppBits;
	decltype(std::chrono::high_resolution_clock::now()) start;
	decltype(std::chrono::high_resolution_clock::now()) stop;


	switch (message) {
	case WM_CREATE://additional things to do when window is created


		hFile = CreateFile(TEXT("splash.bmp"), GENERIC_READ, FILE_SHARE_READ,
			NULL, OPEN_EXISTING, 0, NULL);

		if (hFile == INVALID_HANDLE_VALUE) {
			error = GetLastError();
			return 0;
		}

		error = sizeof(BITMAPFILEHEADER);

		bSuccess = ReadFile(hFile, &bmfh, sizeof(BITMAPFILEHEADER),
			&dwBytesRead, NULL);

		if (!bSuccess || (dwBytesRead != sizeof(BITMAPFILEHEADER))
			|| (bmfh.bfType != *(WORD*) "BM"))
		{
			//			CloseHandle(hFile);
			return NULL;
		}
		dwInfoSize = bmfh.bfOffBits - sizeof(BITMAPFILEHEADER);

		pbmi = (BITMAPINFO*)malloc(dwInfoSize);

		bSuccess = ReadFile(hFile, pbmi, dwInfoSize, &dwBytesRead, NULL);

		if (!bSuccess || (dwBytesRead != dwInfoSize))
		{
			free(pbmi);
			CloseHandle(hFile);
			return NULL;
		}
		hBitmap = CreateDIBSection(NULL, pbmi, DIB_RGB_COLORS, (VOID * *)& pBits, NULL, 0);
		ReadFile(hFile, pBits, bmfh.bfSize - bmfh.bfOffBits, &dwBytesRead, NULL);



		GetObject(hBitmap, sizeof(BITMAP), &Bitmap);
		return 0;
	case WM_LBUTTONDOWN:
		hdc = GetDC(hwnd);
		hdcMem = CreateCompatibleDC(hdc);
		GetObject(hBitmap, sizeof(BITMAP), &Bitmap);
		temppBits = (BYTE*)malloc(Bitmap.bmWidth * Bitmap.bmHeight * 3);
		memcpy(temppBits, pBits, Bitmap.bmWidth * Bitmap.bmHeight * 3);
		hBitmap = CreateDIBSection(NULL, pbmi, DIB_RGB_COLORS, (VOID * *)& pBits, NULL, 0);

		start = std::chrono::high_resolution_clock::now();
		assembly_brighten(&Bitmap, 30, temppBits);
		stop = std::chrono::high_resolution_clock::now();
		auto durationASM = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start);

		start = std::chrono::high_resolution_clock::now();
		mmx_brighten(&Bitmap, 30, temppBits);
		stop = std::chrono::high_resolution_clock::now();
		auto durationMMX = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start);

		memcpy(pBits, temppBits, Bitmap.bmWidth * Bitmap.bmHeight * 3);

		start = std::chrono::high_resolution_clock::now();
		nonAsMbrighten(&Bitmap, 30, temppBits);
		stop = std::chrono::high_resolution_clock::now();
		auto durationC = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start);


		char szBuffer[1000];
		snprintf(szBuffer,sizeof(szBuffer), "Assembly was %0.3f times faster than C, MMX was %0.3f times faster than C", (double)durationC.count()/(double)durationASM.count(), (double)durationC.count()/(double)durationMMX.count());
		MessageBoxA(hwnd, szBuffer, "Performance", MB_OK);
		
		SelectObject(hdcMem, hBitmap);

		
		BitBlt(hdc, 0, 0, Bitmap.bmWidth, Bitmap.bmHeight,
			hdcMem, 0, 0, SRCCOPY);
		return 0;
	case WM_RBUTTONDOWN:
		hdc = GetDC(hwnd);
		hdcMem = CreateCompatibleDC(hdc);
		GetObject(hBitmap, sizeof(BITMAP), &Bitmap);
		temppBits = (BYTE*)malloc(Bitmap.bmWidth * Bitmap.bmHeight * 3);
		memcpy(temppBits, pBits, Bitmap.bmWidth * Bitmap.bmHeight * 3);
		hBitmap = CreateDIBSection(NULL, pbmi, DIB_RGB_COLORS, (VOID * *)& pBits, NULL, 0);
		nonAsMbrighten(&Bitmap, 30, temppBits);

		SelectObject(hdcMem, hBitmap);

		BitBlt(hdc, 0, 0, Bitmap.bmWidth, Bitmap.bmHeight,
			hdcMem, 0, 0, SRCCOPY);
		return 0;
	case WM_PAINT://what to do when a paint msg occurs
		hdc = BeginPaint(hwnd, &ps);//get a handle to a device context for drawing
		GetClientRect(hwnd, &rect);//define drawing area for clipping
		//GetObject(hBitmap, sizeof(BITMAP), &Bitmap);
		hdcMem = CreateCompatibleDC(hdc);
		SelectObject(hdcMem, hBitmap);

		BitBlt(hdc, 0, 0, Bitmap.bmWidth, Bitmap.bmHeight,
			hdcMem, 0, 0, SRCCOPY);


		EndPaint(hwnd, &ps);//release the device context
		return 0;

	case WM_DESTROY://how to handle a destroy (close window app) msg
		PostQuitMessage(0);
		return 0;
	}
	//return the message to windows for further processing
	return DefWindowProc(hwnd, message, wParam, lParam);
}
