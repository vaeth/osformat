// This file is part of the osformat project and distributed under the
// terms of the GNU General Public License v2.
//
// Copyright (c)
//   Martin Väth <martin@mvath.de>

#ifndef OSFORMAT_OSFORMAT_H_
#define OSFORMAT_OSFORMAT_H_ 1

#include <cassert>  // assert
#include <cstdio>  // size_t, FILE

#include <fstream>  // needed only for overloading, see comment below
#include <ios>
#include <iostream>  // needed only for overloading, see comment below
#include <locale>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

#if __cplusplus >= 201103L
#include <utility>  // std::move
#endif

// We must include all the iostream and fstream stuff:
// It is not sufficient to declare e.g. std::iostream or std::fstream,
// because we must know how to downcast the object to std::ostream.
// Whether this can be done with e.g. static_cast if only the downcast object
// is fully known might be compiler dependent, so better not rely on it.

namespace osformat {

class Error {
 public:
  enum Code {
    kNone = 0,
    kWriteFailed,
    kFlushFailed,
    kTooManyArguments,
    kTooFewArguments,
    kTooEarlyArgument,
    kLocaleArgIsNoLocale,
    kLocaleMustNotBeOutput,
    kPrecisionArgIsNotNumeric,
    kWidthArgIsNotNumeric,
    kFillArgIsNotChar,
    kTrailingPercentage,
    kNumberWithoutDollar,
    kNumberOverflow,
    kMissingSpecifier,
    kUnknownSpecifier,
    kMissingFillCharacter,
    kEnd
  };

  static const char *c_str(Code c) {
    unsigned int index(static_cast<unsigned int>(c));
    assert(index < static_cast<unsigned int>(kEnd));
    return Description[index];
  }

  static std::string as_string(Code c) {
    return c_str(c);
  }

  static void append(std::string s, Code c) {
    s.append(c_str(c));
  }

 private:
  static const char *Description[];

// As being a purely static object, this is not meant to be instantiated.
#if __cplusplus >= 201103L
// Return readable error codes in case of C++-11 if the user attempts it:
  Error() = delete;
#else  // __cplusplus < 201103L
// When we cannot delete, we get misleading errors but the same effect
// if our constructor is private:
  Error() {}
#endif  // __cplusplus
};


// C++98 has no strongly typed enum class.
// But for the Format() constructor, it is important to have a numerical type
// which does not automatigically convert from/to a standard type
// (i.e. which provides neither a cast into such a standard type not a
// constructor which accepts a standard type).
// Therefore, we provide an independent class.

class Special {
 public:
  typedef unsigned int Flags;

#if __cplusplus >= 201103L
  constexpr
#endif
  static const Flags
    kNone         = 0,
    kNewline      = 1 << 1,
    kFlush        = 1 << 2,
    kAll          = (1 << 3) - 1;

  Special()
    : flags_(kNone) {
  }

  // Instead of parameter constructors, we provide static functions which
  // return the required object

  static Special New(Flags f) {
    Special a;
    a.flags_ = f;
    return a;
  }

  static Special None() {
    return Special();
  }

  static Special Newline() {
    return New(kNewline);
  }

  static Special Flush() {
    return New(kNewline);
  }

  static Special NewlineFlush() {
    return New(kNewline | kFlush);
  }

  static Special FlushNewline() {
    return New(kFlush | kNewline);
  }

  // Assignments an bit operations can be fully allowed.
  // This is complete overkill, but who knows what the user might want to do...

  Special operator=(Flags f) {
    flags_ = f;
    return *this;
  }

  void assign(Flags f) {
    flags_ = f;
  }

  void assign(Special s) {
    flags_ = s.flags_;
  }

  Flags get() const {
    return flags_;
  }

  void SetBits(Flags f) {
    flags_ |= f;
  }

  void SetBits(Special s) {
    flags_ |= s.flags_;
  }

  void ClearBits(Flags f) {
    flags_ &= (f ^ kAll);
  }

  void ClearBits(Special s) {
    flags_ &= (s.flags_ ^ kAll);
  }

  bool HaveBits(Flags f) const {
    return ((flags_ & f) != kNone);
  }

  bool HaveBits(Special s) const {
    return ((flags_ & s.flags_) != kNone);
  }

  Special operator|=(Flags f) {
    flags_ |= f;
    return *this;
  }

  Special operator|=(Special s) {
    flags_ |= s.flags_;
    return *this;
  }

  Special operator&=(Flags f) {
    flags_ &= f;
    return *this;
  }

  Special operator&=(Special s) {
    flags_ &= s.flags_;
    return *this;
  }

  Special operator|(Flags f) const {
    return New(flags_ | f);
  }

  friend Special operator|(Flags f, Special s) {
    return New(f | s.flags_);
  }

  Special operator|(Special s) const {
    return New(flags_ | s.flags_);
  }

  Special operator&(Flags f) const {
    return New(flags_ & f);
  }

  friend Special operator&(Flags f, Special s) {
    return New(f & s.flags_);
  }

  Special operator&(Special s) const {
    return New(flags_ & s.flags_);
  }

  Special operator^(Flags f) const {
    return New(flags_ ^ f);
  }

  friend Special operator^(Flags f, Special s) {
    return New(f ^ s.flags_);
  }

  Special operator^(Special s) const {
    return New(flags_ ^ s.flags_);
  }

  Special operator~() const {
    return New(~flags_);
  }

