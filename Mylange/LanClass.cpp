//#include <memory>
//#include <optional>
//#include <stdexcept>
//#include <string>
//#include <vector>
//
//#include "LanClass.h"
//#include "LanVariable.h"
//#include "LanType.h"


//std::optional<LanVariable> LanCasting::RunMethod(MylangeInterpreter& mi,
//	const std::string& methodName,
//	std::vector<LanVariable> args) const
//{
//	string methodId = LanFunction::GetId(methodName, args);
//	if (this->ClassInfo->Methods.find(methodId) == this->ClassInfo->Methods.end()) {
//		throw std::runtime_error("Method not found: " + methodId);
//	}
//	return this->ClassInfo->Methods[methodId]->Execute(mi, args);
//}
