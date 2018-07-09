//*****************************************************************************************************************
// File: Comm.cpp
// Author: Michael Middleton
// Date: 06/09/2015
// Description: This file contains the various com related funcions used to open and close com ports, read and 
// write data to/from com ports, update and flush com buffers, and set specific baud rates
//*****************************************************************************************************************

// header file includes
#include <stdio.h>
#include <windows.h>
#include "ftd2xx.h"
#include "Comm.h"

// macro definitions
#define SUCCESS 0x01
#define FAIL 0x00
#define MAXCMDLEN 50

// external global vars
extern HANDLE activeComm; // pointer to active comm port 
extern char guc_serialRXBuffer[64]; // holds incoming packets
extern char guc_serialTXBuffer[64]; // holds outgoing packets
extern int RXBufferPacketLocation;
extern bool endOfRXPacket;
extern uchar exitCondition;

//*****************************************************************************************************************
// Name: ucOpenComPort
// Description: Opens a com port connection to a particular port governed by the parameters passed to the
// CreateFile() function in the windows api. A description of the parameters opening this port are as follows:
//			lpFileName - The name of the file or device to be created or opened
//			dwDesiredAccess - The requested access to the file or device, which can be summarized as read, write, 
//							  both, or zero. For this application, GENERIC_READ, and GENERIC_WRITE were used	
//			dwShareMode - The requested sharing mode of the file or device, which can be read, write, both, delete,
//						  all of these, or none. For this application, the zero parameter dictates that once the 
//						  port is opened, the com port cannot be shared and cannot be opened again until the 
//						  handle to the port is closed.
//			lpSecurityAttributes - A pointer to a structure that contains two separate but related data members; a
//								   security descriptor, and a boolean balue that determines whether the returned
//								   handle can be inherited by child processes. 
//			dwCreationDisposition - An action to take on a file or device that exists or does not exist. Since this
//									com port exists already and we are just trying to connect to it, OPEN_EXISTING
//			dwFlagsAndAttributes - This field handles flags which dictate the way in which this connection will
//								   perform, such as with encryption, archival, etc. FILE_ATTRIBUTE_NORMAL is how
//								   this particular connection will perform
//			hTemplateFile - 	   A valid handle to a tempate file with the GENERIC_READ access right. 
//			HANDLE - The return parameter of this function. If the function succeeds, the return is an open
//					 handle to the specified file, device, named pipe, or mail slot. If the function fails, the 
//					 return value is INVALID_HANDLE_EXCEPTION.
//*****************************************************************************************************************
HANDLE ucOpenComPort(char* portCom){
	HANDLE hCom = CreateFile(portCom, 						// filename
							GENERIC_READ|GENERIC_WRITE, 	// share mode
							0, 								// security attributes
							0, 								// creation disposition
							OPEN_EXISTING, 					// flags and attributes
							FILE_ATTRIBUTE_NORMAL, 			// template file
							//FILE_FLAG_OVERLAPPED,
							0);								//
	
	// if com port failed to open, return exception
	if(hCom == INVALID_HANDLE_VALUE){
		//printf("unable to open portCom\r\n");
		return NULL;
	}	// end if
	
	activeComm = hCom; // assigns new opened port to global active comm pointer
	return activeComm; // return successfully created handle
	
} // end ucOpenComPort

//*****************************************************************************************************************
// Name: vCloseComPort
// Description: Closes the com port of the specified handle
//*****************************************************************************************************************
void vCloseComPort(HANDLE serialHandle){
	CloseHandle(serialHandle); // close port
	activeComm = NULL; // clears reference to global active comm var
	
} // end vCloseComPort

//*****************************************************************************************************************
// Name: ucReadDevice
// Description: reads from a serial device
//*****************************************************************************************************************
uchar ucReadDevice(HANDLE hfile, 					// handle to device
					LPVOID lpBuffer,  				// pointer to buffer that receives the data read from a file or device
					DWORD nNumberOfBytesToRead, 	// maximum number of bytes to be read
					LPDWORD lpNumberOfBytesRead, 	// pointer to var that receives the number of bytes read
					LPOVERLAPPED lpOverlapped){ 	// pointer to overlapped structure
						
	// call to winapi ReadFile method
	return ReadFile(hfile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);

} // end ucReadDevice

