+++
date = '2025-02-10T22:29:48+01:00'
modified = '2025-02-16T18:00:00+01:00'
draft = false
description = 'Banking'
linkTitle = 'Banking system for NES/Gameboy emulators'
title = 'A bulletproof banking system for NES/Gameboy emulators'
summary = 'Short guide on how to implement a convenient and powerful memory banking system for emulators.'
author = 'Comba92'
tags = ['nes', 'gameboy', 'emulation', 'coding', 'rust']
keywords = ['nes', 'emulator', 'emulation', 'coding', 'programming', 'banks', 'memory', 'memory banking', 'memory banks', 'development', 'devlog', 'resource', 'banking', 'gameboy', 'implementation', 'rust']
aliases = ['/page/posts/banking']
+++
Peeking at nesdev's wiki page about [mappers](https://www.nesdev.org/wiki/Mapper), can be daunting, knowing you will have to implement dozens of those to get more games working. Most people will either implement a few and call it a day with their NES emulation, and don't even bother trying with the hardest one.
![Mmc3 board](mmc3.png "An MMC3 cartridge board (upside-down). You can clearly see the the PRG chip (left), CHR chip (right), and the mapper (bottom). The small chip on the right is [Nintendo's CIC](https://en.wikipedia.org/wiki/CIC_(Nintendo)), the lockout chip to prevent piracy. [Image Source](https://nesdoug.com/2019/11/11/23-advanced-mapper-mmc3/) ")
<br>
It is true that to get most games running, you only need to implement the first 5 Nintendo mappers (NROM, MMC1, UxROM, CNROM, MMC3). Even if they only are 5, we have a lot of duplicate functionality. And banking can be a little tricky, as there is a bit of math involved with it. Whenever you are involved with math in code, you definetely want to abstract it out as soon as possible, so you don't have to deal with it again later. This is what I call "nasty code"™️.

For the Gameboy, banking works exactly the same as the NES, so we can use the same mechanism for both systems.

Mappers mostly have to do the same things:
- Handle their registers writes to the PRG-ROM address range;
- Access to banked PRG/CHR addresses;
- Nametables mirroring (it is the same concept as PRG/CHR banking!)
- Other custom (and usually rare) functionality.

We surely do not want to implement each mapper logic from scratch. With this in mind, we can ease our mappers development with some good old abstraction.

## The Mapper interface
First, all mappers should provide a generic interface to work with them. They should store a banking object, which holds all the current game bankings configuration (PRG-ROM, CHR, PRG-RAM, and Nametables VRAM, yes, we also hold the PPU mirroring in there!)
The idea is this:
1. When we build the mapper, we set up the bankings, and any other needed mapper state.
2. When we WRITE to the registers (PRG-ROM address range) to change banks, we update the banking configuration.
3. When we READ to the cartridge PRG-ROM, or READ/WRITE to the CHR, PRG-RAM, or Nametables VRAM, we simply translate the address given the current banking configuration.
4. For more complex mappers, on each CPU clock, we can handle IRQs, audio expansions, and timers.
5. For even more complex mappers, like MMC5, we can add cpu bus and ppu bus custom callbacks.

{{<callout icon="lightbulb">}}
  I figured ALL mappers would need to hold the bankings object, so I have decided to move it out of the mapper object, so that i only have to define it once on the upper object owning the mapper, and then pass it to the mappers through their methods (this means less typing when defining mappers, probably).
{{</callout>}}

{{<callout icon="code">}}
  Only if you are using Rust!
  Rust shenanigans: as we can't know at compile which mapper to use, we are using a [trait object](https://doc.rust-lang.org/book/ch17-02-trait-objects.html), and we have to wrap it inside a [Box](https://doc.rust-lang.org/book/ch15-01-box.html) (heap allocated object), as it's size isn't known at compile time. 
  <br>
  We also need to provide the signature 'Box<Mapper> where Self: Sized' in the trait new() function definition, as the compiler told me to do so. We don't need to specify the where clause whenever we are implementing it, tho.
  Also, don't forget to add Send as a supertrait of the Mapper trait, so we can share the emulator between threads.
{{</callout>}}

This is a sketch of how the mapper interface should be, and I provide some default implementations, as most mappers will always have the same ranges mappings:

