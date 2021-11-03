#pragma once

#include "source/common/protobuf/protobuf.h"
#include "envoy/common/pure.h"
#include "envoy/config/typed_config.h"

#include "absl/strings/string_view.h"
#include "absl/types/optional.h"

#include "source/extensions/filters/common/expr/context.h"
#include "source/extensions/filters/common/expr/custom_functions.h"

#include "eval/public/activation.h"
#include "eval/public/cel_expression.h"
#include "eval/public/cel_value.h"
#include "eval/public/cel_function_adapter.h"

#include "envoy/extensions/filters/common/expr/custom_vocabulary/v3/custom_vocabulary_interface.pb.h"
#include "envoy/extensions/filters/common/expr/custom_vocabulary/swg/v3/swg.pb.h"

#include "envoy/protobuf/message_validator.h"


//for vocabulary, table
//value producer name, pointer to wrapper for that value
//for lazy functions, table
/*
 * function name, function class definition - need the name in two places
 * eagerly evaluated functions, just add to the list
 */
namespace Envoy {
namespace Extensions {
namespace Filters {
namespace Common {
namespace Expr {

using Activation = google::api::expr::runtime::Activation;
using CelFunctionRegistry = google::api::expr::runtime::CelFunctionRegistry;
using CustomVocabularyInterfaceConfig = envoy::extensions::filters::common::expr::custom_vocabulary::v3::CustomVocabularyInterfaceConfig;
using SwgCustomVocabularyInterfaceConfig = envoy::extensions::filters::common::expr::custom_vocabulary::swg::v3::SwgCustomVocabularyInterfaceConfig;

class CustomVocabularyWrapper : public BaseWrapper {
public:
  CustomVocabularyWrapper(Protobuf::Arena& arena,
                          const StreamInfo::StreamInfo& info)
                          : arena_(arena), info_(info) {
      arena_.SpaceUsed();
      info_.attemptCount();
   }
  absl::optional<CelValue> operator[](CelValue key) const override;

private:
  Protobuf::Arena& arena_;
  const StreamInfo::StreamInfo& info_;
};

class CustomVocabularyInterface {
 public:
  void FillActivation(Activation *activation, Protobuf::Arena& arena,
                                  const StreamInfo::StreamInfo& info) const;

  void RegisterFunctions(CelFunctionRegistry *registry) const;
};

using CustomVocabularyInterfacePtr = std::unique_ptr<CustomVocabularyInterface>;

class CustomVocabularyInterfaceFactory : public Envoy::Config::TypedFactory {
public:
  std::string category() const override { return "envoy.expr.custom_vocabulary"; }

  ProtobufTypes::MessagePtr createEmptyConfigProto() override {
    return ProtobufTypes::MessagePtr {
        new CustomVocabularyInterfaceConfig()};
  }
  virtual CustomVocabularyInterfacePtr createInterface(
      const Protobuf::Message& config, ProtobufMessage::ValidationVisitor& validation_visitor) PURE;
};

class SwgCustomVocabularyInterfaceFactory : public CustomVocabularyInterfaceFactory {
public:
 CustomVocabularyInterfacePtr createInterface(
      const Protobuf::Message& config, ProtobufMessage::ValidationVisitor& validation_visitor)  override;

  std::string name() const override {
    std::cout << "*********** name envoy.expr.custom_vocabulary.swg" << std::endl;
    return "envoy.expr.custom_vocabulary.swg";
  }

};

} // namespace Expr
} // namespace Common
} // namespace Filters
} // namespace Extensions
} // namespace Envoy