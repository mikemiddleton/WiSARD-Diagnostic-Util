//*****************************************************************************************************************
// File: Parser.cpp
// Author: Michael Middleton
// Date: 06/09/2015
// Description: This file contains the functions which parse packets from CP serial connection and extract data
//*****************************************************************************************************************

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
#include "LevelUp.h"
#include "Parser.h"

// macro definitions
#define SUCCESS 0x01
#define FAIL 0x00

// namespace uses
using namespace std;

// external global variables
extern HANDLE activeComm;
extern char guc_serialRXBuffer[64]; // holds incoming packets
extern char guc_serialTXBuffer[64]; // holds outgoing packets 
extern int RXBufferPacketLocation;
extern bool endOfRXPacket;

// global variables
data_item data_item_array[256]; // global array of data items (report payloads)

data_element data_element_array[256]; // global array of data elements
packet packets_to_parse[256]; // contains unparsed packets
int numPackets; // number of packets in packets_to_parse
int numBytes = 0; // number of bytes in packet
int numDataElements = 0; // number of data elements in data_element_array

int next_data_item = 0;
int numItems = 0;
int packet_len = 0;

// flags to indicate whether a data element
// is from a test or a fluke measurement for
// calibration constants
int all_SPST_data_recieved[4];


// final report struct
typedef struct{
	// name (ex: EMTY, STM, ST, CMSTM, ect.)
	char SP1_name[8]; // 0-7 
	char SP2_name[8]; // 8-15
	char SP3_name[8]; // 16-23
	char SP4_name[8]; // 24-31
	uchar types[4];
	// SUCCESS -> 0x01  FAIL -> 0x00
	uchar LED_test; 	// 32
	uchar radio_disc_test; // 33 
	uchar radio_op_test; // 34
	uchar buzzer_test; // 35
	uchar valve_test; // 36
	uchar SD_test;	// 37
	uchar FRAM_test; // 38
	int SP1_data[28]; // to accomadate SP-ST calibration constants
	int SP2_data[28]; // to accomadate SP-ST calibration constants
	int SP3_data[28]; // to accomadate SP-ST calibration constants
	int SP4_data[28]; // to accomadate SP-ST calibration constants
	double SP_calibration_constants[4]; // [ 0c1, 25c1, 0c2, 25c2, ... 0c4, 25c4] <- format
	int batt_reading; // in mV
	int mcu_temp; // in degrees C
	uchar diagMode; // 0x00 is deployment, 0x01 is field (start of new stuff)
	uchar SP1_types[4]; // 0x00 or 0x52 mean empty, 0x78 - 5TM, 0x7A - 5TE, 0x6C - MPS-6
	uchar SP2_types[4];
	uchar SP3_types[4];
	uchar SP4_types[4];
} reportSummary;

//*****************************************************************************************************************
// Name: findCalibrationConstants
// Description: Uses 2-point calibration (0C and 25C source voltages) to obtain a bias average
//*****************************************************************************************************************
void findCalibrationConstants(reportSummary* rptSum){
	double bias;
	uint SPNum; // SP loop counter
	char* SPName = 0; // pointer to SP name
	int* SPData = 0; // pointer to SP data

	// Loop through all SPs
	for (SPNum = 0; SPNum < 4; SPNum++){

		// generalize names and data for easy iteration
		if (SPNum == 0){
			SPName = rptSum->SP1_name;
			SPData = rptSum->SP1_data;
		}
		else if (SPNum == 1){
			SPName = rptSum->SP2_name;
			SPData = rptSum->SP2_data;
		}
		else if (SPNum == 2){
			SPName = rptSum->SP3_name;
			SPData = rptSum->SP3_data;
		}
		else if (SPNum == 3){
			SPName = rptSum->SP4_name;
			SPData = rptSum->SP4_data;
		} // end else if

		// check if SP is an SP-ST
		if (strcmp("ST", SPName) == 0){
			// initialize counter and variables
			int dataPointCount = 8;
			int avg0c = 0;
			int avg25c = 0;

			// while there are more values to process
			while (dataPointCount < 18){
				avg0c += SPData[dataPointCount];
				avg25c += SPData[dataPointCount + 10];
				dataPointCount++;
			} // end while more datapoints

			// calculate average for each
			avg0c /= 10;
			avg25c /= 10;

			double final0 = (double)(avg0c*10);
			double final25 = (double)(avg25c*10);
			final0 /= 1000;
			final25 /= 1000;

			// calculate how far off from 25 it is
			final25 -= 25;

			// average 2-point offsets
			bias = (final0 + final25) / 2;

			// cast and get precision
			//bias = (double)(bias * 100);
			//bias /= 100;

			rptSum->SP_calibration_constants[SPNum] = bias;
		} // end if(ST)
		
		// if not an SP-ST, continue to next SP
		else
			// 0xDE is used to denote no ST in this slot
			rptSum->SP_calibration_constants[SPNum] = (double) 0xDE;

	} // end for each SP

} // end findCalibrationConstants

//*****************************************************************************************************************
// Name: writeReportToFile
// Description: writes the contents of a report struct to a log file using the windows API. Assumes handle to 
// a file that already exists, code which calls this function will need to create file if it does not exist.
//*****************************************************************************************************************
bool writeReportToFile(HANDLE hfile, LPCVOID lpBuffer, report rpt){
	bool retVal = false; // assumes failure

	// open file

	// write system time to file

	// loop through report and write to file

	// close file 

	// return error code
	return retVal;

} // end writeReportToFile

