# smtp-nel-filter


## 简介


基于[NEL](https://github.com/siegfried415/libnel)语言，网络安全系统的协议开发人员和安全规则开发人员可以独立地、高效地定义一个攻击检测系统，为了让上述开发人员进一步了解NEL开发语言，我们制作了smtp-nel-filter，以演示如何利用NEL进行协议分析引擎和攻击检测引擎的开发。该系统包括：

a) 协议分析引擎（./src/\*.c, ./src/\*.re），这部分代码是在libnids提供的ids开发框架基础上，增加了对smtp协议和mime协议的分析功能，演示了如何在C语言中调用libnel提供的API进行协议分析引擎的开发；

b) NEL规则，本项目提供了一些使用NEL定义的协议分析规则（./rules/smtp/smtp.nel）和攻击检测规则(./rules/smtp/attacks.nel)，帮助规则开发人员利用NEL提供的描述能力来写出高效的协议规则和攻击规则；


## 编译
确保系统已经安装libnids以及libnids所需的libpcap、libnet、libgthread-2.0、libglib2.0，然后安装re2c库（re2c的作用是根据*.re文件自动转换成协议分析的*.c源文件），最后确保正常编译和安装libnel和libnlib，当上述准备工作完成后:

```console
make
```

## 执行
通过如下的命令，读取rules/smtp/smtp.nel中的nel协议规则文件（以及攻击规则）生成协议分析引擎和攻击检测引擎，对smtp_data.pcap文件中的数据包进行分析，协议分析和攻击检测结果将会输出到控制台上。

```console
./src/smtp-nel-filter -r ./pcap/smtp_data.pcap ./rules/smtp/smtp.nel
```


