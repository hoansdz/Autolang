#ifndef GENERIC_DATA_HPP
#define GENERIC_DATA_HPP

#include "frontend/parser/node/Node.hpp"
#include "frontend/parser/node/CreateNode.hpp"

namespace AutoLang {

struct GenericData {
	HashMap<DeclarationOffset, DeclarationOffset>
	    newPositionOfStaticDeclaration;
	HashMap<ClassDeclaration *, ExprNode *> mustRenameNodes;
	std::vector<GenericDeclarationNode *> genericDeclarations;
	std::vector<std::pair<DeclarationNode *, HasClassIdNode *>>
	    staticDeclaration;
	HashMap<LexerStringId, Offset> genericDeclarationMap;
	HashMap<DeclarationNode *, DeclarationNode *> reflectDeclarationMap;
	std::vector<DeclarationNode *> allDeclaration;
	inline GenericDeclarationNode *findDeclaration(LexerStringId nameId) {
		auto it = genericDeclarationMap.find(nameId);
		if (it == genericDeclarationMap.end()) {
			return nullptr;
		}
		return genericDeclarations[it->second];
	}
	~GenericData() {
		for (auto declaration : genericDeclarations) {
			delete declaration;
		}
	}
};

} // namespace AutoLang

#endif