#ifndef PARSE_H
#define PARSE_H

#define ITEM_TYPE_SENSOR 1
#define ITEM_TYPE_BATT 2
#define ITEM_TYPE_ACTUATOR 3

// type definitions
typedef unsigned char uchar; 	// hex: 00
typedef unsigned int uint; 		// hex: 0000
typedef unsigned long ulong; 	// hex: 0000 0000

// structure for report items
typedef struct{
	uchar procID;
	uchar data_gen_id;
	ulong time;
	uchar rep_len;
}report;

// structure for data elements
typedef struct{
	uchar DEID;
	uchar de_len;
	uchar de_ver;
	uchar proc_ID;
	ulong time_stamp;
	uchar* payload;
}data_element;

// struct definition for message items
typedef struct{
	uint destination; // network destination address
	uint source; // network source address
} network;

// struct definition for data items
typedef struct{
	uchar DEID;
	uchar de_ver;
	uchar proc_id; 
	ulong proc_time;
	uchar data_gen_id;
	uchar transducer_id;
	uchar data_len;
	uchar item_type;
	uchar data[8];
}data_item;

// raw packet to pull off of buffer
typedef struct{
	char contents[64];
	ulong packetLen;
}packet;

// function declarations 
uchar nextDataElement(char*, uchar , uchar);
uchar nextItem(char* , uchar , uchar , uchar , uchar);
uchar parsePacket_sensors(char* , ulong);
uchar parsePacket_batt();
bool addByteToRXBuffer(char*, uchar);	
void resetByteBufferLocation(void);
void vParseAllPackets(void);

// move this to another file?
bool writeReportToFile(HANDLE, LPCVOID, report);

#endif