 private:
  Flags flags_;
};


class Format {
 private:
  class Defines {
   public:
    typedef unsigned int Flags;

#if __cplusplus >= 201103L
    constexpr
#endif
    static const Flags
      kNone      = 0,
      kLocale    = 1 << 1,
      kPrecision = 1 << 2,
      kWidth     = 1 << 3,
      kFill      = 1 << 4,
      kArg       = 1 << 5,
      kAll       = (1 << 6) - 1;

   private:
    Defines() {}  // Do not instantiate this purely static class by accident
  };

  class Extensions {
   public:
    typedef unsigned int Flags;

#if __cplusplus >= 201103L
    constexpr
#endif
    static const Flags
      kNone       = 0,
      kIgnore     = 1 << 1,  // Ignore the whole argument
      kPlusSpace  = 1 << 2,  // + -> ' '
      kStringNpos = 1 << 3,  // std::string::npos translation
      kAll        = (1 << 4) - 1;

   private:
    Extensions() {}  // Do not instantiate this purely static class by accident
  };

  class Manip {
   public:
    std::ostringstream ostream_;
    Extensions::Flags extensions_;
    Defines::Flags need_;
    Manip()
      : extensions_(Extensions::kNone), need_(Defines::kNone) {
    }
  };

  class References {
   public:
    Defines::Flags set_these_;
    Manip *manip_;
    References(Defines::Flags set_these, Manip *manip)
      : set_these_(set_these), manip_(manip) {
    }
  };

  // The Parse class contains the data needed to parse the formatstring,
  // to process the % operators, and to output the result the first time.
  // After this, the whole data is superfluous and will be removed from Format.

  class Parse {
   public:
    // The list of formats in the order of the format string
    typedef std::vector<Manip*> FormatList;
    FormatList format_;

    // The indices of format specifiers (begin, end + 1) in the format string
    typedef std::vector<std::string::size_type> BorderList;
    BorderList borders_;

    // The indirect references to format_ in the order of the arguments
    typedef std::vector<References> ArgsDefines;
    typedef std::vector<ArgsDefines> ArgsList;
    typedef ArgsList::size_type size_type;
    ArgsList args_;

    // The next argument parsed by the % operator
    ArgsList::const_iterator current_arg_;

    // Is the whole stuff only considered to be an implicit %s?
    bool simple_;

    std::string *append_;
    FILE *file_;
    std::ostream *ostream_;

    Parse(bool simple, std::string *append, FILE *file,
        std::ostream *ostream)
      : simple_(simple), append_(append), file_(file), ostream_(ostream) {
    }

    ~Parse();

    void SetIndirect(size_type argnum, Defines::Flags set_these, Manip *manip);
  };

  bool abort_;
  bool *success_;
  mutable Error::Code error_;
  mutable size_t count_;

  std::string text_;  // The format string or result

  Parse *parse_;
  Special flags_;

  typedef std::vector<bool> SpecifiedList;

  void Init(std::string *append, FILE *file, std::ostream *ostream,
    bool format);

  void Init(std::string *append, FILE *file, std::ostream *ostream);

  bool ParseFormat();

  bool SetArg(Defines::Flags set_these, Parse::ArgsDefines *define_queue,
    SpecifiedList *specified, Manip *manip, std::string::size_type *index);

  void HandleArgumentNumber(Parse::size_type argnum,
    SpecifiedList *specified);

  // Parse a number of nonzero length from start to end (after last digit).
  // Return true if no error.
  template<class T> bool ParseNumber(T *number,
      std::string::size_type start, std::string::size_type end) {
    T result = static_cast<T>(text_[start] - '0');
    for (;;) {
      if (++start == end) {
        *number = result;
        return true;
      }
      T next_result = (result * 10) + static_cast<T>(text_[start] - '0');
      if (next_result <= result) {
        Throw(Error::kNumberOverflow);
        return false;
      }
      result = next_result;
    }
  }

  void FinishInsertingArgs();

  void OutputInternal(std::string *append) const;

  void OutputInternal(FILE *file) const;

  void OutputInternal(std::ostream& ostream) const;

  void InitialOutput();

  void Check() const {
    if (parse_ != NULL) {
      Throw(Error::kTooFewArguments);
    }
  }

  void Throw(Error::Code error) const;

  // This is the default template to catch errors at runtime:
  template<class T> bool SetLocale(std::ostream *, const T&) {
    Throw(Error::kLocaleArgIsNoLocale);
    return false;
  }

  // The valid argument is specialized:
  bool SetLocale(std::ostream *os, const std::locale& arg) {
    os->imbue(arg);
    return true;
  }

  // This is the default template to catch errors at runtime:
  template<class T> bool SetPrecision(std::ostream *, const T&) {
    Throw(Error::kPrecisionArgIsNotNumeric);
    return false;
  }

  // To not run into the default "error" template, we need explicit
  // specializations for every valid conversion from a basic type.
  bool SetPrecision(std::ostream *os, bool arg) {
    os->precision(static_cast<std::streamsize>(arg));
    return true;
  }

  bool SetPrecision(std::ostream *os, char arg) {
    os->precision(static_cast<std::streamsize>(arg));
    return true;
  }

  bool SetPrecision(std::ostream *os, signed char arg) {
    os->precision(static_cast<std::streamsize>(arg));
    return true;
  }

  bool SetPrecision(std::ostream *os, unsigned char arg) {
    os->precision(static_cast<std::streamsize>(arg));
    return true;
  }

  bool SetPrecision(std::ostream *os, short arg) {  // NOLINT(runtime/int)
    os->precision(static_cast<std::streamsize>(arg));
    return true;
  }

  bool SetPrecision(std::ostream *os,
      unsigned short arg) {  // NOLINT(runtime/int)
    os->precision(static_cast<std::streamsize>(arg));
    return true;
  }

