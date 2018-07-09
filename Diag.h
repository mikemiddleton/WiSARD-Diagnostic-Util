#ifndef DIAG_H 
#define DIAG_H

typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned char uchar;

// function declarations 
void vRunDiagSequence(void);
void vStreamSensorData(void);
void vInteractiveMode(void);
void vTransitionState(uchar);
void vGetPacket(void);
void vCheckForDiagPacket(void);
void getKeyboardCMD(void);

#endif
