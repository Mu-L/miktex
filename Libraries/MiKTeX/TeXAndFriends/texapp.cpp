/**
 * @file texapp.cpp
 * @author Christian Schenk
 * @brief MiKTeX TeX base implementation
 *
 * @copyright Copyright © 1996-2022 Christian Schenk
 *
 * This file is part of the MiKTeX TeXMF Framework.
 *
 * The MiKTeX TeXMF Framework is licensed under GNU General Public License
 * version 2 or any later version.
 */

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <miktex/Configuration/ConfigNames>
#include <miktex/Util/Tokenizer>

#if defined(MIKTEX_TEXMF_SHARED)
#   define C4PEXPORT MIKTEXDLLEXPORT
#else
#   define C4PEXPORT
#endif
#define C1F0C63F01D5114A90DDF8FC10FF410B
#include "miktex/C4P/C4P.h"

#if defined(MIKTEX_TEXMF_SHARED)
#   define MIKTEXMFEXPORT MIKTEXDLLEXPORT
#else
#   define MIKTEXMFEXPORT
#endif
#define B8C7815676699B4EA2DE96F0BD727276
#include "miktex/TeXAndFriends/TeXApp.h"

#include "internal.h"

using namespace std;

using namespace MiKTeX::Core;
using namespace MiKTeX::TeXAndFriends;
using namespace MiKTeX::Util;

#define EXPERT_SRC_SPECIALS 0

class TeXApp::impl
{
public:
    int optBase;
    bool enableMLTeX;
    bool enableEncTeX;
    int synchronizationOptions;
    bitset<32> sourceSpecials;
    IFormatHandler* formatHandler;
    int lastLineNum;
    PathName lastSourceFilename;
};

TeXApp::TeXApp() :
    pimpl(make_unique<impl>())
{
}

TeXApp::~TeXApp() noexcept
{
}

void TeXApp::Init(vector<char*>& args)
{
    TeXMFApp::Init(args);

    IAm(TeXEngine);

    pimpl->enableEncTeX = false;
    pimpl->enableMLTeX = false;
    pimpl->lastLineNum = -1;
# define SYNCTEX_NO_OPTION INT_MAX
    pimpl->synchronizationOptions = SYNCTEX_NO_OPTION;
}

void TeXApp::OnTeXMFStartJob()
{
    TeXMFApp::OnTeXMFStartJob();
    shared_ptr<Session> session = GetSession();
    ShellCommandMode shellCommandMode = session->GetShellCommandMode();
    EnableShellCommands(shellCommandMode);
}

void TeXApp::Finalize()
{
    pimpl->lastSourceFilename = "";
    pimpl->sourceSpecials.reset();
    TeXMFApp::Finalize();
}

enum {
    OPT_DISABLE_WRITE18,
    OPT_ENABLE_ENCTEX,
    OPT_ENABLE_MLTEX,
    OPT_ENABLE_WRITE18,
    OPT_FONT_MAX,
    OPT_FONT_MEM_SIZE,
    OPT_HASH_EXTRA,
    OPT_MAX_IN_OPEN,
    OPT_MEM_BOT,
    OPT_NEST_SIZE,
    OPT_RESTRICT_WRITE18,
    OPT_SAVE_SIZE,
    OPT_SRC_SPECIALS,
    OPT_SYNCTEX,
    OPT_TRIE_OP_SIZE,
    OPT_TRIE_SIZE,
};

