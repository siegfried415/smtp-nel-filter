# smtp-nel-filter


## 简介

利用应用程序本身固有的弱点或基本的通信协议中的弱点的攻击是目前攻击者们所使用的主要攻击手段，对以，以阻隔网络级攻击为主要目的的防火墙已经不能担负起防范攻击的重任，因此，需要使用多层安全网关来保护公司网络免受这些威胁，入侵防御系统（IPS）就是在这种背景下产生的。

从2000年BlackICE公司提出了inline保护这个概念，国外的主流IPS厂商已经经历了以软件为主的第一代IPS到以硬件为主要特征的第二代IPS（1G\~2G）的过渡。和第一代基于软件的产品不同，第二代产品由于使用了ASIC或者FPGA技术，在性能上有了极大的提升。

但是，在第二代IPS产品中，开发商过于注重IPS作为一个网关设备的通过效率，而在检测的深度、规则的适用性、以及可扩充性上投入不够。比如某些特定的攻击的检测逻辑都是由一个硬件来完成的，但随着攻击方式的日益增加、攻击水平的日益提高，这种结构的IPS的弱点将会逐渐暴露出来。

为了克服这些硬件解决方案的局限，东软在流过滤平台基础上，提出了事件过滤平台，使得攻击规则的作者能够基于[NEL](https://github.com/siegfried415/nel)快速地定制攻击规则，而不再需要每新增加一条规则，就要重构系统，这大幅度地降低了开发的难度，能够有效地检测和防止应用层攻击。

为了吸引广大开发者了解NEL技术，并在这个平台系统上进行协议和攻击检测规则的开发，我们使用NEL制作了smtp-nel-filter，该系统的总体软件结构包括：

a) 应用级协议分析引擎，这部分代码使用C语言来编写，它负责根据流过滤系统提供的可靠的数据流进行基本的协议分析，并向上层的事件流过滤系统提供基本事件，在研究版中，我们给出了SMTP实现的源代码，供开发人员参考；

b) NEL事件过滤平台，首先用户使用NEL书写协议限制规则和攻击防范规则，平台将根据这些规则生成高效的事件过滤引擎，该引擎将完成攻击的检测和防范工作，从而降低了攻击分析成本；

![软件体系结构](/image/image1.PNG)


## 应用级协议分析技术

目前，直接针对网络层的攻击越来越少，更多的攻击是试图利用应用级的弱点，这是由于下面几个原因造成的：首先，该层包含了黑客的最终目标------真正的用户数据。其次，应用层支持许多协议，所以它包含了大量潜在的攻击方法。第三，因为应用层有更多的安全漏洞，所以相对于较低的那些层，在该层检测并防范攻击更加困难。

这种攻击方法的重要变化需要入侵防御系统去理解应用程序的行为以抵御对应用程序的攻击和入侵。为此我们使用了协议分析技术，以有效对抗异常网络流量，发现高级的变形攻击。协议分析包括协议识别和协议异常检测两部分工作，前者能够根据协议规范分析出原始数据祯中的含义，后者根据RFC、以及厂商定义的专有通信规则识别不符合的流量，一旦出现异常，系统可以根据相应的安全决策做出响应。

协议异常检测主要有三个优点：

1. 因为具有对协议上下文的精确理解，相对于基于包的检测，大大提高了检测的准确性；

2. 能够检测一些偏离了协议标准的未知攻击;

3. 可以大大缩小搜索范围，从而大幅度地提高了检测效率；

## NetEye 事件过滤平台

NEL事件过滤平台是一个独立于应用的事件过滤、分析系统，该系统使用NEL（Network Event Language）来定义规则，这些规则经过NEL编译器编译后，在内存中形成可以执行的分析器，该分析器将对一系列输入事件进行过滤和分析。NEL编译器和分析器的总控逻辑都封装在一个名为libnel.a的函数库中，用户只需要将libnel.a联编到自己的程序中，就能快速构造一个事件过滤系统。