//*****************************************************************************************************************
// Name: ucWriteDevice
// Description: writes to a serial device
//*****************************************************************************************************************
uchar ucWriteDevice(HANDLE hfile,					// handle to device
					LPCVOID lpBuffer,				// pointer to buffer with data to write
					DWORD nNumberOfBytesToWrite,	// number of bytes to be written to device
					LPDWORD lpNumberOfBytesWritten, // pointer to var which tracks bytes written
					LPOVERLAPPED lpOverlapped){     // pointer to overlapped structure
	
	// call to winapi ReadFile method
	bool retVal = WriteFile(hfile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
	FlushFileBuffers(hfile);
	//if(retVal)
	//	cout << "\nTRUETRUETRUE" << endl;
	//else{
	//	cout << "\nFAILFAILFAIL" << endl;
	//}
	return SUCCESS;						
} // end ucWriteDevice

//*****************************************************************************************************************
// Name: vUpdateSerialRXBuffer
// Description: reads packets from the serial into the global rx buffer
//*****************************************************************************************************************
void vUpdateSerialRXBuffer(HANDLE hfile){
	// pulls incoming contents into serial rx buffer	
	ucReadDevice(hfile, guc_serialRXBuffer, 1, NULL, NULL);
	
} // end vUpdateSerialRXBuffer

//*****************************************************************************************************************
// Name: vUpdateSerialTXBuffer
// Description: writes packets to the serial TX buffer to be sent over serial
//*****************************************************************************************************************
void vUpdateSerialTXBuffer(HANDLE hfile){
	// pushes contents into serial Tx buffer	
	ucWriteDevice(hfile, guc_serialTXBuffer, 1, NULL, NULL);
	
} // end vUpdateSerialTXBuffer

//*****************************************************************************************************************
// Name: vFlushSerialRXBuffer
// Description: zeroes out the RX Buffer
//*****************************************************************************************************************
void vFlushSerialRXBuffer(){
	for(int i = 0; i < sizeof(guc_serialRXBuffer); i++){ // loop through each index in the buffer
		guc_serialRXBuffer[i] = 0x00; // zero out its contents
	} // end for()
	
} // end vFlushSerialRXBuffer

//*****************************************************************************************************************
// Name: vFlushSerialTXBuffer
// Description: zeroes out the TX Buffer
//*****************************************************************************************************************
void vFlushSerialTXBuffer(){
	for(int i = 0; i < sizeof(guc_serialTXBuffer); i++){ // loop through each index in the buffer
		guc_serialTXBuffer[i] = 'd'; // zero out its contents
	} // end for()
	
} // end vFlushSerialRXBuffer

//*****************************************************************************************************************
// Name: vChangeBaudRate
// Description: updates the comm settings structure with a new baud rate
//*****************************************************************************************************************
void vChangeBaudRate(HANDLE hfile, DWORD baud){
	DCB settings; // creates a struct variable
	GetCommState(hfile, &settings); // populates struct with default settings
	settings.BaudRate = baud; 		// updates struct with new baud rate
	settings.ByteSize = 8;
	settings.StopBits = ONESTOPBIT;
	settings.Parity = ODDPARITY;
} // end vChangeBaudRate

//*****************************************************************************************************************
// Name: vChangeBaudRate115200
// Description: updates the comm settings structure with 115200 baud
//*****************************************************************************************************************
void vChangeBaudRate115200(HANDLE hfile){
	DCB settings; // creates a struct variable
	GetCommState(hfile, &settings); 		// populates struct with default settings
	settings.BaudRate = CBR_115200; 		// updates struct with new baud rate
	settings.ByteSize = 8;
	settings.StopBits = ODDPARITY;
	settings.Parity = 1;
} // end vChangeBaudRate

//*****************************************************************************************************************
// Name: ucCommInit
// Description: initializes serial comm
//*****************************************************************************************************************
uchar ucCommInit(){
	//ucOpenSpecifiedComPort(); // open user specified com port
	//ucOpenComPort("\\\\.\\COM10"); // note: temporary
	char comPortString[20];
	int commandVal;
	
	printf("Which com port would you like to open? ");
	scanf("%d", &commandVal);
	sprintf(comPortString, "\\\\.\\COM%d", commandVal);
	
	ucOpenComPort(comPortString);
	vChangeBaudRate115200(activeComm); // set default baud 
	// to do: add intelligent comm port opening
	return SUCCESS;
	
} // end ucCommInit

//*****************************************************************************************************************
// Name: dwGetFTDIInfo()
// Description: gets the ftdi chip info
//*****************************************************************************************************************
FT_STATUS dwGetFTDIInfo(char* out, FT_HANDLE ftHandle){
	//FT_HANDLE ftHandle;
	FT_DEVICE ftDevice;
	FT_STATUS ftStatus;
	DWORD deviceID;
	char SerialNumber[16];
	char Description[64];
	
	ftStatus = FT_Open(0, &ftHandle);
	if(ftStatus != FT_OK){
		// FT_Open failed
		return ftStatus;
	}
	
	ftStatus = FT_GetDeviceInfo(
		ftHandle,
		&ftDevice,
		&deviceID,
		SerialNumber,
		Description,
		NULL
	);
	
	FT_Close(ftHandle);
	
	// write serial number to pointer location
	for(int i=0; i<16; i++){
		out[i] = SerialNumber[i];
	}
	
	return ftStatus;
}


