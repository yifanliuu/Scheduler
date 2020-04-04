# Process Scheduler
- 详细要求见`statement\说明.md`
### Version 1.0 FIFO算法

最初版本假设events列表中只有一个event。

**规则1**：`kTimer`时钟中断时：不进行任何操作。

**规则2**：`kTaskArrival`任务到达时：

- 如果上一个任务结束了，`next_cpu`就设置这个到来的任务；如果CPU中有任务，放入Ready队列中。

**规则3**：`kTaskFinish`任务结束时：

- 如果Ready队列不为空，`next_cpu`设置为Ready队列队头；否则CPU闲置，`next_cpu = 0` 。

**规则4**：`kIoRequest`发出I/O请求时：

- 令CPU闲置。

- 当I/O资源空闲时，`next_io`设为当前请求I/O的task；否则放入Blocked队列中。

**规则5**：`kIoEnd`完成I/O操作时：

- 令`next_io = 0`。

- 当CPU资源空闲时，`next_cpu`设为当前完成I/O的task；否则放入Ready队列中。

### Version 2.0 Round-Robin算法

最初版本假设events列表中只有一个event。

**规则1**：`kTimer`时钟中断时：上一个task释放CPU，下一个task需要CPU

- 如果CPU中有运行的task， 那么就停止，然后将`current_cpu`加入Ready队列中，并将Ready队列中的下一个task设置为`next_cpu`。

**规则2**：`kTaskArrival`任务到达时：意味着需要CPU资源

- 如果CPU中有正在运行的task，那么到达的task加入到Ready队列中；如果CPU空闲，那么`next_cpu`设置为该到达的task。

**规则3**：`kTaskFinish`任务结束时：意味着释放CPU资源

- 如果Ready队列不为空，那么`next_cpu`设置为队列的下一个task；否则，CPU闲置。		

**规则4**：`kIoRequest`发出I/O请求时：意味着释放CPU，需要I/O

- **关于CPU的决策：**如果Ready队列不为空，CPU资源转移给Ready队列的下一个task；否则CPU闲置。
- **关于I/O的决策：**如果当前I/O资源空闲，那么`next_io`为当前task；否则，放入Blocked队列。

**规则5**：`kIoEnd`完成I/O操作时：意味着释放I/O，需要CPU

- **关于I/O的决策：**如果Blocked队列不为空，I/O资源转移给Blocked队列的下一个task；否则I/O闲置。
- **关于CPU的决策：**如果当前CPU资源空闲，那么`next_cpu`为当前task；否则，放入Ready队列。

#### Round-Robin 多event修复

注意到上述版本中，结果都是 **Wrong Answer**，经过测试得知是因为没有考虑events队列中有多个event的情况。通过测试，发现在单event版本中存在两个明显问题：

1. 任务到达`kTaskArrival`和任务结束`kTaskFinish`同时发生，且`kTaskArrival`的处理在`kTaskFinish`的处理之前（因为多event操作时，需要遍历events列表，遍历即有顺序）`kTaskFinish`的处理中`next_cpu`的更改可能会覆盖掉`kTaskArrival`处理的决策，导致某些task的CPU阶段永远无法进行，最后出现 TLE (time limit exceed) 错误。
2. 任务请求I/O`kIoRequest`和`kIoEnd`同时发生，无论他们的顺序如何，同样会出现`next_cpu`和`next_io`的更改可能会覆盖掉之前处理的决策，最后导致TLE错误。

**解决方案**：

1. 在开始判断之前将`next_cpu/io`赋值为`current_cpu/io`，后续对于当前cpu/io资源是否空闲的判断都改为对`next_cpu/io`的判断。
   - 因为第一个event对`next_cpu/io`修改后，第二个event在进行判断的时候就不会更改`next_cpu/io`（因为在以上规则中只有cpu/io资源空闲的时候，才会修改next_cpu/io）

2. 将Round-Robin中的规则3，4，5分别改为：

>**规则3**：`kTaskFinish`任务结束时：意味着释放CPU资源
>
>- ==**如果CPU资源(`next_cpu`)空闲，并且**==如果Ready队列不为空，那么`next_cpu`设置为队列的下一个task；否则，CPU闲置。		