为了描述攻击，IDS、IPS厂商都制作了一些攻击描述语言，比如NFR的N-Code语言、Emerald的P-BEST语言、STAT的STATL语言，此外Bro和Snort也各自有相应的攻击描述语言，但是上述的这些系统都没有清楚地将协议分析、攻击规则定义、攻击检测这些过程彻底地分开，这就导致必须由一个规模比较大的开发小组来维护整个系统的开发。

为了解决这个问题，我们设想了另外的一种开发模式，即将上述工作分配给协议分析小组、攻击分析小组、和事件过滤平台（以后将简称为平台）小组，由各小组独立完成，这样每个小组都在其领域内对系统进行不断地扩充而又不会影响其他小组。具体来说，平台小组负责维护事件检测逻辑的开发（如使用多少个线程来进行检测、如何检测、先检测哪些事件,再检测哪些事件等）,这样协议分析人员和攻击检测人员就不必通晓引擎的内部实现机制，便能够独立地处理各自领域内的问题。

为了实现此目标，NEL被设计成一种"事件描述语言"，它具有常用过程性语言的语言实体（变量、函数），能够以过程性的方式操作这些语言实体来完成运算。不仅如此，NEL还提供了事件、以及定义事件之间关系的规则。通过"事件"我们将平台、协议分析和攻击检测联系在一起。事件规则是描述性的，NEL编译器负责将这些描述性规则转换为过程性的内部结构，并能够高效地实现对事件的逐层推理。

NEL的核心设计思想是权衡描述能力、易用性、运行效率三者之间的关系，找到一个最佳的平衡点。一方面，NEL具有很强的描述能力，可以使管理者非常方便地描述各种应用逻辑，包括时序事件序列（顺序、继承、聚合等）和约束关系（包括各种逻辑表达式和谓词函数）；另一方面，NEL对规则在内存中的组织方式进行了大幅优化，将运行期所必需的运算降低到最低，提高了引擎的执行效率。

通过对特定NEL脚本进行编译自动生成某个特定协议的引擎的流程请见图4.1：

![协议引擎的生成流程](/image/image2.PNG)

一般来说，使用平台实现某个应用级引擎，协议开发者首先必须使用C语言来书写协议分析过程中所需的处理函数（如从数据包中分析出协议事件的函数、以及对某些协议约束关系进行检验的函数），这些处理函数经过编译后生成目标代码（\*.o），然后和引擎库（libnel.a）链接后就形成了可执行的二进制文件。

其次，协议开发者需要使用NEL来书写协议规则，协议规则的主要目的是将C语言中定义的内存数据影射为NEL所能够理解的事件，这是通过event关键字来实现的，这些用NEL来描述的协议事件将为攻击规则开发者提供一个事件编程接口。

最后，攻击规则开发者将根据自己对于攻击特征的了解，使用上述事件编程接口对攻击进行定义（比如定义某个事件内部变量之间的约束关系，或者定义多个事件之间的约束关系），在此过程中，规则开发者依然可以使用宿主语言中的各种语言实体。

在这种模式下，C语言是一个宿主语言，而NEL是一个寄生语言，平台提供了无缝地使用宿主语言中的语言实体的功能，使得NEL的作者能够方便地利用C语言的遗产代码。

生成的引擎可以以"解释模式"和"编译模式"这两种方式下执行，在解释模式下，事件引擎直接根据语法树进行分类，在编译模式下，将调用编译后的分析函数而不再使用语法树进行分类，编译模式的分类效率要比解释模式高出一个数量级。在研究版中，我们只提供解释模式。

## 规则

根据规则和真实攻击的关联程度，我们可以定义五种类型的规则：协议限制规则、漏洞规则、攻击规则、行为规则、和用户自定义应用级访问控制规则，这五类规则的叠加既可以提高检测的准确性，又能够有效地降低开发成本。

