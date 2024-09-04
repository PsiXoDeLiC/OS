#include <stdio.h>
#include <stdbool.h>
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

void inverse() {
    struct node *prev = NULL;
    struct node *current = tasks;
    struct node *next = NULL;

    while (current != NULL) {
        next = current->next;
        current->next = prev;
        prev = current;
        current = next;
    }
    tasks = prev;
}

void pick_NextTask(){
    bool first = true;
    while (tasks){
        struct node *current = tasks;
        while (current != NULL) {
            Task *currentTask = current->task;
            if(first){
                total_response_time += start_time;
                total_waiting_time -= currentTask->burst;
            }
            int executionTime = (currentTask->burst < QUANTUM) ? currentTask->burst : QUANTUM;
            run(currentTask, executionTime);
            currentTask->burst -= executionTime;
            start_time += executionTime;

            if (currentTask->burst <= 0) {
                total_waiting_time += start_time;
                total_turnaround_time += start_time;
                struct node *temp = current;
                current = current->next;
                delete(&tasks, temp->task);
                free(temp->task);
                free(temp);
            } else {
                current = current->next;
            }
        }
        first = false;
    }
}

void schedule() {
    inverse();
    pick_NextTask();
    float avg_turnaround_time = total_turnaround_time / countTasks;
    float avg_waiting_time = total_waiting_time / countTasks;
    float avg_response_time = total_response_time / countTasks;
    printf("Average Turnaround Time: %.2f\n", avg_turnaround_time);
    printf("Average Waiting Time: %.2f\n", avg_waiting_time);
    printf("Average Response Time: %.2f\n", avg_response_time);
}
