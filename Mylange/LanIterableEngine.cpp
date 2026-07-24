#include "LanClass.h"
#include "LanIterableEngine.h"
#include "LanType.h"
#include "LanVariable.h"
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>


LanIterableEngine::LanIterableEngine(std::vector<std::pair<std::string, LanType>> keys, 
	LanVariable matrixVar)
{
	this->Keys = std::move(keys);
	this->IsUnpackingIter = (this->Keys.size() > 1);

	if (matrixVar.Type.IsArrayType())
	{

		for (auto& elementRow : std::get<LanArray> (matrixVar.Value))
		{
			if (this->IsUnpackingIter) {
				if (!elementRow->Type.IsArrayType())
					throw std::runtime_error("Element value needs to be an array for iter of multiple keys. Got: " + elementRow->Type.ToString());

				LanArray row;

				for (auto& element : std::get<LanArray>(elementRow->Value))
				{
					row.push_back(
						std::make_shared<LanVariable>(
							element->Type,
							std::move(element->Value)
						)
					);
				}

				this->Values.push_back(std::move(row));
			}
			else {
				this->Values.push_back(*elementRow);
			}
		}
	}
	else if (matrixVar.Type.IsSetType()) {
		throw std::runtime_error("Not Implemented Yet. LanIterableEngine.cpp:49");
		//for (auto& pair : std::get<LanSet>(matrixVar.Value)) {
		//	LanArray p;
		//	// First one is string, the second one is the variable
		//	auto key_string = std::make_shared<LanVariable>(
		//		LanType(LanTypeEnum::TypeString), 
		//		LanVariable::LanValue{ pair.first }
		//	);
		//	p.push_back(move(key_string)); p.push_back(pair.second);
		//	this->Values.push_back(move(p));
		//}
	}
	else throw std::runtime_error("Variable value needs to be an array for iter.");
}
