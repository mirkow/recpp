
#include "utils.h"

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Lex/Lexer.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Refactoring.h>
#include <filesystem>
#include <iostream>
#include <llvm/Support/CommandLine.h>
#include <string>

using namespace clang::tooling;
using namespace llvm;
using clang::SourceLocation;
using clang::SourceManager;
using clang::ast_matchers::MatchFinder;

struct FunctionToCppMover : public clang::ast_matchers::MatchFinder::MatchCallback
{
    using repl_map_t = std::map<std::string, clang::tooling::Replacements>;

    auto matcher(std::string const& fname)
    {
        using namespace clang::ast_matchers;
        // clang-format off
    return
    functionDecl(
      hasName(fname)
    ).bind(fd_bd_name_);
        // clang-format on
    } // matcher

    struct FunctionInfo
    {
        SourceLocation full_function;
        SourceLocation function_name_location;
        bool isDefinition;
    };
    std::vector<FunctionInfo> functionInfos;

    std::string getCharacterData(clang::SourceManager& sm, const clang::SourceRange& range)
    {
        return std::string(
            sm.getCharacterData(range.getBegin()), sm.getCharacterData(range.getEnd()));
    }

    std::string getSymbolString(clang::SourceManager& sm, const clang::SourceRange& range)
    {
        return std::string(
            sm.getCharacterData(range.getBegin()),
            sm.getCharacterData(recpp::utils::getEndPositionOfToken(range.getEnd(), sm)));
    }
    virtual void run(MatchFinder::MatchResult const& result) override
    {
        using namespace clang;
        FunctionDecl* f_decl = const_cast<FunctionDecl*>(
            result.Nodes.getNodeAs<FunctionDecl>(fd_bd_name_));
        if (f_decl)
        {
            SourceManager& sm(result.Context->getSourceManager());
            // auto parents = result.Context->getParents(*f_decl);
            // for(auto & p : parents)
            // {
            //   p.dump(llvm::outs(), sm);
            // }
            auto fullName = f_decl->getQualifiedNameAsString();
            std::cout << "fullname: " << fullName << " has body: "
                      << (f_decl->doesThisDeclarationHaveABody() ? "true" : "false")
                      << ", definition: " << f_decl->isThisDeclarationADefinition() << " at: "
                      << sm.getFilename(f_decl->getNameInfo().getSourceRange().getBegin()).str()
                      << ":"
                      << sm.getSpellingLineNumber(f_decl->getNameInfo().getSourceRange().getBegin())
                      << std::endl;
            if (!f_decl->doesThisDeclarationHaveABody())
            {
                std::cout << "Decl doesn't have a body" << std::endl;
                return;
            }
            std::cout << " return type: " << getSymbolString(sm, f_decl->getReturnTypeSourceRange())
                      << std::endl;
            std::cout << " param: " << getSymbolString(sm, f_decl->getParametersSourceRange())
                      << std::endl;
            std::cout << " full decl string: "
                      << getSymbolString(sm, f_decl->DeclaratorDecl::getSourceRange()) << std::endl;
            auto full_decl_string = getSymbolString(sm, f_decl->DeclaratorDecl::getSourceRange());
            SourceRange decl_range(f_decl->getSourceRange());
            SourceLocation decl_begin(decl_range.getBegin());
            // std::cout << "Location: " << sm.getCharacterData(decl_begin) <<
            // std::endl;
            SourceLocation decl_start_end(decl_range.getEnd());
            SourceLocation decl_end_end(recpp::utils::getEndPositionOfToken(decl_start_end, sm));

            const char* buff_begin(sm.getCharacterData(decl_begin));
            const char* buff_end(sm.getCharacterData(decl_end_end));
            std::string const func_string(buff_begin, buff_end);

            // now you have original source of declaration, output to new file etc.
            std::cout << "Captured function " << f_decl->getNameAsString() << " declaration:\n'''\n"
                      << func_string << "\n'''\n";
            // Generate a replacement to eliminate the function declaration in
            // the original source
            uint32_t const decl_length = sm.getFileOffset(decl_end_end) -
                sm.getFileOffset(decl_begin);
            StringRef funcRef(buff_begin, decl_length);

            if (recpp::utils::startsWith(full_decl_string, "inline "))
            {
                full_decl_string = full_decl_string.substr(7);
            }
            Replacement removal(sm, decl_begin, decl_length, full_decl_string + ";");
            // now add to the replacements for the source file in which this
            // declaration was found

            std::string filename = sm.getFilename(decl_begin).str();

            filename = recpp::utils::make_canonical_abs(std::string(filename));
            std::cout << "Current folder: " << std::filesystem::current_path().string()
                      << std::endl;
            std::cout << filename << " workspace: " << workspace_folder << std::endl;
            if (!recpp::utils::isSubpath(workspace_folder, filename))
            {
                std::cout << filename << " is not a subpath of " << workspace_folder << std::endl;
                return;
            } // we dont want to edit files outside of the workspace
            if (repls_[filename].add(removal))
            {
                std::cerr << "Failed to enter replacement to map for file " << filename << "\n";
            }
            filename = searchCorrespondingFile(filename).value();
            std::string func_def = funcRef.str();
            if (recpp::utils::startsWith(func_def, "static "))
            {
                func_def = func_def.substr(7);
            }
            if (recpp::utils::startsWith(func_def, "inline "))
            {
                func_def = func_def.substr(7);
            }
            Replacement insertion(filename, std::filesystem::file_size(filename), 0, func_def);
            if (repls_[filename].add(insertion))
            {
                std::cerr << "Failed to enter replacement to map for file " << filename << "\n";
            }
        }
        else
        {
        }
        return;
    } // run

