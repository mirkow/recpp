#pragma once

#include <iostream>

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/Refactoring.h>


#include "utils.h"


namespace recpp
{
class FunctionToCppMover : public clang::ast_matchers::MatchFinder::MatchCallback
{
public:
    using ReplacementMap = std::map<std::string, clang::tooling::Replacements>;
    using MatchFinder = clang::ast_matchers::MatchFinder;


    auto matcher(std::string const& functionName)
    {
        using namespace clang::ast_matchers;

        return functionDecl(hasName(functionName)).bind(fdBdName);
    }

    auto matcher2(std::string const& functionName)
    {
        using namespace clang::ast_matchers;

        return functionDecl(hasParent(recordDecl(
                                hasDescendant(functionDecl(hasName(functionName)).bind(fdBdName)))))
            .bind("siblingFunction");
    }

    struct FunctionInfo
    {
        clang::SourceLocation fullFunction;
        clang::SourceLocation functionNameLocation;
        bool isDefinition;
    };
    std::vector<FunctionInfo> functionInfos;

    void myFunc() {}


    virtual void run(MatchFinder::MatchResult const& result); // run

    std::optional<std::filesystem::path> searchCorrespondingFile(const std::filesystem::path& path);

    void finalizeReplacements()
    {
        for (auto& pair : bestReplacements)
        {
            if (replacements[pair.first].add(pair.second))
            {
                std::cerr << "Failed to enter replacement to map for file " << pair.first << "\n";
            }
        }
    }

    explicit FunctionToCppMover(
        ReplacementMap& replacements,
        const std::string& workspaceFolder,
        const std::string& filepath,
        int offset) :
        replacements(replacements),
        workspaceFolder(workspaceFolder),
        filepath(filepath),
        fileOffset(offset)
    {
    }
    ReplacementMap& replacements;
    std::string workspaceFolder;
    std::string filepath;
    int fileOffset;
    std::string const fdBdName = "f_decl";
    std::set<std::string> cppExtensions = { ".cpp", ".cc", ".cxx" },
                          hExtensions = { ".h", ".hpp", ".hh" };

private:
    struct
    {
        int offset = -1;
        std::string functionName;
    } previousFuncDecl;
    std::map<std::string, clang::tooling::Replacement> bestReplacements;

}; // struct FunctionToCppMover
} // namespace recpp
