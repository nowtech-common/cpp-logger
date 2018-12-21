# C++ template-based log library

Originally, this library was designed to use in an embedded environment as a debugging log library.
It was designed to be lean and versatile
with different options for tailoring the logger behavior for smaller
or larger MCUs. 

There are two log call flavours:
  - The all-purpose function call-like solution, but it heavily relies on variadic templates, stack usage may be in the order of kilobytes for many passed parameters. **This holds for all the tasks using the log system.** For low stack usage, avoid many parameters. For small compiled size, avoid many parameter footprints (i.e. avoid many template instantiations).
  - The `std::ostream`-like solution, which uses minimal stack. It has only one limitation: it is unable to surpress the header like the other one's sendNoHeader call.

The library reserves some buffers and a queue during construction, and makes
small heap allocations during thread / log app registrations. After it, no heap
modification occurs, so it will be safe to use from embedded code.

The code was written to possibly conform MISRA C++ and High-Integrity
C++.

The library is published under the [MIT license](https://opensource.org/licenses/MIT).

Copyright 2018 Now Technologies Zrt.

## API

The library is divided into two classes:

  - `nowtech::Log` for high-level API and template logic.
  - `nowtech::LogOsInterface` and subclasses for OS or library-dependent layer.

The nowtech::Log class provides a high-level template based interface for logging characters, C-style
strings, signed and unsigned integers and floating point types. This
class was designed to use with 32 bit MCUs with small memory, performs
serialization and number conversion itself. It uses a heap-allocated
buffers and requires logging threads to register themselves. To ensure
there is enough memory for all application functions, object
construction and thread registration should be done as early as
possible.  

### The class operates roughly as follows:

The message is constructed on the caller stack and is broken into
chunks identified by the artificially created task ID.
Therefore, all tasks should register themselves on the very beginning
of their life. Calling from ISRs is also possible, but all such calls will get the
same ID.

These resulting chunks (log message parts) are then fed into a
common queue, either blocking to wait for free place in the queue or letting
chunks dropped. Concurrent loggings can get their chunks interleaved
in the queue. If the queue becomes full, chunks will be dropped, which
results in corrupted messages.

An algorithm ensures the messages are de-interleaved in the order of
their first chunk in the queue and fed into one of the two transmission buffers,
while the other one may be transmitted. A timeout value is
used to ensure transmission in a defined amount of time even if the
transmission buffer is not full.  
This class can have a stub implementation in an other .cpp file to
prevent logging in release without the need of #ifdefs and macros.

The Log class shall have only one instance, and can be accessed via a
static method.  
If code size matters, the template argument pattern should come from a
limited= number parameter type combinations, and possibly only a
small number of parameters.

### Log initialization

The following steps are required to initialize the log system:

1.  Create a `nowtech::LogConfig` object with the desired settings.
    This instance should not be destructed.
2.  Create an object of the desired `Nowtech::LogOsInterface` subclass. This instance should not be destructed.
3.  Create the Log instance using the above two objects. This instance should not be destructed.
4.  Register the required topics or application areas to let only a
    subset of logs be printed using `Log::registerApp`. These are enum
    values, and can be defined in `nowtech::LogApp`. For example, if
    you have there `System`, `Connection` and `Watchdog` defined, and
    you only register `Connection` and `Watchdog`, all the calls
    starting with `Log::send(nowtech::LogApp::cSystem` will be
    discarded.
5.  Call `Log::registerCurrentTask();` in all the tasks you want to log
    from. This is necessary for the log system to avoid message interleaving from different tasks.

Note, the Log system behaves as a singleton, and defining two instances won't work.

### Log configuration

Log message format can be controlled

  - by the contents of the `LogConfig` object used during initialization. These are application-wide settings.
  - by the actual method template called and its parameters. These control only this message or one of its parameters.

Format of numeric values are described by `nowtech::LogFormat`. Its
constructor takes two parameters:

  - base: numeric system base, only effective for integer numbers. Can
    be 2, 10 or 16, all other values result in error message. Floating
    point numbers always use 10.
  - fill: number of digits to emit with zero fill, or 0 if no fill.

#### LogConfig

The default values defined in this struct are good for general-purpose
use.

Field | Possible values | Default value | Effect
------|-----------------|---------------|--------
`cDefault`|constant     |LogFormat(10, 0)|This is the default logging format.
`cNone`|constant        |LogFormat(0, 0)|Can be used to omit printing the next parameter = (-:
`cBn` |constant         |LogFormat(2, *n*)|Used for n-bit binary output, where *n* can be 8, 16, 24 or 32.
cD*n* |constant         |LogFormat(10, *n*)|Used for n-digit decimal output, where n can be 2-8.
cX*n* |constant         |LogFormat(16, *n*)|Used for n-digit hexadecimal output, where *n* can be 2, 4, 6 or 8.
`logFromIsr`|bool       |false          |If false, log calls from ISR are discarded. If true, logging from ISR works. However, in this mode the message may be truncated if the actual free space in the queue is too small.
`chunkSize`|uint32_t    |8              |Total message chunk size to use in queue and buffers. The net capacity is one less, because the task ID takes a character. Messages are not handled as a string of characters, but as a series of chunks. '\\n' signs the end of a message.
`queueLength`|uint32_t  |64             |Length of a queue in chunks. Increasing this value decreases the probability of message truncation when the queue stores more chunks.
`circularBufferLength`|uint32_t|64      |Length of the circular buffer used for message sorting, measured also in chunks. This should have the same length as the queue, but one can experiment with it.
`transmitBufferLength`|uint32_t|32      |Length of a buffer in the transmission double-buffer pair, in chunks. This should have half the length as the queue, but one can experiment with it. To be absolutely sure, this can have the same length as the queue, and the log system will also manage bursts of logs.
`appendStackBufferLength`|uint16_t|34   |Length of stack-reserved buffer for number to string conversion. The default value is big enough to hold 32 bit binary numbers. Can be reduced if no binary output is used and stack space is limited.
`pauseLength`|uint32_t|100              |Length of a pause in ms during waiting for transmission of the other buffer or timeout while reading from the queue.
`refreshPeriod`|uint32_t|100            |Length of the period used to wait for messages before transmitting a partially filled transmission buffer. The shorter the value the more prompt the display.
`blocks`|bool           |true           |Signs if writing the queue from tasks can block or should return on the expense of possibly losing chunks. Note, that even in blocking mode the throughput can not reach the theoretical throughput (such as UART bps limit). **Important\!** In non-blocking mode high demands will result in loss of complete messages and often an internal lockup of the log system.
`taskRepresentation`|`cNone`, `cId`, `cName`|TaskRepresentation::cId|Representation of a task in the message header, if any. It can be missing, numeric task ID or OS task name.
`appendBasePrefix`|bool |false          |True if number formatter should append 0b or 0x.
`taskIdFormat`|see LogFormat above|`cX2`|Format for displaying the task ID in the message header, if it is displayed as ID.
`tickFormat`|see LogFormat above|`cD5`|Format for displaying the OS ticks in the header, if any. Should be `LogFormat::cNone` to disable tick output.
`int8Format`|see LogFormat above|`cDefault`|Applies to numeric parameters of this type without preceding format parameter.
`int16Format`|see LogFormat above|`cDefault`|Applies to numeric parameters of this type without preceding format parameter.
`int32Format`|see LogFormat above|`cDefault`|Applies to numeric parameters of this type without preceding format parameter.
`int64Format`|see LogFormat above|`cDefault`|Applies to numeric parameters of this type without preceding format parameter.
`uint8Format`|see LogFormat above|`cDefault`|Applies to numeric parameters of this type without preceding format parameter.
`uint16Format`|see LogFormat above|`cDefault`|Applies to numeric parameters of this type without preceding format parameter.
`uint32Format`|see LogFormat above|`cDefault`|Applies to numeric parameters of this type without preceding format parameter.
`uint64Format`|see LogFormat above|`cDefault`|Applies to numeric parameters of this type without preceding format parameter.
`floatFormat`|see LogFormat above|`cD5`|Applies to numeric parameters of this type without preceding format parameter.
`doubleFormat`|see LogFormat above|`cD8`|Applies to numeric parameters of this type without preceding format parameter.
`alignSigned`|bool      |false          |If true, positive numbers will be prepended with a space to let them align negatives.

### Invocation

#### All-purpose solution

The API has for static method templates, which lets the log system be
called from any place in the application:

  - `static void send(LogApp aApp, Args... args) noexcept;`
  - `static void send(Args... args) noexcept;`
  - `static void sendNoHeader(Args... args) noexcept;`
  - `static void sendNoHeader(Args... args) noexcept;`

The ones with the name `send` behave according to the stored configuration and the actual parameter list.
The ones with the name `sendNoHeader` skip printing a header, if any defined in the config.
The ones receiving the `LogApp` parameter will emit the message
preceded by the string used to register the given `LogApp = ;`parameter
if it was actually registered. If not, the whole message is discarded.
The ones without the `LogApp` parameter will emit the message
unconditionally.

Examples:
```cpp
Log::send(nowtech::LogApp::cSystem, "uint64: ", uint64, " int64: ", int64);
Log::send("uint64: ", uint64, " int64: ", int64);
Log::sendNoHeader(nowtech::LogApp::cSystem, "uint64: ", uint64, " int64: ", int64);
Log::sendNoHeader("uint64: ", uint64, " int64: ", int64);
```

#### std::ostream-like solution

The following entry points are available:
  - `LogShiftChainHelper operator<<(ArgumentType const aValue) noexcept;`
  - `LogShiftChainHelper operator<<(LogApp const aApp) noexcept;`
  - `LogShiftChainHelper operator<<(LogFormat const &aFormat) noexcept;`
  - `LogShiftChainHelper operator<<(LogShiftChainMarker const aMarker) noexcept;`

Due to operator overloading, static access is not available, so the Log instance has to be required:

```cpp
Log::i() << uint8 << ' ' << int8 << Log::end;
Log::i() << nowtech::LogApp::cSystem << uint8 << ' ' << int8 << Log::end;
Log::i() << LC::cX2 << uint8 << int8 << Log::end;
Log::i() << Log::end;
```

Available parameter types to print:

Type        |Printed value          |If it can be preceded by a LogFormat parameter to change the style
------------|-----------------------|-------------------------------------------------------------------
`bool`      |false / true           |no
`char`      |the character          |no
`char*`     |the 0 terminated string|no
`uint8_t`   |formatted numeric value|yes
`uint16_t`  |formatted numeric value|yes
`uint32_t`  |formatted numeric value|yes
`uint64_t`  |formatted numeric value|yes
`int8_t`    |formatted numeric value|yes
`int16_t`   |formatted numeric value|yes
`int32_t`   |formatted numeric value|yes
`int64_t`   |formatted numeric value|yes
`float`     |formatted numeric value in exponential form|yes
`double`    |formatted numeric value in exponential form|yes
anything else, like pure `int`|`-=unknown=-`|no

The logger was initially designed for 32-bit embedded environment with possible few binary-to-printed
converter template function instantiation. From 8 to 32 bit numbers only the 32-bit versions will be created.
Using 64-bit numbers makes the compiler create the 64-bit version(s) as well, depending on the signedness
of the numbers to log.

## OS interface

Doxygen docs for `nowtech::LogOsInterface` follows:

Abstract base class for OS/architecture/dependent log functionality
under the Log class. These objects can be chained under the containing
Log  objects to let logging happen into several sinks. The instance
directly referenced by the Log object will contain OS thread to let
the actual write into the sink(s) happen independently of the
logging.  
Since this object requires OS resources, it must be constructed
during initialization of the application to be sure all resources are
granted.

### Implementations

There are several pluggable OS interfaces.

Header name            |Target          |Extensively tested|Description
-----------------------|----------------|------------------|------------
lognop.h               |imaginary /dev/null|yes            |No-operation interface for no output at all. Can be used to appearantly shut down logging at compile time.
logstmhal.h            |An STM HAL UART device|yes         |An interface for STM HAL making immediate transmits from the actual thread. This comes without any buffering or concurrency support, so messages from different threads may interleave each other.
logfreertosstmhal.h    |An STM HAL UART device|yes         |An interface for STM HAL under FreeRTOS, tested with version 9.0.0. This implementaiton is designed to put as little load on the actual thread as possible. It makes use of the built-in buffering and transmits from its own thread.
logcmsisswo.h          |CMSIS SWO       |not yet           |An interface for CMSIS SWO making immediate transmits from the actual thread. This comes without any buffering or concurrency support, so messages from different threads may interleave each other.
logfreertoscmsisswo.h  |CMSIS SWO       |not yet           |An interface for CMSIS SWO under FreeRTOS, tested with version 9.0.0. This implementaiton is designed to put as little load on the actual thread as possible. It makes use of the built-in buffering and transmits from its own thread.
logstdthreadostream.h  |std::ostream    |not yet           |An interface using STL (even for threads) and boost::lockfree::queue. Thanks to this class, this implementation is lock-free. Note, this class does not own the std::ostream and does nothing but writes to it. Opening, closing etc is responsibility of the user code. The stream should NOT throw exceptions. Note, as this interface does not know interrupts, skipping a thread registration will prevent logging from that thread.

## Compiling

The compiler must support the C++14 standard. The library was taken from a set of libraries. Maybe some HAL-related umbrella header file is missing, which is easy to reconstruct.

Compulsory files are:
  - BanCopyMove.h
  - cmsis_os_utils.cpp
  - cmsis_os_utils.h
  - log.cpp
  - log.h
  - logutil.cpp
  - logutil.h

One of these headers, and if present the related .cpp is also needed:
  - lognop.h
  - logstmhal.h
  - logfreertosstmhal.h
  - logfreertosstmhal.cpp
  - logcmsisswo.h
  - logfreertoscmsisswo.h
  - logfreertoscmsisswo.cpp
  - logstdthreadostream.h
  - logstdthreadostream.cpp

Some interfaces need HAL callbacks. `logfreertoscmsisswo.h` needs such a function:

```cpp
extern "C" void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
  if(huart == &huart3) {
    nowtech::LogFreeRtosCmsisSwo::transmitFinished();
  }
  else { // nothing to do
  }
}
```

`logfreertosstmhal.h` needs a similar one with the corresponding class name.

## TODO

  - Static entry point using a static variable stored in the
    constructor. This would be the place to distinguish between stub
    and functional versions.
  - Eliminate std::map.
  - Fix the lockup bug happening under extreme loads.
  - Fix FreeRTOS isInterrupt.
  - Move interrupt checking int OsInterface
