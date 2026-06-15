# fslog 使用手册

`fslog` 是一个轻量级、header-only 的日志构造库。它只负责把日志字段和文本写入调用方提供的缓冲区，不绑定输出通道，不分配内存，也不管理运行时资源。

`fslog` 的定位是：

> 日志结构构造器，不是通用 formatter。

它负责组合日志等级、标签、时间戳、源码位置、样式和已经是文本的数据。普通数值、布尔值、枚举、字节缓冲区和二进制 payload 必须由调用方或外部
formatter 显式转换为文本后再传入。

库面向嵌入式和可移植 C++ 项目：调用方准备缓冲区，`fslog` 在其中构造一条 null-terminated 日志消息。缓冲区不足时直接截断；启用
ANSI 颜色时，彩色字段会为 reset 序列预留空间，避免截断后污染后续终端输出。

## 特性

- Header-only
- C++17
- 无动态内存分配
- 不依赖 STL 容器
- 不依赖 iostream
- 不依赖 `printf` / `snprintf`
- 输出后端无关，可接 UART、USB CDC、RTT、SWO、文件、环形缓冲区等
- 外部缓冲区驱动，缓冲区不足时安全截断
- 支持日志等级、标签、时间戳、源码位置
- 支持 ANSI 前景色和字段左右修饰符

## 接入

```cmake
add_subdirectory(fslog)

target_link_libraries(your_target
        fslog::fslog
)
```

`fslog` 需要 C++17。

## 基本用法

```cpp
#include "fslog.hpp"

char line[160];

const auto msg = fslog::buffer(line)
                 << fslog::style("[", "]")
                 << fslog::level(fslog::severity::info)
                 << fslog::tag("APP")
                 << fslog::datetime(1716643200, "YYYY-MM-DD hh:mm:ss", fslog::tz_hours(8))
                 << fslog::loc(__FILE__, __LINE__, __func__)
                 << fslog::style("", "")
                 << " system started"
                 << "\r\n";

uart_write(msg.data(), msg.size());
```

`buffer(...)` 可以接收字符数组，也可以接收指针和容量：

```cpp
char line[128];
auto a = fslog::buffer(line);

char *external = get_log_buffer();
auto b = fslog::buffer(external, 128);
```

容量包含末尾的 `'\0'`，有效内容最多写入 `capacity - 1` 字节。构造结果可通过 `data()`、`c_str()`、`size()`、`length()` 或
`view()` 读取。

`builder` 不可拷贝，但支持移动，以保留一行链式调用：

```cpp
const auto msg = fslog::buffer(line)
                 << fslog::tag("APP")
                 << " started";
```

## 输入类型边界

`fslog::builder` 只接受明确的文本输入和日志字段 token。

允许直接追加：

- `char`
- `const char *`
- 字符串字面量
- `fslog::view`
- `fslog::text(...)`
- `fslog::level(...)`
- `fslog::tag(...)`
- `fslog::datetime(...)`
- `fslog::loc(...)`
- `fslog::style(...)`
- `fslog::reset_color()`

示例：

```cpp
msg << 'A';
msg << '\r';
msg << '\n';
msg << "\r\n";
msg << "hello";
msg << fslog::text("abc", 3);
msg << fslog::tag("APP");
msg << fslog::level(fslog::severity::info);
```

不允许直接追加普通数值、布尔值、枚举和字节数据：

```cpp
msg << 123;             // error
msg << 0x41;            // error
msg << uint32_t{123};   // error
msg << uint8_t{'A'};    // error
msg << true;            // error
msg << 3.14f;           // error
msg << some_enum;       // error
```

这些数据必须由调用方或外部 formatter 先转换为文本，再通过 `const char *`、`fslog::text(...)` 或 `fslog::view` 传入。

这个规则用于防止常见误用：

```cpp
uint32_t adc = 1234;

msg << "adc=" << adc;   // error，而不是静默输出低 8 位字符
```

## 文本输入

直接追加 C 字符串：

```cpp
msg << "literal";
msg << text_ptr;       // text_ptr 必须是 null-terminated C 字符串
msg << ' ';
```

`nullptr` 会被安全忽略：

```cpp
const char *text = nullptr;
msg << text;           // 不输出任何内容
```

`text(...)` 有三种常用入口：

```cpp
fslog::text(ptr);          // null-terminated C 字符串
fslog::text(ptr, len);     // 明确长度，只读取 [ptr, ptr + len)
fslog::text(array);        // 字符数组，最多读取数组内 N 个字符，遇到 '\0' 提前结束
```

数组重载适合字符串字面量和固定字符数组：

