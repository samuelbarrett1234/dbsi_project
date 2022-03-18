#include "dbsi_types.h"
#include "dbsi_assert.h"


namespace dbsi
{


std::string trip_pat_type_str(TriplePatternType type)
{
	switch (type)
	{
	case TriplePatternType::VVV:
		return "VVV";
	case TriplePatternType::VVO:
		return "VVO";
	case TriplePatternType::VPV:
		return "VPV";
	case TriplePatternType::SVV:
		return "SVV";
	case TriplePatternType::VPO:
		return "VPO";
	case TriplePatternType::SVO:
		return "SVO";
	case TriplePatternType::SPV:
		return "SPV";
	case TriplePatternType::SPO:
		return "SPO";
	default:
		DBSI_CHECK_PRECOND(false);
	}
}


}  // namespace dbsi
