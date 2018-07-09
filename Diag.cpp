// header file includes
#include <stdio.h>
#include <cstdlib>
#include <string>
#include <conio.h>
#include <process.h>
#include <windows.h>
#include "ftd2xx.h"
#include "Comm.h"
#include "Diag.h"
#include "Parser.h"

// macro definitions
#define SUCCESS 0x01
#define FAIL 0x00
#define MAXCMDLEN 50
#define DEFAULT_STATE 0x00
#define DIAGNOSTIC_STATE 0x01

// namespace includes
using namespace std;

// external global vars
extern HANDLE activeComm;
extern char guc_serialRXBuffer[64]; // holds incoming packets
extern char guc_serialTXBuffer[64]; // holds outgoing packets
extern int RXBufferPacketLocation;
extern bool endOfRXPacket;

extern data_item data_item_array[256]; // global array of data items (report payloads)
extern int next_data_item; 
 
 // global variables
extern packet packets_to_parse[20]; // contains unparsed packets
extern int numPackets; // number of packets in packets_to_parse
extern int numBytes;
 
// non volatile global vars
uchar state = DEFAULT_STATE; // state control variables
ulong rxbuffer_len; 
char* SP_array[4];

// volatile global vars
volatile bool getInput;

/*
// alternative input thread method
unsigned __stdcall inputThread(void* pArguments){
	
		bool comStatus;
		
		while(1){
		// recieve 1 byte
		comStatus = ucReadDevice(activeComm, 					// handle to device
						//guc_serialRXBuffer,  				// pointer to buffer that receives the data read from a file or device
						guc_serialRXBuffer[RXBufferPacketLocation++],
						1, 	// maximum number of bytes to be read
						&rxbuffer_len, 	// pointer to var that receives the number of bytes read
						NULL);
		
		//if( guc_serialRXBuffer[0] != '\0') 
		//if(comStatus) printf("%c", guc_serialRXBuffer[0]) ;
		if(comStatus) printf("%c", guc_serialRXBuffer[RXBufferPacketLocation]);
		
		//guc_serialRXBuffer[0] = '\0';
		Sleep(1); // prevent thread starving
	
	} // end while(1)
}*/

//*****************************************************************************************************************
// Name: vTransitionState
// Description: This function will transition the state machine from the current state to the passed in state
// variable. This function does not determine whether that is the correct thing to do, only performs the state
// transition. Error checking should be performed outside this function
//*****************************************************************************************************************
void vTransitionState(uchar newState){
	state = newState;
} // end vTransitionState

//*****************************************************************************************************************
// Name: vTransitionState
// Description: This function retrieves a packet from the serial connection
//*****************************************************************************************************************
void vGetPacket(){
	uchar Byte;
	bool comStatus;
	DWORD read;
		
	while(1){	
		// recieve 1 byte
		comStatus = ucReadDevice(activeComm, 					// handle to device
			&Byte,  				// pointer to buffer that receives the data read from a file or device
			1, 	// maximum number of bytes to be read
			&read, 	// pointer to var that receives the number of bytes read
			NULL);
				
	// TO DO:
	//  - keep track of packet
	//  - build as it comes in 
	
		// parse packet
		if(read){
			printf("%c", Byte);
			if(addByteToRXBuffer(guc_serialRXBuffer+RXBufferPacketLocation, Byte)) break;
		} // end if(read)
	} // end while(1)
} // end vGetPacket

//*****************************************************************************************************************
// Name: vCheckForDiagPacket
// Description: This method will continuously monitor the incoming bytestream from the serial port connection 
// looking for '0xDEDE' and '0xDDDD' packets which identify diagnostic sensor readings and report packets 
//*****************************************************************************************************************
void vCheckForDiagPacket(){
	// code 
} // end vCheckForDiagPacket


// - REMOVE THIS FUNCTION -
void skipMenu(){
	bool comStatus;
	DWORD read;
	uchar Byte;
	uchar count = 0;
	char last = 0;
	
	while(1){
		// recieve 1 byte
		comStatus = ucReadDevice(activeComm, 					// handle to device
			&Byte,  				// pointer to buffer that receives the data read from a file or device
			1, 	// maximum number of bytes to be read
			&read, 	// pointer to var that receives the number of bytes read
			NULL);
		
		if(read){
			printf("%c", Byte);
		} // end if(read)
			
		if(Byte == '\r' || Byte == '\n'){
			count++;
		} // end if(crlf)
		else{
			count = 0;
		} // end else
		last = Byte;
		
		if(count >= 6){
			break;
			
		} // end if(count >= 6)
	} // end while(1)
} // end skipMenu

