#include "utils.h"

bool recpp::utils::startsWith(const std::string& str, const std::string& substr)
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

std::string recpp::utils::makeCanonicalAbs(std::filesystem::path p)
{
    // std::clog << "path before: " << p.string() << std::endl;
    if (p.is_relative())
    {
        p = std::filesystem::current_path() / p;
        // std::clog << "cwd: " << std::filesystem::current_path().string()
        //           << " path before: " << p.string() << std::endl;
    }
    return std::filesystem::absolute(std::filesystem::canonical(p)).string();
}

bool recpp::utils::isSubpath(
    const std::filesystem::path& path,
    const std::filesystem::path& subpath)
{
    auto canonicalPath = makeCanonicalAbs(path);
    auto canonicalSubpath = makeCanonicalAbs(subpath);
    // std::clog << "path: " << canonical_path << " subpath: " <<
    // canonical_subpath
    //           << std::endl;
    return startsWith(canonicalSubpath, canonicalPath);
}

clang::SourceLocation recpp::utils::getEndPositionOfToken(
    clang::SourceLocation const& startOfToken,
    const clang::SourceManager& sm)
{
    clang::LangOptions lopt;
    return clang::Lexer::getLocForEndOfToken(startOfToken, 0, sm, lopt);
}


std::string recpp::utils::getTypeString(const clang::QualType& type, bool cppStyle)
{
    clang::LangOptions lopt;
    clang::PrintingPolicy policy(lopt);
    if (cppStyle)
    {
        policy.adjustForCPlusPlus();
    }

    return type.getAsString(policy);
}


std::string recpp::utils::getCharacterData(
    const clang::SourceManager& sm,
    const clang::SourceRange& range)
{
    return { sm.getCharacterData(range.getBegin()), sm.getCharacterData(range.getEnd()) };
}


std::string recpp::utils::getSymbolString(
    const clang::SourceManager& sm,
    const clang::SourceRange& range)
{
    return { sm.getCharacterData(range.getBegin()),
             sm.getCharacterData(recpp::utils::getEndPositionOfToken(range.getEnd(), sm)) };
}


std::string recpp::utils::getFunctionDefinitionString(
    const clang::SourceManager& sm,
    const clang::FunctionDecl& fDecl)
{
    const clang::FunctionType* fType = fDecl.getFunctionType();
    bool hasTrailingReturn = false;
    if (clang::FunctionType::classof(fType))
    {
        const clang::FunctionProtoType* fTypeProto = fType->getAs<clang::FunctionProtoType>();
        hasTrailingReturn = fTypeProto->hasTrailingReturn();
    }
    bool isConst = fType ? fType->isConst() : false;

    // const clang::FunctionProtoType* fTypeProto = dynamic_cast<clang::FunctionProtoType>(
    //     fType);
    // std::clog << "FunctionType found: " << (fType ? "true" : "false") << std::endl;
    // std::clog << "has trailing return: " << (hasTrailingReturn ? "true" : "false") << std::endl;
    // std::clog << " return type: "
    //           << getTypeString(fDecl.getReturnType()) // getSymbolString(sm,
    //                                                   // fDecl.getReturnTypeSourceRange())
    //           << std::endl;
    // std::clog << " param: " << getSymbolString(sm, fDecl.getParametersSourceRange()) <<
    // std::endl; std::clog << " full decl string: "
    //           << getSymbolString(sm, fDecl.DeclaratorDecl::getSourceRange()) << std::endl;
    // std::clog << " body: " << getSymbolString(sm, fDecl.getBody()->getSourceRange()) <<
    // std::endl; std::clog << " type spec: "
    //           << getSymbolString(
    //                  sm, clang::SourceRange(fDecl.getTypeSpecStartLoc(),
    //                  fDecl.getTypeSpecEndLoc()))
    //           << std::endl;
    std::string functionDefString;

    if (hasTrailingReturn)
    {
        functionDefString += "auto";
    }
    else
    {
        functionDefString += getTypeString(fDecl.getReturnType());
    }

    functionDefString += std::string(" ") + fDecl.getQualifiedNameAsString() + "(";
    int i = 0;
    auto params = fDecl.getFunctionTypeLoc().getParams();
    for (const clang::ParmVarDecl* const param : params)
    {
        i++;
        functionDefString += getTypeString(param->getType()) + " " + param->getName().str();
        if (i < params.size())
        {
            functionDefString += ", ";
        }
        std::clog
            << "Param: " << param->getName().str() << " type: "
            << getTypeString(param->getType())
            // << getSymbolString(sm, .)

            << " full type?: "
            << getSymbolString(
                   sm, clang::SourceRange(param->getTypeSpecStartLoc(), param->getTypeSpecEndLoc()))
            // << getSymbolString(
            //        sm, param->getTypeSourceInfo()->getTypeLoc().getSourceRange())
            //   << " qualifier: "
            //   << (param->getType().getQualifier().getQualifierLoc().hasQualifier() ||
            //               true ?
            //           getSymbolString(
            //               sm,
            //               param->getType()
            //                   .getQualifier()
            //                   .getQualifierLoc()
            //                   .getSourceRange()) :
            //           "<none>")
            << " default: "
            << (param->hasDefaultArg() ? getSymbolString(sm, param->getDefaultArgRange()) : "none")
            << std::endl;
    }
    functionDefString += ")";
    if (isConst)
    {
        functionDefString += " const";
    }
    if (hasTrailingReturn)
    {
        functionDefString += " -> " + getTypeString(fDecl.getReturnType());
    }

    auto body = fDecl.getBody();
    if (body)
    {
        functionDefString += "\n" + getSymbolString(sm, body->getSourceRange());
    }
    else
    {
        functionDefString += "{}";
    }
    return functionDefString;
}


std::string recpp::utils::getSymbolString(
    const clang::SourceManager& sm,
    const clang::SourceLocation& loc)
{
    return { sm.getCharacterData(loc),
             sm.getCharacterData(recpp::utils::getEndPositionOfToken(loc, sm)) };
}
