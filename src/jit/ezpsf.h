// The main header file for using EzPsf. You should call ezpsf::getPsf<Adapter>(std::string, llvm::orc::LLLazyJit*) to
// create a new function which is returned by the function. Note, this function is not thread safe!
//
// Created by Max Norfolk on 5/29/23.


#pragma once

#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/IRReader/IRReader.h>

#include "jit/ezpsf_visitor.h"
#include "jit/index_generator.h"

namespace fishstore::ezpsf {
    typedef bool (*EzPsf)(EzRecord*, void*);


    // information necessary for a PSF
    struct EzPsfInfo {
        EzPsfInfo(EzPsf psf, std::string name, fishstore::ezpsf::DataType type, std::vector<std::string> fields,
                  std::vector<fishstore::ezpsf::DataType> field_types, std::string ir)
            : psf(psf),
              name(std::move(name)),
              type(type),
              fields(std::move(fields)),
              field_types(std::move(field_types)),
              ir(std::move(ir)) {}

        EzPsfInfo(EzPsf psf, std::string name, fishstore::ezpsf::DataType type, std::vector<std::string> fields,
                  std::vector<fishstore::ezpsf::DataType> field_types, std::string ir,
                  std::unique_ptr<ast::PsfFunctionAst> ast)
            : psf(psf),
              name(std::move(name)),
              type(type),
              fields(std::move(fields)),
              field_types(std::move(field_types)),
              ir(std::move(ir)),
              ast(std::move(ast)) {}

        EzPsfInfo(const EzPsfInfo& other) = delete;

        EzPsfInfo(EzPsfInfo&& other) noexcept = default;

        EzPsfInfo& operator=(const EzPsfInfo& other) = delete;

        EzPsfInfo& operator=(EzPsfInfo&& other) noexcept = default;

        EzPsf psf;
        std::string name;
        fishstore::ezpsf::DataType type;
        std::vector<std::string> fields;
        std::vector<fishstore::ezpsf::DataType> field_types;
        std::string ir;
        std::unique_ptr<ast::PsfFunctionAst> ast = nullptr;
    };

    // The template must support the following:
    // The record type must have an at(size_t) method which returns an object that
    // supports GetAsX() for each X type. The return type of this method should be a
    // nullable as defined in datatypes/conversion_declaration.h
    //
    // These nullables need a Value() and HasValue() method.
    template<typename EzRecordType>
    void initJit(llvm::orc::LLJIT* jit) {
        // setup symbol table
        llvm::orc::MangleAndInterner mangle(jit->getExecutionSession(), jit->getDataLayout());
        auto builtin_symbols = llvm::orc::absoluteSymbols(ezpsf::type_conversion::getSymbolTable<EzRecordType>(mangle));
        LLVM_ERROR_CHECK(jit->getMainJITDylib().define(builtin_symbols));
    }

    template<typename IndexGen>
    EzPsfInfo getPsf(const std::string& str, llvm::orc::LLJIT* jit, IndexGen& index_gen) {
        class : public antlr4::ANTLRErrorListener {
        public:
            std::string source_str;

            void syntaxError(antlr4::Recognizer* recognizer, antlr4::Token* offendingSymbol, size_t line,
                             size_t charPositionInLine, const std::string& msg, std::exception_ptr e) override {
                std::stringstream ss;
                ss << "Syntax error in position " << charPositionInLine << ". [" << msg << "].\n"
                        << "In String '" << source_str << "' Character: '" << source_str[charPositionInLine] << "'.";
                LOG(ss.str());
            }

            void reportAmbiguity(antlr4::Parser* recognizer, const antlr4::dfa::DFA& dfa, size_t startIndex,
                                 size_t stopIndex,
                                 bool exact, const antlrcpp::BitSet& ambigAlts,
                                 antlr4::atn::ATNConfigSet* configs) override {
                LOG("Ambiguity reported");
            }

            void reportAttemptingFullContext(antlr4::Parser* recognizer, const antlr4::dfa::DFA& dfa, size_t startIndex,
                                             size_t stopIndex, const antlrcpp::BitSet& conflictingAlts,
                                             antlr4::atn::ATNConfigSet* configs) override {
                LOG("Attempting Full Context reported");
            }

            void reportContextSensitivity(antlr4::Parser* recognizer, const antlr4::dfa::DFA& dfa, size_t startIndex,
                                          size_t stopIndex, size_t prediction,
                                          antlr4::atn::ATNConfigSet* configs) override {
                LOG("Context Sensitivity reported");
            }
        } error_listener;
        error_listener.source_str = str;

        antlr4::ANTLRInputStream input(str);
        ezpsfLexer lexer(&input);
        lexer.addErrorListener(&error_listener);

        antlr4::CommonTokenStream tokens(&lexer);
        ezpsfParser parser(&tokens);

        parser.addErrorListener(&error_listener);
        auto psf = parser.psf();


        ast::EzPsfVisitor visitor;
        auto psf_ast = visitor.createPsf(psf);
        auto func = psf_ast->generate<IndexGen>(&index_gen);
        std::string name = func->getName().str();


        // output to a file
        // std::error_code ec;
        // llvm::raw_fd_ostream module_out(name + ".txt", ec);
        // llvm_consts::module->print(module_out, nullptr);

        std::string psf_ir;
        llvm::raw_string_ostream module_out(psf_ir);
        llvm_consts::module->print(module_out, nullptr);


        llvm::orc::ThreadSafeModule ts_module(std::move(llvm_consts::module), std::move(llvm_consts::ctx));
        LLVM_ERROR_CHECK(jit->addIRModule(std::move(ts_module)));

        auto expected_symbol = jit->lookup(name);
        ASSERT(expected_symbol);
        auto psf_func_ptr = reinterpret_cast<EzPsf>(expected_symbol->getAddress());
        auto ret_type = psf_ast->return_type;
        auto fields = psf_ast->fields;
        auto field_types = psf_ast->field_types;
        return {psf_func_ptr, name, ret_type, fields, field_types, psf_ir, std::move(psf_ast)};
    }

    template<typename IndexGen = index_gen::CounterIndexGenerator>
    EzPsfInfo getPsf(const std::string& str, llvm::orc::LLJIT* jit) {
        IndexGen temp{};
        return getPsf(str, jit, temp);
    }

    // recovers a PSF from IR. It assumes some information is already saved, and only gets the function pointer
    // back.
    inline EzPsf recoverPsf(const std::string& name, const std::string& ir, llvm::orc::LLJIT* jit) {
        const auto buffer = llvm::MemoryBuffer::getMemBuffer(ir);
        llvm::SMDiagnostic smd;
        auto recovered_ctx = std::make_unique<llvm::LLVMContext>();
        auto recovered_module = llvm::parseIR(*buffer, smd, *recovered_ctx);

        llvm::orc::ThreadSafeModule ts_module(std::move(recovered_module), std::move(recovered_ctx));
        LLVM_ERROR_CHECK(jit->addIRModule(std::move(ts_module)));

        auto expected_symbol = jit->lookup(name);
        ASSERT(expected_symbol);
        return reinterpret_cast<EzPsf>(expected_symbol->getAddress());
    }
}
