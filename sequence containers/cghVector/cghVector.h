/*******************************************************************
*  Copyright(c) 2016 Chen Gonghao
*  All rights reserved.
*
*  chengonghao@yeah.net
*
*  功能:cghVector容器的实现代码
******************************************************************/

#ifndef _CGH_VECTOR_
#define _CGH_VECTOR_

#include "globalConstruct.h"
#include "cghAlloc.h"
#include <memory>

namespace CGH
{
	template<class T, class Alloc = cghAllocator<T>>
	class cghVector
	{
	public:
		typedef T				value_type;
		typedef value_type*		pointer;
		typedef value_type*		iterator;
		typedef value_type&		reference;
		typedef size_t			size_type;
		typedef ptrdiff_t		difference_type;

		//typedef __false_type	has_trivial_default_constructor;
		//typedef __false_type	has_trivial_copy_destructor;
		//typedef __false_type	has_trivial_assignment_operator;
		//typedef __false_type	has_trivial_destructor;
		//typedef __false_type	is_POD_type;

	protected:
		typedef simple_alloc<value_type, Alloc> data_allocator; // 定义空间配置器
		iterator start;
		iterator finish;
		iterator end_of_storage;

		void insert_aux(iterator position, const T& value)
		{
			if (finish != end_of_storage)
			{
				construct(finish, *(finish - 1));
				++finish;
				T x_copy = value;
				std::copy_backward(position, finish - 2, finish - 1);
				*position = x_copy;
			}
			else
			{
				ptrdiff_t old_size = size();
				const size_type len = old_size != 0 ? 2 * old_size : 1;
				/*
					配置原则：如果原大小为0，则配置1个元素大小
					如果原大小不为0，则配置原大小的两倍
				*/
				iterator new_start = data_allocator::allocate(len);
				iterator new_finish = new_start;
				try
				{
					// 把 start 到 position 这段内存拷贝到 new_start 处,返回 new_finish = new_start + ( position - start )
					new_finish = std::uninitialized_copy(start, position, new_start); 
					construct(new_finish, value); // 在 new_finish 处构造新元素
					++new_finish;
					//new_finish = std::uninitialized_copy(position, finish, new_finish);
				}
				catch (std::exception ex)
				{
					// 如果执行失败就要回滚
					destroy(new_start, new_finish);
					data_allocator::deallocate(new_start, len);
					throw;
				}
				destroy(begin(), end());
				deallocate();
				start = new_start;
				finish = new_finish;
				end_of_storage = new_start + len;
			}
		}

		#pragma region 释放内存和析构元素
		void deallocate()
		{
			if (start)
			{
				data_allocator::deallocate(start);
			}
		}
		#pragma endregion

		#pragma region 分配内存和构造元素
		/*
		 *	fill_initialize和allocate_and_fill把cghVector的初始化分为了两步：
		 *	1.fill_initialize的职责是分配一段内存
		 *	2.fill_initialize调用allocate_and_fill，在分配的内存中调用构造函数创建cghVector的元素
		 */
		void fill_initialize(size_type n, const T& value)
		{
			start = allocate_and_fill(n, value);
			finish = start + n;
			end_of_storage = finish;
		}

		iterator allocate_and_fill(size_type n, const T& x)
		{
			iterator result = data_allocator::allocate(n);
			iterator cur = result;
			for (; n > 0; --n, ++cur)
			{
				construct(&*cur, x);
			}
			return result;
		}
		#pragma endregion
	public:
		#pragma region 对cghVector的读操作
		iterator begin(){ return start; } // 返回cghVector头元素的地址
		iterator end(){ return finish; } // 返回cghVector尾元素的地址
		size_type size(){ return size_type(int(end() - begin())); } // cghVector的长度 = 尾元素地址 - 头元素地址
		size_type capacity()const{ return size_type(end_of_storage - start); } // cghVector的容量 = 最大容量地址 - 头元素地址
		bool empty()const { return begin() == end(); } // cghVector是否为空：头元素地址 是否等于 尾元素地址
		reference operator[](size_type n){ return *(begin() + n); } // 返回指定位置的元素引用
		reference front(){ return *begin(); } // 获得头元素位职
		reference back(){ return *(end() - 1); } // 获得尾元素位置
		#pragma endregion

		#pragma region 构造函数
		cghVector() :start(0), finish(0), end_of_storage(0) { } // 初始化空的cghVector
		cghVector(size_type n, const T& value){ fill_initialize(n, value); } // 初始化包含n个值为value的cghVector
		cghVector(int n, const T& value){ fill_initialize(n, value); } // 同上
		cghVector(long n, const T& value){ fill_initialize(n, value); } // 同上
		explicit cghVector(size_type n){ fill_initialize(n, T()); } // 初始化cghVector的长度为n
		#pragma endregion 

		#pragma region 析构函数
		~cghVector()
		{
			destroy(start, finish); // 先调用cghVector中元素的析构函数
			deallocate(); // 再释放cghVector占用的内存
		}
		#pragma endregion

