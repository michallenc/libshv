#include "valuenotavailable.h"
#include "rpcvalue.h"

using namespace std;

namespace shv {
namespace chainpack {

ValueNotAvailable::MetaType::MetaType()
	: Super("ValueNotAvailable")
{
}

void ValueNotAvailable::MetaType::registerMetaType()
{
	static bool is_init = false;
	if(!is_init) {
		is_init = true;
		static MetaType s;
		shv::chainpack::meta::registerType(shv::chainpack::meta::GlobalNS::ID, MetaType::ID, &s);
	}
}

bool ValueNotAvailable::isValueNotAvailable(const RpcValue &rv)
{
	return rv.metaTypeId() == MetaType::ID && rv.metaTypeNameSpaceId() == shv::chainpack::meta::GlobalNS::ID;
}

RpcValue ValueNotAvailable::toRpcValue() const
{
	static RpcValue NA;
	if(!NA.isValid()) {
		NA = RpcValue{"N!A"};
		NA.setMetaTypeId(meta::GlobalNS::MetaTypeId::ValueNotAvailable);
	}
	return NA;
}

} // namespace chainpack
} // namespace shv
