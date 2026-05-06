#ifndef ANOTIFIER_CPP
#define ANOTIFIER_CPP

#include "ANotifier.hpp"
#include "shared/DefaultFunction.hpp"

namespace AutoLang {

std::string ANotifier::toString(AObject *obj) {
	return DefaultFunction::to_string(*this, obj);
}

} // namespace AutoLang

#endif