  bool SetPrecision(std::ostream *os, int arg) {
    os->precision(static_cast<std::streamsize>(arg));
    return true;
  }

  bool SetPrecision(std::ostream *os, unsigned int arg) {
    os->precision(static_cast<std::streamsize>(arg));
    return true;
  }

  bool SetPrecision(std::ostream *os, long arg) {  // NOLINT(runtime/int)
    os->precision(static_cast<std::streamsize>(arg));
    return true;
  }

  bool SetPrecision(std::ostream *os,
      unsigned long arg) {  // NOLINT(runtime/int)
    os->precision(static_cast<std::streamsize>(arg));
    return true;
  }

  bool SetPrecision(std::ostream *os, float arg) {
    os->precision(static_cast<std::streamsize>(arg));
    return true;
  }

  bool SetPrecision(std::ostream *os, double arg) {
    os->precision(static_cast<std::streamsize>(arg));
    return true;
  }

  bool SetPrecision(std::ostream *os, long double arg) {
    os->precision(static_cast<std::streamsize>(arg));
    return true;
  }

#if __cplusplus >= 201103L
  bool SetPrecision(std::ostream *os, char16_t arg) {
    os->precision(static_cast<std::streamsize>(arg));
    return true;
  }

  bool SetPrecision(std::ostream *os, char32_t arg) {
    os->precision(static_cast<std::streamsize>(arg));
    return true;
  }

  bool SetPrecision(std::ostream *os, wchar_t arg) {
    os->precision(static_cast<std::streamsize>(arg));
    return true;
  }

  bool SetPrecision(std::ostream *os,
      long long arg) {  // NOLINT(runtime/int)
    os->precision(static_cast<std::streamsize>(arg));
    return true;
  }

  bool SetPrecision(std::ostream *os,
      unsigned long long arg) {  // NOLINT(runtime/int)
    os->precision(static_cast<std::streamsize>(arg));
    return true;
  }
#endif  // __cplusplus

  // This is the default template to catch errors at runtime:
  template<class T> bool SetWidth(std::ostream *, const T&) {
    Throw(Error::kWidthArgIsNotNumeric);
    return false;
  }

  // To not run into the default "error" template, we need explicit
  // specializations for every valid conversion from a basic type.
  bool SetWidth(std::ostream *os, bool arg) {
    os->width(static_cast<std::streamsize>(arg));
    return true;
  }

  bool SetWidth(std::ostream *os, char arg) {
    if (arg <= 0) {  // Do not trigger a warning if char is unsigned
      if (arg != 0) {
        arg = -arg;
        os->setf(std::ios_base::left,
          std::ios_base::adjustfield);
      }
    }
    os->width(static_cast<std::streamsize>(arg));
    return true;
  }

  bool SetWidth(std::ostream *os, signed char arg) {
    if (arg < 0) {
      arg = -arg;
      os->setf(std::ios_base::left, std::ios_base::adjustfield);
    }
    os->width(static_cast<std::streamsize>(arg));
    return true;
  }

  bool SetWidth(std::ostream *os, unsigned char arg) {
    os->width(static_cast<std::streamsize>(arg));
    return true;
  }

  bool SetWidth(std::ostream *os, short arg) {  // NOLINT(runtime/int)
    if (arg < 0) {
      arg = -arg;
      os->setf(std::ios_base::left, std::ios_base::adjustfield);
    }
    os->width(static_cast<std::streamsize>(arg));
    return true;
  }

  bool SetWidth(std::ostream *os,
      unsigned short arg) {  // NOLINT(runtime/int)
    os->width(static_cast<std::streamsize>(arg));
    return true;
  }

  bool SetWidth(std::ostream *os, int arg) {
    if (arg < 0) {
      arg = -arg;
      os->setf(std::ios_base::left, std::ios_base::adjustfield);
    }
    os->width(static_cast<std::streamsize>(arg));
    return true;
  }

  bool SetWidth(std::ostream *os, unsigned int arg) {
    os->width(static_cast<std::streamsize>(arg));
    return true;
  }

  bool SetWidth(std::ostream *os, long arg) {  // NOLINT(runtime/int)
    if (arg < 0) {
      arg = -arg;
      os->setf(std::ios_base::left,
        std::ios_base::adjustfield);
    }
    os->width(static_cast<std::streamsize>(arg));
    return true;
  }

  bool SetWidth(std::ostream *os,
      unsigned long arg) {  // NOLINT(runtime/int)
    os->width(static_cast<std::streamsize>(arg));
    return true;
  }

  bool SetWidth(std::ostream *os, float arg) {
    if (arg < 0) {
      arg = -arg;
      os->setf(std::ios_base::left, std::ios_base::adjustfield);
    }
    os->width(static_cast<std::streamsize>(arg));
    return true;
  }

  bool SetWidth(std::ostream *os, double arg) {
    if (arg < 0) {
      arg = -arg;
      os->setf(std::ios_base::left, std::ios_base::adjustfield);
    }
    os->width(static_cast<std::streamsize>(arg));
    return true;
  }

  bool SetWidth(std::ostream *os, long double arg) {
    if (arg < 0) {
      arg = -arg;
      os->setf(std::ios_base::left, std::ios_base::adjustfield);
    }
    os->width(static_cast<std::streamsize>(arg));
    return true;
  }

#if __cplusplus >= 201103L
  bool SetWidth(std::ostream *os, char16_t arg) {
    os->width(static_cast<std::streamsize>(arg));
    return true;
  }

  bool SetWidth(std::ostream *os, char32_t arg) {
    os->width(static_cast<std::streamsize>(arg));
    return true;
  }

