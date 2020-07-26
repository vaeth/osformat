// This file is part of the osformat project and distributed under the
// terms of the GNU General Public License v2.
// SPDX-License-Identifier: GPL-2.0-only
//
// Copyright (c)
//   Martin Väth <martin@mvath.de>

#include "osformat/osformat.h"

#include <cctype>  // isdigit

#include <cstdio>  // fwrite, fflush, fprintf
#include <cstdlib>  // abort, NULL

#include <ios>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

using std::ios_base;
using std::ostream;
using std::streamsize;
using std::string;
using std::vector;

namespace osformat {

const char *Error::Description[] = {
  "",
  "not all data was properly written",
  "flush failed",
  "too many arguments passed (or too few specified)",
  "too few arguments passed (or too many specified)",
  "too early argument, e.g. a width is passed only after the argument",
  "argument for ~ is not a locale",
  "locale argument must not be output",
  "argument for . is not numeric",
  "argument for width is not numeric",
  "argument for fill is not a character",
  "trailing % sign",
  "argument number without trailing $",
  "number overflow",
  "missing specifier",
  "unknown specifier",
  "missing fill character",
};

const Special::Flags
  Special::kNone,
  Special::kNewline,
  Special::kFlush,
  Special::kAll;

const Format::Defines::Flags
  Format::Defines::kNone,
  Format::Defines::kLocale,
  Format::Defines::kPrecision,
  Format::Defines::kWidth,
  Format::Defines::kFill,
  Format::Defines::kArg,
  Format::Defines::kAll;

const Format::Extensions::Flags
  Format::Extensions::kNone,
  Format::Extensions::kIgnore,
  Format::Extensions::kPlusSpace,
  Format::Extensions::kStringNpos,
  Format::Extensions::kAll;

// Declarations of some static helper functions:

inline static string::size_type EndArgumentNumber(const string& s,
    string::size_type start);

static string::size_type EndNumber(const string& s, string::size_type start);

inline static bool IsPositiveNumber(char c);


// Definitions of some static helper functions:

// Return end if s contains at start the expression [1-9][0-9]*\$
// Otherwise return string::npos
inline static string::size_type EndArgumentNumber(const string& s,
    string::size_type start) {
  if (!IsPositiveNumber(s[start])) {
    return string::npos;
  }
  string::size_type end(EndNumber(s, start));
  if ((end != s.size()) && (s[end] == '$')) {
    return end + 1;
  }
  return string::npos;
}

// Find the end of a nonnegative number in s, starting at start.
// It is assumed that the first digit has already been veried to be a number.
static string::size_type EndNumber(const string& s, string::size_type start) {
  for (;;) {
    if (++start == s.size()) {
      return start;
    }
    if (!std::isdigit(s[start])) {
      return start;
    }
  }
}

inline static bool IsPositiveNumber(char c) {
  return (std::isdigit(c) && (c != '0'));
}

Format::Parse::~Parse() {
  for (FormatList::iterator it(format_.begin()); it != format_.end(); ++it) {
    delete *it;
  }
}

void Format::Parse::SetIndirect(size_type argnum,
    Format::Defines::Flags set_these, Format::Manip *manip) {
  ArgsDefines& args_defines = args_[argnum];
  for (ArgsDefines::iterator it(args_defines.begin());
    it != args_defines.end(); ++it) {
    if (it->manip_ == manip) {
      it->set_these_ |= set_these;
      return;
    }
  }
#if __cplusplus >= 201103L
    args_defines.emplace_back(set_these, manip);
#else
    args_defines.push_back(References(set_these, manip));
#endif
}

void Format::Throw(Error::Code error) const {
  if (abort_) {
    std::fprintf(stderr, "osformat \"%s\": %s\n", text_.c_str(),
      Error::c_str(error));
    std::fflush(stderr);
    std::abort();
  }
  error_ = error;
  if (success_ != NULL) {
    *success_ = false;
  }
  delete parse_;
  const_cast<Format *>(this)->parse_ = NULL;
}

void Format::Init(string *append, FILE *file, ostream *ostream, bool format) {
  if (abort_) {
    success_ = NULL;
  }
  Parse *parse = parse_ = new Parse(true, append, file, ostream);
  if (!format) {
    InitialOutput();
    return;
  }
  parse->current_arg_ = parse->args_.begin();
  error_ = Error::kTooFewArguments;
  if (success_ != NULL) {
    *success_ = false;
  }
}

void Format::Init(string *append, FILE *file, ostream *ostream) {
  if (abort_) {
    success_ = NULL;
  }
  Parse *parse = parse_ = new Parse(false, append, file, ostream);
  if (!ParseFormat()) {
    return;
  }
  Parse::ArgsList& args = parse->args_;
  if (args.empty()) {
    InitialOutput();
    return;
  }
  parse->current_arg_ = parse->args_.begin();
  error_ = Error::kTooFewArguments;
  if (success_ != NULL) {
    *success_ = false;
  }
}

bool Format::ParseFormat() {
  Parse *parse = parse_;
  SpecifiedList specified;
  Parse::ArgsDefines define_queue;
  string::size_type i(0);

  // Parse the format, maintaining the specified numbers,
  // postponing to define_queue what is not yet specified
  while (i = text_.find('%', i), i != string::npos) {
    string::size_type start(i);
    if (++i == text_.size()) {
      Throw(Error::kTrailingPercentage);
      return false;
    }
    char c(text_[i]);
    if (c == '%') {
      text_.erase(i, 1);
      if (i != text_.size()) {
        continue;
      }
      break;
    }
    parse->borders_.push_back(start);
    Manip *manip = new Manip();
    parse->format_.push_back(manip);
    bool unknown_number(true);
    {
      string::size_type end(EndArgumentNumber(text_, i));
      if (end != string::npos) {
        if (end == text_.size()) {
          Throw(Error::kMissingSpecifier);
          return false;
        }
        unknown_number = false;
        Parse::size_type argnum;
        if (!ParseNumber(&argnum, i, end - 1)) {
          return false;
        }
        HandleArgumentNumber(--argnum, &specified);
        parse->SetIndirect(argnum, Defines::kArg, manip);
        c = text_[(i = end)];
      }
    }
    bool got_specifier(false);
    for (;; c = text_[i]) {
      switch (c) {
        case '#':
          manip->ostream_.setf(ios_base::showbase);
          break;
        case ' ':
          manip->extensions_ |= Extensions::kPlusSpace;
        case '+':
          manip->ostream_.setf(ios_base::showpos);
          break;
        case '0':
          manip->ostream_.fill('0');
          break;
        case '_':
          if (++i == text_.size()) {
            Throw(Error::kMissingFillCharacter);
            return false;
          }
          manip->ostream_.fill(text_[i]);
          break;
        case '/':
          if (SetArg(Defines::kFill, &define_queue, &specified, manip, &i)) {
            continue;
          }
          return false;
        case '-':
          manip->ostream_.setf(ios_base::left, ios_base::adjustfield);
          break;
        case ':':
          manip->ostream_.setf(ios_base::internal, ios_base::adjustfield);
          break;
        case '*':
          if (SetArg(Defines::kWidth, &define_queue, &specified, manip, &i)) {
            continue;
          }
          return false;
        case '.':
          if (++i == text_.size()) {
            Throw(Error::kMissingSpecifier);
            return false;
          }
          c = text_[i];
          if (IsPositiveNumber(c)) {
            string::size_type end(EndNumber(text_, i));
            if (end == text_.size()) {
              Throw(Error::kMissingSpecifier);
              return false;
            }
            streamsize precision;
            if (!ParseNumber(&precision, i, end)) {
              return false;
            }
            manip->ostream_.precision(precision);
            i = end;
            continue;
          }
          if (c == '*') {
            if (SetArg(Defines::kPrecision, &define_queue, &specified, manip,
              &i)) {
              continue;
            }
            return false;
          }
          // a plain . is admissible and interpreted as precision 0
         manip->ostream_.precision(0);
         break;
        case '~':
          if (SetArg(Defines::kLocale, &define_queue, &specified, manip, &i)) {
            continue;
          }
          return false;
          break;
        case 'n':
          got_specifier = true;
          manip->extensions_ |= Extensions::kIgnore;
          break;
        case 's':
          got_specifier = true;
          break;
        case 'S':
          got_specifier = true;
          manip->ostream_.setf(ios_base::boolalpha | ios_base::showpoint);
          manip->extensions_ |= Extensions::kStringNpos;
          break;
        case 'd':
          got_specifier = true;
          manip->ostream_.setf(ios_base::boolalpha);
          manip->extensions_ |= Extensions::kStringNpos;
          break;
        case 'D':
          got_specifier = true;
          manip->ostream_.setf(ios_base::boolalpha | ios_base::uppercase);
          manip->extensions_ |= Extensions::kStringNpos;
          break;
        case 'x':
          got_specifier = true;
          manip->ostream_.setf(ios_base::hex, ios_base::basefield);
          break;
        case 'X':
          got_specifier = true;
          manip->ostream_.setf(ios_base::hex | ios_base::uppercase,
            ios_base::basefield | ios_base::uppercase);
          break;
        case 'o':
          got_specifier = true;
          manip->ostream_.setf(ios_base::oct, ios_base::basefield);
          break;
        case 'O':
          got_specifier = true;
          manip->ostream_.setf(ios_base::oct | ios_base::uppercase,
            ios_base::basefield | ios_base::uppercase);
          break;
        case 'f':
          got_specifier = true;
          manip->ostream_.setf(ios_base::fixed);
          break;
        case 'F':
          got_specifier = true;
          manip->ostream_.setf(ios_base::fixed | ios_base::uppercase);
          break;
        case 'e':
          got_specifier = true;
          manip->ostream_.setf(ios_base::scientific);
          break;
        case 'E':
          got_specifier = true;
          manip->ostream_.setf(ios_base::scientific |
            ios_base::uppercase);
          break;
        case 'a':
          got_specifier = true;
          manip->ostream_.setf(ios_base::fixed |
            ios_base::scientific);
          break;
        case 'A':
          got_specifier = true;
          manip->ostream_.setf(ios_base::fixed |
            ios_base::scientific | ios_base::uppercase);
          break;
        default:
          if (!std::isdigit(c)) {
            Throw(Error::kUnknownSpecifier);
            return false;
          }
          string::size_type end(EndNumber(text_, i));
          if (end == text_.size()) {
            Throw(Error::kMissingSpecifier);
            return false;
          }
          streamsize width;
          if (!ParseNumber(&width, i, end)) {
            return false;
          }
          manip->ostream_.width(width);
          i = end;
          continue;
      }
      if (got_specifier) {
        ++i;
        break;
      }
      if (++i == text_.size()) {
        Throw(Error::kMissingSpecifier);
        return false;
      }
    }
    if (unknown_number) {
#if __cplusplus >= 201103L
      define_queue.emplace_back(Defines::kArg, manip);
#else
      define_queue.push_back(References(Defines::kArg, manip));
#endif
    }
    parse->borders_.push_back(i);
  }

  // Give an argnumber to the postponed requests
  if (!define_queue.empty()) {
    Parse::size_type total_args(define_queue.size());
    for (SpecifiedList::const_iterator it(specified.begin());
      it != specified.end(); ++it) {
      if (*it) {
        ++total_args;
      }
    }
    parse->args_.resize(total_args);
    Parse::size_type argnum(0);
    for (Parse::ArgsDefines::const_iterator it(define_queue.begin());
      it != define_queue.end(); ++it) {
      while ((argnum < specified.size()) && specified[argnum]) {
        ++argnum;
      }
      parse->SetIndirect(argnum++, it->set_these_, it->manip_);
    }
  }
  return true;
}

// Setup set_these to be filled from an argument.
// Assumes that index is one before number; at return it is after number.
// Return true if no error
bool Format::SetArg(Defines::Flags set_these,
    Parse::ArgsDefines *define_queue,
    SpecifiedList *specified, Manip *manip,
    string::size_type *index) {
  manip->need_ |= set_these;
  if (++(*index) == text_.size()) {
    Throw(Error::kMissingSpecifier);
    return false;
  }
  string::size_type end(EndArgumentNumber(text_, *index));
  if (end != string::npos) {
    if (end == text_.size()) {
      Throw(Error::kMissingSpecifier);
      return false;
    }
    Parse::size_type argnum;
    if (!ParseNumber(&argnum, *index, end - 1)) {
      return false;
    }
    *index = end;
    HandleArgumentNumber(--argnum, specified);
    parse_->SetIndirect(argnum, set_these, manip);
  } else {
#if __cplusplus >= 201103L
      define_queue->emplace_back(set_these, manip);
#else
      define_queue->push_back(References(set_these, manip));
#endif
  }
  return true;
}

// Store argnum in specified and resize args_
void Format::HandleArgumentNumber(Parse::size_type argnum,
    SpecifiedList *specified) {
  if (specified->size() <= argnum) {
    specified->resize(argnum + 1);
    parse_->args_.resize(argnum + 1);
  }
  (*specified)[argnum] = true;
}

void Format::FinishInsertingArgs() {
  Parse& parse = *parse_;
  string result;
  Parse::FormatList& formats = parse.format_;
  Parse::FormatList::const_iterator format_iterator(formats.begin());
  Parse::BorderList::const_iterator it(parse.borders_.begin());
  string::size_type current_pos(0);
  for (; format_iterator!= formats.end();
    ++format_iterator, ++it, current_pos = *it, ++it) {
    result.append(text_, current_pos,
      static_cast<string::size_type>(*it - current_pos));
    Manip *manip(*format_iterator);
    Extensions::Flags extensions(manip->extensions_);
    if ((extensions & Extensions::kIgnore) != Extensions::kNone) {
      continue;
    }
    if ((extensions & Extensions::kPlusSpace) == Extensions::kNone) {
      result.append(manip->ostream_.str());
      continue;
    }
    string string_with_plus(manip->ostream_.str());
    string::size_type plus(string_with_plus.find('+'));
    if (plus != string::npos) {
      string_with_plus[plus] = ' ';
    }
    result.append(string_with_plus);
  }
  if (current_pos != string::npos) {
    result.append(text_, current_pos, string::npos);
  }
  text_.assign(result);
  InitialOutput();
}

void Format::OutputInternal(string *append) const {
  append->append(text_);
  error_ = Error::kNone;
  if (success_ != NULL) {
    *success_ = true;
  }
}

void Format::OutputInternal(FILE *file) const {
  bool success(true);
  error_ = Error::kNone;
  count_ = 0;
  if (!text_.empty()) {
    count_ = std::fwrite(text_.c_str(), sizeof(char), text_.size(), file);
    if (count_ < text_.size()) {
      Throw(Error::kWriteFailed);
      success = false;
    }
    if (success && flush()) {
      if (std::fflush(file) != 0) {
        Throw(Error::kFlushFailed);
        success = false;
      }
    }
  }
  if (success_ != NULL) {
    *success_ = success;
  }
}

void Format::OutputInternal(ostream& ostream) const {
  bool success(true);
  error_ = Error::kNone;
  if (!text_.empty()) {
    ostream << text_;
    if (ostream.bad()) {
      Throw(Error::kWriteFailed);
      success = false;
    }
    if (success && flush()) {
      ostream.flush();
      if (ostream.bad()) {
        Throw(Error::kFlushFailed);
        success = false;
      }
    }
  }
  if (success_ != NULL) {
    *success_ = success;
  }
}

void Format::InitialOutput() {
  if (flags_.HaveBits(Special::kNewline)) {
    text_.append(1, '\n');
  }
  if (parse_->append_) {
    OutputInternal(parse_->append_);
  } else if (parse_->file_) {
    OutputInternal(parse_->file_);
  } else if (parse_->ostream_) {
    OutputInternal(*(parse_->ostream_));
  } else {
    error_ = Error::kNone;
    if (success_ != NULL) {
      *success_ = true;
    }
  }
  delete parse_;
  parse_ = NULL;
}

}  // namespace osformat
