// WIZARDS.cpp — Developer "wizard" debug-command handlers
//
// A developer/debug scripting facility (dev build). Wizard commands manipulate a
// small table of named "wizard variables" and can prompt for string input. Each
// handler is a script-command callback that first checks the supplied argument
// count (g_nWizParamCount) and reports misuse through a WIZERR MessageBox.
//
// Wizard variable table: up to 21 entries at g_abWizVarTable (stride 0x88):
//   [+0x00] char name[...]   variable name
//   [+0x20] int  value       current value (0 on define)
//
// Original source: C:\DevStudio\Projects\Crux\WIZARDS.cpp

#include "WIZARDS.h"
#include <windows.h>
#include <string.h>

int  g_nWizParamCount = 0;   // 0x007d7a74
int  g_nWizVarCount   = 0;   // 0x007d7a78

// Wizard variable name table (0x88-byte records, max 21) and the parsed-argument
// scratch buffers, owned by the wizard command parser elsewhere.
extern char g_abWizVarTable[]; // 0x007d6ea8  variable records, stride 0x88
extern char g_szWizArg1[];     // 0x007d7966  first parsed argument (var name)
extern char g_szWizArg2[];     // 0x007d7984  second parsed argument (prompt string)
extern char g_szWizArg3[];     // 0x007d79a2  third parsed argument (reply string)
extern char g_szWizDefault[];  // 0x007d7a80  default reply when only 3 args

// READRES/ERRORS helpers
extern int  Wiz_FindVar(const char* pszName);  // thunk_FUN_00487cb0 — lookup var index, -1 if none
extern void Str_Copy(void* pDst, const void* pSrc);  // FUN_004895e0
extern void Debug_Trace(int nLine, const char* pszFile, const char* pszFmt, ...);

#define WIZ_MAX_VARS 21

// ============================================================
//  Wiz_AddVar  (0x00487db0)
//  Define a new wizard variable (requires exactly 2 command args).
// ============================================================
int Wiz_AddVar(const char* pszCmdName)
{
    if (g_nWizParamCount != 2)
    {
        MessageBoxA(NULL, "WIZERR: Incorrect number of params", pszCmdName, 0x10030);
        return -1;
    }

    if (Wiz_FindVar(g_szWizArg1) != -1)
    {
        MessageBoxA(NULL, "WIZERR: Var already defined", pszCmdName, 0x10030);
        return -1;
    }

    g_nWizVarCount++;
    if (g_nWizVarCount >= WIZ_MAX_VARS)
    {
        MessageBoxA(NULL, "WIZERR: Max Wizard Vars", pszCmdName, 0x10030);
        return -1;
    }

    // record: name at +0x00, value (0) at +0x20, stride 0x88
    Str_Copy(&g_abWizVarTable[g_nWizVarCount * 0x88], g_szWizArg1);
    *(int*)(&g_abWizVarTable[g_nWizVarCount * 0x88 + 0x20]) = 0;
    Debug_Trace(0, "C:\\DevStudio\\Projects\\Crux\\WIZARDS.cpp",
                "Added variable %s", g_szWizArg1);
    return 0;
}

// ============================================================
//  Wiz_AskString  (0x00487f50)
//  Wizard "ask string" command: look up a variable, capture the prompt
//  string and the reply string (requires 3 or 4 args). With 4 args the
//  reply comes from arg3; with 3 args a default reply is used.
//
//  Original debug name: wiz_ask_string(char*, instr)
// ============================================================
int Wiz_AskString(const char* pszCmdName)
{
    char szPrompt[100];
    char szReply[32];

    if (g_nWizParamCount < 3 || g_nWizParamCount > 4)
    {
        MessageBoxA(NULL, "WIZERR: Incorrect number of params", pszCmdName, 0x10030);
        return -1;
    }

    int nVar = Wiz_FindVar(g_szWizArg1);
    if (nVar == -1)
    {
        MessageBoxA(NULL, "WIZERR: No such variable", g_szWizArg1, 0x10030);
        return -1;
    }

    Str_Copy(szPrompt, g_szWizArg2);
    Debug_Trace(0, "C:\\DevStudio\\Projects\\Crux\\WIZARDS.cpp",
                "Found askstring: %s", szPrompt);

    if (g_nWizParamCount == 4)
        Str_Copy(szReply, g_szWizArg3);
    else
        Str_Copy(szReply, g_szWizDefault);

    Debug_Trace(0, "C:\\DevStudio\\Projects\\Crux\\WIZARDS.cpp",
                "Found repstring: %s", szReply);
    return 0;
}

