#pragma once

#include <list>
#include <vector>
#include <memory>
#include <exception>
#include <string>
#include <inttypes.h>
#include <cstddef>
#include <cstring>
#include "vmgc/exceptions.h"
#include "vmgc/gcobject.h"
#include <map>
#include <unordered_map>




namespace vmgc {

// 100MB
#define DEFAULT_MAX_GC_HEAP_SIZE 100*1024*1024
#define DEFAULT_MAX_GC_STRPOOL_SIZE 8*1024*1024

#define	DEFAULT_GC_STR_HASHLIMIT 5
#define DEFAULT_MAX_GC_SHORT_STRING_SIZE 32   //2^DEFAULT_GC_HASHLIMIT

#define DEFAULT_MAX_SMALL_BUFFER_SIZE 128  //8的倍数
#define DEFAULT_SMALL_BUFFER_VECTOR_SIZE (DEFAULT_MAX_SMALL_BUFFER_SIZE/8)     

	struct GcObject;

	struct Buffer {
		intptr_t pos;
		ptrdiff_t size;
		Buffer(intptr_t p, ptrdiff_t s) {
			pos = p;
			size = s;
		}

		Buffer(const Buffer& b) {
			pos = b.pos;
			size = b.size;
		}
	};
	struct Block {
		intptr_t blockpos;
		ptrdiff_t blocksize;

		Block(intptr_t p=0, ptrdiff_t s=0) {
			blockpos = p;
			blocksize = s;
		}

		Block(const Block& b) {
			blockpos = b.blockpos;
			blocksize = b.blocksize;
		}

		bool operator < (const Block &o) const
		{
			return blockpos < o.blockpos ;
		}

	};

	//typedef Buffer Block;

	struct GcBuffer {
		intptr_t pos;
		ptrdiff_t size;
		bool isGcObj;
		//bool isFree;
	};

	class Hasher {
	public:
		size_t operator()(const Block& b) const {
			//calculate hash here.
			return std::hash<long long>()(b.blockpos);
		}
	};
	class Equal
	{
	public:
		bool operator()(const Block& b1, const Block& b2) const {
			return (b1.blockpos == b2.blockpos);
		}
	};


    class GcState {
	private:
		ptrdiff_t _total_malloced_blocks_size;
		ptrdiff_t _used_size;
		ptrdiff_t _max_gc_size;

		std::shared_ptr<std::unordered_map<Block, std::vector<GcBuffer>, Hasher, Equal>> _malloced_blocks_buffers;
		std::shared_ptr<std::vector<std::pair<Block, Buffer>>> _empty_small_buffers[DEFAULT_SMALL_BUFFER_VECTOR_SIZE]; //empty_size => [ [start_ptr, size], ... ]
		std::shared_ptr<std::vector<std::pair<Block, Buffer>>> _empty_big_buffers; //无序的
		std::shared_ptr<std::vector<std::pair<intptr_t, intptr_t> > > _malloced_str_blocks; // [ [start_ptr, size], ... ]
		std::shared_ptr<std::unordered_map<unsigned int,std::pair<intptr_t, ptrdiff_t>>> _gc_strpool;
		std::pair<intptr_t, ptrdiff_t>  _empty_str_buffer; // [start_ptr, size]
		void insert_empty_buffer(const Buffer &buf, const Block &block);
		void GcState::insert_malloced_blocks_buffers(Block &block, GcBuffer& gcbuf, bool isNewBlock);

	public:
		// @throws vmgc::GcException
		GcState(ptrdiff_t max_gc_heap_size = DEFAULT_MAX_GC_HEAP_SIZE);
		virtual ~GcState();

		void* gc_malloc(size_t size, bool isGcObj=false);
		void gc_free(void* p);
		void gc_free_array(void* p, size_t count, size_t element_size);
		void* gc_realloc(void *p, size_t oldsize, size_t newsize);
		void* gc_malloc_vector(size_t count, size_t element_size);
		void* gc_grow_vector(void *p, size_t nelements, size_t* size, size_t element_size, size_t limit);
		ptrdiff_t usedsize() const;
		void gc_free_all();
		void* gc_intern_strpool(size_t sz, size_t strsize, const char* str, bool* isNewStr);

		template <typename T>
		T* gc_new_object()
		{
			size_t sz = sizeof(T);
			auto p = gc_malloc(sz,true);
			if (!p) {
				return nullptr;
			}
			GcObject* obj_p = static_cast<GcObject*>(p);
			new (obj_p)T();
			obj_p->tt = T::type;
			//_gc_objects->push_back((intptr_t) p - (intptr_t) _pstart);
			return static_cast<T*>(obj_p);
		}

		template <typename T>
		T* gc_new_object_vector(size_t count)
		{
			if (count <= 0)
				return nullptr;
			size_t sz = sizeof(T);
			std::vector<T*> malloced_items;
			for (size_t i = 0; i < count; i++) {
				auto p = gc_malloc(sz,true);
				if (!p) {
					for (const auto& item : malloced_items) {
						gc_free(item);
					}
					return nullptr;
				}
				GcObject* obj_p = static_cast<GcObject*>(p);
				new (obj_p)T();
				obj_p->tt = T::type;
				//_gc_objects->push_back((intptr_t)p - (intptr_t)_pstart);
				T* t_p = static_cast<T*>(obj_p);
				malloced_items.push_back(t_p);
			}
			return *(malloced_items.begin());
		}


		#define GC_MINSIZEARRAY 4
		template <typename T>
		T* gc_grow_object_vector(T* p, size_t nelements, size_t* size, size_t limit)
		{
			if (nelements + 1 <= *size)
				return p;
			size_t newsize;
			if ((*size) >= limit / 2) {  /* cannot double it? */
				if ((*size) >= limit)  /* cannot grow even a little? */
					throw GcException(std::string("too many objects in gc vector (limit is ") + std::to_string(limit) + ")");
				newsize = limit;  /* still have at least one free place */
			}
			else {
				newsize = (*size) * 2;
				if (newsize < GC_MINSIZEARRAY)
					newsize = GC_MINSIZEARRAY;  /* minimum size */
			}
			T* new_p = gc_new_object_vector<T>(newsize);
			if (!new_p) {
				return nullptr;
			}
			if (p) {
				for (size_t i = 0; i < (*size); i++) {
					*(new_p + i) = *(p + i);
				}
			}
			if (p || (*size) > 0) {
				gc_free_array(p, *size, sizeof(T));
			}
			*size = newsize;
			return new_p;
		}

		void fill_gc_string(GcObject* p, const char* str, size_t size);

		// short string into str pool, reused
		template <typename T>
		T* gc_intern_string(const char* str, size_t size)
		{
			T* ts = nullptr;
			bool isNewStr = true;
			if (size < DEFAULT_MAX_GC_SHORT_STRING_SIZE) { 
				size_t sz = sizeof(T);
				auto p = gc_intern_strpool(sz, size, str, &isNewStr);
				if (!p) {
					ts = gc_new_object<T>();
					fill_gc_string(ts, str, size);
					return ts;
				}
				GcObject* obj_p = static_cast<GcObject*>(p);
				if (isNewStr) {
					new (obj_p)T();
					fill_gc_string(obj_p, str, size);
					obj_p->tt = T::type;
				}
				ts = static_cast<T*>(obj_p);
			}
			else {
				ts = gc_new_object<T>();
				fill_gc_string(ts, str, size);
			}
			return ts;
		}


    };
}
