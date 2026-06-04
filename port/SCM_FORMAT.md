# SCM cutscene/video stream format (type-16 resources)

Reverse-engineered from `Player_ScmPlayList` (0x00458bb0) + `Player_RenderFrame` (0x0045b260)
and **validated byte-exactly** against INTRO1/INTRO2 in ADVENT.RES.

The intros (INTRO1, INTRO2, INTRO3, …) are type-16 resources. A type-18 companion of the
same name holds a 4-byte frame-target (often 0). The intro **playlist** is the `INTRO` type-4
resource, which lists `INTRO1 INTRO2 INTRO3 …` then `menu`.

## Container layout

```
SCM stream:
  [16-byte stream header]
     +0  u16 magic     = 0x0010
     +2  u16 version   = 1
     +4  u16 frameCount         (INTRO1=135, INTRO2=90, INTRO3=567)
     +6  u16 rate/flags = 14    (intro fps-ish; cf. ADVENT.INI [Flow] FPS=9)
     +8  8 bytes reserved (zero)
  then repeat per FRAME:
     u16 chunkCount               ; 0 terminates the stream
     chunkCount × chunk:
        [8-byte chunk header]
           +0  u32 size           ; payload byte count (excludes header)
           +4  u16 type
           +6  u16 param
        size bytes of payload
```

Validated: INTRO1 = 135 frames / 142 chunks, every chunk type valid, chunks tile the entire
4,548,915-byte blob exactly (final chunkCount=0 lands on EOF).

The engine seeks past the 16-byte header (`fseek 0x10`) then streams frames into a 5000-entry
ring of chunk records; `Player_RenderFrame` consumes one frame's chunks per displayed frame.

## Chunk types (the `type` field)

| type   | meaning            | payload |
|--------|--------------------|---------|
| 0x0010 | VIDEO frame        | Img-format RLE sprite (see below); blitted full-screen at (0,0) via GI_LockActiveSurf |
| 0x0002 | PALETTE update     | `[u8 startIdx][u8 pad][ (size-2) bytes of 6-bit RGB ]` → palette[startIdx..] |
| 0x0100 | SPEECH/lip control | param selects a sub-command (LIPS_ON/OFF, wait-frames, music-loop, voice flags) |
| 0x0040–0x0043 | MUSIC bunch | streamed into the music double-buffer (param hi byte = loop id) |
| 0x0080–0x0083 | AUDIO bunch | streamed to a mixer voice channel |
| 0x0400–0x0402 | LIP / SCI   | lip-sync phoneme data / SCI table |
| 0x1000 | TEXT/subtitle      | passed to Txt_SetString (param = string id) |

For rendering the intros we only need **0x0010 (video)** and **0x0002 (palette)**; audio/lip/
text chunks can be skipped initially.

## Typical frame structure (INTRO1)

```
frame 0 : 4 chunks  [0x100 speech(2B)] [0x82 audio(601586B)] [0x10 video(114500B)] [0x02 pal(770B)]
frame 1 : 1 chunk   [0x10 video(44665B)]      ← delta frames
frame 2 : 1 chunk   [0x10 video(44188B)]
...
```
So frame 0 is a keyframe (sets palette + first full image + starts audio); subsequent frames are
single video chunks (RLE sprites that overwrite/patch the framebuffer).

## VIDEO chunk = Img RLE sprite

The 0x10 payload is the engine's standard paletted RLE sprite (same format `GI_LockActiveSurf`/
Img.cpp blit). Sprite header (from CSDEF/type-6 samples — `10 01 00 80 02 e0 01 08 …`):
```
  +0  u16 flags      (0x0110)
  +2  u8  ?
  +3  u16 width      (0x0280 = 640)
  +5  u16 height     (0x01e0 = 480)
  +7  u8  ?          (0x08)
  ...then per-row RLE: opcode byte selects a line codec (0=literal run, 1=RLE,
     2=skip-RLE, 3=offset-RLE) — see src/Img.cpp PutLine_* for the exact opcodes.
```
(Full pixel-codec spec to be finalised while implementing port/Sprite.cpp from the reversed
Img.cpp PutLine_Direct / PutLine_RLE / PutLine_RLE_Skip / PutLine_RLE_Offset.)

## Palette
6-bit per channel (0..63). Expand to 8-bit via `(v<<2)|(v>>4)`. The keyframe palette chunk sets
the active palette; the video frame indices reference it.

## Porting note
The original streams asynchronously off CD with a worker thread, double-buffering, and rate
pacing. For the SDL port we read synchronously: parse the whole blob, then for each frame apply
its palette chunks and decode its video chunk into the 640×480 framebuffer, presenting at the
stream rate.
