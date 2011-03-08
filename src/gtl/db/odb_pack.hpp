#ifndef GTL_ODB_PACK_HPP
#define GTL_ODB_PACK_HPP
#include <gtl/config.h>
#include <gtl/db/odb.hpp>

#include <boost/iostreams/device/file.hpp>

GTL_HEADER_BEGIN
GTL_NAMESPACE_BEGIN

namespace io = boost::iostreams;
namespace fs = boost::filesystem;

/** \brief Defines the interface of all pack files which is used by the pack object database.
  * \note the template parameters are used only to have meaningful types in the method signatures, 
  * the implementor is not obgliged to use templates at all.
  * \tparam ObjectTraits traits defining object properties
  * \tparam Traits traits identifying pack database specific types
  */
template <class TraitsType>
class odb_pack_file
{
public:
	typedef TraitsType									db_traits_type;
	typedef typename db_traits_type::obj_traits_type	obj_traits_type;
	
	typedef typename db_traits_type::path_type			path_type;
	
public:
	//! Instantiate this instance with the path to a pack file
	odb_pack_file(const path_type& path);
	
public:
	
	
};

/** \brief Defines types used specifically in a packed object database.
  */
template <class ObjectTraits>
struct odb_pack_traits : public odb_file_traits<typename ObjectTraits::key_type,
												typename ObjectTraits::hash_generator_type>
{
	typedef ObjectTraits									obj_traits_type;
	
	//! Type used to handle the reading of packs
	typedef odb_pack_file<odb_pack_traits>					pack_reader_type;
	
	
};


/** \brief Implements a database which reads objects from highly compressed packs.
  * A pack is a file with a collection of objects which are either stored as compressed object 
  * or as a delta against another object ( which may itself be deltified as well ). A base to a delta
  * object is specified either as relative offset within the current pack file, or as key identifying
  * the object in some database, which is not necessarily this one.
  * A pack file comes with a header, which provides the index necessary to quickly find objects within the pack.
  * The details of how the format of the pack looks like, are encapsulated in the PackFile's interface.
  * 
  * Every pack implementation must support reading of existing packs, writing of packs is optional.
  * When writing a new pack, a list of object keys is provided which in turn gets congested into a new pack file.
  * 
  * \tparam ObjectTraits traits to define the objects to be stored within the database
  * \tparam Traits traits specific to the database itself, which includes the actual pack format
  */ 
template <class TraitsType>
class odb_pack :	public odb_base<TraitsType>,
					public odb_file_mixin<typename TraitsType::path_type>
{
public:
	typedef TraitsType									db_traits_type;
	typedef typename db_traits_type::obj_traits_type	obj_traits_type;
	
	typedef typename obj_traits_type::key_type			key_type;
	typedef typename obj_traits_type::char_type			char_type;
	typedef odb_hash_error<key_type>					hash_error_type;
	
	typedef typename db_traits_type::path_type			path_type;
	
public:
	odb_pack(const path_type& root)
	    : odb_file_mixin<path_type>(root)
	{}
	
public:
	
	
};


GTL_NAMESPACE_END
GTL_HEADER_END

#endif // GTL_ODB_PACK_HPP
