/* ANSI-C code produced by gperf version 3.0.4 */
/* Command-line: gperf -m 10 lib/aliases.gperf  */
/* Computed positions: -k'1,4-7,10,$' */

#if !((' ' == 32) && ('!' == 33) && ('"' == 34) && ('#' == 35) \
      && ('%' == 37) && ('&' == 38) && ('\'' == 39) && ('(' == 40) \
      && (')' == 41) && ('*' == 42) && ('+' == 43) && (',' == 44) \
      && ('-' == 45) && ('.' == 46) && ('/' == 47) && ('0' == 48) \
      && ('1' == 49) && ('2' == 50) && ('3' == 51) && ('4' == 52) \
      && ('5' == 53) && ('6' == 54) && ('7' == 55) && ('8' == 56) \
      && ('9' == 57) && (':' == 58) && (';' == 59) && ('<' == 60) \
      && ('=' == 61) && ('>' == 62) && ('?' == 63) && ('A' == 65) \
      && ('B' == 66) && ('C' == 67) && ('D' == 68) && ('E' == 69) \
      && ('F' == 70) && ('G' == 71) && ('H' == 72) && ('I' == 73) \
      && ('J' == 74) && ('K' == 75) && ('L' == 76) && ('M' == 77) \
      && ('N' == 78) && ('O' == 79) && ('P' == 80) && ('Q' == 81) \
      && ('R' == 82) && ('S' == 83) && ('T' == 84) && ('U' == 85) \
      && ('V' == 86) && ('W' == 87) && ('X' == 88) && ('Y' == 89) \
      && ('Z' == 90) && ('[' == 91) && ('\\' == 92) && (']' == 93) \
      && ('^' == 94) && ('_' == 95) && ('a' == 97) && ('b' == 98) \
      && ('c' == 99) && ('d' == 100) && ('e' == 101) && ('f' == 102) \
      && ('g' == 103) && ('h' == 104) && ('i' == 105) && ('j' == 106) \
      && ('k' == 107) && ('l' == 108) && ('m' == 109) && ('n' == 110) \
      && ('o' == 111) && ('p' == 112) && ('q' == 113) && ('r' == 114) \
      && ('s' == 115) && ('t' == 116) && ('u' == 117) && ('v' == 118) \
      && ('w' == 119) && ('x' == 120) && ('y' == 121) && ('z' == 122) \
      && ('{' == 123) && ('|' == 124) && ('}' == 125) && ('~' == 126))
/* The character set is not based on ISO-646.  */
#error "gperf generated tables don't work with this execution character set. Please report a bug to <bug-gnu-gperf@gnu.org>."
#endif

#line 1 "lib/aliases.gperf"
struct alias { int name; unsigned int encoding_index; };

#define TOTAL_KEYWORDS 78
#define MIN_WORD_LENGTH 2
#define MAX_WORD_LENGTH 17
#define MIN_HASH_VALUE 9
#define MAX_HASH_VALUE 111
/* maximum key range = 103, duplicates = 0 */

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
static unsigned int
aliases_hash (register const char *str, register unsigned int len)
{
  static const unsigned char asso_values[] =
    {
      112, 112, 112, 112, 112, 112, 112, 112, 112, 112,
      112, 112, 112, 112, 112, 112, 112, 112, 112, 112,
      112, 112, 112, 112, 112, 112, 112, 112, 112, 112,
      112, 112, 112, 112, 112, 112, 112, 112, 112, 112,
      112, 112, 112, 112, 112,   3, 112, 112,  13,  15,
        4,  13,   3,   5,   4,  16,  20,  13, 112, 112,
      112, 112, 112, 112, 112,  42,  19,   4,  20,   3,
      112,   7, 112,   3, 112,   4,   4,  13,   3,  21,
        9, 112,  20,   8,   6,   3, 112,   3,   3, 112,
      112, 112, 112, 112, 112,  12, 112, 112, 112, 112,
      112, 112, 112, 112, 112, 112, 112, 112, 112, 112,
      112, 112, 112, 112, 112, 112, 112, 112, 112, 112,
      112, 112, 112, 112, 112, 112, 112, 112
    };
  register int hval = len;

  switch (hval)
    {
      default:
        hval += asso_values[(unsigned char)str[9]];
      /*FALLTHROUGH*/
      case 9:
      case 8:
      case 7:
        hval += asso_values[(unsigned char)str[6]];
      /*FALLTHROUGH*/
      case 6:
        hval += asso_values[(unsigned char)str[5]];
      /*FALLTHROUGH*/
      case 5:
        hval += asso_values[(unsigned char)str[4]];
      /*FALLTHROUGH*/
      case 4:
        hval += asso_values[(unsigned char)str[3]];
      /*FALLTHROUGH*/
      case 3:
      case 2:
      case 1:
        hval += asso_values[(unsigned char)str[0]];
        break;
    }
  return hval + asso_values[(unsigned char)str[len - 1]];
}

