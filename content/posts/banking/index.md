+++
date = '2025-02-10T22:29:48+01:00'
draft = true
description = 'Banking'
linkTitle = 'Banking system for NES/Gameboy emulators'
title = 'A bulletproof banking system for NES/Gameboy emulators'
summary = 'Guide on how to implement a powerful memory banking system for an emulator'
[params]
  author = 'Comba92'
tags = ['nes', 'emulation', 'coding']
keywords = ['nes', 'emulation', 'coding', 'banking', 'gameboy', 'implementation']
+++
Looking at nesdev's wiki about [mappers](https://www.nesdev.org/wiki/Mapper), can be daunting, knowing you will have to implement dozens of those to get more games working. Most people will either implement a few and call it a day with their NES emulation, and don't even bother trying with the hardest one.

Mappers basically have to do ALWAYS the same thing, though.
- Write to the mapper's registers in the PRG-ROM address range;
- Access to banked PRG/CHR addresses;
- Other custom (and usually rare) functionality

We surely do not want to code each mapper logic from scratch. With this in mind, we can ease our mappers development with some good old abstraction.

## Implementing Banking
We first need a generic banking mechanism.
A mapper will have a varying amount of 'slots' or 'pages', which are the system's memory ranges, which will be mapped to 'banks', which are cartridge's memory ranges.
A slot will be mapped to a bank depending on what was written to the slot select.

A simple example: [MMC1](https://www.nesdev.org/wiki/MMC1).
// to be compiled...

![Banking example](banking_example.png "Banking example")

We need:
- how big the ROM data is;
- how many slots the mapper provides;
- the offset of the system memory range ($8000 for PRG-ROM, $2000 for Nametables, $6000 for PRG-RAM)

```rust
pub struct Banking {
  data_size: usize,
  bank_size: usize,
  banks_count: usize,
  slots_start: usize,
  slots: Vec<usize>,
}

impl Banking {
  pub fn new(rom_size: usize, slots_start: usize, slot_size: usize, slots_count: usize) -> Self {
    let slots = vec![0; slots_count].into_boxed_slice();
    let bank_size = slot_size;
    let banks_count = rom_size / bank_size;
    Self {
      data_size: rom_size, 
      slots, slots_start, bank_size, banks_count
    }
  }

  pub fn new_prg(header: &CartHeader, slots_count: usize) -> Self {
    let slot_size = 32*1024 / slots_count;
    Self::new(header.prg_size, 0x8000, slot_size, slots_count)
  }

  // new_chr(), new_ciram(), new_sram() are left to the reader as an exercise
}
```

We then provide these basic operations:
```rust
pub fn set_slot(&mut self, slot: usize, bank: usize) {
  let slots_count = self.slots.len();
  self.slots[slot % slots_count] = (bank self.banks_count) * self.bank_size;
}

pub fn translate(&self, addr: usize) -> usize {
  let slot = (addr - self.slots_start) / self.bank_size;
  let slots_count = self.slots.len();
  self.slots[slot % slots_count] + (addr % self.bank_size)
}
```

## Mapper interface
```rust
pub trait Mapper {
  fn new(header: &CartHeader, banks: &mut CartBanking) -> Box<Self> where Self: Sized;

  fn prg_write(&mut self, banks: &mut CartBanking, addr: usize, val: u8);

  fn map_prg_addr(&mut self, banks: &mut CartBanking, addr: usize) -> PrgTarget {
    match addr {
      0x4020..=0x5FFF => PrgTarget::Cart,
      0x6000..=0x7FFF => PrgTarget::SRam(true, banks.sram.translate(addr)),
      0x8000..=0xFFFF => PrgTarget::Prg(banks.prg.translate(addr)),
      _ => unreachable!()
    }
  }

  fn map_ppu_addr(&mut self, banks: &mut CartBanking, addr: usize) -> PpuTarget {
    match addr {
      0x0000..=0x1FFF => PpuTarget::Chr(banks.chr.translate(addr)),
      0x2000..=0x2FFF => PpuTarget::CiRam(banks.ciram.translate(addr)),
      _ => unreachable!()
    }
  }
}
```


## Concluding
Have a look at [my NES emulator](https://github.com/Comba92/nen-emulator).