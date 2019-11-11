#include <iostream>
#include <cryptstr.hpp>

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;

    // simple XOR by Key functor object that is used to obfuscate strings during
    // compile-time.
    constexpr cs::xor_functor<0x1337> functor;

    // cs::crypt() constructs a compile-time encrypted string instance of the type cs::cryptstr.
    // The cryptstr type varies on string-length and functor type.
    constexpr auto crypted1 = cs::crypt(functor, "FIRST CRYPTED STRING");
    constexpr auto crypted2 = cs::crypt(functor, "SECOND CRYPTED STRING");

    // decrypt() returns the de-obfuscated string during runtime.
    // de-obfuscated strings are of the type strview, which cannot be copied or moved from the scope.
    // strview can only be passed by reference or pointer. On destruction of strview, a special unoptimized memset()
    // routine garuantees that all bytes are set to null.
    const auto dec1 = crypted1.decrypt();
    const auto dec2 = crypted2.decrypt();

    // print the de-obfuscated strings
    std::cout << dec1 << std::endl;
    std::cout << dec2 << std::endl;

    // cryptstr instances hold data in a ctstr, which is a compile-time container
    // for strings. ctstr supports various range-checking methods and accessors.
    // you can compare strings during compile time.
    static_assert(crypted1.ct() != crypted2.ct(), "strings should not be equal");

    // construct more compile-time strings, but unencrypted ones.
    constexpr auto plain1 = cs::make_ctstr("HELLO DOG");
    constexpr auto plain2 = cs::make_ctstr("HELLO CAT");

    // the following does not compile since compile-time string accesses are always range-protected.
    // constexpr auto char1 = plain1[42]; // GCC says, it's not a const expression
    constexpr auto char2 = plain2[2]; // valid access since it's in range

    // you can compare substrings and characters via the index operator
    static_assert( (plain1[0] == plain2[0]) && (plain1[1] == plain2[1]) && (plain1[2] == plain2[2]), "must be equal" );

    // example for a full-string comparison
    constexpr auto equal1 = cs::make_ctstr("HELLO");
    constexpr auto equal2 = cs::make_ctstr("HELLO");

    static_assert(equal1 == equal2, "must be equal");

    return 0;
}
