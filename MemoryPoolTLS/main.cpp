// MemoryPoolTLS.cpp : 콘솔 응용 프로그램에 대한 진입점을 정의합니다.
//

#include "stdafx.h"

CMemoryPoolTLS<int> *pMemoryPoolTLS;

unsigned __stdcall Thread(LPVOID param)
{
	int *p = nullptr;

	for (int iCnt = 0; iCnt < 1000; iCnt++)
	{
		p = pMemoryPoolTLS->Alloc();
		*p = iCnt;

		printf("Thread ID : %d			Value : %d\n", GetCurrentThreadId(), iCnt);
		Sleep(10);

		pMemoryPoolTLS->Free(p);
	}

	return 0;
}

int _tmain(int argc, _TCHAR* argv[])
{
	HANDLE hThread1, hThread2;
	DWORD dwThreadID;

	pMemoryPoolTLS = new CMemoryPoolTLS<int>();

	hThread1 = (HANDLE)_beginthreadex(
		NULL,
		0,
		Thread,
		0,
		0,
		(unsigned int *)&dwThreadID
		);

	hThread2 = (HANDLE)_beginthreadex(
		NULL,
		0,
		Thread,
		0,
		0,
		(unsigned int *)&dwThreadID
		);
	
	while (1){};

	return 0;
}

