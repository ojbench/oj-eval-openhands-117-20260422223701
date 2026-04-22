# Problem 117 - Xiaoh的缓存置换

**ACMOJ Problem ID**: 2509

## Table of Contents

- [Problem 117 - Xiaoh的缓存置换](#problem-117-xiaoh的缓存置换)
  - [Table of Contents](#table-of-contents)
  - [Introduction](#introduction)
    - [Background](#background)
  - [Assignment Description](#assignment-description)
    - [Grade Composition](#grade-composition)
  - [Assignment Requirements](#assignment-requirements)
    - [Input Format](#input-format)
    - [Output Format](#output-format)
    - [Samples](#samples)
    - [Data Constraints](#data-constraints)
  - [Per-Testcase Resource Limits](#per-testcase-resource-limits)
  - [Test Data](#test-data)
  - [Submission Requirements](#submission-requirements)
    - [Submission Format](#submission-format)
    - [Submission Guidelines](#submission-guidelines)
    - [Evaluation Notes](#evaluation-notes)
    - [Academic Integrity](#academic-integrity)

## Introduction

### Background

This problem tests algorithmic problem-solving skills.

## Assignment Description

### Grade Composition

| Grading Component | Percentage |
| :--: | :--: |
| Pass **2509. Xiaoh的缓存置换** | 80% |
| Code Review | 20% |

Here are several points that need clarification:

- In the Code Review, we will **strictly examine your code style and repository organization structure, etc.**

- This assignment provides some sample data for testing, stored in the `/workspace/data/117/data_test/` directory. Note that these are not the test cases on the Online Judge. Passing all local test cases does not guarantee that you will pass the OJ tests.

- Besides the provided sample data, we also encourage you to design your own test data based on your program logic to assist debugging.

## Assignment Requirements

### Problem Description

> 请注意，本题**不允许使用STL中的任何容器**，违者的成绩将被Voided。如果有存疑的部分，请咨询助教。

> 请注意，本题**不允许访问除给定网址以外的任何网页**，**不允许尝试从给定网页跳转**，否则将视为严重学术诚信问题

下发文件: [attachment]down.zip[/attachment] / [jbox](https://jbox.sjtu.edu.cn/l/71r7NK)

众所周知，计算机的存储硬件是分级的，其层级结构(Memory Hierarchy)如下所示：

<div style="text-align: center">
<img src="https://acm.sjtu.edu.cn/OnlineJudge/oj-images/76c3c4fc-a39e-4795-9bbf-984f56b4b2b3" style="width: 50%">
</div>


可以看到，在上图中，越靠上的储存介质速度越快，但容量等各种限制越多。例如：一台笔记本电脑中，L1 Cache大约只有512KB, L2 Cache大约有8MB，而DRAM可以有64GB或128GB（只要你有钱），SSD的大小则可以是达到几TB。

但实际使用时，我们却很少能够感受到内存和磁盘读写带来的巨大延时，这是因为计算机在存储上采用了一种策略：将更可能被访问的数据块从下一级存储介质临时复制到上一级存储介质，以加快这些数据块的访问速度，这也就是我们通常称为的缓存。一个好的缓存可以做到让你的电脑仿佛拥有“与最大的存储介质一样大的空间，同时拥有与最快的存储介质一样快的速度”。

但是这带来了一个问题：上一级的存储介质远小于下一级，这意味着我们被迫选择一部分“更容易”被访问的内容缓存至上一级。缓存内容的选择通常基于如下两个假设：

- 空间的局部性（Space Locality）：如果一个数据被访问，那么它附近的数据更容易被访问。
- 时间的局部性（Time Locality）：如果一个数据被访问，那么它在不远的未来更容易被再次访问。

因此我们将数据划分为很多大小相同的连续块，称为页（Page）。每次访问某个数据的时候，我们将缓存其所在的整个页。

上述策略在缓存没满的时候很显然是可以工作的，那么当缓存区域满了该如何处理呢？我们需要使用某种策略“剔除”（Evict）相对而言最不太可能被访问的页。这种策略称为缓存文件置换策略（Page Replacement Algorithm）。

常见的缓存置换策略有：
- FIFO(First In, First Out)：每次需要置换时，选择最早加入缓存的一个页进行置换。
- LRU(Least Recently Used): 选择**最后一次访问**最早的那一个页进行置换。
- MRU(Most Recently Used)：选择**最后一次访问**最晚的那一个页进行置换。
- LRU-K: 选择**倒数第$k$次访问**最早的那一个进行置换。如果最近的访问不满$k$次，那么认为其倒数第$k$次访问的时间是$-\infty$，优先置换。如果有多个不满$k$次的访问，则选择其**最早一次访问**最早的那一个进行置换。

为了避免歧义，我们对上面所述的“最早”、“最晚”做如下约定：当我们说“最后一次访问”时，我们指的是时间轴上最后、离“当前时刻”最近的一次访问，“倒数第$k$次访问”也是同理，是离“当前时刻”第$k$近的访问；当我们说“最早一次访问”时，我们指的是时间轴上最前，离“当前时刻”最远的一次访问。

接下来我们给出一个例子：
假设缓存的大小为$3$个页，我们访问的页用正整数编号。
如果我们访问的顺序为`1 2 3 1 3 4 2 1 3`，那么各个策略的置换操作如下：

| 访问页编号 | FIFO缓存池   | LRU缓存池    | MRU缓存池    | LRU-2缓存池  | 备注                                          |
| ---------- | ------------ | ------------ | ------------ | ------------ | --------------------------------------------- |
| 1          | 1            | 1            | 1            | 1            | 新访问页1，所有池子都加入这个页               |
| 2          | 1 2          | 1 2          | 1 2          | 1 2          | 新访问页2，所有池子都加入这个页               |
| 3          | 1 2 3        | 1 2 3        | 1 2 3        | 1 2 3        | 新访问页3，所有池子都加入这个页               |
| 1          | 1 2 3        | 1 2 3        | 1 2 3        | 1 2 3        | 访问的页在每个缓存池中，不需要修改            |
| 3          | 1 2 3        | 1 2 3        | 1 2 3        | 1 2 3        | 访问的页在每个缓存池中，不需要修改            |
| 4          | 2 3 4(置换1) | 1 3 4(置换2) | 1 2 4(置换3) | 1 3 4(置换2) | 新访问页4，不在每个缓存池中，故各需置换一个页 |

FIFO缓存池在这一步置换最早加入的页，也就是1；LRU缓存池在这一步置换最后一次访问最早的页，也就是2；MRU缓存池置换最后一次访问时间最晚的页，也就是3；LRU-2置换倒数第$2$次访问最早的，由于只有$2$只被访问了一次，因此他的倒数第二次访问时间被认为是$-\infty$，置换2。

| 访问页编号 | FIFO缓存池 | LRU缓存池    | MRU缓存池    | LRU-2缓存池 | 备注                                      |
| ---------- | ---------- | ------------ | ------------ | ----------- | ----------------------------------------- |
| 2          | 2 3 4      | 2 3 4(置换1) | 1 2 4        | 1 2 3       | 访问2不在LRU和LRU-2缓存池，故各置换一个页 |
| 1          | 1 3 4      | 1 2 4(置换3) | 1 2 4        | 1 2 3       | 访问1不在LRU缓存池，故置换一个页          |
| 3          | 1 3 4      | 1 2 3(置换4) | 2 3 4(置换1) | 1 2 3       | 访问3不在LRU、MRU缓存池，故各置换一个页   |

现在Xiaoh希望写缓存置换系统，支持上述$4$种策略的缓存置换系统，并且能够在其中各策略间切换。但是由于他还忙着（准备）写CPU，因此把这个任务交给你，你能帮他完成这个任务吗？

<details> <summary>（小科普，不看不影响写题）为什么我们需要MRU？</summary>
我知道很多人觉得MRU看起来很蠢，似乎并不make sense。但是它的存在是有意义的（而不是为了凑数）：当你顺序地扫描大片的数据时，你会往缓存里面加入非常多的页，其中的绝大部分都只会被访问一次（毕竟是扫描），但是它们会污染你的缓存系统（因为他们的时间戳都很新，你一般不太会置换掉它们）。在这个时候，越新加入的页是越没有意义的，因此需要被立刻置换掉以免它们污染你的缓存池。
</details>

### Input Format



### Output Format



### Samples

No sample data provided for this problem.

### Data Constraints

对于20%的数据，保证：只涉及`FIFO`策略；

对于50%的数据，保证：只涉及`FIFO`、`LRU`、`MRU`策略；

对于最后20%的测试数据，我们将进行一定的复杂度测试，我们要求各个函数的实现复杂度如下（设缓存池大小为$n$）：
- `Visit`、`TryEvict`、`RemovePage`：$O(n)$
- 其余所有函数：$O(1)$

对于每一个Subtask，如果你通过正确性测试，你能够获得这个Subtask 50%的分数；如果你同时通过正确性测试和内存泄漏测试，你能够获得这个Subtask 100%的分数。

## Resource Limits

- **Time Limit**: 2000 ms
- **Memory Limit**: 64 MiB

## Per-Testcase Resource Limits

- **Time Limit (per test case)**: 2000 ms
- **Memory Limit (per test case)**: 64 MiB
- **Disk Usage**: No disk usage is permitted.

## Test Data

The test data for this problem is located at `/workspace/data/117/data_test/`.

Attachments and starter files for this problem are located at `/workspace/data/117/`.

## Submission Requirements

### Submission Format

Submit a single C++ source file to the Online Judge. The OJ provides its own `main.cpp` which `#include "src.hpp"`. Your submission will be written to `src.hpp` and compiled together with the OJ's driver code.

### Submission Guidelines

- The submitted code must be able to compile successfully through the above compilation process
- The program needs to be able to read data from standard input and write results to standard output
- Please ensure the code runs correctly within the given time and space limits
- **You must use C++ or C language** to implement this assignment

### Evaluation Notes

- The evaluation system will test your program using the provided test data
- The program output must exactly match the expected output (including format)
- Exceeding time or memory limits will be judged as the corresponding error type
- Please pay attention to the overall time performance of your code and the time complexity of each part of your algorithm.

### Academic Integrity

If any violations are found during evaluation or code review (including but not limited to using unconventional methods to pass test cases), your final score may be significantly reduced or become **0 points**.
