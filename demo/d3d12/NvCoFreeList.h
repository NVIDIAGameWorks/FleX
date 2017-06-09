/* Copyright (c) 2016, NVIDIA CORPORATION.  All rights reserved.
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited. */

#ifndef NV_CO_FREELIST_H
#define NV_CO_FREELIST_H

#include <assert.h>
#include <stdint.h>

/** \addtogroup common
@{
*/

namespace nvidia {
namespace Common {

#if NV_DEBUG
#	define NV_CO_FREE_LIST_INIT_MEM
#endif

#ifdef NV_CO_FREE_LIST_INIT_MEM
#	define NV_CO_FREE_LIST_INIT_ALLOCATE(ptr) _initAllocate(ptr);
#	define NV_CO_FREE_LIST_INIT_DEALLOCATE(ptr) _initDeallocate(ptr);
#else
#	define NV_CO_FREE_LIST_INIT_ALLOCATE(ptr)
#	define NV_CO_FREE_LIST_INIT_DEALLOCATE(ptr) 
#endif


/*! \brief A freelist is a simple and fast memory allocator that can allocate and free in any order identically sized blocks. 

\details A free list is a memory allocation system that performs allocations/deallocations very quickly for elements which are
all the same size.
In a freelist all elements are the same size, and elements can be allocated and freed in any order, as long as every deallocation
matches every allocation. Both allocation and deallocation are O(1), and generally just a few instructions. The underlying
memory allocator will allocate in large blocks, with multiple elements amortizing a more costly large allocation against lots
of fast small element allocations. */
class FreeList
{
	//NV_CO_DECLARE_CLASS_BASE(FreeList);
public:
		/// Free elements are held in a singly linked list. The minimum size of an element is therefore a pointer
	struct Element
	{
		Element* m_next;
	};
	struct Block
	{
		Block* m_next;				///< The next block
		uint8_t* m_data;				///< The list of the elements each m_elementSize in size
	};

		/// Allocate a single element
	inline void* allocate();
		/// Deallocate a block that was previously allocated with allocate
	inline void deallocate(void* data);

		/// Returns true if this is from a valid allocation
	bool isValidAllocation(const void* dataIn) const;

		/// Get the element size
	inline size_t getElementSize() const { return m_elementSize; }
		/// Get the total size of each individual block allocation in bytes
	inline size_t getBlockSize() const { return m_blockSize; }

		/// Deallocates all elements
	void deallocateAll();
		/// Deallocates all, and frees any backing memory (put in initial state)
	void reset();

		/// Initialize. If called on an already initialized heap, the heap will be deallocated.
	void init(size_t elementSize, size_t alignment, size_t elemsPerBlock);

		/// Default Ctor
	FreeList() { _init(); }
		/// Ctor
	FreeList(size_t elementSize, size_t alignment, size_t elemsPerBlock) { _init(elementSize, alignment, elemsPerBlock); }
		/// Dtor
	~FreeList();

	protected:
		/// Initializes assuming freelist is not constructed
	void _init(size_t elementSize, size_t alignment, size_t elemsPerBlock);
	void* _allocate();
	void _deallocateBlocks(Block* block);
		/// Initializes setting everything to empty (doesn't free anything if already allocated)
	void _init();

	inline static size_t _calcAlignedBlockSize(size_t align) { return (sizeof(Block) + align - 1) & ~(align - 1); }

	void _initAllocate(void* mem);
	void _initDeallocate(void* mem);

	uint8_t* m_top;					///< The top position of the current block
	uint8_t* m_end;					///< The end of the current block

	Block* m_activeBlocks;			///< The blocks there are potentially allocations from
	Block* m_freeBlocks;			///< Blocks that there are no allocations in

	Element* m_freeElements;		///< A singly linked list of elements available

	size_t m_elementSize;
	size_t m_alignment;
	size_t m_blockSize;
	size_t m_blockAllocationSize;	///< The actual allocation size. Maybe bigger than m_blockSize if alignment requires it.
	//MemoryAllocator* m_allocator;
};

// --------------------------------------------------------------------------
inline void* FreeList::allocate()
{
	// First see if there are any freeElements ready to go
	{
		Element* element = m_freeElements;
		if (element)
		{
			m_freeElements = element->m_next;
			NV_CO_FREE_LIST_INIT_ALLOCATE(element)
			return element;
		}
	}
	if (m_top >= m_end)
	{
		return _allocate();
	}
	void* data = (void*)m_top;
	NV_CO_FREE_LIST_INIT_ALLOCATE(data)

	m_top += m_elementSize;
	return data;
}
// --------------------------------------------------------------------------
inline void FreeList::deallocate(void* data)
{
	assert(isValidAllocation(data));

	NV_CO_FREE_LIST_INIT_DEALLOCATE(data)

	// Put onto the singly linked free element list
	Element* ele = (Element*)data;
	ele->m_next = m_freeElements;
	m_freeElements = ele;
}

} // namespace Common 
} // namespace nvidia

/** @} */

#endif // NV_CO_FREELIST_H