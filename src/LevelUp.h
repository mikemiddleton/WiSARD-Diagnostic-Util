#ifndef LVLUP_H 
#define LVLUP_H

// function declarations
int iCalcTemp_SPST(long lRawTicks);
int iCalcLight(long lRawTicks);
int iCalcParticulate(long lRawTicks);
double CJTemp(short ADC_Ticks);
double TypeT_Poly(double HJ_Volts);
double CJ_Voltage(double cJTemperature);
double iCalcTemp_SPST(short Offset_ticks, short CJ_ticks, short Ch_ticks);
int iCalcTemp_SPSTM_5TM(long lRawTicks);
int iCalcMoist_SPSTM_5TM(long lRawTicks);
int iCalcTemp_SPSTM_MPS6(long lRawTicks);
int iCalcMoist_SPSTM_MPS6(long lRawTicks);

#endif
