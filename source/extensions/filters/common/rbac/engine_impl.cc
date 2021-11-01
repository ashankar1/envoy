#include "source/extensions/filters/common/rbac/engine_impl.h"

#include "envoy/config/rbac/v3/rbac.pb.h"
#include "envoy/extensions/filters/common/expr/custom_vocabulary/v3/custom_vocabulary_interface.pb.h"

#include "source/common/http/header_map_impl.h"
#include "source/common/config/utility.h"

#include "source/extensions/filters/common/expr/custom_vocabulary_interface.h"

namespace Envoy {
namespace Extensions {
namespace Filters {
namespace Common {
namespace RBAC {

using CustomVocabularyInterfaceConfig = envoy::extensions::filters::common::expr::custom_vocabulary::v3::CustomVocabularyInterfaceConfig;
using CustomVocabularyInterfaceFactory = Envoy::Extensions::Filters::Common::Expr::CustomVocabularyInterfaceFactory;

RoleBasedAccessControlEngineImpl::RoleBasedAccessControlEngineImpl(
    const envoy::config::rbac::v3::RBAC& rules,
    ProtobufMessage::ValidationVisitor& validation_visitor,
    bool has_custom_vocab_config,
    const CustomVocabularyInterfaceConfig& custom_vocab_config,
    const EnforcementMode mode)
    : action_(rules.action()), mode_(mode) {
  // guard expression builder by presence of a condition in policies

  std::cout << "********** RoleBasedAccessControlEngineImpl" << std::endl;
  if (has_custom_vocab_config) {
    std::cout << "********** has_custom_vocab_config" << std::endl;
    auto& factory =
        Envoy::Config::Utility::getAndCheckFactory<
            CustomVocabularyInterfaceFactory>(custom_vocab_config.config());
    custom_vocabulary_interface_ = factory.createInterface(custom_vocab_config);
  } else {
    std::cout << "********** no custom_vocab_config!" << std::endl;
    custom_vocabulary_interface_ = nullptr;
  }

  for (const auto& policy : rules.policies()) {
    if (policy.second.has_condition()) {
      if (custom_vocabulary_interface_) {
        builder_ = Expr::createBuilder(&constant_arena_,
                                       custom_vocabulary_interface_.get());
      } else {
        builder_ = Expr::createBuilder(&constant_arena_,
                                       nullptr);
      }
      break;
    }
  }

  for (const auto& policy : rules.policies()) {
    policies_.emplace(policy.first, std::make_unique<PolicyMatcher>(policy.second, builder_.get(),
                                                                    validation_visitor));
  }
}

bool RoleBasedAccessControlEngineImpl::handleAction(const Network::Connection& connection,
                                                    StreamInfo::StreamInfo& info,
                                                    std::string* effective_policy_id) const {
  return handleAction(connection, *Http::StaticEmptyHeaders::get().request_headers, info,
                      effective_policy_id);
}

bool RoleBasedAccessControlEngineImpl::handleAction(const Network::Connection& connection,
                                                    const Envoy::Http::RequestHeaderMap& headers,
                                                    StreamInfo::StreamInfo& info,
                                                    std::string* effective_policy_id) const {
  bool matched = checkPolicyMatch(connection, info, headers, effective_policy_id);

  switch (action_) {
  case envoy::config::rbac::v3::RBAC::ALLOW:
    return matched;
  case envoy::config::rbac::v3::RBAC::DENY:
    return !matched;
  case envoy::config::rbac::v3::RBAC::LOG: {
    // If not shadow enforcement, set shared log metadata
    if (mode_ != EnforcementMode::Shadow) {
      ProtobufWkt::Struct log_metadata;
      auto& log_fields = *log_metadata.mutable_fields();
      log_fields[DynamicMetadataKeysSingleton::get().AccessLogKey].set_bool_value(matched);
      info.setDynamicMetadata(DynamicMetadataKeysSingleton::get().CommonNamespace, log_metadata);
    }

    return true;
  }
  default:
    NOT_REACHED_GCOVR_EXCL_LINE;
  }
}

bool RoleBasedAccessControlEngineImpl::checkPolicyMatch(
    const Network::Connection& connection, const StreamInfo::StreamInfo& info,
    const Envoy::Http::RequestHeaderMap& headers, std::string* effective_policy_id) const {
  bool matched = false;

  for (const auto& policy : policies_) {
    if (policy.second->matches(connection, headers, info,
                               (custom_vocabulary_interface_) ? custom_vocabulary_interface_.get() : nullptr)) {
      matched = true;
      if (effective_policy_id != nullptr) {
        *effective_policy_id = policy.first;
      }
      break;
    }
  }

  return matched;
}

} // namespace RBAC
} // namespace Common
} // namespace Filters
} // namespace Extensions
} // namespace Envoy
