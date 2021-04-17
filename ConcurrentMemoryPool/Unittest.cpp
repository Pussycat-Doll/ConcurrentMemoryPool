#define _CRT_SECURE_NO_WARNINGS 1
#include"ThreadCache.h"

#include<vector>
void UnitThreadCache1()
{
	ThreadCache tc;
	std::vector<void* > v;
	for (size_t i = 0; i < 21; ++i)
	//ΪʲôҪ����21���ڴ棿
		//(num==20)�ó������е�ʱ�����if��䣬Ҳ��һelse��䣬������(�׺в���)
	{
		v.push_back(tc.Allocte(7));//����7���ֽ�
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
	[1,128] 8byte���� freelist[0,16) <--- 128/8 = 16
	[129,1024] 16byte���� freelist[16,72) <--- (1024-128)/16 + 16= 72
	[1025,8*1024] 128byte���� freelist[72,128) <--- (8*1024-1024)/128 + 72 = 128
	[8*1024 + 1,64*1024] 1024byte���� freelist[128,184)<---(64*1024 - 8*1024)/1024 + 128 = 184
	��������������ÿһ�����
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
	//���ָ��ptr��4k�����
	void* ptr = SystemAlloc(MAX_PAGES - 1);
	//���ݵ�ַ��ҳ��
	PAGE_ID id = (PAGE_ID)ptr >> PAGE_SHIFT;
	//�ٸ���ҳ������ʼ��ַ
	void* ptrshift = (void*)(id << PAGE_SHIFT);

	//����8���ֽ��ж���
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
		//ΪʲôҪ����21���ڴ棿
			//(num==20)�ó������е�ʱ�����if��䣬Ҳ��һelse��䣬������(�׺в���)
	{
		v.push_back(tc.Allocte(sz));//����sz���ֽ�
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
		//ΪʲôҪ����21���ڴ棿
			//(num==20)�ó������е�ʱ�����if��䣬Ҳ��һelse��䣬������(�׺в���)
	{
		v.push_back(ConcurrentMalloc(sz));//����sz���ֽ�
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