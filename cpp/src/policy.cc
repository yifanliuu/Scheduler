#include "policy.h"
#include <stdio.h>
#include <queue>
#include <cassert>
#include <cmath>

struct priority_task
{
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


static std::priority_queue<priority_task> ReadyTasks;
static std::priority_queue<priority_task> BlockedTasks;

static Event::Task curr_cpu_info;

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

  //Priority Queue
  int cpu = current_cpu;
  int io = current_io;
  Action act;

  for(unsigned int i = 0; i< events.size();i++){

    //0. 时钟中断
    if(events[i].type == Event::Type::kTimer){
      //如果当前cpu中有任务
      if(current_cpu){
        int score = cal_score(curr_cpu_info, events[i].time);

        ReadyTasks.push(priority_task(curr_cpu_info, score));
        
        //printf("ReadyTask: %d\n", ReadyTasks.front());
        //如果就绪队列不为空
        if(ReadyTasks.empty() == 0){
          cpu = ReadyTasks.top().task.taskId;
          curr_cpu_info = ReadyTasks.top().task;
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
        //保存当前cpu信息
        curr_cpu_info = events[i].task;
      }
      //如果cpu中有任务
      else{
       ReadyTasks.push(priority_task(events[i].task, cal_score(events[i].task, events[i].time)));
      }
    }

    //任务结束
    //2. 如果是任务完成
    if(events[i].type == Event::Type::kTaskFinish){
      //如果就绪队列不为空
      if(cpu == 0){
        if(!ReadyTasks.empty()){
          cpu = ReadyTasks.top().task.taskId;
          //保存当前cpu信息
          curr_cpu_info = ReadyTasks.top().task;
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
          cpu = ReadyTasks.top().task.taskId;
          //保存当前cpu信息
          curr_cpu_info = ReadyTasks.top().task;
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
        BlockedTasks.push(priority_task(events[i].task, cal_score(events[i].task, events[i].time)));
      }
    }

    //4. 如果是I/O操作完成
    if(events[i].type == Event::Type::kIoEnd){

      //单个event情况
      if(io == 0){
         //如果Blocked队列未空，则取Block队列中的下一个task放入io
        if(!BlockedTasks.empty()){
          io = BlockedTasks.top().task.taskId;
          BlockedTasks.pop();
        }
        else{
          io = 0;
        }
      }

      //cpu资源空闲时
      if(cpu == 0){
        cpu = events[i].task.taskId;
        curr_cpu_info = events[i].task;
      }
      //不空闲时，放入ready队列
      else{
        ReadyTasks.push(priority_task(events[i].task, cal_score(events[i].task, events[i].time)));
      }   
    }
    //if(events.size()>1 && !have_ktimer)printf("- event[%d]: next cpu: %d, next io: %d\n",i, cpu, io);

  }

  act.cpuTask = cpu;
  act.ioTask = io;
  

  return act;
}