struct stringpool_t
  {
    char stringpool_str9[sizeof("CN")];
    char stringpool_str10[sizeof("L2")];
    char stringpool_str13[sizeof("US")];
    char stringpool_str14[sizeof("GBK")];
    char stringpool_str17[sizeof("UCS-4")];
    char stringpool_str18[sizeof("EUCCN")];
    char stringpool_str19[sizeof("UCS-2")];
    char stringpool_str21[sizeof("L1")];
    char stringpool_str22[sizeof("EUC-CN")];
    char stringpool_str24[sizeof("LATIN2")];
    char stringpool_str26[sizeof("UCS-4LE")];
    char stringpool_str27[sizeof("UCS-2LE")];
    char stringpool_str28[sizeof("CSUCS4")];
    char stringpool_str29[sizeof("ISO646-CN")];
    char stringpool_str30[sizeof("CP936")];
    char stringpool_str31[sizeof("CHINESE")];
    char stringpool_str32[sizeof("CSASCII")];
    char stringpool_str33[sizeof("UTF-32")];
    char stringpool_str34[sizeof("ISO646-US")];
    char stringpool_str35[sizeof("UTF-16")];
    char stringpool_str36[sizeof("UCS-4-INTERNAL")];
    char stringpool_str37[sizeof("UCS-2-INTERNAL")];
    char stringpool_str38[sizeof("UTF-32LE")];
    char stringpool_str39[sizeof("MS936")];
    char stringpool_str40[sizeof("UTF-16LE")];
    char stringpool_str41[sizeof("UCS-4BE")];
    char stringpool_str42[sizeof("UCS-2BE")];
    char stringpool_str43[sizeof("UTF-7")];
    char stringpool_str44[sizeof("ISO-IR-6")];
    char stringpool_str45[sizeof("CP367")];
    char stringpool_str46[sizeof("LATIN1")];
    char stringpool_str47[sizeof("CSUNICODE")];
    char stringpool_str48[sizeof("CHAR")];
    char stringpool_str49[sizeof("GB2312")];
    char stringpool_str50[sizeof("CP819")];
    char stringpool_str51[sizeof("UTF-8")];
    char stringpool_str53[sizeof("UTF-32BE")];
    char stringpool_str54[sizeof("CN-GB")];
    char stringpool_str55[sizeof("UTF-16BE")];
    char stringpool_str56[sizeof("ASCII")];
    char stringpool_str57[sizeof("ISO-IR-57")];
    char stringpool_str58[sizeof("IBM367")];
    char stringpool_str59[sizeof("ISO-10646-UCS-4")];
    char stringpool_str60[sizeof("ISO-10646-UCS-2")];
    char stringpool_str61[sizeof("ISO-IR-58")];
    char stringpool_str62[sizeof("UCS-4-SWAPPED")];
    char stringpool_str63[sizeof("UCS-2-SWAPPED")];
    char stringpool_str67[sizeof("CSGB2312")];
    char stringpool_str68[sizeof("ISO-IR-100")];
    char stringpool_str69[sizeof("ISO-8859-2")];
    char stringpool_str70[sizeof("IBM819")];
    char stringpool_str71[sizeof("US-ASCII")];
    char stringpool_str72[sizeof("ISO-IR-101")];
    char stringpool_str73[sizeof("UNICODELITTLE")];
    char stringpool_str74[sizeof("ISO8859-2")];
    char stringpool_str75[sizeof("UNICODEBIG")];
    char stringpool_str76[sizeof("CSUNICODE11")];
    char stringpool_str77[sizeof("ISO_646.IRV:1991")];
    char stringpool_str78[sizeof("ISO_8859-2")];
    char stringpool_str79[sizeof("GB_2312-80")];
    char stringpool_str80[sizeof("UNICODE-1-1")];
    char stringpool_str81[sizeof("CSUNICODE11UTF7")];
    char stringpool_str83[sizeof("WINDOWS-936")];
    char stringpool_str85[sizeof("ISO8859-1")];
    char stringpool_str86[sizeof("GB18030")];
    char stringpool_str87[sizeof("UNICODE-1-1-UTF-7")];
    char stringpool_str90[sizeof("CSISO58GB231280")];
    char stringpool_str91[sizeof("ISO-8859-1")];
    char stringpool_str94[sizeof("ANSI_X3.4-1986")];
    char stringpool_str95[sizeof("ISO_8859-2:1987")];
    char stringpool_str96[sizeof("WCHAR_T")];
    char stringpool_str97[sizeof("CSISOLATIN2")];
    char stringpool_str100[sizeof("ISO_8859-1")];
    char stringpool_str102[sizeof("CSISO57GB1988")];
    char stringpool_str106[sizeof("ISO_8859-1:1987")];
    char stringpool_str108[sizeof("CSISOLATIN1")];
    char stringpool_str110[sizeof("ANSI_X3.4-1968")];
    char stringpool_str111[sizeof("GB_1988-80")];
  };
