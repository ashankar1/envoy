#include "source/extensions/filters/common/expr/custom_functions.h"

namespace Envoy {
namespace Extensions {
namespace Filters {
namespace Common {
namespace Expr {

absl::Status ConstCelFunction::Evaluate(absl::Span<const CelValue> args,
                                        CelValue* output,
                                        Protobuf::Arena* arena) const {
  args.size();
  arena->SpaceUsed();
  *output = CelValue::CreateInt64(99);
  return absl::OkStatus();
}

CelValue GetConstValue(Protobuf::Arena* arena, int64_t i) {
  i++;
  arena->SpaceUsed();
  return CelValue::CreateInt64(99);
}

} // namespace Expr
} // namespace Common
} // namespace Filters
} // namespace Extensions
} // namespace Envoy
