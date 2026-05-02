#ifndef PARAMETER_CPP
#define PARAMETER_CPP

#include "frontend/parser/Parameter.hpp"
#include "frontend/parser/ParserContext.hpp"
#include "frontend/parser/node/CreateNode.hpp"

namespace AutoLang {

Parameter *Parameter::copy(in_func) {
	Parameter *newParameter = context.parameterPool.push();
	newParameter->parameters.reserve(parameters.size());
	for (auto parameter : parameters) {
		if (!parameter->classDeclaration ||
		    !parameter->classDeclaration->isGenerics(in_data)) {
			newParameter->parameters.push_back(parameter);
			continue;
		}
		newParameter->parameters.push_back(
		    static_cast<DeclarationNode *>(parameter->copy(in_data)));
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
	newParameter->defaultValuePos = defaultValuePos;
	return newParameter;
}

} // namespace AutoLang

#endif