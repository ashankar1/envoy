#pragma once

#include "envoy/common/pure.h"
#include "envoy/config/typed_config.h"

#include "absl/strings/string_view.h"
#include "absl/types/optional.h"

#include "source/extensions/filters/common/expr/context.h"
#include "source/extensions/filters/common/expr/custom_functions.h"

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
  virtual CustomVocabularyInterfacePtr createInterface(const Protobuf::Message& config) PURE;

  std::string category() const override { return "envoy.filters.common.expr.custom_vocabulary"; }
};


} // namespace Expr
} // namespace Common
} // namespace Filters
} // namespace Extensions
} // namespace Envoy