void TeXApp::AddOptions()
{
    TeXMFApp::AddOptions();

    pimpl->optBase = (int)GetOptions().size();

    AddOption("disable-write18", T_("Disable the \\write18{COMMAND} construct."),
        FIRST_OPTION_VAL + pimpl->optBase + OPT_DISABLE_WRITE18);

    AddOption("enable-mltex", T_("Enable MLTeX extensions such as \\charsubdef."),
        FIRST_OPTION_VAL + pimpl->optBase + OPT_ENABLE_MLTEX);

    AddOption("enable-write18", T_("Enable the \\write18{COMMAND} construct."),
        FIRST_OPTION_VAL + pimpl->optBase + OPT_ENABLE_WRITE18);

    AddOption("hash-extra", fmt::format(T_("Set {0} to N."), "hash_extra"),
        FIRST_OPTION_VAL + pimpl->optBase + OPT_HASH_EXTRA,
        POPT_ARG_STRING,
        "N");

    AddOption("max-in-open", fmt::format(T_("Set {0} to N."), "max_in_open"),
        FIRST_OPTION_VAL + pimpl->optBase + OPT_MAX_IN_OPEN,
        POPT_ARG_STRING,
        "N");

    AddOption("mem-bot", T_("Set mem_bot to 0 or 1."),
        FIRST_OPTION_VAL + pimpl->optBase + OPT_MEM_BOT,
        POPT_ARG_STRING | POPT_ARGFLAG_DOC_HIDDEN,
        "N");

    AddOption("nest-size", fmt::format(T_("Set {0} to N."), "nest_size"),
        FIRST_OPTION_VAL + pimpl->optBase + OPT_NEST_SIZE,
        POPT_ARG_STRING,
        "N");

    AddOption("restrict-write18", T_("Partially enable the \\write18{COMMAND} construct."),
        FIRST_OPTION_VAL + pimpl->optBase + OPT_RESTRICT_WRITE18);

    AddOption("save-size", fmt::format(T_("Set {0} to N."), "save_size"),
        FIRST_OPTION_VAL + pimpl->optBase + OPT_SAVE_SIZE,
        POPT_ARG_STRING,
        "N");

    if (!AmI("xetex"))
    {
        AddOption("enable-enctex", T_("Enable EncTeX extensions such as \\mubyte."),
            FIRST_OPTION_VAL + pimpl->optBase + OPT_ENABLE_ENCTEX);
    }

#if WITH_SYNCTEX
    if (AmI("xetex") || AmI("pdftex"))
    {
        AddOption("synctex", T_("Generate SyncTeX data for previewers if nonzero."),
            FIRST_OPTION_VAL + pimpl->optBase + OPT_SYNCTEX,
            POPT_ARG_STRING,
            "N");

    }
#endif

    AddOption("trie-size", fmt::format(T_("Set {0} to N."), "trie_size"),
        FIRST_OPTION_VAL + pimpl->optBase + OPT_TRIE_SIZE,
        POPT_ARG_STRING,
        "N");

    if (!AmI("omega"))
    {
        AddOption("font-max", fmt::format(T_("Set {0} to N."), "font_max"),
            FIRST_OPTION_VAL + pimpl->optBase + OPT_FONT_MAX,
            POPT_ARG_STRING,
            "N");
        AddOption("font-mem-size", fmt::format(T_("Set {0} to N."), "font_mem_size"),
            FIRST_OPTION_VAL + pimpl->optBase + OPT_FONT_MEM_SIZE,
            POPT_ARG_STRING,
            "N");
    }

#if EXPERT_SRC_SPECIALS
    AddOption("src-specials", T_("Insert source specials in certain places of the DVI file.  WHERE is a comma-separated value list of: cr display hbox math par parend vbox."),
        FIRST_OPTION_VAL + pimpl->optBase + OPT_SRC_SPECIALS,
        POPT_ARG_STRING | POPT_ARGFLAG_OPTIONAL,
        "WHERE");
#else
    AddOption("src-specials", T_("Insert source specials in certain places of the DVI file."),
        FIRST_OPTION_VAL + pimpl->optBase + OPT_SRC_SPECIALS);
#endif

    // deprecated options
    AddOption("disable-pipes", OPT_NOOP);
    AddOption("enable-pipes", OPT_NOOP);

    // obsolete options
    AddOption("try-gz\0", OPT_UNSUPPORTED);

    // old option names
    AddOption("src", "src-specials");

    // supported Web2C options
    AddOption("mltex", "enable-mltex");
    AddOption("fmt", "undump");
    AddOptionShortcut("no-shell-escape", { "--disable-write18" });
    AddOptionShortcut("shell-restricted", { "--restrict-write18" });
    AddOptionShortcut("shell-escape", { "--enable-write18" });
    if (!AmI("xetex"))
    {
        AddOption("enc", "enable-enctex");
    }

    // unsupported Web2C options
    AddOption("ipc", OPT_UNSUPPORTED);
    AddOption("ipc-start", OPT_UNSUPPORTED);
    AddOption("output-comment",
        OPT_UNSUPPORTED,
        POPT_ARG_STRING,
        "comment");
}

extern bool inParseFirstLine;