### 协议限制规则

协议限制规则负责协议异常的识别。经过我们的调查，有相当比例的攻击可以通过协议规则就能够发现，比如对于攻击CAN-1999-0098,CAN-1999-0284, CAN-1999-1529, CVE-2000-0042, CVE-2000-0488,CVE-2000-0507, CAN-2000-0657, CAN-2003-0264,
CAN-2004-1291，都可以通过限制"EHLO"请求头的长度而被阻断，相应的协议限制规则ehlo_long.nel定义如下：

<pre><em>
BAD_LONG_EHLO: ehlo_req( $1-> host_name_len >= 512 ) {
    smtp_deny($0, "found a long ehlo command! len =%d\n", $1->len );
}
;
</em></pre>

这是一条非常简单的规则，它只是规定在接收到一个"EHLO"请求事件时，如果该请求事件的请求数据长度（host_name_len）大于512字节时，就关闭掉这条SMTP连接。

我们再看另外一个例子，根据RFC 2821，我们知道在一个合法的SMTP连接过程中，只有在客户端至少发送过一个"RCPT TO"请求命令之后，客户端发送"DATA"请求命令才是合法的，所以我们可以阻挡住所有那些违反这一点的"DATA"请求命令：
<pre><em>
BAD_DATA_REQ: data_req($0->mail_state < SMTP_MAIL_STATE_RCPT ) {
    smtp_deny($0, "Haven 't seen RCPT TO command before this DATA command!\n");
}
;
</em></pre>

Cisco PIX Firewall Bypass攻击就违背了RFC中关于"RCPT TO"的规定，Cisco PIX Firewall可以做SMTP应用层的内容过滤，用户可以配置PIX，使得PIX只允许某些SMTP命令通过，而象EXPN、VRFY等危及安全的命令则丢弃掉。但是它允许位于\"data\" 和
\"\<CR\>\<LF\>\<CR\>\<LF\>.\<CR\>\<LF\>\"之间的所有数据通过，这些数据通常作为邮件正文内容出现，位于这些数据中的EXPN之类的关键字并不过滤抛弃掉。

但是如果在rcpt to命令之前发送data命令，SMTP SERVER将返回503错误，指出需要先发送rcpt to命令。然而，PIX防火墙意识不到这种欺诈行为，它始终允许这之后的数据通过，直到收到 \"\<CR\>\<LF\>\<CR\>\<LF\>.\<CR\>\<LF\>\"。攻击者可以利用这个漏洞做很多事情。
<pre>
helo ciao
mail from: pinco@pallino.it
data ( From here pix disable fixup)
expn guest ( Now i could enumerate user vrfy oracle and have access to all command)
help
whatever command i want
quit
</pre>

这样，即便我们在书写协议限制规则时对具体的攻击（Cisco PIX Firewall Bypass）一无所知，我们也能根据RFC阻挡住相当一部分的攻击，这使得我们的IPS具有一定程度的0-Day保护功能。

### 漏洞规则

对于某类攻击，如果我们能够分析出这类攻击的本质特征（即攻击者为了利用某漏洞不可避免地要使用的攻击手段），那么我们就可以忽略掉这些具体攻击中的特定成分，而以漏洞的本质特征来定义规则，这样定义的规则就叫做漏洞规则。

举例来说，我们发现几乎所有的针对缓冲区溢出的攻击，都是由两部分代码构成的：一部分是利用程序的脆弱性使得程序偏离正常流程的引导代码，另一部分是在引导代码得手后具体进行的攻击实现代码（打开端口、启动一个shell,植入一个木马等）。

为了检测出前者，我们必须研究每个具体攻击的引导部分，然而由于IPS几乎不可能知道被攻击的机器的内存分布情况，因此确切的检测是不可能的。但是对于后者，由于攻击者往往使用相对固定的几种方式来进行，我们就比较容易地根据攻击代码来做出准确的检测。

