


#include <clang/Tooling/CommonOptionsParser.h>
#include <filesystem>
#include <iostream>
#include <llvm/Support/CommandLine.h>
#include <string>

#include <recpp/FunctionMover.h>

using namespace clang::tooling;
using namespace llvm;
using namespace recpp;
using clang::SourceLocation;
using clang::SourceManager;
using clang::ast_matchers::MatchFinder;


static llvm::cl::OptionCategory fmOpts("Common options for function-mover");
static llvm::cl::extrahelp commonHelp(CommonOptionsParser::HelpMessage);
static llvm::cl::extrahelp moreHelp("\nMore help text...\n");
static cl::opt<std::string> workspaceFolder(
    "workspace-folder",
    cl::desc("root folder of the workspace"),
    cl::value_desc("folder"),
    cl::cat(fmOpts));
static cl::opt<std::string> file(
    "f",
    cl::desc("file, where the symbol is located"),
    cl::value_desc("file"),
    cl::cat(fmOpts),
    cl::init(""));

static cl::opt<std::string> symbol(
    "s",
    cl::desc("name of the symbol in question"),
    cl::value_desc("symbol"),
    cl::cat(fmOpts));

static cl::opt<int> offset(
    "o",
    cl::desc("offset or character position in the file content"),
    cl::value_desc("offset"),
    cl::cat(fmOpts),
    cl::init(0));

static cl::opt<bool> dryRun(
    "dry",
    cl::desc("If true, the action is not applied."),
    cl::value_desc("bool"),
    cl::cat(fmOpts),
    cl::init(false));

const char* addlHelp = "(Incomplete) Demo of moving function from one file to another";

int main(int argc, const char** argv)
{
    std::clog << "Current folder: " << std::filesystem::current_path().string() << std::endl;

    CommonOptionsParser optPrs(argc, argv, fmOpts, addlHelp);
    RefactoringTool tool(optPrs.getCompilations(), optPrs.getSourcePathList());
    FunctionToCppMover fm(
        tool.getReplacements(),
        recpp::utils::makeCanonicalAbs(workspaceFolder.c_str()),
        file,
        offset);

    MatchFinder finder;
    std::string const functionName(symbol); // could get from CL options
    finder.addMatcher(fm.matcher2(functionName), &fm);


    // runs, reports, does not overwrite:
    std::clog << "ran matcher" << std::endl;
    if (dryRun.getValue())
    {
        tool.run(newFrontendActionFactory(&finder).get());
    }
    // invoke instead to overwrite original source:
    else
    {
        tool.runAndSave(newFrontendActionFactory(&finder).get());
    }

    for (auto& p : tool.getReplacements())
    {
        auto& fname = p.first;
        auto& replacements = p.second;
        std::clog << "Replacements collected for file \"" << fname << "\""
                  << (dryRun.getValue() ? "(not applied)" : "") << ":\n";
        for (auto& r : replacements)
        {
            std::clog << "\t" << r.toString() << "\n";
        }
    }
    return 0;
}
