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


// TODO:
// ERROR POINT
// This function is not working. Please work on.
bool LanIterableEngine::GetIterable(
	std::vector<std::unordered_map<std::string, std::unique_ptr<LanVariable>>>& vectorOut
)
{
	//  CRITICAL: clear FIRST
	vectorOut.clear();

	//  NOW it is safe
	vectorOut.reserve(this->Values.size());

	for (auto& valueVariant : this->Values)
	{
		if (this->IsUnpackingIter)
		{
			auto& value =
				std::get<std::vector<std::unique_ptr<LanVariable>>>(valueVariant);

			if (this->Keys.size() != value.size())
				throw std::runtime_error("Cannot unpack unequal sized.");

			std::unordered_map<std::string, std::unique_ptr<LanVariable>> umap;

			for (size_t i = 0; i < this->Keys.size(); ++i)
			{
				umap.emplace(this->Keys[i].first, std::move(value[i]));
			}

			value.clear(); // explicit consume

			vectorOut.emplace_back(std::move(umap)); //  move-only
		}
	}

	return true;
}



//bool LanIterableEngine::GetIterable(vector<unordered_map<string, unique_ptr<LanVariable>>>& vectorOut)
//{
//	for (auto& valueVariant : this->Values)
//	{
//		if (this->IsUnpackingIter) {
//			auto& value = get<vector<unique_ptr<LanVariable>>>(valueVariant);
//			if (this->Keys.size() != value.size())
//				throw runtime_error("Cannot unpack unequal sized.");
//			unordered_map<string, unique_ptr<LanVariable>> umap;
//			for (size_t i = 0; i < this->Keys.size(); ++i)
//			{
//				umap.emplace(
//					this->Keys[i].first,
//					std::make_unique<LanVariable>(
//						value[i]->Type,
//						LanVariable::LanValue{ value[i]->Value }
//					)
//				);
//			}
//			vectorOut.push_back(move(umap));
//		}
//		else {
//			/*auto& value = get<unique_ptr<LanVariable>>(valueVariant);
//			auto umap = unordered_map<string, unique_ptr<LanVariable>>{
//				{this->Keys[0].first, move(value)}
//			};
//			vectorOut.push_back(std::move(umap));*/
//		}
//		
//	}
//
//	return true;
//}