//*****************************************************************************************************************
// Name: displayReportSummary - refactored 
// Description: displays the contents of a report summary
//*****************************************************************************************************************
void displayReportSummary(reportSummary rptSum){
	double temp, temp2;
	HANDLE hCon = GetStdHandle(STD_OUTPUT_HANDLE);
	WORD defaultTxt;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(hCon, &csbi);
	defaultTxt = csbi.wAttributes;
	
	uint SPNum; // SP loop counter
	uint TNum; // Transducer loop counter
	char* SPName = 0; // pointer to SP name
	int* SPData = 0; // pointer to SP data
	
	printf("\r\n\r\n------Diagnostic Report Summary------\r\n");

	// Loop through all SPs
	for(SPNum = 0; SPNum < 4; SPNum++){
		
		// generalize names and data for easy iteration
		if(SPNum == 0){
			SPName = rptSum.SP1_name;
			SPData = rptSum.SP1_data;	
		} 
		else if(SPNum == 1){
			SPName = rptSum.SP2_name;
			SPData = rptSum.SP2_data;	
		} 
		else if(SPNum == 2){
			SPName = rptSum.SP3_name;
			SPData = rptSum.SP3_data;
		}
		else if(SPNum == 3){
			SPName = rptSum.SP4_name;
			SPData = rptSum.SP4_data;
		} 
		
		// Print SP name
		printf("\r\nSP slot %d: %s\r\n", SPNum + 1, SPName);
		
		// loop through all transducers on SP
		for(TNum = 0; TNum < 4; TNum++){

			// display transducer number
			printf("Transducer %d: \r\n", TNum + 1);
			
			// switch on current SP type (STM, CMSTM, etc)
			switch(rptSum.types[SPNum]){
				// no type
				case 0x00:
					printf("EMPTY\r\n");
					break;
				
				// STM type
				case 0x01:
					if(SPData[2*TNum] == 0 && SPData[2*TNum + 1] == 0) printf("EMPTY\r\n\r\n");
					else{
						printf("Wet Sensor Readings:\r\n");
						printf("Soil Moisture: %.2f kPa\r\n", (double)(SPData[2*TNum])/100);
						printf("Soil Temp: %.2f C\r\n\r\n", (double)(SPData[2*TNum + 1])/100);
						
						printf("Dry Sensor Readings:\r\n");
						printf("Soil Moisture: %.2f kPa\r\n", (double)(SPData[2*TNum+8])/100);
						printf("Soil Temp: %.2f C\r\n\r\n", (double)(SPData[2*TNum+9])/100);
					}
					break;

				// CMSTM type
				case 0x02:
				if(SPData[2*TNum] == 0 && SPData[2*TNum + 1] == 0) printf("EMPTY\r\n\r\n");
					else{
						if (TNum == 2)
							printf("Valve-slot\r\n\r\n");

						else if (TNum == 3)
							printf("EMPTY\r\n\r\n");

						else{
							printf("Wet Sensor Readings:\r\n");
							printf("Soil Moisture: %.2f kPa\r\n", SPData[2 * TNum]);
							printf("Soil Temp: %.2f C\r\n\r\n", SPData[2 * TNum + 1]);

							printf("Dry Sensor Readings:\r\n");
							printf("Soil Moisture: %.2f kPa\r\n", (double)(SPData[2 * TNum + 8]) / 100);
							printf("Soil Temp: %.2f C\r\n\r\n", (double)(SPData[2 * TNum + 9]) / 100);
						}
					}
					break;

				// ST type
				case 0x03:
					if(SPData[TNum] == 0) printf("EMPTY\r\n\r\n");
					else{
						temp = (double)(SPData[TNum])/100;
						temp2 = (double)(SPData[TNum+4])/100;
						//if(temp > 30 || temp < 10) printf("EMPTY\r\n\r\n");
						if(temp == 0) printf("EMPTY\r\n\r\n"); // Placeholder, replace with previous statement after testing
						else {
							// display temps
							printf("Open-Air Reading\r\n");
							printf("Air Temp: %.2f C\r\n", temp);
							printf("Warm-Air Reading\r\n");
							printf("Air Temp: %.2f C\r\n\r\n", temp2);

							if (TNum == 0)
								// display corrected calibration bias
								printf("Corrected calibration bias: %.2f C\r\n\r\n", (-1)*rptSum.SP_calibration_constants[SPNum]);
						}			
					}
					break;

				// SL type
				case 0x04:
					if(SPData[TNum] == 0) printf("EMPTY\r\n\r\n");
					else
						printf("PAR Reading: %.2d\r\n", SPData[TNum]);
					break;
			}
		}	
	}
	
	// print battery voltage reading
	printf("\r\nBattery Voltage: %dmV\r\n", rptSum.batt_reading);
	if (rptSum.batt_reading < 4500){
		if (rptSum.diagMode == 0) // if deployment mode
			printf("Battery voltage too low!\r\nFull batteries should be above 4.5V - replace with fresh batteries before deployment!\r\n\r\n");
		else // if field test mode
			if (rptSum.batt_reading > 2800) // if we aren't at the battery cutoff point
				printf("Batteries are at an adequate voltage - they should be changed when they fall below 2.8V\r\n\r\n");
			else // if we are at or below the cutoff point, inform user that the batteries should be changed
				printf("Batteries are getting close to depletion, they should be changed as soon as possilbe\r\n\r\n");
	}
	else{
		printf("Battery voltage is good. These batteries are fit for deployment\r\n\r\n");
	}
	
	// print MCU temp
	printf("Internal MCU Temp: %.2fC\r\n", (float)(rptSum.mcu_temp)/100);
	// if below 0 C
	if (rptSum.mcu_temp / 100 < 0){
		printf("Temperature reads below freezing, sensor appears to be malfunctioning\r\n\r\n");
	}
	// if above 50 C
	else if (rptSum.mcu_temp / 100 > 50){
		printf("MCU sensor is reading a very hot temperature, sensor appears to be malfunctioning\r\n\r\n");
	}
	// if between 0 C and 50 C
	else{
		printf("Temperature sensor is working properly\r\n\r\n");
	}
	
	// LED test results
	if (rptSum.LED_test){
		SetConsoleTextAttribute(hCon, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | BACKGROUND_GREEN);
		printf("CPU LED Test:              Passed\n");
		//SetConsoleTextAttribute(hCon, 7 & ~BACKGROUND_GREEN);
		FlushConsoleInputBuffer(hCon);
		//SetConsoleTextAttribute(hCon, 7);
	}
	else {
		SetConsoleTextAttribute(hCon, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | BACKGROUND_RED);
		printf("CPU LED Test:              Failed\n");
		//SetConsoleTextAttribute(hCon, 7 & ~BACKGROUND_RED);
		FlushConsoleInputBuffer(hCon);
		//SetConsoleTextAttribute(hCon, 7);
	}
	
	// Radio discovery mode test
	if (rptSum.radio_disc_test){
		SetConsoleTextAttribute(hCon, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | BACKGROUND_GREEN);
		printf("Radio Discovery Mode Test: Passed\r\n");
		SetConsoleTextAttribute(hCon, 7);
	}
	else {
		SetConsoleTextAttribute(hCon, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | BACKGROUND_RED);
		printf("Radio Discovery Mode Test: Failed\r\n");
		SetConsoleTextAttribute(hCon, 7);
	}
	
	// Radio opmode test
	if (rptSum.radio_op_test){
		SetConsoleTextAttribute(hCon, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | BACKGROUND_GREEN);
		printf("Radio Op-Mode Test:        Passed\r\n");
		SetConsoleTextAttribute(hCon, 7);
	}
	else {
		SetConsoleTextAttribute(hCon, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | BACKGROUND_RED);
		printf("Radio Discovery Mode Test:        Failed\r\n");
		SetConsoleTextAttribute(hCon, 7);
	}
	
	// Buzzer test
	if (rptSum.buzzer_test){
		SetConsoleTextAttribute(hCon, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | BACKGROUND_GREEN);
		printf("Buzzer Test:               Passed\r\n");
		SetConsoleTextAttribute(hCon, 7);
	}
	else {
		SetConsoleTextAttribute(hCon, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | BACKGROUND_RED);
		printf("Buzzer Test:           Failed\r\n");
		SetConsoleTextAttribute(hCon, 7);
	}
	
	// Valve test
	if (rptSum.valve_test){
		SetConsoleTextAttribute(hCon, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | BACKGROUND_GREEN);
		printf("Valve Test:                Passed\r\n");
		SetConsoleTextAttribute(hCon, 7);
	}
	else {
		SetConsoleTextAttribute(hCon, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | BACKGROUND_RED);
		printf("Valve Test:                Failed\r\n");
		SetConsoleTextAttribute(hCon, 7);
	}
	
	// SD Card Test
	if (rptSum.SD_test){
		SetConsoleTextAttribute(hCon, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | BACKGROUND_GREEN);
		printf("SD Card Test:              Passed\r\n");
		SetConsoleTextAttribute(hCon, 7);
	}
	else {
		SetConsoleTextAttribute(hCon, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | BACKGROUND_RED);
		printf("SD Card Test:              Failed\r\n");
		SetConsoleTextAttribute(hCon, 7);
	}
	
	// FRAM Test
	if (rptSum.FRAM_test){
		SetConsoleTextAttribute(hCon, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | BACKGROUND_GREEN);
		printf("FRAM Test:                 Passed\r\n");
		SetConsoleTextAttribute(hCon, 7);
	}
	else {
		SetConsoleTextAttribute(hCon, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | BACKGROUND_RED);
		printf("FRAM Test:                 Failed\r\n");
		SetConsoleTextAttribute(hCon, 7);
	}
	
	printf("\r\n------End of Report------\r\n");
}

