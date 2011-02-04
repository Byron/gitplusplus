#ifndef GITMODEL_ODB_ALLOC_HPP
#define GITMODEL_ODB_ALLOC_HPP
/** \defgroup ODBAlloc Object Allocators
  * Classes implementing different allocators for objects
  */

#include <gitmodel/config.h>
#include <memory>

GITMODEL_HEADER_BEGIN
GITMODEL_NAMESPACE_BEGIN
	
/** \brief handles memory allocation using virtual memory if necessary
  * \ingroup ODBAlloc
  * Allows definition of a gap, under which the normal heap will be used instead
  * The gap keyword is used to determine when it should use normal new memory allocation, 
  * or when to allocate a chunk of virtual memory.
  * \todo Implement the actual semantics. The cool thing about it is that
  * different memory models, like pools, can easily be used
  */
template <class T, size_t gap>
class vmem_allocator : public std::allocator<T>
{
public:
	static const size_t gGap;
	
public:
	vmem_allocator() throw() { }
	vmem_allocator(const vmem_allocator& rhs) throw()
		: std::allocator<T>(rhs) { }
	template<class T1,size_t gap1> vmem_allocator(const vmem_allocator<T1, gap1>&) throw() { }
	~vmem_allocator() throw() { }
};

template <class T, size_t gap>
const size_t vmem_allocator<T, gap>::gGap(gap);
		
GITMODEL_NAMESPACE_END
GITMODEL_HEADER_END

#endif // GITMODEL_ODB_ALLOC_HPP