bool TeXApp::ProcessOption(int optchar, const string& optArg)
{
    bool done = true;
    switch (optchar - FIRST_OPTION_VAL - pimpl->optBase)
    {
    case OPT_DISABLE_WRITE18:
        EnableShellCommands(ShellCommandMode::Forbidden);
        break;
    case OPT_ENABLE_ENCTEX:
        pimpl->enableEncTeX = true;
        break;
    case OPT_ENABLE_MLTEX:
        pimpl->enableMLTeX = true;
        break;
    case OPT_ENABLE_WRITE18:
        if (!inParseFirstLine)
        {
            EnableShellCommands(ShellCommandMode::Unrestricted);
        }
        break;
    case OPT_FONT_MAX:
        GetUserParams()["font_max"] = std::stoi(optArg);
        break;
    case OPT_FONT_MEM_SIZE:
        GetUserParams()["font_mem_size"] = std::stoi(optArg);
        break;
    case OPT_HASH_EXTRA:
        GetUserParams()["hash_extra"] = std::stoi(optArg);
        break;
    case OPT_MAX_IN_OPEN:
        GetUserParams()["max_in_open"] = std::stoi(optArg);
        break;
    case OPT_MEM_BOT:
        GetUserParams()["mem_bot"] = std::stoi(optArg);
        break;
    case OPT_NEST_SIZE:
        GetUserParams()["nest_size"] = std::stoi(optArg);
        break;
    case OPT_RESTRICT_WRITE18:
        if (!inParseFirstLine)
        {
            EnableShellCommands(ShellCommandMode::Restricted);
        }
        break;
    case OPT_SAVE_SIZE:
        GetUserParams()["save_size"] = std::stoi(optArg);
        break;
    case OPT_SRC_SPECIALS:
#if EXPERT_SRC_SPECIALS
        pimpl->sourceSpecials[SourceSpecial::Auto] = true;
        pimpl->sourceSpecials[SourceSpecial::Paragraph] = true;
#else
        if (optArg.empty())
        {
            pimpl->sourceSpecials[(size_t)SourceSpecial::Auto] = true;
            pimpl->sourceSpecials[(size_t)SourceSpecial::Paragraph] = true;
        }
        else
        {
            Tokenizer tok(optArg, ", ");
            while (tok)
            {
                if (*tok == "everypar" || *tok == "par")
                {
                    pimpl->sourceSpecials[(size_t)SourceSpecial::Auto] = true;
                    pimpl->sourceSpecials[(size_t)SourceSpecial::Paragraph] = true;
                }
                else if (*tok == "everyparend" || *tok == "parend")
                {
                    pimpl->sourceSpecials[(size_t)SourceSpecial::ParagraphEnd] = true;
                }
                else if (*tok == "everycr" || *tok == "cr")
                {
                    pimpl->sourceSpecials[(size_t)SourceSpecial::CarriageReturn] = true;
                }
                else if (*tok == "everymath" || *tok == "math")
                {
                    pimpl->sourceSpecials[(size_t)SourceSpecial::Math] = true;
                }
                else if (*tok == "everyhbox" || *tok == "hbox")
                {
                    pimpl->sourceSpecials[(size_t)SourceSpecial::HorizontalBox] = true;
                }
                else if (*tok == "everyvbox" || *tok == "vbox")
                {
                    pimpl->sourceSpecials[(size_t)SourceSpecial::VerticalBox] = true;
                }
                else if (*tok == "everydisplay" || *tok == "display")
                {
                    pimpl->sourceSpecials[(size_t)SourceSpecial::Display] = true;
                }
                else
                {
                    MIKTEX_FATAL_ERROR_2(T_("Unknown source special."), "special", *tok);
                }
                ++tok;
            }
        }
#endif // EXPERT_SRC_SPECIALS
        break;
    case OPT_SYNCTEX:
        pimpl->synchronizationOptions = std::stoi(optArg);
        break;

    case OPT_TRIE_SIZE:
        GetUserParams()["trie_size"] = std::stoi(optArg);
        break;
    case OPT_TRIE_OP_SIZE:
        GetUserParams()["trie_op_size"] = std::stoi(optArg);
        break;
    default:
        done = TeXMFApp::ProcessOption(optchar, optArg);
        break;
    }
    return done;
}

