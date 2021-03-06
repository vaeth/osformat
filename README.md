# osformat

A C++ library for a typesafe printf/sprintf based on << conversion

(C) Martin Väth (martin at mvath.de).
This project is distributed under the terms of the
GNU General Public License v2.
SPDX-License-Identifier: GPL-2.0-only

This is a typesafe `printf`/`fprintf`/`snprintf` type library for C++.
The typesafety is obtained by using the standard `ostream`'s `<<` conversions.
It adds features which would not be available if one would use a plain ostream:

1. It is usable with foreign translations when words need to be reordered.
   This also applies to the case when the provided arguments must be
   reordered or even repeated for certain languages.

2. It does not cluster global modifiers of ostreams. For instance, output of a
   float number in a certain notation over `std::cout` would require to set
   such global modifiers.

3. It provides a simple conversion to strings with a more convenient syntax
   than by using `std::ostringstream`.

The library's functionality covers essentially `std::ostream`,
`boost::format`, `absl::StrFormat`, and various independent implementations of
so-called __StringPrintf__ or typesafe C++ printf variants like
https://github.com/c42f/tinyformat or `src/eixTk/formated.h` from the
__eix__ project.
(See the section __History and Contributions`` for a list of related projects).

Note that if the dependency cost is acceptable, the author would recommend
to use instead `absl::StrFormat` (see https://abseil.io/docs/cpp/guides/format)
since the latter allows even compile-time checking of the arguments.

## Some Features by Examples

- `osformat::SayError("foo has value “%s”") % foo;`

  Similar to `std::cerr << "foo has value “" << foo << "“" << std::endl;`
  or like `fprintf(stderr, "foo has value “%s”\n", foo) && fflush(stderr)`
  (but in contrast to the latter being typesafe, e.g. foo can be a number)

- `osformat::Print(_("“%2$s“ has size %s")) % filesize % file;`

  Works if order has to be exchanged for foreign translations:
  A foreign translator can translate the string into e.g. `%s Bytes in „%s“`
  As customary by such internationalizations, only the `_()` function has to
  return the translated form: No code changes are necessary!

- `osformat::Print("A %1$s is a %1$s") % "rose";`

  The same argument can be accessed several times.
  The output is `A rose is a rose`.

- `osformat::Print("%1$s = %1$#x") % 15;`

  The same argument can be accessed repeatedly with different
  modifiers/specifiers. Output is `15 = 0xf`.

- `osformat::Print("%S %S") % 17 % std::string::npos;`

  A conversion usually not provided by `<<`:
  Output is `17 std::string::npos`

- `std::string r = osformat::Format("Result %*d") % width % number;`

  The minimal width can be specified by an indirect modifier, similar to
  the POSIX `printf` function.

- `osformat::Format(&r, "%2$*1$d") % width % number;`

  `r.append(osformat::Format("%2$*1$d") % width % number);`

  The previous two lines do the same thing: Appending to r.

- `std::cout << osformat::Format("%*2$.*E") % width % precision % (1./7);`

  The global flags for std::cout remain unchanged: Everything is local

- `std::string number_as_text = osformat::Format() % (1./7);`

  An omitted format acts like `%s` but is almost as efficient as

  `std::string number_as_text = (std::ostringstream() << (1./7)).str();`

- `std::cout << osformat::Print("%s ", osformat::Special::Flush()) % foo;`

  Similar to

  `std::cout << foo << " " << std::flush << foo << " " << std::flush;`

  (if `std::ios_base::sync_with_stdio` is set).

  Explanation: The first output (with flushing) to stdout occurs when the
  `osformat::Print` object is constructed. The second output (with flushing)
  occurs when the constructed object is sent to `std::cout`.

What cannot be seen in these examples:
There is a runtime test for output errors (with aborting in case of errors;
a further argument can be provided to "catch" errors in a variable instead.)


## Description

Essentially, an `osformat::Format` class (and inherited classes) are provided.
Its basic functionality is the constructor which can be used as follows:

`osformat::Format([success,] [output,] [format,] [flags]) % arg1 % arg2 % ...`

Depending on the parameters, the constructor directly appends output to a
string, sends it to `FILE`, or to an `std::ostream`.

It is admissible to postpone some or all of the `%` operations, but until all
arguments specified by format are passed, the object is in an error state.
The generated `osformat::Format` object is copyable, but the copy will not
accept any further `%` operations (and thus remain in the error state if not
all arguments have been passed before copying). When C++11 is used, the object
is movable (including the state of postponed `%` operations).

The finished object (once all arguments have been passed) can be converted
into a string or sent with << into an ostream and also has some further
methods described in a separate section __Further Methods__.
Essentially, the finished object behaves like a string, but when sent to
output it also does automatic error checking (in a way determined by the
constructor) and possibly flushes the streams. (Both can be modified later on).

The types of the parameters of the above constructor are:

- `bool *success`

  If specified, sets success to true/false if there was no/some error.
  If NULL, errors are tacitly ignored.
  If not specified, in the error case the constructor prints a diagnostic
  message to stderr and calls std::abort()
  Note the the pointer (and whether it is provided) is stored in the object
  also for all further operations with the object. There is a special methods
  set_success() to modify this pointer or to enable/disable the std::abort()
  handling.

- `std::string *output`
- `FILE *output`
- `std::ostream& output`

  If specified and not NULL, the output is appended to the string,
  sent to the FILE or output to the ostream, respectively.

- `const char *format`
- `const std::string& format`
- `char format`
- `bool format`

  If format is true or omitted, it is treated like `%s` but more efficient.
  If format is false, it is treated like `` (empty string) but more efficient.
  Details of the format are specified in the section __Format__.
  It describes in a manner similiar to the `printf` format how the
  arguments are to be interpreted. It is a design decision that only those
  finer `printf` features are available which can easily be mapped to
  `std::ostream` analogues (perhaps with simple post-/preprocessing):
  It is not attempted to convert numbers without any support from STL.
  Moreover, due to C++ language restrictions, there is a restriction on the
  order of arguments: All indirect modifier arguments must not be specified
  after the corresponding argument.
  To avoid any issues with possible translations of the format string into
  another language, it is therefore recommended, to first specify all the
  arguments of base, fill, precision, or fieldwidth before specifying any
  further actual arguments.

- `osformat::Special flags`

  This parameter can be generated with one of the static functions

  - `osformat::Special::None()`         // Default: Nothing special
  - `osformat::Special::Newline()`      // Append a newline at the end
  - `osformat::Special::Flush()`        // Flush after output
  - `osformat::Special::NewlineFlush()` // Combine the previous two
  - `osformat::Special::FlushNewline()` // dito, just an alternative name

  The osformat::Special type behaves similar to a bitmap type and provides the
  binary operations `|` `&` `^` `~` `|=` `&=` `^=`, but it cannot directly be
  converted to or from a numerical type to make the argument of the
  `osformat::Format` class unambiguous. However, it might be assigned to/from
  the underlying numerical type `osfp:Special::Flags`.
  (Reason for the static functions:
  C++98 does not know strongly typed enums, and static functions in C++98
  are an issue for initialization order. Both could be avoided with C++11.)

All further arguments must match to the specified format string.
Depending on the format string, they are usually converted into strings with

`std::ostream << arg`

where `std::ostream` has a status determined by the format parameter.
Some arguments might also be interpreted for setting this status.

There are some inherited classes:

- `ostream::Print([success,] [format,] [flags]) % arg1 % arg2 % ...`
- `ostream::PrintError([success,] [format,] [flags]) % arg1 % arg2 % ...`
- `ostream::Say([success,] [format,] [flags]) % arg1 % arg2 % ...`
- `ostream::SayError([success,] [format]) % arg1 % arg2 % ...`

These are analogous to `ostream::Format`. The only differences are:

1. The output automatically goes to `stdout` (for `Print` and `Say`) or
   `stderr` (for `PrintError` or `SayError`), respectively.
2. For `Say`, `ostream::Special::Newline()` is always active
   (i.e. even if flags are explicitly specified as `ostream::Special::None()`
   or `ostream::Special::Flush()`, they are interpreted as if specified by
   `ostream::Special:Newline()` or `ostream::Special::NewlineFlush()`,
   respectively).
3. For SayError the flags are `ostream::Special::NewlineFlush()`


## Further Methods

For objects of type `ostream::Format` (or `Print`, `PrintError`, `Say`,
`SayError`) the following further methods are available:

- `void set_success(bool *success)`
- `void set_Success()`

  Defines that all further operations should signal an error to the
  corresponding pointer. If success is `NULL`, the error is tacitly ignored.
  If called without argument, all future errors will lead to the output of a
  diagnostic message, followed by `abort()`.

- `bool flush()`
- `void flush(bool flush_state)`
  Returns whether flush for output is active or sets its state to the specified
  value, respectively.

- `void Output(std::string *output)`
- `void Output(std::ostream &output)`
- `void Output(FILE *output)`

  Output the object to the specified output object. In case of a string,
  the output is appended, and in case of `std::ostream` or `FILE`, it is
  possibly flushed, depending on the state of `flush()`.
  If an error occurs, the behaviour corresponds to that specified by the
  constructor (or by the latest call to set_success if there was one).

- `std::string str()`

  returns the produced string

- `operator std::string()`

  alternatively to `str()`, the object can be cast into the produced string

- `const std::string& StringReference()`

  returns a reference to the produced string

- `string::size c_str()`

  a short form of `StringReference().c_str()`

- std::string::size_type size()

  a short form of `StringReference().size()`

- `bool empty()`
  A short form of `StringReference().empty()`

- `std::size_t count()`

  If previously output to a `FILE`, this returns the number of bytes actually
  written. The value is unspecified if there was no previous output to FILE.

- `void assign(const osformat::Format& source)`
- `void assign(osformat::Format&& source)`

   Assigns the format the value of source, analogously to STL containers

- `osformat::Error::Code error()`

  Returns which error occured:

  * `osformat::Error::kNone = 0`  // no error
  * `osformat::Error::kWriteFailed`  // e.g. count() is lower than size()
  * `osformat::Error::kFlushFailed`  // fflush of the FILE failed
  * `osformat::Error::kTooManyArguments`
  * `osformat::Error::kTooFewArguments`
  * `osformat::Error::kTooEarlyArgument` // E.g. width value is too late
  * `osformat::Error::kLocaleArgIsNoLocale`
  * `osformat::Error::kLocaleMustNotBeOutput`
  * `osformat::Error::kPrecisionArgIsNotNumeric`
  * `osformat::Error::kWidthArgIsNotNumeric`
  * `osformat::Error::kFillArgIsNotChar`

  All other error codes refer to invalid definitions of the format string.

  There is an `ost::Error` object which is purely static: It contains only the
  above constants and the static methods

  * `const char *osformat::Error::c_str(osformat::Error::Code)`
  * `std::string osformat::Error::as_string(osformat::Error::Code)`
  * `void osformat::Error::append(std::string *, osformat::Error::Code)`

  which return/append an English description of the passed

  `osformat::Error::Code`

  The return value of `osformat::Error::c_str` is static and must not be freed.

Strictly speaking, the following is not a method, but it is available
for the `osformat::Format object`:

- `ostream& operator<<(const ostream::Format &ostream, osformat::Format &)`

  When output into a stream, it behaves as output(ostream)


## Format

The format is similar to that of printf as specified by POSIX.
It is composed of zero or more directives: ordinary characters, i.e.
all except `%`, are taken unchanged. The character `%` introduces a conversion
specification which usually refers to arguments. The conversion specification

- `%%`

  is special: It refers to no argument and just outputs the % character.
  Otherwise, a conversion specification has the form

  `% [argnumber$] [modifiers] specifier`

  (Here, spaces are used only for readability, and […] means as usual that
  the corresponding … can be omitted.)

  It is admissible that the same argnumber occurs several times.
  If no argnumber is given, the smallest “free” number is used.
  Here, a number is “free” if it is not used by any explicit argnumber
  specifier within the format. Free numbers are given in the following order:
  First, numbers occuring in indirect modifier arguments of the first conversion
  specificiation are given (in the order given in the string),
  Then the number for the first conversion specification is given.
  Then the numbers occuring in the modifiers of the second conversion
  specification is given, then the numbers for the second conversion, etc.
  This corresponds to POSIX as close as possible.
  Moreover, it ensures (if no explicit numbers are provided), that indirect
  arguments setting modifiers do not occur after any corresponding argument
  they should modify.
  The latter is a restriction due to C++ syntax limitations: An argument must
  not occur earlier in the argument list than any of its indirect modifiers.

  To avoid problems for the case when the format string might come from a
  foreign translation, it is therefore recommended to always order arguments
  for modifiers in the beginning.

  Modifiers consist of a single character, sometimes followed by an
  argument part. The following modifiers exist:

- `#`

  Use `std::ios_base::showbase` when converting. This means that e.g. for
  hexadezimal numbers an `0x` (or `0X`) is prepended.

- `+`

  Use `std::ios_base::showpos` when converting.
  This means that a positive number is output with a leading `+` symbol

` ` ` (space)

  This is like `+` but replaces in the result the first `+` symbol
  (if it exists) by a space character. An additionally specified `+` is
  ignored.

- `0`

  Use 0 instead of space for padding. This is a shortcut for `_0`:

-  `_`_character_ Use _character_ instead of the space character for padding.
  The last character specified takes precedence.

- `/`[_argnumber_`$`]

  Use the character from the specified argument instead of
  space for padding. The last character specified takes precedence.
  This modifier takes precedence over `_‘character’`.
  If it is specified several times, the highest argument number takes
  precedence. If the _argnumber_`$` part is ommitted, the first free number is
  chosen according to the rules explained above.

- `-`

  Use `std::ios_base::left` instead of `std::ios_base::right`
  This means that the padding characters are added to the right

- `:`

  Use `std::ios_base::internal` instead of `std::ios_base::right`
  This means that padding characters are added in the middle e.g. for monetary.
  If both `-` and `:` are specified, the last occurence “wins”

- _number_

  (starting with 1-9). Use _number_ as the minimal field width for padding

- `*`[_argnumber_`$`]

  As _number_, but interpret the corresponding argument as the
  number. If that argument is negative, its absolute value is considered,
  and the modifier `-` becomes active.
  This modifier overrides the _number_ modifier.

- `.`_number_

  Use _number_ as the precision

- `.*`[_argnumber_`$`]

  As `.`_number_, but use the corresponding argument to specify the
  precision. If the _argnumber_`$` part is ommitted, the first free number is
  chosen according to the rules explained above. This takes precedence over
  the `.`_number_ argument.

- `~`[_argnumber_`$`]

  The corresponding argument must be a (const reference to a)
  `std::locale` which should be used for the conversion.

For consistency with POSIX, it is recommended to use the
field list (plain number or `*`) as one of the last modifiers, and the
precision modifier (`.` or `.*`) as the very last one.


Finally, here are the supported specifiers:

- `s`

  The argument can be almost anything: `ostream`'s `<<` is used to convert it.
  Note that if you want to interpret a `char *` as a pointer (not as a string)
  you should `static_cast` it into a `void *`.
  Similarly, to force a character output, make sure to `static_cast` it into a
  `char`.

- `S`

  as `s`, but there are special rules to make the output more readable:

  * (a) A number of length `std::string::size_type` with value
        `std::string::npos` is output as the string `std::string::npos`
  * (b) The value `std::ios_base::boolalpha` is set so that the type `bool` is
        output as `true` or `false`.
  * (c) The value `std::ios_base::showpoint` is set to indicate the type of
        floats

- `d`

  as `S` without (c)

- `D`

  as `d`, but additionally set `std::ios_base::uppercase`

- `x`

  as s, but set `std::ios_base::hex`.
  So the output of numbers is usually in hex `0123456789abcdef`.

- `X`

  as `s`, but set `std::ios_base::hex | std::ios_base::uppercase`
  So the output of numbers is usually in hex `0123456789ABCDEF`.

- `o`

  as `s`, but set `std::ios_base::oct`

- `O`

  as `s`, but set `std::ios_base::oct | std::ios_base::uppercase`

- `f`

  as `s`, but set `std::ios_base::fixed`

- `F`

  as `s`, but set `std::ios_base::fixed | std::ios_base::uppercase`

- `e`

  as `s`, but set `std::ios_base::scientific`

- `E`

  as `s`, but set `std::ios_base::scientific | std::ios_base::uppercase`

- `a`

  as `s`, but set `std::ios_base::fixed | std::ios_base::scientific`

  This usually causes output of floats in internal hexadecimal representation

- `A`

  as `a`, but addtionally set `std::ios_base::uppercase`

- `n`

  dummy argument which is not output. This must be used if arguments are passed
  but should not be printed: Otherwise, this would be considered an error.
  A case when this might be useful is e.g. if a foreign translator of a
  format string explicitly wants to omit certain arguments.


## Corner Cases by Examples

- `osformat::Format("%s %1$s") % 'b' % 'a';`

  This is valid and translates into `a b`!
  The reason is that due to the specifier `%1$s`, the first argument is not
  free in the moment when the number for the first specifier `%s` is decided:
  Any specified argument number (even if it occurs in a later format string)
  is not considered to be a free value for unnumbered arguments.

  The motivation for the counterintuitive behaviour above is that you can
  easily place modifier arguments at the beginning when the format is meant
  to be translated into a foreign language:

- `osformat::Format(_("%*1s %*2s")) % width1 % width2 % string1 % string2;`

  If you would have written

- `osformat::Format(_("%*s %*s")) % width1 % string1 % width2 % string2);`

  the translater could not translate the string such that string1 is endowed
  with width2. Indeed:

- `osformat::Format("%1$*2$s") % some_string % minsize);`

  is not valid because the indirect modifier argument must not be passed after
  the corresponding argument it refers to!
  However, equality is allowed. The following is valid:

- `osformat::Format("%1$*1$d") % string_and_minsize;`

  Modifiers for different arguments may also occur later,
  although this practice is not recommended, because it may cause problems
  when the string becomes translated into a foreign language which requires a
  different order of values independent of modifiers:

- `osformat::Format("A: %*s B: %*s") % Awidth % Avalue % Bwidth % Bvalue;`


## History and Contributions

The project was motivated by `src/eixTk/formated.h` from the __eix__ project
which was originally implemented by Emil Beinroth <emilbeinroth at gmx.net>
and later extended by Martin Väth <martin at mvath.de>.

After first reusing this class in some other projects and motivated by the
necessary `printf` hacks in some google projects, see e.g.
https://google.github.io/styleguide/cppguide.html
it became clear that an independent library might be useful.

The initial implementation was provided by Martin Väth <martin at mvath.de>,
who is also so far the only maintainer.

In this original implementation, some ideas from the above projects were used.
Some ideas of `boost::format` and https://github.com/c42f/tinyformat
(author: Chris Foster) have been taken into account, but the `c` and `p`
specifiers are currently not supported, since this probably cannot be done
compiler independent and is not necessary when the user just casts.
There are several other independent implementations of similar type like
e.g. `StringPrintf` propagated by Stefan Woerthmuller.

Only after writing the project, the author learnt about absl::StrFormat
(see https://abseil.io/docs/cpp/guides/format) which allows even compile-time
checking of the arguments.
