#include "base/assert.hpp"
#include "base/macros.hpp"
#include "base/timegm.hpp"
#include "base/timer.hpp"

#include "std/systime.hpp"
#include "std/target_os.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <iomanip>
#include <sstream>

#ifndef OMIM_OS_LINUX
using std::get_time;
using std::put_time;
#else
#include <cassert>

namespace detail
{
  template <class _CharT> struct get_time_manip
  {
    tm* __tm_;
    const _CharT* __fmt_;

    get_time_manip(tm* __tm, const _CharT* __fmt)
        : __tm_(__tm), __fmt_(__fmt) {}
  };

  template <class _CharT, class _Traits>
  class stream_buf_impl : public std::basic_streambuf<_CharT, _Traits>
  {
    typedef std::basic_streambuf<_CharT, _Traits> base_t;
  public:
    bool parse(const get_time_manip<_CharT>& __x)
    {
      // Workaround works only for a stream buffer under null-terminated string.
      assert(*base_t::egptr() == 0);

      char * res = ::strptime(base_t::gptr(), __x.__fmt_, __x.__tm_);
      if (res == 0)
        return false;
      else
      {
        base_t::setg(base_t::eback(), res, base_t::egptr());
        return true;
      }
    }
  };

  template <class _CharT, class _Traits>
  std::basic_istream<_CharT, _Traits>&
  operator>>(std::basic_istream<_CharT, _Traits>& __is, const get_time_manip<_CharT>& __x)
  {
    if (!reinterpret_cast<stream_buf_impl<_CharT, _Traits>*>(__is.rdbuf())->parse(__x))
      __is.setstate(std::ios_base::failbit);
    return __is;
  }
}

template <class _CharT>
detail::get_time_manip<_CharT> get_time(tm* __tm, const _CharT* __fmt)
{
  return detail::get_time_manip<_CharT>(__tm, __fmt);
}

#endif



namespace my
{

Timer::Timer(bool start/* = true*/)
{
  if (start)
    Reset();
}

// static
double Timer::LocalTime()
{
#ifdef OMIM_OS_WINDOWS_NATIVE
  FILETIME ft;
  GetSystemTimeAsFileTime(&ft);
  uint64_t val = ft.dwHighDateTime;
  val <<= 32;
  val += ft.dwLowDateTime;
  return val / 10000000.0;

#else
  timeval tv;
  ::gettimeofday(&tv, 0);
  return tv.tv_sec + tv.tv_usec / 1000000.0;
#endif
}

std::string FormatCurrentTime()
{
  time_t t = time(NULL);
  std::string s(ctime(&t));

  replace(s.begin(), s.end(), ' ', '_');

  ASSERT_GREATER(s.size(), 1, ());
  s.resize(s.size() - 1);
  return s;
}

uint32_t GenerateYYMMDD(int year, int month, int day)
{
  uint32_t result = (year - 100) * 100;
  result = (result + month + 1) * 100;
  result = result + day;
  return result;
}

uint64_t SecondsSinceEpoch()
{
  return TimeTToSecondsSinceEpoch(::time(nullptr));
}

std::string TimestampToString(time_t time)
{
  if (time == INVALID_TIME_STAMP)
    return std::string("INVALID_TIME_STAMP");

  tm * t = gmtime(&time);
  char buf[21] = { 0 };
#ifdef OMIM_OS_WINDOWS
  sprintf_s(buf, ARRAY_SIZE(buf), "%04d-%02d-%02dT%02d:%02d:%02dZ", t->tm_year + 1900,
            t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
#else
  ::snprintf(buf, ARRAY_SIZE(buf), "%04d-%02d-%02dT%02d:%02d:%02dZ", t->tm_year + 1900,
             t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
#endif

  return buf;
}

namespace
{
bool IsValid(tm const & t)
{
  /// @todo Funny thing, but "00" month is accepted as valid in get_time function.
  /// Seems like a bug in the std library.
  return (t.tm_mday >= 1 && t.tm_mday <= 31 &&
          t.tm_mon >= 0 && t.tm_mon <= 11);
}
}

time_t StringToTimestamp(std::string const & s)
{
  // Return current time in the case of failure
  time_t res = INVALID_TIME_STAMP;

  if (s.size() == 20)
  {
    // Parse UTC format: 1970-01-01T00:00:00Z
    tm t{};
    std::istringstream ss(s);
    ss >> get_time(&t, "%Y-%m-%dT%H:%M:%SZ");

    if (!ss.fail() && IsValid(t))
      res = base::TimeGM(t);
  }
  else if (s.size() == 25)
  {
    // Parse custom time zone offset format: 2012-12-03T00:38:34+03:30
    tm t1{}, t2{};
    char sign;
    std::istringstream ss(s);
    ss >> get_time(&t1, "%Y-%m-%dT%H:%M:%S") >> sign >> get_time(&t2, "%H:%M");

    if (!ss.fail() && IsValid(t1))
    {
      time_t const tt = base::TimeGM(t1);

      // Fix timezone offset
      if (sign == '-')
        res = tt + t2.tm_hour * 3600 + t2.tm_min * 60;
      else if (sign == '+')
        res = tt - t2.tm_hour * 3600 - t2.tm_min * 60;
    }
  }

  return res;
}

HighResTimer::HighResTimer(bool start/* = true*/)
{
  if (start)
    Reset();
}

void HighResTimer::Reset()
{
  m_start = std::chrono::high_resolution_clock::now();
}

uint64_t HighResTimer::ElapsedNano() const
{
  return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - m_start).count();
}

uint64_t HighResTimer::ElapsedMillis() const
{
  return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - m_start).count();
}

double HighResTimer::ElapsedSeconds() const
{
  return std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - m_start).count();
}

time_t SecondsSinceEpochToTimeT(uint64_t secondsSinceEpoch)
{
  std::chrono::time_point<std::chrono::system_clock> const tpoint{std::chrono::seconds(secondsSinceEpoch)};
  return std::chrono::system_clock::to_time_t(tpoint);
}

uint64_t TimeTToSecondsSinceEpoch(time_t time)
{
  auto const tpoint = std::chrono::system_clock::from_time_t(time);
  return std::chrono::duration_cast<std::chrono::seconds>(tpoint.time_since_epoch()).count();
}
}
