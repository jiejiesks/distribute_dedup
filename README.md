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

先在linux环境下在fscryptctl目录下输入make进行编译，然后将可执行二进制文件fscryptctl拷贝进/data目录下，输入以下命令赋予权限

```
chmod 777 /data/fscryptctl
```

输入以下命令生成密钥：

```
head -c 64 /dev/urandom > /data/key
```

将密钥加入文件系统

```
/data/fscryptctl add_key /data/service/el2/100/hmdfs/account < /data/key
```

输入以上这条命令会得到一串字符串，这里假定字符串为f12fccad977328d20a16c79627787a1c，实际情况请按照系统给出的字符串来操作。

输入以下命令给目录加密

```
fscryptctl set_policy f12fccad977328d20a16c79627787a1c /data/service/el2/100/hmdfs/account/files
```

然后将需要测试的文件拷贝进/mnt/hmdfs/100/account/device_view/local/files目录中

然后==重启==，此时/mnt/hmdfs/100/account/device_view/local/files为加密状态，可以通过ls命令查看到文件状态均为乱码，类似以下这种情况

```
AcbnATV97HZzxlmWNoErWS8QkdgTzMzbPU5hjs7XwvyralC5fQCtQA
qXT50ks2,3RzC8kqJ5FvnHgxS6oL2UDa8nsVkCFmoUQQygA3nWzxfA
```

然后在加密状态下进行重删，进行以下操作。

将client_HM、server_HM、dedup.ko放入/data目录下，并赋予可执行权限+x。

配置ip地址输入命令

```
ifconfig eth0 192.168.5.1
```

在/mnt/hmdfs/100/account/device_view/local目录下创建hard目录

先执行服务器的server_HM，然后给每个客户端执行client_HM。（需要几个客户端就在define.h的TOTALTHREAD宏定义配置几，如需要2个客户端则#define TOTALTHREAD 2）

待客户服务器通信结束，在/data目录下执行以下命令

```
insmod dedup.ko
```

插入内核模块实现重定位操作。