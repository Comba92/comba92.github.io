+++
date = '2025-03-28T18:18:52+01:00'
draft = true
description = 'CodingHard'
linkTitle = 'Have we made programming too hard??'
title = 'Have we made programming too hard??'
summary = 'We examine an optimization process, and reflect about the cryptic concepts that makes writing fast software a difficult problem'
author = 'Comba92'

tags = ['coding']
keywords = ['coding', 'programming', 'software', 'programming concepts', 'optimization', 'software optimization']
+++
Today I stumbled upon this video. 
{{< youtube hp4Nlf8IRgs >}}


Now, while watching it, I found it fascintaing, but for different reasons of those you might expect.
I was not fascinated by the optimizations the video presented. I did know about all of them.
The author (i will refer to him as Berna, from now on) gives the impression of a beginner programmer, or probably just a little unexperienced (he points out he is a Python programmer, after all, hehe).
However, he is very eager to learn and wants understand what is happening under the hood, so I admire him for this. 
What is *incredibly fascinating* about this video is, that you get to see how an unexperienced user tackles the optimization process of their project, starting from blissful ignorance to a growingly deeper undestanding of the inner workings of the cryptic and alien technology of computers.

So, I first of all thank the author of the video for sharing his experience without any fear being judged for his errors and mistakes. He does quite a few during his ramblings and brainstorming, trying to get his program faster. There are times where he makes something better, but introduces more problems. Though, he continuosly learns new concepts in the process. In the end, he ends up fixing almost all of his omissions!

It is rare to see such a positive and craved for knowledge spirit. I am pretty sure, most people starting out would have felt underwhelmed and frustrated, and would not have investigated further.
This video lead me to think.

*HAVE WE MADE PROGRAMMING SOFTWARE TOO FREAKING HARD?*

Just think about it: you have just started programming. You end up coding a big project, just like the guy in the video did. The project ends up beign incredibly slow and unoptimized. How many people would have dug deeper, trying to improve it and learn all that domain specific knowledge? Trust me, not too many.
This is basically what is happening right now, in the software development world. Think about web developers, or even worse, AI bros. A lot of frivolous, aloof and detached people, who enters the IT world out of greed and nonchalance, instead of passion and joyful interest.

We need more people with this enthusiasm and intelligence. The video's author got from me a like, a subscription and some free advertisement on my blog.

Now, the optimizations he covers aren't all that difficult, but they made me want to write a blog into it, going into more detail about them, and giving more awarness to the average programmer.

# A word about benchmarking, profiling and flame graphs
-- Talk a little about that...


