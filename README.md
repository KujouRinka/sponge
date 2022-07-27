For build prereqs, see [the CS144 VM setup instructions](https://web.stanford.edu/class/cs144/vm_howto).

### 进度

- [x] lab0: networking warmup
- [x] lab1: stitching substrings into a byte stream
- [x] lab2: the TCP receiver
- [x] lab3: the TCP sender
- [x] lab4: the TCP connection
- [x] lab5: the network interface
- [ ] lab6: the IP router
- [ ] lab7: putting it all together

### 碎碎念

目前实现了直到 lab4 的所有内容，后续看情况补全 lab5 和 lab6 的代码。

目前可以在此拓展的有：

- [ ] 添加快速重传
- [ ] 添加 Tahoe 或者 Reno
- [ ] 添加 SACK
- [ ] 实现 ACK 捎带

个人机器的配置：

- Ubuntu 20.04.3 LTS
- Intel(R) Core(TM) i7-10710U CPU @ 1.10GHz × 12

~~目前的性能，由于我实现的 `byte_stream` 性能问题，benchmark 结果为：~~

```
CPU-limited throughput                : 0.39 Gbit/s
CPU-limited throughput with reordering: 0.37 Gbit/s
```

~~若替换为讲义中提到的高效 `byte_stream`，结果为：~~

```
CPU-limited throughput                : 3.02 Gbit/s
CPU-limited throughput with reordering: 2.44 Gbit/s
```

~~后续研究透彻后会将修改后的 `byte_stream` 补上。~~

已优化 `byte_stream` 具体性能，见下：

- [x] 优化 `byte_stream` 实现

最终优化后的性能如下：
```
CPU-limited throughput                : 4.64 Gbit/s
CPU-limited throughput with reordering: 3.61 Gbit/s
```

## Sponge quickstart

To set up your build directory:

	$ mkdir -p <path/to/sponge>/build
	$ cd <path/to/sponge>/build
	$ cmake ..

**Note:** all further commands listed below should be run from the `build` dir.

To build:

    $ make

You can use the `-j` switch to build in parallel, e.g.,

    $ make -j$(nproc)

To test (after building; make sure you've got the [build prereqs](https://web.stanford.edu/class/cs144/vm_howto) installed!)

    $ make check_labN *(replacing N with a checkpoint number)*

The first time you run `make check_lab...`, it will run `sudo` to configure two
[TUN](https://www.kernel.org/doc/Documentation/networking/tuntap.txt) devices for use during
testing.

### build options

You can specify a different compiler when you run cmake:

    $ CC=clang CXX=clang++ cmake ..

You can also specify `CLANG_TIDY=` or `CLANG_FORMAT=` (see "other useful targets", below).

Sponge's build system supports several different build targets. By default, cmake chooses the `Release`
target, which enables the usual optimizations. The `Debug` target enables debugging and reduces the
level of optimization. To choose the `Debug` target:

    $ cmake .. -DCMAKE_BUILD_TYPE=Debug

The following targets are supported:

- `Release` - optimizations
- `Debug` - debug symbols and `-Og`
- `RelASan` - release build with [ASan](https://en.wikipedia.org/wiki/AddressSanitizer) and
  [UBSan](https://developers.redhat.com/blog/2014/10/16/gcc-undefined-behavior-sanitizer-ubsan/)
- `RelTSan` - release build with
  [ThreadSan](https://developer.mozilla.org/en-US/docs/Mozilla/Projects/Thread_Sanitizer)
- `DebugASan` - debug build with ASan and UBSan
- `DebugTSan` - debug build with ThreadSan

Of course, you can combine all of the above, e.g.,

    $ CLANG_TIDY=clang-tidy-6.0 CXX=clang++-6.0 .. -DCMAKE_BUILD_TYPE=Debug

**Note:** if you want to change `CC`, `CXX`, `CLANG_TIDY`, or `CLANG_FORMAT`, you need to remove
`build/CMakeCache.txt` and re-run cmake. (This isn't necessary for `CMAKE_BUILD_TYPE`.)

### other useful targets

To generate documentation (you'll need `doxygen`; output will be in `build/doc/`):

    $ make doc

To format (you'll need `clang-format`):

    $ make format

To see all available targets,

    $ make help