根据这种思路，我们实现了一个shellcode的检测函数，该函数使用高效的解码算法来判断输入是否是一段具有某种特征的连续机器码。该实现和具体的漏洞、协议实现无关，甚至可以跟OS无关，而只和被攻击的机器的体系结构有关，实验证明这个函数具有比较高的准确率和效率。

利用shellcode检测函数对SMTP中的"VRFY"请求进行缓冲区溢出检测的NEL规则如下：
<pre><em>
event struct smtp_cmd_vrfy VRFY_SHELL_CODE;
VRFY_SHELL_CODE: vrfy_req( nel_has_shellcode($1->mail_box, $1->mail_box_len)) {*
    print("found machine instructions in ehlo command, this maybe a shell code!\n");
    return $1;
}
;

BAD_VRFY_REQ: VRFY_SHELL_CODE {
}
;
</em></pre>

同协议限制规则一样，漏洞规则也具备一定的抽象性，它可以使得开发者能够以比较少的开放代价来实现对于不确定多数的攻击的检测，使得smtp-nel-filter具有一定程度的0-Day保护功能。

###  攻击规则

根据攻击的某个具体的特征定义的检测规则，这种规则又叫签名（signature），攻击规则未必能够表现漏洞的本质，因而可能存在漏报问题。

另外，如果我们已经有一个事先存在的漏洞规则，那么我们对某一特定攻击有了深入的了解，以致于能够精确地确定它的特征的情况下，我们就可以使用二重规则的方式来定义攻击规则，亦即将攻击规则定义到漏洞规则之上，下面我们以C-Mail SMTP Server Remote Buffer Overflow 攻击为例来说明这点：

