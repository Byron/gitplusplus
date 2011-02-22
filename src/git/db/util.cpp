#include <git/db/util.hpp>
#include <git/obj/stream.h>

GIT_NAMESPACE_BEGIN

uchar loose_object_header(	typename git_object_policy_traits::char_type* hdr, 
							typename git_object_policy_traits::object_type type, 
							typename git_object_policy_traits::size_type size)
{
	io::stream<io::basic_array_sink<typename git_object_policy_traits::char_type> > stream(hdr, hdr+32);
	stream << type << " " << size << '\0';
	
	return (uchar)stream.tellp();
}
		
GIT_NAMESPACE_END
