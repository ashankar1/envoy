#pragma once

#include "envoy/config/rbac/v3/rbac.pb.h"
#include "envoy/extensions/filters/common/expr/custom_vocabulary/v3/custom_vocabulary_interface.pb.h"

#include "source/extensions/filters/common/rbac/engine.h"
#include "source/extensions/filters/common/rbac/matchers.h"

namespace Envoy {
namespace Extensions {
namespace Filters {
namespace Common {
namespace RBAC {

class DynamicMetadataKeys {
public:
  const std::string ShadowEffectivePolicyIdField{"shadow_effective_policy_id"};
  const std::string ShadowEngineResultField{"shadow_engine_result"};
  const std::string EngineResultAllowed{"allowed"};
  const std::string EngineResultDenied{"denied"};
  const std::string AccessLogKey{"access_log_hint"};
  const std::string CommonNamespace{"envoy.common"};
};

using DynamicMetadataKeysSingleton = ConstSingleton<DynamicMetadataKeys>;
using CustomVocabularyWrapper = Envoy::Extensions::Filters::Common::Expr::CustomVocabularyWrapper;
using CustomVocabularyInterface = Envoy::Extensions::Filters::Common::Expr::CustomVocabularyInterface;
using CustomVocabularyInterfaceConfig = envoy::extensions::filters::common::expr::custom_vocabulary::v3::CustomVocabularyInterfaceConfig;


enum class EnforcementMode { Enforced, Shadow };

class RoleBasedAccessControlEngineImpl : public RoleBasedAccessControlEngine, NonCopyable {
public:
  RoleBasedAccessControlEngineImpl(const envoy::config::rbac::v3::RBAC& rules,
                                   ProtobufMessage::ValidationVisitor& validation_visitor,
                                   bool has_custom_vocab_config,
                                   const CustomVocabularyInterfaceConfig& custom_vocab_config,
                                   const EnforcementMode mode = EnforcementMode::Enforced);

  bool handleAction(const Network::Connection& connection,
                    const Envoy::Http::RequestHeaderMap& headers, StreamInfo::StreamInfo& info,
                    std::string* effective_policy_id) const override;

  bool handleAction(const Network::Connection& connection, StreamInfo::StreamInfo& info,
                    std::string* effective_policy_id) const override;

private:
  // Checks whether the request matches any policies
  bool checkPolicyMatch(const Network::Connection& connection, const StreamInfo::StreamInfo& info,
                        const Envoy::Http::RequestHeaderMap& headers,
                        std::string* effective_policy_id) const;

  const envoy::config::rbac::v3::RBAC::Action action_;
  const EnforcementMode mode_;
  const CustomVocabularyInterfaceConfig custom_vocab_interface_config_;

  std::map<std::string, std::unique_ptr<PolicyMatcher>> policies_;
  std::unique_ptr<CustomVocabularyInterface> custom_vocabulary_interface_;


  Protobuf::Arena constant_arena_;
  Expr::BuilderPtr builder_;
};

} // namespace RBAC
} // namespace Common
} // namespace Filters
} // namespace Extensions
} // namespace Envoy
