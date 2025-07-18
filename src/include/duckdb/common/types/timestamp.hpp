//===----------------------------------------------------------------------===//
//                         DuckDB
//
// duckdb/common/types/timestamp.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "duckdb/common/limits.hpp"
#include "duckdb/common/string_util.hpp"
#include "duckdb/common/types.hpp"
#include "duckdb/common/winapi.hpp"

#include <functional>

namespace duckdb {

struct date_t;     // NOLINT
struct dtime_t;    // NOLINT
struct dtime_ns_t; // NOLINT
struct dtime_tz_t; // NOLINT

//! Type used to represent a TIMESTAMP. timestamp_t holds the microseconds since 1970-01-01.
struct timestamp_t { // NOLINT
	// NOTE: The unit of value is microseconds for timestamp_t, but it can be
	// different for subclasses (e.g. it's nanos for timestamp_ns, etc).
	int64_t value;

	timestamp_t() = default;
	explicit inline constexpr timestamp_t(int64_t micros) : value(micros) {
	}
	inline timestamp_t &operator=(int64_t micros) {
		value = micros;
		return *this;
	}

	// explicit conversion
	explicit inline operator int64_t() const {
		return value;
	}

	// comparison operators
	inline bool operator==(const timestamp_t &rhs) const {
		return value == rhs.value;
	};
	inline bool operator!=(const timestamp_t &rhs) const {
		return value != rhs.value;
	};
	inline bool operator<=(const timestamp_t &rhs) const {
		return value <= rhs.value;
	};
	inline bool operator<(const timestamp_t &rhs) const {
		return value < rhs.value;
	};
	inline bool operator>(const timestamp_t &rhs) const {
		return value > rhs.value;
	};
	inline bool operator>=(const timestamp_t &rhs) const {
		return value >= rhs.value;
	};

	// arithmetic operators
	timestamp_t operator+(const double &value) const;
	int64_t operator-(const timestamp_t &other) const;

	// in-place operators
	timestamp_t &operator+=(const int64_t &delta);
	timestamp_t &operator-=(const int64_t &delta);

	// special values
	static constexpr timestamp_t infinity() { // NOLINT
		return timestamp_t(NumericLimits<int64_t>::Maximum());
	}                                          // NOLINT
	static constexpr timestamp_t ninfinity() { // NOLINT
		return timestamp_t(-NumericLimits<int64_t>::Maximum());
	}                                             // NOLINT
	static constexpr inline timestamp_t epoch() { // NOLINT
		return timestamp_t(0);
	} // NOLINT
};

//! Type used to represent TIMESTAMP_S. timestamp_sec_t holds the seconds since 1970-01-01.
struct timestamp_sec_t : public timestamp_t { // NOLINT
	timestamp_sec_t() = default;
	explicit inline constexpr timestamp_sec_t(int64_t seconds) : timestamp_t(seconds) {
	}
};

//! Type used to represent TIMESTAMP_MS. timestamp_ms_t holds the milliseconds since 1970-01-01.
struct timestamp_ms_t : public timestamp_t { // NOLINT
	timestamp_ms_t() = default;
	explicit inline constexpr timestamp_ms_t(int64_t millis) : timestamp_t(millis) {
	}
};

//! Type used to represent TIMESTAMP_NS. timestamp_ns_t holds the nanoseconds since 1970-01-01.
struct timestamp_ns_t : public timestamp_t { // NOLINT
	timestamp_ns_t() = default;
	explicit inline constexpr timestamp_ns_t(int64_t nanos) : timestamp_t(nanos) {
	}
};

//! Type used to represent TIMESTAMPTZ. timestamp_tz_t holds the microseconds since 1970-01-01 (UTC).
//! It is physically the same as timestamp_t, both hold microseconds since epoch.
struct timestamp_tz_t : public timestamp_t { // NOLINT
	timestamp_tz_t() = default;
	explicit inline constexpr timestamp_tz_t(int64_t micros) : timestamp_t(micros) {
	}
	explicit inline constexpr timestamp_tz_t(timestamp_t ts) : timestamp_t(ts) {
	}
};

enum class TimestampCastResult : uint8_t {
	SUCCESS,
	ERROR_INCORRECT_FORMAT,
	ERROR_NON_UTC_TIMEZONE,
	ERROR_RANGE,
	STRICT_UTC
};

struct TimestampComponents {
	int32_t year;
	int32_t month;
	int32_t day;

