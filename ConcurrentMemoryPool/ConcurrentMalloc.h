#pragma once

#include"ThreadCache.h"
#include"CentralCache.h"
#include"Pagecache.h"
static void* ConcurrentMalloc(size_t size)
{
	
	if (size <= MAX_SIZE)//[1bytes,64kB]�����Ļ����ȡ
	{
		if (pThreadCache == nullptr)//�̵߳�һ�ν���
		{
			pThreadCache = new ThreadCache();
			//cout << std::this_thread::get_id() << "->" << pThreadCache << endl;
		}
		return pThreadCache->Allocte(size);
	}
	else if (size <= ((MAX_PAGES - 1) << PAGE_SHIFT))//��64kB,128*4kB]��ҳ�����ȡ
	{
		size_t align_size = Sizeclass::_RoundUP(size, 1<<PAGE_SHIFT);
		size_t pagenum = (align_size >> PAGE_SHIFT);
		Span* span = Pagecache::GetInstance().NewSpan(pagenum);
		span->_objSize = align_size;
		void* ptr = (void*)(span->_pageid << PAGE_SHIFT);
		return ptr;
	}
	else//[128*4kb,��]��ϵͳ����
	{
		size_t align_size = Sizeclass::_RoundUP(size, 1 << PAGE_SHIFT);
		size_t pagenum = (align_size >> PAGE_SHIFT);
		SystemAlloc(pagenum);
	}

	return pThreadCache->Allocte(size);
}
static void ConcurrentFree(void* ptr)
{
	PAGE_ID pageid = (PAGE_ID)ptr >> PAGE_SHIFT;
	Span* span = Pagecache::GetInstance().GetIdToSpan(pageid);
	if (span == nullptr)//û��ӳ�䣬˵���ڴ�����ϵͳ
	{
		SystemFree(ptr);
		return;
	}
	size_t size = span->_objSize;
	if (size <= MAX_SIZE)//[1bytes,64kB]�����Ļ����ȡ
		pThreadCache->Deallocte(ptr,size);
	else if (size <= ((MAX_PAGES - 1) << PAGE_SHIFT))//��64kB,128*4kB]��ҳ�����ȡ
	{
		Pagecache::GetInstance().ReleaseSpanToPageCache(span);
	}
}