```rust
struct CartBanking {
  prg: Banking,
  chr: Banking,
  wram: Banking,  // PRG-RAM
  vram: Banking, // Nametables VRAM
}

enum PpuTarget {
  Chr(usize),
  Vram(usize),
}

enum CpuTarget {
  Cart         // addresses 0x4020..0x6000
  WRam(usize), // addresses 0x6000..0x8000
  Prg(usize),  // addresses 0x8000..
}

trait Mapper {
  // Here we set up the mapper registers, and most importantly the banking configuration.
  fn new(header: &CartHeader, banks: &mut CartBanking) -> Box<Self> where Self: Sized;

  // Here we handle the registers writes.
  fn prg_write(&mut self, banks: &mut CartBanking, addr: usize, val: u8);

  // These will dispatch the /write to the correct target.
  // You would probably prefer using a table of handlers instead of checking ranges
  // Have a look at https://comba92.github.io/posts/banking/#bus-dispatch
  fn map_cpu_addr(&mut self, banks: &mut CartBanking, addr: usize) -> CpuTarget {
    match addr {
      0x4020..=0x5FFF => CpuTarget::Cart,
      0x6000..=0x7FFF => CpuTarget::WRam(banks.wram.translate(addr)),
      0x8000..=0xFFFF => CpuTarget::Prg(banks.prg.translate(addr)),
      _ => unreachable!()
    }
  }

  fn map_ppu_addr(&mut self, banks: &mut CartBanking, addr: usize) -> PpuTarget {
    match addr {
      0x0000..=0x1FFF => PpuTarget::Chr(banks.chr.translate(addr)),
      0x2000..=0x2FFF => PpuTarget::Vram(banks.vram.translate(addr)),
      _ => unreachable!()
    }
  }

  // Mapper custom functionality, by default it does nothing.
  fn cpu_clock(&mut self) {}
  fn cpu_bus_callback(&mut self, bus: &mut NesBus) {}
  fn ppu_bus_callback(&mut self, bus: &mut NesBus, ppu: &mut NesPpu) {}
}
```

The new() and prg_write() methods always have to be implemented, of course.

## Implementing Banking
We now need a generic banking mechanism.
On original hardware, bankswitching was INSTANTANEOUS. There was no loading nor delay when banks were switched; everything was handled by the hardware. We can't do that in software, so we will need to compute the correct address to a big array of data.

A mapper will have a varying amount of 'slots' or 'pages', which are the *system*'s memory ranges, mapped to 'banks', which are *cartridge*'s memory ranges.
The number of slots is mapper dependent, you will always have to refer to the mapper's wiki page to know how many there are.
A slot will be mapped to a bank depending on what was written to the slot select register, this is mapper dependant too.
<br>
We will use a slots array, where each value is the configured bank for the specific slot.
Let's take an hypothetic mapper which uses 4 slots in PRG-ROM:

![Banking example](banking_example_bg.png)

```rust
// The hypothetic mapper in the image will configured like so.
let slots = [bank2, bank0, bank1, bankN]
```
When we get an address, we first have to fiure out in which slot we are:

```rust
let slot_number = (address - range_start) / slot_size;
```

We then get the bank starting address:

```rust
let bank_number = slots[slot_number];
let bank_start_address = (bank_number % banks_count) * slot_size;
```
{{<callout icon ="triangle-exclamation">}}
Some games will write bigger banks numbers than the ones avaible in ROM. We must be sure to wrap the bank number around the biggest possible for that game.
{{</callout>}}
Notice how each bank starting address is a constant. Computing this each time we translate address can be saved work. So instead of saving the banks number, we can save their starting address!

```rust
// We can save some computations by storing the banks starting address instead.
let slots = [bank2 * slotSize, bank0 * slotSize, bank1 * slotSize, bankN * slotSize]
```

Finally, we compute the final mapped address.

```rust
let mapped_address = bank_start_address + (address % slot_size);
```

Notice we wrap around the address by the slot size, so that we always get an address inside the bank. We then add this wrapped address to the bank starting address we got from the slot selection.
The mapped address can now be used to address the full ROM range.

