
#include <stdio.h>
#include "LibUVCExt.h"

int main(int argc, char *argv[])
{
	BYTE packetInit[8] = {0x80, 0x02, 0x07, 0x00, 0x00 , 0x00 , 0x00 , 0x00};
	BYTE packetCmd[8] = { 0x80, 0x04, 0x07, 0x00, 0x00 , 0x00 , 0x00 , 0x00 };
	BYTE buffer[8];
	ULONG readCount = 0;
	//
	bool f = LibUVCInit();
	printf("Init OK: %d\n", f);
	f = LibUVCWriteControl(packetInit, 8, &readCount);
	printf("%d\n", f);
	f = LibUVCWriteControl(packetCmd, 8, &readCount);
	printf("%d\n", f);
	f = LibUVCReadControl(buffer, 8, &readCount);
	printf("%d\n", f);
	for (int i = 0; i < readCount; i++)
		printf("readCount %x\n", buffer[i]);
	Sleep(5000);
	f = LibUVCDeInit();
	printf("DeInit OK: %d\n", f);
}