  bool SetWidth(std::ostream *os, wchar_t arg) {
    if (arg <= 0) {  // Do not trigger a warning if wchar_t is unsigned
      if (arg != 0) {
        arg = -arg;
        os->setf(std::ios_base::left, std::ios_base::adjustfield);
      }
    }
    os->width(static_cast<std::streamsize>(arg));
    return true;
  }

  bool SetWidth(std::ostream *os,
      long long arg) {  // NOLINT(runtime/int)
    if (arg < 0) {
      arg = -arg;
      os->setf(std::ios_base::left, std::ios_base::adjustfield);
    }
    os->width(static_cast<std::streamsize>(arg));
    return true;
  }

  bool SetWidth(std::ostream *os,
      unsigned long long arg) {  // NOLINT(runtime/int)
    os->width(static_cast<std::streamsize>(arg));
    return true;
  }
#endif  // __cplusplus

  // This is the default template to catch errors at runtime:
  template<class T> bool SetFill(std::ostream *, const T&) {
    Throw(Error::kFillArgIsNotChar);
    return false;
  }

  // To not run into the default "error" template, we need explicit
  // specializations for every valid conversion from a basic type.
  bool SetFill(std::ostream *os, bool arg) {
    os->fill(static_cast<char>(arg));
    return true;
  }

  bool SetFill(std::ostream *os, char arg) {
    os->fill(static_cast<char>(arg));
    return true;
  }

  bool SetFill(std::ostream *os, signed char arg) {
    os->fill(static_cast<char>(arg));
    return true;
  }

  bool SetFill(std::ostream *os, unsigned char arg) {
    os->fill(static_cast<char>(arg));
    return true;
  }

  bool SetFill(std::ostream *os, short arg) {  // NOLINT(runtime/int)
    os->fill(static_cast<char>(arg));
    return true;
  }

  bool SetFill(std::ostream *os, unsigned short arg) {  // NOLINT(runtime/int)
    os->fill(static_cast<char>(arg));
    return true;
  }

  bool SetFill(std::ostream *os, int arg) {
    os->fill(static_cast<char>(arg));
    return true;
  }

  bool SetFill(std::ostream *os, unsigned int arg) {
    os->fill(static_cast<char>(arg));
    return true;
  }

  bool SetFill(std::ostream *os, long arg) {  // NOLINT(runtime/int)
    os->fill(static_cast<char>(arg));
    return true;
  }

  bool SetFill(std::ostream *os, unsigned long arg) {  // NOLINT(runtime/int)
    os->fill(static_cast<char>(arg));
    return true;
  }

  bool SetFill(std::ostream *os, float arg) {
    os->fill(static_cast<char>(arg));
    return true;
  }

  bool SetFill(std::ostream *os, double arg) {
    os->fill(static_cast<char>(arg));
    return true;
  }

  bool SetFill(std::ostream *os, long double arg) {
    os->fill(static_cast<char>(arg));
    return true;
  }

#if __cplusplus >= 201103L
  bool SetFill(std::ostream *os, char16_t arg) {
    os->fill(static_cast<char>(arg));
    return true;
  }

  bool SetFill(std::ostream *os, char32_t arg) {
    os->fill(static_cast<char>(arg));
    return true;
  }

  bool SetFill(std::ostream *os, wchar_t arg) {
    os->fill(static_cast<char>(arg));
    return true;
  }

  bool SetFill(std::ostream *os, long long arg) {  // NOLINT(runtime/int)
    os->fill(static_cast<char>(arg));
    return true;
  }

  bool SetFill(std::ostream *os,
      unsigned long long arg) {  // NOLINT(runtime/int)
    os->fill(static_cast<char>(arg));
    return true;
  }
#endif  // __cplusplus

  // The standard output function. We must overload it to deal with locale
  template<class T> bool StringStandard(std::ostream *os, const T& arg) {
    (*os) << arg;
    return true;
  }

  // It must be syntactically admissible to call the standard output
  // functions with locales, but semantically it is nonsense
  bool StringStandard(std::ostream *, const std::locale&) {
    Throw(Error::kLocaleMustNotBeOutput);
    return false;
  }

  // The output function with a special treatment os string::npos
  template<class T> bool StringNpos(std::ostream *os, const T& arg) {
    (*os) << arg;
    return true;
  }

  bool StringNpos(std::ostream *os, const std::string::size_type& arg) {
    if (arg == std::string::npos) {
      (*os) << "std::string::npos";
      return true;
    }
    (*os) << arg;
    return true;
  }

  // It must be syntactically admissible to call the standard output
  // functions with locales, but semantically it is nonsense
  bool StringNpos(std::ostream *, const std::locale&) {
    Throw(Error::kLocaleMustNotBeOutput);
    return false;
  }

 public:
  Format(const Format& s) {
    parse_ = NULL;
    assign(s);
  }

  Format& operator=(const Format& s) {
    assign(s);
    return *this;
  }

  void assign(const Format& s) {
    abort_ = s.abort_;
    success_ = s.success_;
    error_ = s.error_;
    flags_ = s.flags_;
    count_ = s.count_;
    text_ = s.text_;
    delete parse_;
    parse_ = NULL;
  }

#if __cplusplus > 201103L
  Format(Format&& s) {
    parse_ = NULL;
    assign(std::move(s));
  }

  Format& operator=(Format&& s) {
    parse_ = NULL;
    assign(std::move(s));
    return *this;
  }

