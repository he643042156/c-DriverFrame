#include <stdio.h>
#include <Windows.h>  
#include <time.h> 
  
#define MYDEVICE L"\\\\.\\MyWDF_LINK"  
#define IOCTL_READ CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WRITE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_IN_DIRECT, FILE_ANY_ACCESS)

#define NUM 1024
#define READ_4M ( 1024 * 4)

int main()  
{  	
	char *p = (char)malloc(READ_4M);
	if (p == NULL)
		goto err;
    HANDLE hDevice = CreateFile(MYDEVICE, GENERIC_READ | GENERIC_WRITE, 
								0, NULL, OPEN_EXISTING, 
								FILE_ATTRIBUTE_NORMAL,NULL);  
  
    if (hDevice == INVALID_HANDLE_VALUE)  
    {  
        wprintf(L"Failed to open device %s, err: %x\n", MYDEVICE, GetLastError());          
    }  
    else  
    {  
       wprintf(L"Open device %s successfully\n", MYDEVICE);  
	   DWORD dwRet = 0;
	   BOOL b;
       // byte buf[100] = {0};  
       // memcpy(buf, "abcdefghiiukil", 20);  
      // b = WriteFile(hDevice, buf, 23, &dwRet, NULL);  
       //wprintf(L"call WriteFile, ret: %d\n", b);  
  
        //char out_buf[READ_4M] = {0};
		char *kernel_buf = NULL;
		int num;
        dwRet = 0;  
		int i = 0;
		int count = 1;

		while (count != 0)
		{
			printf("transfer data(M):");
			fflush(stdin);
			scanf("%d", &count);
			long f = clock(); //start
			for (i = 0; i < (count*256); i++)
			{
				b = ReadFile(hDevice, p, READ_4M, &dwRet, NULL);
			}
			long g = clock();//end
			printf("speed: %.8f mB/s\n", (count) / ((((double)g - f) / CLOCKS_PER_SEC * 1000) / 1000));
		}
		DeviceIoControl(hDevice, IOCTL_READ, p, 232, kernel_buf, 10, &num, (LPOVERLAPPED)NULL);
		free(p);
		CloseHandle(hDevice);
		printf("Closed handle\n");		
    } 
	
err:

    return 0;  
}  