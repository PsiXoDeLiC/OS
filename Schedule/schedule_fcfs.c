#include <stdio.h>
#include <stdlib.h>
#include "task.h"
#include "list.h"
#include "cpu.h"
#include "schedulers.h"
float start_time = 0.0;
float total_turnaround_time = 0.0;
float total_waiting_time = 0.0;
float total_response_time = 0.0;
int task_count = 0;

struct node *tasks = NULL;

void add(char *name, int priority, int burst) {
    Task *newTask = malloc(sizeof(Task));
    newTask->name = name;
    newTask->priority = priority;
    newTask->burst = burst;
    insert(&tasks, newTask);
    task_count++;
}

void pick_NextTask() {
    if (!tasks)
        return;

    Task *task = tasks->task;
    delete(&tasks, task);
    total_response_time += start_time;
    total_waiting_time += start_time;
    start_time += task->burst ;
    total_turnaround_time += start_time;

    pick_NextTask();
    run(task, task->burst);
}

void schedule() {
    pick_NextTask();
    float avg_turnaround_time = total_turnaround_time / task_count;
    float avg_waiting_time = total_waiting_time / task_count;
    float avg_response_time = total_response_time / task_count;
    // Print the results
    printf("Average Turnaround Time: %.2f\n", avg_turnaround_time);
    printf("Average Waiting Time: %.2f\n", avg_waiting_time);
    printf("Average Response Time: %.2f\n", avg_response_time);
}