```cpp
char name[16] = "sensor";
const char raw[3] = {'A', 'B', 'C'};

auto msg = fslog::buffer(line)
           << fslog::text("literal")
           << fslog::text(name)
           << fslog::text(raw);
```

如果只有 `const char *` 且不能保证存在 `'\0'`，不要用 `text(ptr)` 或直接 `<< ptr`，应使用显式长度：

```cpp
auto msg = fslog::buffer(line)
           << fslog::text(payload, payload_size);
```

## `uint8_t` 和二进制缓冲区

`uint8_t`、`uint8_t *`、`uint8_t[]` 不属于 `fslog` 的直接文本输入类型。

如果某段 `uint8_t` 缓冲区确实存放的是文本，调用方应显式转换为 `const char *` 并提供长度：

```cpp
uint8_t rx_buf[64];
std::size_t rx_len = get_rx_len();

auto msg = fslog::buffer(line)
           << "rx="
           << fslog::text(reinterpret_cast<const char *>(rx_buf), rx_len)
           << "\r\n";
```

如果缓冲区是二进制数据，应先在外部转换为十六进制字符串，再传给 `fslog`：

```cpp
uint8_t frame[] = {0x02, 0x10, 0x00, 0x1B, 0xFF};

char hex_text[64];
const std::size_t hex_len = project_hex_dump(
    hex_text,
    sizeof(hex_text),
    frame,
    sizeof(frame)
);

auto msg = fslog::buffer(line)
           << "frame="
           << fslog::text(hex_text, hex_len)
           << "\r\n";
```

`fslog` 不判断 `uint8_t` 数据是文本、数字还是二进制 payload。这个判断必须由调用方完成。

## 字段

### 日志等级

```cpp
fslog::level(fslog::severity::info)
fslog::level(fslog::severity::info, fslog::level_style::long_name)
```

默认输出短等级，例如 `I`。长等级输出 `INFO`。

没有当前样式颜色时，`level(...)` 的颜色由等级固定决定：

- `error`: 亮红
- `warn`: 亮黄
- `info`: 亮绿
- `debug`: 亮青
- `trace`: 亮黑/灰

### 标签

```cpp
fslog::tag("APP")
fslog::style(fslog::color::cyan) << fslog::tag("NET")
```

`tag(nullptr)` 不输出。

### 日期时间

```cpp
fslog::datetime(unix_seconds, "YYYY-MM-DD hh:mm:ss", fslog::tz_hours(8))
```

支持字段：

- 年：`Y`、`YY`、`YYYY`
- 月：`M`、`MM`
- 日：`D`、`DD`
- 时：`h`、`hh`
- 分：`m`、`mm`
- 秒：`s`、`ss`

不支持的重复字段按原文输出。时间戳支持范围为时区调整后的 `1900-01-01` 到 `2099-12-31`；超出范围时该字段不输出。

`fslog` 只保留秒级、固定字段的轻量时间格式化。毫秒、时区文本、locale、DST 等复杂需求应由外部 formatter 或标准库先生成文本，再通过
`fslog::text(...)` 或 `fslog::view` 传入。

### 源码位置

```cpp
fslog::loc(__FILE__, __LINE__, __func__)
```

默认输出：

```text
file.cpp:42 function
```

文件名会自动去掉路径，只保留 basename。

## 后续项的默认样式

`style(left, right)` 和 `style(color, left, right)` 是控制颜色和左右修饰符的 API。它们设置后续输出项的外层包裹符和可选颜色，直到遇到下一次
`style(...)`。

包裹符会逐项添加，不会把多项合并成一个大包裹。

```cpp
fslog::style("[", "]") << fslog::tag("APP") << fslog::level(fslog::severity::info)
```

会让后续每一个输出项分别带上 `[` 和 `]`。

`style(left, right)` 不指定颜色，因此 `level(...)` 仍可在颜色功能开启时使用默认等级色。

`style(fslog::color::none, left, right)` 表示明确指定无颜色，因此会覆盖 `level(...)` 的默认等级色。

`style()` 等同于：

```cpp
fslog::style(fslog::color::none, "", "")
```

| 调用                                      | 后续左右修饰符 | 后续普通文本颜色 | 后续 `level(...)` 颜色 |
|-----------------------------------------|---------|----------|--------------------|
| `style("[", "]")`                       | `[`、`]` | 不着色      | 使用等级默认色            |
| `style(fslog::color::yellow, "<", ">")` | `<`、`>` | 黄色       | 黄色                 |
| `style(fslog::color::none, "[", "]")`   | `[`、`]` | 不着色      | 不着色                |
| `style()`                               | 无       | 不着色      | 不着色                |

示例：