static const struct stringpool_t stringpool_contents =
  {
    "CN",
    "L2",
    "US",
    "GBK",
    "UCS-4",
    "EUCCN",
    "UCS-2",
    "L1",
    "EUC-CN",
    "LATIN2",
    "UCS-4LE",
    "UCS-2LE",
    "CSUCS4",
    "ISO646-CN",
    "CP936",
    "CHINESE",
    "CSASCII",
    "UTF-32",
    "ISO646-US",
    "UTF-16",
    "UCS-4-INTERNAL",
    "UCS-2-INTERNAL",
    "UTF-32LE",
    "MS936",
    "UTF-16LE",
    "UCS-4BE",
    "UCS-2BE",
    "UTF-7",
    "ISO-IR-6",
    "CP367",
    "LATIN1",
    "CSUNICODE",
    "CHAR",
    "GB2312",
    "CP819",
    "UTF-8",
    "UTF-32BE",
    "CN-GB",
    "UTF-16BE",
    "ASCII",
    "ISO-IR-57",
    "IBM367",
    "ISO-10646-UCS-4",
    "ISO-10646-UCS-2",
    "ISO-IR-58",
    "UCS-4-SWAPPED",
    "UCS-2-SWAPPED",
    "CSGB2312",
    "ISO-IR-100",
    "ISO-8859-2",
    "IBM819",
    "US-ASCII",
    "ISO-IR-101",
    "UNICODELITTLE",
    "ISO8859-2",
    "UNICODEBIG",
    "CSUNICODE11",
    "ISO_646.IRV:1991",
    "ISO_8859-2",
    "GB_2312-80",
    "UNICODE-1-1",
    "CSUNICODE11UTF7",
    "WINDOWS-936",
    "ISO8859-1",
    "GB18030",
    "UNICODE-1-1-UTF-7",
    "CSISO58GB231280",
    "ISO-8859-1",
    "ANSI_X3.4-1986",
    "ISO_8859-2:1987",
    "WCHAR_T",
    "CSISOLATIN2",
    "ISO_8859-1",
    "CSISO57GB1988",
    "ISO_8859-1:1987",
    "CSISOLATIN1",
    "ANSI_X3.4-1968",
    "GB_1988-80"
  };
#define stringpool ((const char *) &stringpool_contents)

