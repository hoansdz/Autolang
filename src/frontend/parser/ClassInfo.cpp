#ifndef CLASS_INFO_CPP
#define CLASS_INFO_CPP

#include "ClassInfo.hpp"

namespace AutoLang {

AccessNode *ClassInfo::findDeclaration(in_func, uint32_t line,
                                       std::string &name, bool isStatic) {
	// Find static member
	{
		auto it = staticMember.find(name);
		if (it != staticMember.end()) {
			return context.varPool.push(line, it->second, false, it->second->nullable);
		}
		// Not found
	}
	// if (declarationThis) {
		auto clazz = compile.classes[declarationThis->classId];
		auto it = clazz->memberMap.find(name);
		if (it != clazz->memberMap.end()) {
			auto node = member[it->second];
			if (isStatic)
				ParserError(line, name + " is not static");
			return context.getPropPool.push(
			    line, node, declarationThis->classId,
			    context.varPool.push(line, declarationThis, false, false), name, false,
			    node->nullable, false);
		}
	// }
	return nullptr;
}

ClassInfo::~ClassInfo() {
	for (auto declaration : genericDeclarations) {
		delete declaration;
	}
}

} // namespace AutoLang

#endif