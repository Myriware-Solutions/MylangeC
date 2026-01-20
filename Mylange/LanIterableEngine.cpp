#include "LanIterableEngine.h"
#include "LanVariable.h"

LanIterableEngine::LanIterableEngine(std::vector<std::string> keys, std::unique_ptr < LanVariable > matrixVar)
{
	this->Keys = std::move(keys);

	if (!matrixVar->Type.IsArrayType())
		throw std::runtime_error("Variable value needs to be an array for iter.");

	for (auto& elementRow : std::get<std::vector <unique_ptr< LanVariable> >> (matrixVar->Value))
	{
		if (!elementRow->Type.IsArrayType())
			throw std::runtime_error("Element value needs to be an array for iter.");

		std::vector<std::unique_ptr<LanVariable>> row;

		for (auto& element : std::get<std::vector < unique_ptr< LanVariable> >>  (elementRow->Value))
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
}

vector<unordered_map<string, LanVariable>> LanIterableEngine::GetIterable()
{
	vector<unordered_map<string, LanVariable>> result;

	/*for (auto& value : this->Values)
	{
		if (this->Keys.size() != value.size())
			throw runtime_error("Cannot unpack unequal sized.");

		unordered_map<string, LanVariable> umap;

		for (size_t i = 0; i < this->Keys.size(); ++i)
		{
			umap.try_emplace(this->Keys[i], std::move(*(value[i])));
		}

		result.push_back(std::move(umap));
	}*/

	return result;
}
