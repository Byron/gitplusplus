#include "git/obj/blob.h"

GIT_NAMESPACE_BEGIN

Blob::Blob(std::istream *stream)
	: Object(Object::Type::Blob)
	, m_stream(stream)
{
}

Blob::~Blob()
{
	if (m_stream){
		delete m_stream;
		m_stream = 0;
	}
}


GIT_NAMESPACE_END
