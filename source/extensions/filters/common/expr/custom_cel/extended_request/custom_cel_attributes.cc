#include "source/extensions/filters/common/expr/custom_cel/extended_request/custom_cel_attributes.h"

#include "source/common/http/utility.h"
#include "source/extensions/filters/common/expr/context.h"

#include "eval/public/cel_value.h"

namespace Envoy {
namespace Extensions {
namespace Filters {
namespace Common {
namespace Expr {
namespace CustomCel {
namespace ExtendedRequest {

using google::api::expr::runtime::CreateErrorValue;
using Http::Utility::parseAndDecodeQueryString;
using Http::Utility::QueryParams;

// ExtendedRequestWrapper extends RequestWrapper
// If ExtendedRequestWrapper[key] is not found, then the base RequestWrapper[key] is returned.
absl::optional<CelValue> ExtendedRequestWrapper::operator[](CelValue key) const {
  if (!key.IsString()) {
    return {};
  }
  auto value = key.StringOrDie().value();
  if (value == Query) {
    // return base class value
    if (!return_url_query_string_as_map_) {
      return RequestWrapper::operator[](key);
    }
    absl::string_view path = request_header_map_->getPathValue();
    // path is empty; return error
    if (path == nullptr) {
      return CreateErrorValue(&arena_, "request.query path missing", absl::StatusCode::kNotFound);
    }
    // return a map, even if the query is an empty string
    return getMapFromQueryStr(path);
  }
  return RequestWrapper::operator[](key);
}

// getMapFromQueryStr
// converts QueryParams (std::map) to CelMap
// can take an empty string
absl::optional<CelValue> ExtendedRequestWrapper::getMapFromQueryStr(absl::string_view url) const {
  QueryParams query_params = parseAndDecodeQueryString(url);
  return Utility::createCelMap(arena_, query_params);
}

} // namespace ExtendedRequest
} // namespace CustomCel
} // namespace Expr
} // namespace Common
} // namespace Filters
} // namespace Extensions
} // namespace Envoy