	int32_t hour;
	int32_t minute;
	int32_t second;
	int32_t microsecond;
};

//! The static Timestamp class holds helper functions for the timestamp types.
class Timestamp {
public:
	// min timestamp is 290308-12-22 (BC)
	constexpr static const int32_t MIN_YEAR = -290308;
	constexpr static const int32_t MIN_MONTH = 12;
	constexpr static const int32_t MIN_DAY = 22;

public:
	//! Convert a string in the format "YYYY-MM-DD hh:mm:ss[.f][-+TH[:tm]]" to a timestamp object
	DUCKDB_API static timestamp_t FromString(const string &str);
	//! Convert a string where the offset can also be a time zone string: / [A_Za-z0-9/_]+/
	//! If has_offset is true, then the result is an instant that was offset from UTC
	//! If the tz is not empty, the result is still an instant, but the parts can be extracted and applied to the TZ
	DUCKDB_API static TimestampCastResult TryConvertTimestampTZ(const char *str, idx_t len, timestamp_t &result,
	                                                            bool &has_offset, string_t &tz,
	                                                            optional_ptr<int32_t> nanos = nullptr);
	//! Strict Timestamp does not accept offsets.
	DUCKDB_API static TimestampCastResult TryConvertTimestamp(const char *str, idx_t len, timestamp_t &result,
	                                                          optional_ptr<int32_t> nanos = nullptr,
	                                                          bool strict = false);
	DUCKDB_API static TimestampCastResult TryConvertTimestamp(const char *str, idx_t len, timestamp_ns_t &result);
	DUCKDB_API static timestamp_t FromCString(const char *str, idx_t len, optional_ptr<int32_t> nanos = nullptr);
	//! Convert a date object to a string in the format "YYYY-MM-DD hh:mm:ss"
	DUCKDB_API static string ToString(timestamp_t timestamp);

	DUCKDB_API static date_t GetDate(timestamp_t timestamp);

	DUCKDB_API static dtime_t GetTime(timestamp_t timestamp);
	DUCKDB_API static dtime_ns_t GetTimeNs(timestamp_ns_t timestamp);
	//! Create a Timestamp object from a specified (date, time) combination
	DUCKDB_API static timestamp_t FromDatetime(date_t date, dtime_t time);
	DUCKDB_API static bool TryFromDatetime(date_t date, dtime_t time, timestamp_t &result);
	DUCKDB_API static bool TryFromDatetime(date_t date, dtime_tz_t timetz, timestamp_t &result);
	//! Scale up to ns
	DUCKDB_API static bool TryFromTimestampNanos(timestamp_t ts, int32_t nanos, timestamp_ns_t &result);

	//! Is the character a valid part of a time zone name?
	static inline bool CharacterIsTimeZone(char c) {
		return StringUtil::CharacterIsAlpha(c) || StringUtil::CharacterIsDigit(c) || c == '_' || c == '/' || c == '+' ||
		       c == '-';
	}

	//! True, if the timestamp is finite, else false.
	static inline bool IsFinite(timestamp_t timestamp) {
		return timestamp != timestamp_t::infinity() && timestamp != timestamp_t::ninfinity();
	}

	//! Extract the date and time from a given timestamp object
	DUCKDB_API static void Convert(timestamp_t date, date_t &out_date, dtime_t &out_time);
	//! Extract the date and time from a given timestamp object
	DUCKDB_API static void Convert(timestamp_ns_t date, date_t &out_date, dtime_t &out_time, int32_t &out_nanos);
	//! Returns current timestamp
	DUCKDB_API static timestamp_t GetCurrentTimestamp();