//*****************************************************************************************************************
// Name: displayReportSummary - gross ugly nasty ass function; too much copy pasta
// Description: displays the contents of a report summary
//*****************************************************************************************************************
/*
void displayReportSummary(reportSummary rptSum){
	double temp;
	HANDLE hCon = GetStdHandle(STD_OUTPUT_HANDLE);
	WORD defaultTxt;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(hCon, &csbi);
	defaultTxt = csbi.wAttributes;

	printf("\r\n\r\n------Diagnostic Report Summary------\r\n");
	printf("\r\nSP slot 1: %s\r\n", rptSum.SP1_name);
	// print transducer 1
	printf("Transducer 1: \r\n");
	switch(rptSum.types[0]){
		case 0x00:
			printf("EMPTY\r\n");
			break;
		case 0x01:
			if(rptSum.SP1_data[0] == 0 && rptSum.SP1_data[1] == 0) printf("EMPTY\r\n");
			else{
				printf("Soil Moisture: %.2f kPa\r\n", (double)(rptSum.SP1_data[0])/100);
				printf("Soil Temp: %.2f C\r\n", (double)(rptSum.SP1_data[1])/100);	
			}
			break;
		case 0x02:
			if(rptSum.SP1_data[0] == 0 && rptSum.SP1_data[1] == 0) printf("EMPTY\r\n");
			else{
				printf("Soil Moisture: %.2f kPa\r\n", rptSum.SP1_data[0]);
				printf("Soil Temp: %.2f C\r\n", rptSum.SP1_data[1]);	
			}
			break;
		case 0x03:
			if(rptSum.SP1_data[0] == 0) printf("EMPTY\r\n");
			else{
				temp = (double)(rptSum.SP1_data[0])/100;
				if(temp > 60 || temp < -50) printf("EMPTY\r\n");
				else printf("Air Temp: %.2f C\r\n", temp);				
			}
			break;
		case 0x04:
			if(rptSum.SP1_data[0] == 0) printf("EMPTY\r\n");
			else
				printf("PAR Reading: %.2d\r\n", rptSum.SP1_data[0]);
			break;
	}
	// print transducer 2
	printf("\r\nTransducer 2: \r\n");
	switch(rptSum.types[0]){
		case 0x00:
			printf("EMPTY\r\n");
			break;
		case 0x01:
			if(rptSum.SP1_data[2] == 0 && rptSum.SP1_data[3] == 0) printf("EMPTY\r\n");
			else{
				printf("Soil Moisture: %.2f kPa\r\n", (double)(rptSum.SP1_data[2])/100);
				printf("Soil Temp: %.2f C\r\n", (double)(rptSum.SP1_data[3])/100);				
			}
			break;
		case 0x02:
			if(rptSum.SP1_data[2] == 0 && rptSum.SP1_data[3] == 0) printf("EMPTY\r\n");
			else{
				printf("Soil Moisture: %.2f kPa\r\n", (double)(rptSum.SP1_data[2])/100);
				printf("Soil Temp: %.2f C\r\n", (double)(rptSum.SP1_data[3])/100);				
			}
			break;
		case 0x03:
			if(rptSum.SP1_data[1] == 0) printf("EMPTY\r\n");
			else{
				temp = (double)(rptSum.SP1_data[1])/100;
				if(temp > 60 || temp < -50) printf("EMPTY\r\n");
				else printf("Air Temp: %.2f C\r\n", temp);
			}
			break;
		case 0x04:
			if(rptSum.SP1_data[1] == 0) printf("EMPTY\r\n");
			else
				printf("PAR Reading: %.2d\r\n", rptSum.SP1_data[1]);
			break;
	}	
	// print transducer 3
	printf("\r\nTransducer 3: \r\n");
	switch(rptSum.types[0]){
		case 0x00:
			printf("EMPTY\r\n");
			break;
		case 0x01:
			if(rptSum.SP1_data[4] == 0 && rptSum.SP1_data[5] == 0) printf("EMPTY\r\n");
			else{
				printf("Soil Moisture: %.2f kPa\r\n", (double)(rptSum.SP1_data[4])/100);
				printf("Soil Temp: %.2f C\r\n", (double)(rptSum.SP1_data[5])/100);				
			}
			break;
		case 0x02:
			//printf("Soil Moisture: %d\r\n", rptSum.SP1_data[4]);
			//printf("Soil Temp: %dC\r\n", rptSum.SP1_data[5]);
			if(rptSum.valve_test == 1) printf("Valve: Functioning\r\n");
			else printf("Valve: Missing or Malfunctioning\r\n");
			break;
		case 0x03:
			if(rptSum.SP1_data[2] == 0) printf("EMPTY\r\n");
			else{
				temp = (double)(rptSum.SP1_data[2])/100;
				if(temp > 60 || temp < -50) printf("EMPTY\r\n");
				else printf("Air Temp: %.2f C\r\n", temp);
			}
			break;
		case 0x04:
			if(rptSum.SP1_data[2] == 0) printf("EMPTY\r\n");
			else
				printf("PAR Reading: %.2d\r\n", rptSum.SP1_data[2]);
			break;
	}		
	// print transducer 4
	printf("\r\nTransducer 4: \r\n");
	switch(rptSum.types[0]){
		case 0x00:
			printf("EMPTY\r\n");
			break;
		case 0x01:
			if(rptSum.SP1_data[6] == 0 && rptSum.SP1_data[7] == 0) printf("EMPTY\r\n");
			else{
				printf("Soil Moisture: %.2f kPa\r\n", (double)(rptSum.SP1_data[6])/100);
				printf("Soil Temp: %.2f C\r\n", (double)(rptSum.SP1_data[7])/100);				
			}
			break;
		case 0x02:
			if(rptSum.SP1_data[6] == 0 && rptSum.SP1_data[7] == 0) printf("EMPTY\r\n");
			else{
				printf("Soil Moisture: %.2f kPa\r\n", (double)(rptSum.SP1_data[6])/100);
				printf("Soil Temp: %.2f C\r\n", (double)(rptSum.SP1_data[7])/100);				
			}
			break;
		case 0x03:
			if(rptSum.SP1_data[3] == 0) printf("EMPTY\r\n");
			else{
				temp = (double)(rptSum.SP1_data[3])/100;
				if(temp > 60 || temp < -50) printf("EMPTY\r\n");
				else printf("Air Temp: %.2f C\r\n", temp);
			}
			break;
		case 0x04:
			if(rptSum.SP1_data[3] == 0) printf("EMPTY\r\n");
			else
				printf("PAR Reading: %.2d\r\n", rptSum.SP1_data[3]);
			break;
	}	
	
	printf("\r\nSP slot 2: %s\r\n", rptSum.SP2_name);
	// print transducer 1
	printf("Transducer 1: \r\n");
	switch(rptSum.types[1]){
		case 0x00:
			printf("EMPTY\r\n");
			break;
		case 0x01:
			if(rptSum.SP2_data[0] == 0 && rptSum.SP2_data[1] == 0) printf("EMPTY\r\n");
			else{
				printf("Soil Moisture: %.2f kPa\r\n", (double)(rptSum.SP2_data[0])/100);
				printf("Soil Temp: %.2f C\r\n", (double)(rptSum.SP2_data[1])/100);				
			}
			break;
		case 0x02:
			if(rptSum.SP2_data[0] == 0 && rptSum.SP2_data[1] == 0) printf("EMPTY\r\n");
			else{
				printf("Soil Moisture: %.2f kPa\r\n", (double)(rptSum.SP2_data[0])/100);
				printf("Soil Temp: %.2f C\r\n", (double)(rptSum.SP2_data[1])/100);				
			}
			break;
		case 0x03:
			if(rptSum.SP2_data[0] == 0) printf("EMPTYr\n");
			else{
				temp = (double)(rptSum.SP2_data[0])/100;
				if(temp > 60 || temp < -50) printf("EMPTY\r\n");
				else printf("Air Temp: %.2f C\r\n", temp);
			}
			break;
		case 0x04:
			if(rptSum.SP2_data[0] == 0) printf("EMPTY\r\n");
			else
				printf("PAR Reading: %.2d\r\n", rptSum.SP2_data[0]);
			break;
	}
	// print transducer 2
	printf("\r\nTransducer 2: \r\n");
	switch(rptSum.types[1]){
		case 0x00:
			printf("EMPTY\r\n");
			break;
		case 0x01:
			if(rptSum.SP2_data[2] == 0 && rptSum.SP2_data[3] == 0) printf("EMPTY\r\n");
			else{
				printf("Soil Moisture: %.2f kPa\r\n", (double)(rptSum.SP2_data[2])/100);
				printf("Soil Temp: %.2f C\r\n", (double)(rptSum.SP2_data[3])/100);	
			}
			break;
		case 0x02:
			if(rptSum.SP2_data[2] == 0 && rptSum.SP2_data[3] == 0) printf("EMPTY\r\n");
			else{
				printf("Soil Moisture: %.2f kPa\r\n", (double)(rptSum.SP2_data[2])/100);
				printf("Soil Temp: %.2f C\r\n", (double)(rptSum.SP2_data[3])/100);				
			}
			break;
		case 0x03:
			if(rptSum.SP2_data[1] == 0) printf("EMPTY\r\n");
			else{
				temp = (double)(rptSum.SP2_data[1])/100;
				if(temp > 60 || temp < -50) printf("EMTPY\r\n");
				else printf("Air Temp: %.2f C\r\n", temp);
			}
			break;
		case 0x04:
			if(rptSum.SP2_data[1] == 0) printf("EMPTY\r\n");
			else
				printf("PAR Reading: %.2d\r\n", rptSum.SP2_data[1]);
			break;
	}	
	// print transducer 3
	printf("\r\nTransducer 3: \r\n");
	switch(rptSum.types[1]){
		case 0x00:
			printf("EMPTY\r\n");
			break;
		case 0x01:
			if(rptSum.SP2_data[4] == 0 && rptSum.SP2_data[5] == 0) printf("EMPTY\r\n");
			else{
				printf("Soil Moisture: %.2f kPa\r\n", (double)(rptSum.SP2_data[4])/100);
				printf("Soil Temp: %.2f C\r\n", (double)(rptSum.SP2_data[5])/100);				
			}
			break;
		case 0x02:
			//printf("Soil Moisture: %d\r\n", rptSum.SP2_data[4]);
			//printf("Soil Temp: %dC\r\n", rptSum.SP2_data[5]);
			if(rptSum.valve_test == 1) printf("Valve: Functioning\r\n");
			else printf("Valve: Missing or Malfunctioning\r\n");
			break;
		case 0x03:
			if(rptSum.SP2_data[2] == 0) printf("EMPTY\r\n");
			else{
				temp = (double)(rptSum.SP2_data[2])/100;
				if(temp > 60 || temp < -50) printf("EMPTY\r\n");
				else printf("Air Temp: %.2f C\r\n", temp);
			}
			break;
		case 0x04:
			if(rptSum.SP2_data[2] == 0) printf("EMPTY\r\n");
			else
				printf("PAR Reading: %.2f\r\n", rptSum.SP2_data[2]);
			break;
	}		
	// print transducer 4
		printf("\r\nTransducer 4: \r\n");
	switch(rptSum.types[1]){
		case 0x00:
			printf("EMPTY\r\n");
			break;
		case 0x01:
			if(rptSum.SP2_data[6] == 0 && rptSum.SP2_data[7] == 0) printf("EMPTY\r\n");
			else{
				printf("Soil Moisture: %.2f kPa\r\n", (double)(rptSum.SP2_data[6])/100);
				printf("Soil Temp: %.2f C\r\n", (double)(rptSum.SP2_data[7])/100);				
			}
			break;
		case 0x02:
			if(rptSum.SP2_data[6] == 0 && rptSum.SP2_data[7] == 0) printf("EMPTY\r\n");
			else{
				printf("Soil Moisture: %.2f kPa\r\n", (double)(rptSum.SP2_data[6])/100);
				printf("Soil Temp: %.2f C\r\n", (double)(rptSum.SP2_data[7])/100);				
			}
			break;
		case 0x03:
			if(rptSum.SP2_data[3] == 0) printf("EMPTY\r\n");
			else{
				temp = (double)(rptSum.SP2_data[3])/100;
				if(temp > 60 || temp < -50) printf("EMPTY\r\n");
				else printf("Air Temp: %.2f C\r\n", temp);
			}
			break;
		case 0x04:
			if(rptSum.SP2_data[3] == 0) printf("EMPTY\r\n");
			else
				printf("PAR Reading: %.2d\r\n", rptSum.SP2_data[3]);
			break;
	}	
	
	printf("\r\nSP slot 3: %s\r\n", rptSum.SP3_name);
	// print transducer 1
	printf("Transducer 1: \r\n");
	switch(rptSum.types[2]){
		case 0x00:
			printf("EMPTY\r\n");
			break;
		case 0x01:
			if(rptSum.SP3_data[0] == 0 && rptSum.SP3_data[1] == 0) printf("EMPTY\r\n");
			else{
				printf("Soil Moisture: %.2f kPa\r\n", (double)(rptSum.SP3_data[0])/100);
				printf("Soil Temp: %.2f C\r\n", (double)(rptSum.SP3_data[1])/100);				
			}
			break;
		case 0x02:
			if(rptSum.SP3_data[0] == 0 && rptSum.SP3_data[1] == 0) printf("EMPTY\r\n");
			else{
				printf("Soil Moisture: %.2f kPa\r\n", (double)(rptSum.SP3_data[0])/100);
				printf("Soil Temp: %.2f C\r\n", (double)(rptSum.SP3_data[1])/100);				
			}
			break;
		case 0x03:
			if(rptSum.SP3_data[0] == 0) printf("EMPTY\r\n");
			else{
				temp = (double)(rptSum.SP3_data[0])/100;
				if(temp > 60 || temp < -50) printf("EMPTY\r\n");
				else printf("Air Temp: %.2f C\r\n", temp);
			}
			break;
		case 0x04:
			if(rptSum.SP3_data[0] == 0) printf("EMPTY\r\n");
			else
				printf("PAR Reading: %.2d\r\n", rptSum.SP3_data[0]);
			break;
	}
	// print transducer 2
	printf("\r\nTransducer 2: \r\n");
	switch(rptSum.types[2]){
		case 0x00:
			printf("EMPTY\r\n");
			break;
		case 0x01:
			if(rptSum.SP3_data[2] == 0 && rptSum.SP3_data[3] == 0) printf("EMPTY\r\n");
			else{
				printf("Soil Moisture: %.2f kPa\r\n", (double)(rptSum.SP3_data[2])/100);
				printf("Soil Temp: %.2f C\r\n", (double)(rptSum.SP3_data[3])/100);				
			}
			break;
		case 0x02:
			if(rptSum.SP3_data[2] == 0 && rptSum.SP3_data[1] == 0) printf("EMPTY\r\n");
			else{
				printf("Soil Moisture: %.2f kPa\r\n", (double)(rptSum.SP3_data[2])/100);
				printf("Soil Temp: %.2f C\r\n", (double)(rptSum.SP3_data[3])/100);				
			}
			break;
		case 0x03:
			if(rptSum.SP3_data[1] == 0) printf("EMPTY\r\n");
			else{
				temp = (double)(rptSum.SP3_data[1])/100;
				if(temp > 60 || temp < -50) printf("EMPTY\r\n");
				else printf("Air Temp: %.2f C\r\n", temp);
			}
			break;
		case 0x04:
			if(rptSum.SP3_data[1] == 0) printf("EMPTY\r\n");
			else
				printf("PAR Reading: %.2d\r\n", rptSum.SP3_data[1]);
			break;
	}	
	// print transducer 3
	printf("\r\nTransducer 3: \r\n");
	switch(rptSum.types[2]){
		case 0x00:
			printf("EMPTY\r\n");
			break;
		case 0x01:
			if(rptSum.SP3_data[4] == 0 && rptSum.SP3_data[5] == 0) printf("EMPTY\r\n");
			else{
				printf("Soil Moisture: %.2f kPa\r\n", (double)(rptSum.SP3_data[4])/100);
				printf("Soil Temp: %.2f C\r\n", (double)(rptSum.SP3_data[5])/100);				
			}
			break;
		case 0x02:
			//printf("Soil Moisture: %d\r\n", rptSum.SP3_data[4]);
			//printf("Soil Temp: %dC\r\n", rptSum.SP3_data[5]);
			if(rptSum.valve_test == 1) printf("Valve: Functioning\r\n");
			else printf("Valve: Missing or Malfunctioning\r\n");
			break;
		case 0x03:
			if(rptSum.SP3_data[2] == 0) printf("EMPTY\r\n");
			else{
				temp = (double)(rptSum.SP3_data[2])/100;
				if(temp > 60 || temp < -50) printf("EMPTY\r\n");
				else printf("Air Temp: %.2f C\r\n", temp);
			}
			break;
		case 0x04:
			if(rptSum.SP3_data[2] == 0) printf("EMTPY\r\n");
			else
				printf("PAR Reading: %.2d\r\n", rptSum.SP3_data[2]);
			break;
	}		
	// print transducer 4
		printf("\r\nTransducer 4: \r\n");
	switch(rptSum.types[2]){
		case 0x00:
			printf("EMPTY\r\n");
			break;
		case 0x01:
			if(rptSum.SP3_data[6] == 0 && rptSum.SP3_data[7] == 0) printf("EMPTY\r\n");
			else{
				printf("Soil Moisture: %.2f kPa\r\n", (double)(rptSum.SP3_data[6])/100);
				printf("Soil Temp: %.2f C\r\n", (double)(rptSum.SP3_data[7])/100);				
			}
			break;
		case 0x02:
			if(rptSum.SP3_data[6] == 0 && rptSum.SP3_data[7] == 0) printf("EMPTY\r\n");
			else{
				printf("Soil Moisture: %.2f kPa\r\n", (double)(rptSum.SP3_data[6])/100);
				printf("Soil Temp: %.2f C\r\n", (double)(rptSum.SP3_data[7])/100);				
			}
			break;
		case 0x03:
			if(rptSum.SP3_data[3] == 0) printf("EMPTY\r\n");
			else{
				temp = (double)(rptSum.SP3_data[3])/100;
				if(temp > 60 || temp < -50) printf("EMPTY\r\n");
				else printf("Air Temp: %.2f C\r\n", temp);
			}
			break;
		case 0x04:
			if(rptSum.SP3_data[3] == 0) printf("EMTPY\r\n");
			else
				printf("PAR Reading: %.2d\r\n", rptSum.SP3_data[3]);
			break;
	}	
	
	printf("\r\nSP slot 4: %s\r\n", rptSum.SP4_name);
	// print transducer 1
	printf("Transducer 1: \r\n");
	switch(rptSum.types[3]){
		case 0x00:
			printf("EMPTY\r\n");
			break;
		case 0x01:
			if(rptSum.SP4_data[0] == 0 && rptSum.SP4_data[1] == 0) printf("EMPTY\r\n");
			else{
				printf("Soil Moisture: %.2f kPa\r\n", (double)(rptSum.SP4_data[0])/100);
				printf("Soil Temp: %.2f C\r\n", (double)(rptSum.SP4_data[1])/100);				
			}
			break;
		case 0x02:
			if(rptSum.SP4_data[0] == 0 && rptSum.SP4_data[1] == 0) printf("EMPTY\r\n");
			else{
				printf("Soil Moisture: %.2f kPa\r\n", (double)(rptSum.SP4_data[0])/100);
				printf("Soil Temp: %.2f C\r\n", (double)(rptSum.SP4_data[1])/100);				
			}
			break;
		case 0x03:
			if(rptSum.SP4_data[0] == 0) printf("EMPTY\r\n");
			else{
				temp = (double)(rptSum.SP4_data[0])/100;
				if(temp > 60 || temp < -50) printf("EMPTY\r\n");
				else printf("Air Temp: %.2f C\r\n", temp);
			}
			break;
		case 0x04:
			if(rptSum.SP4_data[0] == 0) printf("EMPTY\r\n");
			else
				printf("PAR Reading: %.2d\r\n", rptSum.SP4_data[0]);
			break;
	}
	// print transducer 2
	printf("\r\nTransducer 2: \r\n");
	switch(rptSum.types[3]){
		case 0x00:
			printf("EMPTY\r\n");
			break;
		case 0x01:
			if(rptSum.SP4_data[2] == 0 && rptSum.SP4_data[3] == 0) printf("EMPTY\r\n");
			else{
				printf("Soil Moisture: %.2f kPa\r\n", (double)(rptSum.SP4_data[2])/100);
				printf("Soil Temp: %.2f C\r\n", (double)(rptSum.SP4_data[3])/100);	
			}
			break;
		case 0x02:
			if(rptSum.SP4_data[2] == 0 && rptSum.SP4_data[3] == 0) printf("EMPTY\r\n");
			else{
				printf("Soil Moisture: %.2f kPa\r\n", (double)(rptSum.SP4_data[2])/100);
				printf("Soil Temp: %.2f C\r\n", (double)(rptSum.SP4_data[3])/100);				
			}
			break;
		case 0x03:
			if(rptSum.SP4_data[1] == 0) printf("EMPTY\r\n");
			else{
				temp = (double)(rptSum.SP4_data[1])/100;
				if(temp > 60 || temp < -50) printf("EMPTY\r\n");
				else printf("Air Temp: %.2f C\r\n", temp);
			}
			break;
		case 0x04:
			if(rptSum.SP4_data[1] == 0) printf("EMPTY\r\n");
			else
				printf("PAR Reading: %.2d\r\n", rptSum.SP4_data[1]);
			break;
	}	
	// print transducer 3
	printf("\r\nTransducer 3: \r\n");
	switch(rptSum.types[3]){
		case 0x00:
			printf("EMPTY\r\n");
			break;
		case 0x01:
			if(rptSum.SP4_data[4] == 0 && rptSum.SP4_data[5] == 0) printf("EMPTY\r\n");
			else{
				printf("Soil Moisture: %.2f kPa\r\n", (double)(rptSum.SP4_data[4])/100);
				printf("Soil Temp: %.2f C\r\n", (double)(rptSum.SP4_data[5])/100);				
			}
			break;
		case 0x02:
			//printf("Soil Moisture: %d\r\n", rptSum.SP4_data[4]);
			//printf("Soil Temp: %dC\r\n", rptSum.SP4_data[5]);
			if(rptSum.valve_test == 1) printf("Valve: Functioning\r\n");
			else printf("Valve: Missing or Malfunctioning\r\n");
			break;
		case 0x03:
			if(rptSum.SP4_data[2] == 0) printf("EMPTY\r\n");
			else{
				temp = (double)(rptSum.SP4_data[2])/100;
				if(temp > 60 || temp < -50) printf("EMPTY\r\n");
				else printf("Air Temp: %.2f C\r\n", temp);
			}
			break;
		case 0x04:
			if(rptSum.SP4_data[2] == 0) printf("EMPTY\r\n");
			else
				printf("PAR Reading: %.2d\r\n", rptSum.SP4_data[2]);
			break;
	}		
	// print transducer 4
		printf("\r\nTransducer 4: \r\n");
	switch(rptSum.types[3]){
		case 0x00:
			printf("EMPTY\r\n");
			break;
		case 0x01:
			if(rptSum.SP4_data[6] == 0 && rptSum.SP4_data[7] == 0) printf("EMPTY\r\n");
			else{
				printf("Soil Moisture: %.2f kPa\r\n", (double)(rptSum.SP4_data[6])/100);
				printf("Soil Temp: %.2f C\r\n", (double)(rptSum.SP4_data[7])/100);				
			}
			break;
		case 0x02:
			if(rptSum.SP4_data[6] == 0 && rptSum.SP4_data[7] == 0) printf("EMPTY\r\n");
			else{
				printf("Soil Moisture: %.2f kPa\r\n", (double)(rptSum.SP4_data[6])/100);
				printf("Soil Temp: %.2f C\r\n", (double)(rptSum.SP4_data[7])/100);				
			}
			break;
		case 0x03:
			if(rptSum.SP4_data[3] == 0) printf("EMPTY\r\n");
			else{
				temp = (double)(rptSum.SP4_data[3])/100;
				if(temp > 60 || temp < -50) printf("EMPTY");
				else printf("Air Temp: %.2f C\r\n", temp);
			}
			break;
		case 0x04:
			if(rptSum.SP4_data[3] == 0) printf("EMPTY\r\n");
			else
				printf("PAR Reading: %.2d\r\n", rptSum.SP4_data[3]);
			break;
	}	
	
	// print battery voltage reading
	printf("\r\nBattery Voltage: %dmV\r\n", rptSum.batt_reading);
	if (rptSum.batt_reading < 4500){
		if (rptSum.diagMode == 0) // if deployment mode
			printf("Battery voltage too low!\r\nFull batteries should be above 4.5V - replace with fresh batteries before deployment!\r\n\r\n");
		else // if field test mode
			if (rptSum.batt_reading > 2800) // if we aren't at the battery cutoff point
				printf("Batteries are at an adequate voltage - they should be changed when they fall below 2.8V\r\n\r\n");
			else // if we are at or below the cutoff point, inform user that the batteries should be changed
				printf("Batteries are getting close to depletion, they should be changed as soon as possilbe\r\n\r\n");
	}
	else{
		printf("Battery voltage is good. These batteries are fit for deployment\r\n\r\n");
	}
	
	// print MCU temp
	printf("Internal MCU Temp: %.2fC\r\n", (float)(rptSum.mcu_temp)/100);
	// if below 0 C
	if (rptSum.mcu_temp / 100 < 0){
		printf("Temperature reads below freezing, sensor appears to be malfunctioning\r\n\r\n");
	}
	// if above 50 C
	else if (rptSum.mcu_temp / 100 > 50){
		printf("MCU sensor is reading a very hot temperature, sensor appears to be malfunctioning\r\n\r\n");
	}
	// if between 0 C and 50 C
	else{
		printf("Temperature sensor is working properly\r\n\r\n");
	}
	
	// LED test results
	if (rptSum.LED_test){
		SetConsoleTextAttribute(hCon, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | BACKGROUND_GREEN);
		printf("CPU LED Test:              Passed\n");
		//SetConsoleTextAttribute(hCon, 7 & ~BACKGROUND_GREEN);
		FlushConsoleInputBuffer(hCon);
		//SetConsoleTextAttribute(hCon, 7);
	}
	else {
		SetConsoleTextAttribute(hCon, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | BACKGROUND_RED);
		printf("CPU LED Test:              Failed\n");
		//SetConsoleTextAttribute(hCon, 7 & ~BACKGROUND_RED);
		FlushConsoleInputBuffer(hCon);
		//SetConsoleTextAttribute(hCon, 7);
	}
	
	// Radio discovery mode test
	if (rptSum.radio_disc_test){
		SetConsoleTextAttribute(hCon, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | BACKGROUND_GREEN);
		printf("Radio Discovery Mode Test: Passed\r\n");
		SetConsoleTextAttribute(hCon, 7);
	}
	else {
		SetConsoleTextAttribute(hCon, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | BACKGROUND_RED);
		printf("Radio Discovery Mode Test: Failed\r\n");
		SetConsoleTextAttribute(hCon, 7);
	}
	
	// Radio opmode test
	if (rptSum.radio_op_test){
		SetConsoleTextAttribute(hCon, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | BACKGROUND_GREEN);
		printf("Radio Op-Mode Test:        Passed\r\n");
		SetConsoleTextAttribute(hCon, 7);
	}
	else {
		SetConsoleTextAttribute(hCon, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | BACKGROUND_RED);
		printf("Radio Discovery Mode Test:        Failed\r\n");
		SetConsoleTextAttribute(hCon, 7);
	}
	
	// Buzzer test
	if (rptSum.buzzer_test){
		SetConsoleTextAttribute(hCon, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | BACKGROUND_GREEN);
		printf("Buzzer Test:               Passed\r\n");
		SetConsoleTextAttribute(hCon, 7);
	}
	else {
		SetConsoleTextAttribute(hCon, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | BACKGROUND_RED);
		printf("Buzzer Test:           Failed\r\n");
		SetConsoleTextAttribute(hCon, 7);
	}
	
	// Valve test
	if (rptSum.valve_test){
		SetConsoleTextAttribute(hCon, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | BACKGROUND_GREEN);
		printf("Valve Test:                Passed\r\n");
		SetConsoleTextAttribute(hCon, 7);
	}
	else {
		SetConsoleTextAttribute(hCon, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | BACKGROUND_RED);
		printf("Valve Test:                Failed\r\n");
		SetConsoleTextAttribute(hCon, 7);
	}
	
	// SD Card Test
	if (rptSum.SD_test){
		SetConsoleTextAttribute(hCon, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | BACKGROUND_GREEN);
		printf("SD Card Test:              Passed\r\n");
		SetConsoleTextAttribute(hCon, 7);
	}
	else {
		SetConsoleTextAttribute(hCon, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | BACKGROUND_RED);
		printf("SD Card Test:              Failed\r\n");
		SetConsoleTextAttribute(hCon, 7);
	}
	
	// FRAM Test
	if (rptSum.FRAM_test){
		SetConsoleTextAttribute(hCon, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | BACKGROUND_GREEN);
		printf("FRAM Test:                 Passed\r\n");
		SetConsoleTextAttribute(hCon, 7);
	}
	else {
		SetConsoleTextAttribute(hCon, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | BACKGROUND_RED);
		printf("FRAM Test:                 Failed\r\n");
		SetConsoleTextAttribute(hCon, 7);
	}
	
	printf("\r\n------End of Report------\r\n");
	
} // end displayReportSummary
*/

