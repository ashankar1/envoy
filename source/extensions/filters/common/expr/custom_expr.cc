#include "source/extensions/filters/common/expr/custom_expr.h"

namespace Envoy {
namespace Extensions {
namespace Filters {
namespace Common {
namespace Expr {

CelValue GetConstValue(Protobuf::Arena* arena, int64_t i) {
  i++;
  arena->SpaceUsed();
  std::cout << "********* GetConstValue Eager" << std::endl;
  return CelValue::CreateInt64(99);
}

} // namespace Expr
} // namespace Common
} // namespace Filters
} // namespace Extensions
} // namespace Envoy