	//! Convert the epoch (in sec) to a timestamp
	DUCKDB_API static timestamp_t FromEpochSecondsPossiblyInfinite(int64_t s);
	DUCKDB_API static timestamp_t FromEpochSeconds(int64_t s);
	//! Convert the epoch (in ms) to a timestamp
	DUCKDB_API static timestamp_t FromEpochMsPossiblyInfinite(int64_t ms);
	DUCKDB_API static timestamp_t FromEpochMs(int64_t ms);
	//! Convert the epoch (in microseconds) to a timestamp
	DUCKDB_API static timestamp_t FromEpochMicroSeconds(int64_t micros);
	//! Convert the epoch (in nanoseconds) to a timestamp
	DUCKDB_API static timestamp_t FromEpochNanoSecondsPossiblyInfinite(int64_t nanos);
	DUCKDB_API static timestamp_t FromEpochNanoSeconds(int64_t nanos);

	//! Construct ns timestamps from various epoch units
	DUCKDB_API static timestamp_ns_t TimestampNsFromEpochMicros(int64_t micros);
	DUCKDB_API static timestamp_ns_t TimestampNsFromEpochMillis(int64_t millis);

	//! Try convert a timestamp to epoch (in nanoseconds)
	DUCKDB_API static bool TryGetEpochNanoSeconds(timestamp_t timestamp, int64_t &result);
	//! Convert the epoch (in seconds) to a timestamp
	DUCKDB_API static int64_t GetEpochSeconds(timestamp_t timestamp);
	//! Convert the epoch (in ms) to a timestamp
	DUCKDB_API static int64_t GetEpochMs(timestamp_t timestamp);
	//! Convert a timestamp to epoch (in microseconds)
	DUCKDB_API static int64_t GetEpochMicroSeconds(timestamp_t timestamp);
	//! Convert a timestamp to epoch (in nanoseconds)
	DUCKDB_API static int64_t GetEpochNanoSeconds(timestamp_t timestamp);
	DUCKDB_API static int64_t GetEpochNanoSeconds(timestamp_ns_t timestamp);
	//! Convert a timestamp to a rounded epoch at a given resolution.
	DUCKDB_API static int64_t GetEpochRounded(timestamp_t timestamp, const int64_t power_of_ten);
	//! Convert a timestamp to a Julian Day
	DUCKDB_API static double GetJulianDay(timestamp_t timestamp);

	//! Decompose a timestamp into its components
	DUCKDB_API static TimestampComponents GetComponents(timestamp_t timestamp);
	DUCKDB_API static time_t ToTimeT(timestamp_t);
	DUCKDB_API static timestamp_t FromTimeT(time_t);

	DUCKDB_API static bool TryParseUTCOffset(const char *str, idx_t &pos, idx_t len, int &hh, int &mm, int &ss);

	DUCKDB_API static string FormatError(const string &str);
	DUCKDB_API static string FormatError(string_t str);
	DUCKDB_API static string UnsupportedTimezoneError(const string &str);
	DUCKDB_API static string UnsupportedTimezoneError(string_t str);
	DUCKDB_API static string RangeError(const string &str);
	DUCKDB_API static string RangeError(string_t str);
};

} // namespace duckdb

namespace std {

//! Timestamp
template <>
struct hash<duckdb::timestamp_t> {
	std::size_t operator()(const duckdb::timestamp_t &k) const {
		using std::hash;
		return hash<int64_t>()((int64_t)k);
	}
};
template <>
struct hash<duckdb::timestamp_ms_t> {
	std::size_t operator()(const duckdb::timestamp_ms_t &k) const {
		using std::hash;
		return hash<int64_t>()((int64_t)k);
	}
};
template <>
struct hash<duckdb::timestamp_ns_t> {
	std::size_t operator()(const duckdb::timestamp_ns_t &k) const {
		using std::hash;
		return hash<int64_t>()((int64_t)k);
	}
};
template <>
struct hash<duckdb::timestamp_sec_t> {
	std::size_t operator()(const duckdb::timestamp_sec_t &k) const {
		using std::hash;
		return hash<int64_t>()((int64_t)k);
	}
};
template <>
struct hash<duckdb::timestamp_tz_t> {
	std::size_t operator()(const duckdb::timestamp_tz_t &k) const {
		using std::hash;
		return hash<int64_t>()((int64_t)k);
	}
};
} // namespace std
