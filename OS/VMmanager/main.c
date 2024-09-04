#include <stdio.h>

#define PAGE_MAX_SIZE 256
#define TLB_MAX_SIZE 16
#define FRAME_SIZE 256

const int mask = 255;

FILE *addresses;
FILE *store;
FILE *output;

struct page{
    int page;
    int frame;
};

int tlb_size = 0;
int frame_size = 0;
int pages_size = 0;
int processed_pages = 0;
int page_fault_cnt = 0;
int tlb_hit = 0;

struct page tlb[TLB_MAX_SIZE];
struct page pages[PAGE_MAX_SIZE];
char physic[FRAME_SIZE][FRAME_SIZE];


int from_store(int num) {
    if (frame_size >= PAGE_MAX_SIZE || pages_size >= PAGE_MAX_SIZE) {
        return -1;
    }

    fseek(store, num * FRAME_SIZE, SEEK_SET);
    fread(physic[frame_size], sizeof(char), FRAME_SIZE, store);

    pages[pages_size].page = num;
    pages[pages_size].frame = frame_size;
    pages_size++;

    return frame_size++;
}

void insert_tlb(int page, int frame) { // сдвигаем записи вверх удаляя самую старую, для добавления новой
        int i;
        for (i = 0; i < tlb_size; i++) {
            if (tlb[i].page == page) {
                while (i < tlb_size - 1) { // свдигаем записи вверх для освобождения места под новую запись
                    tlb[i] = tlb[i + 1];
                    i++;
                }
                break;
            }
        }

        if (i == tlb_size) // если не совпално вставляем в конец
            for (int j =0; j < i; j++)
                tlb[j] = tlb[j + 1];

        tlb[i].page = page;
        tlb[i].frame = frame;

        if (tlb_size < TLB_MAX_SIZE - 1 )
            tlb_size++;

    }

int get_frame(int logic) {
    int offset = logic & mask;
    int num = (logic >> 8) & mask;

    int frame = -1;

    for (int i = 0; i <= tlb_size; i++) {
        if (tlb[i].page == num) {
            frame = tlb[i].frame;
            ++tlb_hit;
            break;
        }
    }

    for (int i = 0; i < pages_size && frame == -1; ++i) {
        if (pages[i].page == num) {
            frame = pages[i].frame;
            break;
        }
    }

    if (frame == -1) {
        frame = from_store(num);
        ++page_fault_cnt;
    }
    insert_tlb(num, frame);
    
    
      freopen("output.txt", "a", stdout);
    
    printf("Virtual address: %d Physical address: %d Value: %d\n", logic, (frame << 8) | offset, physic[frame][offset]);

    return 0;
}



int main(int argc, char *argv[]) {

    addresses = fopen(argv[1], "r");
    if (argc != 2 || addresses == NULL){
        printf("input file not found\n");
        return 1;
    }
    store = fopen("BACKING_STORE.bin", "r");
    if (store == NULL){
        printf("backing_store not found\n");
        return 1;
    }

    int logic = 0;

    while (fscanf(addresses, "%d", &logic) == 1){
        get_frame(logic);
        ++processed_pages;
    }

    printf("\nPage fault frequency = %0.2f%%\n", page_fault_cnt * 100. / processed_pages);
    printf("TLB frequency = %0.2f%%\n", tlb_hit * 100. / processed_pages);
    return 0;
}
