#ifndef GENERATOR_STREAM_HPP
#define GENERATOR_STREAM_HPP

#include <gtl/config.h>

GTL_HEADER_BEGIN
GTL_NAMESPACE_BEGIN

/** \note for this type to work properly, one would need the constructor-inheritance feature
  * of the upcoming 0x standard, which unfortunately is not yet implemented in gcc
  */
template <class Hash, class Generator, class StreamBase>
class generator_stream : public StreamBase
{
	// using StreamBase::StreamBase
};


GTL_HEADER_END
GTL_NAMESPACE_END



#endif // GENERATOR_STREAM_HPP
