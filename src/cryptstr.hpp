#pragma once

// global includes
#include <utility>
#include <string>
#include <stdexcept>

// simple predefs
namespace predef {
/* predef types
    -1 equals error
*/
enum class compiler_t : unsigned short {
    msvc = 0,
    gcc = 1,
    clang = 2,

    none = static_cast<unsigned short>(-1)
};
}

/* identify compilers */
#if defined(__clang__) && !defined(CS_COMPILER)
#   define CS_COMPILER predef::compiler_t::clang
#   define CS_CLANG
#endif
#if defined(__GNUC__) && !defined(CS_COMPILER)
#   define CS_COMPILER predef::compiler_t::gcc
#   define CS_GCC
#endif
#if defined(_MSC_VER) && !defined(CS_COMPILER)
#   define CS_COMPILER predef::compiler_t::msvc
#   define CS_MSVC
#endif


/* default predef defines
    none equals error
*/
#ifndef CS_COMPILER
#   define CS_COMPILER predef::compiler_t::none
#endif

/* assert for valid PREDEF macro */
static_assert( CS_COMPILER != predef::compiler_t::none, "invalid compiler");

/**
    CS_NO_OPTIMIZE

    To assure memset/memzero calls are not stripped by optimization, we should turn off everything we can.
    CS_NO_OPTIMIZE should be used for routines that have to execute as specified.

    Support for: GCC, CLANG, MSVC and all compatible compilers

    List of relevant GCC function attributes:
        no_icf: prevents a function from being merged with another semantically equal function
        no_reorder: no-reordering of functions specified with this attribute
        noclone: no cloning of functions for specialized cases
        noipa: no intra-procedural optimizations of caller and callee
            implies no_icf, no_clone and noinline
        optimize(-O0): specify optimization level for the specified routine

    List of relevant CLANG function attributes:
        optnone: disable all optimization for the specified function

    List of relevant MSVC function attributes:
        #pragma optimize ("", off)
            ...
        #pragma optimize ("", on)
*/

/* set optimization function attributes according to set compiler */
#if defined(CS_CLANG)
#   define CS_NO_OPTIMIZE __attribute__((optnone))
#elif defined(CS_GCC)
#   define CS_NO_OPTIMIZE __attribute__((no_icf)) __attribute__((no_reorder)) __attribute__((noclone)) \
        __attribute__((noipa)) __attribute__((optimize("-O0")))
#elif defined(CS_MSVC)
#   define CS_NO_OPTIMIZE
#endif

#define CS_INLINE


/* volatile memory routines
    you should not call them in tight loops
*/
namespace cs {

/* memset
    set num bytes of ptr to value
*/
#ifdef CS_MSVC
#   pragma optimize("", off)
#endif
static volatile void* CS_NO_OPTIMIZE memset( void* ptr, int value, size_t num )  {
    volatile char* char_ptr = static_cast<volatile char*>(ptr);
    while( num-- > 0) {
        *char_ptr = static_cast<char>( value );
        ++char_ptr;
    }
    return ptr;
}
#ifdef CS_MSVC
#   pragma optimize("", on)
#endif

/* memzero
    set num bytes of ptr to zero
*/
#ifdef CS_MSVC
#   pragma optimize("", off)
#endif
static void CS_NO_OPTIMIZE memzero( void* ptr, size_t num ) {
    volatile char* char_ptr = static_cast<volatile char*>(ptr);
    while( num-- > 0) {
        *char_ptr = 0;
        ++char_ptr;
    }
}
#ifdef CS_MSVC
#   pragma optimize("", on)
#endif

}

/* securememory zero classes
*/
namespace cs {

/*
    Zeros all bytes of *this on destructor call. Parent type needs to be
    supplied to zero the correct size.

        struct MyType : sm::zero<MyType> {
            ...
        };
*/
template < class T >
struct zero {
    typedef T parent_type;

#ifdef CS_MSVC
#   pragma optimize("", off)
#endif
    CS_NO_OPTIMIZE ~zero() {
        memzero( static_cast<void*>(this), sizeof(parent_type));
    }
#ifdef CS_MSVC
#   pragma optimize("", on)
#endif

