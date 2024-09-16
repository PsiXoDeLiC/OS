#include <stdio.h>
#include <stdlib.h>
#include "task.h"
#include "list.h"
#include "cpu.h"
#include "schedulers.h"

struct node *tasks = NULL;
int countTasks = 0;
float start_time = 0.0;
float total_turnaround_time = 0.0;
float total_waiting_time = 0.0;
float total_response_time = 0.0;

void add(char *name, int priority, int burst) {
    Task *newTask = malloc(sizeof(Task));
    newTask->name = name;
    newTask->priority = priority;
    newTask->burst = burst;
    countTasks++;
    insert(&tasks, newTask);
}

void sort_by_burst() {
    struct node *sorted = NULL;
    struct node *current = tasks;

    while (current != NULL) {
        struct node *next = current->next;

        if (sorted == NULL || sorted->task->burst >= current->task->burst) {
            current->next = sorted;
            sorted = current;
        } else {
            struct node *temp = sorted;
            while (temp->next != NULL && temp->next->task->burst < current->task->burst) {
                temp = temp->next;
            }
            current->next = temp->next;
            temp->next = current;
        }

        current = next;
    }
    tasks = sorted;
}

void pick_NextTask(){
    if (tasks == NULL) {
        return;
    }

    while (tasks) {
        Task *currentTask = tasks->task;
        total_response_time += start_time;
        total_waiting_time += start_time;
        run(currentTask, currentTask->burst);
        start_time += currentTask->burst ;
        total_turnaround_time += start_time;
        delete(&tasks, currentTask);
    }
}

void schedule() {
    sort_by_burst();
    pick_NextTask();
    float avg_turnaround_time = total_turnaround_time / countTasks;
    float avg_waiting_time = total_waiting_time / countTasks;
    float avg_response_time = total_response_time / countTasks;
    printf("Average Turnaround Time: %.2f\n", avg_turnaround_time);
    printf("Average Waiting Time: %.2f\n", avg_waiting_time);
    printf("Average Response Time: %.2f\n", avg_response_time);
}
