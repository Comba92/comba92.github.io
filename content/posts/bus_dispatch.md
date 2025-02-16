+++
date = '2025-02-16T14:12:15+01:00'
draft = false
description = 'BusDispatch'
linkTitle = 'Bus addresses dispatch in emulators: a few solutions'
title = 'Bus addresses dispatch in emulators: two solutions to the problem'
summary = 'Every emulator needs some kind of address dispatch so that data can be redirected to the correct devices. There are two solutions for this problem.'
author = 'Comba92'
tags = ['rust', 'coding', 'nes', 'gameboy', 'emulation']
keywords = []
+++
When developing emulators, you will need to implement some sort of "*bus dispatch*" mechanism. This is because most systems use a [memory mapped](https://en.wikipedia.org/wiki/Memory-mapped_I/O_and_port-mapped_I/O) model, where the full address space accessible might redirect on different memory areas, devices or registers, depending on the address ranges read/written to.

In hardware, this is no problem at all, as the manifacturer canwire the bus lines to its liking. In software however, we have to find a solution. There are two that i can think of.

## The problem
We usually have two memory read and write functions as such:
```rust 
fn read(bus, address) -> value;
fn write(bus, address, value);
```
Based on the address and the system's memory map, we can access different devices and peripherals. So we have to find a way to dispatch the address to the correct target.

## If-else chain
The first naive solution is obviously to check in what range the address falls.
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
Branch prediction is a complex topic, over the scope of this article. 
In the emulator case, memory access is mostly unpredictable, so the emulator's system CPU will have an hard time predicting the address disptach.

## Dispatch table
Another solution is a [dispatch table](https://en.wikipedia.org/wiki/Dispatch_table).
We store an array of function pointers, and index it given the address.

Naively, we could have an array of function pointers as big as the addressable space, so that we can index the handler directly with the address. For example, on 16bit system, we would need a 64kb array of function pointers.
As we are accessing the array directly, we don't need any conditional branching.

```rust
let access_handlers = [
  access_ram,
  access_ram,
  access_gpu,
  access_gpu,
  access_io,
  // and so on
];

let handler_index = index_handler_from_address(address);
let handler = access_handlers[handler_index];
handler(address, value);
```

We can however be a little smarter: we should find a smaller possible page size which addresses a common device target, so we can use a smaller dispatch table. For example, if we can find that the smaller page size is 8kb big (the first 8kb only address RAM, the next 8kb only address the GPU, and so on), the can decrease the 64kb size array to an array of just 8 entries! 
Indexing the handlers array is then just a matter of taking the highest bits of the address, based on how big the handlers pages are.
If we then need more fine grained control on the handlers, we can use a subtable (IO register are usually only used in a single address, so we would need a different handler for each one).

This methods has it's drawbacks too, however. It has to do with [branch target predicion](https://en.wikipedia.org/wiki/Branch_target_predictor) and likely [cache locality](https://en.wikipedia.org/wiki/Locality_of_reference). 
Most importantly, function pointers can't be [inlined](https://en.wikipedia.org/wiki/Inline_expansion) (obviously), so we always incur in [function call overhead](https://stackoverflow.com/questions/31779335/why-is-there-overhead-when-calling-functions).

### Dispatch table in action: Gameboy
To be compiled...

### Dispatch table in action: NES
To be compiled...

## So, what's faster?
This is an hard question, because it is not easy to tell with just speculation. You should never speculate about a program's performance; there are a lot of variables involved, and you should always benchmark and profile before coming to any conclusion. I leave the question of what method is faster to the reader; as I can't provide a definitive answer.

## Methods comparison: my NES emulator
To be compiled...

## Read more...
- [Execution pipeline](https://en.wikipedia.org/wiki/Instruction_pipelining)
- [The cost of branch prediction](https://en.algorithmica.org/hpc/pipelining/branching/)