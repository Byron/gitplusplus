#ifndef GIT_OBJ_COMMIT_H
#define GIT_OBJ_COMMIT_H

#include <git/config.h>
#include <git/obj/object.hpp>
#include <git/obj/stream.h>

#include <vector>
#include <string>


GIT_HEADER_BEGIN
GIT_NAMESPACE_BEGIN

using std::string;
		
/** \brief object representing a commit, which keeps information about a distinct repository state
  */
class Commit : public Object
{
public:
	typedef typename git_object_traits_base::key_type		key_type;
	typedef std::vector<key_type>							key_vector_type;	//!< vector of keys
	
	static const string			default_encoding;	//!< encoding used if nothing else is specified
	
private:
	key_type					m_tree_key;		//!< key to our root tree
	ActorDate					m_author;		//!< information about the author
	ActorDate					m_committer;	//!< information about the committer
	string						m_message;		//!< commit message
	key_vector_type				m_parent_keys;	//!< zero or more parent keys
	string						m_encoding;		//!< encoding used for the message and author data, defaults to utf8
	
public:
    Commit();
	
	Commit(const Commit&) = default;
	Commit(Commit&&) = default;
	
	bool operator == (const Commit&) const;
	
public:
	//! \return read-only key of the commit's tree
	const key_type& tree_key() const {
		return m_tree_key;
	}
	
	//! \return modifyable key of the commit's tree
	key_type& tree_key() {
		return m_tree_key;
	}
	
	//! \return read-only committer information
	const ActorDate& committer() const {
		return m_committer;
	}
	
	//! \return modifyable committer information
	ActorDate& committer() {
		return m_committer;
	}
	
	//! \return read-only author information
	const ActorDate& author() const {
		return m_author;
	}
	
	//! \return modifyable author information
	ActorDate& author() {
		return m_author;
	}
	
	//! \return read-only commit message
	const string& message() const {
		return m_message;
	}
	
	//! \return modifyable commit message
	string& message() {
		return m_message;
	}
	
	//! \return read-only vector of parent keys
	const key_vector_type& parent_keys() const {
		return m_parent_keys;
	}
	
	//! \return modifyable vector of parent keys
	key_vector_type& parent_keys() {
		return m_parent_keys;
	}
	
	//! \return read-only string identifying the encoding
	const string& encoding() const {
		return m_encoding;
	}
	
	//! \return read-only string identifying the encoding
	string& encoding() {
		return m_encoding;
	}
};

git_basic_ostream& operator << (git_basic_ostream& stream, const Commit& inst);
git_basic_istream& operator >> (git_basic_istream& stream, Commit& inst);

GIT_NAMESPACE_END
GIT_HEADER_END

#endif // GIT_OBJ_COMMIT_H