//*****************************************************************************************************************
// Name: vPopulateReportResults
// Description: Pulls elements from a packet and places them in the report structure (assumes destination removed)
//*****************************************************************************************************************
void vPopulateReportResults(char* packet, reportSummary* rptSum){
	int i; // byte counter
	int buffCount; // tracks bytes for report
	
	// SP1
	for(i = 0; i < 7; i++){
		rptSum->SP1_name[i] = packet[i+2];
	} // end for(SP1)
	// SP2
	for(i = 0; i < 7; i++){
		rptSum->SP2_name[i] = packet[i+10];
	} // end for(SP2)
	// SP3
	for(i = 0; i < 7; i++){
		rptSum->SP3_name[i] = packet[i+18];
	} // end for(SP3)
	// SP4
	for(i = 0; i < 7; i++){
		rptSum->SP4_name[i] = packet[i+26];
	} // end for(SP4)
	
	buffCount = 34;
	// pulls out test results
	rptSum->LED_test = packet[34];
	rptSum->radio_disc_test = packet[35];
	rptSum->radio_op_test = packet[36];
	rptSum->buzzer_test = packet[37];
	rptSum->valve_test = packet[38];
	rptSum->SD_test = packet[39];
	rptSum->FRAM_test = packet[40];
	rptSum->diagMode = packet[41];

	// SP1 types
	for (i = 0; i < 4; i++){
		rptSum->SP1_types[i] = packet[i + 42];
	} // end for(SP1)
	
	// SP2 types
	for (i = 0; i < 4; i++){
		rptSum->SP2_types[i] = packet[i + 46];
	} // end for(SP2)
	
	// SP3 types
	for (i = 0; i < 4; i++){
		rptSum->SP3_types[i] = packet[i + 50];
	} // end for(SP3)

	// SP4 types
	for (i = 0; i < 4; i++){
		rptSum->SP4_types[i] = packet[i + 54];
	} // end for(SP4)

} // end vPopulateReportResults

