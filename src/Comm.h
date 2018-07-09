#ifndef COMM_H
#define COMM_H

// namespaces and typedefs
using namespace std;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned char uchar;

HANDLE ucOpenComPort(char*);
void vCloseComPort(HANDLE);
uchar ucReadDevice(HANDLE, LPVOID, DWORD, LPDWORD, LPOVERLAPPED);
uchar ucWriteDevice(HANDLE, LPCVOID, DWORD, LPDWORD, LPOVERLAPPED);			
void vUpdateSerialRXBuffer(HANDLE);
void vUpdateSerialTXBuffer(HANDLE);
void vFlushSerialRXBuffer();
void vFlushSerialTXBuffer();
void vChangeBaudRate(HANDLE hfile, DWORD baud);
void vChangeBaudRate115200(HANDLE hfile);
uchar ucCommInit();
FT_STATUS dwGetFTDIInfo(char* out, FT_HANDLE port);

#endif
