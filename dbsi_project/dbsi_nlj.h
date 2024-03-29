#ifndef DBSI_NLJ_H
#define DBSI_NLJ_H


#include <vector>
#include <memory>
#include "dbsi_types.h"
#include "dbsi_iterator.h"


namespace dbsi
{


class RDFIndex;  // forward declaration


namespace joins
{


/*
* Creates an iterator which returns the join of the given
* set of patterns, using nested (index) loop join. Whether
* or not an index is actually used is up to the implementation
* of `rdf_idx`. The order in which iteration is done will
* be *exactly* the order of `pattern`, i.e. an iterator will
* first be created for patterns[0], which will then bind
* variables into the rest of the expressions, then an iterator
* for patterns[1] will be created, etc...
*/
std::unique_ptr<ICodedVarMapIterator> create_nested_loop_join_iterator(
	const RDFIndex& rdf_idx,
	std::vector<CodedTriplePattern> patterns
);


/*
* Rearrange the given join product of patterns into a (hopefully)
* more efficient one. Good idea to call this just before
* `create_nested_loop_join_iterator`. It will permute the array
* given as input.
* 
* Implementation according to the reference:
* Petros Tsialiamanis, Lefteris Sidirourgos, Irini Fundulaki,
* Vassilis Christophides, Peter A. Boncz. Heuristics-based query
* optimisation for SPARQL. Proc. of the 15th Int. Conf. on Extending
* Database Technology (EDBT 2012), pages 324�335, Berlin, Germany,
* March 27�30 2012. ACM.
*/
void greedy_join_order_opt(
	std::vector<CodedTriplePattern>& patterns
);


}  // namespace joins
}  // namespace dbsi


#endif  // DBSI_NLJ_H