//*****************************************************************************************************************
// Name: getKeyboardCMD
// Description: This function will retrieve a single byte from the user via the keyboard
//*****************************************************************************************************************
void getKeyboardCMD(){
	guc_serialTXBuffer[0] = _getch();
		
	//loop here
	ucWriteDevice(activeComm,					// handle to device
			guc_serialTXBuffer,				// pointer to buffer with data to write
			1,	// number of bytes to be written to device
			NULL, // pointer to var which tracks bytes written
			NULL);
			
}// end getKeyboardCMD

//*****************************************************************************************************************
// Name: vRunDiagSequence
// Description: This function will perform the following steps:
//	1) Attempt to open a com port of the user's choosing
//	2) Attempt to place the connected WiSARD into diagnostic mode
//	3) Listen for the diagnostic packets, denoted by '0xDEDE' and '0xDDDD' desination addresses
//	4) Parse incoming diagnostic packets, extract appropriate information and sensor readings
//	5) Call the appropriate level-up functions for each sensor reading
//	6) Construct a report from the data-set
//	7) Display the report for the user to read and save to a log file, if they choose
//*****************************************************************************************************************
void vRunDiagSequence(){
	HANDLE hThread; // com port handle
	unsigned threadID; // REMOVE IF UNUSED
	bool comStatus; // REMOVE IF UNUSED
	DWORD read; 
	
	printf("Running Diagnostic Sequence...\r\n");
	
	// begin running the diagnostic sequence
	
	// TO DO:
	// - run CP tests
	//		- test battery voltage
	//		- test buzzer
	//		- test LED
	//		- test SD card
	
	// - run SP sensor tests
	//		- test each SP
	//		- test each sensor on each SP
	
	// - run radio tests
	//		- test radio in discovery mode
	//		- test each radio in opmode
	
	// attempt to open com port
	if (activeComm == NULL){
		if (ucCommInit() == NULL){
			printf("UNABLE TO OPEN COM PORT\r\n");
		} // end if
		else{
			printf("COM PORT OPENED\r\n");
		} // end else
	}

	
	// place cp in diagnostic mode
	guc_serialTXBuffer[0] = 0x1B; // 'esc'
	guc_serialTXBuffer[1] = '1'; // '1'
	guc_serialTXBuffer[2] = '1'; // '1'
	guc_serialTXBuffer[3] = '1';
	guc_serialTXBuffer[4] = 0x0D; // 'enter'
	
	// send commands over serial to CP
	for(int i = 0; i < 5; i++){
		ucWriteDevice(activeComm,	// handle to device
			guc_serialTXBuffer + i,	// pointer to buffer with data to write
			1,	// number of bytes to be written to device
			NULL, // pointer to var which tracks bytes written
			NULL);
			Sleep(1000);
	} // end for()
	
	// prevent starving
	COMMTIMEOUTS ct;
	memset((void*) &ct, 0, sizeof(ct));
	ct.ReadIntervalTimeout = MAXDWORD;
	ct.ReadTotalTimeoutMultiplier = 0;
	ct.ReadTotalTimeoutConstant = 0;
	ct.WriteTotalTimeoutMultiplier = 20000L / 115200L;
	ct.WriteTotalTimeoutConstant = 0;
	SetCommTimeouts(activeComm, &ct);
	
	// Spawns new thread to handle input
	//hThread = (HANDLE) _beginthreadex(NULL, 0, &inputThread, NULL, 0, &threadID);
	printf(">");
	/*
	while(1){
	
		if(_kbhit()){
			// capture characters from user
			guc_serialTXBuffer[0] = _getch();
		
			//loop here
			ucWriteDevice(activeComm,					// handle to device
					guc_serialTXBuffer,				// pointer to buffer with data to write
					1,	// number of bytes to be written to device
					NULL, // pointer to var which tracks bytes written
					NULL);
		} // end if
		else{

			uchar Byte;
			
			// recieve 1 byte
			comStatus = ucReadDevice(activeComm, 					// handle to device
				&Byte,  				// pointer to buffer that receives the data read from a file or device
				1, 	// maximum number of bytes to be read
				&read, 	// pointer to var that receives the number of bytes read
				NULL);
			
// TO DO:
//  - keep track of packet
//  - build as it comes in 

			// parse packet
			if(read){
				printf("%c", Byte);
				if(addByteToRXBuffer(guc_serialRXBuffer+RXBufferPacketLocation, Byte)) break;
			} 

		} // end else

		
	} // end while(1)
	*/
	// waits for menu before proceeding to parse packets
	skipMenu();
	
	while(1){
		// if we are not ready to parse packets
		//if(state == DEFAULT_STATE){
			
		//}
		
		//else{
			vGetPacket();
			//parsePacket_sensors(guc_serialRXBuffer, rxbuffer_len); // add new byte to buffer
			resetByteBufferLocation(); // resets counter
			//getKeyboardCMD();
		//}
	}
	return;
} // end vRunDiagSequence

