+++
date = '2025-02-11T16:11:37+01:00'
draft = false
description = 'NesRefcell'
linkTitle = 'Rust Refcells: how I (un)safely got rid of them'
title = 'Rust Refcells: how I (un)safely got rid of them in my project'
summary = 'Refcells have a big overhead, but you can easily get rid of them based on some hard restrictions.'
[params]
  author = 'Comba92'
tags = ['rust', 'coding', 'nes', 'emulation']
keywords = ['nes', 'emulation', 'coding', 'implementation', 'rust', 'refcell', 'interior mutability', 'unsafe', 'mutability', 'pointers', 'raw pointers']
aliases = ['/page/posts/nes_refcell']
+++
Rust enforces immutability as the default. While it is a sane habit to learn, very often we need interal mutation of objects, and we also want to have these objects referenced in multiple places. Having a multiple mutable reference to an object is not allowed in Rust. Only immutable refenceres can be multiple.

While developing my NES emulator, found myself in this situation. I have a Cartridge object, which holds the ROM data. This object should be used and MUTATED by two other objects: the CPU and PPU.
The correct solution for mutable and multiple ownership in Rust would be this:
1. wrap the object inside a RefCell, which gives interior mutability. The object can now be mutated by taking a mutable borrow, which is checked at runtime. At most one borrow can be active at a time; if a second borrow is requested, the program will crash.
2. wrap the RefCell inside an Rc, which gives immutable multiple ownership, so that the object can be shared safely in multible places.

This RustBook chapter explains it more clearly: https://doc.rust-lang.org/book/ch15-05-interior-mutability.html

The problem with refcells is that they have to check at runtime how many borrows they are sharing, as there can only be a single mutable borrow. This seems like a small overhead, but when we are accessing the cartridge ROM data millions of time per frame, we are summing up a lot of overhead penalities which will slow the emulator down.

Although I didn't really experience any kind of slowdown in my emulator by using with refcells, and I don't really know how much overhead there might be, I still wanted to try getting rid of them. Call it a challenge. Let's examine a few solutions.

The first one is to completely rethink our software design. This is not really an option, as I have a basically complete emulator.
<br>
The second was to only redesign my cartridge implementation. This came up to me while writing this article. We could just split up the cartridge in two objects: one for the CPU, which can only access PRG-ROM, PRG-RAM and the mapper registers, one for the PPU, which can only access CHR and Nametable VRAM. But there would still be the problem of sharing the mapper registers state, as only CPU can write to the registers. This is a possible solution which could have been explored more.


## The solution 
The third solution, which I did end up using, is resort to *unsafe* and *raw* pointers. You heard me right. Rust's guys are probably going to lynch me...
<br>
The cool thing is that, **in my case, using pointers is totally 100% SAFE!!**[^1]. How?

#### Pointer safety conditions
1. I know for sure the pointer to the cartridge will NEVER be freed; it will live as long the emulator lives. This means we can't have a deallocated memory dereference.
2. I know for sure I am never changing the pointed value of the pointer: it will always point to the same cartridge object. This means we can't have dangling references.
3. There will always be only one consumer mutating the cartridge object. Whenever i was mutating it, i always got a borrow and immediately released it. The refcell was basically useless: there is no need for checking borrows at runtime.

[^1]: actually, there is only one case where these conditions don't hold, but we will have a look at it later.

Now, with this idea in mind, the naive thing to do was to get a pointer to the cartridge object, and change every call to its methods to pointer dereferences, and wrap them in an unsafe block. But, do we really have the time and energy to do that? My emulator was basically finished at this time, and this meant i would have to go trough a big codebase changing stuff with the chance of breaking something.
We can be smarter. We are programmers, for god's sake.

The big brain solution here is to provide an interface to access the pointer similiar to the one provided by refcell.
So that we can easily do this:
```rust
// change this
refcell.borrow_mut().call_mutable_method();

// to this
pointer.get_mut().call_mutable_method();
```
No unsafe spreading across the code. No big code modification. Just a simple replace with ctrl+f.

### The code
Here is what i've come up with. An objects which safely wraps a pointer. It safely holds a reference to the data, and safely returns a reference to it.
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

Simple as. Just change all the borrow_mut() calls to as_mut() calls and we're done. Got totally rid of refcells with minimal effort!
We only had to use unsafe ONCE, to dereference the pointer inside the as_ref() and as_mut().

