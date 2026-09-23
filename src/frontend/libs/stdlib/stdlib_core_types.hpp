#ifndef STDLIB_CORE_TYPES_HPP
#define STDLIB_CORE_TYPES_HPP

#define STDLIB_CORE_TYPES_SOURCE STDLIB_CORE_TYPES_SOURCE_STR
inline constexpr const char* STDLIB_CORE_TYPES_SOURCE_STR = R"###(
@no_constructor
@no_extends
class Bytes {

}

@no_extends
@no_constructor
class Null {

}

@no_extends
@no_constructor
class Any {
	@native("to_string")
	fun toString(): String
}

@no_extends
@no_constructor
class Void {

}

@no_extends
@no_constructor
class Function {

}


class Exception(val message: String) {
	
}
)###";

#endif
