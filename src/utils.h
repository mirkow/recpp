#pragma once

#include <filesystem>
#include <string>
#include <clang/Basic/SourceManager.h>
#include <clang/Lex/Lexer.h>

namespace recpp::utils
{

bool startsWith(const std::string& str, const std::string& substr);

std::string make_canonical_abs(std::filesystem::path p);

bool isSubpath(const std::filesystem::path& path, const std::filesystem::path& subpath);

clang::SourceLocation getEndPositionOfToken(
    clang::SourceLocation const& startOfToken,
    clang::SourceManager& sm);

} // namespace recpp::utils