/*
typedef struct{
	uchar id; // message id
	uchar flags; // message flags
	uint number; // message number
	uint address; // message source address
	uchar length; // message length
	data_element* data_elem;
} message;

typedef struct{
	network net_layer; // network layer
	message msg_layer; // message layer
	uchar* payload; // packet payload
} packet; 
*/
/*
	uchar DEID;
	uchar de_ver;
	uchar proc_id; 
	ulong proc_time;
	uchar data_gen_id;
	uchar transducer_id;
	uchar data_len;
	uchar item_type;
	uchar data[8];
	*/

//*****************************************************************************************************************
// Name: addByteToRXBuffer
// Description: Adds a new byte to the end of the buffer
//*****************************************************************************************************************
bool addByteToRXBuffer(char* buffer, uchar newByte){
	buffer[RXBufferPacketLocation] = newByte;
	if(RXBufferPacketLocation == 10){
		packet_len = buffer[RXBufferPacketLocation] + 6;
	} // end if
	
	// figure out end of packet
	if(++RXBufferPacketLocation == packet_len) return TRUE;
	else return FALSE;
	
} // end addByteToRXBuffer

//*****************************************************************************************************************
// Name: resetByteBufferLocation
// Description: Resets the global buffer index from the end of a packet
// Note: Code which invokes this function is responsible for ensuring that the end of the packet has been reached
//*****************************************************************************************************************
void resetByteBufferLocation(){
	RXBufferPacketLocation = 0; 
	packet_len = 0;
	
} // end resetByteBufferLocation

//*****************************************************************************************************************
// Name: printDataItems
// Description: Prints out all of the items from the data_item array
//*****************************************************************************************************************
/*
void printDataItems(){
	for(uchar i = 0; i < next_data_item; i++){
		//printf("%d, %u\r\n", i, next_data_item);
		printf("Data Item -- %d\r\n", i);
		printf("DEID: %ux\r\n",data_item_array[i].DEID);
		printf("de_ver: %ux\r\n", data_item_array[i].de_ver);
		printf("proc_time: %ux\r\n", data_item_array[i].proc_time);
		printf("data_gen_id: %lux\r\n", data_item_array[i].data_gen_id);
		printf("transducer_id: %ux\r\n", data_item_array[i].transducer_id);
		printf("data_len: %ux\r\n", data_item_array[i].data_len);
		printf("item_type: %ux\r\n", data_item_array[i].item_type);
		printf("data_1: %ux\r\n", data_item_array[i].data[0]);
		printf("data_2: %ux\r\n", data_item_array[i].data[1]);
		printf("data_3: %ux\r\n", data_item_array[i].data[2]);
		printf("data_4: %ux\r\n", data_item_array[i].data[3]);
		printf("data_5: %ux\r\n", data_item_array[i].data[4]);
		printf("data_6: %ux\r\n", data_item_array[i].data[5]);
		printf("data_7: %ux\r\n", data_item_array[i].data[6]);
		printf("data_8: %ux\r\n", data_item_array[i].data[7]);		
	} // end for
	
} // end printDataItems
*/

