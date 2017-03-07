// This file is part of the osformat project and distributed under the
// terms of the GNU General Public License v2.
//
// Copyright (c)
//   Martin Väth <martin@mvath.de>

#include "osformat/osformat.h"

#include <iostream>
#include <locale>
#include <ostream>
#include <sstream>
#include <string>

using std::cout;
using std::locale;
using std::ostringstream;
using std::string;

using osformat::Error;
using osformat::Format;
using osformat::Print;
using osformat::PrintError;
using osformat::Say;
using osformat::SayError;
using osformat::Special;

int main() {
  string r = Format("Result %*d", Special::Newline()) % 2 % 1;
  r.append(Format("%2$*1$d\n") % 9 % 1);
  (Format(&r, "%2$*1$d\n") % 9 % 1);
  cout << Format("%*2$.*E\n") % 3 % 7 % (1./7) << r;
  if (
    (r != "Result  1\n        1\n        1\n") ||
    ((Say("%1$s = %1$#x") % 15).str() == "15 = 0xf") ||
    ((Say("%*1s%*2s\n") % 2 % 3 % 4 % 5).str() == " 4  5\n") ||
    ((Say("%*s %*s") % 2 % 4 % 3 % 5).str() == " 4  5\n") ||
    ((Say() % -1).str() == "-1") ||
    (string(Say("Hello%%", Special::Flush())) != "Hello%\n") ||
    ((Format(stdout, Special::Newline()) % "you").str()
      != "you\n") ||
    ((Say("%s") % "Hello").str() != "Hello\n") ||
    ((Say("%s %s") % "Hello" % "you").str() != "Hello you\n") ||
    ((Say("%2$s %1$s") % "you" % "Hello").str() != "Hello you\n") ||
    ((Say("%2$s%s%s") % "lo" % "Hel" % " you").str() != "Hello you\n") ||
    ((Say("%*s") % false % "Hello").str() != "Hello\n") ||
    ((Say("%*s") % 7 % "Hello").str() != "  Hello\n") ||
    ((Say("%*s") % ('H' - 'A') % "Hello").str() != "  Hello\n") ||
    ((Say("%/*s") % 'x' % 7 % "Hello").str() != "xxHello\n") ||
    ((Say("%_x*s") % 7 % "Hello").str() != "xxHello\n") ||
    ((Say("%/1$*s") % 'x' % 7 % "Hello").str() != "xxHello\n") ||
    ((Say("%2$/2$*s") % 7 % 'x').str() != "xxxxxxx\n") ||
    ((Say("%0*s") % 7 % "Hello").str() != "00Hello\n") ||
    ((Say("%1$s %1$s") % "Hello").str() != "Hello Hello\n") ||
    ((Say("%S %S") % 17 % string::npos).str() != "17 std::string::npos\n") ||
    ((Say("%s %1$s") % 'b' % 'a').str() != "a b\n") ||
    ((Say("foo has value '%s'") % 17.5).str() != "foo has value '17.5'\n") ||
    ((Say("%2$s %s") % 0 % "file").str() != "file 0\n") ||
    ((Say("%s %1$s") % "rose" % "Rose").str() != "Rose rose\n") ||
    ((Say("A %1$s is a %1$s") % "Rose").str() != "A Rose is a Rose\n") ||
    ((Say("%.3e") % (1./7)).str() != "1.429e-01\n") ||
    ((Say("%.*e") % 3 % (1./7)).str() != "1.429e-01\n") ||
    ((Say("%.3E") % (1./7)).str() != "1.429E-01\n") ||
    ((Say("%.3E") % (1./7)).str() != "1.429E-01\n") ||
    ((Say("%04.1f") % (1./7)).str() != "00.1\n") ||
    ((Say("%+.1F") % (1./7)).str() != "+0.1\n") ||
    ((Say("% 05.1F") % (1./7)).str() != "0 0.1\n") ||
    ((Say("%.2f") % .5).str() != "0.50\n") ||
    ((Say("%.2s") % .5).str() != "0.5\n") ||
    ((Say("%.2a") % .5).str() != "0x1p-1\n") ||
    ((Say("%.2A") % .5).str() != "0X1P-1\n") ||
    ((Say("%~d") % locale("de_DE") % .5).str() != "0,5\n") ||
    ((Say("%~1$d") % locale("de_DE") % .5).str() != "0,5\n") ||
    ((Say("%x") % 15).str() != "f\n") ||
    ((Say("%#x") % 15).str() != "0xf\n") ||
    ((Say("%#X") % 15).str() != "0XF\n") ||
    ((Say("%o") % 8).str() != "10\n") ||
    ((Say("%#O") % 8).str() != "010\n") ||
    ((Say("empty%1$n%n") % 1 % 2).str() != "empty\n") ||
    ((Say("A: %*s B: %*s") % 2 % 4 % 3 % 5).str() != "A:  4 B:   5\n") ||
    false) {
    return 1;
  }
  bool ok(true);
  Say a(&ok, "%1$*2$s");
  a % 1 % 2;
  if (ok || (a.error() != Error::kTooEarlyArgument)) {
    return 1;
  }
  ostringstream os;
  os << Say("Hello");
  cout << Print(Special::NewlineFlush()) % "FOO";
  Say("All tests passed");
  return 0;
}