<pre>
event struct smtp_cmd_vrfy VRFY_SHELL_CODE;
BAD_VRFY_REQ: VRFY_SHELL_CODE(nel_frag_match($1->mail_box, $1->mail_box_len, "\x83\xc1\x0A\xFF\xE1\")) {
    smtp_deny($0, "Found a C-Mail SMTP Server Remote Buffer Overflow Exploit!\n");
    return 0;
}
;
</pre>

请注意在上面的定义中，攻击事件不是定义在*vrfy_req*这个原子事件（从协议分析引擎直接传递过来的事件）之上，而是定义到*VRFY_SHELL_CODE*这个漏洞事件之上的，也就是说，对于*vrfy_req*事件，当它满足 *nel_has_shellcode(\$1-\>mail_box,
\$1-\>mail_box_len)*  的判断后，它将形成一个*VRFY_SHELL_CODE*事件，在此基础之上，如果该*VRFY_SHELL_CODE*事件还能满足*nel_frag_match(\$1-\>mail_box,\$1-\>mail_box_len, \"\\x83\\xc1\\x0A\\xFF\\xE1\" )*，那么它将形成一个
*BAD_VRFY_REQ*事件，并将连接阻断。

这样，当我们有对于某种攻击的精确规则，我们就可以方便地检测出这种攻击。

即使我们没有攻击规则，只要我们有对于该攻击的漏洞规则存在，我们还是有可能发现它。这使得smtp-nel-filter在不断地扩充规则的过程中，还能保持比较好的检测能力。

### 行为规则

有一些恶意行为，我们无法通过单包、或者单事件来检测到，而必须综合考虑多个包、事件之间的关系，才能进行准确的检测，我们称这样的规则叫为"行为规则"。在NEL中可以使用变量或者多事件规则来书写行为规则，另外，我们还要介绍一种基于字符流的检测方法，这其实是一种隐含的行为规则。

1. 使用变量来保存状态

在NEL中，\$0用来指向连接节点的指针，当SMTP服务器对客户的EHLO请求应答时，将服务器所能接收的命令保存到\$0-\>cmd_allow中，那么当后续的MAIL FROM请求到达时，我们可根据保存在连接节点中的cmd_allow来判断MAIL FROM之后所携带的关键字是否合法。

例如：
<pre><em>
BAD_MAIL_REQ: mail_req( $1->key == SMTP_MAIL_SIZE && $0->cmd_allow[SMTP_ALLOW_SIZE] == 0 ) { 
        smtp_deny($0, "found a MAIL FROM command with keyword SIZE without declaration of SIZE at previous EHLO ACK!\n");
}
;
</em></pre>

在NEL中，我们不仅可以像C语言一样地使用数组脚标，还能够无缝地使用在C语言中定义的枚举（enum）常量，这给我们书写NEL规则带来了极大的便利。

2. 使用多事件来保持状态

根据前面的介绍，Cisco PIX Firewall Bypass攻击的问题就出在，在发送rcpt to命令之前就发送了data命令，我们可以定义多事件规则来检测它：
<pre><em>
BAD_COMDS: data_req($0->mail_state == SMTP_MAIL_STATE_ORIGN ) ack($2->code == 503) {
        smtp_deny($0, "There maybe a Cisco PIX Firewall SMTP Content Filting Evasion Attack!\n");
}
;
</em></pre>
3. 基于状态的字符流过滤

在SMTP的协议描述中，TEXT是以这种方式定义的：
<pre><em>
TEXT : text
     | TEXT text ;
</em></pre>

这意味着，每当SMTP协议分析引擎传递给事件过滤引擎一个text原子事件，事件过滤引擎就会形成一个TEXT事件，TEXT中的数据保存到一个struct stream的结构中，使用nel_stream_match就能自动字符流的匹配，其中，中间匹配状态的保留都是由系统自动执行的。
<pre><em>
BAD_TEXT: TEXT( nel_stream_match($1, "some bad strings")) {
        smtp_deny($0, "found forbiden string in TEXT!\n");
}
;
</em></pre>

### 用户自定义规则

用户根据自己特定的网络环境以及安全需求自己定义应用级的访问控制规则，这种规则叫做用户自定义规则。最简单的用户自定义的方式是在NEL语言内修改在C语言协议解析代码中定义的协议解析变量，比如
<pre><em>
init{
    var_disable_vrfy_cmd = 1;
}
</em></pre>

有时候，我们需要在连接节点上增加一些历史信息，并根据它来做后续的判断，但是smtp-nel-filter可能并没有在连接节点上为此预留对应的数据位，这时候我们就需要使用NEL的自定义结构体的功能来扩充\$0-\>user字段，比如我们要增加一个vrfy_cnt,
来记录SMTP客户端发送的VRFY请求的次数，
<pre><em>
struct smtp_info_user{
    int vrfy_cnt;
};
</em></pre>

并在连接建立的时候，为\$0-\>user分配空间，并对\$0-\>user-\>vrfy_cnt进行初始化：
<pre><em>
void nel_smtp_info_init(struct smtp_info *info) {
    info->user=(void *)new(sizeof(struct smtp_info_user));
    if(info->user == 0) {
        print("nel_smtp_info_init: nel_malloc failed");
        return;
    }
    ((struct smtp_info_user *)info->user)->vrfy_cnt = 0;
    print("nel_smtp_info_init, vrfy_cnt=", ((structsmtp_info_user *)info->user)->vrfy_cnt);
    return;
}
</em></pre>

在连接关闭的时候，释放掉\$1-\>user所占用的内存。
<pre><em>
void nel_smtp_info_free(struct smtp_info *info) { 
    if(info->user != 0){
        delete(info->user);
    }
}

init {
    register_init($0, nel_smtp_info_init);
    register_free($0, nel_smtp_info_free);
}
</em></pre>

用户自定义的攻击规则可以写成这样：
<pre><em>
BAD_VRFY_REQ: vrfy_req ( ++((struct smtp_info_user *)$0->user)->vrfy_cnt > max_vrfy_cnt ) {
    smtp_deny($0, "found too much of VRFY command !\n");
}
;
</em></pre>

不仅如此，我们还可以在NEL中使用自定义的函数，从而大大提高规则的可定制性，下面我们以字符串匹配为例来说明这个问题。

使用NEL的字符串匹配函数是实现字符串匹配的最简单的方式，此时我们可以使用NEL所支持的关系算符或者谓词来书写规则，例如为了在NEL中实现对"EHLO"命令的阻断，我们增加一个ehlo_nel_match.nel：
<pre><em>
EHLO_BAD_STRING: ehlo_req(nel_frag_match($1->host_name, $1->host_name_len, "foo"))  {
    smtp_deny($0, "found \"foo\" in ehlo command, this maybe a attack!\n");
}
;
</em></pre>

而又由于NEL支持和C语言类似的表达式判断逻辑，对于那些不是很复杂的逻辑，我们将获得非常高的灵活性，但由于这个判断是在复杂的NEL引擎中进行的，这或多或少地带来了效率的下降。

在某些复杂的情况下，我们可能需要将数据定义在NEL空间，并在NEL中调用C语言中预定义的检测函数（只要该检测函数在C语言中已经定义过）。比如，对于"EHLO"命令，如果我们没有在C语言中定义黑名单数组的检测逻辑，而我们又有这种需求的话，攻击规则的书写人员就可以在NEL中定义这样的一个数组：
<pre><em>
static char *ehlo_black_list[] = {
    "foo",
    "bar",
    "bla\"
};

static struct match_info *ehlo_black_list_tree;
</em></pre>

然后调用C语言中的初始化函数，来对此数组进行初始化
<pre><em>
init{
    ehlo_black_list_tree = c_frag_match_init(ehlo_black_list, 3 );
}
</em></pre>

然后在EHLO事件发生时，我们可以以这种方式来定义用户自定义规则：
<pre><em>
EHLO_BAD_STRING: ehlo_req(c_frag_match($1->host_name, $1->host_name_len, ehlo_black_list_tree)) {
    smtp_deny($0, "found supectious user in ehlo command, this maybe a attack!\n");
}
;
</em></pre>

当系统提供了c_frag_match这种比较高效的检测函数，我们就可以采用这种兼具灵活性和高效的方式来实现。

当然，我们也可以完全使用NEL来完成上述的工作，这种方式具有最高的灵活性,但是这种做法一般来说可能不是高效的。比如，上面的black_list如果全在NEL中实现的话，将会是：
<pre><em>
static int ehlo_black_list_num ;
static char *ehlo_black_list[] = {
    "foo",
    "bar",
    "bla\"
};

init {
    int i;
    ehlo_black_list_num = sizeof(ehlo_black_list) / sizeof(char *);
    for(i = 0; i < ehlo_black_list_num; i++) {
        print("ehlo_black_list[%d] = %s\n\", i, ehlo_black_list[i]);
    }
}
</em></pre>

用户在nel中自行定义my_ehlo_match为（返回值大于0为真，否则为假 )：
<pre><em>
int my_ehlo_black_list_match(char *data, int len) {
    int i;
    for(i = 0; i < ehlo_black_list_num; i++) {
        if(nel_strstr(data, ehlo_black_list[i]) != 0) {
            print("nel_strstr matched!\n");
            return 1;
        }
    }
    return 0;*
}
</em></pre>

然后在MAIL_FROM事件发生时，我们可以以这种方式来定义：
<pre><em>
EHLO_BAD_STRING: ehlo_req(my_ehlo_black_list_match($1->host_name, $1->host_name_len)) {
    smtp_deny($0, "found supectious user in ehlo command, this maybe a attack!\n");
}
;
</em></pre>

这样，smtp-nel-filter通过提供协议限制规则、漏洞规则、攻击规则、行为规则、和用户自定义应用访问控制这五类规则来阻隔各种层次的攻击，这五类规则的叠加既可以提高检测的准确性，又能够有效地降低开发代价。
