#pragma once
// WIZARDS.cpp — Developer "wizard" debug-command handlers
//
// A small developer/debug scripting facility: commands that define and query
// "wizard variables" and prompt for string input. Each handler validates the
// wizard command argument count (g_nWizParamCount) and pops up a WIZERR
// MessageBox on misuse. Almost certainly dev-build only.

extern int  g_nWizParamCount;   // 0x007d7a74  argument count for the current wizard command
extern int  g_nWizVarCount;     // 0x007d7a78  number of defined wizard variables (max 21)

// Define a new wizard variable by name (requires exactly 2 args).
// Returns 0 on success, -1 on error (already defined / table full / bad argc).
int Wiz_AddVar(const char* pszCmdName);

// Ask-string wizard command: look up a variable, read prompt + reply strings
// (requires 3 or 4 args). Returns 0 on success, -1 on error.
int Wiz_AskString(const char* pszCmdName);

// --- Interpreter internals ---------------------------------------------

// Clear the wizard variable table and reset the count. (wiz_reset_structs)
void Wiz_ResetStructs(void);

// Return 0 if the token has any non-whitespace char, -1 if blank/empty.
// (wiz_verify_string)
int  Wiz_VerifyString(const char* pszStr);

// Collapse whitespace runs to single spaces, in place. (trim_str)
void Wiz_TrimStr(char* pszStr);

// Quote-aware tokeniser. Fills g_szWizArgs / g_nWizParamCount.
// Returns 0 on success, -1 on unbalanced quotes. (wiz_split_strings)
int  Wiz_SplitStrings(char* pszStr);

// Match g_szWizArgs[0] against the command vocabulary:
//   "ADDVAR" -> 1, "ASKSTRING" -> 2, otherwise -1 (Unknown Command).
// (wiz_get_command)
int  Wiz_GetCommand(void);

// Read the next non-blank, non-comment ("//") line from a wizard file.
// Returns TRUE if a line was read into pszLine, FALSE at EOF.
// (wiz_get_next_line)
int  Wiz_GetNextLine(void* pFile, char* pszLine);

// Open <SaveGameDir>\wizards\<pszWizName> and interpret it line by line.
// (wiz_run)
void Wiz_Run(const char* pszWizName);
