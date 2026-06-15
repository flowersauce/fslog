#ifndef FS_LOGLAB_LOG_FORMAT_HPP
#define FS_LOGLAB_LOG_FORMAT_HPP

/**
 * @file fslog.hpp
 * @brief 轻量日志构造库。
 * @author Flowersauce
 *
 * 库只把字符、C 字符串、定长文本和日志字段 token 追加到调用方提供的缓冲区，不绑定具体运行环境，也不管理运行时资源。
 * 普通整数、浮点数、布尔值、十六进制等业务数据应由调用方先格式化为字符串，再传入本库。
 * 本库保留 Unix 时间戳到日历时间字符串的格式化能力，因为它是日志时间戳的常用字段。
 * 超出缓冲区容量时直接截断，非法字段不输出；缓冲区始终以 '\0' 结尾。彩色字段会为 ANSI reset 预留空间。
 *
 * 基本用法：
 *
 * @code
 * char log_buffer[128];
 * auto msg = fslog::buffer(log_buffer)
 *          << fslog::level(fslog::severity::info)
 *          << fslog::tag("APP")
 *          << fslog::tag("usb")
 *          << fslog::datetime(unix_seconds, "YYMMDD hh:mm:ss", fslog::tz_hours(8))
 *          << "ready"
 *          << "\r\n";
 *
 * uart_write(msg.data(), msg.size());
 * @endcode
 *
 * 数组缓冲区可直接传入；指针缓冲区需要同时传入容量。
 */

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace fslog
{
	/**
	 * @brief ANSI 前景色。
	 *
	 * `none` 不输出颜色转义。
	 */
	enum class color : uint8_t
	{
		none,
		black,
		red,
		green,
		yellow,
		blue,
		magenta,
		cyan,
		white,
		bright_black,
		bright_red,
		bright_green,
		bright_yellow,
		bright_blue,
		bright_magenta,
		bright_cyan,
		bright_white,
	};

	/**
	 * @brief 非拥有只读字符视图。
	 *
	 * 既可作为构造结果的返回视图，也可作为定长文本输入。输入时只读取 `[data, data + size)`，
	 * 不要求 `data` 指向以 '\0' 结尾的字符串。当 `data == nullptr` 时视为空视图，`size` 会被忽略。
	 */
	struct view
	{
		const char *data;
		std::size_t size;

		[[nodiscard]] constexpr bool empty() const
		{
			return (data == nullptr) || (size == 0U);
		}

		[[nodiscard]] constexpr const char *begin() const
		{
			return data;
		}

		[[nodiscard]] constexpr const char *end() const
		{
			return data == nullptr ? nullptr : (data + size);
		}
	};

	/**
	 * @brief 定长文本 token。
	 *
	 * 用户通常通过 `text(...)` 创建该 token，而不是直接构造。
	 */
	struct text_token
	{
		view content;
	};

	/**
	 * @brief 计算以 `\0` 结尾的 C 字符串长度。`data == nullptr` 时返回 0。
	 */
	[[nodiscard]] inline std::size_t text_size(const char *data)
	{
		if (data == nullptr)
		{
			return 0U;
		}

		std::size_t size = 0U;
		while (data[size] != '\0')
		{
			++size;
		}
		return size;
	}

	/**
	 * @brief 在固定容量内计算 C 字符串长度，最多读取 `capacity` 个字符。
	 */
	[[nodiscard]] constexpr std::size_t bounded_text_size(const char *data, const std::size_t capacity)
	{
		if (data == nullptr)
		{
			return 0U;
		}

		std::size_t size = 0U;
		while ((size < capacity) && (data[size] != '\0'))
		{
			++size;
		}
		return size;
	}

	/**
	 * @brief 创建定长文本 token。
	 *
	 * 只读取 `[data, data + size)`；适合非 `\0` 结尾片段、协议字段和已知长度的格式化结果。
	 * `data == nullptr` 时不会输出。
	 */
	[[nodiscard]] inline constexpr text_token text(const char *data, const std::size_t size)
	{
		return {{data, size}};
	}

	/**
	 * @brief 创建以 `\0` 结尾的 C 字符串文本 token。
	 *
	 * 调用方必须保证 `data` 指向有效的 null-terminated 字符串；`data == nullptr` 时不会输出。
	 */
	[[nodiscard]] inline text_token text(const char *data)
	{
		return {{data, text_size(data)}};
	}

	/**
	 * @brief 创建字符数组文本 token，最多读取数组内的 `N` 个字符，遇到 `\0` 提前结束。
	 *
	 * 适合字符串字面量和固定字符数组。即使数组内没有 `\0`，也只会读取数组范围内的 `N` 个字符。
	 */
	template<std::size_t N>
	[[nodiscard]] constexpr text_token text(const char (&data)[N])
	{
		return {{data, bounded_text_size(data, N)}};
	}

	/**
	 * @brief 用于 level() 的日志等级。
	 */
	enum class severity : uint8_t
	{
		error,
		warn,
		info,
		debug,
		trace,
	};

	/**
	 * @brief 控制整条消息是否输出颜色转义。
	 */
	enum class color_mode : uint8_t
	{
		off,
		on,
	};

	/**
	 * @brief 控制 level() 输出短名还是长名。
	 */
	enum class level_style : uint8_t
	{
		short_name,
		long_name,
	};

	struct tz_offset
	{
		int32_t minutes;
	};

	/**
	 * @brief 创建分钟级时区偏移。
	 */
	[[nodiscard]] constexpr tz_offset tz_minutes(const int32_t minutes)
	{
		return {minutes};
	}

	/**
	 * @brief 创建小时级时区偏移。
	 */
	[[nodiscard]] constexpr tz_offset tz_hours(const int32_t hours)
	{
		return {hours * 60};
	}

	namespace detail
	{
		// Token 类型放在 detail 中，公开 API 通过工厂函数创建，减少用户补全噪音。
		struct level_token
		{
			severity	value;
			level_style style;
		};

		struct tag_token
		{
			const char *value;
		};

		struct style_token
		{
			bool		has_text_color;
			color		text_color;
			const char *left;
			const char *right;
		};

		struct reset_color_token
		{
		};

		struct loc_token
		{
			const char *file;
			int			line;
			const char *function;
		};

		struct datetime_token
		{
			int64_t		unix_seconds;
			const char *format;
			tz_offset	offset;
		};

		template<typename T>
		struct remove_cvref
		{
			using type = typename std::remove_cv<typename std::remove_reference<T>::type>::type;
		};

		template<typename T>
		using remove_cvref_t = typename remove_cvref<T>::type;

		template<typename T>
		struct is_blocked_scalar
		{
			using value_type = remove_cvref_t<T>;
			static constexpr bool value =
				(std::is_arithmetic<value_type>::value || std::is_enum<value_type>::value) && !std::is_same<value_type, char>::value;
		};
	} // namespace detail

	/**
	 * @brief 创建日志级别字段。
	 */
	[[nodiscard]] constexpr detail::level_token level(const severity value, const level_style style = level_style::short_name)
	{
		return {value, style};
	}

	/**
	 * @brief 创建标签字段。默认不添加左右修饰符，`nullptr` 不输出。
	 */
	[[nodiscard]] inline constexpr detail::tag_token tag(const char *value)
	{
		return {value};
	}

	/**
	 * @brief 设置后续输出项的默认颜色和包裹符，直到下一次 style(...)。
	 */
	[[nodiscard]] inline constexpr detail::style_token style(const char *left, const char *right)
	{
		return {false, color::none, left, right};
	}

	[[nodiscard]] inline constexpr detail::style_token style(const color text_color = color::none, const char *left = "", const char *right = "")
	{
		return {true, text_color, left, right};
	}

	/**
	 * @brief 重置 ANSI 颜色。
	 *
	 * 一般不需要手动调用；字段颜色和 `style(...)` 会自动 reset。
	 */
	[[nodiscard]] inline constexpr detail::reset_color_token reset_color()
	{
		return {};
	}

	/**
	 * @brief 创建源码位置字段。
	 *
	 * 典型用法是传入 `__FILE__`、`__LINE__`、`__func__`，输出类似 `app.cpp:42 RunOnce` 的字段，便于从日志定位源码位置。
	 */
	[[nodiscard]] constexpr detail::loc_token loc(const char *file, const int line, const char *function)
	{
		return {file, line, function};
	}

	/**
	 * @brief 创建 Unix 时间格式化字段。
	 *
	 * 格式字符串按连续相同字母识别字段：
	 * - `YYYY` 输出 4 位年份，`YY` 输出 2 位年份，`Y` 输出不补零的完整年份。
	 * - `MM`/`M`、`DD`/`D`、`hh`/`h`、`mm`/`m`、`ss`/`s` 分别输出补零/不补零字段。
	 * - 其它重复长度不是合法字段，例如 `YYY`、`MMM`、`DDD`，会原样输出。
	 * - 其它字符原样输出。
	 *
	 * 本库只保留秒级、固定字段的轻量时间格式化；毫秒、时区文本、locale、DST 等复杂需求应由外部 formatter
	 * 生成文本后通过 `text(...)` 或 `view` 传入。
	 */
	[[nodiscard]] constexpr detail::datetime_token datetime(const int64_t unix_seconds,
															const char *format,
															const tz_offset offset = tz_offset{0})
	{
		return {unix_seconds, format, offset};
	}

	namespace detail
	{
		inline constexpr auto	 kColorReset	= "\x1b[0m";
		inline constexpr int32_t kDaysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

		[[nodiscard]] constexpr const char *level_name(const severity value, const level_style style)
		{
			if (style == level_style::long_name)
			{
				switch (value)
				{
					case severity::error:
						return "ERROR";
					case severity::warn:
						return "WARN";
					case severity::info:
						return "INFO";
					case severity::debug:
						return "DEBUG";
					case severity::trace:
						return "TRACE";
					default:
						return "UNKNOWN";
				}
			}

			switch (value)
			{
				case severity::error:
					return "E";
				case severity::warn:
					return "W";
				case severity::info:
					return "I";
				case severity::debug:
					return "D";
				case severity::trace:
					return "T";
				default:
					return "?";
			}
		}

		[[nodiscard]] constexpr color severity_color(const severity value)
		{
			switch (value)
			{
				case severity::error:
					return color::bright_red;
				case severity::warn:
					return color::bright_yellow;
				case severity::info:
					return color::bright_green;
				case severity::debug:
					return color::bright_cyan;
				case severity::trace:
					return color::bright_black;
				default:
					return color::white;
			}
		}

		[[nodiscard]] constexpr const char *color_escape(const color value)
		{
			switch (value)
			{
				case color::black:
					return "\x1b[30m";
				case color::red:
					return "\x1b[31m";
				case color::green:
					return "\x1b[32m";
				case color::yellow:
					return "\x1b[33m";
				case color::blue:
					return "\x1b[34m";
				case color::magenta:
					return "\x1b[35m";
				case color::cyan:
					return "\x1b[36m";
				case color::white:
					return "\x1b[37m";
				case color::bright_black:
					return "\x1b[90m";
				case color::bright_red:
					return "\x1b[91m";
				case color::bright_green:
					return "\x1b[92m";
				case color::bright_yellow:
					return "\x1b[93m";
				case color::bright_blue:
					return "\x1b[94m";
				case color::bright_magenta:
					return "\x1b[95m";
				case color::bright_cyan:
					return "\x1b[96m";
				case color::bright_white:
					return "\x1b[97m";
				default:
					return "";
			}
		}

		[[nodiscard]] constexpr bool is_leap_year(const int32_t year)
		{
			return ((year % 4) == 0) && (((year % 100) != 0) || ((year % 400) == 0));
		}

		[[nodiscard]] constexpr int32_t days_in_year(const int32_t year)
		{
			return is_leap_year(year) ? 366 : 365;
		}

		[[nodiscard]] constexpr int32_t days_in_month(const int32_t year, const int32_t month)
		{
			if (month == 2)
			{
				return is_leap_year(year) ? 29 : 28;
			}
			return kDaysInMonth[month - 1];
		}

		struct date_time
		{
			int32_t year;
			int32_t month;
			int32_t day;
			int32_t hour;
			int32_t minute;
			int32_t second;
		};

		[[nodiscard]] inline bool unix_to_date_time(const int64_t unix_seconds, const int32_t offset_minutes, date_time &out)
		{
			constexpr int64_t kMinSupported = -2208988800LL; // 1900-01-01 00:00:00 UTC
			constexpr int64_t kMaxSupported = 4102444799LL;	 // 2099-12-31 23:59:59 UTC
			const int64_t	  adjusted		= unix_seconds + (static_cast<int64_t>(offset_minutes) * 60LL);
			if ((adjusted < kMinSupported) || (adjusted > kMaxSupported))
			{
				return false;
			}

			int64_t days = adjusted / 86400LL;
			int64_t rem	 = adjusted % 86400LL;
			if (rem < 0)
			{
				rem += 86400LL;
				--days;
			}

			int32_t year = 1970;
			while (days < 0)
			{
				--year;
				days += days_in_year(year);
			}
			while (days >= days_in_year(year))
			{
				days -= days_in_year(year);
				++year;
			}

			int32_t month = 1;
			while (days >= days_in_month(year, month))
			{
				days -= days_in_month(year, month);
				++month;
			}

			out.year   = year;
			out.month  = month;
			out.day	   = static_cast<int32_t>(days) + 1;
			out.hour   = static_cast<int32_t>(rem / 3600LL);
			out.minute = static_cast<int32_t>((rem % 3600LL) / 60LL);
			out.second = static_cast<int32_t>(rem % 60LL);
			return true;
		}

		[[nodiscard]] inline const char *base_name(const char *path)
		{
			if (path == nullptr)
			{
				return "";
			}

			const char *last = path;
			for (const char *it = path; *it != '\0'; ++it)
			{
				if ((*it == '/') || (*it == '\\'))
				{
					last = it + 1;
				}
			}
			return last;
		}
	} // namespace detail

	/**
	 * @brief 绑定外部缓冲区的日志构造器。
	 *
	 * 容量包含结尾的 `'\0'`，最多写入 `capacity - 1` 字节有效内容。
	 *
	 * @code
	 * char log_buffer[128];
	 * auto line = fslog::buffer(log_buffer) << fslog::level(fslog::severity::warn) << " short\r\n";
	 * uart_write(line.data(), line.size());
	 * @endcode
	 *
	 * 指针缓冲区用法：
	 *
	 * @code
	 * auto msg = fslog::buffer(reinterpret_cast<char *>(0x24040000), 256)
	 *          << fslog::level(fslog::severity::info) << " external buffer\r\n";
	 * uart_write(msg.data(), msg.length());
	 * @endcode
	 *
	 * 超出容量时直接截断。
	 *
	 * builder 绑定外部缓冲区并记录当前写入位置，因此不可拷贝。为了支持
	 * `auto msg = fslog::buffer(buf) << ...;` 这种一行链式用法，builder 支持安全移动。
	 *
	 * 不同线程可并发使用独立 builder 和独立缓冲区；共享 builder、共享缓冲区或共享输出后端时，调用方负责同步。
	 */
	class builder
	{
	public:
		/**
		 * @brief 绑定外部缓冲区。
		 *
		 * @param buffer 缓冲区起始地址。
		 * @param capacity 缓冲区容量，包含结尾的 '\0'。
		 * @param color_output 颜色输出模式开关。
		 */
		explicit builder(char *buffer, const std::size_t capacity, const color_mode color_output = color_mode::off)
			: color_output_{color_output}
		{
			if ((buffer != nullptr) && (capacity > 0U))
			{
				buffer_	  = buffer;
				capacity_ = capacity;
			}
			else
			{
				buffer_	  = empty_;
				capacity_ = sizeof(empty_);
			}
			terminate();
		}

		builder(const builder &)			= delete;
		builder &operator=(const builder &) = delete;

		builder(builder &&other) noexcept
		{
			move_from(other);
		}

		builder &operator=(builder &&other) noexcept
		{
			if (this != &other)
			{
				move_from(other);
			}
			return *this;
		}

		/**
		 * @brief 获取构造结果起始地址。
		 */
		[[nodiscard]]
		const char *data() const
		{
			return buffer_;
		}

		/**
		 * @brief 获取以 '\0' 结尾的构造结果。
		 */
		[[nodiscard]]
		const char *c_str() const
		{
			return buffer_;
		}

		/**
		 * @brief 获取构造结果长度，不包含结尾的 '\0'。
		 */
		[[nodiscard]]
		std::size_t size() const
		{
			return size_;
		}

		/**
		 * @brief 获取构造结果长度，不包含结尾的 '\0'。
		 */
		[[nodiscard]]
		std::size_t length() const
		{
			return size();
		}

		/**
		 * @brief 获取构造结果视图。
		 */
		[[nodiscard]]
		fslog::view view() const
		{
			return {data(), size()};
		}

		/**
		 * @brief 追加字符串、字符或字段 token。
		 */
		builder &operator<<(const detail::level_token &token) &
		{
			append_styled(detail::level_name(token.value, token.style), resolve_level_color(token.value));
			return *this;
		}

		builder &&operator<<(const detail::level_token &token) &&
		{
			*this << token;
			return static_cast<builder &&>(*this);
		}

		builder &operator<<(const detail::tag_token &token) &
		{
			append_styled(token.value);
			return *this;
		}

		builder &&operator<<(const detail::tag_token &token) &&
		{
			*this << token;
			return static_cast<builder &&>(*this);
		}

		builder &operator<<(const detail::style_token &token) &
		{
			style_has_color_ = token.has_text_color;
			style_color_	 = token.text_color;
			style_left_		 = token.left;
			style_right_	 = token.right;
			return *this;
		}

		builder &&operator<<(const detail::style_token &token) &&
		{
			*this << token;
			return static_cast<builder &&>(*this);
		}

		builder &operator<<(const detail::reset_color_token &) &
		{
			if (color_output_ == color_mode::on)
			{
				append(detail::kColorReset);
			}
			return *this;
		}

		builder &&operator<<(const detail::reset_color_token &token) &&
		{
			*this << token;
			return static_cast<builder &&>(*this);
		}

		builder &operator<<(const detail::loc_token &token) &
		{
			const bool color_started = begin_color(style_has_color_ ? style_color_ : color::none);
			append(style_left_);
			append(detail::base_name(token.file));
			append(':');
			append_line_number(token.line);
			append(' ');
			append(token.function == nullptr ? "" : token.function);
			append(style_right_);
			end_color(color_started);
			return *this;
		}

		builder &&operator<<(const detail::loc_token &token) &&
		{
			*this << token;
			return static_cast<builder &&>(*this);
		}

		builder &operator<<(const detail::datetime_token &token) &
		{
			detail::date_time dt = {};
			if ((token.format == nullptr) || !detail::unix_to_date_time(token.unix_seconds, token.offset.minutes, dt))
			{
				return *this;
			}

			const bool color_started = begin_color(style_has_color_ ? style_color_ : color::none);
			append(style_left_);
			append_time(dt, token.format);
			append(style_right_);
			end_color(color_started);
			return *this;
		}

		builder &&operator<<(const detail::datetime_token &token) &&
		{
			*this << token;
			return static_cast<builder &&>(*this);
		}

		builder &operator<<(const char *text) &
		{
			if (text != nullptr)
			{
				append_styled(text);
			}
			return *this;
		}

		builder &&operator<<(const char *text) &&
		{
			*this << text;
			return static_cast<builder &&>(*this);
		}

		builder &operator<<(const char value) &
		{
			append_styled(value);
			return *this;
		}

		builder &&operator<<(const char value) &&
		{
			*this << value;
			return static_cast<builder &&>(*this);
		}

		template<typename T, typename std::enable_if<detail::is_blocked_scalar<T>::value, int>::type = 0>
		builder &operator<<(T) & = delete;

		template<typename T, typename std::enable_if<detail::is_blocked_scalar<T>::value, int>::type = 0>
		builder &&operator<<(T) && = delete;

		builder &operator<<(const fslog::view text) &
		{
			if (!text.empty())
			{
				append_styled(text);
			}
			return *this;
		}

		builder &&operator<<(const fslog::view text) &&
		{
			*this << text;
			return static_cast<builder &&>(*this);
		}

		builder &operator<<(const text_token &token) &
		{
			if (token.content.empty())
			{
				return *this;
			}

			append_styled(token.content);
			return *this;
		}

		builder &&operator<<(const text_token &token) &&
		{
			*this << token;
			return static_cast<builder &&>(*this);
		}

	private:
		void move_from(builder &other) noexcept
		{
			if (other.buffer_ == other.empty_)
			{
				buffer_	  = empty_;
				capacity_ = sizeof(empty_);
			}
			else
			{
				buffer_	  = other.buffer_;
				capacity_ = other.capacity_;
			}

			size_			 = other.size_;
			tail_reserve_	 = other.tail_reserve_;
			color_output_	 = other.color_output_;
			style_has_color_ = other.style_has_color_;
			style_color_	 = other.style_color_;
			style_left_		 = other.style_left_;
			style_right_	 = other.style_right_;

			other.buffer_		   = other.empty_;
			other.capacity_		   = sizeof(other.empty_);
			other.size_			   = 0U;
			other.tail_reserve_	   = 0U;
			other.style_has_color_ = false;
			other.style_color_	   = color::none;
			other.style_left_	   = "";
			other.style_right_	   = "";
			other.terminate();
		}

		void terminate() const
		{
			buffer_[size_] = '\0';
		}

		void ignore_overflow() const
		{
			terminate();
		}

		void append(const char value)
		{
			if (is_full())
			{
				ignore_overflow();
				return;
			}
			buffer_[size_++] = value;
			terminate();
		}

		void append(const char *text)
		{
			if (text == nullptr)
			{
				return;
			}
			for (const char *it = text; *it != '\0'; ++it)
			{
				append(*it);
				if (is_full())
				{
					return;
				}
			}
		}

		void append(const fslog::view text)
		{
			if (text.data == nullptr)
			{
				return;
			}
			for (std::size_t i = 0U; i < text.size; ++i)
			{
				append(text.data[i]);
				if (is_full())
				{
					return;
				}
			}
		}

		[[nodiscard]] bool begin_color(const color value)
		{
			if ((color_output_ == color_mode::off) || (value == color::none))
			{
				return false;
			}

			const char		 *escape	  = detail::color_escape(value);
			const std::size_t escape_size = text_size(escape);
			const std::size_t reset_size  = text_size(detail::kColorReset);
			if ((escape_size == 0U) || (remaining_capacity() <= (escape_size + reset_size)))
			{
				return false;
			}

			append(escape);
			tail_reserve_ = reset_size;
			return true;
		}

		void end_color(const bool color_started)
		{
			if (!color_started)
			{
				return;
			}

			tail_reserve_ = 0U;
			append(detail::kColorReset);
		}

		[[nodiscard]] color resolve_level_color(const severity value) const
		{
			return style_has_color_ ? style_color_ : detail::severity_color(value);
		}

		void append_styled(const char *text, const color text_color)
		{
			if (text == nullptr)
			{
				return;
			}
			const bool color_started = begin_color(text_color);
			append(style_left_);
			append(text);
			append(style_right_);
			end_color(color_started);
		}

		void append_styled(const char *text)
		{
			append_styled(text, style_has_color_ ? style_color_ : color::none);
		}

		void append_styled(const char value)
		{
			const bool color_started = begin_color(style_has_color_ ? style_color_ : color::none);
			append(style_left_);
			append(value);
			append(style_right_);
			end_color(color_started);
		}

		void append_styled(const fslog::view text)
		{
			const bool color_started = begin_color(style_has_color_ ? style_color_ : color::none);
			append(style_left_);
			append(text);
			append(style_right_);
			end_color(color_started);
		}

		void append_unsigned_decimal(uint32_t value, const uint8_t min_width)
		{
			char		digits[10] = {};
			std::size_t count	   = 0U;
			do
			{
				digits[count++] = static_cast<char>('0' + (value % 10U));
				value /= 10U;
			} while (value != 0U);

			while (count < min_width)
			{
				digits[count++] = '0';
			}

			while (count > 0U)
			{
				append(digits[--count]);
			}
		}

		[[nodiscard]] static std::size_t field_width(const char *text, const char field)
		{
			std::size_t width = 0U;
			while (text[width] == field)
			{
				++width;
			}
			return width;
		}

		void append_datetime_value(const int32_t value, const uint8_t min_width)
		{
			append_unsigned_decimal(static_cast<uint32_t>(value), min_width);
		}

		void append_literal(const char *text, const std::size_t size)
		{
			for (std::size_t i = 0U; i < size; ++i)
			{
				append(text[i]);
			}
		}

		void append_time(const detail::date_time &dt, const char *format)
		{
			for (const char *it = format; *it != '\0';)
			{
				const std::size_t width = field_width(it, *it);
				if (*it == 'Y')
				{
					if (width == 1U)
					{
						append_datetime_value(dt.year, 0U);
					}
					else if (width == 2U)
					{
						append_datetime_value(dt.year % 100, 2U);
					}
					else if (width == 4U)
					{
						append_datetime_value(dt.year, 4U);
					}
					else
					{
						append_literal(it, width);
					}
					it += width;
				}
				else if (*it == 'M')
				{
					if (width <= 2U)
					{
						append_datetime_value(dt.month, width == 2U ? 2U : 0U);
					}
					else
					{
						append_literal(it, width);
					}
					it += width;
				}
				else if (*it == 'D')
				{
					if (width <= 2U)
					{
						append_datetime_value(dt.day, width == 2U ? 2U : 0U);
					}
					else
					{
						append_literal(it, width);
					}
					it += width;
				}
				else if (*it == 'h')
				{
					if (width <= 2U)
					{
						append_datetime_value(dt.hour, width == 2U ? 2U : 0U);
					}
					else
					{
						append_literal(it, width);
					}
					it += width;
				}
				else if (*it == 'm')
				{
					if (width <= 2U)
					{
						append_datetime_value(dt.minute, width == 2U ? 2U : 0U);
					}
					else
					{
						append_literal(it, width);
					}
					it += width;
				}
				else if (*it == 's')
				{
					if (width <= 2U)
					{
						append_datetime_value(dt.second, width == 2U ? 2U : 0U);
					}
					else
					{
						append_literal(it, width);
					}
					it += width;
				}
				else
				{
					append(*it);
					++it;
				}
			}
		}

		void append_line_number(const int value)
		{
			if (value < 0)
			{
				append('-');
				const auto magnitude = static_cast<uint32_t>(-(value + 1)) + 1U;
				append_unsigned_decimal(magnitude, 0U);
				return;
			}
			append_unsigned_decimal(static_cast<uint32_t>(value), 0U);
		}

		[[nodiscard]]
		bool is_full() const
		{
			return remaining_capacity() <= tail_reserve_;
		}

		[[nodiscard]]
		std::size_t remaining_capacity() const
		{
			const std::size_t writable = writable_capacity();
			return (size_ >= writable) ? 0U : (writable - size_);
		}

		[[nodiscard]]
		std::size_t writable_capacity() const
		{
			return (capacity_ == 0U) ? 0U : (capacity_ - 1U);
		}

		mutable char empty_[1]		  = {};
		char		*buffer_		  = empty_;
		std::size_t	 capacity_		  = sizeof(empty_);
		std::size_t	 size_			  = 0U;
		std::size_t	 tail_reserve_	  = 0U;
		color_mode	 color_output_	  = color_mode::off;
		bool		 style_has_color_ = false;
		color		 style_color_	  = color::none;
		const char	*style_left_	  = "";
		const char	*style_right_	  = "";
	};

	/**
	 * @brief 绑定数组缓冲区并创建构造器。
	 */
	template<std::size_t N>
	[[nodiscard]] builder buffer(char (&storage)[N], const color_mode color_output = color_mode::off)
	{
		return builder{storage, N, color_output};
	}

	/**
	 * @brief 绑定指针和容量并创建构造器。
	 */
	[[nodiscard]] inline builder buffer(char *storage, const std::size_t capacity, const color_mode color_output = color_mode::off)
	{
		return builder{storage, capacity, color_output};
	}

} // namespace fslog

#endif /* FS_LOGLAB_LOG_FORMAT_HPP */
