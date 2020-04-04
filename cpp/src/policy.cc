#include "policy.h"
#include <stdio.h>
#include <queue>
#include <cassert>

static std::queue<int> ReadyTasks;
static std::queue<int> BlockedTasks;

Action policy(const std::vector<Event>& events, int current_cpu, int current_io) {

 // test
 /*
  bool have_ktimer = 0;
  for (size_t i = 0; i < events.size(); i++)
  {
    if(events[i].type == Event::Type::kTimer){
      have_ktimer = 1;
    }
  }
  

  if(events.size()>1 && !have_ktimer){
    printf("\ncurr_cpu: %d, curr_io: %d\n", current_cpu, current_io);
    for(unsigned i = 0; i<events.size();i++){
      printf("  time: %d, type: %d, task: %d\n", events[i].time, events[i].type, events[i].task.taskId);
    }
  }
*/




  //Round-Robin
  int cpu = current_cpu;
  int io = current_io;
  Action act;

  for(unsigned int i = 0; i< events.size();i++){

    //0. 时钟中断
    
    if(events[i].type == Event::Type::kTimer){
      //如果当前cpu中有任务
      if(current_cpu){
        ReadyTasks.push(current_cpu);
        //printf("ReadyTask: %d\n", ReadyTasks.front());
        //如果就绪队列不为空
        if(ReadyTasks.empty() == 0){
          cpu = ReadyTasks.front();
          //printf("cpu: %d\n", cpu);
          ReadyTasks.pop();
        }
        else{
          cpu = 0;
        }
      }
      
    }
    


    //1.任务到达
    if(events[i].type == Event::Type::kTaskArrival){
      //如果上一个任务没在运行
      if(current_cpu == 0){
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
      if(cpu == 0){
        if(!ReadyTasks.empty()){
          cpu = ReadyTasks.front();
          ReadyTasks.pop();
        }
        else{
          cpu = 0;
        }
      }   
    }

    //3. 如果被阻塞
    if(events[i].type == Event::Type::kIoRequest){
      //如果readytasks不为空, 并且将Ready队列中的下一个task放入CPU
      if(cpu == 0){
        if(!ReadyTasks.empty()){
          cpu = ReadyTasks.front();
          ReadyTasks.pop();
        }
        //否则cpu空闲
        else{
          cpu = 0;
        }
      }
      

      //当I/O资源空闲时，进行I/O
      if(io == 0){
        io = events[i].task.taskId;
      }
      //不空闲时，放入阻塞队列
      else{
        BlockedTasks.push(events[i].task.taskId);
      }
    }

    //4. 如果是I/O操作完成
    if(events[i].type == Event::Type::kIoEnd){

      //单个event情况
      if(io == 0){
         //如果Blocked队列未空，则取Block队列中的下一个task放入io
        if(!BlockedTasks.empty()){
          io = BlockedTasks.front();
          BlockedTasks.pop();
        }
        else{
          io = 0;
        }
      }

      //cpu资源空闲时
      if(cpu == 0){
        cpu = events[i].task.taskId;
      }
      //不空闲时，放入ready队列
      else{
        ReadyTasks.push(events[i].task.taskId);
      }   
    }
    //if(events.size()>1 && !have_ktimer)printf("- event[%d]: next cpu: %d, next io: %d\n",i, cpu, io);

  }

  act.cpuTask = cpu;
  act.ioTask = io;
  

  return act;
}