//*****************************************************************************************************************
// Name: nextDataElement
// Description: Returns the index of the next data element
//*****************************************************************************************************************
uchar nextDataElement(char* packet, uchar packet_len, uchar prev_de){
	uchar first_de = 11;
	uchar prev_len = (prev_de == 0) ? 0 : packet[prev_de + 1];
	uchar msg_len = packet[10];
	
	if(prev_de == 0){
		return first_de; // if there is no previous data element, then this is the first one
	} // end if
	else if(prev_de + prev_len >= msg_len + 6){
		return 0; // if there is no next data element, return 0
	} // end else if
	else{ 
		return prev_de + prev_len; // return next data element
	} // end else
	
} // end nextDataElement

/*
//*****************************************************************************************************************
// Name: nextItem
// Description: Returns the index of the next report item
//*****************************************************************************************************************
uchar nextItem(char* packet, uchar packet_len, uchar de_index, uchar report_index, uchar prev_item){
	uchar prev_len = 2 + packet[prev_item + 1];
	uchar de_end = packet[de_index + 1] + de_index;
	
	if(prev_item == 0){
		return report_index + 5; // 5 indices past the starting index
	} // end if
	else if(de_end <= prev_item + prev_len){
		return 0; // indicates end of data element report
	} // end if
	else{
		return prev_item + prev_len; // places you at the start of the next item
	} // end else
	
} // end nextItem
*/

//*****************************************************************************************************************
// Name: vFlushAllDEs
// Description: empties the global data elements array
//*****************************************************************************************************************
void vFlushAllDEs(){
	int i;
	memset(data_element_array, 0, 256*sizeof(data_element));
	numDataElements = 0;

} // end vFlushAllDEs

//*****************************************************************************************************************
// Name: vFlushAllPackets
// Description: This function will flush all packets from the global packet array
//*****************************************************************************************************************
void vFlushAllPackets(){
	int iCount;
	int iCount2;
	// loop through packets in array
	for(iCount = 0; iCount < numPackets; iCount++){
		// loop through bytes in packet
		for(iCount2 = 0; iCount2 < 64; iCount2++){
			packets_to_parse[iCount].contents[iCount2] = 0;
		} // end loop through bytes in packet
	} // end loop through packets in array
	
	// reset the packet counter in the array
	numPackets = 0;
		
} // end vFlushAllPackets

//data_item data_item_array[256]; // global array of data items (report payloads)
//packet packets_to_parse[256]; // contains unparsed packets
//int numPackets; // number of packets in packets_to_parse

//*****************************************************************************************************************
// Name: parsePacket
// Description: parses a packet into its data elements
//*****************************************************************************************************************
void parsePacket(packet* packet){
	int nextIndex;
	int prevIndex = 0;
	// code
	while((nextIndex = nextDataElement(packet->contents, packet->packetLen, prevIndex)) != 0){
		prevIndex = nextIndex;
		if(nextIndex >= packet->packetLen - 2){
			break;
		}
		
		data_element_array[numDataElements].DEID = packet->contents[nextIndex];
		data_element_array[numDataElements].de_len = packet->contents[nextIndex + 1];
		data_element_array[numDataElements].de_ver = packet->contents[nextIndex + 2];
		data_element_array[numDataElements].proc_ID = packet->contents[nextIndex + 3];
		data_element_array[numDataElements].time_stamp = *(ulong*)(packet->contents+nextIndex + 4);
		data_element_array[numDataElements].payload = (uchar*)(packet->contents+nextIndex + 8);

		numDataElements++;
	} // end while
	
	return; 
} // end parsePacket


/*

data_element data_element_array[256]; // global array of data elements
packet packets_to_parse[256]; // contains unparsed packets
int numPackets; // number of packets in packets_to_parse
int numBytes = 0; // number of bytes in packet
int numDataElements = 0; // number of data elements in data_element_array

int next_data_item = 0;
int numItems = 0;
int packet_len = 0;

// final report struct
typedef struct{
	// name (ex: EMTY, STM, ST, CMSTM, ect.)
	char SP1_name[8]; // 0-7 
	char SP2_name[8]; // 8-15
	char SP3_name[8]; // 16-23
	char SP4_name[8]; // 24-31
	// SUCCESS -> 0x01  FAIL -> 0x00
	uchar LED_test; 	// 32
	uchar radio_disc_test; // 33 
	uchar radio_op_test; // 34
	uchar buzzer_test; // 35
	uchar valve_test; // 36
	uchar SD_test;	// 37
	uchar FRAM_test; // 38
	int SP1_data[8];
	int SP2_data[8];
	int SP3_data[8];
	int SP4_data[8];
	int batt_reading; // in mV
	int mcu_temp; // in degrees C
} reportSummary;
*/

