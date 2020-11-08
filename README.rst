pRedis： 带有miss penalty感知的redis
============================

本项目是修改自注释版的 Redis 源码，
原始代码来自： https://github.com/antirez/redis 。

在现有redis的基础上，
1) 增加了key-value占用内存的追踪、key-level的miss penalty的追踪，并通过动态规划划分其中各个penalty class的内存。
2) 同时增加了自动dump/load内存的机制，实现内存波峰波谷的平滑转换。
3) 动态策略切换机制，可以在pRedis(pla)和HC策略之间进行动态切换。


Have fun!