### Allocation
Now, we have to be extra careful with creating and destroying this SharedData object.

When making a new SharedData, we should allocate the inner object in the heap with a Box. THIS IS REALLY IMPORTANT. You don't want a pointer which points to a stack value, don't you? We then consume the box with [Box::into_raw()](https://doc.rust-lang.org/std/boxed/struct.Box.html#method.into_raw), and return a pointer to the allocated value.

### Copy
When we need a new reference to the pointed data, we can just clone the full SharedData object. Rust's derive macro provides us with an auto generated clone() method, which simply copies the pointer.

### Deallocation
When destroying a SharedData, it gets tricky though. We are dealing with pointers after all. 
<br>
If we don't manually deallocate the pointer, we will get a memory leak: the data will stay in the heap and we won't have any more access to it. Rust provides the [Drop trait](https://doc.rust-lang.org/std/ops/trait.Drop.html) to deal with manual destruction of objects, but we should be EXTREMELY careful with pointers deallocations.
It would be very naively intuitive to implement the Drop trait for SharedData. We CANNOT implement Drop on SharedData, however. If we did that, and we're in a situations where we cloned the same SharedData object multiple times, when a single SharedData gets dropped, all the others will become dangling pointers, and crash the program the moment you derefence them. UNSAFE!!
Here's why Rust doesn't want you to fiddle with pointers...

For my emulator, the Bus is the 'parent' owner of SharedData, which then passes cloned copies to the PPU and APU objects. As PPU and APU are also owned by the Bus in my emulator, i can simply reserve the job of deallocating the data to the Bus. When the bus gets dropped, the PPU and APU will be dropped too, and with them all the dangling references to SharedData.
For deallocating pointers the heap, [Box::from_raw()](https://doc.rust-lang.org/std/boxed/struct.Box.html#method.from_raw) should be used, then drop the boxed value.

```rust
impl Drop for Bus {
  fn drop(&mut self) {
    // This is needed, as we're manually managing a pointer to heap
    unsafe { drop(Box::from_raw(self.cart.0)) }
  }
}
```

## A little problem: loading savestates
I have a little problem. There is one excpetion where my [pointer safety conditions](#pointer-safety-conditions) aren't true. Whenever i *load savestates*.
When i save a state, i do not serialize the ROM data[^2]. So in the deserialize, there won't be any PRG-ROM nor CHR-ROM. Also, you can't really serialize pointers, as they will change in different runtimes executions.
I am missing some stuff when deserializing. We have to carefully bake into the new emulator context the cartridge data, and create new pointers accordingly. Luckily for us, we only two places where we use pointers in the whole emulator, so it is easily manageable. 
You can now clearly see how dangerous pointers can be.
I use [mem::take()](https://doc.rust-lang.org/std/mem/fn.take.html) here, as it comes in handy for taking out data from structs.

[^2]: if we want to be fussy, I only serialize CHR-RAM and PRG-RAM, if there are any.

```rust
pub fn load_ctx_from_emu(&mut self, other: Emulator) {
  // save prg and chr in temp values
  let old_cart = self.get_bus().cart.as_mut();
  let prg = core::mem::take(&mut old_cart.prg);
  let chr = core::mem::take(&mut old_cart.chr);
  let sram = core::mem::take(&mut old_cart.sram);

  // copy the new emulator.
  // the old one gets dropped, with all its data.
  *self = other;

  // the new emulator is missing prg and chr; we take the temp ones
  let new_cart = self.get_bus().cart.as_mut();
  new_cart.prg = prg;
  new_cart.sram = sram;
  // we only copy the temp chr if it is not chr ram, as that has already been deserialized by serde
  if !new_cart.header.uses_chr_ram {
    new_cart.chr = chr;
  }

  // When loading a savestate, we have to clone again the new cart, 
  // and re-wire it to the relative devices.
  let ppu_cart = self.get_bus().cart.clone();
  self.get_ppu().wire_cart(ppu_cart);
  let apu_cart = self.get_bus().cart.clone();
  self.get_apu().wire_cart(apu_cart);
}
```

## Conclusion
This is the kind of software engineering that really makes you enjoy coding. Be sure to check out [my Nes emulator](https://github.com/Comba92/nen-emulator).