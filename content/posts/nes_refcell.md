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
3. We aren't using multithreading, and there will always be only one consumer mutating the cartridge object. The refcell was basically useless: there is no need for checking borrows at runtime in my case.

Now, with this idea in mind, the naive thing to do was to change every call to the cartridge methods to pointer dereferences, and wrap them in an unsafe block. But, do we really have the time and energy to do that? My emulator was basically finished at this time, and this meant i would have to go trough a big codebase changing stuff with the chance of breaking something.
We can be smarter. We are programmers, for god's sake.

The big brain solution here is to provide an interface to access the pointer similiar to the one provided by refcell.
So that we can easily do this:
```rust
// change this
refcell.borrow_mut().call_mutable_method();

// to this
pointer.get_mut().call_mutable_method();
```
No unsafe spreading across the code. No big code modification.
Here is what i've come up with.
```rust
#[derive(Clone)]
struct SharedData<T>(pub *mut T);

impl<T> SharedData<T> {
  pub fn new(data: T) -> Self {
    // save data into heap, the get its pointer
    Self(Box::into_raw(Box::new(data)))
  }

  // this doesn't mutate SharedData, but mutates the pointed data.
  pub fn as_mut(&self) -> &mut T {
    unsafe { self.0.as_mut().expect("pointer should always be valid") }
  }
}

impl AsRef<T> for SharedData<T> {
  fn as_ref(&self) -> &T {
    unsafe { self.0.as_ref().expect("pointer should always be valid") }
  }
}
```

Simple as. Just change all the borrow_mut() calls to as_mut() calls and we're done. Got totally rid of refcells!
We only had to use unsafe ONCE, to dereference the pointer inside the as_ref() and as_mut().

Now we have to be extra careful with creating and destroying this SharedData object.

When making a new SharedData, we should allocate the inner object in the heap with a Box. THIS IS REALLY IMPOTANT. You don't want a pointer which points to a stack value, don't you? We then consume the box, and return a pointer to the allocated value.

When we need a new reference to the pointed data, we can just clone the full SharedData object. 

When destroying a SharedData, it is tricky. We are dealing with pointers after all. If we don't manually deallocate the pointer, we will get a memory leak: the data will stay in the heap and we won't have any more access to it. Rust provides the Drop trait to deal with manual deallocation. We CANNOT implement Drop on SharedData, however. If we did that, if a single SharedData would be dropped, all the others will become dangling pointers. UNSAFE!!
 
```rust
impl Drop for Bus {
  fn drop(&mut self) {
    // This is needed, as we're manually managing a cart pointer to heap
    unsafe {
      drop(Box::from_raw(self.cart.0))
    }
  }
}
```

```rust
pub fn load_from_emu(&mut self, other: Emulator) {
  // save prg and chr in temp values
  let old_cart = self.get_bus().cart.as_mut();
  let prg = core::mem::take(&mut old_cart.prg);
  let chr = core::mem::take(&mut old_cart.chr);

  // copy the new emulator
  *self = other;

  // the new emulator is missing prg and chr; we take the temp ones
  let new_cart = self.get_bus().cart.as_mut();
  new_cart.prg = prg;
  // we only copy the temp chr if it is not chr ram, as that has already been deserialized by serde
  if !new_cart.header.uses_chr_ram {
    new_cart.chr = chr;
  }

  // When loading a savestate, we have to clone again the new cart, 
  // and re-wire it to the relative devices.
  let ppu_cart = self.cpu.bus.cart.clone();
  self.get_ppu().wire_cart(ppu_cart);
  let apu_cart = self.cpu.bus.cart.clone();
  self.get_apu().wire_cart(apu_cart);
}
```