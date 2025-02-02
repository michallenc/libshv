#pragma once

#include "../shviotqtglobal.h"

#include <shv/chainpack/accessgrant.h>
#include <shv/core/utils/shvpath.h>

#include <map>

namespace shv {
namespace core { namespace utils { class ShvUrl; } }
namespace iotqt {
namespace acl {

struct SHVIOTQT_DECL_EXPORT AclAccessRule
{
public:
	std::string service;
	std::string pathPattern;
	std::string method;
	shv::chainpack::AccessGrant grant;

	static const char *ALL_SERVICES;

	AclAccessRule() {}
	AclAccessRule(const std::string &path_pattern, const std::string &method = std::string())
		: pathPattern(path_pattern), method(method) {}
	AclAccessRule(const std::string &path_pattern, const std::string &method, const shv::chainpack::AccessGrant &grant)
		: pathPattern(path_pattern), method(method), grant(grant) {}

	chainpack::RpcValue toRpcValue() const;
	static AclAccessRule fromRpcValue(const chainpack::RpcValue &rpcval);

	bool isValid() const { return !pathPattern.empty() && grant.isValid(); }
	bool isMoreSpecificThan(const AclAccessRule &other) const;
	bool isPathMethodMatch(const shv::core::utils::ShvUrl &shv_url, const std::string &method) const;
};

class SHVIOTQT_DECL_EXPORT AclRoleAccessRules : public std::vector<AclAccessRule>
{
public:
	shv::chainpack::RpcValue toRpcValue() const;
	shv::chainpack::RpcValue toRpcValue_legacy() const;

	//void sortMostSpecificFirst();

	static AclRoleAccessRules fromRpcValue(const shv::chainpack::RpcValue &v);
};

} // namespace acl
} // namespace iotqt
} // namespace shv

