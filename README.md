# 多设备协同去重

## User

由于RK3568使用的是arm64架构，而linux上用的基本都是X86架构，因此需要用到交叉编译器。详细配置可见此网站https://segmentfault.com/a/1190000040169088。配置好交叉编译器后，确定需要几个客户端，修改define.h中TOTALTHREAD的宏定义。

然后在dedup目录下执行make命令，获得client_HM和server_HM两个二进制可执行文件。

## Kernel

替换openharmony3.1.1 Release源码对应的目录

在kernel/linux/linux-5.10/include/linux/fs.h中，添加一个宏定义S_DEDUP

```
#define S_VERITY	(1 << 16) /* Verity file (using fs/verity/) */
#define S_DEDUP		(1 << 17) /*for dup file*/
```

然后编译即可

## Kernel_modules

在out/kernel/src_tmp/linux-5.10/drivers目录下，创建一个dedup目录，然后将kernel_modules的三个文件拷贝至目录中，然后修改drivers目录下的Makefile和Kconfig

Kconfig

```
source "drivers/dedup/Kconfig"
```

Makefile

```
obj-$(CONFIG_DEDUP)		+= dedup/
```

然后在out/kernel/src_tmp/linux-5.10目录下输入以下命令

```
export OHOS_ROOT=~/Documents/DevEco/Projects/RK3568
export PATH=$OHOS_ROOT/prebuilts/clang/ohos/linux-x86_64/llvm/bin:$PATH
export PATH=$OHOS_ROOT/prebuilts/gcc/linux-x86/arm/gcc-linaro-7.5.0-arm-linux-gnueabi/bin:$PATH
export MAKE_OPTIONS="ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- CC=clang HOSTCC=clang"
export PRODUCT_PATH=vendor/hihope/RK3568
./make-ohos.sh TB-RK3568X0
```

进而编译出dedup.ko内核模块

## RK3568

将client_HM和server_HM放入/data目录下，并赋予可执行权限+x。

先执行服务器的server_HM，然后给每个客户端执行client_HM。

待客户服务器通信结束，将dedup.ko放入/data目录下输入命令

```
insmod dedup.ko
```

插入内核模块实现重定位操作。