static const struct alias aliases[] =
  {
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
#line 72 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str9, ei_iso646_cn},
#line 66 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str10, ei_iso8859_2},
    {-1}, {-1},
#line 21 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str13, ei_ascii},
#line 83 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str14, ei_ces_gbk},
    {-1}, {-1},
#line 33 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str17, ei_ucs4},
#line 79 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str18, ei_euc_cn},
#line 24 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str19, ei_ucs2},
    {-1},
#line 58 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str21, ei_iso8859_1},
#line 78 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str22, ei_euc_cn},
    {-1},
#line 65 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str24, ei_iso8859_2},
    {-1},
#line 37 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str26, ei_ucs4le},
#line 31 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str27, ei_ucs2le},
#line 35 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str28, ei_ucs4},
#line 70 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str29, ei_iso646_cn},
#line 84 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str30, ei_cp936},
#line 77 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str31, ei_gb2312},
#line 22 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str32, ei_ascii},
#line 41 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str33, ei_utf32},
#line 14 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str34, ei_ascii},
#line 38 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str35, ei_utf16},
#line 49 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str36, ei_ucs4internal},
#line 47 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str37, ei_ucs2internal},
#line 43 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str38, ei_utf32le},
#line 85 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str39, ei_cp936},
#line 40 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str40, ei_utf16le},
#line 36 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str41, ei_ucs4be},
#line 27 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str42, ei_ucs2be},
#line 44 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str43, ei_utf7},
#line 16 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str44, ei_ascii},
#line 19 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str45, ei_ascii},
#line 57 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str46, ei_iso8859_1},
#line 26 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str47, ei_ucs2},
#line 88 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str48, ei_local_char},
#line 80 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str49, ei_euc_cn},
#line 55 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str50, ei_iso8859_1},
#line 23 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str51, ei_utf8},
    {-1},
#line 42 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str53, ei_utf32be},
#line 81 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str54, ei_euc_cn},
#line 39 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str55, ei_utf16be},
#line 13 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str56, ei_ascii},
#line 71 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str57, ei_iso646_cn},
#line 20 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str58, ei_ascii},
#line 34 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str59, ei_ucs4},
#line 25 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str60, ei_ucs2},
#line 75 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str61, ei_gb2312},
#line 50 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str62, ei_ucs4swapped},
#line 48 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str63, ei_ucs2swapped},
    {-1}, {-1}, {-1},
#line 82 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str67, ei_euc_cn},
#line 54 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str68, ei_iso8859_1},
#line 61 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str69, ei_iso8859_2},
#line 56 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str70, ei_iso8859_1},
#line 12 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str71, ei_ascii},
#line 64 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str72, ei_iso8859_2},
#line 32 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str73, ei_ucs2le},
#line 68 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str74, ei_iso8859_2},
#line 28 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str75, ei_ucs2be},
#line 30 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str76, ei_ucs2be},
#line 15 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str77, ei_ascii},
#line 62 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str78, ei_iso8859_2},
#line 74 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str79, ei_gb2312},
#line 29 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str80, ei_ucs2be},
#line 46 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str81, ei_utf7},
    {-1},
#line 86 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str83, ei_cp936},
    {-1},
#line 60 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str85, ei_iso8859_1},
#line 87 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str86, ei_gb18030},
#line 45 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str87, ei_utf7},
    {-1}, {-1},
#line 76 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str90, ei_gb2312},
#line 51 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str91, ei_iso8859_1},
    {-1}, {-1},
#line 18 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str94, ei_ascii},
#line 63 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str95, ei_iso8859_2},
#line 89 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str96, ei_local_wchar_t},
#line 67 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str97, ei_iso8859_2},
    {-1}, {-1},
#line 52 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str100, ei_iso8859_1},
    {-1},
#line 73 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str102, ei_iso646_cn},
    {-1}, {-1}, {-1},
#line 53 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str106, ei_iso8859_1},
    {-1},
#line 59 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str108, ei_iso8859_1},
    {-1},
#line 17 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str110, ei_ascii},
#line 69 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str111, ei_iso646_cn}
  };

#ifdef __GNUC__
__inline
#if defined __GNUC_STDC_INLINE__ || defined __GNUC_GNU_INLINE__
__attribute__ ((__gnu_inline__))
#endif
#endif
const struct alias *
aliases_lookup (register const char *str, register unsigned int len)
{
  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = aliases_hash (str, len);

      if (key <= MAX_HASH_VALUE && key >= 0)
        {
          register int o = aliases[key].name;
          if (o >= 0)
            {
              register const char *s = o + stringpool;

              if (*str == *s && !strcmp (str + 1, s + 1))
                return &aliases[key];
            }
        }
    }
  return 0;
}
