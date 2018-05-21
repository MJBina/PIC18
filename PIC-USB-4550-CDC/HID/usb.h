unsigned char GetEP1 (unsigned char buffer[]);
unsigned char PutEP1 (unsigned char bytes, unsigned char buffer[]);
unsigned char PutEP2 (unsigned char bytes, unsigned char buffer[]);
extern void StallUSBEP(char);
extern void UnstallUSBEP(char);
extern void CheckSleep();
extern void ServiceUSBInt();
extern void InitUSB();
extern void RemoteWakeup();
extern void SoftDetachUSB();




