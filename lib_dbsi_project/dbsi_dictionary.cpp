#include "dbsi_dictionary.h"
#include "dbsi_assert.h"


namespace dbsi
{


CodedResource Dictionary::encode(const Resource& r)
{
	// if the resource was new, this is what its new key would be
	const CodedResource potential_new_code = m_decoder.size();

	// do a lookup, but insert if the resource doesn't exist, with a new ID
	// (note that map's `insert` does not update the value if the key already
	// exists)
	auto [iter, is_new] = m_encoder.insert(std::make_pair(r, potential_new_code));

	if (is_new)
		// store address of iterator's key (NOT `r`)
		m_decoder.push_back(&iter->first);

	// when you decode the return value, it should give the input to this function
	DBSI_CHECK_INVARIANT(*m_decoder[iter->second] == r);

	return iter->second;
}


Resource Dictionary::decode(CodedResource i) const
{
	DBSI_CHECK_PRECOND(i < m_decoder.size());
	return *m_decoder[i];
}


}  // namespace dbsi
