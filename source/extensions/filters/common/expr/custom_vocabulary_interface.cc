#include "source/extensions/filters/common/expr/custom_vocabulary_interface.h"

namespace Envoy {
namespace Extensions {
namespace Filters {
namespace Common {
namespace Expr {

absl::optional<CelValue> CustomVocabularyWrapper::operator[](CelValue key) const {
  if (!key.IsString()) {
    return {};
  }
  auto value = key.StringOrDie().value();
  if (value == "team") {
    std::string name("swg");
    std::cout << "custom vocabulary team!  swg!" << std::endl;
    return CelValue::CreateString(&name);
  } else if (value == "ip") {
    std::cout << "custom vocabulary ip!" << std::endl;
    auto upstream_local_address = info_.upstreamLocalAddress();
    if (upstream_local_address != nullptr) {
      std::cout << "upstream_local_address: " << upstream_local_address->asStringView() << std::endl;
      return CelValue::CreateStringView(upstream_local_address->asStringView());
    } else {
      std::cout << "upstream_local_address is null" << std::endl;
    }
  }

  return {};
}

void CustomVocabularyInterface::FillActivation(Activation *activation, Protobuf::Arena& arena,
                                  const StreamInfo::StreamInfo& info) const {

  //words
  activation->InsertValueProducer("custom",
                                      std::make_unique<CustomVocabularyWrapper>(arena, info));
  //functions
  auto func_name = absl::string_view("constFunc2");
  absl::Status status = activation->InsertFunction(std::make_unique<ConstCelFunction>(func_name));
}

void CustomVocabularyInterface::RegisterFunctions(CelFunctionRegistry* registry) const {
  //lazy functions
  auto status = registry->RegisterLazyFunction(ConstCelFunction::CreateDescriptor("constFunc2"));
  if (!status.ok()) {
    throw CelException(
        absl::StrCat("failed to register lazy functions: ", status.message()));
  }
  //eagerly evaluated functions
  status = google::api::expr::runtime::FunctionAdapter<CelValue, int64_t>::
  CreateAndRegister(
      "constFunc", false,
      [](Protobuf::Arena* arena, int64_t i)
          -> CelValue { return GetConstValue(arena, i); },
      registry);
  if (!status.ok()) {
    throw CelException(
        absl::StrCat("failed to register eagerly evaluated functions: ", status.message()));
  }
}

CustomVocabularyInterfacePtr CustomVocabularyInterfaceFactory::createInterface(
    const Protobuf::Message& config) {
//  const auto& typed_config = MessageUtil::downcastAndValidate<
//      const envoy::extensions::filters::http::custom_vocabulary::v3::CustomVocabularyInterfaceConfig&>(config);
  return std::make_unique<CustomVocabularyInterface>();
}

REGISTER_FACTORY(CustomVocabularyInterfaceFactory, Envoy::Config::TypedFactory);

} // namespace Expr
} // namespace Common
} // namespace Filters
} // namespace Extensions
} // namespace Envoy
