#ifndef PARAMETER_CPP
#define PARAMETER_CPP

#include "frontend/parser/Parameter.hpp"
#include "frontend/parser/ParserContext.hpp"
#include "frontend/parser/node/CreateNode.hpp"

namespace Autolang {

Parameter *Parameter::copy(in_func) {
	Parameter *newParameter = context.parameterPool.push();
	newParameter->parameters.reserve(parameters.size());
	for (auto parameter : parameters) {
		if (!parameter->classDeclaration ||
		    !parameter->classDeclaration->isGenerics(in_data)) {
			newParameter->parameters.push_back(parameter);
			continue;
		}
		auto newDeclaration =
		    static_cast<DeclarationNode *>(parameter->copy(in_data));
		newParameter->parameters.push_back(newDeclaration);
		// std::cerr << context.lexerString[newDeclaration->baseName] << " "
		//           << newDeclaration->classDeclaration->getName(in_data) << "\n";
	}
	if (!parameterDefaultValues.empty()) {
		newParameter->parameterDefaultValues.reserve(
		    parameterDefaultValues.size());
		for (auto value : parameterDefaultValues) {
			newParameter->parameterDefaultValues.push_back(
			    static_cast<HasClassIdNode *>(value->copy(in_data)));
		}
		context.defaultValueParameter.push_back(newParameter);
	}
	if (!destructurePatterns.empty()) {
		newParameter->destructurePatterns.reserve(destructurePatterns.size());
		auto funcInfo = context.getCurrentFunctionInfo(in_data);
		for (const auto &item : destructurePatterns) {
			DestructurePatternItem newItem;
			if (item.source && funcInfo && funcInfo->reflectDeclarationMap.count(item.source)) {
				newItem.source = funcInfo->reflectDeclarationMap[item.source];
			} else {
				newItem.source = item.source;
			}
			newItem.targets.reserve(item.targets.size());
			for (auto *t : item.targets) {
				if (t && funcInfo && funcInfo->reflectDeclarationMap.count(t)) {
					newItem.targets.push_back(funcInfo->reflectDeclarationMap[t]);
				} else {
					newItem.targets.push_back(t);
				}
			}
			newParameter->destructurePatterns.push_back(std::move(newItem));
		}
	}
	newParameter->defaultValuePos = defaultValuePos;
	return newParameter;
}

} // namespace Autolang

#endif