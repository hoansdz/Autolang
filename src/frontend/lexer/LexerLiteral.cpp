#include "frontend/lexer/Lexer.hpp"
#include "frontend/ACompiler.hpp"
#include "frontend/parser/ParserContext.hpp"

namespace Autolang {
namespace Lexer {

std::string loadNumber(Context &context, uint32_t &i) {
	bool hasDot = false;
	bool hasUnderscore = false;
	bool scientific = false;
	char chr;
	if (context.line[context.pos] == '0' && !isEndOfLine(context, i)) {
		switch (context.line[i]) {
			case 'x':
			case 'X': {
				++i;
				bool hasDigits = false;
				for (; !isEndOfLine(context, i); ++i) {
					char c = context.line[i];
					switch (c) {
						case '0': case '1': case '2': case '3': case '4':
						case '5': case '6': case '7': case '8': case '9':
						case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
						case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
							hasDigits = true;
							continue;
						case '_':
							hasUnderscore = true;
							continue;
						default:
							break;
					}
					break;
				}
				if (!hasDigits) {
					throw LexerError(context.linePos, "Expected hex digit after 0x");
				}
				if (!isEndOfLine(context, i)) {
					switch (context.line[i]) {
						case 'l':
						case 'L':
						case 'u':
						case 'U': {
							++i;
							if (!isEndOfLine(context, i)) {
								switch (context.line[i]) {
									case 'l':
									case 'L':
									case 'u':
									case 'U':
										++i;
										break;
									default:
										break;
								}
							}
							break;
						}
						default:
							break;
					}
					if (!isEndOfLine(context, i) && (std::isalnum(static_cast<unsigned char>(context.line[i])) || context.line[i] == '_')) {
						throw LexerError(
						    context.linePos,
						    std::string("Unexpected character '") + context.line[i] +
						        "' after numeric literal: " +
						        std::string(context.line + context.pos, i - context.pos) +
						        context.line[i] + "\nHint: Separate number from identifier with space or operator");
					}
				}
				goto ended;
			}
			case 'b':
			case 'B': {
				++i;
				bool hasDigits = false;
				for (; !isEndOfLine(context, i); ++i) {
					char c = context.line[i];
					switch (c) {
						case '0':
						case '1':
							hasDigits = true;
							continue;
						case '_':
							hasUnderscore = true;
							continue;
						default:
							break;
					}
					break;
				}
				if (!hasDigits) {
					throw LexerError(context.linePos, "Expected binary digit after 0b");
				}
				if (!isEndOfLine(context, i)) {
					switch (context.line[i]) {
						case 'l':
						case 'L':
						case 'u':
						case 'U': {
							++i;
							if (!isEndOfLine(context, i)) {
								switch (context.line[i]) {
									case 'l':
									case 'L':
									case 'u':
									case 'U':
										++i;
										break;
									default:
										break;
								}
							}
							break;
						}
						default:
							break;
					}
					if (!isEndOfLine(context, i) && (std::isalnum(static_cast<unsigned char>(context.line[i])) || context.line[i] == '_')) {
						throw LexerError(
						    context.linePos,
						    std::string("Unexpected character '") + context.line[i] +
						        "' after numeric literal: " +
						        std::string(context.line + context.pos, i - context.pos) +
						        context.line[i] + "\nHint: Separate number from identifier with space or operator");
					}
				}
				goto ended;
			}
			case 'o':
			case 'O': {
				++i;
				bool hasDigits = false;
				for (; !isEndOfLine(context, i); ++i) {
					char c = context.line[i];
					switch (c) {
						case '0': case '1': case '2': case '3':
						case '4': case '5': case '6': case '7':
							hasDigits = true;
							continue;
						case '_':
							hasUnderscore = true;
							continue;
						default:
							break;
					}
					break;
				}
				if (!hasDigits) {
					throw LexerError(context.linePos, "Expected octal digit after 0o");
				}
				if (!isEndOfLine(context, i)) {
					switch (context.line[i]) {
						case 'l':
						case 'L':
						case 'u':
						case 'U': {
							++i;
							if (!isEndOfLine(context, i)) {
								switch (context.line[i]) {
									case 'l':
									case 'L':
									case 'u':
									case 'U':
										++i;
										break;
									default:
										break;
								}
							}
							break;
						}
						default:
							break;
					}
					if (!isEndOfLine(context, i) && (std::isalnum(static_cast<unsigned char>(context.line[i])) || context.line[i] == '_')) {
						throw LexerError(
						    context.linePos,
						    std::string("Unexpected character '") + context.line[i] +
						        "' after numeric literal: " +
						        std::string(context.line + context.pos, i - context.pos) +
						        context.line[i] + "\nHint: Separate number from identifier with space or operator");
					}
				}
				goto ended;
			}
			default:
				break;
		}
	}
	for (; !isEndOfLine(context, i); ++i) {
		chr = context.line[i];
		switch (chr) {
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				continue;
			case 'e':
			case 'E': {
				if (scientific)
					throw LexerError(context.linePos,
					                 "Unknown value: " +
 					                     std::string(context.line + context.pos,
 					                                 i - context.pos) +
					                     "\nHint: Format scientific notation as '1e10' or '1.5e-3'");
				scientific = true;
				if (isEndOfLine(context, ++i)) {
					--i;
					goto ended;
				}
				chr = context.line[i];
				if (std::isdigit(static_cast<unsigned char>(chr))) {
					continue;
				}
				if (chr == '+' || chr == '-') {
					if (isEndOfLine(context, ++i)) {
						--i;
						goto ended;
					}
					chr = context.line[i];
					if (!std::isdigit(static_cast<unsigned char>(chr))) {
						std::string num = std::string(
						    context.line + context.pos, i - context.pos);
						throw LexerError(context.linePos,
						                 std::string("Expected digit after ") +
						                     num + " but '" + chr +
						                     "' was found\nHint: Provide a digit after exponent sign");
					}
					continue;
				}
				std::string num =
				    std::string(context.line + context.pos, i - context.pos);
				throw LexerError(
				    context.linePos,
				    std::string("Expected digit after exponent '") + num +
				        "' but '" + chr + "' was found, did you mean " + num +
				        "0 ?\nHint: Add a digit after exponent 'e' or 'E'");
			}
			case '_': {
				hasUnderscore = true;
				if (isEndOfLine(context, ++i)) {
					--i;
					goto ended;
				}
				chr = context.line[i];
				if (!std::isdigit(static_cast<unsigned char>(chr))) {
					std::string num = std::string(context.line + context.pos,
					                              i - context.pos - 1);
					throw LexerError(
					    context.linePos,
					    std::string("Expected digit after ") + num + "_ but '" +
					        chr + "' was found, did you mean " + num + " ?\nHint: Do not end numeric separator '_' without trailing digits");
				}
				continue;
			}
			case '.': {
				if (scientific) {
					std::string num = std::string(context.line + context.pos,
					                              i - context.pos);
					throw LexerError(context.linePos,
 					                 "Expected integer exponent after " + num +
 					                     ", but '.' was found\nHint: Scientific exponent must be an integer");
				}
				if (hasDot) {
					if (isEndOfLine(context, i + 1) ||
					    !std::isdigit(static_cast<unsigned char>(context.line[i + 1]))) {
						--i;
						goto ended;
					}
					std::string num = std::string(context.line + context.pos,
					                              i - context.pos);
					throw LexerError(context.linePos,
					                 "Expected digit after " + num +
					                     " but '.' was found, did you mean " +
					                     num + "0 ?\nHint: Remove extra decimal point '.'");
				}
				if (isEndOfLine(context, ++i) ||
				    !std::isdigit(static_cast<unsigned char>(context.line[i]))) {
					--i;
					goto ended;
				}
				hasDot = true;
				continue;
			}
			case 'f':
			case 'F':
			case 'd':
			case 'D': {
				++i;
				if (!isEndOfLine(context, i) &&
				    (std::isalnum(static_cast<unsigned char>(context.line[i])) || context.line[i] == '_')) {
					throw LexerError(
					    context.linePos,
					    std::string("Unexpected character '") + context.line[i] +
					        "' after numeric literal: " +
					        std::string(context.line + context.pos, i - context.pos) +
					        context.line[i] + "\nHint: Separate number from identifier with space or operator");
				}
				goto ended;
			}
			case 'l':
			case 'L':
			case 'u':
			case 'U': {
				++i;
				if (!isEndOfLine(context, i)) {
					switch (context.line[i]) {
						case 'l':
						case 'L':
						case 'u':
						case 'U': {
							++i;
							if (!isEndOfLine(context, i)) {
								switch (context.line[i]) {
									case 'l':
									case 'L':
										++i;
										break;
									default:
										break;
								}
							}
							break;
						}
						default:
							break;
					}
				}
				if (!isEndOfLine(context, i) &&
				    (std::isalnum(static_cast<unsigned char>(context.line[i])) || context.line[i] == '_')) {
					throw LexerError(
					    context.linePos,
					    std::string("Unexpected character '") + context.line[i] +
					        "' after numeric literal: " +
					        std::string(context.line + context.pos, i - context.pos) +
					        context.line[i] + "\nHint: Separate number from identifier with space or operator");
				}
				goto ended;
			}
			default: {
				if (std::isalpha(static_cast<unsigned char>(chr))) {
					throw LexerError(
					    context.linePos,
					    std::string("Unexpected character '") + chr +
					        "' after numeric literal: " +
					        std::string(context.line + context.pos, i - context.pos) +
					        chr + "\nHint: Separate number from identifier with space or operator");
				}
				goto ended;
			}
		}
	}
ended:;
	if (hasUnderscore) {
		std::string newStr;
		size_t size = i - context.pos;
		newStr.reserve(size);
		auto pos = context.line + context.pos;
		for (int j = 0; j < size; ++j) {
			char chr = pos[j];
			if (chr == '_')
				continue;
			newStr += chr;
		}
		return newStr;
	}
	return std::string(context.line + context.pos, i - context.pos);
}

template <bool addLParen, bool isChar, bool isRawString>
void loadQuote(Context &context, uint32_t &i) {
	constexpr char quote = isChar ? '\'' : '\"';
	bool isSpecialCase = false;
	std::string newStr;
	char chr;
back:;
	for (; !isEndOfLine(context, i); ++i) {
		chr = context.line[i];
		if (!isSpecialCase) {
			switch (chr) {
				case '\\': {
					if constexpr (!isRawString) {
						isSpecialCase = true;
					} else {
						newStr += '\\';
					}
					continue;
				}
				//("Hello ${value + value} ${value}")
				case '$': {
					if (isEndOfLine(context, ++i)) {
						if constexpr (isRawString) {
							newStr += '$';
							break;
						}
						throw LexerError(context.linePos,
						                 std::string("Expected '") + quote +
						                     "' but not found\nHint: Close string literal with '" + quote + "'");
					}
					if constexpr (isChar) {
						throw LexerError(context.linePos,
						                 "Invalid char literal: interpolation "
 						                 "is not allowed\nHint: Char literal '...' cannot contain '${...}' string interpolation");
						return;
					}
					auto linePos = context.linePos;
					chr = context.line[i];
					if (chr != '{') {
						if (!std::isalpha(chr) && chr != '_') {
							--i;
							chr = context.line[i];
							break;
						}
						if constexpr (addLParen) {
							context.tokens.emplace_back(linePos,
							                            TokenType::LPAREN);
						}
						context.tokens.emplace_back(
						    linePos, TokenType::STRING,
						    pushLexerString(context, std::move(newStr)));
						context.tokens.emplace_back(linePos,
 						                            TokenType::PLUS);
						context.pos = i;
						pushIdentifier(context, i);
						if (context.line[i] == quote) {
							if constexpr (!isRawString) {
								++i;
								if constexpr (addLParen) {
									context.tokens.emplace_back(
									    linePos, TokenType::RPAREN);
								}
								return;
							} else {
								if (!isEndOfLine(context, i + 2) &&
								    context.line[i + 1] == '\"' &&
								    context.line[i + 2] == '\"') {
									i += 3;
									if constexpr (addLParen) {
										context.tokens.emplace_back(
										    linePos, TokenType::RPAREN);
									}
									return;
								}
							}
						}
						context.tokens.emplace_back(linePos, TokenType::PLUS);
						loadQuote<false, isChar, isRawString>(context, i);
						if constexpr (addLParen) {
							context.tokens.emplace_back(linePos,
							                            TokenType::RPAREN);
						}
						return;
					}
					if constexpr (addLParen) {
						context.tokens.emplace_back(linePos, TokenType::LPAREN);
					}
					context.tokens.emplace_back(
					    linePos, TokenType::STRING,
					    pushLexerString(context, std::move(newStr)));
					context.tokens.emplace_back(linePos, TokenType::PLUS);
					// '{' => '(' to support "Hello ${a}"  => ("Hello" + (a))
					// instead of ("Hello" + {a})
					uint32_t bracketReplacePos = context.tokens.size();
					pushAndEnsureBracket(context, i);
					context.tokens[bracketReplacePos].type = TokenType::LPAREN;
					context.tokens.back().type = TokenType::RPAREN;
					if constexpr (!isRawString) {
						if (isEndOfLine(context, i)) {
							throw LexerError(linePos,
							                 std::string("Expected '") + quote +
							                     "' but not found\nHint: Close string literal with '" + quote + "'");
						}
						if (context.line[i] == quote) {
							++i;
							if constexpr (addLParen) {
								context.tokens.emplace_back(linePos,
								                            TokenType::RPAREN);
							}
							return;
						}
					} else {
						if (!isEndOfLine(context, i + 2) &&
						    context.line[i] == '\"' &&
						    context.line[i + 1] == '\"' &&
						    context.line[i + 2] == '\"') {
							i += 3;
							if constexpr (addLParen) {
								context.tokens.emplace_back(linePos,
								                            TokenType::RPAREN);
							}
							return;
						}
					}

					context.tokens.emplace_back(linePos, TokenType::PLUS);
					loadQuote<false, isChar, isRawString>(context, i);
					if constexpr (addLParen) {
						context.tokens.emplace_back(linePos,
						                            TokenType::RPAREN);
					}
					return;
				}
			}
			if (chr == quote) {
				++i;
				if constexpr (isChar) {
					switch (newStr.size()) {
						case 0: {
							context.tokens.emplace_back(
							    context.linePos, TokenType::NUMBER,
							    pushLexerString(context, "0"));
							return;
						}
						case 1: {
							uint8_t value = newStr[0];
							context.tokens.emplace_back(
							    context.linePos, TokenType::NUMBER,
							    pushLexerString(context,
							                    std::to_string(value)));
							return;
						}
						default: {
							throw LexerError(
							    context.linePos,
							    "Invalid char literal: expected "
							    "exactly 1 Unicode code point, got " +
							        std::to_string(newStr.size()) + "\nHint: Char literal must contain exactly one character, use \"...\" for strings");
						}
					}
				}
				if constexpr (!isRawString) {
					context.tokens.emplace_back(
					    context.linePos, TokenType::STRING,
					    pushLexerString(context, std::move(newStr)));
					return;
				} else {
					if (!isEndOfLine(context, i + 1) &&
					    context.line[i] == '\"' &&
					    context.line[i + 1] == '\"') {
						i += 2;
						context.tokens.emplace_back(
						    context.linePos, TokenType::STRING,
						    pushLexerString(context, std::move(newStr)));
						return;
					}
					newStr += '\"';
					newStr += context.line[i];
					continue;
				}
			}
			newStr += chr;
			continue;
		}
		switch (chr) {
			case 'n':
				newStr += '\n';
				break;
			case 't':
				newStr += '\t';
				break;
			case '\\':
				newStr += '\\';
				break;
			case '\'':
				newStr += '\'';
				break;
			case '\"':
				newStr += '\"';
				break;
			case 'r':
				newStr += '\r';
				break;
			case '0':
				newStr += '\0';
				break;
			default:
				throw LexerError(context.linePos,
				                 std::string("Unknown escape sequence '\\") +
				                     chr + "'\nHint: Use valid escape sequence such as \\n, \\t, \\\\, \\', \\\", \\r, \\0");
		}
		isSpecialCase = false;
	}
	if constexpr (isRawString) {
		if (!nextLine(context, ParserContext::mode->rawData.data(), i)) {
			throw LexerError(context.linePos,
			                 std::string("Expected '\"\"\"' but not found\nHint: Close multiline raw string with '\"\"\"'"));
		}
		newStr += '\n';
		goto back;
	}
	throw LexerError(context.linePos,
	                 std::string("Expected '") + quote + "' but not found\nHint: Close string literal with '" + quote + "'");
}

template void loadQuote<true, false, true>(Context &context, uint32_t &i);
template void loadQuote<true, false, false>(Context &context, uint32_t &i);
template void loadQuote<true, true, false>(Context &context, uint32_t &i);
template void loadQuote<false, false, true>(Context &context, uint32_t &i);
template void loadQuote<false, false, false>(Context &context, uint32_t &i);
template void loadQuote<false, true, false>(Context &context, uint32_t &i);

} // namespace Lexer
} // namespace Autolang
