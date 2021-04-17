#define _CRT_SECURE_NO_WARNINGS 1

#include"ThreadCache.h"
#include"CentralCache.h"
void* ThreadCache::Allocte(size_t size)//申请内存
{
	size_t index = Sizeclass::ListIndex(size);//计算申请内存在freelist中的下标
	FreeList& freeList = _freelist[index];
	if (!freeList.Empty())//线程缓存中有内存
		return freeList.Pop();
	else//没有内存,要从CentralCache取内存
	{
		return FetchFromCentralCache(Sizeclass::RoundUp(size));//根据size的大小还要进行对齐
		//为什么要有CentralCache?为了均衡资源和为内存碎片做准备
	} 
}
 
//释放内存，不是真的释放内存，而是把不用的再重新再挂到freelist中
void ThreadCache::Deallocte(void* ptr,size_t size)
{
	size_t index = Sizeclass::ListIndex(size);
	//根据内存大小计算其在自由链表中的下标
	FreeList& freeList = _freelist[index];
	freeList.Push(ptr);//挂上自由链表

	//对象个数满足之前内存对齐申请的一批个数，也就那一批就都使用了
	size_t num = Sizeclass::NumMoveSize(size);
	if (freeList.Num() >= num)
	{//当释放的内存太多，就直接还回CentralCache
		ListTooLong(freeList,num,size);
	}
}
void ThreadCache::ListTooLong(FreeList& freeList, size_t num,size_t size)
{
	void* start = nullptr, * end = nullptr;
	//先从自由链表弹出
	freeList.PopRange(start, end, num);

	NextObj(end) = nullptr;
	//归给下一层
	CentralCache::GetInsatnce().ReleaseListToSpans(start,size);
}
//独立测试threadcache
//从中心缓存获取Num个对象，返回其中一个的指针，剩下的num-1被挂到freelist中等待申请
//void* ThreadCache::FetchFromCentralCache(size_t index)
//{
//	//模拟取内存对象 的代码，测试ThreadCache的逻辑
//	size_t num = 20;//与单个对象的大小有关，还要进行调整，获取批量的内存
//	size_t size = (index + 1) * 8;//计算单个内存的大小
//	char* start = (char*)malloc(size*num);
//	char* cur = start; 
//	for (size_t i = 0; i < num - 1; ++i)//将其连起来//为什么是num-1
//	{
//		char* next = cur + size;
//		NextObj(cur) = next;
//		cur = next;
//	}
//	NextObj(cur) = nullptr;//最后一个要指向空
//	void* head = NextObj(start);
//	void* tail = cur;
//
//	_freelist[index].PushRange(head, tail);
//
//	return start;
//}

void* ThreadCache::FetchFromCentralCache(size_t size)
{
	//根据一次要申请内存的大小分配一定数量的内存对象
	//要的小了多给几个，要的大的少给几个
	size_t num = Sizeclass::NumMoveSize(size);
	//从centralcache中获取内存
	void* start = nullptr;
	void* end = nullptr;
	//ActuallNum 实际给内存对象的个数
	size_t ActuallNum = CentralCache::GetInsatnce().FetchRangeObj(start, end, num, size);
	if (ActuallNum == 1)//至少获取一个内存对象，因为0个也就是申请失败会抛异常
		return start;
	else//多个内存对象被申请
	{
		size_t index = Sizeclass::ListIndex(size);
		FreeList& list = _freelist[index];
		list.PushRange(NextObj(start), end, ActuallNum-1);//取走要用的一个，再把剩下的挂起来，备用
		return start;
	}
}