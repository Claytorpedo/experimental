# experimental
Experimental projects sliced out of my personal (private) game engine project. See `Tools/header/Tools`.

This is generally using MSVC latest or clang trunk. Note that building the Benchmark project requires google benchmark to be built statically.

--------
### ranges/remove.hpp ranges/erase.hpp
Provides unstable `remove`, `remove_if`, and `erase`, `erase_if` operations.

### BitEnum.hpp
A strong type for enum classes with bitfield operations, similar to adding global overloads that check for a magic enum value or type trait class.

### charconv.hpp
A wrapper for `std::to_chars` so it's less boilerplate if you're doing a lot of them.

### iterator.hpp
Based on `boost/iterator_facade`, but `constexpr`/`noexcept` friendly. Makes some safety features that may have overhead cost opt-in where it is not possible to detect for certain they are needed.

### scope.hpp
Implementation of `<scope>` from `std::experimental`. Similar to `gsl::finally`.

### small_string.hpp
Constexpr friendly string type with customizable small storage optimization.

### small_vector.hpp
Constexpr friendly vector type with customizable small storage optimization.

### scope.hpp
Implementation of `<scope>` from `std::experimental`. Similar to `gsl::finally`.

### StrongType.hpp
Very experimental strong type class. Designed to allow opting in to different behaviours.

### type_traits.hpp
A small collection of helpful type traits, includeing the `is_detected` pattern from `std/experimental`.

### Vectorizer.hpp
Makes it easy to iterate on a vector-like class (e.g. `struct Colour { int r, g, b; };`). Similar in use to the anonymous-struct-in-anonymous-namespace approach, but avoids UB in constant expressions so they can be used in them.