    std::optional<std::filesystem::path> searchCorrespondingFile(const std::filesystem::path& path)
    {
        if (workspace_folder.empty())
        {
            throw std::runtime_error("workspace folder must not be empty!");
        }
        std::cout << "Searching corresponding file for " << path.string() << " in workspace "
                  << workspace_folder << std::endl;
        namespace fs = std::filesystem;
        auto ext = path.extension();

        auto is_cpp_ext = [&](const std::string& ext) {
            return std::find(cpp_extensions.begin(), cpp_extensions.end(), ext) !=
                cpp_extensions.end();
        };
        auto is_h_ext = [&](const std::string& ext) {
            return std::find(h_extensions.begin(), h_extensions.end(), ext) != h_extensions.end();
        };
        std::set<std::string> corresponding_exts;
        if (is_cpp_ext(ext))
        {
            corresponding_exts = h_extensions;
        }
        else if (is_h_ext(ext))
        {
            corresponding_exts = cpp_extensions;
        }
        auto canonical_workspace_folder = recpp::utils::make_canonical_abs(workspace_folder);
        int iterationCounter = 0;
        for (auto& ext : corresponding_exts)
        {
            fs::path search_path = recpp::utils::make_canonical_abs(path.parent_path());
            auto corresponding_file_name = path;
            corresponding_file_name = corresponding_file_name.replace_extension(ext).filename();
            std::cout << "looking for: " << corresponding_file_name.string() << std::endl;
            while (true)
            {
                for (const auto& dirEntry : fs::recursive_directory_iterator(search_path))
                {
                    if (!dirEntry.is_directory())
                    {
                        continue;
                    }
                    // std::cout << "Checking dir: " << dirEntry.path().string()
                    //           << std::endl;
                    auto candidate_file_name = dirEntry / corresponding_file_name;
                    if (fs::exists(candidate_file_name))
                    {
                        std::cout << "found file: " << candidate_file_name << std::endl;
                        return candidate_file_name;
                    }
                }
                search_path = search_path.parent_path();
                bool is_sub_path = recpp::utils::isSubpath(canonical_workspace_folder, search_path);
                if (!is_sub_path)
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

    explicit FunctionToCppMover(
        repl_map_t& repls,
        const std::string& workspace_folder,
        const std::string& filepath,
        int offset) :
        repls_(repls), workspace_folder(workspace_folder), filepath(filepath), file_offset(offset)
    {
    }
    repl_map_t& repls_;
    std::string workspace_folder;
    std::string filepath;
    int file_offset;
    std::string const fd_bd_name_ = "f_decl";
    std::set<std::string> cpp_extensions = { ".cpp", ".cc", ".cxx" },
                          h_extensions = { ".h", ".hpp", ".hh" };

}; // struct FunctionToCppMover

static llvm::cl::OptionCategory FMOpts("Common options for function-mover");
static llvm::cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);
static llvm::cl::extrahelp MoreHelp("\nMore help text...\n");
static cl::opt<std::string> workspace_folder(
    "workspace-folder",
    cl::desc("root folder of the workspace"),
    cl::value_desc("folder"),
    cl::cat(FMOpts));
static cl::opt<std::string> file(
    "f",
    cl::desc("file, where the symbol is located"),
    cl::value_desc("file"),
    cl::cat(FMOpts),
    cl::init(""));

static cl::opt<std::string> symbol(
    "s",
    cl::desc("name of the symbol in question"),
    cl::value_desc("symbol"),
    cl::cat(FMOpts));

static cl::opt<int> offset(
    "o",
    cl::desc("offset or character position in the file content"),
    cl::value_desc("offset"),
    cl::cat(FMOpts),
    cl::init(0));

const char* addl_help = "(Incomplete) Demo of moving function from one file to another";

int main(int argc, const char** argv)
{
    std::cout << "Current folder: " << std::filesystem::current_path().string() << std::endl;

    CommonOptionsParser opt_prs(argc, argv, FMOpts, addl_help);
    RefactoringTool tool(opt_prs.getCompilations(), opt_prs.getSourcePathList());
    std::cout << "tool created" << std::endl;
    FunctionToCppMover fm(
        tool.getReplacements(),
        recpp::utils::make_canonical_abs(workspace_folder.c_str()),
        file,
        offset);
    std::cout << "mover created" << std::endl;
    MatchFinder finder;
    std::string const function_name(symbol); // could get from CL options
    finder.addMatcher(fm.matcher(function_name), &fm);
    std::cout << "added matcher" << std::endl;

    // runs, reports, does not overwrite:
    // tool.run(newFrontendActionFactory(&finder).get());
    std::cout << "ran matcher" << std::endl;
    // tool.run(newFrontendActionFactory(&finder).get());
    // invoke instead to overwrite original source:
    tool.runAndSave(newFrontendActionFactory(&finder).get());

    for (auto& p : tool.getReplacements())
    {
        auto& fname = p.first;
        auto& repls = p.second;
        std::cout << "Replacements collected for file \"" << fname << "\" (not applied):\n";
        for (auto& r : repls)
        {
            std::cout << "\t" << r.toString() << "\n";
        }
    }
    return 0;
} // main


// End of file
