# Lab: system calls
## System call tracing
要求实现一个类似于strace的系统调用。在调用后每次使用系统调用会打印出调用的信息。

主要思路是在`proc`结构体中增加一项`tracemask`用于标识当前进程是否开启了某个调用的trace。然后仿照其他系统调用的写法新增系统调用，处理好参数以及错误处理。

最后在`syscall`函数中检查当前进程是否开启了该系统调用的trace，如果开启则打印调用信息。调用信息需要另开一张表记录系统调用名称。

## Sysinfo
新增系统调用来获取系统的一些信息。

主要难度在于实现获取剩余内存空间和当前进程数这两个函数上。要求清楚xv6的空闲内存是怎么组织的和进程的管理方式。

xv6使用一个单向链表`kmem.freelist`维护空闲空间。初始时内核将所有的空闲空间分页后，每一个页都被插入到这个链表中。在调用`kalloc`后从中取出一个页，调用`kfree`后插入一个页。所以获取剩余内存空间的方法就是遍历这个链表，统计页的个数，计算出剩余内存空间大小。

xv6使用一个全局数组`proc`来管理进程，所有的进程信息都在这个数组中。只需要遍历这个数组，排除标记为`UNUSED`的进程，就可以得到当前进程数。

统计这些内核共享信息的时候要记得加锁（或许也不用

## 总结

这个lab要求对xv6的系统调用实现方式有一定的理解（除了trap层发生的事情），并且要求对xv6的内存管理和进程管理有一定的了解。这个lab中学到的添加系统调用的方法对后面的lab很有用，
