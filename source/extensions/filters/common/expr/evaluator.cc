#include "source/extensions/filters/common/expr/evaluator.h"

#include "envoy/common/exception.h"

#include "source/extensions/filters/common/expr/library/custom_library.h"

#include "eval/public/builtin_func_registrar.h"
#include "eval/public/cel_expr_builder_factory.h"

namespace Envoy {
namespace Extensions {
namespace Filters {
namespace Common {
namespace Expr {

ActivationPtr createActivation(Protobuf::Arena& arena, const StreamInfo::StreamInfo& info,
                               const Http::RequestHeaderMap* request_headers,
                               const Http::ResponseHeaderMap* response_headers,
                               const Http::ResponseTrailerMap* response_trailers,
                               CustomLibrary* custom_library) {
  auto activation = std::make_unique<Activation>();

  if (custom_library && custom_library->replace_default_library()) {
    custom_library->FillActivation(activation.get(), arena, info, request_headers, response_headers,
                                   response_trailers);
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

  if (custom_library && !custom_library->replace_default_library()) {
    custom_library->FillActivation(activation.get(), arena, info, request_headers, response_headers,
                                   response_trailers);
  }

  return activation;
}

BuilderPtr createBuilder(Protobuf::Arena* arena, const CustomLibrary* custom_library) {
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

  if (custom_library) {
    custom_library->RegisterFunctions(builder->GetRegistry());
  }

  return builder;
}

ExpressionPtr createExpression(Builder& builder, const google::api::expr::v1alpha1::Expr& expr) {
  google::api::expr::v1alpha1::SourceInfo source_info;
  auto cel_expression_status = builder.CreateExpression(&expr, &source_info);
  if (!cel_expression_status.ok()) {
    throw CelException(
        absl::StrCat("failed to create an expression: ", cel_expression_status.status().message()));
  }
  return std::move(cel_expression_status.value());
}

absl::optional<CelValue> evaluate(const Expression& expr, Protobuf::Arena& arena,
                                  const StreamInfo::StreamInfo& info,
                                  const Http::RequestHeaderMap* request_headers,
                                  const Http::ResponseHeaderMap* response_headers,
                                  const Http::ResponseTrailerMap* response_trailers,
                                  CustomLibrary* custom_library) {
  auto activation = createActivation(arena, info, request_headers, response_headers,
                                     response_trailers, custom_library);
  auto eval_status = expr.Evaluate(*activation, &arena);
  if (!eval_status.ok()) {
    return {};
  }

  return eval_status.value();
}

bool matches(const Expression& expr, const StreamInfo::StreamInfo& info,
             const Http::RequestHeaderMap& headers) {
  return matches(expr, info, headers, nullptr);
}

bool matches(const Expression& expr, const StreamInfo::StreamInfo& info,
             const Http::RequestHeaderMap& headers, CustomLibrary* custom_library) {
  Protobuf::Arena arena;
  auto eval_status = Expr::evaluate(expr, arena, info, &headers, nullptr, nullptr, custom_library);
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