		#pragma region 对cghVector的写操作
		/**
		*  弹出尾元素
		*/
		void pop_back() 
		{
			--finish;
			destroy(finish);
		}

		/**
		*  在cghVector末尾插入一个元素
		*/
		void push_back(const T& x)
		{
			// 判断cghVector的容量是否满了，如果没满我们直接在已有的内存区域上构造元素
			if (finish != end_of_storage)
			{
				construct(finish, x);
				++finish;
			}
			else // 如果满了我们就要重新分配内存并重新构造函数
			{
				insert_aux(end(), x);
			}
		}

		/**
		*  清除[first, last)区间内的元素
		*/
		iterator erase(iterator first, iterator last)
		{
			// 把last到finish这段元素拷贝以first作为起始处的内存空间，返回first + ( finish - last ) 的地址
			iterator i = std::copy(last, finish, first);
			destroy(i, finish); // 销毁(first + ( finish - last ), finish]这段内存
			finish = finish - (last - first); // 重新设置finish
			return first;
		}

		/**
		*  清除某个位置上的元素
		*/
		iterator erase(iterator position)
		{
			if (position + 1 != end())
			{
				copy(position + 1, finish, position);
			}
			--finish;
			destroy(finish);
			return position;
		}

		/* 从position开始，插入n个元素，元素初值为x */
		void insert(iterator position, size_type n, const T& x)
		{
			if (n != 0) // 当n != 0 时才插入
			{
				if (size_type(end_of_storage - finish) >= n) // 备用空间大于新增元素个数
				{
					T x_copy = x;
					const size_type elems_after = finish - position;
					iterator old_finish = finish;
					if (elems_after > n)
					{
						uninitialized_copy(finish - n, finish, finish);
						finish += n;
						copy_backward(position, old_finish - n, old_finish);
						fill(position, position + n, x_copy);
					}
					else
					{
						uninitialized_fill_n(finish, n - elems_after, x_copy);
						finish += n - elems_after;
						uninitialized_copy(position, old_finish, finish);
						finish += elems_after;
						fill(position, old_finish, x_copy);
					}
				}
				else
				{
					const size_type old_size = size();
					const size_type len = old_size + max(old_size, n);
					iterator new_start = data_allocator::allocate(len);
					iterator new_finish = new_start;
					new_finish = uninitialized_copy(start, position, new_start);
					new_finish = uninitialized_fill_n(new_finish, n, x);
					new_finish = uninitialized_copy(position, finish, new_finish);
					destroy(start, finish);
					deallocate();
					start = new_start;
					finish = new_finish;
					end_of_storage = new_start + len;
				}
			}
		}

		void swap(cghVector<T, Alloc>& __x) {
			std::swap(start, __x.start);
			std::swap(finish, __x.finish);
			std::swap(end_of_storage, __x.end_of_storage);
		}
		#pragma endregion

		#pragma region heap
		inline void make_heap(iterator first, iterator last)
		{
			if (last - first < 2)return;
			difference_type len = last - first;
			difference_type parent = len / 2;
			while (true)
			{
				_adjust_heap(first, parent, len, T(*(first + parent)));
				if (parent == 0)return;
				--parent;
			}
		}

		inline void push_heap(iterator first, iterator last)
		{
			difference_type holeIndex = last - first - 1;
			difference_type topIndex = 0;
			T value = T(*(last - 1));
			_push_heap(first, holeIndex, topIndex, value);
		}

		inline void pop_heap(iterator first, iterator last)
		{
			__pop_heap(first, last - 1, last - 1, T(*(last - 1)));
		}

		inline void sort_heap(iterator first, iterator last)
		{
			while (last - first > 1)
			{
				pop_heap(first, last -- );
			}
		}
	protected:
		void __pop_heap(iterator first, iterator last, iterator result, T value)
		{
			*result = *first;
			_adjust_heap(first, difference_type(0), difference_type(last - first), value);
		}

		void _adjust_heap(iterator first, difference_type holeIndex, difference_type len, T value)
		{
			difference_type topIndex = holeIndex;
			difference_type secondChild = 2 * holeIndex + 2;
			while (secondChild < len)
			{
				if (*(first + secondChild) < *(first + (secondChild - 1)))
				{
					--secondChild;
				}
				*(first + holeIndex) = *(first + secondChild);
				holeIndex = secondChild;
				secondChild = 2 * (secondChild + 1);
			}
			if (secondChild == len)
			{
				*(first + holeIndex) = *(first + (secondChild - 1));
				holeIndex = secondChild - 1;
			}
			_push_heap(first, holeIndex, topIndex, value);
		}

		void _push_heap(iterator first, difference_type holeIndex, difference_type topIndex, T value)
		{
			difference_type parent = (holeIndex - 1) / 2;
			while (holeIndex>topIndex && *(first + parent) < value)
			{
				*(first + holeIndex) = *(first + parent);
				holeIndex = parent;
				parent = (holeIndex - 1) / 2;
			}
			*(first + holeIndex) = value;
		}
		#pragma endregion
	};
}

#endif