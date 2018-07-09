//*****************************************************************************************************************
// File: LevelUp.cpp
// Author: Michael Middleton
// Date: 06/09/2015
// Description: This file contains the functions which convert raw ADC ticks to meaningul units
//*****************************************************************************************************************

// header file includes
#include <cmath>
#include "LevelUp.h"

// **** TO DO **** //
// - Add conversions to level up Light board readings
// - Add conversions to level up MPS-6 sensor readings
// - Add conversinos to level up particulate sensor readings

//*****************************************************************************************************************
// Name: CJTemp(short ADC_Ticks)
// Description: compute temp of cold junction
//*****************************************************************************************************************
double CJTemp(short ADC_Ticks){
	double ADCResolution = 2.5/4096;
	double r0 = 10000;
	double CJTemperature, R, Vin;
	
	// compute voltage from ticks
	Vin = ADCResolution * ADC_Ticks;
	R = (2.5-Vin)/(Vin/r0); // compute thermistor resistance by solving the voltage divider equation
	
	// coefficients for the steinhart-hart equation
	double A = 1.1224922E-3;
	double B = 2.3591320E-4;
	double C = 7.4995733E-8;
	
	// compute degrees
	CJTemperature = (1/(A+B*log(R) + C*pow(log(R), 3))) - 273.15;
	
	return CJTemperature;
}

//*****************************************************************************************************************
// Name: TypeT_Poly(double HJ_Volts)
// Description: compute the temp of the TC from the voltage after cold junction voltage has been determined
//*****************************************************************************************************************
double TypeT_Poly(double HJ_Volts){
	double Temperature;
	
	Temperature = 0.100860910 + (HJ_Volts) * 25727.94369 + pow(HJ_Volts, 2) * -767345.8295 + pow(HJ_Volts, 3) * 78025595.81 + pow(HJ_Volts, 4) * -9247486589L + pow(HJ_Volts, 5) * 6.97688e-011 + pow(HJ_Volts,6) * -2.66192e+013 + pow(HJ_Volts, 7) * 3.94078E+014;
	return Temperature;
}

//*****************************************************************************************************************
// Name: CJ_Voltage(double cJTemperature)
// Description: determine what the voltage would be for a type-T TC at the cold junction given the thermistor temp
//*****************************************************************************************************************
double CJ_Voltage(double cJTemperature){
	short counter;
	short counter2=0;
	double TC_Voltages[482];
	double TC_Degrees[482];
	double VoltageSum = 0;
	double abs;
	
	for(counter=0; counter<482; counter++){
		TC_Voltages[counter] = (counter - 241) * 8e-006; // initialize all voltage values
	}
	for(counter=0; counter<482; counter++){
		TC_Degrees[counter] = TypeT_Poly(TC_Voltages[counter]);
	}
	for(counter=0; counter<482; counter++){
		abs = TC_Degrees[counter] - cJTemperature;
		if(abs < 0)
			abs *= -1;
		if(abs < 1){
			VoltageSum += TC_Voltages[counter];
			counter2 += 1;
		}
	}
	
	double in_volt = VoltageSum/counter2;
	return in_volt;
}

//*****************************************************************************************************************
// Name: iCalcTemp_SPST
// Description: Performs level-up operations for the SPST board temperature readings
//*****************************************************************************************************************
double iCalcTemp_SPST(short Offset_ticks, short CJ_ticks, short Ch_ticks){
	double CJTemperature;
	double TicksAdj, HJ_volts;
	
	// Get the temperature at the cold junction
	CJTemperature = CJTemp(CJ_ticks);
	
	// Convert the cold junction temperature to a type-t thermocouple voltage
	double CJ_volt = CJ_Voltage(CJTemperature);
	
	// Apply the offset to account for any op-amp bias
	TicksAdj = Ch_ticks - Offset_ticks;
	
	// Compute the voltage of the hot junction
	HJ_volts = TicksAdj * 2.5 / (4096) / 250; // calculate the voltage
	HJ_volts = HJ_volts + CJ_volt;
	
	// calculate temp with the polynomial
	return TypeT_Poly(HJ_volts);
	
} // end iCalcTemp_SPST

//*****************************************************************************************************************
// Name: iCalcTemp_SPSTM
// Description: Performs level-up operations for the SPSTM board temperature readings 5TM sensor
//*****************************************************************************************************************
int iCalcTemp_SPSTM_5TM(long lRawTicks){
	// operation variables
	int retVal;
	int tRaw2;
	
	// calculate temp
	if(lRawTicks == 1023) // if sensor is not function properly, return this value
		return 1023;
	else if(lRawTicks <= 900) // if temp is below threshhold, no modification needed
		tRaw2 = lRawTicks;
	else if(lRawTicks > 900)  // if temp is above threshold, perform scaled offset
		tRaw2 = 900 + 5*(lRawTicks - 900);
		 
	//retVal = (tRaw2 - 400)/10; // calc temp in degrees celcius
	retVal = (tRaw2 - 400)*10; // calc temp in degrees celcius
	return retVal;
	
} // end iCalcTemp_SPSTM

//*****************************************************************************************************************
// Name: iCalcMoist_SPSTM
// Description: Performs level-up operations for the SPSTM board soil moisture readings 5TM sensor
//*****************************************************************************************************************
int iCalcMoist_SPSTM_5TM(long lRawTicks){
	double retVal;
		// calculate soil moisture: Formula taken from 5TM Users Manual
		 retVal = (4.3)*pow(10,-6)*pow(lRawTicks,3) - (5.5)*pow(10,-4)*pow(lRawTicks,2) + (2.92)*pow(10,-2)*lRawTicks - (5.3)*pow(10,-2);
		//retVal = (4.6*10^(-6))*(lRawTicks^3) - (5.5*10^(-4))*(lRawTicks^2) + (2.92*(10^-2))*lRawTicks - (5.3*(10^-2));
	return (int)(retVal*100);
	
} // end iCalcMoist_SPSTM

//*****************************************************************************************************************
// Name: iCalcTemp_SPSTM_MPS6
// Description: Performs level-up operations for the SPSTM board temperature readings MPS-6 sensor
//*****************************************************************************************************************
int iCalcTemp_SPSTM_MPS6(long lRawTicks){
	int retVal = (int)lRawTicks;
	// code goes here
	return retVal*10;
} // end iCalcTemp_SPSTM_MPS6

//*****************************************************************************************************************
// Name: iCalcTemp_SPSTM
// Description: Performs level-up operations for the SPSTM board temperature readings
//*****************************************************************************************************************
int iCalcMoist_SPSTM_MPS6(long lRawTicks){
	int retVal = (int)lRawTicks;
	// code goes here
	return retVal;
} // end iCalcMoist_SPSTM_MPS6

//*****************************************************************************************************************
// Name: iCalcLight
// Description: Performs level-up operations for the SPSL board light intensity readings 
//*****************************************************************************************************************
int iCalcLight(long lRawTicks){
	int retVal = 0;
	// calculate temp
	return retVal;
	
} // end iCalcLight

//*****************************************************************************************************************
// Name: iCalcParticulate
// Description: Performs the level-up operations for the SPSTM board particulate sensor readings
//*****************************************************************************************************************
int iCalcParticulate(long lRawTicks){
	int retVal = 0;
	// calculate particulate concentration
	return retVal;
} // end iCalcParticulate