  void assign(Format&& s) {
    abort_ = std::move(s.abort_);
    success_ = std::move(s.success_);
    error_ = std::move(s.error_);
    flags_ = std::move(s.flags_);
    count_ = std::move(s.count_);
    text_ = std::move(s.text_);
    delete parse_;
    parse_ = std::move(s.parse_);
    s.parse_ = NULL;
  }
#endif  // __cplusplus

  ~Format() {
    delete parse_;
  }

// We cannot rely on default arguments, because the variable format is last.
// So we have to deal with the exponential explosion of cases manually:

  Format(bool *success, std::string *output, const char *format, Special flags)
    : abort_(false), success_(success), text_(format), flags_(flags) {
    Init(output, NULL, NULL);
  }

  Format(bool *success, std::string *output, const std::string& format,
      Special flags)
    : abort_(false), success_(success), text_(format), flags_(flags) {
    Init(output, NULL, NULL);
  }

  Format(bool *success, std::string *output, char format, Special flags)
    : abort_(false), success_(success), text_(1, format), flags_(flags) {
    Init(output, NULL, NULL);
  }

  Format(bool *success, std::string *output, bool format, Special flags)
    : abort_(false), success_(success), flags_(flags) {
    Init(output, NULL, NULL, format);
  }

  Format(bool *success, std::string *output, const char *format)
    : abort_(false), success_(success), text_(format) {
    Init(output, NULL, NULL);
  }

  Format(bool *success, std::string *output, const std::string& format)
    : abort_(false), success_(success), text_(format) {
    Init(output, NULL, NULL);
  }

  Format(bool *success, std::string *output, char format)
    : abort_(false), success_(success), text_(1, format) {
    Init(output, NULL, NULL);
  }

  Format(bool *success, std::string *output, bool format)
    : abort_(false), success_(success) {
    Init(output, NULL, NULL, format);
  }

  Format(bool *success, std::string *output, Special flags)
    : abort_(false), success_(success), flags_(flags) {
    Init(output, NULL, NULL, true);
  }

  Format(bool *success, std::string *output)
    : abort_(false), success_(success) {
    Init(output, NULL, NULL, true);
  }

  Format(bool *success, FILE *output, const char *format, Special flags)
    : abort_(false), success_(success), text_(format), flags_(flags) {
    Init(NULL, output, NULL);
  }

  Format(bool *success, FILE *output, const std::string& format, Special flags)
    : abort_(false), success_(success), text_(format), flags_(flags) {
    Init(NULL, output, NULL);
  }

  Format(bool *success, FILE *output, char format, Special flags)
    : abort_(false), success_(success), text_(1, format), flags_(flags) {
    Init(NULL, output, NULL);
  }

  Format(bool *success, FILE *output, bool format, Special flags)
    : abort_(false), success_(success), flags_(flags) {
    Init(NULL, output, NULL, format);
  }

  Format(bool *success, FILE *output, const char *format)
    : abort_(false), success_(success), text_(format) {
    Init(NULL, output, NULL);
  }

  Format(bool *success, FILE *output, const std::string& format)
    : abort_(false), success_(success), text_(format) {
    Init(NULL, output, NULL);
  }

  Format(bool *success, FILE *output, char format)
    : abort_(false), success_(success), text_(1, format) {
    Init(NULL, output, NULL);
  }

  Format(bool *success, FILE *output, bool format)
    : abort_(false), success_(success) {
    Init(NULL, output, NULL, format);
  }

  Format(bool *success, FILE *output, Special flags)
    : abort_(false), success_(success), flags_(flags) {
    Init(NULL, output, NULL, true);
  }

  Format(bool *success, FILE *output)
    : abort_(false), success_(success) {
    Init(NULL, output, NULL, true);
  }

  Format(bool *success, std::ostream& output, const char *format,
      Special flags)
    : abort_(false), success_(success), text_(format), flags_(flags) {
    Init(NULL, NULL, &output);
  }

  Format(bool *success, std::ostream& output, const std::string& format,
      Special flags)
    : abort_(false), success_(success), text_(format), flags_(flags) {
    Init(NULL, NULL, &output);
  }

  Format(bool *success, std::ostream& output, char format, Special flags)
    : abort_(false), success_(success), text_(1, format), flags_(flags) {
    Init(NULL, NULL, &output);
  }

  Format(bool *success, std::ostream& output, bool format, Special flags)
    : abort_(false), success_(success), flags_(flags) {
    Init(NULL, NULL, &output, format);
  }

  Format(bool *success, std::ostream& output, const char *format)
    : abort_(false), success_(success), text_(format) {
    Init(NULL, NULL, &output);
  }

  Format(bool *success, std::ostream& output, const std::string& format)
    : abort_(false), success_(success), text_(format) {
    Init(NULL, NULL, &output);
  }

  Format(bool *success, std::ostream& output, char format)
    : abort_(false), success_(success), text_(1, format) {
    Init(NULL, NULL, &output);
  }

  Format(bool *success, std::ostream& output, bool format)
    : abort_(false), success_(success) {
    Init(NULL, NULL, &output, format);
  }

  Format(bool *success, std::ostream& output, Special flags)
    : abort_(false), success_(success), flags_(flags) {
    Init(NULL, NULL, &output, true);
  }

  Format(bool *success, std::ostream& output)
    : abort_(false), success_(success) {
    Init(NULL, NULL, &output, true);
  }

  Format(bool *success, const char *format, Special flags)
    : abort_(false), success_(success), text_(format), flags_(flags) {
    Init(NULL, NULL, NULL);
  }

  Format(bool *success, const std::string& format, Special flags)
    : abort_(false), success_(success), text_(format), flags_(flags) {
    Init(NULL, NULL, NULL);
  }

