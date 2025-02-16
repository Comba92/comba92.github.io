+++
date = '2025-02-16T14:12:15+01:00'
draft = true
description = 'BusDispatch'
linkTitle = 'Bus addresses dispatch in emulators: a few solutions'
title = 'Bus addresses dispatch in emualtors: a few solutions to the problem'
summary = 'Every emulator needs some kind of address dispatch so that data can be redirected to the correct devices. There are two solutions for this problem.'
author = 'Comba92'
tags = ['rust', 'coding', 'nes', 'gameboy', 'emulation']
keywords = []
+++
When developing emulators, you will need to implement some sort of "*bus dispatch*" mechanism. This is because most systems use a [memory mapped](https://en.wikipedia.org/wiki/Memory-mapped_I/O_and_port-mapped_I/O) model, where the full address space accessible might redirect on different memory areas, devices or registers, depending on the address ranges read/written to.

In hardware, this is no problem at all, as the manifacturer canwire the bus lines to its liking. In software however, we have to find a solution. There are two that i can think of.

## The problem
We usually have two functions as such:
```rust 
fn read(bus, address) -> value;
fn write(bus, address, value);
```
Based on the address and the system's memory map, we could access different devices and peripherals. So we have to find a way to dispatch the address to the correct target.

## If-else chain
The naive solution is obviously to check in what range the address falls.
This can be done easily with just a bunch of if-elses:
```rust
if ram_range.contains(addr) {
  // access ram
} else if gpu_range.contains(addr) {
  // access gpu
} else if io_range.contains(addr) {
  // access io registers
} else if // and so on...
```

While this is a more than enough solution, it has a few problems.
First of all, for every access, we would need to check every address range individually. This might be bad if there are a lot of different ranges and peripherals.
But the biggest problem is more subtle. It has to do with [branch prediction](https://en.wikipedia.org/wiki/Branch_predictor).


## Dispatch table
Another solution is a [dispatch table](https://en.wikipedia.org/wiki/Dispatch_table).
This will get rid of all conditional branching.
Naively, we could have an array of function pointers as big as the addressable space. For example, on 16bit system, we would need a 64kb array of function pointers. We can then set the address handlers based on what we're expecting an address to do.

We have introduced a different problem, though. It has to do with [branch target predicion](https://en.wikipedia.org/wiki/Branch_target_predictor) and likely [cache locality](https://en.wikipedia.org/wiki/Locality_of_reference).


### Dispatch table in action: Gameboy

### Dispatch table in action: NES


## What's faster?
I don't know! Will benchmark it later... Coming soon


## Read more
[Pipeline](https://en.wikipedia.org/wiki/Instruction_pipelining)
[The cost of branch prediction](https://en.algorithmica.org/hpc/pipelining/branching/)