    /* member set_zero call
     *  should be used to zero dynamic memory before ~zero is called.
    */
#ifdef CS_MSVC
#   pragma optimize("", off)
#endif
    void CS_NO_OPTIMIZE set_zero(void* ptr, size_t length) {
        memzero( static_cast<void*>(ptr), length);
    }
    template < class U >
    void CS_NO_OPTIMIZE set_zero(U* ptr) {
        zero(ptr, sizeof(U));
    }
#ifdef CS_MSVC
#   pragma optimize("", on)
#endif
};

/*
    Zeros all bytes in deallocate/destroy method calls. Base should be a
    std::allocator or compatible class.
*/
template < class Base >
struct zero_plugin_allocator : Base {
    typedef Base base_type;
    typedef typename Base::value_type value_type;
    typedef typename Base::pointer pointer;

#ifdef CS_MSVC
#   pragma optimize("", off)
#endif
    /*  destroy objects
    */
    void CS_NO_OPTIMIZE destroy( pointer p ) {
        memzero( static_cast<void*>(p), sizeof(value_type));
        Base::destroy(p);
    }
    template < class U >
    void CS_NO_OPTIMIZE destroy( U* p ) {
        memzero( static_cast<void*>(p), sizeof(U));
        Base::template destroy<U>(p);
    }

    /*  deallocate objects
    */
    void CS_NO_OPTIMIZE deallocate( pointer p, size_t n ) {
        memzero(static_cast<void*>(p), sizeof(value_type) * n);
        Base::deallocate(p, n);
    }
#ifdef CS_MSVC
#   pragma optimize("", on)
#endif
};

/*  Standalone zero-allocator
*/
#ifdef CS_STL_INTEROP
template < class T >
struct std_zero_allocator : zero_plugin_allocator< std::allocator<T> > {};
#endif

// A compile-time string class. All operators and routines are constexpr.
// Checking for ranges/content can happen at compile-time.
template < class CharType, size_t N >
struct ctstr {
    typedef CharType char_type;
    static constexpr size_t ct_size = N;

    // construct an empty ctstr instance
    constexpr ctstr() noexcept : str{}, len(N) {}

    // construct a ctstr from a char array
    template < size_t Y >
    constexpr ctstr(const char_type (&_str)[Y] ) noexcept : str{}, len(Y) {
        static_assert(N == Y, "invalid sizes");
        for ( size_t i = 0; N > i; ++i ) {
           str[i] = _str[i];
        }
    }

    // construct a ctstr instance from another one, allowing nested processing of ctstr instances
    template < size_t Y >
    constexpr ctstr(const ctstr<char_type,Y>& _other ) noexcept : str{}, len(Y) {
        static_assert (N == Y, "invalid sizes");
        for ( size_t i = 0; N > i; ++i ) {
            str[i] = _other.str[i];
        }
    }

    // return pointer to string elements
    constexpr const char_type* const get() const noexcept { return &str[0]; }

    // statitcally range-checked accessor
    constexpr const char_type operator [] ( size_t _index ) const {
        return ( _index < len ) ? str[_index] : throw std::out_of_range("index out of range");
    }

    // return size of string in elements
    constexpr size_t size() const noexcept {
        return len;
    }