//*****************************************************************************************************************
// Name: vStreamSensorData
// Description: This method will display stream data from the wisard
//*****************************************************************************************************************
void vStreamSensorData(){
	printf("Running Data Streaming Sequence...\r\n");
	
	// if no comm port open...
	if(activeComm == NULL){
		// open comm port
	}
		
		// TO DO:
		// - add prompt/message for when com port is closed
		// - add selection menu for choosing an SP/channel to stream
		
	while(1){ // operational loop
		if(_kbhit())
			break; // return to main
	} // end while(1)
	
} // end vStreamSensorData

//*****************************************************************************************************************
// Name: asciiToNibble
// Description: converts a nibble to a uchar
//*****************************************************************************************************************
uchar asciiToNibble(char c){
	if(c >= '0' && c<= '9'){
		return (uchar)(c - '0');
	} // end if(digit)
	else if(c >= 'A' && c <= 'F'){
		return (uchar)(c - 'A' + 10);
	} // end if(hex A-F)
	else{
		return 0;
	} // end else
	
} // end asciiToNibble

//*****************************************************************************************************************
// Name: vInteractiveMode
// Description: This mode allows putty-like interaction with the device. User will be able to view WiSARD activity
// streaming across the serial connection from the WiSARD as well as interact with it via the keyboard. This mode
// will allow the user to place the WiSARD in diagnostic mode, but this mode does not have the functionality to
// parse the reports.
//*****************************************************************************************************************
void vInteractiveMode(){
	HANDLE hThread;
	unsigned threadID;
	bool comStatus;
	DWORD read;
	// parser vars
	int DEcount = 0;
	int DDcount = 0;
	int Dcount = 0;
	int nibbleCount = 0;
	bool parsing = false;
	bool lastPacket = false;
	bool allPackets = false;
	char SerialNumber[16];
	char Description[64];

	
	printf("Interactive Mode\r\n");
	
	// attempt to open com port
	if (activeComm == NULL){
		if (ucCommInit() == NULL){
			printf("UNABLE TO OPEN COM PORT\r\n");
			return;
		} // end if
		else{
			printf("COM PORT OPENED\r\n");
		} // end else
	} // end if

	
	// set com parameters to prevent I/O blocking
	COMMTIMEOUTS ct;
	memset((void*) &ct, 0, sizeof(ct));
	ct.ReadIntervalTimeout = MAXDWORD;
	ct.ReadTotalTimeoutMultiplier = 0;
	ct.ReadTotalTimeoutConstant = 0;
	ct.WriteTotalTimeoutMultiplier = 20000L / 115200L;
	ct.WriteTotalTimeoutConstant = 0;
	SetCommTimeouts(activeComm, &ct);
	
	// Grab the number of attached devices
	/*
	ftStatus = FT_ListDevices(&numDevs, NULL, FT_LIST_NUMBER_ONLY);
	if(ftStatus != FT_OK){
		printf("Status not ok!\r\n");
	}
	
	for(int currentDevice=0; currentDevice<numDevs; currentDevice++){
		ftStatus = FT_ListDevices((PVOID)currentDevice, Buffer, FT_LIST_BY_INDEX|FT_OPEN_BY_DESCRIPTION);
		//*portDescription = [NSString stringWithCString:Buffer encoding:NSASCIIStringEncoding];
		printf("%s\r\n",*Buffer);
	}*/
	
	// get Serial Number of FTDI chip
	/*
	ftStatus = dwGetFTDIInfo(SerialNumber, activeComm);
	if(ftStatus != FT_OK)
		printf("Uh oh, didn't work: %d\r\n", ftStatus);
	else
		printf("FTDI SERIAL NUMBER: %d\r\n", SerialNumber[1]);	
	*/
	/*
	while(1){
		getPacket();
		//parsePacket_sensors(guc_serialRXBuffer, rxbuffer_len); // add new byte to buffer
		resetByteBufferLocation(); // resets counter
		if(kbhit()) getKeyboardCMD();
	}
	return;*/
	
	// main interactive loop
	//while(1){
		while(1){
	
			if(_kbhit()){
				// capture characters from user
				guc_serialTXBuffer[0] = _getch();
			
				//loop here
				ucWriteDevice(activeComm,					// handle to device
						guc_serialTXBuffer,				// pointer to buffer with data to write
						1,	// number of bytes to be written to device
						NULL, // pointer to var which tracks bytes written
						NULL);
			} // end if
			else{
		
				uchar Byte;
				
				// recieve 1 byte
				comStatus = ucReadDevice(activeComm, 					// handle to device
					&Byte,  				// pointer to buffer that receives the data read from a file or device
					1, 	// maximum number of bytes to be read
					&read, 	// pointer to var that receives the number of bytes read
					NULL);
		
				// parse packet
				if(read){
					printf("%c", Byte);
					
					// global variables
					// packet packets_to_parse[20]; // contains unparsed packets
					// int numPackets; // number of packets in packets_to_parse
					// packets_to_parse[numPackets].contents[numBytes++] = Byte
					
					// begin packet capture
					if(parsing == false){
						if(Byte == 'D' && Dcount == 0){
							Dcount++;
						} // end if(byte)
						else if(Byte == 'D' && Dcount == 1){
							DDcount++;
							DEcount = 0;
							Dcount = 0;
						} // end if(byte)
						else if(Byte == 'E' && Dcount == 1){
							DEcount++;
							DDcount = 0;
							Dcount = 0;
						} // end if(byte)
						else{
							Dcount = 0;
							DDcount = 0;
							DEcount = 0;
						} // end if(byte)
						
						if(DDcount == 2){
							parsing = true;
							Dcount = 0;
							DDcount = 0;
							DEcount = 0;
							lastPacket = true;
							packets_to_parse[numPackets].contents[numBytes++] = 0xDD;
							packets_to_parse[numPackets].contents[numBytes++] = 0xDD;
						} // end if(DDcount)
						else if(DEcount == 2){
							parsing = true;
							Dcount = 0;
							DDcount = 0;
							DEcount = 0;
							packets_to_parse[numPackets].contents[numBytes++] = 0xDE;
							packets_to_parse[numPackets].contents[numBytes++] = 0xDE;
						} // end if(DDcount == 2)
					} // end if(parsing)
					else{
						if(Byte == '\r'){
							DDcount = 0;
							DEcount = 0;
							parsing = false;
							if(lastPacket){
								allPackets = true;							
							} // end if(lastPacket)
							packets_to_parse[numPackets].packetLen = numBytes;
							numPackets++;
							numBytes = 0;
							
							if(lastPacket) break;
							// put packet in array
						} // end if(Byte)
						else{
							// save byte to next spot in packet array
							if(++nibbleCount == 2){
								nibbleCount = 0;
								packets_to_parse[numPackets].contents[numBytes] <<= 4;
								packets_to_parse[numPackets].contents[numBytes] |= asciiToNibble(Byte);
								numBytes++;
							} // end if second half of byte
							else{
								packets_to_parse[numPackets].contents[numBytes] = asciiToNibble(Byte);
							} // if parsing and first half of byte
						} // end else
					} // end else
						
		
						
					/**
					if(parsing == false){
						if (DD & count == 0)
							DD count += 1
						else if(DE && DE count == 0)
							DE count = 1
						else if( 'DD'&&  DDcount == 1)
							write DD to first 2 spots in packet array
							last packet = true
							parsing = true
							ddcount = 0
						else if( DE && DE count == 1)
							write DE to first 2 spots in packet array
							parsing = true
							DE count = 0
						else
							DD count = 0
							DE count = 0
					}
					else 
						if(\n)
							set DDcount = 0
							DE count = 0
							parsing = false
							if(last packet == true)
								last packet = false
								all packets = true
							put packet in the array
						else
							save byte to next spot in packet array
						
					*/
				} // end if(read)
				
				/**
				if(all packets == true)
					call parse all packets
					all packets = false
					break
				*/
			} // end else
		} // end while(1)
			
		vParseAllPackets();
	//} // end while
} // end vInteractiveMode

