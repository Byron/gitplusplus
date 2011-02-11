#ifndef GENERATOR_FILTER_HPP
#define GENERATOR_FILTER_HPP

#include <gtl/config.h>
#include <boost/iostreams/filter/aggregate.hpp>
#include <iostream> // debug
GTL_HEADER_BEGIN
GTL_NAMESPACE_BEGIN

/** \note key() will only be provided as non-null key once the filter was closed due to stream-end
  * \note in the current implementation, you have to query the hash before closing the attached sink, 
  *		otherwise the hash so far will be lost.
  */
template <class Hash, class Generator, typename Ch=uchar>
class generator_filter
{
public:
	typedef Hash hash_type;
	typedef Generator generator_type;
	typedef Ch char_type;
	
private:
	bool m_needs_reset;
	generator_type m_generator;
	
	void handle_reset() {
		if (m_needs_reset) {
			m_generator.reset();
			m_needs_reset = false;
		}
	}
	
public:
    struct category
        : boost::iostreams::dual_use,
          boost::iostreams::filter_tag,
          boost::iostreams::multichar_tag,
		  boost::iostreams::closable_tag,
          boost::iostreams::optimally_buffered_tag
        { };
	
    explicit generator_filter()
		: m_needs_reset(false)
    { }

public:
	//! @{ Interface
	void hash(hash_type& outKey) {
		m_generator.hash(outKey);
	}
	
	//! Call before calling hash, in case the stream was no officially closed
	void finalize() throw() {
		try {
			m_generator.finalize();
		} catch (std::exception){}
	}
	
	hash_type hash() {
		hash_type tmp;
		m_generator.hash(tmp);
		return tmp;
	}
	//! @}
	

public:	
    std::streamsize optimal_buffer_size() const { return 1024*4; }

    template<typename Source>
    std::streamsize read(Source& src, char_type* s, std::streamsize n)
    {
        std::streamsize result = boost::iostreams::read(src, s, n);
        if (result == -1)
            return -1;
		
		std::cerr << "read with " << result << "bytes" << std::endl;
		handle_reset();
		m_generator.update(s, result);
        return result;
    }

    template<typename Sink>
    std::streamsize write(Sink& snk, const char_type* s, std::streamsize n)
    {
        std::streamsize result = boost::iostreams::write(snk, s, n);
		handle_reset();
		m_generator.update(s, result);
        return result;
    }
	
    template<typename Sink>
    void close(Sink& sink, BOOST_IOS::openmode which)
    {
		m_needs_reset = true;
    }
};

GTL_NAMESPACE_END
GTL_HEADER_END

#endif // GENERATOR_FILTER_HPP