>**规则4**：`kIoRequest`发出I/O请求时：意味着释放CPU，需要I/O
>
>- **关于CPU的决策：**如果Ready队列不为空，CPU资源转移给Ready队列的下一个task；否则CPU闲置。
>- **关于I/O的决策：**如果==**当前I/O资源(`next_io`)**==空闲，那么`next_io`为当前task；否则，放入Blocked队列。

>**规则5**：`kIoEnd`完成I/O操作时：意味着释放I/O，需要CPU
>
>- **关于I/O的决策：**如果Blocked队列不为空，I/O资源转移给Blocked队列的下一个task；否则I/O闲置。
>- **关于CPU的决策：**如果==**当前CPU资源(`next_cpu`)**==空闲，那么`next_cpu`为当前task；否则，放入Ready队列。

### Version 3.0 & 3.1 Score-Based Priority Queue

由于基本的Round-Robin算法的效果并不是很好，考虑进一步优化。优化思路如下：

1. 采取多级反馈队列算法。
   - 在时钟中断发生的时候，需要对当前运行的task降级到下一队列，但是由于无法获取当前运行的task的队列信息，重新开一个数据结构进行储存又比较麻烦，于是这个方案不可行。

2. score-based优先队列。
   - 队列中的task按照score分数进行排序。
   - 其他的规则均不改变。
   - 在这个方案中有两个issue需要解决：**score如何计算？数据结构如何设计？**

#### 数据结构设计

- 优先队列中元素的数据结构：

  ```c++
  struct priority_task{
    Event::Task task;
    int score; // 分数越高，优先级越高
    priority_task(const Event::Task t, int sc){
      task = t;
      score = sc;
    }
  
    bool operator < (const priority_task& p_task) const{
      return score < p_task.score; //大顶堆
    }
  };
  ```

- 优先队列的定义：

  ```c++
  static std::priority_queue<priority_task> ReadyTasks;
  static std::priority_queue<priority_task> BlockedTasks;
  ```

- 由于时钟中断的时候，`current_cpu`的task信息无法获得，从而无法计算score并存入Ready队列中。那么需要一个新的静态变量来储存当前运行的task的信息。（I/O方面没有这种问题）

  ```c++
  static Event::Task curr_cpu_info;
  ```



#### score的计算机制

注意到有以下4个可以利用的数据：

- `task.deadline`
- `event.time`
- `task.arrivalTime`
- `task.priority`

**Version 3.0**

- 利用`task.deadline`和`event.time`信息，提高紧急任务的优先级。

- 另外考虑`task.priority`对score的贡献。

  ```c++
  int cal_score(const Event::Task task, int time){
    int rest_time = task.deadline - time;
    int priority_factor = 0;
    if(task.priority == Event::Task::Priority::kHigh){
      priority_factor = 1000;
    }
    else if(task.priority == Event::Task::Priority::kLow){
      priority_factor = 200;
    }
    return priority_factor - rest_time;
  }
  ```

**Version 3.1**

- 将`task.arrivalTime`纳入考虑范围，惩罚长任务。

  ```c++
  int cal_score(const Event::Task task, int time){
    int rest_time = task.deadline - time;
    int elapsed_time = time - task.arrivalTime;
    int priority_factor = 0;
    //int score = 0;
    if(task.priority == Event::Task::Priority::kHigh){
      priority_factor = 7000;
    }
    else if(task.priority == Event::Task::Priority::kLow){
      priority_factor = 3000;
    }
    if(rest_time <= 0){ //已经过时的任务
      return - __INT32_MAX__;
    }
    return -rest_time + priority_factor-0.5 * elapsed_time;
  }
  ```
---

## 实验总结

- 确实应该先实现一个比较简单的**正确**的算法然后再不断优化。
- 本次实验中三个地方费的时间比较长：
  1. 在一开始写的时候，一直都无法运行，后来发现是I/O结束抢占CPU任务的原因，这样CPU中运行的任务的运行情况（如已在CPU中运行多长时间）将永远丢失，不会再产生让它继续运行的event，从而会导致 TLE 错误。（这一块卡挺久的，后来全部删了重新写FIFO了……）
  2. 第二处是多event修复的时候，需要测试多组数据来发现问题。
  3. 第三处是计算score的函数设计的时候，因为有4个因素需要考虑，所以试了很多次不同的组合……

- 想好再写还是比较重要的，不然比较容易迷失方向o(*￣▽￣*)ブ。

