#ifndef LEXER_HPP
#define LEXER_HPP

#include <iostream>
#include <fstream>
#include <vector>
#include "ankerl/unordered_dense.h"

#define AUTOLANG_DEBUG_

inline void printDebug(std::string msg)
{
#ifdef AUTOLANG_DEBUG
	std::cerr << msg << '\n';
#endif
}

inline void printDebug(long msg)
{
#ifdef AUTOLANG_DEBUG
	std::cerr << msg << '\n';
#endif
}

namespace AutoLang
{

	struct ParserContext;

	namespace Lexer
	{

		enum class TokenType : uint8_t
		{
			COMMENT_SINGLE_LINE,
			// ===== Literals =====
			NUMBER,		//	Giá trị số: 123, 3.14
			STRING,		//	Chuỗi: "abc"
			IDENTIFIER, //	Tên biến, tên hàm: abc, foo

			// ===== Operators =====
			PLUS, //	Toán tử cộng: +
			PLUS_PLUS,
			MINUS, //	Trừ: -
			MINUS_MINUS,
			STAR,		 //	Nhân: *
			SLASH,		 //	Chia: /
			PERCENT,	 //	Modulo: %
			EQUAL,		 //	Gán: =
			PLUS_EQUAL,	 //	Cộng gán: +=
			MINUS_EQUAL, //	Trừ gán: -=
			STAR_EQUAL,	 //	Nhân gán: *=
			SLASH_EQUAL, //	Chia gán: /=
			PERCENT_EQUAL,

			// ===== Comparisons =====
			EQEQ,	 //	So sánh bằng: ==
			NOTEQ,	 //	Không bằng: !=
			EQEQEQ,	 //	So sánh bằng: ===
			NOTEQEQ, //	Không bằng: !==
			LT,		 //	Nhỏ hơn: <
			GT,		 //	Lớn hơn: >
			LTE,	 //	Không lớn hơn: <=
			GTE,	 //	Không nhỏ hơn: >=

			// ===== Logical operators =====
			AND_AND, //	Và logic: and
			OR_OR,	 //	Hoặc logic: or

			// ===== Delimiters =====
			LPAREN,	  //	Mở ngoặc tròn: (
			RPAREN,	  //	Đóng ngoặc tròn: )
			LBRACE,	  //	Mở ngoặc nhọn: {
			RBRACE,	  //	Đóng ngoặc nhọn: }
			LBRACKET, //	Mở ngoặc vuông: [
			RBRACKET, //	Đóng ngoặc vuông: ]
			COMMA,	  //	Dấu phẩy: ,
			DOT,	  //	Dấu chấm: .
			QMARK_DOT,//    Dấu ?.
			DOT_DOT,
			SEMICOLON, //	Dấu chấm phẩy: ;
			COLON,	   //	Dấu hai chấm: :
			QMARK,	   //   Dấu hỏi chấm
			EXMARK,    //   Dấu chấm than

			// ===== Keywords =====
			IF,		  //	Câu lệnh điều kiện: if
			ELSE,	  //	Nhánh else
			WHILE,	  //	Vòng lặp while
			FOR,	  //	Vòng lặp for
			FUNC,	  //	Định nghĩa hàm: func / def
			RETURN,	  //	Trả giá trị về: return
			VAL,	  //	Khai báo biến bất biến: val
			VAR,	  //	Khai báo biến thay đổi: var
			BREAK,	  //	Dừng vòng lặp: break
			CONTINUE, //	Bỏ qua vòng lặp hiện tại: continue
			NOT,
			AND,
			OR,
			IN,
			PUBLIC,
			PRIVATE,
			PROTECTED,
			CLASS,
			CONSTRUCTOR,
			STATIC,
			TRY,
			CATCH,
			THROW,

			// ===== Special =====
			END_OF_FILE, //	Kết thúc file
			INVALID		 //	Token không hợp lệ
		};