// ------------------------------------------------------------
//  Interpreter internals
//
//  A "wizard" is a small script file under  <SaveGameDir>\wizards\.
//  Each non-blank, non-comment line is tokenised into argument tokens
//  (g_szWizArgs, stride 0x1e), the first token is a command keyword, and
//  the keyword selects a handler (Wiz_AddVar / Wiz_AskString).
// ------------------------------------------------------------

extern char g_szWizArgs[];          // 0x007d7948  arg tokens, stride 0x1e; [0] = command keyword
extern char g_abSaveGameDir[];      // save-game directory root (g_abWizVarTable declared above)

extern void* Str_Copy(void* pDst, const void* pSrc);              // FUN_004895e0  (strcpy-like)
extern void* Str_Cat(void* pDst, const void* pSrc);               // FUN_004895f0  (strcat-like)
extern void* File_Open(const char* pszPath, const char* pszMode); // FUN_0048a340  (fopen-like)
extern char* File_GetS(char* pszBuf, int nMax, void* pFile);      // FUN_0048a790  (fgets-like)
extern int   Char_IsSpace(int ch);                                // FUN_0048b6d0  (isspace-like)
extern int   Str_Compare(const void* a, const void* b);           // FUN_0049a830  (used by Wiz_FindVar)

#define WIZ_ARG_STRIDE 0x1e   // bytes per parsed argument token

// ============================================================
//  Wiz_ResetStructs  (0x00488300)
//  Clear the wizard variable table and the defined-variable count.
//  Original: void wiz_reset_structs(void)
// ============================================================
void Wiz_ResetStructs(void)
{
    memset(g_abWizVarTable, 0, 0xaa0);   // 21 * 0x88 records
    g_nWizVarCount = 0;
}

// ============================================================
//  Wiz_VerifyString  (0x00488710)
//  Return 0 if the token contains at least one non-whitespace char,
//  -1 if it is empty or entirely whitespace.
//  Original: int wiz_verify_string(char *instr)
// ============================================================
int Wiz_VerifyString(const char* pszStr)
{
    for (int i = 0; pszStr[i] != '\0'; i++)
    {
        if (Char_IsSpace((unsigned char)pszStr[i]) == 0)
            return 0;        // found real content
    }
    return -1;               // blank / empty
}

// ============================================================
//  Wiz_TrimStr  (0x00488a10)
//  Collapse runs of whitespace down to single spaces and drop leading
//  whitespace, writing the result back into the input buffer.
//  Original: void trim_str(char *instr)  (file-local helper in WIZARDS.cpp)
// ============================================================
void Wiz_TrimStr(char* pszStr)
{
    char szOut[100];
    int  nState = 0;     // 0 = start, 1 = pending space, 2 = wrote char
    int  nOut   = 0;

    memset(szOut, 0, sizeof(szOut));

    for (unsigned i = 0; i < strlen(pszStr); i++)
    {
        if (Char_IsSpace((unsigned char)pszStr[i]) == 0)
        {
            if (nState == 1)          // emit one space between words
                szOut[nOut++] = ' ';
            szOut[nOut++] = pszStr[i];
            nState = 2;
        }
        else
        {
            nState = 1;               // remember pending whitespace
        }
    }

    Str_Copy(pszStr, szOut);
}