# The optimizations
## Scopes and data construction
Berna is initializing a complex object (the rng device) inside each call of a very often used function (the rng getter). This is a big performance hit.
What happens is, that for every function call, it first initializes the random device, which requires complex logic and [costly memory allocation](https://en.wikipedia.org/wiki/Memory_management#Efficiency). It then produces a random number, then immediately destroys the random device, which requires memory deallocation. The initial object creation was all for nothing!

This is usually a problem of:
1. understanding how an object is created, and how much overhead its creation requires;
2. understanding scopes, and the cost of allocations.

The random device only needs to be constructed ONCE. So, we should store it somewhere easily accessible, and reuse it for every rng getter call.
Another thing to be aware of is that, in languages like C++, when an object goes out of scope, its destructor is also called, which in some cases requires complex logic and deallocations, which might are costly too.
Turns out that the repeated rng device creations and deallocation were a big performance bottlenecks. Memory management is more costly than you think.

[Scopes](https://en.wikipedia.org/wiki/Scope_(computer_science)) are usually a difficult topic for beginners, for whatever reason. Whenever you declare a variable, you should be aware that it will be valid only for the duration of the scope it is defined in!! Often, it is useful to define an object in an upper scope, so that it will be allocated fewer times! This is an incredibly important topic you should master.

## Static, const, and dynamic data
You should differentiate between non changing values and changing values (static and dynamic).
Now, namings changes based on the language, but for C++:
- consts values are values which CAN'T change
- statics are values only initialised ONCE (only allocated once!)
- dynamics/automatics are values which CAN change.

In Rust, static values would be immutables, and dynamic values would be mutables.
By decorating variables with consts and static keywords, you can tell the compiler on how to deal with them, and saving useless and costly allocations.

## Collections and asymptotic complexity
Berna's using its data structures wrong! It pushed some values in a list, and then popped the values from its head. If you knew how a list works and a little bit of asymptotic complexity, you would know that popping from a list's head is a linear ( O(n) ) operation. That takes a lot of time!! You should pop from the back of the list, which is a constant ( O(1) ) operation!! Turns out, this simple change gives Berna a big performance win.

You should know A LOT about data structures!!
Now, don't get scared. There are a lot of weird exotic data structures, but turns out you only need to know the 2 main to be a good programmer. 
Lists/Vectors/DynamicArrays (i will call them Lists) and HashMaps/HashTables/Dictionaries (i will call them Maps).
(let's thank our dear IT scientists friends, for never agreeing on the namings of things)

Let's see first what's the deal with asymptotic complexity, and then analyze the most useful data structures.

### Asymptotic complexity
In short, asymptotic complexity is a way of measuring *how fast a function is on the worst case based how many entries it has to be performed*, alas, how fast it is depending on its input.
Data structures contains a set of entries of size n. If we want to iterate through all the entries, it will take n operations. The complexity is O(n).
What is the meaning of the O? It means [UPPER BOUND](https://en.wikipedia.org/wiki/Big_O_notation), meaning, we need **at maximum** n operations for the function to complete. The "at maximum" is the most important thing here.   
Let's say we want to search for some entry x in a list. We'll have to iterate through all the entries, and stop when we find x. The complexity is still O(n), because we might have to iterate through all n entries (if x turns out to be the last), thus making n operations in total.

Big O complexities can usually be categorized as only these, from slowest to faster: 
- O(1) - constant time, is an operation which doesn't depend on its input; it always take the same time. For example, taking the last element of a list.
- O(log n) - logarithmic time, usually binary search, or a search in a sorted tree structure
- O(n) - linear time, usually a search in an unsorted list or any kind of iterations.
- O(n log n) - semilinear time, usually sorting.
- O(n^2) - quadratic time - you mostly want to get faster than this, but in some rare cases you can't get any better

This video explains it better than me, it is entertaining to watch but probably not very beginner friendly:
https://www.youtube.com/watch?v=7VHG6Y2QmtM

### Lists
Lists are implemented as [dynamic arrays](https://en.wikipedia.org/wiki/Dynamic_array). They have some sort of allocated capacity, and a count of how many items they contain (often called size or length).
Access to any entry is O(1) given the index.
A push to the **back** of the list is O(1). However, if the size exceeds the capacity, a new reallocation will take place.
Lists have the same properties as [stacks](https://en.wikipedia.org/wiki/Stack_(abstract_data_type)) (they are the same things)

Inserting or removing is O(n), as it requires moving all other entries to the right for an insert or to the left for a remove.

Lists aren't the best structures for searching and sorting.
Search is O(n), and sorting is O(n log n).

### Maps
Maps are a little more complex [to implement](https://en.wikipedia.org/wiki/Hash_table), but under the hood, they are simply dynamic arrays of key-value pairs, and keys are converted to indexes with an [hash function](https://en.wikipedia.org/wiki/Hash_function).
Maps offers O(1) (amortized) insertions, removal, and access. This might be paradoxical, if maps are so fast, why even use lists in the first place??

That's because there is the problem of [key collisions](https://en.wikipedia.org/wiki/Hash_table#Collision_resolution). I won't delve into the details here, but just know that maps have a bigger overhead than lists, as they have to compute an hash and deal with the collisions. That amortized O(1) just means that it is constant time on average! If there are too many collisions, it degrades to O(n)!!

Anyway, maps are perfect when you need fast searches. Search is done by keys, and are O(1) amortized.
Maps are usually unordered, and sorting is not possible (because of the hashed keys).
You can implement sorted maps by using tree structures though, and most languages provide them.

HashSets/Sets are basically the same thing, except they only have a key and no value.

### Deques, LinkedLists, Trees and PriorityQueues
Deques are basically enhanched lists, with added O(1) head insertion and removal. 

Linked lists are usually teached in academics, and have its uses, but trust me, you don't want to use it as a list replacement. Why? [Cache locality](https://en.wikipedia.org/wiki/Linked_list)...

Trees are good to know. They are self-sorting structures, meaning that every insertion will be automatically sorted. Their basic operations usually are O(log n).

PriorityQueues/BinaryHeaps are used when you want an unsorted list but want to extract entries like the minimal or maximal in O(1) time.

### Conclusion
Master when and how to use lists or maps, and you should be good to go.
I reccomend this page: 
https://doc.rust-lang.org/std/collections/

## Always do less work
Berna's was deserializing a static JSON string to an object in each iteration. The deserializing is usually very fast, but here you're doing more work which can be easily avoided.
Just deserialize the string once at the beginning!! No deserialization in each iteration means less work to do.

Always do less work!! This should be obvious.
https://en.wikipedia.org/wiki/Redundant_code

## Duplication, copy-pasting, refactoring and abstracting
[Don't Repeat yourself!](https://en.wikipedia.org/wiki/Don%27t_repeat_yourself)
https://en.wikipedia.org/wiki/Duplicate_code
https://en.wikipedia.org/wiki/Rule_of_three_(computer_programming)
https://en.wikipedia.org/wiki/Code_refactoring
https://en.wikipedia.org/wiki/Abstraction_principle_(computer_programming)
https://en.wikipedia.org/wiki/Copy-and-paste_programming
https://en.wikipedia.org/wiki/Anti-pattern
https://en.wikipedia.org/wiki/Code_smell
https://en.wikipedia.org/wiki/Design_smell
https://en.wikipedia.org/wiki/Software_rot

## Passing by value/copy and by reference
Berna's was allocating a new vector of a known capacity for each function call. The way he fixes is to pass a single vector by reference. This gives massive performance boost. Again, the cost of allocations.

## Data compression
[Data compression](https://en.wikipedia.org/wiki/Data_compression) is a complex topic, and it is very context dependant. You won't always get beneficial memory savings, especially with a general compression algorithm. It is always best to examine carefully the problem at hand, and manually verify if any compression algorithm is actually useful. Also keep in mind that encoding and decoding might have a considerable overhead. You might also be interested in a custom, ad-hoc compression algorithm for your use case. Remember: a specializaed algorithm is always better than a general one.

In Berna's case, compressing the data packets saves SOME memory, but it is definetely not the performance bottleneck. It is useful in its case, but not really that important. He uses [gzip](https://en.wikipedia.org/wiki/Gzip), which is a popular and powerful compression alg for data trasmission.

## Multithreading and data sharing
Berna's was getting a runtime segmentation fault at seeminglessly random times. This was because he was sharing the rng device through multiple threads, and without any locking, he was causing a race condition. The way he fixes it is by decorating the rng device with the thread_local keyword, which i didn't even knew existed. Basically, he's making each thread have a new instance of the rng device. He notes that this decreases performance a bit.


## Batching
Berna's realizes the problem of batching in two occasions:
the first is with the rng getter. The rng getter has to do some work to spit out a random value, it is not instantaneus. So, asking for a random value continuosly in the middle of other work is WAY less efficient than asking for a big number of random values all at once, and only THEN doing the work. This works, because, [cache locality](https://en.wikipedia.org/wiki/Locality_of_reference).

This concept is intuitive if you think about the real world, too. Example: you have a thousand of paper to copmile. After you compile one, you have to get to your boss office to deliver the paper. Would you rather compile one paper, then deliver it to your boss office (so, waste more time walking than compiling the papers!!), and continue until you're done with all paper, or would you rather compile all the paper first, and then deliver it all at once??

## Printing, systemcalls, and the cost of abstractions
The second occasion is with the console logging.

## I/O and networking
Databases are slower than Rest APIs...

## Other cool optimization videos and channels reccomendation
- https://www.youtube.com/watch?v=DMQ_HcNSOAI
- https://www.youtube.com/watch?v=5rb0vvJ7NCY
- https://www.youtube.com/watch?v=IroPQ150F6c
- https://www.youtube.com/watch?v=40JzyaOYJeY
- https://www.youtube.com/watch?v=R-DEp62qDeE
- https://www.youtube.com/@nicbarkeragain/videos