		static const ankerl::unordered_dense::map<std::string, TokenType> CAST = {
			{"var", TokenType::VAR},
			{"val", TokenType::VAL},
			{"not", TokenType::NOT},
			{"while", TokenType::WHILE},
			{"if", TokenType::IF},
			{"else", TokenType::ELSE},
			{"and", TokenType::AND_AND},
			{"for", TokenType::FOR},
			{"in", TokenType::IN},
			{"or", TokenType::OR_OR},
			{"func", TokenType::FUNC},
			{"return", TokenType::RETURN},
			{"continue", TokenType::CONTINUE},
			{"break", TokenType::BREAK},
			{"try", TokenType::TRY},
			{"catch", TokenType::CATCH},
			{"throw", TokenType::THROW},
			{"class", TokenType::CLASS},
			{"static", TokenType::STATIC},
			{"private", TokenType::PRIVATE},
			{"public", TokenType::PUBLIC},
			{"protected", TokenType::PROTECTED},
			{"constructor", TokenType::CONSTRUCTOR},

			{"&", TokenType::AND},
			{"|", TokenType::OR},
			{"&&", TokenType::AND_AND},
			{"||", TokenType::OR_OR},
			{"//", TokenType::COMMENT_SINGLE_LINE},
			{"?", TokenType::QMARK},
			{"?.", TokenType::QMARK_DOT},
			{"!", TokenType::EXMARK},
			{".", TokenType::DOT},
			{"..", TokenType::DOT_DOT},
			{"+", TokenType::PLUS},
			{"++", TokenType::PLUS_PLUS},
			{"-", TokenType::MINUS},
			{"--", TokenType::MINUS_MINUS},
			{"*", TokenType::STAR},
			{"/", TokenType::SLASH},
			{"%", TokenType::PERCENT},
			{"+=", TokenType::PLUS_EQUAL},
			{"-=", TokenType::MINUS_EQUAL},
			{"*=", TokenType::STAR_EQUAL},
			{"/=", TokenType::SLASH_EQUAL},
			{"=", TokenType::EQUAL},
			{"<", TokenType::LT},
			{">", TokenType::GT},
			{"==", TokenType::EQEQ},
			{"!=", TokenType::NOTEQ},
			{"===", TokenType::EQEQEQ},
			{"!==", TokenType::NOTEQEQ},
			{">=", TokenType::GTE},
			{"<=", TokenType::LTE},
		};

		struct Token
		{
			uint32_t line;
			uint32_t indexData;
			TokenType type;
			Token(uint32_t line, TokenType type, uint32_t indexData) : line(line), indexData(indexData), type(type) {}
			Token(uint32_t line, TokenType type) : line(line), indexData(0), type(type) {}
			std::string toString(ParserContext &context);
		};

		struct Estimate
		{
			uint32_t declaration = 0;
			uint32_t classes = 0;
			uint32_t functions = 0;
			uint32_t constructorNode = 0;
			uint32_t ifNode = 0;
			uint32_t whileNode = 0;
			uint32_t returnNode = 0;
			uint32_t setNode = 0;
			uint32_t binaryNode = 0;
		};

		struct Context
		{
			ParserContext *mainContext;
			const char *line;
			size_t lineSize;
			size_t nextLinePosition = 0;
			size_t totalSize;
			uint32_t linePos;
			uint32_t pos;
			uint32_t absolutePos;

			Estimate estimate;
		};

		inline bool nextLine(Context &context, const char *lines, uint32_t &i);
		inline bool isOperator(char chr);
		inline bool isEndOfLine(Context &context, uint32_t &i);
		std::vector<Token> load(ParserContext *mainContext, const char *path, Context &context);
		std::vector<Token> load(ParserContext *mainContext, std::pair<const char *, size_t> &lineData, Context &context);
		std::string loadQuote(Context &context, char quote, uint32_t &i);
		std::string loadIdentifier(Context &context, uint32_t &i);
		std::string loadNumber(Context &context, uint32_t &i);
		TokenType loadOp(Context &context, uint32_t &i);
		void pushIdentifier(Context &context, std::vector<Token> &tokens, uint32_t &i);
		uint32_t pushLexerString(Context &context, std::string &&str);

	}
}

#endif