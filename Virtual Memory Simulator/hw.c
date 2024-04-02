#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct Memory {
    int address, data;
};
struct PageTable {
    int valid_bit, dirty_bit, page_num, useFreq;
};
struct Memory main_memory[32]; //4 pages mm
struct Memory virtual_memory[128];
struct PageTable p_table[16];  //16 pages vm
int pgn = 0; //check max is reached
int fifo = 0, lru = 0;
int vpgArr[4] = { -1,-1,-1,-1 }; //queue for mm
/*void printvpgArr() {
    printf("vpg array: %i,%i,%i,%i\n", vpgArr[0],vpgArr[1],vpgArr[2],vpgArr[3]);
}*/
void push(int item) {
    if (pgn > 4) return;
    for (int i = 0; i < 4; i++) {
        if (vpgArr[i] == -1) {
            vpgArr[i] = item;
            break;
        }
    }
}
void pop() {
    //if(vpgArr[0]==-1) printf("No item in the queue.\n");

    for (int i = 0; i < 3; i++) {
        //if(i<3 && vpg[i+1]==-1) break; //usually this won't happen since it is called when mm is full
        vpgArr[i] = vpgArr[i + 1];
    }
    vpgArr[3] = -1;
}


void init() {
    for (int i = 0; i < 128; i++) {
        if (i < 32) {
            if (i < 16) {
                p_table[i].valid_bit = 0;
                p_table[i].dirty_bit = 0;
                p_table[i].page_num = i;
                p_table[i].useFreq = 0;
            }
            main_memory[i].data = -1;
            main_memory[i].address = i;
        }
        virtual_memory[i].data = -1;
        virtual_memory[i].address = i;

    }
}
void showtable() {
    for (int i = 0; i < 16; i++) {
        printf("%i:%i:%i:%i\n", i, p_table[i].valid_bit, p_table[i].dirty_bit, p_table[i].page_num);
    }
}
void showmain(char ppn[]) {
    int pg = atoi(ppn);
    if (pg > 3) {
        printf("Please enter 0-3\n");
        return;
    }
    for (int i = pg * 8; i < (pg + 1) * 8; i++) {
        printf("%i: %i\n", i, main_memory[i].data);
    }
}
int getVPN(char args[]) {
    double dvpn = atoi(args) / 8; //divided by 8 addresses
    int vpg = (int)dvpn;
    return vpg;
}
//copy mm to disk and move disk to mm
void switchMemory(int vpg, int vpginVM) {
    int saveAddress = vpg * 8;
    int toVMAddress = vpginVM * 8;
    for (int i = p_table[vpg].page_num * 8; i < ((p_table[vpg].page_num) + 1) * 8; i++) {
        //move mm to vm
        virtual_memory[saveAddress] = main_memory[i];
        //move vm to mm
        main_memory[i] = virtual_memory[toVMAddress];
        saveAddress++;
        toVMAddress++;
    }
    p_table[vpginVM].valid_bit = 1;
    //need to handle dirty bit and usefreq later
    p_table[vpginVM].page_num = p_table[vpg].page_num;
    p_table[vpg].valid_bit = 0;
    p_table[vpg].dirty_bit = 0;
    p_table[vpg].useFreq = 0;
    p_table[vpg].page_num = vpg;
}
void dolru(int newvpg) {
    int least = p_table[vpgArr[0]].useFreq;
    int vpg = vpgArr[0];
    int geti = 0;
    for (int i = 1; i < 4; i++) {
        if (least > p_table[vpgArr[i]].useFreq) {
            least = p_table[vpgArr[i]].useFreq;
            vpg = vpgArr[i];
            geti = i;
        }
    }
    switchMemory(vpg, newvpg);
    vpgArr[geti] = newvpg;
}
void dofifo(int newvpg) {
    int longestVPG = vpgArr[0];
    switchMemory(longestVPG, newvpg);
    pop();
}

void read(char args[]) {
    int vpg = getVPN(args);
    int offset = atoi(args) % 8;
    if (p_table[vpg].valid_bit == 0) {
        printf("A Page Fault Has Occurred\n");
        //move pg to mm
        if (pgn == 4) {
            if (lru == 1) dolru(vpg);
            else dofifo(vpg); //by default, it is fifo 
        }
        else {
            p_table[vpg].page_num = pgn;
            pgn++;
        }
        p_table[vpg].valid_bit = 1;
        push(vpg);
    }
    p_table[vpg].useFreq++;
    int mmaddress = (p_table[vpg].page_num) * 8 + offset;
    printf("%i\n", main_memory[mmaddress].data);
}
void write(char virtual_addr[], char num[]) {
    int vpg = getVPN(virtual_addr);
    int offset = atoi(virtual_addr) % 8;
    int data = atoi(num);
    if (p_table[vpg].valid_bit == 0) {
        printf("A Page Fault Has Occurred\n");
        //move pg to mm
        if (pgn == 4) {
            if (lru == 1) dolru(vpg);
            else dofifo(vpg); //by default, it is fifo 
        }
        else {
            p_table[vpg].page_num = pgn;
            pgn++;
        }
        p_table[vpg].valid_bit = 1;
        push(vpg);
    }
    p_table[vpg].useFreq++;
    p_table[vpg].dirty_bit = 1;
    int mmaddress = (p_table[vpg].page_num) * 8 + offset;
    main_memory[mmaddress].data = data;
}

void clearArgv(char* argv[10]) {
    for (int i = 0; i < 10; i++) {
        argv[i] = NULL;
    }
}
int parseline(char input[100], char* argv[10]) {
    clearArgv(argv);
    char* token = strtok(input, " \n");
    int i = 0;
    while (token != NULL) {
        argv[i] = token;
        i++;
        token = strtok(NULL, " \n");
    }
    return i;
}

void loop() {
    char input[100];
    char* args[10];
    do {
        printf("> ");
        fgets(input, 100, stdin);
        int arg_size = parseline(input, args);
        if (strcmp(args[0], "read") == 0 && arg_size == 2) read(args[1]);
        else if (strcmp(args[0], "write") == 0 && arg_size == 3) write(args[1], args[2]);
        else if (strcmp(args[0], "showtable") == 0) showtable();
        else if (strcmp(args[0], "showmain") == 0 && arg_size == 2) showmain(args[1]);
        else if (strcmp(args[0], "quit") == 0) return;
    } while (1);
}
int main(int argc, char** argv) {
    if (argv[1] == NULL || strcmp(argv[1], "FIFO") == 0) fifo = 1;
    else if (strcmp(argv[1], "LRU") == 0) lru = 1;
    init();
    loop();
    return 0;
}