// ============================================================
//  Wiz_SplitStrings  (0x004883b0)
//  Quote-aware tokeniser. Splits teststr into argument tokens stored in
//  g_szWizArgs (stride 0x1e) and sets g_nWizParamCount. Quoted spans are
//  kept verbatim (and always stored); bare tokens are stored only if
//  Wiz_VerifyString accepts them. An odd number of quotes is an error.
//  Original: int wiz_split_strings(char *teststr)
//  Returns 0 on success, -1 on unbalanced quotes.
// ============================================================
int Wiz_SplitStrings(char* pszStr)
{
    int nQuotes = 0;

    g_nWizParamCount = 0;
    for (int i = 0; pszStr[i] != '\0'; i++)
    {
        if (pszStr[i] == '\"')
            nQuotes++;
    }

    if ((nQuotes & 1) == 1)
    {
        MessageBoxA(NULL, "WIZERR: Odd number of quotation marks", pszStr, 0x10030);
        return -1;
    }

    Str_Cat(pszStr, " ");        // sentinel trailing space

    int i = 0;
    while (pszStr[i] != '\0')
    {
        char szToken[100];
        Str_Copy(szToken, "");

        // skip leading whitespace
        while (pszStr[i] != '\0' && Char_IsSpace((unsigned char)pszStr[i]) != 0)
            i++;

        bool bQuoted = (pszStr[i] == '\"');
        if (bQuoted)
            i++;
        int nStart = i;

        // scan to the matching terminator (closing quote, or whitespace)
        for (; pszStr[i] != '\0'; i++)
        {
            int bEnd = bQuoted ? (pszStr[i] == '\"')
                               : Char_IsSpace((unsigned char)pszStr[i]);
            if (bEnd != 0)
            {
                strncpy(szToken, pszStr + nStart, i - nStart);
                szToken[i - nStart] = '\0';
                break;
            }
        }
        i++;

        // store quoted tokens always; bare tokens only if non-blank
        if ((!bQuoted && Wiz_VerifyString(szToken) == 0) || bQuoted)
        {
            Str_Copy(&g_szWizArgs[g_nWizParamCount * WIZ_ARG_STRIDE], szToken);
            g_nWizParamCount++;
        }
    }
    return 0;
}

// ============================================================
//  Wiz_GetCommand  (0x004887e0)
//  Match the leading argument token (g_szWizArgs[0]) against the wizard
//  command keyword vocabulary and return a handler selector.
//  Original: int wiz_get_command()
//
//  COMMAND VOCABULARY (statically-recoverable, complete):
//      "ADDVAR"     -> 1   (Wiz_AddVar)
//      "ASKSTRING"  -> 2   (Wiz_AskString)
//  anything else    -> -1  ("WIZERR: Unknown Command")
// ============================================================
int Wiz_GetCommand(void)
{
    if (strcmp(g_szWizArgs, "ADDVAR") == 0)
        return 1;

    if (strcmp(g_szWizArgs, "ASKSTRING") == 0)
        return 2;

    MessageBoxA(NULL, "WIZERR: Unknown Command", g_szWizArgs, 0x10030);
    return -1;
}

// ============================================================
//  Wiz_GetNextLine  (0x004888d0)
//  Read the next "meaningful" line from the wizard file: fgets, trim it,
//  and skip blank lines and comments (lines beginning with "//"). Returns
//  TRUE if a usable line was read into cur_line, FALSE at EOF.
//  Original: BOOL wiz_get_next_line(FILE *inf, char *cur_line)
// ============================================================
int Wiz_GetNextLine(void* pFile, char* pszLine)
{
    char szBuf[100];
    int  bUsable = 0;

    do
    {
        if (bUsable != 0)
        {
            Str_Copy(pszLine, szBuf);
            return 1;
        }

        bUsable = 1;
        File_GetS(szBuf, 100, pFile);
        if ((((unsigned*)pFile)[3] & 0x10) != 0)   // FILE _flag & _IOEOF
            return 0;

        Wiz_TrimStr(szBuf);
        if (strlen(szBuf) == 0)
            bUsable = 0;
        if (strncmp(szBuf, "//", 2) == 0)          // comment line
            bUsable = 0;
    } while (true);
}

// ============================================================
//  Wiz_Run  (0x00488110)
//  Open <SaveGameDir>\wizards\<wizname> and interpret it line by line:
//  reset state, then for each line split into args, identify the command,
//  and dispatch to Wiz_AddVar / Wiz_AskString. Aborts on any handler -1.
//  Original: void wiz_run(char *wizname)
// ============================================================
void Wiz_Run(const char* pszWizName)
{
    char szPath[100];
    char szLine[100];

    Str_Copy(szPath, g_abSaveGameDir);
    Str_Cat(szPath, "wizards\\");
    Str_Cat(szPath, pszWizName);

    void* pFile = File_Open(szPath, "r");
    if (pFile == NULL)
    {
        MessageBoxA(NULL, "Unable to open wizard", "ERROR", 0x10030);
        return;
    }

    Wiz_ResetStructs();

    while (Wiz_GetNextLine(pFile, szLine) != 0)
    {
        if (Wiz_SplitStrings(szLine) == -1)
            return;
        if (g_nWizParamCount == 0)
            continue;

        int nCmd = Wiz_GetCommand();
        if (nCmd == -1)
            return;

        if (nCmd == 1)
        {
            if (Wiz_AddVar(szLine) == -1)
                return;
        }
        else if (nCmd == 2)
        {
            if (Wiz_AskString(szLine) == -1)
                return;
        }
    }
}
