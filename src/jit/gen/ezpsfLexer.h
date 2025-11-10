
// Generated from /scratch/mnorfolk/Code/Clion/FishStore-private/src/jit/ezpsf.g4 by ANTLR 4.13.2

#pragma once


#include "antlr4-runtime.h"




class  ezpsfLexer : public antlr4::Lexer {
public:
  enum {
    PERIOD = 1, COMMA = 2, SEMICOLON = 3, COLON = 4, ARROW = 5, QUESTION = 6, 
    STR_T = 7, BYTE_T = 8, INT32_T = 9, INT64_T = 10, UINT32_T = 11, UINT64_T = 12, 
    FLOAT_T = 13, DOUBLE_T = 14, BOOL_T = 15, DEFAULT = 16, RETURN = 17, 
    LIKE = 18, ILIKE = 19, IF = 20, ELIF = 21, ELSE = 22, WHILE = 23, FOR = 24, 
    BREAK = 25, TRUE = 26, FALSE = 27, LIT_NULL = 28, JSON = 29, INT = 30, 
    FLOAT = 31, STRING = 32, COMMENT = 33, BLOCK_COMMENT = 34, WHITESPACE = 35, 
    OPEN_BRACE = 36, CLOSE_BRACE = 37, OPEN_SQUARE = 38, CLOSE_SQUARE = 39, 
    OPEN_PAREN = 40, CLOSE_PAREN = 41, BAR = 42, PLUS = 43, MINUS = 44, 
    MULTI = 45, FLOAT_DIV = 46, INT_DIV = 47, MOD = 48, EQUALS = 49, LT = 50, 
    LT_EQ = 51, GT = 52, GT_EQ = 53, EQ = 54, NEQ = 55, BIT_AND = 56, BIT_XOR = 57, 
    AND = 58, OR = 59, NOT = 60
  };

  explicit ezpsfLexer(antlr4::CharStream *input);

  ~ezpsfLexer() override;


  std::string getGrammarFileName() const override;

  const std::vector<std::string>& getRuleNames() const override;

  const std::vector<std::string>& getChannelNames() const override;

  const std::vector<std::string>& getModeNames() const override;

  const antlr4::dfa::Vocabulary& getVocabulary() const override;

  antlr4::atn::SerializedATNView getSerializedATN() const override;

  const antlr4::atn::ATN& getATN() const override;

  // By default the static state used to implement the lexer is lazily initialized during the first
  // call to the constructor. You can call this function if you wish to initialize the static state
  // ahead of time.
  static void initialize();

private:

  // Individual action functions triggered by action() above.

  // Individual semantic predicate functions triggered by sempred() above.

};

