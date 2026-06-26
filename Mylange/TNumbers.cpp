#include "TypeFunctions.h"
#include "MylangeInterpreter.h"


static bool r = []() {
	TypeFunctions::Register("toString", [](const std::string& scopeId, MylangeInterpreter& mi, const std::vector<std::unique_ptr<LanVariable>>& args) -> LanVariable {
		if (args.size() != 1) throw runtime_error("toString expects exactly 1 argument.");
		return LanVariable(
			LanType(LanTypeEnum::TypeString),
			LanVariable::LanValue{ args[0]->ToString() }
		);
		});
	return true;
}();