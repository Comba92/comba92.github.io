+++
date = '2025-02-11T16:11:37+01:00'
draft = true
description = 'NesRefcell'
linkTitle = 'Rust Refcells: how I (un)safely got rid of them'
title = 'Rust Refcells: how I (un)safely got rid of them in my project'
summary = 'Refcells have a big overhead, but you can easily get rid of them based on some hard restrictions.'
[params]
  author = 'Comba92'
tags = ['rust', 'coding', 'nes', 'emulation']
keywords = ['nes', 'emulation', 'coding', 'implementation', 'rust', 'refcell', 'interior mutability', 'unsafe', 'mutability', 'pointers', 'raw pointers']
+++
Rust enforces immutability as the default. It is a sane habit to learn, but very often we need to mutate internally objects, and we also want to have these objects referenced in multiple places. Having a multiple mutable reference to something is not allowed in Rust. Only immutable refenceres can be multiple.

I found myself in this situation. While developing a NES emulator, i have a Cartridge object, which holds the ROM data. This object should be used and MUTATED by two other objects: the CPU and PPU.
The correct solution for Rust would be this:
1. wrap the object inside a RefCell, which gives interior mutability. The object can now be mutated by taking a mutable borrow which is checked at runtime
2. wrap the object inside an Rc, which gives immutable multiple ownership, so that the object can be shared safely in multible places.

The problem with refcells is that they have to check at runtime how many borrows they are sharing, as there can only be a single mutable borrow. This seems like a small overhead, but when we are accessing the cartridge ROM data millions of time per frame, we are summing up a lot of overhead penalities which will slow the emulator down.

I didn't experience any kind of slowdown by using my emulator with refcells, and i don't really know how how much overhead the refcells really have. I still wanted to get rid of them, call it a challenge.

We have two other solutions.

The first one crossed my mind while i was writing this article. We could just split up the cartridge in two: one for the CPU, which can only access PRG-ROM, PRG-RAM and the mapper registers, one for the PPU, which can only access CHR and Nametable VRAM. But there would still be the problem of sharing the banking configuration, as only CPU can write to the registers. This is a possible solution which would need more analysis.

The second one, which i did end up using, is resort to *unsafe raw* pointers. You heard me right. Rust's guys are probably going to lynch me...
The cool thing is that, in my case, using pointers is totally 100% SAFE. How?

1. I know for sure the pointer to the cartridge will NEVER be freed; it will live as long the emulator lives. This means we can't have a deallocated memory dereference.
2. I know for sure I am never changing the pointed value of the pointer: it will always point to the same cartridge object. This means we can't have dangling references.
3. We aren't using multithreading, and there will always be only one consumer mutating the cartridge object. The refcell was basically useless: there is no need for checking borrows at runtime.

With this in mind 