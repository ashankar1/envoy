#include "source/extensions/filters/common/expr/library/custom_functions.h"

#include "source/common/protobuf/protobuf.h"

#include "eval/public/cel_value.h"

namespace Envoy {
namespace Extensions {
namespace Filters {
namespace Common {
namespace Expr {
namespace Library {

absl::Status GetProductCelFunction::Evaluate(absl::Span<const CelValue> args, CelValue* output,
                                             Protobuf::Arena* arena) const {
  // using arena so that it will not be unused
  arena->SpaceUsed();
  int64_t value = args[0].Int64OrDie() * args[1].Int64OrDie();
  *output = CelValue::CreateInt64(value);
  return absl::OkStatus();
}

absl::Status GetDoubleCelFunction::Evaluate(absl::Span<const CelValue> args, CelValue* output,
                                            Protobuf::Arena* arena) const {
  // using arena so that it will not be unused
  arena->SpaceUsed();
  int64_t value = 2 * args[0].Int64OrDie();
  *output = CelValue::CreateInt64(value);
  return absl::OkStatus();
}

absl::Status Get99CelFunction::Evaluate(absl::Span<const CelValue> args, CelValue* output,
                                        Protobuf::Arena* arena) const {
  // using arena and args so that it will not be unused
  arena->SpaceUsed();
  args.size();

  *output = CelValue::CreateInt64(99);
  return absl::OkStatus();
}

CelValue GetSquareOf(Protobuf::Arena* arena, int64_t i) {
  // using arena so that it will not be unused
  arena->SpaceUsed();
  return CelValue::CreateInt64(i * i);
}

CelValue GetNextInt(Protobuf::Arena* arena, int64_t i) {
  // using arena so that it will not be unused
  arena->SpaceUsed();
  return CelValue::CreateInt64(i + 1);
}

} // namespace Library
} // namespace Expr
} // namespace Common
} // namespace Filters
} // namespace Extensions
} // namespace Envoy