TeXApp::Write18Result TeXApp::Write18(const string& command, int& exitCode) const
{
    shared_ptr<Session> session = GetSession();
    Session::ExamineCommandLineResult examineResult;
    string examinedCommand;
    string safeCommandLine;
    tie(examineResult, examinedCommand, safeCommandLine) = session->ExamineCommandLine(command);
    if (examineResult == Session::ExamineCommandLineResult::SyntaxError)
    {
        LogError(fmt::format("syntax error: {0}", command));
        return Write18Result::QuotationError;
    }
    if (examineResult != Session::ExamineCommandLineResult::ProbablySafe && examineResult != Session::ExamineCommandLineResult::MaybeSafe)
    {
        LogError(fmt::format("command is unsafe: {0}", command));
        return Write18Result::Disallowed;
    }
    string toBeExecuted;
    switch (GetShellCommandMode())
    {
    case ShellCommandMode::Unrestricted:
        if (session->RunningAsAdministrator() && !session->GetConfigValue(MIKTEX_CONFIG_SECTION_CORE, MIKTEX_CONFIG_VALUE_ALLOW_UNRESTRICTED_SUPER_USER).GetBool())
        {
            LogError(fmt::format("not allowed with elevated privileges: {0}", command));
            return Write18Result::Disallowed;
        }
        toBeExecuted = command;
        break;
    case ShellCommandMode::Forbidden:
        LogError(fmt::format("command not executed: {0}", command));
        return  Write18Result::Disallowed;
    case ShellCommandMode::Query:
        // TODO
    case ShellCommandMode::Restricted:
        if (examineResult != Session::ExamineCommandLineResult::ProbablySafe || command.find_first_of('|') != string::npos)
        {
            LogError(fmt::format("command not allowed: {0}", command));
            return Write18Result::Disallowed;
        }
        toBeExecuted = safeCommandLine;
        break;
    default:
        MIKTEX_UNEXPECTED();
    }
    if (examineResult == Session::ExamineCommandLineResult::ProbablySafe)
    {
        LogInfo(fmt::format("executing restricted write18 shell command: {0}", toBeExecuted));
    }
    else
    {
        LogWarn(fmt::format("executing unrestricted write18 shell command: {0}", toBeExecuted));
    }
    Process::ExecuteSystemCommand(toBeExecuted, &exitCode);
    LogInfo(fmt::format("write18 exit code: {0}", exitCode));
    return examineResult == Session::ExamineCommandLineResult::ProbablySafe ? Write18Result::ExecutedAllowed : Write18Result::Executed;
}

ShellCommandMode TeXApp::GetWrite18Mode() const
{
    return GetShellCommandMode();
}

bool TeXApp::Write18P() const
{
    return GetShellCommandMode() == ShellCommandMode::Unrestricted
        || GetShellCommandMode() == ShellCommandMode::Restricted
        || GetShellCommandMode() == ShellCommandMode::Query;
}

bool TeXApp::MLTeXP() const
{
    return pimpl->enableMLTeX;
}

int TeXApp::GetSynchronizationOptions() const
{
    return pimpl->synchronizationOptions;
}

bool TeXApp::EncTeXP() const
{
    return pimpl->enableEncTeX;
}

bool TeXApp::IsSourceSpecialOn(SourceSpecial s) const
{
    return pimpl->sourceSpecials[(std::size_t)s];
}

bool TeXApp::SourceSpecialsP() const
{
    return pimpl->sourceSpecials.any();
}

void TeXApp::SetFormatHandler(IFormatHandler* formatHandler)
{
    pimpl->formatHandler = formatHandler;
}

IFormatHandler* TeXApp::GetFormatHandler() const
{
    return pimpl->formatHandler;
}

int TeXApp::MakeSrcSpecial(int sourceFileName, int line) const
{
    IStringHandler* stringHandler = GetStringHandler();
    C4P::C4P_integer oldpoolptr = stringHandler->poolptr();
    MiKTeX::Util::PathName fileName(GetTeXString(sourceFileName));
    const std::size_t BUFSIZE = MiKTeX::Core::BufferSizes::MaxPath + 100;
    char szBuf[BUFSIZE];
#if _MSC_VER >= 1400
    sprintf_s(szBuf, BUFSIZE, "src:%d%s%s", line, isdigit(fileName[0]) ? " " : "", fileName.GetData());
#else
    sprintf(szBuf, "src:%d%s%s", line, isdigit(fileName[0]) ? " " : "", fileName.GetData());
#endif
    std::size_t len = MiKTeX::Util::StrLen(szBuf);
    CheckPoolPointer(stringHandler->poolptr(), len);
    char* lpsz = szBuf;
    while (*lpsz != 0)
    {
        stringHandler->strpool()[stringHandler->poolptr()] = *lpsz++;
        stringHandler->poolptr() += 1;
    }
    return oldpoolptr;
}

bool TeXApp::IsNewSource(int sourceFileName, int line) const
{
    return pimpl->lastSourceFilename != PathName(GetTeXString(sourceFileName)) || pimpl->lastLineNum != line;
}

void TeXApp::RememberSourceInfo(int sourceFileName, int line) const
{
    pimpl->lastSourceFilename = GetTeXString(sourceFileName);
    pimpl->lastLineNum = line;
}