  Format(bool *success, char format, Special flags)
    : abort_(false), success_(success), text_(1, format), flags_(flags) {
    Init(NULL, NULL, NULL);
  }

  Format(bool *success, bool format, Special flags)
    : abort_(false), success_(success), flags_(flags) {
    Init(NULL, NULL, NULL, format);
  }

  Format(bool *success, const char *format)
    : abort_(false), success_(success), text_(format) {
    Init(NULL, NULL, NULL);
  }

  Format(bool *success, const std::string& format)
    : abort_(false), success_(success), text_(format) {
    Init(NULL, NULL, NULL);
  }

  Format(bool *success, char format)
    : abort_(false), success_(success), text_(1, format) {
    Init(NULL, NULL, NULL);
  }

  Format(bool *success, bool format)
    : abort_(false), success_(success) {
    Init(NULL, NULL, NULL, format);
  }

  Format(bool *success, Special flags)
    : abort_(false), success_(success), flags_(flags) {
    Init(NULL, NULL, NULL, true);
  }

  explicit Format(bool *success)
    : abort_(false), success_(success) {
    Init(NULL, NULL, NULL, true);
  }

  Format(std::string *output, const char *format, Special flags)
    : abort_(true), text_(format), flags_(flags) {
    Init(output, NULL, NULL);
  }

  Format(std::string *output, const std::string& format, Special flags)
    : abort_(true), text_(format), flags_(flags) {
    Init(output, NULL, NULL);
  }

  Format(std::string *output, char format, Special flags)
    : abort_(true), text_(1, format), flags_(flags) {
    Init(output, NULL, NULL);
  }

  Format(std::string *output, bool format, Special flags)
    : abort_(true), flags_(flags) {
    Init(output, NULL, NULL, format);
  }

  Format(std::string *output, const char *format)
    : abort_(true), text_(format) {
    Init(output, NULL, NULL);
  }

  Format(std::string *output, const std::string& format)
    : abort_(true), text_(format) {
    Init(output, NULL, NULL);
  }

  Format(std::string *output, char format)
    : abort_(true), text_(1, format) {
    Init(output, NULL, NULL);
  }

  Format(std::string *output, bool format)
    : abort_(true) {
    Init(output, NULL, NULL, format);
  }

  Format(std::string *output, Special flags)
    : abort_(true), flags_(flags) {
    Init(output, NULL, NULL, true);
  }

  explicit Format(std::string *output)
    : abort_(true) {
    Init(output, NULL, NULL, true);
  }

  Format(FILE *output, const char *format, Special flags)
    : abort_(true), text_(format), flags_(flags) {
    Init(NULL, output, NULL);
  }

  Format(FILE *output, const std::string& format, Special flags)
    : abort_(true), text_(format), flags_(flags) {
    Init(NULL, output, NULL);
  }

  Format(FILE *output, char format, Special flags)
    : abort_(true), text_(1, format), flags_(flags) {
    Init(NULL, output, NULL);
  }

  Format(FILE *output, bool format, Special flags)
    : abort_(true), flags_(flags) {
    Init(NULL, output, NULL, format);
  }

  Format(FILE *output, const char *format)
    : abort_(true), text_(format) {
    Init(NULL, output, NULL);
  }

  Format(FILE *output, const std::string& format)
    : abort_(true), text_(format) {
    Init(NULL, output, NULL);
  }

  Format(FILE *output, char format)
    : abort_(true), text_(1, format) {
    Init(NULL, output, NULL);
  }

  Format(FILE *output, bool format)
    : abort_(true) {
    Init(NULL, output, NULL, format);
  }

  Format(FILE *output, Special flags)
    : abort_(true), flags_(flags) {
    Init(NULL, output, NULL, true);
  }

  explicit Format(FILE *output)
    : abort_(true) {
    Init(NULL, output, NULL, true);
  }

  Format(std::ostream& output, const char *format, Special flags)
    : abort_(true), text_(format), flags_(flags) {
    Init(NULL, NULL, &output);
  }

  Format(std::ostream& output, const std::string& format, Special flags)
    : abort_(true), text_(format), flags_(flags) {
    Init(NULL, NULL, &output);
  }

  Format(std::ostream& output, char format, Special flags)
    : abort_(true), text_(1, format), flags_(flags) {
    Init(NULL, NULL, &output);
  }

  Format(std::ostream& output, bool format, Special flags)
    : abort_(true), flags_(flags) {
    Init(NULL, NULL, &output, format);
  }

  Format(std::ostream& output, const char *format)
    : abort_(true), text_(format) {
    Init(NULL, NULL, &output);
  }

  Format(std::ostream& output, const std::string& format)
    : abort_(true), text_(format) {
    Init(NULL, NULL, &output);
  }

  Format(std::ostream& output, char format)
    : abort_(true), text_(1, format) {
    Init(NULL, NULL, &output);
  }

  Format(std::ostream& output, bool format)
    : abort_(true) {
    Init(NULL, NULL, &output, format);
  }

  Format(std::ostream& output, Special flags)
    : abort_(true), flags_(flags) {
    Init(NULL, NULL, &output, true);
  }

  explicit Format(std::ostream& output)
    : abort_(true) {
    Init(NULL, NULL, &output, true);
  }

  Format(const char *format, Special flags)
    : abort_(true), text_(format), flags_(flags) {
    Init(NULL, NULL, NULL);
  }

  Format(const std::string& format, Special flags)
    : abort_(true), text_(format), flags_(flags) {
    Init(NULL, NULL, NULL);
  }

  Format(char format, Special flags)
    : abort_(true), text_(1, format), flags_(flags) {
    Init(NULL, NULL, NULL);
  }