```cpp
const auto msg = fslog::buffer(line)
                 << fslog::tag("APP")
                 << fslog::style(fslog::color::yellow, "<", ">")
                 << "warning"
                 << fslog::text("name")
                 << fslog::tag("NET")
                 << fslog::style("", "")
                 << " normal";
```

输出中 `warning`、`text("name")` 和 `tag("NET")` 都会分别被 `<`、`>` 包裹并着色，`normal` 会恢复默认样式。

## 数值格式化

`fslog` 不直接格式化整数、浮点数、布尔值、枚举、十六进制等业务值。

需要输出数值时，调用方应先使用外部 formatter、轻量 `printf` 替代库，或者项目自己的转换函数生成文本。

示例：

```cpp
char value_text[16];

const std::size_t value_len = project_format_u32_dec(
    value_text,
    sizeof(value_text),
    adc_value
);

const auto msg = fslog::buffer(line)
                 << fslog::tag("ADC")
                 << " value="
                 << fslog::text(value_text, value_len)
                 << "\r\n";
```

十六进制示例：

```cpp
char address_text[16];

const std::size_t address_len = project_format_u32_hex(
    address_text,
    sizeof(address_text),
    address
);

const auto msg = fslog::buffer(line)
                 << "addr=0x"
                 << fslog::text(address_text, address_len)
                 << "\r\n";
```

如果项目使用轻量 formatter，例如可裁剪浮点支持的 `printf` 替代库，也应在外部完成格式化，再把结果交给 `fslog`。

`fslog` 不关心 formatter 的来源，只要求传入的数据已经是文本。

## 线程安全

`fslog` 不使用全局可变状态。不同线程使用独立 `builder` 和独立缓冲区时可并发使用；共享同一个 `builder`、同一块缓冲区或同一个输出后端时，调用方负责同步。

## 颜色

颜色由 `color_mode` 统一控制：

```cpp
fslog::buffer(line);                       // 默认不输出颜色
fslog::buffer(line, fslog::color_mode::on);
fslog::buffer(line, fslog::color_mode::off);
```

支持的 ANSI 前景色：

```cpp
fslog::color::none
fslog::color::black
fslog::color::red
fslog::color::green
fslog::color::yellow
fslog::color::blue
fslog::color::magenta
fslog::color::cyan
fslog::color::white
fslog::color::bright_black
fslog::color::bright_red
fslog::color::bright_green
fslog::color::bright_yellow
fslog::color::bright_blue
fslog::color::bright_magenta
fslog::color::bright_cyan
fslog::color::bright_white
```

启用颜色时，`fslog` 会在彩色字段后输出 ANSI reset 序列。缓冲区空间不足时，彩色字段会为 reset 序列预留空间，避免终端颜色泄漏到后续输出。

## 截断行为

`fslog` 永远不会写出调用方提供的缓冲区容量。

如果缓冲区容量为 `N`，最多写入 `N - 1` 个有效字符，并保证末尾存在 `'\0'`。

```cpp
char line[16];

const auto msg = fslog::buffer(line)
                 << "this message is longer than buffer";

uart_write(msg.data(), msg.size());
```

当输出内容超过容量时，日志会被截断。调用方应根据实际日志长度选择合适的缓冲区大小。

## 输出后端

`fslog` 不负责发送日志。构造完成后，调用方可以把结果交给任意输出后端：

```cpp
const auto msg = fslog::buffer(line)
                 << fslog::tag("USB")
                 << " configured"
                 << "\r\n";

usb_cdc_write(reinterpret_cast<const uint8_t *>(msg.data()), msg.size());
```

UART 示例：

```cpp
uart_write(msg.data(), msg.size());
```

RTT 示例：

```cpp
rtt_write(msg.data(), msg.size());
```

文件或环形缓冲区示例：

```cpp
log_ring_write(msg.data(), msg.size());
```

## 推荐分层

在嵌入式项目中，推荐分层如下：

```text
业务代码
  ↓
外部 formatter / 项目数值转换函数
  ↓
fslog 日志结构构造
  ↓
App_Log_Write / UART / USB CDC / RTT / SWO
```

示例：

```cpp
char line[128];
char value_text[16];

const std::size_t value_len = project_format_u32_dec(
    value_text,
    sizeof(value_text),
    adc_value
);

const auto msg = fslog::buffer(line)
                 << fslog::level(fslog::severity::info)
                 << ' '
                 << fslog::tag("ADC")
                 << " value="
                 << fslog::text(value_text, value_len)
                 << "\r\n";

App_Log_Write(
    reinterpret_cast<const uint8_t *>(msg.data()),
    msg.size()
);
```

`fslog` 不应该直接依赖具体硬件输出通道，输出通道也不应该理解日志字段语义。二者之间只通过已经构造好的文本消息连接。
