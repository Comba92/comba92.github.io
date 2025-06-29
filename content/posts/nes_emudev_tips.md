+++
date = '2025-05-26T11:41:09+02:00'
draft = false
description = 'NesEmuDevTips'
linkTitle = 'Nes emulator development guide and tips'
title = 'Nes emulator development guide with tips focused on fast optimizations'
summary = 'A detailed guide focused on implementing an higly optmizied Nes emulator, written by Brad Taylor.'
author = 'Brad Taylor'
tags = ['nes', 'emulation']
keywords = ['nes', 'emulator', 'emulation', 'coding', 'programming', 'optimization', 'development', 'devlog', 'resource', 'banking', 'implementation', 'brad taylor', 'guide', 'tips']
+++

**This article is just a better formatted version of the original [paper by Brad Taylor](https://www.nesdev.org/NES%20emulator%20development%20guide.txt).
All credits due to him.**

----

Brad Taylor (BTTDgroup@hotmail.com) <br>
4th release: April 23rd, 2004 <br>
Thanks to the NES community. http://nesdev.parodius.com. <br>
recommended literature: 2A03/2C02/FDS technical reference documents <br>

## Overview of document
- a guide for programmers writing their own NES/FC emulator software
- provides many code optimization tips (with focus placed on the x86-based personal computing platform)
- provides lists of features to implement in an emulator intended for public-domain release
- created in an effort to improve the quality of the user's NES gaming experience

## Topics discussed
- [General PPU emulation](#general-ppu-emulation) <br>
- [Pixel rendering techniques](#pixel-rendering-techniques) <br>
- [Merging playfield & object pixels](#merging-playfield--object-pixels) <br>
- [Frame store optimizations](#frame-store-optimizations) <br>
- [Smooth audio reproduction](#frame-store-optimizations) <br>
- [6502 instruction decoding & execution techniques](#6502-instruction-decoding--execution-techniques) <br>
- [Emulation address decoding](#emulation-address-decoding) <br>
- [Hardware port queueing](#hardware-port-queueing) <br>
- [Threading NES applications](#threading-nes-applications) <br>
- [Emulator features to support](#emulator-features-to-support) <br>
- [New object-oriented NES file format specification](#new-object-oriented-nes-file-format-specification) <br>


----------------
## General PPU emulation
----------------
Most likely, the key to your emulator's performance will be based on the
speed at which it can render NES graphics. It's pretty easy to write a slow
PPU render engine, since overall there's a good deal of work that has to be
done. Accurate emulation of the PPU is difficult, due to all the trickery
various NES games use to achieve special video effects (like split screen
scrolling), otherwise not possible by "clean" or conventional means. In
reality, all these "tricks" are simply accomplished by writing to the
appropriate PPU (or related) registers at the right time during the
rendering of a frame (picture).

On a hardware level, the CPU & PPU in the NES run simultaniously. This is
why a game can be coded to make a write out to a PPU register at a certain
time during a frame, and the result of this is that the (on-screen) effect
occurs in a specific location on the screen. Thus, the first instinct one
has for writing a NES emulator is to execute both the CPU & PPU engines
alternately on every (NES) clock cycle. The results of this will give very
accurate emulation, BUT- doing this will also be VERY processor intense
(this will mostly be due to all the overhead of transfering program control
to so many hardware emulation routines in such little time (1 CPU clock
cycle)). As a result, emulators coded like this turn out to be the slowest
ones.


### PPU info
--------
NES graphics consist of a single scrollable playfield, and 64
objects/sprites. The screen resolution is 256x240 pixels, and while games
can control the graphics on a per-pixel basis, it is usually avoided since
it's pretty difficult. Instead, the PPU makes displaying graphics easier for
the programmer by dividing the screen up into tiles, which index an 8x8
pixel bitmap to appear in that particular spot. Each object defines 1 or 2
tiles to be displayed on a randomly-accessable xy coordinate on the screen.
There are also 8 palette tables in the PPU that bitmap data can refer to
(playfield & object bitmap data each have 4 palettes). Each palette has 3
indexable colors, as tile bitmaps only consist of 2 bits per pixel (the 00
combination is considered transparency). A single transparency color palette
register is also defined, and is only used as the on-screen color when
overlapping pixels (due to objects being placed on the playfield) of all
playfield/object pixels are defined as transparent.

As graphics are rendered (as described in the "2C02 technical reference"
document), the name tables are accessed sequentially to reference a tile's
bitmap, which gets used as the pixel data for the area of the screen that
the name table index entry corresponds to (offset by the scroll register
values). Attribute tables, which are layed out the same way that name tables
are (except with lower resolution- 1 attribute entry represents a 2x2
cluster of on-screen tiles), determine the palette select value for the
group of tiles to use (1 of 4).

Objects attribute memory (sprite RAM, or "OAM" which contain private tile
index and palette select information) is evaluated every single scanline
(y-coordinate entries are examined), and in-range objects have thier tile
bitmaps loaded into the PPU inbetween scanlines. The contents are then
merged with the playfield's pixel data in real-time.


### Accurate & efficient PPU emulation
----------------------------------
For the most part, PPU operations are linear and sequential, making them
easy to design algorithms for. By rendering playfields and objects on a
per-tile basis, emulation can be made quick & easy. However, games that use
special effects (mid-frame PPU trickery) require special handling, which in
turn complicates algorithm design.

By implementing a clock cycle counter in the CPU core, it is possible for
emulated PPU hardware to know exactly when a read/write is occuring to a
PPU-related register (or otherwise, a register which is going to change the
rendering of graphics from that point on). Therefore, when a write to a PPU
register occurs, the PPU engine can then determine if the write is going to
change the way the picture is going to be rendered, and at the exact clock
cycle (which really translates into a screen position).

For example, say the CPU engine is executing instructions. Then, on clock
cycle 13000 (relative to the last VINT), a write to the PPU's scroll
registers are made (which causes a split-screen effect). Now, first the PPU
translates 13000 CC's into X/Y coordinates (in this case, on-screen scanline
93, roughly pixel #126 (the equations to do these calculations will be
revealed later)). Ideally*, all pixels before this point will now be
rendered to a buffer, using the data in the PPU registers prior to the
write. Now the screen area before the write occured has been rendered
accurately, and the screen will progressively continue to be updated in this
fashion as more mid-frame writes occur. If no more occur, when the CPU
arrives at the ## of clock cycles per frame, the rest of the image (if any)
can be rendered.

* As will be discussed in the following "Frame store optimizations" and "Hardware port queueing" sections, maintaining a "stack", or more specifically, a queue of mid-frame PPU changes (which effect how successive rendering in the frame occurs), and only executing a PPU render routine once per frame (which then processes the stack of mid-frame writes) is a more efficient way of dividing up emulation tasks in your emulator.

### Knowing when to update the screen
---------------------------------
The following list describes PPU status registers/bits that if a game
modified/changed mid-frame, would change the way the rest of the frame is
rendered.

O = update objects, P = update playfield.

O object enable bit <br>
O left column objects clipping <br>
O 8/16 scanline objects <br>
O active object pattern table <br>
O pattern table bankswitch (which effects active object pattern table) <br>

PO color emphasis bits <br>
PO black & white/color select <br>

P playfield enable bit <br>
P left column playfield clipping <br>
P scroll registers <br>
P X/Y name table selection <br>
P name table bankswitch (hypothetical) <br>
P active playfield pattern table <br>
P pattern table bankswitch (which effects active playfield pattern table) <br>

Note that any PPU mapped memory (which means name, pattern, attribute &
palette tables) can only be changed while objects & the playfield are
disabled (unless cartridge hardware provides a way to do this through the
CPU memory map). Since the screen is blanked to black during this time
(regardless of the current transparency color the palette is programmed
with), these writes do not effect how the screen is rendered, and
subsequently, updating the screen can be postponed.


### Collision flag
--------------
Games without hardware for scanline counting often poll this bit to find out
when to make a write out to a PPU register which will result in a split
screen, or a pattern table swap/bankswitch. The collision flag is set when
the first non-transparent pixel of object 0 collides with a playfield pixel
that is also non-xparent. Since the screen position of the first colliding
pixel can be determined at any time (and therefore, exact CPU clock cycle at
which the collision is expected to occur), when a game requests the status
of this flag for the first time, a routine part of the PPU engine can
calculate at which clock cycle this flag will be set (calculations will be
shown later). Subsequent requests for the collision flag's status after this
would then only require the engine to compare the current CPU clock cycle,
to the calculated collision clock cycle. Whenever a mid-frame change occurs
(whether it effects the playfield, or objects), the clock cycle at which the
collision flag will go off will have to be recalculated (unless it has
already gone off).


### MMC3 IRQ timer
--------------
The MMC3's IRQ timer relies on the toggling of the PPU's A13 line 42 times a
scanline. Basically, it's counting operation is more or less at a constant
rate (meaning predictable). However, when the PPU bus is disabled (via
disabling the playfield & objects, or during the V-blank period), the
counter must quit counting. Manual toggling of PPU address bits during this
time will have to be intercepted, and the IRQ timer advanced appropriately.


### CPUCC to X/Y coordinate equations
---------------------------------
The PPU renders 3 pixels in one CPU clock. Therefore, by multiplying the CPU
CC figure by 3, we get the total amount of pixels that have been rendered
(including non-displayed ones) since the VINT.

341 pixels are rendered per scanline (although only 256 are displayed).
Therefore, by dividing PPUCC by this, we get the ## of completely rendered
scanlines since the VINT.

21 blank scanlines are rendered before the first visible one is displayed.
So, to get a scanline offset into the actual on-screen image, we simply
subtract the amount of non-displayed scanlines. Note that if this yeilds a
negative number, the PPU is still in the V-blank period.

PPUCC = CPUCC x 3
Scanline = PPUCC div 341 - 21; X- coordinate
PixelOfs = PPUCC mod 341; Y- coordinate
CPUcollisionCC = ((Y+21)x341+X)/3

Note that if the PixelOfs equation yeilds a number higher than 255, the PPU
is in the H-blank period.


### Note on emulating Tengen's Ms.Pac Man game
------------------------------------------
For emulators with poor 6502 cycle count provisions, there is a small
problem that may arise when trying to run this game. During initialization,
this game will loop, waiting for $2002's vbl flag to set. When a NMI occurs,
the NMI routine reads $2002 and throws away the value. Even though the NMI
routine saves the A register from the main loop (where $2002 was loaded),
the only way the PC will exit this loop is if $2002 returns the vbl flag set
*just* before the NMI is executed. since the NMI is invoked pending the
completion of the current instruction, and the vbl flag *IS* the NMI flag,
the VBL flag must get set in the middle of the LDA instruction. Since there
are 2 instructions in the main loop, there's about a 50% chance of the read
value from $2002 being pushed on the stack with the vbl bit set. A
work-around for emulators that can't handle this mid-instruction taboo, is
to set the vbl bit slightly before the NMI routine is invoked.


### Other notes
-----------
- some games rely on the proper implementation of collision, and dropping object flags in register $2002. this is usually done to implement up to 3 independent horizontally-tiled scrollable playfields. Make sure these flags are set at the right time, and stay set until scanline 20 of the next frame (relative to /NMI).
- (courtesy of Xodnizel): "When I messed around with emulating MMC3 games in this manner (described above), I got the best results by reset ing the count-to-42 counter to 0 on writes to $c001. Or in other words I reset the "count to zero" counter to 42."

--------------------------
## Pixel rendering techniques
--------------------------
3 rendering techniques are described in this section. They are all real-time
techniques. An unreleased version of this document discussed a tile
cache-based rendering solution. However, tile caching quickly loses it's
effectiveness for those games that use mid-frame (or even mid-scanline)
trickery to change character sets, or even palette values. Additionally,
with powerful seventh-generation Pentium processor-based PC's being the
slowest computers out there these days, there's really no need to use bitmap
caching algorithms to emulate NES graphics anymore, as was neccessary in the
days of 486-based PCs in order to achieve full NES framerate emulation.


### Basic
-----
This method, which is the most straightforward, is to store the PPU's
52-color matrix as constant data in the VGA palette registers (or otherwise,
other palette registers used for an 8-bit per pixel graphics mode). Before a
pixel can be drawn, pixel color is calculated (via pattern table & palette
select data). The PPU palette registers are looked up in some way or
another, and the contents of the palette register element is written to a
virtual frame buffer as the pixel data. This technique is the easiest to
implement, and provides the most accurate PPU emulation. However, since
every pixel drawn requires an independent palette look-up, this method is
naturally very slow.

One way to speed up this rendering style is to create a palette table
designed for looking up 2 or more pixels simultaniously. The advantages are
clear: you could easily shave alot of time (close to half with a 2
simultanious color lookup) off playfield rendering. The disadvantages are
that the lookup table grows from 2^2x1=4 bytes for a single pixel lookup, to
2^4x2=32 bytes for a 2-pixel lookup, to 2^8x4=1024 bytes for a 4-pixel
lookup. Each of the palette's 4 colors is also mirrored across these tables,
and this has to be maintained. Since I've never tried this optimization
technique, I can't tell you how effective it is (or when it stops being
effective).

Another way to increase the speed of this approach is to change the bit
ordering of the pattern tables stored in memory to favor this rendering
algorithm. For example, store the bitmap for any scanline of a tile in an 8-
2-bit packed pixel format, instead of the 2- 8-bit planar method used by
default. By doing this, it will allow the tile rendering routine to easily
extract the 2-bit number for indexing the 4 color palette associated with
the particular tile. Of course, by changing the pattern tables, whenever
pattern table memory is read or written to, the format of the data will have
to be converted. Since this happens much less often (even in games that use
CHR-RAM), it's a good idea.


### VGA palette register indexed
----------------------------
This method involves programming the VGA palette registers to reflect the
direct values the PPU palette registers contain. The VGA palette would be
divided into 64- 4 color palettes. When sequential horizontal pixels are to
be drawn, a large (32-bit or more) pattern table data fetch can occur
(pixels for pattern table tiles should be organized in memory so that the 2
bits for each horizontally sequential pixel are stored in 8-bit increments).
This chunk of fetched pixel data can then be masked (so that other pixel
data from the chunk is not used), an indexed "VGA palette select" value can
be added to the value, and finally can then be written out to the virtual
frame buffer in one single store operation. The "VGA palette select" value
is fetched via the VGA palette select table, which corresponds to the 8
classic PPU palettes (4x2 elements in the table; therefore, a tile's
attribute data (either PF or OBJ) is used as the index into this table).
This table indicates which 4-color group of 64 groups in the VGA palette to
use for color selection for the group of pixels being written. The idea is
that when a mid-frame palette change occurs (or at any time, for that
matter), the affected PPU palette in this table is changed to point to where
the new palette modifications will be made in the VGA's palette. The
corresponding VGA palette entries will also have to be updated appropriately
(generally, VGA palette updates will be made in a ring-buffer fashion. A
pointer which keeps track of the first available 4 palette entries will be
incremented when any entries in a 4-color PPU palette are changed).

Basically, this method offers the fastest possible way to render NES
graphics, since data is fetched from pattern table memory and written
directly to the virtual frame buffer. The number of pixels processed
simultaniously can be as high as 8 (with MMX instructions). However, the #
of mid-screen PPU palette modifications possible is limited to 64 times (or
32 for PF and 32 for OBJs, if one of the bits in every pixel needs to be
used to distinguish a playfield pixel from an object), but multiple
consecutive modifications to a single 4-color PPU palette only count as one
actual modification.


### MMX instruction-based rendering
-------------------------------
In 1995, the x86 architecture became blessed with MMX instructions: a set of
single function, multiple data, RISC-like instructions, that provide
solutions for solving a large amount of modern-day logic puzzles. Nearly all
the instructions have a very low 1 or 2 clock cycle latency across all x86
class CPUs which support them, hence these instructions are very desirable
to use. The instructions work around an 8 element (64-bits/element) flat
register file, which overlaps the legacy x87's mantissa registers. The BIG
deal about use of MMX instructions for pixel rendering is that 8 pixels can
be operated on simultaniously, providing each pixel is no larger than a
byte. The following assembly-written routine can fully calculate pixel color
for 8 horizontally sequential pixels for every loop iteration (the example
actually renders the first 4 scanlines of a tile).

Note: Pentium 4 and Athlon XP/64 processors support 128-bit versions of MMX
instructions, so this could allow you to increase performance quite a bit
more than what is already offered by the algorithm documented below. Very
useful for virtual NES multitasking, when 20 or more NES screens need to be
animated & displayed simultaniously in a high-resolution screen mode.

The pattern tables have already been reorganized so that the bitmap data for
4 scanlines of tile data can be loaded into an MMX register, and used in the
most efficient way possible. Pixel data for 4 sequential scanlines under the
same horizontal coordinate is stored in a single byte, with the 2 MSBs
containing the lowest logical scanline coordinate. Sequential bytes, up to
the 8th one, contain the pixel data for every successive horizontal
position. Finally, the first 8 bytes of a tile's pattern table data contain
the full bitmap data for the first 4 scanlines of the tile, and the next 8
bytes contain the last 4 scanlines.


####################################

;register assignments <br>
;-------------------- <br>
;EAX: destination pixel pointer <br>
;EBX: points to the palette to be used for this tile (essentially determined <br>
by the attribute table lookup) <br>
;ESI: source pointer for 32-pixel bitmap to be loaded from pattern table <br>
;MM4: (8 - fine horizontal scroll value)*8 <br>
;MM5: ( fine horizontal scroll value)*8 <br>
 <br>
 <br>
;fetch 32 pixels from pattern table, organized as 8 horizontal x 4 vertical. <br>
movq mm3,[esi] <br>
mov ecx,-4; load negative loop count <br>
 <br>
;move constants related to color calculation directly into registers. These <br>
have to be stored in memory since MMX instructions don't allow the use of <br>
immediate data as an operand. <br>
@1: movq mm0,_C0x8; contains C0C0C0C0C0C0C0C0h <br>
movq mm1,_00x8; contains 0000000000000000h <br>
movq mm2,_40x8; contains 4040404040404040h <br>
 <br>
;generate masks depending on the magnitude of the 2 MSBs in each packed byte <br>
(note that this is a signed comparison). <br>
pcmpgtb mm0,mm3 <br>
pcmpgtb mm1,mm3 <br>
pcmpgtb mm2,mm3 <br>
psllq mm3,2; shift bitmap to access next scanline of <br>
pixels <br>
 <br>
;to perform color lookup, a precalculated palette table is used & ANDed with <br>
the resulting masks of the last operation. Since XOR operations are used to <br>
combine the results, this requires the elements in the palette table to be <br>
XORed with adjacent values, so that they'll be cancelled out at the end of <br>
the logic processing here. The required precalculated XOR combination of <br>
each color element is shown in the comments below by the corresponding <br>
element. Note that each lookup is 8 bytes wide; this requires the same <br>
palette data for a single element to be mirrored across all 8 sequential <br>
bytes. <br>
pand mm0,[ebx+00]; 2^3 <br>
pand mm1,[ebx+08]; 3^0 <br>
pand mm2,[ebx+16]; 0^1 <br>
pxor mm0,[ebx+24]; 1 <br>
pxor mm1,mm2 <br>
pxor mm0,mm1 <br>
 <br>
;this logic performs shift functionality, in order to implement fine <br>
horizontal scrolling. The alternative to this is simply writing 64 bits out <br>
to unaligned addresses for fine H scroll values other than zero, but since <br>
this can incur large penalties on modern processors, this is generally the <br>
preferred way to generate the fine horizontal scroll effect. <br>
movq mm1,mm0 <br>
psllq mm0,mm4 <br>
psrlq mm1,mm5 <br>
por mm0,[eax] <br>
movq [eax+8],mm1 <br>
movq [eax ],mm0 <br>
 <br>
;loop maintenence <br>
add eax,LineLen; advance pixel pointer to next scanline <br>
position <br>
inc ecx <br>
jnz @1 <br>

###################################

To use the renderer, point EAX to the beginning of your render buffer (due
to how the fine horizontal scrolling works, tiles must be rendered next to
each other, incrementing along the horizontal tile axis). Without some ugly
extra logic, the render buffer will have to be increased in size by 8 pixels
per scanline, to accomodate for the extra tile pattern fetch required
whenever the fine horizontal scroll value is not equal to zero. Once the
routine has been executed enough times to fill your render buffer, consider
the starting horizontal coordinates of the rendered playfield to be offset
by 8 pixels, due to a required "spilloff area" for when the first tile
pattern for that line needs to be shifted off the screen.


### Branch prediction
-----------------
Pentium MMX and later processors have improved branch prediction hardware
over the original Pentium, and consequently can correctly detect a branch
condition pattern, so long as the condition does not stay the same for more
than 4 times in a row. The new system is based on keeping track of the last
4 known conditions for any branch that may be allocated in the BTB. Those 4
bits are used to index a 16-element table to fetch 2 bits that indicate the
predicted branch condition (strongly taken, taken, not taken, strongly not
taken), which is then written back after using saturated addition to
increment or decrement the value, based on the actual branch condition that
came from the program.

- The above MMX-based renderer requires 4 or less loop iterations to render tiles. This loop count is very suitable for efficient execution on modern-day processors. So long as this loop count stays relatively constant during playfield rendering, and always less than 5, very little mispredicts should occur.
- Don't modify the above algorithm to draw a full 8-scanline tile. Instead, use another loop counter to have the renderer code reused when more 4-scanline tile blocks have to be drawn.
- Try to keep a render buffer of at least 32 scanlines. This size is sufficient to ensure that object scanline render counts for the largest sized-ones can stay constant at 16 throughout rendering (provided the game doesn't do anything to disturb the continuity of object rendering), and this will help avoid branch mispredicts in the object renderer loop.

---------------------------------
## Merging playfield & object pixels
---------------------------------
The most efficient way to effectively combine playfield & object data into
your final rendered frame, is to always first, render your playfield (or a
section of it, in the case of dealing with a split screen) directly to the
image buffer itself. At this point, to effectively merge object pixels with
the playfield's, each pixel in your image buffer must have an extra 2 bits
associated with it, one of which will represent the transparency status for
a playfield pixel, and the other the same, except for object pixels (when
drawn later).

Naturally, after rendering the playfield, the image buffer won't have any
pixels with the transparency status for object pixels marked as false. But
now, as objects are rendered, the condition on that the actual pixel is
drawn, depends on these two transparency status bits, the objects own
transparency status, and it's priority. Starting in the order from object 0
(highest priority) up to 63, object bitmaps are "merged" with the playfield,
in the fashion that the following few lines of pseudo-code will show:

IF(SrcOBJpixel.xpCond=FALSE)THEN <br>

IF((DestPixel.OBJxpCond=TRUE) AND ((DestPixel.PFxpCond=TRUE)OR(SrcOBJpixel.Pri=foreground))) THEN <br>
DestPixel.data := SrcOBJpixel.data <br>
FI <br>
DestPixel.OBJxpCond := FALSE <br>
FI <br>

So, as you can see, the destination's OBJxpCond is marked as false, even if
the object's pixel is not meant to be drawn. This is to prevent the pixels
of lower priority (numerically higher-numbered) objects from being drawn in
those locations.

This may raise the question, "Why do you render objects in the order of
0->63 (effectively requiring 2 bits for transparency status), when you can
render them in the opposite direction (which only requires 1 bit for
transparency status)?" The answer is because of what happens on a priority
clash (see the "PPU pixel priority quirk" section of the "2C02 technical
reference" document). Rendering objects in order of 0->63 is the only way to
emulate this PPU feature properly (and some games DO depend on the
functionality of this, as it provides a way to force the playfield to hide
foreground priority object pixels). Otherwise (for 63->0), it would be
neccessary to merge objects to an image buffer filled with the current
transparency color, and then, merge playfield data with the buffer as well.
Granted, this technique will only require 1 transparency (background
priority) status bit per pixel, but since merge operations are slow, and
this technique requires way more of them, this technique is inferior to the
aforementioned one.


### Other tips
----------
- Depending on your implementation of pixel rendering, you may be able to store the 2 transparency status bits inside the pixel data itself. For example, if only 52 combinations of a rendered pixel are being generated, the upper 2 bits in the pixel's byte can be used for this storage. This may mean that you'll have to mirror your video buffer's palette register RGB information 4 times, but is otherwise a good idea. For 8-bit color VGA modes, a legacy mask register (3C6h) allows the programmer to mask out any bits of the written pixel data that are unrelated to color generation. - Don't use branching to avoid drawing a pixel out somewhere. First of all, it only allows you to process 1 pixel at a time, which is slow. Second, CPUs have a hard time predicting branches based on random data (or at minimum, data that produces a branch pattern which is too long to be stored in the CPU's branch target buffers). Finally, sequences of SIMD arithmetic and logical operations can be used to merge multiple bytes of data simultaniously (espically with MMX instructions).
- Avoid unaligned memory access to any data area used by your rendering routines. Each unaligned store incurs a minimum penalty of 3 clocks on a 486, and many more clocks on modern processors. Generally, the shift & merge code required to align data which may be stored on any bit boundary, is not going to take more than 5 clocks on any processor. (The MMX-coded example previously shown, demonstrates how to do the shift & merge operation.)
- Inline code in small loops with a constant ## of iterations, espically if the loop count is low, and is the most inner. This reduces overhead by avoiding a backwards branch, and the loop & index counters required. For example, when drawing tiles, it would be a good idea to inline the code to draw 8 horizontal pixels.

-------------------------
## Frame store optimizations
-------------------------
One of the simplest approaches to render emulation is to draw the entire
playfield to the video buffer, and then place applicable object data in the
buffer afterwards (this makes object/playfield pixel decisions easier to
calculate since the playfield pixels are already rendered). This
straight-forward approach would also be the most efficient way to deal with
rendering NES graphics, were there not certain caveats of using the video
buffer.

- Video frame buffer reading is painfully slow. No matter how fast of a
video card a computer has, reading video memory is going to be at least 10
(ten!) times slower than writing to it. This would effect the time when
objects are being rendered, when the contents of the playfield that underlap
the object need to be read in & merged with the object's pixels.
- Writing to random places in video memory is painfully slow. Because modern
I/O devices (PCI,AGP) in PC's share address lines with data lines (over the
same bus), there's overhead in writing to a random place in the video
memory. This idea was designed with data streaming in mind, so that when
sequential writing occurs (a pretty common thing), bus lines which would
otherwise be wasted on keeping track of a sequentially-increasing address
can now be used to carry data. So, a non-sequential transfer of data to the
video card could take as much as double the amount of time that a sequential
transfer does. This point alone makes rendering the playfield on a per-tile
basis (where you're basically only storing 8 sequential pixels at a time for
each scanline of the tile) directly to the video buffer one of the worst
approaches. Sequential data transfers to the video card are close to optimal
at 256 bytes at a time and up.
- Writing to random, unaligned places in video memory is incredibly slow.
This is because a merge operation has to take place on the hardware level: a
read from the video card (which is already slow), and the write. However,
this operation is only required at the beginning & end of an unaligned,
sequential transfer. Thus, unaligned data streaming to the video card has
less of penalty the larger the transfer is (I measured an 11% additional
overhead on an unaligned sequential store of 512 bytes to the video card
buffer, and double that for a 256-byte xfer). Video addresses which are
divisible by 64 bytes are considered to be aligned.
- Writing to the video memory in small (byte-by-byte) store operations is a
bad idea. While modern PC CPU/chipset hardware may be able to detect &
combine several little sequential stores to the video buffer as full-size
ones, there's no guarantee this will happen. Older hardware certainly
doesn't do this. And- if the small writes aren't being combined together,
than guess what? The chipset will perform a merge operation for each data
item transfered that isn't full size. Obviously, this could be the worst
possible way to send data to the video buffer (well, next to storing small
data at random places in video memory).
- Writing to a non- linear frame buffer (LFB) is slow. At least on one card
I tested, there was a 333% increase in video buffer write speed, after
switching from using the legacy one at address 000A0000. I understand that
basically any PCI video card has LFB-capabilities, but may be inaccessable
due to it's BIOS, or drivers. I guess that this is really a responsibility
of the OS, but either way: use the LFB any way you can.

Now you should see that it's just not a good idea to render graphics
directly to the video buffer (although I don't think any one would do this,
anyway). Old versions of this document discussed using a virtual frame
buffer, which was basically a buffer allocated in regular memory used to
render graphics to (instead of directly to the video buffer). When the
virtual buffer was full, it would then be copied to the video buffer in a
large, sequential operation (just the way the video card likes it!).
However, this method is actually quite inefficient, as the next paragraph
explains.


### Brief info on x86 on-chip caches
--------------------------------
If you know how virtual memory works, well, a CPU cache is basically like a
hardware version of this, although not for a disk drive, but main system
RAM. A CPU caches data on a (so-called) line-by-line basis. Each line is
anywhere from 16 (486) to 32 (Pentium) to 64 (Athlon) bytes in size, and
will most likely grow larger in future CPUs. So, if only a single byte needs
to be read from memory, an entire line is actually loaded into the cache
(this is why data alignment, and grouping related data fields together in
records is important to guarantee the effectiveness of a CPU's cache). This
action also pushes another line out of the cache (ideally the least-recently
used one), and if it's dirty (modified), it will be written back to main
memory.

486's started the on-chip x86 CPU cache trend, with a whole 8K bytes shared
between both data and code. Intel 486DX4 models had 16K bytes. Pentiums had
seperate 8K byte caches, each for data & code. 6th generation x86 processors
again, doubled the on-chip cache size (although maintained the seperate
code/data cache architecture started by the Pentium). The point is, the size
of the (level-1) cache is basically the size of memory that the CPU can
randomly access for the smallest amount of time possible. For even a 486,
this means up to 8K bytes of cachable data structures, which can actually be
quite a bit of memory, if the software is written carefully.

On-chip level 2 cache-based x86 CPU's (introduced with the second-generation
Intel Celeron core) effectively expand the amount of cachable data the CPU
holds, while even sometimes hiding access latencies, by speculatively
loading level-2 cached data structures into the level-1 cache, when the
caching algorithm thinks that the data is going to be used very soon by the
software algorithm. A good example of this would be a routine which performs
sequential operations on a large array of memory.

The trick to effective use of the cache is all how software is written. The
best thing to do, is to write software algorithms which work with an amount
of temporary memory smaller than the size of the CPU's level-1 cache. Even
computational algorithms which appear to require a large amount of memory,
can sometimes be broken down into sub-algorithms, in order to reduce the
required amount of temporary memory. While taking this approach does incur a
little load/store overhead, it's more important that your data stay in the
cache any way it can. These guidelines will pretty much guarantee that your
software will perform in the most efficient way on any CPU with an internal
cache.


### The virtual frame buffer caveat
-------------------------------
Lets consider the virtual frame buffer (VFB) model. We start rendering our
playfield. Name tables and pattern tables are accessed, and that's fine (the
name tables are easily cached, and even some pattern table data gets
cached). Then, we store our rendered pixel data to our buffer. The pixel
data is stored to the VFB using a data size of 4 bytes (or otherwise, the
biggest size that the processor will allow the programmer to store with).
However, the CPU's cache line size is always bigger than this, and therefore
the CPU performs a merge operation with written data, and the cache line of
the data being written to.

Now- here's the first problem: the target of the store operation to the VFB
is unlikely to be in the cache. This means that the CPU ends up actually
*reading* main memory after your first 4-byte pixel store. Of course, now
you can write to this line for free, but main memory access is slow, and
considering what we're doing here (which is exclusively store operations),
it's kind of ridiculous that the programmer has no way of telling the
processor that the merge operation (and moreover the reading of main memory)
is unneccessary, since we plan on overwriting all the original data in that
particular line (extensions to the MMX instructions introduced with the
Pentium 2 and later processors offer reasonable ways of dealing with
non-temporal storage).

Anyway, you get the idea: after every few stores to the VFB occur, a new
line from the VFB will be read in from main memory (or, the level-2 cache,
if it's in there). But guess what? this isn't even the worst part of it. As
you keep filling the VFB, your CPU's cache overflows, since your CPU's L1
cache is smaller than the VFB you're working on. This means that not only
will your VFB-rendering eventually push any lines out of the cache which
aren't used directly by the render routine (causing lost cycles for even
local routines that may need them immediately after the render), but after
the render when you go to copy the VFB to the video memory, the entire
buffer has to be loaded back into the CPU's cache.

Of course, size of CPU cache is everything here. Due to the sequential
access patterns of the virtual frame buffer rendering model, this algorithm
may actually perform modestly on CPU's with large on-chip level-2 caches
(due to the speculative loading of data from the level-2 to the level-1
cache). However, I can't say I know what the performance penalties may be
for running this algorithm on CPU's with external level-2 caches. So in
general, I would recommend against using the virtual frame buffer algorithm
model targetted for CPUs without an on-chip level-2 cache of at least 128KB.


### Scanline stores
---------------
By reducing the size of the VFB from full size down to a few scanlines (or
even just one), most or all of the caveats of what has been mentioned can be
avoided. Since typically a VFB scanline is 256 bytes (in the example for the
NES's PPU), this makes the memory requirement small enough to ensure good
performance even on a 486.

Of course, this creates a new problem for writing the PPU render engine-
tiles can no longer be rendered completely (unless you're using an
8-scanline VFB, but the rest of this topic assumes you're using only a
single scanline VFB). Some overhead caused by only rendering a single
scanline of a tile at a time can be avoided by pre-calculating pointer work
for each sequential tile, and storing it in an array, so that calculations
can be reused for the tile's other scanlines. A similar technique can be
done for object pointer calculations as well.

Prehaps a possible performance boost obtainable through a careful scanline
rendering engine design, is that storing rendered playfield pixels directly
to the video buffer may be permitted, since all pixels on the playfield's
scanline can be rendered sequentially, and thus, can be stored out that way.
However, there are conditions that determine the effectiveness of this.

First, dealing with object pixels which overlap areas of any playfield
scanline will be very difficult (without the use of at least a scanline
buffer), since the playfield tile rendering is usually performed
sequentially, while object tiles generally need to be rendererable at random
horizontal coordinates on the same scanline (in order to emulate object
priorities properly).

The second condition depends on alignment. If the PPU's fine horizontal
scroll offset is evenly divisible by the size being used to store pixel data
to the frame buffer, then alignment isn't a problem. However, in the case
that it's not (and this will occur often, since almost all NES games use
smooth horizontal scrolling), then a method of shifting and merging pixels
in the CPU registers should be used to effectively perform the smooth
horizontal scrolling, in order to avoid a misaligned data store, and the
unforgivable penalty which is associated with performing this action
directly to the frame buffer.


### Overcoming letterboxed displays
-------------------------------
Since the NES doesn't use any complex functions to generate it's graphics
(such as tilt, shift, twist, swivel, rotate, or scale), anti-aliasing has
never been important for pixel-perfect emulation of NES graphics. However,
due to the strange nature of VGA resolutions, to avoid ending up with a
letterboxed NES game screen display (that's one where there are large black
borders of unused screen area on the sides), you will either need to scale
the emulated graphics yourself, or find a way to get the video adaptor
hardware to do it.

For scaling graphics intended to be displayed on a computer monitor,
anti-aliasing is super-important to ensure that only a minimum screen
resolution is required to ensure that artifacts (i.e., distorted or
asymmetric pixels) are as indistinguishable to gamers as possible. A ratio
of 5 destination to 2 source pixels can be used to stretch 256 source pixels
to 640 destination ones (a very common VGA horizontal resolution). For
calculating the color for the middle pixel of the 5, the two source color
values have to be averaged. Note that this requires pixels to be pure
RGB-values (as opposed to palette index values). Other VGA resolutions, such
as 512x384, may also provide some usefulness.


-------------------------
## Smooth audio reproduction
-------------------------
This chapter describes ways to improve NES sound emulation.


### overview
--------
Very few NES emulators out there emulate sound channel operations to the
precision that the NES does it at, and the result is that emulation of some
high-frequency rectangle and noise waves that many NES games produce on a
frequent basis, will end up sounding like there are artifacts in the audio
(i.e., two or more apparent frequencies present, even though only one
frequency is supposed to be heard). Increasing sample playback frequencies
can fix this problem, but in the end, sampling frequencies on sound cards
found in PC's and such can only go so high.


### why are there artifacts in the high frequencies?
------------------------------------------------
The NES's sound generators each have an audio output update rate/resolution
of 1.79 million samples per second (approx). Compared to the average sound
blaster payback rate (44100 Hz), this means that the NES's sound channels
have 3125/77, or 40 and 45/77ths times the sample resolution. So, when just
one calculated PCM sample needs to represent 40.6 from the NES's sound
channels (in the same timeframe), it's no wonder the audio sounds so
terrible at high frequencies: approximately 39.6 source audio samples have
been skipped over, and assumed to be all equal to the single sample.


### solutions
---------
Sound blasters have hardware in place to overcome this transparently from
the user, whenever audio signal digital capture is desired. The proof is in
sampling NES music at 44100 Hz, 16 bits/sample: there is no distinguishable
difference between how the real-time generated analog audio from the NES
sounds when compared to the digitally captured sample track. They're either
using primitive RC integrator function circuits on the inputs of it's ADCs
to approximate a time-accumulated average voltage between ADC samples, or
they are sampling the signal many times faster than the output PCM sample
rate (some 2^n multiple), and using digital averaging hardware to produce
each "downsampled" PCM result. Here's more, courtesy of an NESdev veteran:

"What I'm suggesting is that you do the above at a high sampling rate, some
power-of-2 multiple of the output rate, for example, 4x44100 = 176400
samples per second. You would add every four samples together, and divide
by four (downsample), and that would be your output sample.

Suppose your wave amplitude is 1. Here are some examples of generating a
single output sample:

EXAMPLE 1
Oversample Results: 1, 1, 1, 1
Downsampled Output: (1 + 1 + 1 + 1) / 4 = 4 / 4 = 1

EXAMPLE 2
Oversample Results: 1, 1, -1, -1
Downsampled Output: (1 + 1 + -1 + -1) / 4 = 0 / 4 = 0

EXAMPLE 3
Oversample Results: 1, -1, 1, 1
Downsampled Output: (1 + -1 + 1 + 1) / 4 = 3 / 4 = 0.75

So your output samples will not always be a simple 1 or -1. You're really
raising the sampling rate, and then converting the results back to the
output sampling rate."


### simple rectangle channel implementation
---------------------------------------
Simple sound channels like rectangle wave can be designed to approximate the
accurate output of the channel without having to resort to any downsampling
techniques.

- Use a whole-numerator-based wavelength counter to decrement by 40 and
45/77 after every PCM sample is rendered; this simulates the elapsed time in
regular 6502 CPU clock cycles that passes between PCM samples being played
back at 44100 Hz.
- When the wavelength.whole counter goes negative (count expires), this not
only means that the rectangle wave output has toggled somewhere in the
middle of the PCM sample timeframe, but also that volume output will scale
based on how many cycles the channel output was positive during the PCM
sample timeframe. To calculate this, the leftover value in the wavelength
counter can be used.
- If the leftover wavelength value represents the wave while positive, then
the wavelength.whole value can be negated; otherwise, add 40 and 45/77ths to
it.
- To calculate the final PCM output sample, simply scale the channel's
volume level by the ratio between the adjusted wavelength counter, and 40
and 45/77ths.
- Caveat: output rectangle waveforms may not change state more than once per
produced PCM sample, and this makes accurate emulation of wavelengths less
than 40 and 45/77 clock cycles not directly possible with this algorithm.
However, wavelengths that go below this value may be raised from here by the
absolute difference of the two values, to produce an output wave pattern
similar to the actual one that would be produced. Generally though, these
frequencies cannot be heard by humans, and therefore accurate implementation
is not as important, if neccessary at all.


### other notes
-----------
- Always represent non-integer-based counters (like ones that have to
increment by numbers like 40 and 45/77ths) with rational
whole-numerator-denominator grouped integers, rather than using floating
point numbers to represnt the ratio. While floating point numbers can be
very precise, due to how rational number bit patterns repeat forever,
calculations are never 100% guaranteed accurate, and this makes successive
calculations based on calculated data a bad idea. However, whole-numerator
counters can be incremented with integer delta values to guarantee no
arithimetic calculation accuracy loss. Finally, these actions should be
carried out if the numerator becomes numerically greater than the
denominator after an increment operation:
* decrement the numerator count value by the denominator.
* increment the whole number counter.
- Make sure you use cycle count information passed to sound hardware
emulation routines from the CPU core to effect sound channel outputs at
correct times in the emulated frame. That means that sound channel operation
updates should *not* be on a per-frame basis, even though this technique
works for the majority of NES game music code. Many writes to sound channel
registers are effective almost immediately after the write, and apparently,
some NES games actually take advantage of timed sound port code to produce
some really neat sounding effects. Also, for emulators that support more
than the regular amount of 6502 clock cycles per frame, sound hardware
should ignore any clock cycles greater than 29780 and 2/3rds, relative to
when the game's main sound animation routine was last triggered (assuming
that PPU-based NMIs are used for sound animation, but sometimes the 2A03's
frame counter is used for this).


------------------------------------------------
## 6502 instruction decoding & execution techniques
------------------------------------------------
- Instruction component-based emulation. This core model breaks all 6502
opcodes down into just two components: addressing mode, and ALU operation.
Since addressing modes and ALU operations are combined to make all 6502
opcodes, it seems to make sense to emulate 6502 opcodes on this basis. As a
result, only essential 6502 core routines will need to be coded, and this
will not only save big on code memory, it will make implementation easier.
Also, this technique is only slightly slower than the opcode-handled
approach, due to the extra jump in the instruction decoding process, but
this is made up for in the host CPU's cache performance, due to more
efficient use of code structures. In general, this technique will yield the
best well-rounded performance for any PC platform.
- Instruction-based 6502 opcode interpretation. In this CPU core model,
fetched 6502 opcodes are used as an index into a 256-element jump table,
where each jump target points to an inlined routine that handles all the
6502 actions to mimic for that instruction. This CPU model is the most
popular, as it's the easiest to implement, and can actually be reasonably
fast, depending on how well the opcode handlers are written (inlining
subroutines and unrolling any loops contained under opcode handlers will be
important for speedy emulation). The only real drawback of this technique is
that it doesn't make very optimal use of memory storage area, as many code
sequences under opcode handlers will have to be duplicated dozens of times.
This will cause somewhat of a performance penalty on those CPU's with
smaller (16KB or less) L1 code caches.
- Dynamic 6502 opcode recompiliation. In this CPU core model, 6502 opcodes
are decoded, but instead of emulating the behaviour of the CPU with
subroutines, platform-specific CPU machine code based on the decoded
instruction is generated and executed to do that instead. Eventually all
6502 opcodes will be translated & cached in the emulator's memory map,
provided adequate processing time is given to the core to trample through
all the 6502 code it may ever execute. The throughput of executing
recompiled 6502 instructions can actually be higher than doing so on a real
6502 itself, provided the programmer does a good job of implementing
optimizations in the recompiled instructions (i.e., the requirement of
including flag maintenence code for most recompiled instructions is not
neccessary, since only branch and add/subtract instructions rely on them.
Another optimization may be possible through the use of clock cycle tables
for 6502 code segments (code that's defined between branch targets or PC
xfer instructions), in order to eliminate clock cycle maintenence
instructcions in some of the recompiled code as well). Caveats of this CPU
core model (besides very complicated implementation of the architecture),
include the requirement for large amounts of RAM (a few or more megabytes),
and other complexities that arise when a 6502 program frequently modifies
it's own code (stored in RAM) which has already been translated & cached by
the CPU engine. For multitasking dozens, even hundreds of NES applications
on a single, state of the art computer however, dynamic recompiliation is
the only way to go.
- Microcode-based 6502 opcode interpretation. In this CPU core model, when a
6502 opcode is fetched, the byte is used as an index into a 256-element
table containing a short list of subroutine pointers for each element, that
represent the actions that the 6502 engine will take on each clock cycle
that the opcode instruction executes for. These microcode sequences are
reused across different opcodes in different combinations, in order to form
the actions that a single opcode performs. There is alot less microcode
instructions to deal with than there is opcode instructions, and this
reduces core complexity. By revolving events that occur in your 6502 core
around a microcode table, you can make it possible for a new 6502
instruction (i.e., an old "jam" one) to modify the table, so that future NES
applications may be allowed to program in their own custom, more useful and
efficient 6502 instructions, in order to improve the speed & quality of an
NES game. Because of the clock-cycle granular execution, this emulator model
is more object-oriented than any other, and provides the closest possible
simulation of the events that occur in a real 6502 (this includes simple and
logical implementation of all dead 6502 instruction cycles). In terms of
average emulation speed however, this technique falls very short of others.


### Other tips
----------
- Some NES games rely on the extra dummy store cycle that RMW instructions
perform on a 6502. This is usually done to pulse a bit in the mapper port,
with a single RMW instruction. Other 6502 "features" (even undocumented
opcodes) may also be assumed to be implemented in the host CPU for an NES
game (or sometimes game genie codes/patches), so don't skip over any details
during your implementation of a core. For more info, check out the "2A03
technical reference" document.
- Implement a clock cycle counter into your 6502 engine, which will be
maintained by every 6502 instruction executed. This counter will mainly be
used by the PPU to figure out how timed writes will effect how the output
image will be rendered. However, if used also as a terminal counter, when
the count expires, program control can be transferred to the handler
originally requesting the count operation (like for generating the PPU
VINT/NMI signal). Also, don't forget that you can manage any number of
"virtual cycle counters", without ever having to make the CPU core maintain
more than one physical one. NES hardware may have several IRQ-generating
counters going simultaniously, but the order in which each will cause an IRQ
is always known to the emulator, which is why the cycle count register only
has to be programmed with the count value for the next IRQ to occur (after
which, the next count to expire can be loaded into the cycle count
register).
- As 6502 instructions usually require the P (processor status, or flags)
register to be updated after an ALU operation, the x86's (or otherwise,
another platform-dependent CPU) ALU instructions updates it's flags register
in a similar mannar. Therefore, after emulating the ALU behaviour of a 6502
instruction with an x86 one, use instructions like "LAHF" or "SETcc" to
acquire the status of sign, zero, carry, and overflow condition codes.
Furthermore, have your emulator store the 6502 flags in the format that
they're stored in on the x86 CPU. This way, the flags do not have to be
formatted, thus saving time. The only time the flags will have to be
converted to/from the 6502 order, is when 6502 instructions PHP, PLP, BRK
#xx, RTS, and hardware interrupts are executed. Since these happen much less
often than more common arithmetic and logical instructions, it's more
efficient to handle the flags in this way.
- use platform-specific CPU registers to store some commonly-accessed 6502
pointer registers in if possible, as this reduces load/store dependencies,
and address generation interlocks (AGIs) in emulation software code. This
basically includes the PC, S, X, Y, and TMPADDR 6502 internal registers.
- the 6502 apparently has about 12 opcodes which jam the machine
(processor). These opcodes are ideal for implementing emulator-specific
custom 6502 instruction set extentions for trap/debug purposes.


--------------------------
## Emulation address decoding
--------------------------
Emulation address decoding is taking a formed 6502 address, plus the 6502's
read/write status, and running it through (most the time) static logic to
determine the access method, and the emulator-equivelant memory address that
this 6502 address corresponds to, in order to emulate access to that memory
location properly. With this approach, these decoded addresses in your
emulator can be treated as either a direct pointer to the data, or as a
pointer to a subroutine that the CPU core calls when additional code
neccessary to accurately emulate the events of that 6502 clock cycle. The
best-known technique for doing this is discussed as follows.

Using a 1:1 address decode look-up tables for both read & write 6502 memory
maps is the fastest and most accurate way to determine where an NES memory
area is, and what address it maps to. Generally, a byte should be used as a
single element in the memory maps to represent the type of mem area (up to
256 types for each table), and you'll have 128KB of them, since the 6502's
R/W line is also used during address calculations. Even though this
technique _seems_ to waste a lot of memory, the memory decode tables are
most commonly accessed in parallel with memory areas containing NES ROM and
RAM structures, and this means that cached data structures residing in the
emu's host CPU (due to simulated 6502 memory bus transfers) will usually
never require more than twice the amount as normal. This is a small price to
pay to ensure that adapting your 6502 core engine to any foreign NES/FC
architecture/technology, is as easy as adding a few new memory area type
handlers to your emulator's core, and then building a new address decoder
table.


----------------------
## Hardware port queueing
----------------------
Hardware port queueing allows the CPU to write out (and sometimes even read
in) data targetted at a hardware port in the virtual 6502's memory map,
without having to break CPU emulation to call a hardware port emulation
routine/handler. This is possible through buffering writes out to the
particular port, so that a hardware emulation routine may process the data
later on (i.e., out-of-order from the one the CPU core issues it in).

### pros
----
- program control transfers are evaded when common hardware ports are
accessed by the CPU core. This in turn reduces code & data cache misses, and
espically branch mispredicts, in the physical CPU running the emulation
software.
- dynamically adding hardware devices to the CPU core's virtual memory map
will be easier, due to the architectural enhancements that hardware port
queueing requires the CPU core to support.
- less code will be produced in the emulator software's image file, due to
there being less hardware port emulation handlers present.
- large overhead penalties that are incurred when hardware emulation routine
loops (like for rendering pixels, creating audio samples, etc...) have to be
broken (due to the CPU core writing out to the hardware handler at that
moment in the simulated frame), can be avoided. This is important for 2
reasons:
1. your NES emulator core engines can now be designed to operate in one big
loop, without having to worry about intervention from other hardware devices
during the same virtual NES emulation time, unless it's absolutely
necessary. This means that say, the PPU engine can render a complete frame
at any instant (as opposed to having to depend on data sent to the PPU
engine in real-time via the CPU core), thanks to hardware port queueing.
2. no matter how your NES-written 6502 code abuses the PPU, APU, MMC, etc.
hardware in the NES, your core engines of all these devices can all now be
designed to use a nearly constant amount of CPU clock cycles on the physical
processor running your emulator's software, thanks to the simple loop design
of emulator core devices, in combination with branchless code solutions to
if/else constructs and the like.


### cons
----
- uses some extra data structures/memory
- more difficult to implement than standard real-time handler-based approach

### overview
--------
The hardware port queueing concept is only benificial for those hardware
devices that do not interact with (i.e., change or effect the operation of)
the CPU core, outside of readable ports like $2002. So, for example, you
wouldn't want to buffer writes to the cart mapper hardware if it's effecting
a PRG-bank (due to the fact that the write is supposed to effect CPU
emulation immediately), but the opposite is true for CHR-bank changes. So,
this is essentially the criteria that you must base your decisions on, when
deciding which hardware ports should be queued.

Hardware devices that generate interrupts on the CPU are a little easier to
deal with, since interrupt sources almost always come from some sort of
on-going counter in the NES (the MMC3's scanline counter, is a slight
exception, since it relies on the clocking of A13 on the virtual PPU).
Execution of the events that are to occur on the terminal count clock cycle
can be queued to the CPU by creating an instance of a virtual cycle counter
by the hardware emulation routine that needs it.


### implementation
--------------
The "port queueing" idea really revolves around assigning back & forward
pointers to _all_ hardware-related (PPU, in this example) memory addresses
that can be modified by the CPU. These pointers then link into a 1+2 way
list that represents the queued data for that memory address. This means a
pair of pointers for:

- each standard PPU registers (2000-2007, though you might not need to do
all of them (keep reading...))
- each palette memory element
- each OAM element
- each name table element*
- each patten table element
- any bankswitching regs
- each element in CHR-RAM, if it exists*
- etc...

(* only physical addresses need to be considered here, since any
bankswitches will be queued.)

When the CPU core decodes writes to ports like $4014, the CPU core will
examine that port's status as a queued port, along with the pointer to the
last allocated link in the list of queued writes for that port will be
decoded. If queueing is enabled for this port, the CPU will use the pointer
info, along with memory allocation info and the current cycle count, to
insert a new link into that list, containing the CPU write data.


### attributes of a list element
----------------------------
- CPU clock cycle this write occured on, relative to last write
- next allocated link for this list
- last allocated link for this list
- frame ptr link
- data

A relative clock cycle tag value allows hardware emulation routines reading
the value later on to determine when the next related write to this port
occurs.

Fwd/back pointers are used in each element in the list for 2-way travel.
This is required, since it is often neccessary for the hardware to know the
last-known value of any memory it may have access to.

A third, one-way pointer in each element in the list will be used to link
all nodes created from the same core engine in your emulator together. This
makes deallocation of all those links very easy, with list length being a
direct function of the number of hardware writes that occured that frame
(so, generally not that much). Note that links with the "last allocated
link" field = 0 are *not* to be deallocated, since these represent links
that must be present for the next frame's calculations.

For writing to ports like $2004 and $2007, which are designed to have data
streamed into it, this will require some additional logic on the CPU core's
part to calculate the link list address (since there's an additional lookup,
and an address increment required). This would normally be done with a
hardware port handler, but this approach would be frowned upon, since the
whole point of implementing hardware port queueing is to avoid transfering
emulation program control out of the CPU core into other modules, unless
absolutely neccessary.

For handling CPU reads from hardware ports, it's a simple matter of
determining whether or not the port handler has to be called or not. For
example, when $2002 is read, it's status often doesn't change until a
certain (independent) clock cycle in the frame has been reached. In this
case, the port would be read for the first time, and the handler would be
invoked. The handler would then calculate the next clock cycle at which
$2002's status is expected to change, and creates a virtual cycle counter
instance, programmed to execute another $2002-related handler when the cycle
count expires. Meanwhile, the handler changes the CPU memory map layout so
that subsequent reads from this port simply causes the CPU core to read from
a regular memory address, where the last known port value is stored, thus
avoiding unneccessary calling of $2002's read handler, until the virtual
counter goes off.

For handling CPU reads from ports like $2004 and $2007, the CPU core simply
has to return the last-known value of the element being accessed from the
array queues.


--------------------------
## Threading NES applications
--------------------------
Lately, x86-based PC's have become so blazingly fast, that emulating just
one virtual NES on a modern PC, would seem to be a waste of processing
power. With that said, modern PC's have enough processing power to emulate
dozens of virtual NES machines, but there is one big problem with
multitasking NES applications: they were never designed to be threaded.
Instead, an entire frame's worth of NES CPU clocks have to be wasted for
each NES application, in order to consider the application's frame
calculations complete, whether or not this may be true (and if not, a
slowdown will occur). The following hints and tips suggest ways to reduce
wasted time in virtual 6502 emulation normally lost due to spin-wait, poll,
or cycle count loops.

- Interrupt routine thread tracking. All interrupt routines can be threaded
regardless of whether or not a proper RTI instruction is executed at the end
of the handler. By trapping access to the PC address value saved on the
stack from the executing interrupt, a handler could gain control the next
moment that the old PC address address is accessed again, which will be most
likely when the interrupt routine is done. There is an exception to this:
games that only set flags in the interrupt handler, and then return. In this
case, the thread will be short, which is why access to the saved PC address
should be accessed twice, before an interrupt-based thread should be
considered finished.
- For ports frequently used in polling loops (like $2002), these handlers
can do a basic poll loop comparison to the current location of the PC, to
determine if the port is being polled, and the condition under which the
loop will be exited. Since flags like vblank, >8sprites, and priobjcollision
all happen at a static moment in an emulated frame, it's easy to make the
PPU handler advance the CPU's cycle counter directly to the clock cycle at
which these flags will meet the loop exit condition, and thus saving virtual
6502 CPU time.
- Writes to NES hardware conditioning their on-going operation, which have
not been preceeded by an interrupt event or a polled port, can be assumed to
be timed by cycle counting code. In this case, if an algorithm can detect
the presence of a simple cycle counting loop, tens of thousands of host CPU
clocks per frame can be saved by replacing this type of 6502 loop, with
special 6502 jam instructions which just tells your 6502 core to wait for a
specified cycle count before proceeding.

----------------------------
## Emulator features to support
----------------------------
This section merely contains some innovative and interesting suggestions for
features to support in new NES emulators being developed.


- Compatability with original NES/SNES controllers (a document explaining
how to connect them to a PC is the "NES 4 player adapter documentation").
This not only allows gamers to play NES games on your emulator with an
original controller/lightgun/etc. (rather than having to use the keyboard),
but also allows unused buttons on a SNES controller to have customizable
functionality during gameplay (game/state change, suspend, fast forward,
save/load machine state, and reset functions would be most handy).
- Fully adjustable virtual PPU framerate emulation. This control allows
gamers to program on-the-fly, the PPU's framerate speed. Since pretty much
all game code revolves around PPU frame interrupts, changing this frequency
effectively changes the speed at which the game runs (usually controls audio
as well). This can be useful for fast-forward or slow-motion effects,
espically when spare controller buttons are used to accomplish the effects.
Additionally, I've discovered with my friends that playing an NES game at a
higher framerate (90 Hz in our case) really adds new challenges and fun to
just about any old NES game you can think of.
- Fully adjustable virtual APU framerate emulation. For games that use this
interrupt source, changing the frequency of this signal will change the
playback speed of the game's audio.
- Slow downs in NES games should be eliminated by either providing the user
a way to adjust the number of CPU clocks to execute per PPU frame, or by
threading the game's NMI handler. Besides, if the player wants to slow down
the game action, they should be able to do it by activating a slow-motion
button, as opposed to being forced to slow down simply whenever the game's
frame calculations get a little too heavy for a standard 29780 2/3 cc-based
frame.
- sprites displayed per scanline should be adjustable (for development
purposes), or if not, unlimited (since this eliminates *alot* of sprite
flicker).
- provide a way to let the user custom mix audio generated by any NES sound
hardware used by the game, into 6 audio tracks for playback through a 5.1
soundsystem.
- provide the user a way to program in an alternate, custom waveform to be
used for triangle wave channel playback, and as well as the 4+4 duty cycles
used between the rectangle wave channels. pitch bending, and programmable
sound delays performed on a cloned audio channel source is also another way
some neat new sounds can be heard on the NES for any old game.
- Allow the user to specify a custom size and additional scroll offset to
apply to the displayed PPU playfield (rather than just defaulting it to
256x240, 0:0+ScrollCtrs) in your emulator. This not only allows gamers to
crop the edges of an NES game's playfield that has messy graphics around
there, but it also allows the gamer to extend the size of the playfield to
include displaying the contents of 1 or 3 other nametables simultaniously,
as is very useful for games like Pin Ball, Wrecking Crew, Super Mario Bros.,
Duck Tales, Metroid, Jackal, and Gauntlet to name a few. An option should
also be provided to prevent PPU scroll counters (X or Y) from being used in
the final playfield scroll offset caclulation, but rather have them applied
to the offset of the object frame (this causes the objects to move around
the screen, rather than having the playfield do that while objects stay
relatively in the middle of the playfield).
- Provide a graphics filter for virtual OAM set swapping. This technique is
used when the game needs to display more objects than the PPU hardware
supports per frame. Games alternate between two (or more) OAM sets between
frames, and this does let the gamer see the extra objects, but not without
having to settle for a large amount of flickering sprites. A primitive
technique for filtering OAM set swaps is to extend the number of sprites
displayed on any frame to include one or more from previous frames.
Normally, only the last frame's OAM set needs to be saved to eliminate
serious flicker from sprites in games like Mega Man 2, but somtimes two or
more old OAM sets are neccessary. In this case, it's better to implement a
sophisticated OAM set pattern search engine that eliminates the high
overhead of re-rendering a same typed & placed sprite appearing in 2 or more
OAM sets.
- Provide rewind play motion and record NES movie support. these two work
together, along with save states, to produce NES movies of only your finest
play performances in a favorite game.
- Support hardware emulated FDS ROM BIOS subroutines. This essentially
reduces disk load and save wait times to null. As a result, old FDS-based
famicom games will run as fast as ROM-based ones.
- Support an on-line text & art galery. Users should be able to look through
a collection of bitmap-formatted images relating to NES stuff (this may be
screenshots, scanned pages of instruction booklets, label art, etc.). Just
think of how the "Super Mario All Stars" game selection menu looks, and now
pretend that there are many more selections, and they span off in two
dimensions. Now you're talking about an interesting new feature to implement
in an NES emulator.
- Allow multiple instances of virtual NES machines in your emulator. This
has the potential to allow a gamer with a very fast PC to transform their
NES/FC game ROM colection into a personal home NES/FC video arcade, with the
help of a high-resolution video display mode. The emulator can automatically
search the local file repository to collect a list of all available NES ROM
images and the like, and loads game states (initial, if no others) into
virtual monitor screens emulated in the emulator's main operating window.
The emulator's "viewing window" will allow the user to scroll around the
virtual wall of NES video monitor screens; this is how the user may navigate
around between different NES games and states (i.e.; we toss the concept of
having to choose games and states by filename completely out the window).
Game states can initially be loaded into a virtual monitor matrix based on a
square spiral algorithm, but after this, cut, copy, paste, move, and delete
operations could be performed on those game states to manipulate them as the
user sees fit, possibly increasing or decreasing the size of the monitor
matrix. And of course, personal emulation settings can be stored for each
game state, so that for example, only selected NES games states will be
animated during the time the NES arcade emulator runs (suspended game states
can simply be displayed that way on monitors in the virtual NES arcade).

-------------------------------------------------
## New object-oriented NES file format specification
-------------------------------------------------
This section details a new, extremely easy to use standard for digital data
storage of NES ROM images and related information, which provides as much
object-orientation for the individual files as possible.


### What does object-orientation mean?
----------------------------------
In this case, I'm using it to describe the ability for the user to access
specific information related to any NES/FC game stored ditigally on a local
file repository, whether it be program ROM data, pattern table ROM data,
mapper information, pictures and other digital images (label art, game
manual pages, etc.), game save states, battery RAM states, etc., without
having to rely on any custom or proprietary NES/FC software, simply by
storing the several components that make up a digital copy of an NES game
into individual files of known/established types, and grouping those files
in a subdirectory folder named after the game.

Take for example, the UNIF standard: this is an excellent example of a
monolithic file format structure. UNIF is a file format that forces people
to rely on UNIF-conforming tools to access the data chunks inside it, be
them bitmaps, jpegs, program ROM data, etc., when if these data chunks were
just stored as individual files in a directory on the local repository,
there would be no need for the UNIF-guidelines to access this data.

So basically, the idea here is to use existing file formats to store all
information related to a single game, within a private directory on your
filesystem amongst others, making up your electronic NES game library. All
like file types may have similar extentions, while having different
filenames, usually relating to the specific description of what the file
represents (i.e., files relating to save state info, may have a title that
describes the location or game status of the state, or patch files may
describe the operation of the patch during emulation, etc). As other
relivant file formats (like *.jpeg, *.gif, *.bmp, etc.) have been long
established computer standards, only file formats relating to NES operation
are defined here.

*.PRG a perfect digital copy of the game's program ROM. <br>
*.CHR a perfect digital copy of the game's pattern table ROM, or RAM (for <br>
save states). <br>
*.MMC a text tag, identifying the PRG/CHR ROMs complete mapper type <br>
*.INES a classic 16-byte iNES header used as an alternative to the *.MMC <br>
file. <br>
*.WRAM 2K RAM tied to 2A03 (6502) bus <br>
*.VRAM 2K RAM tied to 2C02 bus <br>
*.XRAM extra RAM (other than CHR RAM) used on the game cart <br>
*.SRAM any battery-backed RAM used on the game cart <br>
*.PRGPATCH program ROM patch <br>
*.CHRPATCH character ROM patch <br>
*.PRGHACK text file containing a list of program ROM patches. <br>
*.CHRHACK text file containing a list of character ROM patches. <br>

This list isn't complete (as 2A03, 2C02, and MMC memory structures will
always be emulator-specific), but it should give you an idea of how to
seperate files relating to raw dumps of large internal memory structures
used inside the NES, in order to improve the portability of the ROM files,
large RAM structures, save state dumps, patches, hacks, and such.

*.PRG and *.CHR: the digital contents of program & character ROMs found on
the NES game board. It would be nice to see these files maintain at all
times a 2^n count of bytes, except when other PRG/CHR ROMs have to be
appended to their respective files, due to the possibility that an NES game
may use two or more differently-sized ROMs to make up a larger one (before
1987, this was mostly done to increase a game's ROM size with more chips,
since it seems that ROMs larger than 32KB were just either very expensive,
or not available back then). The filename always relates to the name of the
game, including if it's been hacked, country it's from, or whatever. *.CHR
files that are produced for save state purposes when NES game carts use
CHR-RAM, use a related save state's description as a filename.

*.MMC: simply a regular text file containing a tag of the ASCII-encoded
board type that NES and Famicom games use. Use the file's size to determine
length of the digital tag. The UNIF format does a pretty good job of
outlining the various NES/Famicom cart board types there are; these are the
text tags to use for this file. The *.MMC filename indicates the PRG ROM
associated with the mapper type specified by the MMC file.

*.INES: a 16 byte file containing the iNES header equivelant of what a *.MMC
text file would normally represent. This file only exists because digital
storage of NES game ROMs is currently dominated by the dated iNES format.
Support is not recommended in new emulators (if you're not part of the
solution, you're part of the problem, right?).

*.WRAM, *.VRAM, *.XRAM: these all define files which contain mirror images
of the RAM chips they represent in the NES being emulated. The filename for
all of them relates to the save state description.

*.SRAM: defines the game's battery-backed RAM area. Filename relates to
description of backed-up RAM (game and state specific). Maintaining multiple
copies of SRAM is useful for storing more saved game data than just one SRAM
file allows (which is usually 3 save files per SRAM, though this is always
game specific).

*.PRGPATCH, *.CHRPATCH: These files contain a (little-endian) 32-bit offset,
followed by the raw data to be patched into the ROM type indicated by the
extention. Filename here always relates to the effects the patch has during
emulation. Filesize is used to determine the length of the patch (minus 4 to
exclude the offset value).

*.PRGHACK, *.CHRHACK: these files define lists in plain text that define the
patch files to apply to game emulation, when this specific HACK file is
chosen to be applied for the emulated game. The filename relates to the
group of patches you've chosen for this file (normally, this doesn't matter,
but it's useful for storing multiple hack profiles (ones that make the game
easier, harder, wierd, behave like an NSF file, change graphics, etc)). Use
ASCII formfeed and/or carriage return codes (13 and 10) to seperate listed
patch types in the file.


### notes
-----
- when more than one file of type *.PRG, *.CHR (when not RAM-based), or
*.SRAM is stored in a single game's directory, the emulator is responsible
for making sure the gamer may select the active RAM/ROM(s) to use during
emulation, since game emulation can only be based on one source of these.
- the emulator must have the ability to detect & present all the different
HACK files available in a game's directory, since the effects of only a
single HACK file may be applied to a selected game ROM in there.
- Any emulator-specific file formats should be clearly documented by the
author.
- Any game ROMs that do not have a matching MMC-typed filename in the same
directory, should cause the emulator to refuse to emulate the game ROMs.
- This format _does_ complicate the transportation of NES ROM files a bit
for general emulator users/gamers, but in the end, there's only the PRG,
MMC, and optional CHR and SRAM-typed files required for transport (so, 2..4
files max). This is hardly difficult for even a basic user to comprehend.

