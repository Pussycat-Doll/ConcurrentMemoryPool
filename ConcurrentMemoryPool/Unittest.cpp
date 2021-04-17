#define _CRT_SECURE_NO_WARNINGS 1
#include"ThreadCache.h"

#include<vector>
void UnitThreadCache1()
{
	ThreadCache tc;
	std::vector<void* > v;
	for (size_t i = 0; i < 21; ++i)
	//为什么要申请21次内存？
		//(num==20)让程序运行的时候既走if语句，也走一else语句，都试试(白盒测试)
	{
		v.push_back(tc.Allocte(7));//申请7个字节
	}
	for (size_t i = 0; i < 21; ++i)
	{
		printf("[%d]->%p\n", i, v[i]);
	}
	for (auto ptr : v)
	{
		tc.Deallocte(ptr, 7);
	}
}
void UnitTestSizeClass()
{
	/*
	[1,128] 8byte对齐 freelist[0,16) <--- 128/8 = 16
	[129,1024] 16byte对齐 freelist[16,72) <--- (1024-128)/16 + 16= 72
	[1025,8*1024] 128byte对齐 freelist[72,128) <--- (8*1024-1024)/128 + 72 = 128
	[8*1024 + 1,64*1024] 1024byte对齐 freelist[128,184)<---(64*1024 - 8*1024)/1024 + 128 = 184
	根据条件来测试每一种情况
	*/
	cout << Sizeclass::RoundUp(1) << endl;
	cout << Sizeclass::RoundUp(127) << endl<<endl;

	cout << Sizeclass::RoundUp(129) << endl;
	cout << Sizeclass::RoundUp(1023) << endl<<endl;

	cout << Sizeclass::RoundUp(1025) << endl;
	cout << Sizeclass::RoundUp(8*1024-1) << endl << endl;

	cout << Sizeclass::RoundUp(8 * 1024 + 1) << endl;
	cout << Sizeclass::RoundUp(64 * 1024) << endl << endl;

	cout << Sizeclass::ListIndex(1) << endl;
	cout << Sizeclass::ListIndex(127) << endl << endl;

	cout << Sizeclass::ListIndex(129) << endl;
	cout << Sizeclass::ListIndex(1023) << endl << endl;

	cout << Sizeclass::ListIndex(1025) << endl;
	cout << Sizeclass::ListIndex(8 * 1024-1) << endl << endl;

	cout << Sizeclass::ListIndex(8 * 1024 + 1) << endl;
	cout << Sizeclass::ListIndex(64 * 1024) << endl << endl;
}
void UnitTestSystemAlloc()
{
	//检测指针ptr是4k对齐的
	void* ptr = SystemAlloc(MAX_PAGES - 1);
	//根据地址算页号
	PAGE_ID id = (PAGE_ID)ptr >> PAGE_SHIFT;
	//再根据页号算起始地址
	void* ptrshift = (void*)(id << PAGE_SHIFT);

	//按照8个字节切对象
	char* obj1 = (char*)ptr;
	char* obj2 = (char*)ptr + 8;
	char* obj3 = (char*)ptr + 16;
	PAGE_ID id1 = (PAGE_ID)obj1 >> PAGE_SHIFT;
	PAGE_ID id2 = (PAGE_ID)obj2 >> PAGE_SHIFT;
	PAGE_ID id3 = (PAGE_ID)obj3 >> PAGE_SHIFT;
}
void UnitTestThreadCache2()
{
	ThreadCache tc;
	std::vector<void* > v;
	size_t sz = 7;
	for (size_t i = 0; i < Sizeclass::NumMoveSize(sz); ++i)
		//为什么要申请21次内存？
			//(num==20)让程序运行的时候既走if语句，也走一else语句，都试试(白盒测试)
	{
		v.push_back(tc.Allocte(sz));//申请sz个字节
	}
	for (size_t i = 0; i < 21; ++i)
	{
		printf("[%d]->%p\n", i, v[i]);
	}
	for (auto ptr : v)
	{
		tc.Deallocte(ptr, 7);
	}
}
#include "ConcurrentMalloc.h"
void func1()
{
	ThreadCache tc;
	std::vector<void* > v;
	size_t sz = 7;
	for (size_t i = 0; i < Sizeclass::NumMoveSize(sz)+1; ++i)
		//为什么要申请21次内存？
			//(num==20)让程序运行的时候既走if语句，也走一else语句，都试试(白盒测试)
	{
		v.push_back(ConcurrentMalloc(sz));//申请sz个字节
	}
	for (size_t i = 0; i < 21; ++i)
	{
		printf("[%d]->%p\n", i, v[i]);
	}
	for (auto ptr : v)
	{
		ConcurrentFree(ptr);
	}
}
//int main()
//{
//	//UnitTestThreadCache2();
//	//UnitTestSystemAlloc();
//	//UnitTestSizeClass();
//	//UnitThreadCache();
//	
//	void* ptr1 = ConcurrentMalloc(1);
//	void* ptr2 = ConcurrentMalloc(65 << PAGE_SHIFT);
//	void* ptr3 = ConcurrentMalloc(129<<PAGE_SHIFT);
//
//	ConcurrentFree(ptr1);
//	ConcurrentFree(ptr2);
//	ConcurrentFree(ptr3);
//	system("pause");
//	return 0;
//}