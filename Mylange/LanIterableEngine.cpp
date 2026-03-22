#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "LanIterableEngine.h"
#include "LanVariable.h"
#include "LanType.h"
#include "LanClass.h"


LanIterableEngine::LanIterableEngine(std::vector<std::pair<std::string, LanType>> keys, 
	std::unique_ptr<LanVariable> matrixVar)
{
	this->Keys = std::move(keys);
	this->IsUnpackingIter = (this->Keys.size() > 1);

	if (matrixVar->Type.IsArrayType())
	{

		for (auto& elementRow : std::get<std::vector<unique_ptr<LanVariable>>> (matrixVar->Value))
		{
			if (this->IsUnpackingIter) {
				if (!elementRow->Type.IsArrayType())
					throw std::runtime_error("Element value needs to be an array for iter of multiple keys. Got: " + elementRow->Type.ToString());

				std::vector<std::unique_ptr<LanVariable>> row;

				for (auto& element : std::get<std::vector < unique_ptr< LanVariable> >>(elementRow->Value))
				{
					row.push_back(
						std::make_unique<LanVariable>(
							element->Type,
							std::move(element->Value)
						)
					);
				}

				this->Values.push_back(std::move(row));
			}
			else {
				this->Values.push_back(std::move(elementRow));
			}
		}
	}
	else if (matrixVar->Type.IsSetType()) {
		for (auto& pair : std::get<std::unordered_map<string, unique_ptr<LanVariable>>>(matrixVar->Value)) {
			vector<unique_ptr<LanVariable>> p;
			// First one is string, the second one is the variable
			auto key_string = make_unique<LanVariable>(
				LanType(LanTypeEnum::TypeString), 
				LanVariable::LanValue{ pair.first }
			);
			p.push_back(move(key_string)); p.push_back(move(pair.second));
			this->Values.push_back(move(p));
		}
	}
	else throw std::runtime_error("Variable value needs to be an array for iter.");
}
