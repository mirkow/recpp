#pragma once

#include <clang/AST/Decl.h>
#include <clang/AST/PrettyPrinter.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/AST/TypeLoc.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Lex/Lexer.h>
#include <filesystem>
#include <iostream>
#include <string>

namespace recpp::utils
{

bool startsWith(const std::string& str, const std::string& substr);

std::string makeCanonicalAbs(std::filesystem::path p);

bool isSubpath(const std::filesystem::path& path, const std::filesystem::path& subpath);

clang::SourceLocation getEndPositionOfToken(
    clang::SourceLocation const& startOfToken,
    const clang::SourceManager& sm);

std::string getCharacterData(const clang::SourceManager& sm, const clang::SourceRange& range);

std::string getSymbolString(const clang::SourceManager& sm, const clang::SourceRange& range);

std::string getSymbolString(const clang::SourceManager& sm, const clang::SourceLocation& loc);

std::string getTypeString(const clang::QualType& type, bool cppStyle = true);

std::string getFunctionDefinitionString(
    const clang::SourceManager& sm,
    const clang::FunctionDecl& fDecl);

} // namespace recpp::utils