  Format(bool format, Special flags)
    : abort_(true), flags_(flags) {
    Init(NULL, NULL, NULL, format);
  }

  explicit Format(const char *format)
    : abort_(true), text_(format) {
    Init(NULL, NULL, NULL);
  }

  explicit Format(const std::string& format)
    : abort_(true), text_(format) {
    Init(NULL, NULL, NULL);
  }

  explicit Format(char format)
    : abort_(true), text_(1, format) {
    Init(NULL, NULL, NULL);
  }

  explicit Format(bool format)
    : abort_(true) {
    Init(NULL, NULL, NULL, format);
  }

  explicit Format(Special flags)
    : abort_(true), flags_(flags) {
    Init(NULL, NULL, NULL, true);
  }

  Format()
    : abort_(true) {
    Init(NULL, NULL, NULL, true);
  }

  void set_success() {
    abort_ = true;
    success_ = NULL;
  }

  void set_success(bool *success) {
    success_ = success;
    abort_ = false;
  }

  bool flush() const {
    return (flags_.HaveBits(Special::kFlush));
  }

  void flush(bool flush_state) {
    if (flush_state) {
      flags_.SetBits(Special::kFlush);
    } else {
      flags_.ClearBits(Special::kFlush);
    }
  }

  void Output(std::string *append) const {
    Check();
    OutputInternal(append);
  }

  void Output(FILE *file) const {
    Check();
    OutputInternal(file);
  }

  void Output(std::ostream& ostream) const {
    Check();
    OutputInternal(ostream);
  }

  const std::string& StringReference() const {
    Check();
    return text_;
  }

  std::string str() const {
    Check();
    return text_;
  }

  operator std::string() const {
    return str();
  }

  const char *c_str() const {
    Check();
    return text_.c_str();
  }

  std::string::size_type size() const {
    Check();
    return text_.size();
  }

  bool empty() const {
    Check();
    return text_.empty();
  }

  bool count() const {
    return count_;
  }

  Error::Code error() const {
    return error_;
  }

  friend std::ostream& operator<<(std::ostream& os, const Format& f) {
    f.Output(os);
    return os;
  }

  template<class T> Format& operator%(const T& arg) {
    Parse *parse(parse_);
    if (!parse) {
      if (error_ == Error::kNone) {
        Throw(Error::kTooManyArguments);
      }
      return *this;
    }
    if (parse->simple_) {
      std::ostringstream os;
      if (!StringStandard(&os, arg)) {
        return *this;
      }
      text_.assign(os.str());
      InitialOutput();
      return *this;
    }
    const Parse::ArgsDefines& defines = *(parse->current_arg_);
    for (Parse::ArgsDefines::const_iterator it(defines.begin());
      it != defines.end(); ++it) {
      Manip *manip(it->manip_);
      std::ostream& os = manip->ostream_;
      Defines::Flags set_these(it->set_these_);
      if ((set_these & Defines::kLocale) != Defines::kNone) {
        if (!SetLocale(&os, arg)) {
          return *this;
        }
        manip->need_ &= ~Defines::kLocale;
      }
      if ((set_these & Defines::kPrecision) != Defines::kNone) {
        if (!SetPrecision(&os, arg)) {
          return *this;
        }
        manip->need_ &= ~Defines::kPrecision;
      }
      if ((set_these & Defines::kWidth) != Defines::kNone) {
        if (!SetWidth(&os, arg)) {
          return *this;
        }
        manip->need_ &= ~Defines::kWidth;
      }
      if ((set_these & Defines::kFill) != Defines::kNone) {
        if (!SetFill(&os, arg)) {
          return *this;
        }
        manip->need_ &= ~Defines::kFill;
      }
      // It is important to process kArg last so that all data could be set
      if ((set_these & Defines::kArg) == Defines::kNone) {
        continue;
      }
      if (manip->need_ != Defines::kNone) {
        Throw(Error::kTooEarlyArgument);
        return *this;
      }
      Extensions::Flags extensions(manip->extensions_);
      if ((extensions & Extensions::kIgnore) != Extensions::kNone) {
        continue;
      }
      if ((extensions & Extensions::kStringNpos) != Extensions::kNone) {
        if (!StringNpos(&os, arg)) {
          return *this;
        }
      } else {
        if (!StringStandard(&os, arg)) {
          return *this;
        }
      }
    }
    if (++(parse->current_arg_) == parse_->args_.end()) {
      FinishInsertingArgs();
    }
    return *this;
  }
};

class Print : public Format {
 public:
  Print(bool *success, const char *format, Special flags)
    : Format(success, stdout, format, flags) {
  }

  Print(bool *success, const std::string& format, Special flags)
    : Format(success, stdout, format, flags) {
  }

  Print(bool *success, char format, Special flags)
    : Format(success, stdout, format, flags) {
  }

  Print(bool *success, bool format, Special flags)
    : Format(success, stdout, format, flags) {
  }

  Print(bool *success, const char *format)
    : Format(success, stdout, format) {
  }

  Print(bool *success, const std::string& format)
    : Format(success, stdout, format) {
  }

  Print(bool *success, char format)
    : Format(success, stdout, format) {
  }

  Print(bool *success, bool format)
    : Format(success, stdout, format) {
  }

  Print(bool *success, Special flags)
    : Format(success, stdout, flags) {
  }

  explicit Print(bool *success)
    : Format(success, stdout) {
  }

  Print(const char *format, Special flags)
    : Format(stdout, format, flags) {
  }

  Print(const std::string& format, Special flags)
    : Format(stdout, format, flags) {
  }

