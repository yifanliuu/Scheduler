#include "policy.h"
#include <stdio.h>
#include <queue>
#include <cassert>

static std::queue<int> ReadyTasks;
static std::queue<int> BlockedTasks;

Action policy(const std::vector<Event>& events, int current_cpu, int current_io) {
  //FIFO
  int cpu = current_cpu;
  int io = current_io;
  Action act;

  
  //一个even来了之后
  for(unsigned int i = 0; i< events.size();i++){
    //时钟中断 不管
    //1.任务到达
    if(events[i].type == Event::Type::kTaskArrival){
      //如果上一个任务没在运行
      if(current_cpu == 0 && current_io == 0){
        cpu = events[i].task.taskId;
      }
      //如果cpu中有任务
      else{
       ReadyTasks.push(events[i].task.taskId);
      }
    }

    //任务结束
    //2. 如果是任务完成
    if(events[i].type == Event::Type::kTaskFinish){
      //如果就绪队列不为空
      if(!ReadyTasks.empty()){
        cpu = ReadyTasks.front();
        ReadyTasks.pop();
      }
      else{
        cpu = 0;
      }
    }

    //3. 如果被阻塞
    if(events[i].type == Event::Type::kIoRequest){
      cpu = 0;

      //当I/O资源空闲时，进行I/O
      if(current_io == 0){
        io = events[i].task.taskId;
      }
      //不空闲时，放入阻塞队列
      else{
        BlockedTasks.push(events[i].task.taskId);
      }
    }

    //4. 如果是I/O操作完成
    if(events[i].type == Event::Type::kIoEnd){
      //如果Blocked队列未空，则取Block队列中的下一个task放入io
      if(!BlockedTasks.empty()){
        io = BlockedTasks.front();
        BlockedTasks.pop();
      }
      else{
        io = 0;
      }
      // 可以重新开始cpu部分运行
      cpu = events[i].task.taskId;   
    }
    
 

  }

  act.cpuTask = cpu;
  act.ioTask = io;
  return act;
}