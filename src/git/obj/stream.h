#ifndef GIT_OBJ_STREAM_H
#define GIT_OBJ_STREAM_H

#include <git/config.h>
#include <iostream>

#include <git/obj/object.hpp>
#include <git/db/util.hpp>

GIT_HEADER_BEGIN
GIT_NAMESPACE_BEGIN

//! \brief Thrown for errors during deserialization
class DeserializationError : public ObjectError
{};

//! \brief Thrown for errors during serialization
class SerializationError :	public ObjectError
{};

git_basic_ostream& operator << (git_basic_ostream& stream, const Actor& inst);
git_basic_istream& operator >> (git_basic_istream& stream, Actor& inst);

git_basic_ostream& operator << (git_basic_ostream& stream, const TimezoneOffset& inst);
git_basic_istream& operator >> (git_basic_istream& stream, TimezoneOffset& inst);

git_basic_ostream& operator << (git_basic_ostream& stream, const ActorDate& inst);
git_basic_istream& operator >> (git_basic_istream& stream, ActorDate& inst);

//! Convert strings previously generated using operator >>
//! \note if the given token does not resolve to a type, Type::None will be returned.
git_basic_istream& operator >> (git_basic_istream& stream, Object::Type& type);

//! Write type enumeration members into a stream as string, using a format compatible
//! to be used in git loose objects
git_basic_ostream& operator << (git_basic_ostream& stream, Object::Type type);

GIT_NAMESPACE_END
GIT_HEADER_END


#endif // GIT_OBJ_STREAM_H
