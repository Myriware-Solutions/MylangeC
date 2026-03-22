#include "LanClass.h"
#include "LanVariable.h"
#include "LanType.h"
#include "LanFunction.h"

std::unique_ptr<LanClass> LanClass::Clone() const
{
	return std::make_unique<LanClass>(Name, Properties, DefaultValues, Methods);
}

optional<unique_ptr<LanVariable>> LanCasting::RunMethod(MylangeInterpreter& mi, const string& scopeId, const std::string& methodName, const std::vector<std::unique_ptr<LanVariable>>& args) const
{
	string methodId = LanFunction::GetId(methodName, args);
	if (this->ClassInfo->Methods.find(methodId) == this->ClassInfo->Methods.end()) {
		throw std::runtime_error("Method not found: " + methodId);
	}
	return move(this->ClassInfo->Methods[methodId]->Execute(scopeId, mi, args));
}