    // compile-time comparison with other strings
    template < size_t Y >
    constexpr bool operator == ( const char (&_str)[Y] ) const noexcept {
        if ( Y != N )
            return false;
        return equal(_str);
    }
    template < size_t Y >
    constexpr bool operator == ( const ctstr<char_type,Y> _other ) const noexcept {
        if ( Y != N )
            return false;
        return equal(_other.str);
    }
    template < size_t Y >
    constexpr bool operator != ( const char (&_str)[Y] ) const noexcept {
        if ( Y != N )
            return true;
        return !equal(_str);
    }
    template < size_t Y >
    constexpr bool operator != ( const ctstr<char_type,Y> _other ) const noexcept {
        if ( Y != N )
            return true;
        return !equal(_other.str);
    }

private:
    // compare with another string
    template < size_t Y >
    constexpr bool equal(const char (&_str)[Y] ) const noexcept {
        for ( size_t i = 0; N > i; ++i ) {
            if ( str[i] != _str[i] ) {
                return false;
            }
        }
        return true;
    }

public:
    char_type str[N];
    const size_t len;
};
// make helpers
template < class CharType, size_t N >
constexpr ctstr<CharType,N> make_ctstr(const CharType (&_str)[N] ) noexcept {
    return ctstr<CharType,N>(_str);
}
template < class CharType, size_t N >
constexpr ctstr<CharType,N> make_ctstr(const ctstr<CharType,N>& _other ) noexcept {
    return ctstr<CharType,N>(_other);
}

// applies a functor to every char of a ctstr and returns another one
// functor is of the form CharType functor(CharType*,size_t len, size_t index)
template < class CharType, size_t N, class Functor >
constexpr ctstr<CharType,N> transform(const ctstr<CharType,N>& _other, Functor _functor) noexcept {
    CharType values[N] = {};
    for ( size_t i = 0; N > i; ++i ) {
        values[i] = _functor(_other.get(),N,i);
    }
    return ctstr<CharType,N>(values);
}
template < class CharType, size_t N, class Functor >
constexpr ctstr<CharType,N> transform(const CharType (&_str)[N], Functor _functor) noexcept {
    CharType values[N] = {};
    for ( size_t i = 0; N > i; ++i ) {
        values[i] = _functor(_str,N,i);
    }
    return ctstr<CharType,N>(values);
}


// construct a new ctstr instance with the specified Functor transformation
template < class CharType, size_t N, class Functor >
constexpr ctstr<CharType,N> construct_transform(const ctstr<CharType,N>& _other, Functor _functor ) noexcept {
    CharType values[N] = {};
    for ( size_t i = 0; N > i; ++i ) {
        values[i] = _functor(_other.get(),N,i);
    }
    return ctstr<CharType,N>(values);
}
template < class CharType, size_t N, class Functor >
constexpr ctstr<CharType,N> construct_transform(const CharType (&_str)[N], Functor _functor ) noexcept {
    CharType values[N] = {};
    for ( size_t i = 0; N > i; ++i ) {
        values[i] = _functor(_str,N,i);
    }
    return ctstr<CharType,N>(values);
}

// A standard XOR functor for obfuscation
template < int Key >
struct xor_functor {
    template < class CharType >
    constexpr CharType operator () ( const CharType* str, size_t len, size_t index ) noexcept {
        (void)len;
        return str[index] ^ Key;
    }
};

// A view into an obfuscated_string instance.
// Automatically zeroes all bytes on destruction of the object.
// strview can only be passed by reference or pointer.
template < class CharType >
struct strview : std::basic_string<CharType>, zero<strview<CharType>> {
    typedef CharType char_type;

    // Disallowed Behavior

    // copying strview instances should not be possible. every read in a new scope should
    // be a fresh strview instance.
    strview( const strview<char_type>& ) = delete;
    strview& operator = ( const strview<char_type>& ) = delete;

    // no construction from std::basic_string
    // only viable source is cryptstr
    strview(const std::basic_string<char_type>&) = delete;
    strview& operator = ( const std::basic_string<char_type>&) = delete;

    // no implicit conversion to std::basic_string
    operator std::basic_string<CharType> () const = delete;

    // no move assignment allowed
    strview& operator = ( strview&& ) = delete;

    // Allowed Behavior

    // only allowed constructor is from a ctstr instance
    template < size_t N >
    strview( const ctstr<char_type,N>& _str) : std::basic_string<char_type>(_str.get(), _str.size()) {}

    // default move behavior
    strview(strview&&) = default;

    // duplicate std::string operator and construction behavior
    ~strview() {
        memzero((void*)this->c_str(), this->size() * sizeof(CharType));
    }
};

// A obfuscated string instance which can only be read via a
// strview instance.
template < class CharType, size_t N, class Functor >
struct cryptstr {
    typedef CharType char_type;
    typedef Functor functor_type;

    // construct a cryptstr instance from another one, allowing nested processing of ctstr instances
    // \param _functor Functor object that was used to transform _other
    // \param _other crypted compile-time string instance
    template < size_t Y >
    constexpr cryptstr(functor_type _functor, const ctstr<char_type,Y>& _other ) noexcept : data(_other), functor(_functor) {
        static_assert(Y == N, "invalid sizes");
    }

    // returns the string's size
    constexpr size_t size() const { return N; }

    // returns the internal ctstr
    constexpr const ctstr<char_type,N>& ct() const {
        return data;
    }

    // returns an unobfuscated string instance
    strview<CharType> decrypt() const {
        auto raw = transform(data, functor);
        strview<CharType> view(raw);
        memzero( (void*)const_cast<CharType*>(raw.get()), sizeof(CharType) * N);
        return view;
    }

public:
    Functor functor;
    const ctstr<char_type,N> data;
};
// constructs a cryptstr from an uncrypted source
template < class CharType, size_t N, class Functor >
constexpr cryptstr<CharType,N,Functor> crypt(Functor _functor, const ctstr<CharType,N>& _other) {
    return cryptstr<CharType, N, Functor>(_functor, transform(_other, _functor) );
}
template < class CharType, size_t N, class Functor >
constexpr cryptstr<CharType,N,Functor> crypt(Functor _functor, const CharType (&_str)[N] ) {
    return cryptstr<CharType,N,Functor>(_functor, transform(_str, _functor));
}

}