//*****************************************************************************************************************
// Name: vParseAllPackets
// Description: This function will parse all of the packets in the global packet array
//*****************************************************************************************************************
void vParseAllPackets(void){
	int parseCount = 0; // loop counter
	reportSummary rptSum; // new report structure
	memset(&rptSum, 0, sizeof(rptSum));

	// zero out this array, STs will use this array
	// to indicate whether they've recieved all their data
	int stcount = 0;
	int ST_calibration_count[4];
	for (stcount = 0; stcount < 4; stcount++){
		all_SPST_data_recieved[stcount] = 0;
		ST_calibration_count[stcount] = 8;
	}
	
	// do the last packet first, since it has initialization info
	vPopulateReportResults(packets_to_parse[numPackets - 1].contents, &rptSum);
	
	// loop through all packets and parse each one into data items
	for(parseCount = 0; parseCount < numPackets - 1; parseCount++){
		if(packets_to_parse[parseCount].packetLen >= 23)
			parsePacket(&packets_to_parse[parseCount]);
		else
			continue;
	} // end for
	
	// flush all packets
	//vFlushAllPackets();
	
	// first, set types for CMSTM based on valve test, in case no sensors are plugged in (because there won't be any)
	// data elements in that case
	if(strcmp("CMSTM", rptSum.SP1_name) == 0 && (rptSum.valve_test == 1))
		rptSum.types[0] = 0x02;
	if(strcmp("CMSTM", rptSum.SP2_name) == 0 && (rptSum.valve_test == 1))
		rptSum.types[1] = 0x02;
	if(strcmp("CMSTM", rptSum.SP3_name) == 0 && (rptSum.valve_test == 1))
		rptSum.types[2] = 0x02;
	if(strcmp("CMSTM", rptSum.SP4_name) == 0 && (rptSum.valve_test == 1))
		rptSum.types[3] = 0x02;
	
	
	int dataElementCount;
	// loop through all data elements and level up each data point
	for(dataElementCount = 0; dataElementCount < numDataElements; dataElementCount++){
		int NameIndex = data_element_array[dataElementCount].proc_ID;
		char* name = 0;
		int* dArray = 0;
		uchar* type = 0;

		if(NameIndex == 1){
			name = rptSum.SP1_name;	
			dArray = rptSum.SP1_data;
			type = rptSum.SP1_types;
		} 
		else if(NameIndex == 2){
			name = rptSum.SP2_name;
			dArray = rptSum.SP2_data;
			type = rptSum.SP2_types;
		} 
		else if(NameIndex == 3){
			name = rptSum.SP3_name;
			dArray = rptSum.SP3_data;
			type = rptSum.SP3_types;
		} 
		else if(NameIndex == 4){
			name = rptSum.SP4_name;
			dArray = rptSum.SP4_data;
			type = rptSum.SP4_types;
		} 
		else{
			name = "EMTY";
			dArray = 0;
			type = 0;
		}

		// counters to keep track of proper value placement into report struct
		int typeCounter = 0;
		int typeCounterTracker = 0;
		
		// if data point is from STM
		if(strcmp("STM", name) == 0){
			int val; // for storing data point
			int len; // for storing length of data point
			int channel; // for tracking channel of data point
			int byteIndex; // for tracking location in data element
			// int iCalcMoist_SPSTM_5TM(long lRawTicks)
			// int iCalcTemp_SPSTM_5TM(long lRawTicks)
			// 1: soil moisture, 2: soil temp
			
			// set type
			//rptSum.types[dataElementCount] = 0x01;
			rptSum.types[data_element_array[dataElementCount].proc_ID - 1] = 0x01;

			for(byteIndex=0; byteIndex < data_element_array[dataElementCount].de_len-8; ){
				channel = data_element_array[dataElementCount].payload[0 + byteIndex]; // channel is first byte in payload
				len = data_element_array[dataElementCount].payload[1 + byteIndex]; // length is second
				val = data_element_array[dataElementCount].payload[2 + byteIndex]; // actual data point is third
				for(int i = 1; i < len; i++){ // if data is longer than one byte, handle it
					//data_element_array[dataElementCount].payload[2 + byteIndex] <<= 8;
					//data_element_array[dataElementCount].payload[2 + byteIndex] |= data_element_array[dataElementCount].payload[2+i + byteIndex];
					val <<= 8;
					val |= data_element_array[dataElementCount].payload[2+i+byteIndex];
				} // end for(each byte in datapoint)
				// if channel is even and transducer is present...
				if(channel % 2 == 0 && val != 0x02){
					// use temp level up function
					//if (type[channel - 1] == 0x78)
					if (type[typeCounter] == 0x78)
						val = iCalcTemp_SPSTM_5TM((long)val);
					else if (type[typeCounter] == 0x6C)
					//else if (type[channel - 1] == 0x6C)
						val = iCalcTemp_SPSTM_MPS6((long)val);
				}
				// if channel is odd and transducer is present...
				else if(channel % 2 == 1 && val != 0x02){
					//if (type[channel - 1] == 0x78)
					if (type[typeCounter] == 0x78)
						// use moist level up function
						val = iCalcMoist_SPSTM_5TM((long)val);
					else if (type[typeCounter] == 0x6C)
					//else if (type[channel - 1] == 0x6C)
						val = iCalcMoist_SPSTM_MPS6((long)val);
				}
				// otherwise, no transducer present
				else{
					val = 0; // place zero into data array
				}
				// place leveled up value into structure
				if (channel == 0){
					// do nothing
				}
				else if (dArray[channel - 1] == 0)
					dArray[channel - 1] = val;
				else
					dArray[channel + 7] = val;
				
				// if the first part of the datapoint (the type corresponds to 2 datapoints)
				if (typeCounterTracker == 0){
					typeCounterTracker = 1;
				}
				// if the second part of the datapoint (the type corresponds to 2 datapoints)
				else if (typeCounterTracker == 1){
					typeCounterTracker = 0;
					typeCounter++;
				}

				byteIndex += len + 2;
				
			} // end for all data points in an STM data element
				
			// reset counters
			typeCounterTracker = 0;
			typeCounter = 0;

		} // end if data element is STM 
		
		// if data point is from CMSTM
		else if(strcmp("CMSTM", name) == 0){
			int val; // for storing data point
			int len; // for storing length of data point
			int channel; // for tracking channel of data point
			int byteIndex; // for tracking location in data element
			// int iCalcMoist_SPSTM_5TM(long lRawTicks)
			// int iCalcTemp_SPSTM_5TM(long lRawTicks)
			// 1: soil moisture, 2: soil temp
			
			// set type
			//rptSum.types[dataElementCount] = 0x02;
			rptSum.types[data_element_array[dataElementCount].proc_ID - 1] = 0x02;
			
			for(byteIndex=0; byteIndex < data_element_array[dataElementCount].de_len-8; ){
				channel = data_element_array[dataElementCount].payload[0+byteIndex]; // channel is first byte in payload
				len = data_element_array[dataElementCount].payload[1+byteIndex]; // length is second
				val = data_element_array[dataElementCount].payload[2+byteIndex]; // actual data point is third
				for(int i = 1; i < len; i++){ // if data is longer than one byte, handle it
					//data_element_array[dataElementCount].payload[2] <<= 8; 
					//data_element_array[dataElementCount].payload[2] |= data_element_array[dataElementCount].payload[2+i];
					val <<= 8;
					val |= data_element_array[dataElementCount].payload[2+i+byteIndex];
				} // end for(each byte in datapoint)
				// if channel is even and transducer is present...
				if(channel % 2 == 0 && val != 0x02){
					if (type[typeCounter] == 0x78)
					//if (type[channel - 1] == 0x78)
						// use temp level up function
						val = iCalcTemp_SPSTM_5TM((long)val);
					else if (type[typeCounter] == 0x6C)
					//else if (type[channel - 1] == 0x6C)
						val = iCalcTemp_SPSTM_MPS6((long)val);
				}
				// if channel is odd and transducer is present...
				else if(channel % 2 == 1 && val != 0x02){
					if (type[typeCounter] == 0x78)
					//if (type[channel - 1] == 0x78)
						// use moist level up function
						val = iCalcMoist_SPSTM_5TM((long)val);
					else if (type[typeCounter] == 0x6C)
					//else if (type[channel - 1] == 0x6C)
						val = iCalcMoist_SPSTM_MPS6((long)val);
				}
				// otherwise, no transducer present
				else{
					val = 0; // place zero into data array
				}
				// place leveled up value into structure
				if (channel == 0){
					// do nothing
				}
				else if (dArray[channel - 1] == 0)
					dArray[channel - 1] = val;
				else
					dArray[channel + 7] = val;
				
				// if the first part of the datapoint (the type corresponds to 2 datapoints)
				if (typeCounterTracker == 0){
					typeCounterTracker = 1;
				}
				// if the second part of the datapoint (the type corresponds to 2 datapoints)
				if (typeCounterTracker == 1){
					typeCounterTracker = 0;
					typeCounter++;
				}

				byteIndex += len + 2;
			} // end for all data points in an STM data element
			
			// reset trackers
			typeCounterTracker = 0;
			typeCounter = 0;

		} // end if data element is from a CMSTM

		// if data point is from a SPST
		else if(strcmp("ST", name) == 0){
			int val;
			short val1 = 0; // for storing offset
			short val2 = 0; // for storing CJ
			short val3 = 0; // for storing ch
			short val4 = 0; // for storing offset
			short val5 = 0; // for storing CJ
			short val6 = 0; // for storing ch
			//short val7 = 0; // for storing offset
			//short val8 = 0; // for storing CJ
			//short val9 = 0; // for storing ch
			//short val10 = 0; // for storing offset
			//short val11 = 0; // for storing CJ
			//short val12 = 0; // for storing ch
			int finalVal; // for storing leveled up temp
			int len;
			int channel;
			int byteIndex;
			// int iCalcTemp_SPST(short Offset_ticks, short CJ_ticks, short Ch_ticks)
			// 1: cold junction, 2: offset, 3: adc ticks
			
			// set type
			//rptSum.types[dataElementCount] = 0x03;
			rptSum.types[data_element_array[dataElementCount].proc_ID - 1] = 0x03;
			
			for(byteIndex = 0; byteIndex < data_element_array[dataElementCount].de_len-8; ){
				channel = data_element_array[dataElementCount].payload[0+byteIndex]; // channel is first byte in payload
				len = data_element_array[dataElementCount].payload[1+byteIndex]; // length is second
				val = data_element_array[dataElementCount].payload[2+byteIndex]; // actual data point is third
				for(int i = 1; i < len; i++){
					//data_element_array[dataElementCount].payload[2] <<= 8;
					//data_element_array[dataElementCount].payload[2] |= data_element_array[dataElementCount].payload[2+i];
					val <<= 8;
					val |= data_element_array[dataElementCount].payload[2+i+byteIndex];
				} // end for(each byte in data point)
				
				switch(channel){
					case 1:
						val1 = val;
						//val2 = val; // its CJ
						break; 
					case 2:
						val2 = val;
						//val1 = val; // its offset
						break;
					case 3:
						val3 = val; // its adc ticks
						break;
					case 4:
						val4 = val;
						//val5 = val; // its CJ
						break; 	
					case 5:
						val5 = val;
						//val4 = val; // its offset
						break;
					case 6:
						val6 = val; // its adc ticks
						break;
				/*
					case 7:
						val7 = val;
						//val8 = val; // its CJ
						break; 	
					case 8:
						val8 = val;
						//val7 = val; // its offset
						break;
					case 9:
						val9 = val; // its adc ticks
						break;
					case 0x0A:
						val10 = val;
						//val11 = val; // its CJ
						break; 	
					case 0x0B:
						val11 = val;
						//val10 = val; // its offset
						break;
					case 0x0C:
						val12 = val; // its adc ticks
						break;
						*/
				} // end switch(channel)
				
				byteIndex += len + 2;
				
			} // end for(each byte in data element)	
			
			// only do the hot/cold tests while the data tests haven't all been processed
			if (all_SPST_data_recieved[data_element_array[dataElementCount].proc_ID - 1] == 0){

				int lvlCount; // loop through each set of 3 values and level up if they exist
				for (lvlCount = 0; lvlCount < 4; lvlCount++){

					// if we're looking at the first set of values and a transducer is plugged in
					if (lvlCount == 0 && val3 != 0x02){
						finalVal = (int)(100 * iCalcTemp_SPST((short)(val1), (short)(val2), (short)(val3))); // level up values
						if (channel == 0){
							// do nothing
						}
						else if (dArray[0] == 0)
							dArray[0] = finalVal; // put temp in index 0
						else
							dArray[4] = finalVal;
					} // end if first set of values

					// if we're looking at the second set of values and a transducer is plugged in
					else if (lvlCount == 1 && val4 != 0x02){
						finalVal = (int)(100 * iCalcTemp_SPST((short)(val1), (short)(val2), (short)(val4))); // level up values
						if (channel == 0){
							// do nothing
						}
						else if (dArray[1] == 0)
							dArray[1] = finalVal; // put temp in index 1
						else
							dArray[5] = finalVal;
					} // end if second set of values

					else if (lvlCount == 2 && val5 != 0x02){
						//if(val7 != 0){ // if channels actually exist
						finalVal = (int)(100 * iCalcTemp_SPST((short)(val1), (short)(val2), (short)(val5))); // level up values
						if (channel == 0){
							// do nothing
						}
						else if (dArray[2] == 0)
							dArray[2] = finalVal; // put temp in index 2
						else
							dArray[6] = finalVal;
						//}
					} // end if third set of values
					else if (lvlCount == 3 && val6 != 0x02){
						//if(val10 != 0){ // if channels actually exist
						finalVal = (int)(100 * iCalcTemp_SPST((short)(val1), (short)(val2), (short)(val6))); // level up values
						if (channel == 0){
							// do nothing
						}
						else if (dArray[3] == 0)
							dArray[3] = finalVal; // put temp in index 3
						else{
							// final test data point
							dArray[7] = finalVal;

							// indicate in the array that all test data for this SP has been recieved
							all_SPST_data_recieved[data_element_array[dataElementCount].proc_ID - 1] = 1;
						}
						//}
					} // end if fourth set of values
					else{
						continue;
					} // end else
				} // end for each group of 3 values
			}// end if all_SPST_data_recieved[data_element_array[dataElementCount].proc_ID - 1] == 0;

			// if all_SPST_data_recieved[data_element_array[dataElementCount].proc_ID - 1] == 1;
			else{
				// if slot is empty
				if(channel == 0){
					// do nothing
				}
				else if (val3 == 0x02){
					dArray[ST_calibration_count[data_element_array[dataElementCount].proc_ID - 1]] = 0x02;
					
					// increment counter so we know where to put the next data point
					ST_calibration_count[data_element_array[dataElementCount].proc_ID - 1] += 1;
				}

				// if slot isn't empty
				else{
					// level up calibration sample for transducer 1 and place into the data array
					dArray[ST_calibration_count[data_element_array[dataElementCount].proc_ID - 1]] = (int)(100 * iCalcTemp_SPST((short)(val1), (short)(val2), (short)(val3)));
					
					// increment counter so we know where to put the next data point
					ST_calibration_count[data_element_array[dataElementCount].proc_ID - 1] += 1;
				}

			} // end else
					
		} // end if data element is ST
		
		// if data element is from an SL
		else if(strcmp("SL", name) == 0){
			// fill in this code
						
			// set type
			//rptSum.types[dataElementCount] = 0x04;
			rptSum.types[data_element_array[dataElementCount].proc_ID - 1] = 0x04;
			
		} // end if data element is from an SL
		
		// if data element is from CP
		else if(data_element_array[dataElementCount].proc_ID == 0){
			int val; // for storing data point
			int len; // for storing length of data point
			int channel; // for tracking channel of data point
			int byteIndex; // for tracking location in data element
			
			// set type
			//rptSum.types[data_element_array[dataElementCount].proc_ID] = 0x00;
			
			for(byteIndex = 0; byteIndex < data_element_array[dataElementCount].de_len-8; ){
				channel = data_element_array[dataElementCount].payload[0+byteIndex]; // channel is first byte in payload
				len = data_element_array[dataElementCount].payload[1+byteIndex]; // length is second
				val = data_element_array[dataElementCount].payload[2+byteIndex]; // actual data point is third
				for(int i = 1; i < len; i++){
					//data_element_array[dataElementCount].payload[2] <<= 8;
					//data_element_array[dataElementCount].payload[2] |= data_element_array[dataElementCount].payload[2+i];
					val <<= 8;
					val |= data_element_array[dataElementCount].payload[2+i+byteIndex];
				} // end for(each byte in data point)
				
				// reset byte index counter
				byteIndex += len + 2;
				
			} // end for(each byte in data element)
			
			// if data element indicates a battery reading
			if(channel == 0x02){
				rptSum.batt_reading = val;
			}
			// if data element indicates an internal temp reading
			else if(channel == 0x03){
				rptSum.mcu_temp = val;
			}
			// if data element indicates another reading
			else{
				// do nothing
			}
		} // end if data element is from CP
	} // end loop through all data elements
	
	// figure out calibration constants
	findCalibrationConstants(&rptSum);

	// display report
	displayReportSummary(rptSum);
	
	// flush all packets
	vFlushAllPackets();

	// flush all data elements
	vFlushAllDEs();

} // end vParseAllPackets

