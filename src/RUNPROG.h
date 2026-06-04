#pragma once
// RUNPROG.cpp — Script bytecode virtual machine
//
// RunProg_Exec is the game's script interpreter: a ~400-opcode switch that
// drives every verb interaction, dialogue, cutscene, room transition, and
// puzzle. See RUNPROG_OPCODES.md for the full opcode specification.

// --- Globals ---
extern void* g_pScriptPrograms;   // 0x007114c8  program-ptr array, indexed by program ID
extern int   g_nScriptNextOp;     // 0x0070a1f0  interpreter continuation value

// Script variable / register file (1500 ints). Despite the name it is the
// general-purpose script variable store, persisted across saves.
extern int   g_anSpeechPlayed[];  // 0x004da... (see SPEECH.cpp)

// --- Dispatcher ---
// param_1 = script program ID (index into g_pScriptPrograms)
// id      = invocation context / entity id
void RunProg_Exec(unsigned int nProgId, int nId);

// --- Helpers ---
void RunProg_WaitMoveDone(int nProgId, int bInterruptible, int* pbInterrupted);
void RunProg_SelectAreaContext(void);
void RunProg_PlayScmWithPaletteGuard(int nName, int nFlags, int a, int b);
void RunProg_ClearTrackedSounds(void);
void RunProg_StopAndClearTrackedSounds(void);
void RunProg_TrackSound(int nChannelId);
void RunProg_RestorePaletteSnapshot(void);
void RunProg_Nop(void);
