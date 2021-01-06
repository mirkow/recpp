#include "FunctionMover.h"

#include <clang/Lex/Lexer.h>

using namespace recpp;


void recpp::FunctionToCppMover::run(const MatchFinder::MatchResult& result)
{

    using namespace clang;
    using namespace utils;
    FunctionDecl* fDecl = const_cast<FunctionDecl*>(result.Nodes.getNodeAs<FunctionDecl>(fdBdName));
    FunctionDecl* fSiblingDecl = const_cast<FunctionDecl*>(
        result.Nodes.getNodeAs<FunctionDecl>("siblingFunction"));
    if (fSiblingDecl)
    {
        std::clog << "Sibling func: " << fSiblingDecl->getQualifiedNameAsString() << std::endl;
    }
    if (fDecl)
    {
        SourceManager& sm(result.Context->getSourceManager());

        SourceRange declRange(fDecl->getSourceRange());
        SourceLocation declBegin(declRange.getBegin());
        SourceLocation declStartEnd(declRange.getEnd());
        SourceLocation declEndEnd(recpp::utils::getEndPositionOfToken(declStartEnd, sm));
        auto fullName = fDecl->getQualifiedNameAsString();
        auto currentFile = sm.getFilename(declBegin);
        if (currentFile.str() != filepath)
        {
            std::clog << "Not correct file (" << currentFile.str() << ") expected: " << filepath
                      << std::endl;
            return;
        }
        auto startFileOffset = sm.getFileOffset(declBegin);
        auto siblingFunctionFileOffset = sm.getFileOffset(
            fSiblingDecl->getSourceRange().getBegin());
        if (siblingFunctionFileOffset >= startFileOffset)
        {
            std::clog << "Decl is later: "
                      << sm.getSpellingLineNumber(fSiblingDecl->getSourceRange().getBegin());
            return;
        }
        if ((previousFuncDecl.offset >= 0 &&
             previousFuncDecl.offset >= siblingFunctionFileOffset) ||
            fSiblingDecl->doesThisDeclarationHaveABody() || !fSiblingDecl->isDefined())
        {
            std::clog << "not suitable sibling:  "
                      << sm.getSpellingLineNumber(fSiblingDecl->getSourceRange().getBegin())
                      << " has body:" << fSiblingDecl->doesThisDeclarationHaveABody()
                      << " is defined: " << fSiblingDecl->isDefined() << std::endl;

            return;
        }
        previousFuncDecl.offset = siblingFunctionFileOffset;
        previousFuncDecl.functionName = fSiblingDecl->getQualifiedNameAsString();
        if (auto siblingDef = fSiblingDecl->getDefinition())
        {
            auto fSiblingSourceEnd = siblingDef->getSourceRange().getEnd();
            auto siblingFuncDefFilepath = sm.getFilename(fSiblingSourceEnd);
            auto siblingFuncDefLine = sm.getSpellingLineNumber(fSiblingSourceEnd);
            std::clog << "Sibling def found at: " << siblingFuncDefFilepath.str() << ":"
                      << siblingFuncDefLine << std::endl;
        }
        if (fileOffset > sm.getFileOffset(declEndEnd) || fileOffset < sm.getFileOffset(declBegin))
        {
            std::clog << "Not correct declaration (" << fullName << ") at " << currentFile.str()
                      << ":" << sm.getSpellingLineNumber(declBegin) << std::endl;
            return;
        }
        if (!fDecl->doesThisDeclarationHaveABody())
        {
            std::clog << "Decl doesn't have a body" << std::endl;
            return;
        }
        if (fDecl->isTemplateDecl())
        {
            std::clog << "Templated functions cannot be moved to cpp" << std::endl;
            return;
        }
        std::clog << "fullname: " << fullName
                  << " has body: " << (fDecl->doesThisDeclarationHaveABody() ? "true" : "false")
                  << " isTemplateDecl: " << (fDecl->isTemplateDecl() ? "true" : "false")
                  << ", definition: " << fDecl->isThisDeclarationADefinition() << " at: "
                  << sm.getFilename(fDecl->getNameInfo().getSourceRange().getBegin()).str() << ":"
                  << sm.getSpellingLineNumber(fDecl->getNameInfo().getSourceRange().getBegin())
                  << std::endl;

        auto functionDefString = getFunctionDefinitionString(sm, *fDecl);
        std::clog << "full def str: " << functionDefString << std::endl;


        auto fullDeclString = getSymbolString(sm, fDecl->DeclaratorDecl::getSourceRange());


        const char* buffBegin(sm.getCharacterData(declBegin));
        const char* buffEnd(sm.getCharacterData(declEndEnd));
        std::string const funcString(buffBegin, buffEnd);

        std::clog << "Captured function " << fDecl->getNameAsString() << " declaration:\n'''\n"
                  << funcString << "\n'''\n";

        uint32_t const declLength = sm.getFileOffset(declEndEnd) - sm.getFileOffset(declBegin);
        StringRef funcRef(buffBegin, declLength);

        if (recpp::utils::startsWith(fullDeclString, "inline "))
        {
            fullDeclString = fullDeclString.substr(7);
        }
        clang::tooling::Replacement removal(sm, declBegin, declLength, fullDeclString + ";");


        std::string filename = sm.getFilename(declBegin).str();

        filename = recpp::utils::makeCanonicalAbs(std::string(filename));
        std::clog << "Current folder: " << std::filesystem::current_path().string() << std::endl;
        std::clog << filename << " workspace: " << workspaceFolder << std::endl;
        if (!recpp::utils::isSubpath(workspaceFolder, filename))
        {
            std::clog << filename << " is not a subpath of " << workspaceFolder << std::endl;
            return;
        } // we dont want to edit files outside of the workspace
        bestReplacements[filename] = removal;
        if (replacements[filename].add(removal))
        {
            std::cerr << "Failed to enter replacement to map for file " << filename << "\n";
        }
        filename = searchCorrespondingFile(filename).value();

        functionDefString = "\n\n" + functionDefString + "\n";
        clang::tooling::Replacement insertion(
            filename, std::filesystem::file_size(filename), 0, functionDefString);
        bestReplacements[filename] = insertion;
    }
    else
    {
    }
    return;
}
std::optional<std::filesystem::path> recpp::FunctionToCppMover::searchCorrespondingFile(
    const std::filesystem::path& path)
{
    if (workspaceFolder.empty())
    {
        throw std::runtime_error("workspace folder must not be empty!");
    }
    std::clog << "Searching corresponding file for " << path.string() << " in workspace "
              << workspaceFolder << std::endl;
    namespace fs = std::filesystem;
    auto ext = path.extension();

    auto isCppExt = [&](const std::string& ext) {
        return std::find(cppExtensions.begin(), cppExtensions.end(), ext) != cppExtensions.end();
    };
    auto isHExt = [&](const std::string& ext) {
        return std::find(hExtensions.begin(), hExtensions.end(), ext) != hExtensions.end();
    };
    std::set<std::string> correspondingExts;
    if (isCppExt(ext))
    {
        correspondingExts = hExtensions;
    }
    else if (isHExt(ext))
    {
        correspondingExts = cppExtensions;
    }
    auto canonicalWorkspaceFolder = recpp::utils::makeCanonicalAbs(workspaceFolder);
    int iterationCounter = 0;
    for (auto& ext : correspondingExts)
    {
        fs::path searchPath = recpp::utils::makeCanonicalAbs(path.parent_path());
        auto correspondingFileName = path;
        correspondingFileName = correspondingFileName.replace_extension(ext).filename();
        std::clog << "looking for: " << correspondingFileName.string() << std::endl;
        while (true)
        {
            for (const auto& dirEntry : fs::recursive_directory_iterator(searchPath))
            {
                if (!dirEntry.is_directory())
                {
                    continue;
                }
                // std::clog << "Checking dir: " << dirEntry.path().string()
                //           << std::endl;
                auto candidateFileName = dirEntry / correspondingFileName;
                if (fs::exists(candidateFileName))
                {
                    std::clog << "found file: " << candidateFileName << std::endl;
                    return candidateFileName;
                }
            }
            searchPath = searchPath.parent_path();
            bool isSubPath = recpp::utils::isSubpath(canonicalWorkspaceFolder, searchPath);
            if (!isSubPath)
            {
                break;
            }
            if (iterationCounter > 1000)
            {
                return {}; // avoid inifinite loops
            }
            iterationCounter++;
        }
    }
    return {};
}