  Print(char format, Special flags)
    : Format(stdout, format, flags) {
  }

  Print(bool format, Special flags)
    : Format(stdout, format, flags) {
  }

  explicit Print(const char *format)
    : Format(stdout, format) {
  }

  explicit Print(const std::string& format)
    : Format(stdout, format) {
  }

  explicit Print(char format)
    : Format(stdout, format) {
  }

  explicit Print(bool format)
    : Format(stdout, format) {
  }

  explicit Print(Special flags)
    : Format(stdout, flags) {
  }

  Print()
    : Format(stdout) {
  }
};

class PrintError : public Format {
 public:
  PrintError(bool *success, const char *format, Special flags)
    : Format(success, stderr, format, flags) {
  }

  PrintError(bool *success, const std::string& format, Special flags)
    : Format(success, stderr, format, flags) {
  }

  PrintError(bool *success, char format, Special flags)
    : Format(success, stderr, format, flags) {
  }

  PrintError(bool *success, bool format, Special flags)
    : Format(success, stderr, format, flags) {
  }

  PrintError(bool *success, const char *format)
    : Format(success, stderr, format) {
  }

  PrintError(bool *success, const std::string& format)
    : Format(success, stderr, format) {
  }

  PrintError(bool *success, char format)
    : Format(success, stderr, format) {
  }

  PrintError(bool *success, bool format)
    : Format(success, stderr, format) {
  }

  PrintError(bool *success, Special flags)
    : Format(success, stderr, flags) {
  }

  explicit PrintError(bool *success)
    : Format(success, stderr) {
  }

  PrintError(const char *format, Special flags)
    : Format(stderr, format, flags) {
  }

  PrintError(const std::string& format, Special flags)
    : Format(stderr, format, flags) {
  }

  PrintError(char format, Special flags)
    : Format(stderr, format, flags) {
  }

  PrintError(bool format, Special flags)
    : Format(stderr, format, flags) {
  }

  explicit PrintError(const char *format)
    : Format(stderr, format) {
  }

  explicit PrintError(const std::string& format)
    : Format(stderr, format) {
  }

  explicit PrintError(char format)
    : Format(stderr, format) {
  }

  explicit PrintError(bool format)
    : Format(stderr, format) {
  }

  explicit PrintError(Special flags)
    : Format(stderr, flags) {
  }

  PrintError()
    : Format(stderr) {
  }
};

class Say : public Format {
 public:
  Say(bool *success, const char *format, Special flags)
    : Format(success, stdout, format, flags | Special::kNewline) {
  }

  Say(bool *success, const std::string& format, Special flags)
    : Format(success, stdout, format, flags | Special::kNewline) {
  }

  Say(bool *success, char format, Special flags)
    : Format(success, stdout, format, flags | Special::kNewline) {
  }

  Say(bool *success, bool format, Special flags)
    : Format(success, stdout, format, flags | Special::kNewline) {
  }

  Say(bool *success, const char *format)
    : Format(success, stdout, format, Special::Newline()) {
  }

  Say(bool *success, const std::string& format)
    : Format(success, stdout, format, Special::Newline()) {
  }

  Say(bool *success, char format)
    : Format(success, stdout, format, Special::Newline()) {
  }

  Say(bool *success, bool format)
    : Format(success, stdout, format, Special::Newline()) {
  }

  Say(bool *success, Special flags)
    : Format(success, stdout, flags | Special::kNewline) {
  }

  explicit Say(bool *success)
    : Format(success, stdout, Special::Newline()) {
  }

  Say(const char *format, Special flags)
    : Format(stdout, format, flags | Special::kNewline) {
  }

  Say(const std::string& format, Special flags)
    : Format(stdout, format, flags | Special::kNewline) {
  }

  Say(char format, Special flags)
    : Format(stdout, format, flags | Special::kNewline) {
  }

  Say(bool format, Special flags)
    : Format(stdout, format, flags | Special::kNewline) {
  }

  explicit Say(const char *format)
    : Format(stdout, format, Special::Newline()) {
  }

  explicit Say(const std::string& format)
    : Format(stdout, format, Special::Newline()) {
  }

  explicit Say(char format)
    : Format(stdout, format, Special::Newline()) {
  }

  explicit Say(bool format)
    : Format(stdout, format, Special::Newline()) {
  }

  explicit Say(Special flags)
    : Format(stdout, flags | Special::kNewline) {
  }

  Say()
    : Format(stdout, Special::Newline()) {
  }
};

class SayError : public Format {
 public:
  SayError(bool *success, const char *format)
    : Format(success, stdout, format, Special::NewlineFlush()) {
  }

  SayError(bool *success, const std::string& format)
    : Format(success, stdout, format, Special::NewlineFlush()) {
  }

  SayError(bool *success, char format)
    : Format(success, stdout, format, Special::NewlineFlush()) {
  }

  SayError(bool *success, bool format)
    : Format(success, stdout, format, Special::NewlineFlush()) {
  }

  explicit SayError(bool *success)
    : Format(success, stdout, Special::NewlineFlush()) {
  }

  explicit SayError(const char *format)
    : Format(stdout, format, Special::NewlineFlush()) {
  }

  explicit SayError(const std::string& format)
    : Format(stdout, format, Special::NewlineFlush()) {
  }

  explicit SayError(char format)
    : Format(stdout, format, Special::NewlineFlush()) {
  }

  explicit SayError(bool format)
    : Format(stdout, format, Special::NewlineFlush()) {
  }

  SayError()
    : Format(stdout, Special::NewlineFlush()) {
  }
};

}  // namespace osformat

#endif  // OSFORMAT_OSFORMAT_H_
