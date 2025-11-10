
// Generated from /scratch/mnorfolk/Code/Clion/FishStore-private/src/jit/ezpsf.g4 by ANTLR 4.13.2

#pragma once


#include "antlr4-runtime.h"




class  ezpsfParser : public antlr4::Parser {
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

  enum {
    RulePsf = 0, RuleExpr = 1, RuleLiteral = 2, RuleAnyId = 3, RuleType = 4
  };

  explicit ezpsfParser(antlr4::TokenStream *input);

  ezpsfParser(antlr4::TokenStream *input, const antlr4::atn::ParserATNSimulatorOptions &options);

  ~ezpsfParser() override;

  std::string getGrammarFileName() const override;

  const antlr4::atn::ATN& getATN() const override;

  const std::vector<std::string>& getRuleNames() const override;

  const antlr4::dfa::Vocabulary& getVocabulary() const override;

  antlr4::atn::SerializedATNView getSerializedATN() const override;


  class PsfContext;
  class ExprContext;
  class LiteralContext;
  class AnyIdContext;
  class TypeContext; 

  class  PsfContext : public antlr4::ParserRuleContext {
  public:
    PsfContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<ExprContext *> expr();
    ExprContext* expr(size_t i);
    antlr4::tree::TerminalNode *JSON();
    antlr4::tree::TerminalNode *ARROW();
    antlr4::tree::TerminalNode *DEFAULT();
    antlr4::tree::TerminalNode *OPEN_PAREN();
    antlr4::tree::TerminalNode *CLOSE_PAREN();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  PsfContext* psf();

  class  ExprContext : public antlr4::ParserRuleContext {
  public:
    ExprContext(antlr4::ParserRuleContext *parent, size_t invokingState);
   
    ExprContext() = default;
    void copyFrom(ExprContext *context);
    using antlr4::ParserRuleContext::copyFrom;

    virtual size_t getRuleIndex() const override;

   
  };

  class  CastExprContext : public ExprContext {
  public:
    CastExprContext(ExprContext *ctx);

