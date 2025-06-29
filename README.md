# experimental
Experimental projects sliced out of my personal (private) game engine project. See `Tools/header/Tools`.

This is generally using MSVC latest or clang trunk. Note that building the Benchmark project requires google benchmark to be built statically.

--------
### ranges/remove.hpp ranges/erase.hpp
Provides unstable `remove`, `remove_if`, and `erase`, `erase_if` operations.

### array.hpp
Allocator-aware array supporting uninitialized storage. Can be used to implement other contaniers.

### BitEnum.hpp
A strong type for enum classes with bitfield operations, similar to adding global overloads that check for a magic enum value or type trait class.

```
enum class Test { None = 0, A = 0b0001, B = 0b0010, C = 0b0100 };
BitEnum<Test> test{Test::A, Test::C};
if (test.all_of(Test::A, Test::B) {}
```

### charconv.hpp
A wrapper for `std::to_chars` so it's less boilerplate if you're doing a lot of them.

```
ToCharsConverter<std::int32_t> converter;
assert(converter(12) == "12");
assert(converter(-500) == "-500");
// Compiles as long as the type can fit in the internal buffer.
assert(converter(4294967295u) == "4294967295");
```

### enum_map.hpp
A contiguous array indexed by enum values. Enforces a value to be specified for the array.

```
enum class E1 { One = 1, /*Two removed*/ Three = 3, Four = 4 };

enum_map<E1, int> map(
  enum_arg<E1::One>(1),
  enum_arg<E1::Three>(3),
  enum_arg<E1::Four>(400))
  
map[E1::One] = 77;
for (const auto& [key, value] : map) {}
for (E1 key : map.keys()) {}
for (int i : map.values()) {}
// etc...
  
constexpr enum_map<E1, int> map2(
  enum_default(7), // explicitly set defaults.
  enum_arg<E1::Three>(3));
assert(map2[E1::One] == 7);
assert(map2[E1::Three] == 3)
assert(map2[E1::Four] == 7);

// Doesn't compile, E1::Three missing.
enum_map<E1, int> map3(enum_arg<E1::Four>(4), enum_arg<E1::One>(1));
// Doesn't compile, E1::Three specified twice.
enum_map<E1, int> map3(
  enum_arg<E1::One>(1),
  enum_arg<E1::Three>(3),
  enum_arg<E1::Three>(3));
```

### iterator.hpp
Based on `boost/iterator_facade`, but `constexpr`/`noexcept` friendly. Makes some safety features that may have overhead cost opt-in where it is not possible to detect for certain they are needed.

```
template <typename T>
class ptr_iterator_t : public iterator_t<ptr_iterator_t<T>, T> {
	T* t_ = nullptr;

	friend iterator_accessor;
	constexpr T*& get_index() noexcept { return t_; }
	CTP_NONCONST_ITERATOR_INTEROP_CONSTRUCTOR_HEAD_DEFAULT(ptr_iterator_t, T)
		: t_{other.t_}
	{}
	constexpr auto peek() noexcept { return t_; }
public:
	constexpr ptr_iterator_t() noexcept = default;
	constexpr ptr_iterator_t(T* t) noexcept : t_{t} {}
};
```

### scope.hpp
Implementation of `<scope>` from `std::experimental`. Similar to `gsl::finally`.

### small_string.hpp
Constexpr friendly string type with customizable small storage optimization.

```
// Can fit 77 characters in small mode, but will allocate to the heap at 78+ characters.
small_string<77> not_null_terminated_str;
// Can fit 77 characters plus the null terminator in small mode,
// but will allocate to the heap at 78+ characters.
small_zstring<77> null_terminated_str;

// Can hold at most 77 characters.
fixed_string<77> not_null_terminated_fixed_str;
// Can hold at most 77 characters plus the null terminator.
fixed_zstring<77> null_terminated_fixed_str;
```

### small_vector.hpp
Constexpr friendly vector type with customizable small storage optimization.

```
small_vector<int, 7> small; // If used with 7 or fewer elements, stays on the stack.
fixed_vector<int, 7> small; // Can hold at most 7 elements.
```

### StrongType.hpp
Very experimental strong type class. Designed to allow opting in to different behaviours.

### type_traits.hpp
A small collection of helpful type traits, includeing the `is_detected` pattern from `std/experimental`.

### Vectorizer.hpp
Makes it easy to iterate on a vector-like class (e.g. `struct Colour { int r, g, b; };`). Similar in use to the anonymous-struct-in-anonymous-union approach, but avoids UB in constant expressions so they can be used in them.

```
struct Vec3 : Vectorizer<Vec3, int, 3> {
	int x, y, z;
	using Vectorizer::Vectorizer;
	CTP_MAKE_VECTORIZER_LIST(Vec3, x, y, z)
};

constexpr Vec3 vec{1,2,3};
assert(vec.x == 1);
assert(vec.y == 2);
assert(vec.z == 3);

for (int i : vec) {}
```

### zstring_view.hpp
null-terminated string_view, providing .c_str(). Use for interop with C-style functions.
