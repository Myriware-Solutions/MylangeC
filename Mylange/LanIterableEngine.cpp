#include "LanIterableEngine.h"
#include "LanVariable.h"

LanIterableEngine::LanIterableEngine(std::vector<std::pair<std::string, LanType>> keys, 
	std::unique_ptr<LanVariable> matrixVar)
{
	if (!matrixVar->Type.IsArrayType())
		throw std::runtime_error("Variable value needs to be an array for iter.");

	this->Keys = std::move(keys);
	this->IsUnpackingIter = (this->Keys.size() > 1);

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