    ExprContext *expr();
    antlr4::tree::TerminalNode *OPEN_SQUARE();
    TypeContext *type();
    antlr4::tree::TerminalNode *CLOSE_SQUARE();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  PlusMinusExprContext : public ExprContext {
  public:
    PlusMinusExprContext(ExprContext *ctx);

    antlr4::Token *op = nullptr;
    std::vector<ExprContext *> expr();
    ExprContext* expr(size_t i);
    antlr4::tree::TerminalNode *PLUS();
    antlr4::tree::TerminalNode *MINUS();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  OrExprContext : public ExprContext {
  public:
    OrExprContext(ExprContext *ctx);

    std::vector<ExprContext *> expr();
    ExprContext* expr(size_t i);
    antlr4::tree::TerminalNode *OR();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  ParenExprContext : public ExprContext {
  public:
    ParenExprContext(ExprContext *ctx);

    antlr4::tree::TerminalNode *OPEN_PAREN();
    ExprContext *expr();
    antlr4::tree::TerminalNode *CLOSE_PAREN();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  BitXorExprContext : public ExprContext {
  public:
    BitXorExprContext(ExprContext *ctx);

    std::vector<ExprContext *> expr();
    ExprContext* expr(size_t i);
    antlr4::tree::TerminalNode *BIT_XOR();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  CmpExprContext : public ExprContext {
  public:
    CmpExprContext(ExprContext *ctx);

    antlr4::Token *op = nullptr;
    std::vector<ExprContext *> expr();
    ExprContext* expr(size_t i);
    antlr4::tree::TerminalNode *EQ();
    antlr4::tree::TerminalNode *NEQ();
    antlr4::tree::TerminalNode *LT();
    antlr4::tree::TerminalNode *GT();
    antlr4::tree::TerminalNode *LT_EQ();
    antlr4::tree::TerminalNode *GT_EQ();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  IndexExprContext : public ExprContext {
  public:
    IndexExprContext(ExprContext *ctx);

    std::vector<ExprContext *> expr();
    ExprContext* expr(size_t i);
    antlr4::tree::TerminalNode *OPEN_SQUARE();
    antlr4::tree::TerminalNode *CLOSE_SQUARE();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  BitOrExprContext : public ExprContext {
  public:
    BitOrExprContext(ExprContext *ctx);

    std::vector<ExprContext *> expr();
    ExprContext* expr(size_t i);
    antlr4::tree::TerminalNode *BAR();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  PreUnaryExprContext : public ExprContext {
  public:
    PreUnaryExprContext(ExprContext *ctx);

    antlr4::Token *op = nullptr;
    ExprContext *expr();
    antlr4::tree::TerminalNode *MINUS();
    antlr4::tree::TerminalNode *NOT();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  TernaryExprContext : public ExprContext {
  public:
    TernaryExprContext(ExprContext *ctx);

    std::vector<ExprContext *> expr();
    ExprContext* expr(size_t i);
    antlr4::tree::TerminalNode *QUESTION();
    antlr4::tree::TerminalNode *COLON();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  BarExprContext : public ExprContext {
  public:
    BarExprContext(ExprContext *ctx);

    std::vector<antlr4::tree::TerminalNode *> BAR();
    antlr4::tree::TerminalNode* BAR(size_t i);
    ExprContext *expr();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  LikeExprContext : public ExprContext {
  public:
    LikeExprContext(ExprContext *ctx);

    antlr4::Token *op = nullptr;
    ExprContext *expr();
    antlr4::tree::TerminalNode *STRING();
    antlr4::tree::TerminalNode *LIKE();
    antlr4::tree::TerminalNode *ILIKE();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  MultiDivExprContext : public ExprContext {
  public:
    MultiDivExprContext(ExprContext *ctx);

    antlr4::Token *op = nullptr;
    std::vector<ExprContext *> expr();
    ExprContext* expr(size_t i);
    antlr4::tree::TerminalNode *MULTI();
    antlr4::tree::TerminalNode *FLOAT_DIV();
    antlr4::tree::TerminalNode *INT_DIV();
    antlr4::tree::TerminalNode *MOD();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  BitAndExprContext : public ExprContext {
  public:
    BitAndExprContext(ExprContext *ctx);

    std::vector<ExprContext *> expr();
    ExprContext* expr(size_t i);
    antlr4::tree::TerminalNode *BIT_AND();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  LitExprContext : public ExprContext {
  public:
    LitExprContext(ExprContext *ctx);

    LiteralContext *literal();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  IdExprContext : public ExprContext {
  public:
    IdExprContext(ExprContext *ctx);

    AnyIdContext *anyId();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  AndExprContext : public ExprContext {
  public:
    AndExprContext(ExprContext *ctx);

    std::vector<ExprContext *> expr();
    ExprContext* expr(size_t i);
    antlr4::tree::TerminalNode *AND();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  ExprContext* expr();
  ExprContext* expr(int precedence);
  class  LiteralContext : public antlr4::ParserRuleContext {
  public:
    LiteralContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *INT();
    antlr4::tree::TerminalNode *FLOAT();
    antlr4::tree::TerminalNode *STRING();
    antlr4::tree::TerminalNode *TRUE();
    antlr4::tree::TerminalNode *FALSE();
    antlr4::tree::TerminalNode *LIT_NULL();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  LiteralContext* literal();

  class  AnyIdContext : public antlr4::ParserRuleContext {
  public:
    AnyIdContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *OPEN_PAREN();
    TypeContext *type();
    antlr4::tree::TerminalNode *CLOSE_PAREN();
    antlr4::tree::TerminalNode *JSON();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  AnyIdContext* anyId();

  class  TypeContext : public antlr4::ParserRuleContext {
  public:
    TypeContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *STR_T();
    antlr4::tree::TerminalNode *BYTE_T();
    antlr4::tree::TerminalNode *INT32_T();
    antlr4::tree::TerminalNode *INT64_T();
    antlr4::tree::TerminalNode *UINT32_T();
    antlr4::tree::TerminalNode *UINT64_T();
    antlr4::tree::TerminalNode *FLOAT_T();
    antlr4::tree::TerminalNode *DOUBLE_T();
    antlr4::tree::TerminalNode *BOOL_T();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  TypeContext* type();


  bool sempred(antlr4::RuleContext *_localctx, size_t ruleIndex, size_t predicateIndex) override;

  bool exprSempred(ExprContext *_localctx, size_t predicateIndex);

  // By default the static state used to implement the parser is lazily initialized during the first
  // call to the constructor. You can call this function if you wish to initialize the static state
  // ahead of time.
  static void initialize();

private:
};