// data_element_array

/*
typedef struct{
	uchar DEID;
	uchar de_len;
	uchar de_ver;
	ulong time_stamp;
	uchar* payload;
}data_element;
*/

/*
//*****************************************************************************************************************
// Name: parsePacket_sensors
// Description: Fill in and parse a packet containing sensor data elements
//*****************************************************************************************************************
uchar parsePacket_sensors(char* packet, ulong packet_len){
	network n1;
	data_element de1;
	report rep1;
	data_item di1;
	
	n1.destination = packet[0] & 0xFF;
	n1.destination = (n1.destination << 8) | (packet[1] & 0xFF);
	
	n1.source = packet[2] & 0xFF;
	n1.source = (n1.source << 8) | (packet[2] & 0xFF);
	
	int next_de = 0, prev_de = 0;
	while((next_de = nextDataElement(packet, packet_len, prev_de)) != 0){
		prev_de = next_de;
		de1.DEID = packet[next_de];
		de1.de_len = packet[next_de + 1];
		de1.de_ver = packet[next_de + 2];		
		
		// populate report
		rep1.procID = packet[next_de + 3];
		rep1.data_gen_id = packet[next_de + 4];
		ulong time = (packet[next_de + 5] << 24) | (packet[next_de + 6] << 16) | (packet[next_de + 7] << 8) | (packet[next_de + 8]);
		uchar rep_len = packet[next_de + 9];
		
		int next_val = 0, prev_val = 0;
		
		// while there is another data item in the packet, parse the next items
		while((next_val = nextItem(packet, packet_len, next_de, next_de + 3, prev_val)) != 0){
			prev_val = next_val;
			data_item_array[next_data_item].transducer_id = packet[next_val];
			data_item_array[next_data_item].data_len = packet[next_val + 1];
			
			// copy values from packet into global data item array
			data_item_array[next_data_item].item_type = ITEM_TYPE_SENSOR;
			for(int i = 0; i < data_item_array[next_data_item].data_len; i++){
				data_item_array[next_data_item].data[i] = packet[next_val + 1 + i];
			} // end for
			
			// for convenience, place these values into this struct
			data_item_array[next_data_item].DEID = de1.DEID;
			data_item_array[next_data_item].de_ver = de1.de_ver;
			data_item_array[next_data_item].proc_id = rep1.procID;
			data_item_array[next_data_item].proc_time = time;
			data_item_array[next_data_item].data_gen_id = rep1.data_gen_id;

			// increment data item counter
			next_data_item++;	
		} // end while
	} // end while
	
	// print the data items parsed from the packet
	//printDataItems();

} // end parsePacket_sensors
*/

//*****************************************************************************************************************
// PROBABLY NEED TO REMOVE THIS
// Name: parsePacket_batt
// Description: fills in and parset a packet containing battery sense data elements
//*****************************************************************************************************************
uchar parsePacket_batt(){
	return 0;	
} // end parsePacket_batt


/*
// this method will parse a message, placing each item into it's correct location
void vParseMessage(uchar cMessage[], uchar cMessage_len, packet* p1){
	// if there is data and it's not an empty packet...
	if(p1 != null && cMessage_len >= 13){
		// make new struct vars
		
		
		// combine crc bytes
		int crc = cMessage[cMessage_len - 2];
		crc = (crc << 8) | cMessage_len - 1];
		
		int payload_count = 0;
		uchar* message_payload;
		
		// for each byte in the packet (excluding the CRC bytes)
		for(int i = 0; i < cMessage_len - 2; i++){
			// use switch over array index to parse out package
			switch(i){
				case 0:
					p1->net_layer.destination = cMessage[i] & 0xFF;
					break;
				case 1:
					p1->net_layer.destination = (network.destination << 8) | (cMessage[i] & 0xFF);
					break;
				case 2:
					p1->net_layer.source = cMessage[i] & 0xFF;
					break;
				case 3:
					p1->net_layer.source = (network.source << 8) | (cMessage[i] & 0xFF);
					break;
				case 4:
					p1->msg_layer.id = cMessage[i] & 0xFF;
					break;
				case 5:
					p1->msg_layer.flags = cMessage[i] & 0xFF;
					break;
				case 6:
					p1->msg_layer.number = cMessage[i] & 0xFF;
					break;
				case 7:
					p1->msg_layer.number = (message.number << 8) | (cMessage[i] & 0xFF);
					break;
				case 8:
					p1->msg_layer.address = cMessage[i] & 0xFF;
					break;
				case 9:
					p1->msg_layer.address = (message.address << 8) | (cMessage[i] & 0xFF);
					break;
				case 10:
					// TODO: ensure message length is populated
					p1->msg_layer.length = (cMessage[i] & 0xFF);
					// if the message length is 7, then it is empty
					if(p1->msg_layer.length > 7){
						// then there is at least one data element in the message
						message_payload = new uchar[p1->msg_layer.length - 7];
					}
					break;
				default:
					// if none of the cases aboce are met, then the current byte is part of the payload
					if(payload_count < p1->msg_layer.length){
						message_payload[payload_count++] = cMessage[i];
					}
					break;
			} // end switch
		} // end for()
		
		//-------------------------------------
		// populate data_element 
		//-------------------------------------
		int j = 0; k = 0;

		p1->msg_layer.data_elem = new data_element[]
		
		// for each byte in the message payload
		for(int i = 0; i < p1->msg_layer.length - 7; i++){
			switch(k){
				case 0:
					data_element[j] = new data_element;
					data_element[j].id = message_payload[i] & 0xFF;
					break;
				case 1:
					data_element[j].length = message_payload[i] & 0xFF;			// check this
					data_element[j].payload = new uchar[data_element[j].length - 3]; // check this
					
					if(payload_count - data_element[j].length > 0){					// not right
						data_element_count++;										// not right
						data_element = extend_data_element_array(data_element, 1);  // not right
						payload_count = payload_count 0 data_element[j].length;     // not right
					} // end if
					
				case 2:
					data_element[j].version = message_payload[i] & 0xFF;		// check
					break;
				default:
					data_element[j].payload[k-3] = message_payload[i]; 			// check
					break;
			} // end switch
			
			// if we reached the end of the current data element, reset the counters
			if(k == data_element[j].length - 1){
				j++;
				k = 0;
			}
			else{
				k++;
			}
		}
			
		// fill in the data element(s) info
		for(int i = 0; i < data_element_count; i++){				// check
			// if the data element is a report...
			if(data_element[j].id == 2){
				// init the data element counters
				int data_element_index = 0;
				report_length = 0;
				// init a temporary array used to store and validate a report
				uchar* temp_data;
				
				// store the processor id for this data element
				int processor_id = data_element[i].payload[data_element_index++] & 0xFF;     // check don't think this is right
				
				// calculate and store the timestamp for this data element
				int timestamp = data_element[i].payload[data_element_index++] & 0xFF;
				timestamp = (timestamp << 8) | (data_element[i].payload[data_element_index++] & 0xFF);
				timestamp = (timestamp << 8) | (data_element[i].payload[data_element_index++] & 0xFF);
				timestamp = (timestamp << 8) | (data_element[i].payload[data_element_index++] & 0xFF);
				
				// generate reports for each data element object
				while(data_element_index < data_element[i].payload.length){
					// create a new report
					report rpt = new report;
					// store the reports process id form the data element
					rpt.procID = processor_id;
					// store the data source id (sensor id, etc.)
					rpt.data_gen_id = data_element[i].payload[data_element_index++] & 0xFF;
					//store the data sample timestamp form the data element
					rpt.time = timestamp;
					// store the length of this report
					report_length = data_element[i].payload[data_element_index++] & 0xFF;
					
					// -- crazy channel map codec garbage here -- 
					
					// the wisards reports conitain data points that can be 1-8 bytes because they get truncated if the value has leading 0s
					if(report_length == 1){
						rpt.data_type = (uchar)
					} 
					
				}
			}
		}
			
		} // end for()
			
	} // end if() 
	
	
} // end vParseMessage() */
