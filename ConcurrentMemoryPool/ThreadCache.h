#pragma once
#include"Common.h"

class ThreadCache
{
public:
	//�����ڴ���ͷ��ڴ�
	void* Allocte(size_t size);
	void Deallocte(void* ptr, size_t size);
	//�����Ļ����ȡ�ڴ�
	void* FetchFromCentralCache(size_t index);
	//������������ж��󳬹�һ�����Ⱦ�Ҫ�ͷŸ����Ļ���
	void ListTooLong(FreeList& freeList, size_t num,size_t size);
private:
	FreeList _freelist[NFREELIST];//���ָ�������(ӳ���) 
};

_declspec (thread) static ThreadCache*  pThreadCache = nullptr;