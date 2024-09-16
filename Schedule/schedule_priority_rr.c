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
    newTask->used = 0;
    countTasks++;
    insert(&tasks, newTask);
}
void sort_by_priority() {
    struct node *sorted = NULL;
    struct node *current = tasks;

    while (current != NULL) {
        struct node *next = current->next;

        if (sorted == NULL || sorted->task->priority <= current->task->priority) {
            current->next = sorted;
            sorted = current;
        } else {
            struct node *temp = sorted;
            while (temp->next != NULL && temp->next->task->priority > current->task->priority) {
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
    while (tasks){
        struct node *current = tasks;
        int prev = 0;
        while(current != NULL) {
            Task *currentTask = current->task;
            if(current->task->used == 0){
                total_response_time += start_time;
                total_waiting_time -= currentTask->burst;
                current->task->used = 1;
            }
            if(current->next != NULL && (current->task->priority == current->next->task->priority || prev > 0)) {
                prev++;
                int executionTime = (currentTask->burst < QUANTUM) ? currentTask->burst : QUANTUM;
                run(currentTask, executionTime);
                start_time += executionTime;
                currentTask->burst -= executionTime;
                if (currentTask->burst <= 0) {
                    prev--;
                    total_turnaround_time += start_time;
                    total_waiting_time += start_time;
                    struct node *temp = current;
                    current = current->next;
                    delete(&tasks, temp->task);
                    free(temp->task);
                    free(temp);
                }else{
                    if(current->task->priority == current->next->task->priority){
                        current = current->next;
                    }
                    else{
                        break;
                    }
                }
            }
            else {
                run(currentTask, currentTask->burst);
                start_time += currentTask->burst;
                total_waiting_time += start_time;
                total_turnaround_time += start_time;
                delete(&tasks, currentTask);
                break;
            }
        }
    }
}

void schedule() {
    sort_by_priority();
    pick_NextTask();
    float avg_turnaround_time = total_turnaround_time / countTasks;
    float avg_waiting_time = total_waiting_time / countTasks;
    float avg_response_time = total_response_time / countTasks;
    printf("Average Turnaround Time: %.2f\n", avg_turnaround_time);
    printf("Average Waiting Time: %.2f\n", avg_waiting_time);
    printf("Average Response Time: %.2f\n", avg_response_time);
}