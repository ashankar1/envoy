#pragma once

#include "envoy/stats/stats_macros.h"

#include "source/common/common/fmt.h"
#include "source/common/singleton/const_singleton.h"
#include "source/extensions/filters/common/rbac/engine_impl.h"

#include "envoy/extensions/filters/common/expr/custom_vocabulary/v3/custom_vocabulary_interface.pb.h"

namespace Envoy {
namespace Extensions {
namespace Filters {
namespace Common {
namespace RBAC {

using CustomVocabularyInterfaceConfig = envoy::extensions::filters::common::expr::custom_vocabulary::v3::CustomVocabularyInterfaceConfig;

/**
 * All stats for the enforced rules in RBAC filter. @see stats_macros.h
 */
#define ENFORCE_RBAC_FILTER_STATS(COUNTER)                                                         \
  COUNTER(allowed)                                                                                 \
  COUNTER(denied)

/**
 * All stats for the shadow rules in RBAC filter. @see stats_macros.h
 */
#define SHADOW_RBAC_FILTER_STATS(COUNTER)                                                          \
  COUNTER(shadow_allowed)                                                                          \
  COUNTER(shadow_denied)

/**
 * Wrapper struct for shadow rules in RBAC filter stats. @see stats_macros.h
 */
struct RoleBasedAccessControlFilterStats {
  ENFORCE_RBAC_FILTER_STATS(GENERATE_COUNTER_STRUCT)
  SHADOW_RBAC_FILTER_STATS(GENERATE_COUNTER_STRUCT)
};

RoleBasedAccessControlFilterStats
generateStats(const std::string& prefix, const std::string& shadow_prefix, Stats::Scope& scope);

template <class ConfigType>
std::unique_ptr<RoleBasedAccessControlEngineImpl>
createEngine(const ConfigType& config, ProtobufMessage::ValidationVisitor& validation_visitor) {
  if (config.has_rules()) {
    if (config.has_custom_vocab_config()) {
      return std::make_unique<RoleBasedAccessControlEngineImpl>(
          config.rules(), validation_visitor, true, config.custom_vocab_config(), EnforcementMode::Enforced);
    } else {
      CustomVocabularyInterfaceConfig custom_vocab_config;
      return std::make_unique<RoleBasedAccessControlEngineImpl>(
          config.rules(), validation_visitor, false, custom_vocab_config, EnforcementMode::Enforced);

    }
  }
  return nullptr;
}

template <class ConfigType>
std::unique_ptr<RoleBasedAccessControlEngineImpl>
createShadowEngine(const ConfigType& config,
                   ProtobufMessage::ValidationVisitor& validation_visitor) {
  if (config.has_shadow_rules()) {
    if (config.has_custom_vocab_config()) {
      return std::make_unique<RoleBasedAccessControlEngineImpl>(
          config.shadow_rules(), validation_visitor, true, config.custom_vocab_config(), EnforcementMode::Enforced);
    } else {
      CustomVocabularyInterfaceConfig custom_vocab_config;
      return std::make_unique<RoleBasedAccessControlEngineImpl>(
          config.shadow_rules(), validation_visitor, false, custom_vocab_config, EnforcementMode::Enforced);

    }
  }
  return nullptr;
}

std::string responseDetail(const std::string& policy_id);

} // namespace RBAC
} // namespace Common
} // namespace Filters
} // namespace Extensions
} // namespace Envoy
