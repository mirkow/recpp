#include "utils.h"

namespace recpp::utils
{
bool startsWith(const std::string& str, const std::string& substr)
{
    if (substr.size() > str.size())
    {
        return false;
    }
    for (size_t i = 0; i < str.size() && i < substr.size(); i++)
    {
        if (str[i] != substr[i])
        {
            return false;
        }
    }
    return true;
}

std::string make_canonical_abs(std::filesystem::path p)
{
    // std::cout << "path before: " << p.string() << std::endl;
    if (p.is_relative())
    {
        p = std::filesystem::current_path() / p;
        // std::cout << "cwd: " << std::filesystem::current_path().string()
        //           << " path before: " << p.string() << std::endl;
    }
    return std::filesystem::absolute(std::filesystem::canonical(p)).string();
}

bool isSubpath(const std::filesystem::path& path, const std::filesystem::path& subpath)
{
    auto canonical_path = make_canonical_abs(path);
    auto canonical_subpath = make_canonical_abs(subpath);
    // std::cout << "path: " << canonical_path << " subpath: " <<
    // canonical_subpath
    //           << std::endl;
    return startsWith(canonical_subpath, canonical_path);
}

clang::SourceLocation getEndPositionOfToken(
    clang::SourceLocation const& startOfToken,
    clang::SourceManager& sm)
{
    clang::LangOptions lopt;
    return clang::Lexer::getLocForEndOfToken(startOfToken, 0, sm, lopt);
}

} // namespace recpp::utils