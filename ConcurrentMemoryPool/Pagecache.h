#pragma once
#include"Common.h"
class Pagecache
{
public:
	Span* NewSpan(size_t num);//num页的span
	Span* _NewSpan(size_t num);//num页的span
	Span* GetIdToSpan(PAGE_ID id);
	void ReleaseSpanToPageCache(Span* span);
	
	static Pagecache& GetInstance()//单例模式——懒汉模式
	{
		static Pagecache pagecaCheInst;
		return pagecaCheInst;
	}
private:
	Pagecache()
	{}
	Pagecache(const Pagecache&) = delete;
	//构造函数，拷贝构造函数私有化，保证全局只有一个
	SpanList _spanlists[MAX_PAGES];
	std::unordered_map<PAGE_ID, Span*> _idSpanMap;//建立span与其所在页号的映射
	std::mutex _mtx;
};
