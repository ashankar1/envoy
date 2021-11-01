#include "source/extensions/filters/common/expr/evaluator.h"

#include "envoy/common/exception.h"

#include "eval/public/builtin_func_registrar.h"
#include "eval/public/cel_expr_builder_factory.h"

#include "source/extensions/filters/common/expr/custom_functions.h"

#include "source/extensions/filters/common/expr/custom_vocabulary_interface.h"

namespace Envoy {
namespace Extensions {
namespace Filters {
namespace Common {
namespace Expr {

ActivationPtr createActivation(Protobuf::Arena& arena, const StreamInfo::StreamInfo& info,
                               const Http::RequestHeaderMap* request_headers,
                               const Http::ResponseHeaderMap* response_headers,
                               const Http::ResponseTrailerMap* response_trailers,
                               const CustomVocabularyInterface* custom_vocabulary_interface
                               ) {
  auto activation = std::make_unique<Activation>();

  std::cout << "*************** createActivation" << std::endl;

  if (custom_vocabulary_interface) {
    std::cout << "*************** createActivation: has custom_vocabulary_interface" << std::endl;
    custom_vocabulary_interface->FillActivation(activation.get(), arena, info);
  }

  activation->InsertValueProducer(Request,
                                  std::make_unique<RequestWrapper>(arena, request_headers, info));
  activation->InsertValueProducer(Response, std::make_unique<ResponseWrapper>(
                                                arena, response_headers, response_trailers, info));
  activation->InsertValueProducer(Connection, std::make_unique<ConnectionWrapper>(info));
  activation->InsertValueProducer(Upstream, std::make_unique<UpstreamWrapper>(info));
  activation->InsertValueProducer(Source, std::make_unique<PeerWrapper>(info, false));
  activation->InsertValueProducer(Destination, std::make_unique<PeerWrapper>(info, true));
  activation->InsertValueProducer(Metadata,
                                  std::make_unique<MetadataProducer>(info.dynamicMetadata()));
  activation->InsertValueProducer(FilterState,
                                  std::make_unique<FilterStateWrapper>(info.filterState()));


  return activation;
}

BuilderPtr createBuilder(Protobuf::Arena* arena, const CustomVocabularyInterface* custom_vocabulary_interface) {
  google::api::expr::runtime::InterpreterOptions options;

  // Security-oriented defaults
  options.enable_comprehension = false;
  options.enable_regex = true;
  options.regex_max_program_size = 100;
  options.enable_string_conversion = false;
  options.enable_string_concat = false;
  options.enable_list_concat = false;

  // Enable constant folding (performance optimization)
  if (arena != nullptr) {
    options.constant_folding = true;
    options.constant_arena = arena;
  }

  auto builder = google::api::expr::runtime::CreateCelExpressionBuilder(options);
  auto register_status =
      google::api::expr::runtime::RegisterBuiltinFunctions(builder->GetRegistry(), options);
  if (!register_status.ok()) {
    throw CelException(
        absl::StrCat("failed to register built-in functions: ", register_status.message()));
  }

  std::cout << "*************** createBuilder" << std::endl;

  if (custom_vocabulary_interface) {
    std::cout << "*************** createBuilder: has custom_vocabulary_interface" << std::endl;
    custom_vocabulary_interface->RegisterFunctions(builder->GetRegistry());
  }

  std::cout << "*************** createBuilder" << std::endl;
  return builder;
}

ExpressionPtr createExpression(Builder& builder, const google::api::expr::v1alpha1::Expr& expr) {
  google::api::expr::v1alpha1::SourceInfo source_info;
  auto cel_expression_status = builder.CreateExpression(&expr, &source_info);
  if (!cel_expression_status.ok()) {
    throw CelException(
        absl::StrCat("failed to create an expression: ", cel_expression_status.status().message()));
  }
  std::cout << "*************** createExpression" << std::endl;

  return std::move(cel_expression_status.value());
}

absl::optional<CelValue> evaluate(const Expression& expr, Protobuf::Arena& arena,
                                  const StreamInfo::StreamInfo& info,
                                  const Http::RequestHeaderMap* request_headers,
                                  const Http::ResponseHeaderMap* response_headers,
                                  const Http::ResponseTrailerMap* response_trailers,
                                  const CustomVocabularyInterface* custom_vocabulary_interface) {
    std::cout << "*************** evaluate" << std::endl;
  auto activation =
      createActivation(arena, info, request_headers, response_headers, response_trailers, custom_vocabulary_interface);
  auto eval_status = expr.Evaluate(*activation, &arena);
  if (!eval_status.ok()) {
    return {};
  }

  return eval_status.value();
}

bool matches(const Expression& expr, const StreamInfo::StreamInfo& info,
             const Http::RequestHeaderMap& headers) {
  Protobuf::Arena arena;
  std::cout << "*************** matches no custom vocab interface" << std::endl;
  auto eval_status = Expr::evaluate(expr, arena, info, &headers, nullptr, nullptr, nullptr);
  if (!eval_status.has_value()) {
    return false;
  }
  auto result = eval_status.value();
  return result.IsBool() ? result.BoolOrDie() : false;
}

bool matches(const Expression& expr, const StreamInfo::StreamInfo& info,
             const Http::RequestHeaderMap& headers,
             const CustomVocabularyInterface* custom_vocabulary_interface) {
  Protobuf::Arena arena;
  std::cout << "*************** matches - custom vocab interface" << std::endl;
  auto eval_status = Expr::evaluate(expr, arena, info, &headers, nullptr, nullptr, custom_vocabulary_interface);
  if (!eval_status.has_value()) {
    return false;
  }
  auto result = eval_status.value();
  return result.IsBool() ? result.BoolOrDie() : false;
}

std::string print(CelValue value) {
  switch (value.type()) {
  case CelValue::Type::kBool:
    return value.BoolOrDie() ? "true" : "false";
  case CelValue::Type::kInt64:
    return absl::StrCat(value.Int64OrDie());
  case CelValue::Type::kUint64:
    return absl::StrCat(value.Uint64OrDie());
  case CelValue::Type::kDouble:
    return absl::StrCat(value.DoubleOrDie());
  case CelValue::Type::kString:
    return std::string(value.StringOrDie().value());
  case CelValue::Type::kBytes:
    return std::string(value.BytesOrDie().value());
  case CelValue::Type::kMessage:
    return value.IsNull() ? "NULL" : value.MessageOrDie()->ShortDebugString();
  case CelValue::Type::kDuration:
    return absl::FormatDuration(value.DurationOrDie());
  case CelValue::Type::kTimestamp:
    return absl::FormatTime(value.TimestampOrDie(), absl::UTCTimeZone());
  default:
    return absl::StrCat(CelValue::TypeName(value.type()), " value");
  }
}

} // namespace Expr
} // namespace Common
} // namespace Filters
} // namespace Extensions
} // namespace Envoy
