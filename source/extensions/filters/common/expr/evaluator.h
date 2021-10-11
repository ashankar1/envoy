#pragma once

#include "envoy/stream_info/stream_info.h"

#include "source/common/http/headers.h"
#include "source/common/protobuf/protobuf.h"
#include "source/extensions/filters/common/expr/context.h"

#include "eval/public/cel_expression.h"
#include "eval/public/cel_value.h"
#include "eval/public/cel_function_adapter.h"

#include "source/extensions/filters/common/expr/custom_expr.h"

namespace Envoy {
namespace Extensions {
namespace Filters {
namespace Common {
namespace Expr {

using Activation = google::api::expr::runtime::Activation;
using ActivationPtr = std::unique_ptr<Activation>;
using Builder = google::api::expr::runtime::CelExpressionBuilder;
using BuilderPtr = std::unique_ptr<Builder>;
using Expression = google::api::expr::runtime::CelExpression;
using ExpressionPtr = std::unique_ptr<Expression>;
using CustomVocabularyWrapper = Envoy::Extensions::Filters::Common::Expr::CustomVocabularyWrapper;
using CelFunctionRegistry = google::api::expr::runtime::CelFunctionRegistry;
using CelValue = google::api::expr::runtime::CelValue;
using ConstCelFunction = Envoy::Extensions::Filters::Common::Expr::ConstCelFunction;

class CustomVocabularyInterface {
 public:
  void FillActivation(Activation *activation) const {
    activation->InsertValueProducer("custom",
                                      std::make_unique<CustomVocabularyWrapper>());
    auto func_name = absl::string_view("constFunc2");
    absl::Status status = activation->InsertFunction(std::make_unique<ConstCelFunction>(func_name));
//const StreamInfo::StreamInfo& info
    //  , custom_vocab_map
//Protobuf::Arena& arena
//info
  }
};

// Creates an activation providing the common context attributes.
// The activation lazily creates wrappers during an evaluation using the evaluation arena.
ActivationPtr createActivation(Protobuf::Arena& arena, const StreamInfo::StreamInfo& info,
                               const Http::RequestHeaderMap* request_headers,
                               const Http::ResponseHeaderMap* response_headers,
                               const Http::ResponseTrailerMap* response_trailers,
                               const CustomVocabularyInterface* custom_vocab);

// Creates an expression builder. The optional arena is used to enable constant folding
// for intermediate evaluation results.
// Throws an exception if fails to construct an expression builder.
BuilderPtr createBuilder(Protobuf::Arena* arena);

// Creates an interpretable expression from a protobuf representation.
// Throws an exception if fails to construct a runtime expression.
ExpressionPtr createExpression(Builder& builder, const google::api::expr::v1alpha1::Expr& expr);

// Evaluates an expression for a request. The arena is used to hold intermediate computational
// results and potentially the final value.
absl::optional<CelValue> evaluate(const Expression& expr, Protobuf::Arena& arena,
                                  const StreamInfo::StreamInfo& info,
                                  const Http::RequestHeaderMap* request_headers,
                                  const Http::ResponseHeaderMap* response_headers,
                                  const Http::ResponseTrailerMap* response_trailers);

// Evaluates an expression and returns true if the expression evaluates to "true".
// Returns false if the expression fails to evaluate.
bool matches(const Expression& expr, const StreamInfo::StreamInfo& info,
             const Http::RequestHeaderMap& headers);

// Returns a string for a CelValue.
std::string print(CelValue value);

// Thrown when there is an CEL library error.
class CelException : public EnvoyException {
public:
  CelException(const std::string& what) : EnvoyException(what) {}
};



} // namespace Expr
} // namespace Common
} // namespace Filters
} // namespace Extensions
} // namespace Envoy
