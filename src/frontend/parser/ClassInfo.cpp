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
			return new VarNode(line, it->second, false, it->second->nullable);
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
			return new GetPropNode(
			    line, node, declarationThis->classId,
			    new VarNode(line, declarationThis, false, false), name, false,
			    node->nullable, false);
		}
	// }
	return nullptr;
}

ClassInfo::~ClassInfo() {}

} // namespace AutoLang

#endif