{{<callout icon="pencil">}}
Be aware that divisions and modulos are computationally expensive operations, and we can do better.
They can be optimized for our dear computer friends by using bitwise operators.
Have a look at how [modulos](https://en.wikipedia.org/wiki/Modulo#Performance_issues) can be optimized here.
Can we do the same for the multiplication and division? Why?
The solution is [here](#optimizing-banking)
{{</callout>}}

### An example: MMC1's CHR banking
Let's take a mapper for our examples: the [MMC1](https://www.nesdev.org/wiki/MMC1) CHR banking, as i think it covers most of our use cases.
The MMC1 has two modes for CHR banking. Let's look at mode1 first.

In this mode, there are two CHR slots. As the CHR address range is 8kb ($0 to $1000), each slot will be 4kb.

Whenever we write to CHR bank 0 ($C000 to $DFFF) or CHR bank 1 ($E000 to $FFFF), we will set the first slot to that bank number.
```rust
// for $C000 to $DFFF
chr_banks.set_slot(0, value_written);

// for $E000 to $FFFF
chr_banks.set_slot(1, value_written);
```

Whenever we access CHR, we will get a mapped address to access the full CHR range.
```rust
let mapped_addr = chr_banks.translate(address);
```

We basically don't have to think about mapping anymore: our abstraction is doing all the hard calculations for us!

Now, what for mode 0? Mode 0 will use a SINGLE CHR slot; this means it will be big 8kb. But our configuration is using 2. To deal with this change, we simply treat the 8kb slot as two 4kb slots. This will mean that whenever we write to the chr bank register in this mode, we have to set both slots. We can do something like this:

```rust
// MMC1 will cut the first bit of the bank number in this mode, as this will always give even bank numbers.
let bank = value_written & !1;
chr_banking.set_slot(0, value_written);
chr_banking.set_slot(1, value_written + 1)
```

In conclusion, when setting up the banks, we always set the slot size as the smallest possible slot size the mapper can have. When dealing with bigger slots, we update the relative slots as if they where still mapping to the smallest banks.
<br>
**We also have to be sure to update ALL slots whenever we change modes**. This is incredibly important, because the mode switch instantly changes how the bank selects behave. Mapper should provide an update_bankings() method, which updates all slots whenever a mode is changed.

Now, let's code the actual functions.

### The code: requirements
{{<icon "list">}} What we need:
- how big the ROM data is;
- how many slots the mapper provides;
- how big a slot is;
- the offset of the system memory range ($8000 for PRG-ROM, $2000 for Nametables, $6000 for PRG-RAM).

{{<callout icon="triangle-exclamation">}}
The Baking constructor assumes rom_size and slot_size are powers of two, and slots_count should be an even number. This is important or else the calculations won't work.
{{</callout>}}

### The code: initialization
```rust
struct Banking {
  data_size: usize,
  bank_size: usize,
  banks_count: usize,
  slots_start: u16,
  slots: Vec<usize>,
}

impl Banking {
  pub fn new(
    rom_size: usize, 
    slots_start: u16, 
    slot_size: usize, 
    slots_count: usize
  ) -> Self {
    let slots = vec![0; slots_count].into_boxed_slice();
    let bank_size = slot_size as usize;
    let banks_count = rom_size / bank_size;

    Self {
      data_size: rom_size, 
      slots, slots_start, bank_size, banks_count
    }
  }

  pub fn new_chr(header: &CartHeader, pages_count: usize) -> Self {
    let pages_size = 8*1024 / pages_count;
    Self::new(header.chr_real_size(), 0x0000, pages_size, pages_count)
  }
}
```
{{<callout icon="pencil">}}
  The methods new_prg(), new_wram(), and new_vram() are left as an exercise to the reader!
{{</callout>}}
{{<collapse summary="Click for solution 😠">}}
  ```rust
  pub fn new_prg(header: &CartHeader, slots_count: usize) -> Self {
    let slot_size = 32*1024 / slots_count;
    let res = Self::new(header.prg_size, 0x8000, slot_size, slots_count);
    res.set_slot(res.slots.len() - 1, res.banks_count - 1); // PRG usually have the last slot fixed to the last bank, so that interrupt handlers are always accessible
    res
  }

  pub fn new_wram(header: &CartHeader) -> Self {
    Self::new(header.wram_real_size(), 0x6000, 8*1024, 1)
  }
  pub fn new_vram(header: &CartHeader) -> Self {
    let mut res = Self::new(4*1024, 0x2000, 1024, 4);
    if header.mirroring != Mirroring::FourScreen {
      res.banks_count = 2;
    }

    // this method is on the third exercise!
    res.update_mirroring(header.mirroring);
    res
  }
  ```
  {{</collapse>}}


### The code: operations
We then provide these basic operations. Notice how set_slot saves the bank starting address, instead of the bank number. Also be sure to always wrap them around the maximum avaible count.

```rust
pub fn set_slot(&mut self, slot: usize, bank: usize) {
  // some games might write bigger bank numbers than really avaible
  let bank = (bank % self.banks_count);
  // i do not expect to write outside the slots array.
  // for safety, you might want to index slots with (slot % self.slots.len())
  self.slots[slot] = bank * self.bank_size;
}

pub fn translate(&self, addr: u16) -> usize {
  // i do not expect to write outside the slots array here either. 
  // the bus object should take responsibilty to always pass correct addresses in range.
  // for safety, you might want to index slots with (slot % self.slots.len())
  let slot = (addr - self.slots_start) as usize / self.bank_size;
  self.slots[slot] + (addr % self.bank_size)
}
```
{{<callout icon="pencil">}}
  It would be incredibly convenient to have a method which configures a CIRAM banking given a Nametable mirroring. This is left as an exercise to the reader!
{{</callout>}}
{{<collapse summary="Click for solution 😠">}}
  Nametable mirroring is the perfect use case for our banking system, as nametable VRAM can be treated as slots and banks!
  <br>
  Learn about nametable mirroring here: https://www.nesdev.org/wiki/Mirroring#Nametable_Mirroring
  ```rust
  pub fn update_mirroring(&mut self, mirroring: Mirroring) {
    match mirroring {
      Mirroring::Horizontal => {
        self.set_slots(0, 0);
        self.set_slots(1, 0);
        self.set_slots(2, 1);
        self.set_slots(3, 1);
      }
      Mirroring::Vertical => {
        self.set_slots(0, 0);
        self.set_slots(1, 1);
        self.set_slots(2, 0);
        self.set_slots(3, 1);
      }
      Mirroring::SingleScreenA => for i in 0..4 {
        self.set_slots(i, 0);
      }
      Mirroring::SingleScreenB => for i in 0..4 {
        self.set_slots(i, 1);
      }
      Mirroring::FourScreen => for i in 0..4 {
        self.set_slots(i, i);
      }
    }
  }
  ```
{{</collapse>}}

## Bus dispatch
What about bus dispatching? As we're here dealing with addresses and accessing memory, it's a good time to talk about bus dispatching too.
When developing emulators, you will need to implement some sort of "*bus dispatch*" mechanism. This is because most systems use a [memory mapped](https://en.wikipedia.org/wiki/Memory-mapped_I/O_and_port-mapped_I/O) model, where the full address space accessible might redirect on different memory areas, devices or registers, depending on the address ranges read/written to, and have any sort of side effects.

In hardware, this is no problem at all, as the manifacturer can wire the bus lines to its liking. In software however, we have to find a solution. There are two that i can think of.

### If-else chain
The first naive solution is obviously to check in what range the address falls.
This can be done easily with just a bunch of if-elses:
```rust
if ram_range.contains(addr) {
  // access ram
} else if ppu_range.contains(addr) {
  // access ppu
} else if io_range.contains(addr) {
  // access io registers
} else if // and so on...
```

While this is a more than enough solution, it has a few problems.
First of all, for every access, we would need to check every address range individually. This might be bad if there are a lot of different ranges and peripherals.
But the biggest problem is more subtle. It has to do with [branch prediction](https://en.wikipedia.org/wiki/Branch_predictor). 
Branch prediction is a complex topic, over the scope of this article. 
In the emulator case, memory access is mostly unpredictable, so the emulator's system CPU will have an hard time predicting the address disptach.

## Dispatch table
Another solution is a [dispatch table](https://en.wikipedia.org/wiki/Dispatch_table).
There are multiple approaches to this method, too.

Naively, we could have an array of function pointers as big as the addressable space, so that we can index the handler directly with the address. For example, on a 16bit system, we would need a 64 * 1024 (max addressable value) entries array of function pointers. We would build this array programmatically at startup.
We don't need to check for any ranges now (so no conditional branching!), as we are accessing the array directly.
We also gained an interesting property: we can now change the table entries to our liking, and inject it with custom handlers. This is very handy for mapper-dependent functionality or debugging, for example, and would have been very ankward to do with the if-else chain.

However this still has problems, in my opinion. An array of 64*1024 entries is a little too big. Function pointers are 64bit (8 bytes) in a x86_64 system, so this would mean 8 * 64 * 1024 = 524288, half a megabyte. This would almost fill a CPU cache.
Also, we didn't solve the branching problem completely, as we now have to incur in a function call penalty instead.

We can do two things to make it faster.
First, instead of using function pointers, use handers IDs instead. We define an enum and use it as entries to the table. Enums can be represented as 8bit values, this way we'll be using only 65536 bytes instead of 524288. We then use a switch to dispatch the correct handler. Like so:

```rust
#[repr(u8)]
pub enum CpuHandler {
    Ram,
    Ppu,
    IO,
    Wram,
    Prg,
    OpenBus,
    Mapper,

    // Some examples of custom handlers!
    PpuMMC3,
    PpuMMC5,
    PrgCustom,
}

#[repr(u8)]
pub enum PpuHandler {
    Chr,
    ChrRam,
    Vram,
    Palette,
    OpenBus,
    
    // Some examples of custom handlers!
    ChrMMC2,
    ChrMMC3,
    ChrRamMMC3,
    ChrMMC5,
    VramMMC5,
}

let cpu_handler_id = cpu_bus_table[addr];
match cpu_handler_id {
    CpuHandler::Ram => handle_ram_read(),
    CpuHandler::Ppu => handle_ppu_read(),
    CpuHandler::IO => handle_io_read(),
    // and so on...
}
```

The second thing we can do is, make a table with less entries. Look at NES's [CPU memory map](https://www.nesdev.org/wiki/CPU_memory_map) carefully. Notice how the devices are mapped to big ranges of addresses. RAM, for example, occupies 0 to 0x1fff, and PRG occupies 0x8000 to 0xffff. We can choose an address granularity that will map to a SINGLE handler ID.
This way you can make the dispatch table as small as *8 ENTRIES only*. Like so:

```rust
let cpu_bus_table = [
    CpuHandler::Ram, // 0x0000 to 0x2000
    CpuHandler::Ppu, // 0x2000 to 0x4000
    CpuHandler::IO,  // 0x4000 to 0x6000
    CpuHandler::Mapper, // 0x6000 to 0x8000
    CpuHandler::Prg, // 0x8000 to 0xa000
    CpuHandler::Prg, // 0xa000 to 0xc000
    CpuHandler::Prg, // 0xc000 to 0xe000
    CpuHandler::Prg, // 0xe000 to 0xffff
];
```
Surprised? The same applies to the (PPU memory map)[https://www.nesdev.org/wiki/PPU_memory_map] (i invite you to find a good table size for this too)!

We now only need a formula to map an address to the correct entry in the smaller table. This is very simple.
If the table is 8 entries big, this means we need 3 bits (log2(8) = 3) to address it.
Those 3 bits are taken from the higher part of the original address. So we shift the 16bit address right `16 - log2(table_size)`. In our case, its `addr >> 13`.

```rust
let handler_index = addr >> (16 - ilog2(cpu_bus_table.len()))
let handler_id = cpu_bus_table[handler_index];

match handler_id {
    CpuHandler::Ram => handle_ram_read(),
    CpuHandler::Ppu => handle_ppu_read(),
    CpuHandler::IO => handle_io_read(),
    // and so on...
}
```

## Optimizing Banking
The first exercise was asking about how you can optimize the banking system, as there are some super nerd trickery we can employ here. The explanation is hidden, if you'd like to think about it before continuing.

<details>
<summary>Click for explanation</summary>
ROM sizes are ALWAYS a power of two! And also mapper banks are always going to be a power of two, as there are always an even number of slots. This means we can turn rems to bitwise ands, and multiplications and divisions to bitshifts!

{{<callout icon="stack-overflow">}}
Refer to [this Stackoverflow post about mults and divs](https://stackoverflow.com/questions/25787613/division-and-multiplication-by-power-of-2) and [this Stackoverflow post about rems](https://stackoverflow.com/questions/6670715/mod-of-power-2-on-bitwise-operators).
{{</callout>}}

```
a % b == a & (b-1)
a * b == a << log2(b)
a / b == a >> log2(b)

if and only if b is a power of 2.
```

We have to explicitly do that in code, because the compiler won't catch these optimizations if the values aren't constants.
We might also want to cache the log2() values when we initialize the mapper, as they are constant.
This is the final optimized code.

{{<callout icon="triangle-exclamation">}}
This model makes a lot of assumptions.
- rom_size and page_size are a power of two.
- rom_size SHOULD NOT be less than page_size.
- pages_count should be an even number.
- bank_size and banks_count SHOULD NOT be odd numbers.
This is important or the calculations will break.
It's up to the reader to come up with solutions and handle edge cases. 
{{</callout>}}

Note that here slots are named "pages" instead.

```rust
  pub fn new(rom_size: usize, pages_start: u16, page_size: usize, pages_count: usize) -> Self {
    let bankings = vec![0; pages_count].into_boxed_slice();
    let bank_size = page_size;
    let banks_count = rom_size / bank_size;

    // we cache the log2() values
    let bank_size_for_shift = bank_size.ilog2() as usize;
    let banks_size_mask  = bank_size.saturating_sub(1);
    let banks_count_mask = banks_count.saturating_sub(1);

    Self { bankings, data_size: rom_size, pages_start, bank_size, bank_size_for_shift, banks_count, banks_size_mask, banks_count_mask }
  }

  pub fn set_page(&mut self, page: usize, bank: usize) {
    // originally:  bank = bank % self.banks_count;
    // becomes:     bank = bank & (self.banks_count-1);
    let bank = bank & (self.banks_count_mask);
    
    // originally:  self.bankings[page] = bank * self.bank_size;
    // becomes:     self.bankings[page] = bank << ilog2(self.bank_size)
    self.bankings[page] = bank << self.bank_size_shift;
  }

  pub fn translate(&self, addr: u16) -> usize {
    // originally   page = (addr - self.pages_start) / self.bank_size;
    // becomes      page = (addr - self.pages_start) >> ilog2(self.bank_size);
    let page = ((addr - self.pages_start) as usize) >> self.bank_size_shift;
    
    // originally:  self.bankings[page] + (addr % self.bank_size)
    // becomes:     self.bankings[page] | (addr & (self.bank_size-1))
    self.bankings[page] | (addr & self.bank_size_mask)
  }
```
</details>

## Banking system in action: UxROM
We now have a very handy and convenient interface for developing mappers.
Look at how simple it is to fully implement UxROM:

```rust
struct UxROM;

impl Mapper for UxROM {
  fn new(header: &CartHeader, banks: &mut CartBanking)-> Box<Self> {
    // we have 2 bankable slots avaible for PRG, only 1 for CHR.
    banks.prg = Banking::new_prg(header, 2);
    banks.chr = Banking::new_chr(header, 1);
    // the CHR only slot should not be banked, so set it to the first (and only) CHR bank.
    // as it is set to 0, we don't even have to initialize this, depening on the language you're using.
    banks.chr.set_slot(0, 0);
    Box::new(Self)
  }

  fn prg_write(&mut self, banks: &mut CartBanking, addr: usize, val: u8) {
    if (0x8000..=0xFFFF).contains(&addr) {
      let select = val & 0b1111;
      banks.prg.set_page(self.banked_page as usize, select as usize);
    }
  }
}
```
And that's it! {{<icon "check">}} It is now incredibly addicting to implement mappers. Have fun!
{{<callout icon="list">}}
  [NROM](https://www.nesdev.org/wiki/NROM), [UxROM](https://www.nesdev.org/wiki/UxROM), [CNROM](https://www.nesdev.org/wiki/CNROM), [AxROM](https://www.nesdev.org/wiki/AxROM) are easy mappers which you should definetely implement in your emulator. 

  [MMC1](https://www.nesdev.org/wiki/MMC1) is the most used mapper of the NES. It is fairly more complex, but defintely worth to implement.

  [MMC3](https://www.nesdev.org/wiki/MMC3) is more complex, but is used in almost 300 games.

  [MMC2](https://www.nesdev.org/wiki/MMC2) is only used by Punch Out!!! but can be pretty satifying to implement.

  [VRC6](https://www.nesdev.org/wiki/VRC6) is only used by the japanese version of Castlevania III and other few games, but it has an incredible expansion audio chip which is very fun to implement and finally hear.
{{</callout>}}

## Banking system in action: my NES emulator
I have developed a NES emulator and roughly 30 mappers are working flawlessly with this system.
Have a look at it here: [my NES emulator](https://github.com/Comba92/nen-emulator).
