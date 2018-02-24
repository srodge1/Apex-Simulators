#include <stdio.h>
#include <stdlib.h>
#include<string.h>

#ifndef VALID
#define VALID 1
#endif
#ifndef INVALID
#define INVALID 0
#endif

long data_Memory[1000];
int pc = 0;
int line_Number = 0;

void fetch();
void decode();
void execute();         //PC value starts from '0', if 1 is desired substract 1 from pc calculations at every step
void mul2();
void div2();
void div3();
void div4();
void memory();
void writeback();
void intialize();
void simulate(char file_name[]);
void display();

typedef struct {
    long value;
    int status;
    int ins_id;
}registers;

registers register_File[16] = {};

typedef struct {

    int address;  //address offset only for load and store

    char opcode[25];
    int src1;
    int src2;
    int dest;
    int literal;
    long result;
    int index;    // comparing with ind to check if the last arithmatic ins has reached wb // for BZ, BAL & BNZ send the pc value along
    int branch;  // 1 if decoded to be taken 0 otherwise
    int id; // for avoiding the conflict which ins is updating status of the dest reg

} Instructions;
Instructions instruction[100];
Instructions * ptr_instruction = instruction;

const Instructions nop = {0, "nop", 0, 0, 0, 0, 0.0, 0, 0, 0};
Instructions if_input = {0, "nop", 0, 0, 0, 0, 0.0, 0, 0, 0};
Instructions idrf_input = {0, "nop", 0, 0, 0, 0, 0.0, 0, 0, 0};
Instructions iex_input = {0, "nop", 0, 0, 0, 0, 0.0, 0, 0, 0};
Instructions imul2_input = {0, "nop", 0, 0, 0, 0, 0.0, 0, 0, 0};
Instructions idiv2_input = {0, "nop", 0, 0, 0, 0, 0.0, 0, 0, 0};
Instructions idiv3_input = {0, "nop", 0, 0, 0, 0, 0.0, 0, 0, 0};
Instructions idiv4_input = {0, "nop", 0, 0, 0, 0, 0.0, 0, 0, 0};
Instructions imem_input = {0, "nop", 0, 0, 0, 0, 0.0, 0, 0, 0};
Instructions iwb_input = {0, "nop", 0, 0, 0, 0, 0.0, 0, 0, 0};

int last_arithmetic_ind = 100;
long last_arithmetic_result = -10;
int ind = 0;
int bflag = 0; // adds one more cycle for branch instructions
int hflag = 0;
int mflag = 0;
int dflag = 0;
int d1flag = 0;
int d2flag = 0;
int id = 0; //for conflicting register status validation
int ch = 0;


int main() {

    char file_name[20];

    while (1){
        printf("\n Enter '1' for 'INITIALIZATION', '2' to 'SIMULATE', '3' to 'DISPLAY', and '0' to TERMINATE\n\t");
        scanf("%d", &ch);

        if (ch == 1)
            intialize();
        else if (ch == 2){
            printf("\n Enter the filename you want to simulate : \t ");
            scanf("%s", file_name);
            simulate(file_name);
        }
        else if (ch == 3)
            display();
        else if (ch == 0)
            break;
        else
            printf("\n Enter a Valid Command");
    }

}

void simulate(char file_name[]){
    printf("\n\t Enter the no. of clock cycles: ");
    int cycles = 0;
    scanf("%d", &cycles);

    FILE * ptr_File = fopen (file_name,"r");

    char line[255];

    while( feof( ptr_File ) == 0){

        fgets (line, 255, ptr_File);
        printf("\n%d", line_Number);
        printf("\t%s", line);
        line_Number++;

        sscanf(line,"%[^,]", ptr_instruction->opcode);

        if (!(strcmp(ptr_instruction->opcode, "MOVC"))){
            sscanf(line, "%[^,],R%d,#%d", ptr_instruction->opcode, &ptr_instruction->dest, &ptr_instruction->literal);
            printf("\t%s R%d %d", ptr_instruction->opcode, ptr_instruction->dest, ptr_instruction->literal);
            }
                else if (!(strcmp(ptr_instruction->opcode, "STORE"))){
                    sscanf(line, "%[^,],R%d,R%d,#%d", ptr_instruction->opcode, &ptr_instruction->src1, &ptr_instruction->src2, &ptr_instruction->literal);
                    printf("\t%s R%d R%d %d", ptr_instruction->opcode, ptr_instruction->src1, ptr_instruction->src2, ptr_instruction->literal);
                }
                else if (!(strcmp(ptr_instruction->opcode, "LOAD"))){
                    sscanf(line, "%[^,],R%d,R%d,#%d", ptr_instruction->opcode, &ptr_instruction->dest, &ptr_instruction->src1, &ptr_instruction->literal);
                    printf("\t%s R%d R%d %d", ptr_instruction->opcode, ptr_instruction->dest, ptr_instruction->src1, ptr_instruction->literal);
                }
                else if ((!(strcmp(ptr_instruction->opcode, "MUL")) || !(strcmp(ptr_instruction->opcode, "ADD")) || !(strcmp(ptr_instruction->opcode, "SUB")) || !(strcmp(ptr_instruction->opcode, "OR")) || !(strcmp(ptr_instruction->opcode, "AND")) || !(strcmp(ptr_instruction->opcode, "EX-OR")))){
                    sscanf(line, "%[^,],R%d,R%d,R%d", ptr_instruction->opcode, &ptr_instruction->dest, &ptr_instruction->src1, &ptr_instruction->src2);
                    printf("\t%s R%d R%d R%d", ptr_instruction->opcode, ptr_instruction->dest, ptr_instruction->src1, ptr_instruction->src2);
                }
                else if ((!(strcmp(ptr_instruction->opcode, "BZ")) || !(strcmp(ptr_instruction->opcode, "BNZ")))){
                    sscanf(line, "%[^,],#%d", ptr_instruction->opcode, &ptr_instruction->literal);
                    printf("\t%s %d", ptr_instruction->opcode, ptr_instruction->literal);
                }
                else if (!(strcmp(ptr_instruction->opcode, "JUMP"))){
                    sscanf(line, "%[^,],R%d,#%d", ptr_instruction->opcode, &ptr_instruction->src1, &ptr_instruction->literal);
                    printf("\t%s R%d %d", ptr_instruction->opcode, ptr_instruction->src1, ptr_instruction->literal);
                }
                else if (!(strcmp(ptr_instruction->opcode, "HALT"))){
                    sscanf(line, "%[^,]", ptr_instruction->opcode);
                    printf("\t%s", ptr_instruction->opcode);
                }
                else if (!(strcmp(ptr_instruction->opcode, "DIV"))) {
                    sscanf(line, "%[^,],R%d,R%d,R%d", ptr_instruction->opcode, &ptr_instruction->dest, &ptr_instruction->src1, &ptr_instruction->src2);
                    printf("\t%s R%d R%d R%d", ptr_instruction->opcode, ptr_instruction->dest, ptr_instruction->src1, ptr_instruction->src2);
                }
                else if (!(strcmp(ptr_instruction->opcode, "JAL"))){
                    sscanf(line, "%[^,],R%d,R%d,#%d", ptr_instruction->opcode, &ptr_instruction->dest, &ptr_instruction->src1, &ptr_instruction->literal);
                    printf("\t%s R%d R%d %d", ptr_instruction->opcode, ptr_instruction->dest, ptr_instruction->src1, ptr_instruction->literal);
                }
                else{
                    printf("\t%s", "Not a valid opcode");
                }
                ptr_instruction++;

    }
    fclose(ptr_File);

    for (int i = 1; i <= cycles; i++){
        printf("\n--------Cycle No. = %d---------", i);
        writeback();
        memory();
        if (dflag == 1 || hflag == 1){
            if (d2flag == 1 || hflag == 1)
                div4();
            if (d1flag == 1 || hflag == 1)
                div3();
            div2();
        }
        if (mflag == 1)
            mul2();
        execute();
        decode();
        fetch();
        if (hflag == 100)
            break;
    }
}

void fetch(){
    if((pc <= line_Number) && hflag == 0){
        if_input = instruction[pc];
        if (!(strcmp(if_input.opcode, "MOVC"))){
            if(!(strcmp(idrf_input.opcode, "nop"))){
                if (bflag == 0){
                    printf("\n Fetch stage : \t\t %s R%d %d", if_input.opcode, if_input.dest, if_input.literal);
                    idrf_input = if_input;
                    if_input = nop;
                    pc++;
                }
                else{
                    bflag = 0;
                    printf("\n Fetch stage : \t\t idle");
                }
            }
            else{
                printf("\n Fetch stage : \t\t %s R%d %d stalled", if_input.opcode, if_input.dest, if_input.literal);
            }
        }
        else if (!(strcmp(if_input.opcode, "STORE"))){
            if(!(strcmp(idrf_input.opcode, "nop"))){
                if (bflag == 0){
                    printf("\n Fetch stage : \t\t %s R%d R%d %d", if_input.opcode, if_input.src1, if_input.src2, if_input.literal);
                    idrf_input = if_input;
                    if_input = nop;
                    pc++;
                }
                else{
                    bflag = 0;
                    printf("\n Fetch stage : \t\t idle");
                }
            }
            else{
                printf("\n Fetch stage : \t\t %s R%d R%d %d stalled", if_input.opcode, if_input.src1, if_input.src2, if_input.literal);
            }
        }
        else if (!(strcmp(if_input.opcode, "LOAD"))){
            if(!(strcmp(idrf_input.opcode, "nop"))){
                if (bflag == 0){
                    printf("\n Fetch stage : \t\t %s R%d R%d %d", if_input.opcode, if_input.dest, if_input.src1, if_input.literal);
                    idrf_input = if_input;
                    if_input = nop;
                    pc++;
                }
                else{
                    bflag = 0;
                    printf("\n Fetch stage : \t\t idle");
                }
            }
            else{
                printf("\n Fetch stage : \t\t %s R%d R%d %d stalled", if_input.opcode, if_input.dest, if_input.src1, if_input.literal);
            }
        }
        else if (!(strcmp(if_input.opcode, "ADD"))){
            if(!(strcmp(idrf_input.opcode, "nop"))){
                if (bflag == 0){
                    printf("\n Fetch stage : \t\t %s R%d R%d R%d", if_input.opcode, if_input.dest, if_input.src1, if_input.src2);
                    idrf_input = if_input;
                    if_input = nop;
                    pc++;
                }
                else{
                    bflag = 0;
                    printf("\n Fetch stage : \t\t idle");
                }
            }
            else{
                printf("\n Fetch stage : \t\t %s R%d R%d R%d stalled", if_input.opcode, if_input.dest, if_input.src1, if_input.src2);
            }
        }
        else if (!(strcmp(if_input.opcode, "SUB"))){
            if(!(strcmp(idrf_input.opcode, "nop"))){
                if (bflag == 0){
                    printf("\n Fetch stage : \t\t %s R%d R%d R%d", if_input.opcode, if_input.dest, if_input.src1, if_input.src2);
                    idrf_input = if_input;
                    if_input = nop;
                    pc++;
                }
                else{
                    bflag = 0;
                    printf("\n Fetch stage : \t\t idle");
                }
            }
            else{
                printf("\n Fetch stage : \t\t %s R%d R%d R%d stalled", if_input.opcode, if_input.dest, if_input.src1, if_input.src2);
            }
        }
        else if (!(strcmp(if_input.opcode, "MUL"))){
            if(!(strcmp(idrf_input.opcode, "nop"))){
                if (bflag == 0){
                    printf("\n Fetch stage : \t\t %s R%d R%d R%d", if_input.opcode, if_input.dest, if_input.src1, if_input.src2);
                    idrf_input = if_input;
                    if_input = nop;
                    pc++;
                }
                else{
                    bflag = 0;
                    printf("\n Fetch stage : \t\t idle");
                }
            }
            else{
                printf("\n Fetch stage : \t\t %s R%d R%d R%d stalled", if_input.opcode, if_input.dest, if_input.src1, if_input.src2);
            }
        }
        else if (!(strcmp(if_input.opcode, "DIV"))){
            if(!(strcmp(idrf_input.opcode, "nop"))){
                if (bflag == 0){
                    printf("\n Fetch stage : \t\t %s R%d R%d R%d", if_input.opcode, if_input.dest, if_input.src1, if_input.src2);
                    idrf_input = if_input;
                    if_input = nop;
                    pc++;
                }
                else{
                    bflag = 0;
                    printf("\n Fetch stage : \t\t idle");
                }
            }
            else{
                printf("\n Fetch stage : \t\t %s R%d R%d R%d stalled", if_input.opcode, if_input.dest, if_input.src1, if_input.src2);
            }
        }
        else if (!(strcmp(if_input.opcode, "AND"))){
            if(!(strcmp(idrf_input.opcode, "nop"))){
                if (bflag == 0){
                    printf("\n Fetch stage : \t\t %s R%d R%d R%d", if_input.opcode, if_input.dest, if_input.src1, if_input.src2);
                    idrf_input = if_input;
                    if_input = nop;
                    pc++;
                }
                else{
                    bflag = 0;
                    printf("\n Fetch stage : \t\t idle");
                }
            }
            else{
                printf("\n Fetch stage : \t\t %s R%d R%d R%d stalled", if_input.opcode, if_input.src1, if_input.src2, if_input.src2);
            }
        }
        else if (!(strcmp(if_input.opcode, "OR"))){
            if(!(strcmp(idrf_input.opcode, "nop"))){
                if (bflag == 0){
                    printf("\n Fetch stage : \t\t %s R%d R%d R%d", if_input.opcode, if_input.dest, if_input.src1, if_input.src2);
                    idrf_input = if_input;
                    if_input = nop;
                    pc++;
                }
                else{
                    bflag = 0;
                    printf("\n Fetch stage : \t\t idle");
                }
            }
            else{
                printf("\n Fetch stage : \t\t %s R%d R%d R%d stalled", if_input.opcode, if_input.dest, if_input.src1, if_input.src2);
            }
        }
        else if (!(strcmp(if_input.opcode, "EX-OR"))){
            if(!(strcmp(idrf_input.opcode, "nop"))){
                if (bflag == 0){
                    printf("\n Fetch stage : \t\t %s R%d R%d R%d", if_input.opcode, if_input.dest, if_input.src1, if_input.src2);
                    idrf_input = if_input;
                    if_input = nop;
                    pc++;
                }
                else{
                    bflag = 0;
                    printf("\n Fetch stage : \t\t idle");
                }
            }
            else{
                printf("\n Fetch stage : \t\t %s R%d R%d R%d stalled", if_input.opcode, if_input.dest, if_input.src1, if_input.src2);
            }
        }
        else if (!(strcmp(if_input.opcode, "BNZ"))){
            if(!(strcmp(idrf_input.opcode, "nop"))){
                if (bflag == 0){
                    printf("\n Fetch stage : \t\t %s %d", if_input.opcode, if_input.literal);
                    if_input.index = (pc);
                    idrf_input = if_input;
                    if_input = nop;
                    pc++;
                }
                else{
                    bflag = 0;
                    printf("\n Fetch stage : \t\t idle");
                }
            }
            else{
                printf("\n Fetch stage : \t\t %s %d stalled", if_input.opcode, if_input.literal);
            }
        }
        else if (!(strcmp(if_input.opcode, "BZ"))){
            if(!(strcmp(idrf_input.opcode, "nop"))){
                if (bflag == 0){
                    printf("\n Fetch stage : \t\t %s %d", if_input.opcode, if_input.literal);
                    if_input.index = (pc);
                    idrf_input = if_input;
                    if_input = nop;
                    pc++;
                }
                else{
                    bflag = 0;
                    printf("\n Fetch stage : \t\t idle");
                }
            }
            else{
                printf("\n Fetch stage : \t\t %s %d stalled", if_input.opcode, if_input.literal);
            }
        }
        else if (!(strcmp(if_input.opcode, "HALT"))){
            if(!(strcmp(idrf_input.opcode, "nop"))){
                if (bflag == 0){
                    printf("\n Fetch stage : \t\t %s", if_input.opcode);
                    idrf_input = if_input;
                    if_input = nop;
                    pc++;
                }
                else{
                    bflag = 0;
                    printf("\n Fetch stage : \t\t idle");
                }
            }
            else{
                printf("\n Fetch stage : \t\t %s stalled", if_input.opcode);
            }
        }
        else if (!(strcmp(if_input.opcode, "JUMP"))){
            if(!(strcmp(idrf_input.opcode, "nop"))){
                if (bflag == 0){
                    printf("\n Fetch stage : \t\t %s R%d %d", if_input.opcode, if_input.src1, if_input.literal);
                    idrf_input = if_input;
                    if_input = nop;
                    pc++;
                }
                else{
                    bflag = 0;
                    printf("\n Fetch stage : \t\t idle");
                }
            }
            else{
                printf("\n Fetch stage : \t\t %s R%d %d stalled", if_input.opcode, if_input.src1, if_input.literal);
            }
        }
        else if (!(strcmp(if_input.opcode, "JAL"))){
            if(!(strcmp(idrf_input.opcode, "nop"))){
                if (bflag == 0){
                    printf("\n Fetch stage : \t\t %s R%d R%d %d", if_input.opcode, if_input.dest, if_input.src1, if_input.literal);
                    pc++;
                    if_input.index = (pc*4 + 4000);
                    idrf_input = if_input;
                    if_input = nop;

                }
                else{
                    bflag = 0;
                    printf("\n Fetch stage : \t\t idle");
                }
            }
            else{
                printf("\n Fetch stage : \t\t %s R%d R%d %d stalled", if_input.opcode, if_input.dest, if_input.src1, if_input.literal);
            }
        }
        // else if
    }
    else
        printf("\n Fetch stage : \t\t idle");
}

void decode(){
    //printf("\n\t\t ind : %d", ind);
    if((strcmp(idrf_input.opcode, "nop"))){
        if (!(strcmp(idrf_input.opcode, "MOVC"))){
                if(!(strcmp(iex_input.opcode, "nop")) && register_File[idrf_input.dest].status == VALID){
                    printf("\n Decode stage : \t %s R%d %d", idrf_input.opcode, idrf_input.dest, idrf_input.literal);
                    register_File[idrf_input.dest].status = INVALID;
                    idrf_input.id = id;
                    register_File[idrf_input.dest].ins_id = idrf_input.id;
                    //printf("\n reg : %d,  ins : %d, id : %d", register_File[idrf_input.dest].ins_id, idrf_input.id, id );
                    id++;
                    iex_input = idrf_input;
                    idrf_input = nop;
                }
                else
                    printf("\n Decode stage : \t %s R%d %d stalled", idrf_input.opcode, idrf_input.dest, idrf_input.literal);
        }
        else if (!(strcmp(idrf_input.opcode, "STORE"))){
                if((!(strcmp(iex_input.opcode, "nop"))) && register_File[idrf_input.src1].status == VALID && register_File[idrf_input.src2].status == VALID){
                    printf("\n Decode stage : \t %s R%d R%d %d", idrf_input.opcode, idrf_input.src1, idrf_input.src2, idrf_input.literal);
                    iex_input = idrf_input;
                    idrf_input = nop;
                }
                else
                    printf("\n Decode stage : \t %s R%d R%d %d stalled", idrf_input.opcode, idrf_input.src1, idrf_input.src2, idrf_input.literal);
        }
        else if (!(strcmp(idrf_input.opcode, "LOAD"))){
                if((!(strcmp(iex_input.opcode, "nop"))) && register_File[idrf_input.src1].status == VALID && register_File[idrf_input.dest].status == VALID){
                    register_File[idrf_input.dest].status = INVALID;
                    printf("\n Decode stage : \t %s R%d R%d %d", idrf_input.opcode, idrf_input.dest, idrf_input.src1, idrf_input.literal);
                    idrf_input.id = id;
                    register_File[idrf_input.dest].ins_id = idrf_input.id;
                    id++;
                    iex_input = idrf_input;
                    idrf_input = nop;
                }
                else
                    printf("\n Decode stage : \t %s R%d R%d %d stalled", idrf_input.opcode, idrf_input.dest, idrf_input.src1, idrf_input.literal);
        }
        else if (!(strcmp(idrf_input.opcode, "ADD"))){
                if((!(strcmp(iex_input.opcode, "nop"))) && register_File[idrf_input.src1].status == VALID && register_File[idrf_input.src2].status == VALID && register_File[idrf_input.dest].status == VALID){
                    register_File[idrf_input.dest].status = INVALID;
                    printf("\n Decode stage : \t %s R%d R%d R%d", idrf_input.opcode, idrf_input.dest, idrf_input.src1, idrf_input.src2);
                    idrf_input.index = ind;
                    ind++;
                    idrf_input.id = id;
                    register_File[idrf_input.dest].ins_id = idrf_input.id;
                    id++;
                    iex_input = idrf_input;
                    idrf_input = nop;
                }
                else
                    printf("\n Decode stage : \t %s R%d R%d R%d stalled", idrf_input.opcode, idrf_input.dest, idrf_input.src1, idrf_input.src2);
        }
        else if (!(strcmp(idrf_input.opcode, "SUB"))){
                if((!(strcmp(iex_input.opcode, "nop"))) && register_File[idrf_input.src1].status == VALID && register_File[idrf_input.src2].status == VALID && register_File[idrf_input.dest].status == VALID){
                    register_File[idrf_input.dest].status = INVALID;
                    printf("\n Decode stage : \t %s R%d R%d R%d", idrf_input.opcode, idrf_input.dest, idrf_input.src1, idrf_input.src2);
                    idrf_input.index = ind;
                    ind++;
                    idrf_input.id = id;
                    register_File[idrf_input.dest].ins_id = idrf_input.id;
                    id++;
                    iex_input = idrf_input;
                    idrf_input = nop;
                }
                else
                    printf("\n Decode stage : \t %s R%d R%d R%d stalled", idrf_input.opcode, idrf_input.dest, idrf_input.src1, idrf_input.src2);
        }
        else if (!(strcmp(idrf_input.opcode, "MUL"))){
                if((!(strcmp(iex_input.opcode, "nop"))) && register_File[idrf_input.src1].status == VALID && register_File[idrf_input.src2].status == VALID && register_File[idrf_input.dest].status == VALID){
                    register_File[idrf_input.dest].status = INVALID;
                    printf("\n Decode stage : \t %s R%d R%d R%d", idrf_input.opcode, idrf_input.dest, idrf_input.src1, idrf_input.src2);
                    idrf_input.index = ind;
                    ind++;
                    idrf_input.id = id;
                    register_File[idrf_input.dest].ins_id = idrf_input.id;
                    id++;
                    iex_input = idrf_input;
                    idrf_input = nop;
                }
                else
                    printf("\n Decode stage : \t %s R%d R%d R%d stalled", idrf_input.opcode, idrf_input.dest, idrf_input.src1, idrf_input.src2);
        }
        else if (!(strcmp(idrf_input.opcode, "DIV"))){
                if((!(strcmp(iex_input.opcode, "nop"))) && register_File[idrf_input.src1].status == VALID && register_File[idrf_input.src2].status == VALID && register_File[idrf_input.dest].status == VALID){
                    register_File[idrf_input.dest].status = INVALID;
                    printf("\n Decode stage : \t %s R%d R%d R%d", idrf_input.opcode, idrf_input.dest, idrf_input.src1, idrf_input.src2);
                    idrf_input.index = ind;
                    ind++;
                    idrf_input.id = id;
                    register_File[idrf_input.dest].ins_id = idrf_input.id;
                    id++;
                    iex_input = idrf_input;
                    idrf_input = nop;
                }
                else
                    printf("\n Decode stage : \t %s R%d R%d R%d stalled", idrf_input.opcode, idrf_input.dest, idrf_input.src1, idrf_input.src2);
        }
        else if (!(strcmp(idrf_input.opcode, "AND"))){
                if((!(strcmp(iex_input.opcode, "nop"))) && register_File[idrf_input.src1].status == VALID && register_File[idrf_input.src2].status == VALID && register_File[idrf_input.dest].status == VALID){
                    register_File[idrf_input.dest].status = INVALID;
                    printf("\n Decode stage : \t %s R%d R%d R%d", idrf_input.opcode, idrf_input.dest, idrf_input.src1, idrf_input.src2);
                    idrf_input.index = ind;
                    ind++;
                    idrf_input.id = id;
                    register_File[idrf_input.dest].ins_id = idrf_input.id;
                    id++;
                    iex_input = idrf_input;
                    idrf_input = nop;
                }
                else
                    printf("\n Decode stage : \t %s R%d R%d R%d stalled", idrf_input.opcode, idrf_input.dest, idrf_input.src1, idrf_input.src2);
        }
        else if (!(strcmp(idrf_input.opcode, "OR"))){
                if((!(strcmp(iex_input.opcode, "nop"))) && register_File[idrf_input.src1].status == VALID && register_File[idrf_input.src2].status == VALID && register_File[idrf_input.dest].status == VALID){
                    register_File[idrf_input.dest].status = INVALID;
                    printf("\n Decode stage : \t %s R%d R%d R%d", idrf_input.opcode, idrf_input.dest, idrf_input.src1, idrf_input.src2);
                    idrf_input.index = ind;
                    ind++;
                    idrf_input.id = id;
                    register_File[idrf_input.dest].ins_id = idrf_input.id;
                    id++;
                    iex_input = idrf_input;
                    idrf_input = nop;
                }
                else
                    printf("\n Decode stage : \t %s R%d R%d R%d stalled", idrf_input.opcode, idrf_input.dest, idrf_input.src1, idrf_input.src2);
        }
        else if (!(strcmp(idrf_input.opcode, "EX-OR"))){
                if((!(strcmp(iex_input.opcode, "nop"))) && register_File[idrf_input.src1].status == VALID && register_File[idrf_input.src2].status == VALID && register_File[idrf_input.dest].status == VALID){
                    register_File[idrf_input.dest].status = INVALID;
                    printf("\n Decode stage : \t %s R%d R%d R%d", idrf_input.opcode, idrf_input.dest, idrf_input.src1, idrf_input.src2);
                    idrf_input.index = ind;
                    ind++;
                    idrf_input.id = id;
                    register_File[idrf_input.dest].ins_id = idrf_input.id;
                    id++;
                    iex_input = idrf_input;
                    idrf_input = nop;
                }
                else
                    printf("\n Decode stage : \t %s R%d R%d R%d stalled", idrf_input.opcode, idrf_input.dest, idrf_input.src1, idrf_input.src2);
        }
        else if (!(strcmp(idrf_input.opcode, "BNZ"))){
                if((!(strcmp(iex_input.opcode, "nop"))) && last_arithmetic_result != 0 && last_arithmetic_ind == (ind - 1)){
                    printf("\n Decode stage : \t %s %d", idrf_input.opcode, idrf_input.literal);

                    //set to one if taken branch
                    idrf_input.branch = 1;
                    iex_input = idrf_input;
                    idrf_input = nop;
                }
                else if((!(strcmp(iex_input.opcode, "nop"))) && last_arithmetic_result == 0 && last_arithmetic_ind == (ind - 1)){
                    printf("\n Decode stage : \t %s %d", idrf_input.opcode, idrf_input.literal);
                    idrf_input.branch = 0;
                    iex_input = idrf_input;
                    idrf_input = nop;
                }
                else
                    printf("\n Decode stage : \t %s %d stalled", idrf_input.opcode, idrf_input.literal);
        }
        else if (!(strcmp(idrf_input.opcode, "BZ"))){
                if((!(strcmp(iex_input.opcode, "nop"))) && last_arithmetic_result == 0 && last_arithmetic_ind == (ind - 1)){
                    printf("\n Decode stage : \t %s %d", idrf_input.opcode, idrf_input.literal);

                    //set to one if taken branch
                    idrf_input.branch = 1;
                    iex_input = idrf_input;
                    idrf_input = nop;
                }
                else if((!(strcmp(iex_input.opcode, "nop"))) && last_arithmetic_result != 0 && last_arithmetic_ind == (ind - 1)){
                    printf("\n Decode stage : \t %s %d", idrf_input.opcode, idrf_input.literal);
                    idrf_input.branch = 0;
                    iex_input = idrf_input;
                    idrf_input = nop;
                }
                else
                    printf("\n Decode stage : \t %s %d stalled", idrf_input.opcode, idrf_input.literal);
        }
        else if (!(strcmp(idrf_input.opcode, "HALT"))){
            if (!(strcmp(iex_input.opcode, "nop"))){
                printf("\n Decode stage : \t %s ", idrf_input.opcode);
                if_input = nop;
                hflag = 1;   // HALT flag
                iex_input = idrf_input;
                idrf_input = nop;
            }
            else
                printf("\n Decode stage : \t %s stalled", idrf_input.opcode);
        }
        else if (!(strcmp(idrf_input.opcode, "JUMP"))){
                if(!(strcmp(iex_input.opcode, "nop"))) {
                    printf("\n Decode stage : \t %s R%d %d", idrf_input.opcode, idrf_input.src1, idrf_input.literal);
                    iex_input = idrf_input;
                    if_input = nop;
                    bflag = 1;
                    idrf_input = nop;
                }
                else
                    printf("\n Decode stage : \t %s R%d %d", idrf_input.opcode, idrf_input.src1, idrf_input.literal);
        }
        else if (!(strcmp(idrf_input.opcode, "JAL"))){
                if(!(strcmp(iex_input.opcode, "nop"))) {
                    register_File[idrf_input.dest].status = INVALID;
                    printf("\n Decode stage : \t %s R%d R%d %d", idrf_input.opcode, idrf_input.dest, idrf_input.src1, idrf_input.literal);
                    iex_input = idrf_input;
                    if_input = nop;
                    bflag = 1;
                    idrf_input = nop;
                }
                else
                    printf("\n Decode stage : \t %s R%d R%d %d", idrf_input.opcode, idrf_input.dest, idrf_input.src1, idrf_input.literal);
        }
        //else if
    }
    else
        printf("\n Decode stage : \t idle");
}

void execute(){
    if((strcmp(iex_input.opcode, "nop"))){
        if (!(strcmp(iex_input.opcode, "MOVC"))){
            iex_input.result = iex_input.literal;
            if (iex_input.id == register_File[iex_input.dest].ins_id)
                register_File[iex_input.dest].status = VALID;
            register_File[iex_input.dest].value = iex_input.result;

            if(!(strcmp(imem_input.opcode, "nop"))){
                printf("\n Execution stage : \t %s R%d %d", iex_input.opcode, iex_input.dest, iex_input.literal);
            //printf("\n reg : %d,  ins : %d, id : %d", register_File[iex_input.dest].ins_id, iex_input.id, id );
                imem_input = iex_input;
                iex_input = nop;
            }
            else
                printf("\n Execution stage : \t %s R%d %d stalled", iex_input.opcode, iex_input.dest, iex_input.literal);
        }
        else if (!(strcmp(iex_input.opcode, "STORE"))){
            iex_input.address = register_File[iex_input.src2].value + iex_input.literal;
            if(!(strcmp(imem_input.opcode, "nop"))){
                printf("\n Execution stage : \t %s R%d R%d %d", iex_input.opcode, iex_input.src1, iex_input.src2, iex_input.literal);
                imem_input = iex_input;
                iex_input = nop;
            }
            else
                printf("\n Execution stage : \t %s R%d R%d %d stalled", iex_input.opcode, iex_input.src1, iex_input.src2, iex_input.literal);
        }
        else if (!(strcmp(iex_input.opcode, "LOAD"))){
            iex_input.address = register_File[iex_input.src1].value + iex_input.literal;
            if(!(strcmp(imem_input.opcode, "nop"))){
                printf("\n Execution stage : \t %s R%d R%d %d", iex_input.opcode, iex_input.dest, iex_input.src1, iex_input.literal);
                imem_input = iex_input;
                iex_input = nop;
            }
            else
                printf("\n Execution stage : \t %s R%d R%d %d stalled", iex_input.opcode, iex_input.dest, iex_input.src1, iex_input.literal);
        }
        else if (!(strcmp(iex_input.opcode, "ADD"))){
            iex_input.result = register_File[iex_input.src1].value + register_File[iex_input.src2].value;
            if (iex_input.id == register_File[iex_input.dest].ins_id)
                register_File[iex_input.dest].status = VALID;
                register_File[iex_input.dest].value = iex_input.result;
                last_arithmetic_ind = iex_input.index;
                last_arithmetic_result = iex_input.result;

            if(!(strcmp(imem_input.opcode, "nop"))){
                printf("\n Execution stage : \t %s R%d R%d R%d", iex_input.opcode, iex_input.dest, iex_input.src1, iex_input.src2);
                imem_input = iex_input;
                iex_input = nop;
            }
            else
                printf("\n Execution stage : \t %s R%d R%d R%d stalled", iex_input.opcode, iex_input.dest, iex_input.src1, iex_input.src2);
        }
        else if (!(strcmp(iex_input.opcode, "SUB"))){
            iex_input.result = register_File[iex_input.src1].value - register_File[iex_input.src2].value;
            if (iex_input.id == register_File[iex_input.dest].ins_id)
                register_File[iex_input.dest].status = VALID;
                register_File[iex_input.dest].value = iex_input.result;
                last_arithmetic_ind = iex_input.index;
                last_arithmetic_result = iex_input.result;

            if(!(strcmp(imem_input.opcode, "nop"))){
                printf("\n Execution stage : \t %s R%d R%d R%d", iex_input.opcode, iex_input.dest, iex_input.src1, iex_input.src2);
                imem_input = iex_input;
                iex_input = nop;
            }
            else
                printf("\n Execution stage : \t %s R%d R%d R%d stalled", iex_input.opcode, iex_input.dest, iex_input.src1, iex_input.src2);
        }
        else if (!(strcmp(iex_input.opcode, "MUL"))){
            if(!(strcmp(imul2_input.opcode, "nop"))){
                printf("\n Mul1 stage : \t\t %s R%d R%d R%d", iex_input.opcode, iex_input.dest, iex_input.src1, iex_input.src2);
                imul2_input = iex_input;
                iex_input = nop;
                mflag = 1;
            }
            else
                printf("\n Mul1 stage : \t\t %s R%d R%d R%d stalled", iex_input.opcode, iex_input.dest, iex_input.src1, iex_input.src2);
        }
        else if (!(strcmp(iex_input.opcode, "DIV"))){
            if(!(strcmp(idiv2_input.opcode, "nop"))){
                printf("\n DU1 stage : \t\t %s R%d R%d R%d", iex_input.opcode, iex_input.dest, iex_input.src1, iex_input.src2);
                idiv2_input = iex_input;
                iex_input = nop;
                dflag = 1;
            }
            else
                printf("\n DU1 stage : \t\t %s R%d R%d R%d stalled", iex_input.opcode, iex_input.dest, iex_input.src1, iex_input.src2);
        }
        else if (!(strcmp(iex_input.opcode, "AND"))){
            iex_input.result = register_File[iex_input.src1].value & register_File[iex_input.src2].value;
            if (iex_input.id == register_File[iex_input.dest].ins_id)
                register_File[iex_input.dest].status = VALID;
                register_File[iex_input.dest].value = iex_input.result;
                last_arithmetic_ind = iex_input.index;
                last_arithmetic_result = iex_input.result;

            if(!(strcmp(imem_input.opcode, "nop"))){
                printf("\n Execution stage : \t %s R%d R%d R%d", iex_input.opcode, iex_input.dest, iex_input.src1, iex_input.src2);
                imem_input = iex_input;
                iex_input = nop;
            }
            else
                printf("\n Execution stage : \t %s R%d R%d R%d stalled", iex_input.opcode, iex_input.dest, iex_input.src1, iex_input.src2);
        }
        else if (!(strcmp(iex_input.opcode, "OR"))){
            iex_input.result = register_File[iex_input.src1].value | register_File[iex_input.src2].value;
            if (iex_input.id == register_File[iex_input.dest].ins_id)
                register_File[iex_input.dest].status = VALID;
                register_File[iex_input.dest].value = iex_input.result;
                last_arithmetic_ind = iex_input.index;
                last_arithmetic_result = iex_input.result;

            if(!(strcmp(imem_input.opcode, "nop"))){
                printf("\n Execution stage : \t %s R%d R%d R%d", iex_input.opcode, iex_input.dest, iex_input.src1, iex_input.src2);
                imem_input = iex_input;
                iex_input = nop;
            }
            else
                printf("\n Execution stage : \t %s R%d R%d R%d stalled", iex_input.opcode, iex_input.dest, iex_input.src1, iex_input.src2);
        }
        else if (!(strcmp(iex_input.opcode, "EX-OR"))){
            iex_input.result = register_File[iex_input.src1].value ^ register_File[iex_input.src2].value;
            if (iex_input.id == register_File[iex_input.dest].ins_id)
                register_File[iex_input.dest].status = VALID;
                register_File[iex_input.dest].value = iex_input.result;
                last_arithmetic_ind = iex_input.index;
                last_arithmetic_result = iex_input.result;

            if(!(strcmp(imem_input.opcode, "nop"))){
                printf("\n Execution stage : \t %s R%d R%d R%d", iex_input.opcode, iex_input.dest, iex_input.src1, iex_input.src2);
                imem_input = iex_input;
                iex_input = nop;
            }
            else
                printf("\n Execution stage : \t %s R%d R%d R%d stalled", iex_input.opcode, iex_input.dest, iex_input.src1, iex_input.src2);
        }
        else if (!(strcmp(iex_input.opcode, "BNZ"))){
            if(!(strcmp(imem_input.opcode, "nop"))){
                printf("\n Execution stage : \t %s %d", iex_input.opcode, iex_input.literal);
                iex_input.result = (iex_input.index + (iex_input.literal/4));
                imem_input = iex_input;
                iex_input = nop;
            }
            else
                printf("\n Execution stage : \t %s R%d R%d R%d stalled", iex_input.opcode, iex_input.dest, iex_input.src1, iex_input.src2);
        }
        else if (!(strcmp(iex_input.opcode, "BZ"))){
            if(!(strcmp(imem_input.opcode, "nop"))){
                printf("\n Execution stage : \t %s %d", iex_input.opcode, iex_input.literal);
                iex_input.result = (iex_input.index + (iex_input.literal/4));
                imem_input = iex_input;
                iex_input = nop;
            }
            else
                printf("\n Execution stage : \t %s R%d R%d R%d stalled", iex_input.opcode, iex_input.dest, iex_input.src1, iex_input.src2);
        }
        else if (!(strcmp(iex_input.opcode, "HALT"))){
            if(!(strcmp(imem_input.opcode, "nop"))){
                printf("\n Execution stage : \t %s ", iex_input.opcode);
                idiv2_input = iex_input;
                iex_input = nop;
                hflag = 1;
            }
            else
                printf("\n Execution stage : \t %s ", iex_input.opcode);
        }
        else if (!(strcmp(iex_input.opcode, "JUMP"))){
            if(!(strcmp(imem_input.opcode, "nop"))){
                printf("\n Execution stage : \t %s R%d %d", iex_input.opcode, iex_input.src1, iex_input.literal);
                iex_input.result = (register_File[iex_input.src1].value + iex_input.literal - 4000 )/4;
                imem_input = iex_input;
                bflag = 1;
                iex_input = nop;
            }
            else
                printf("\n Execution stage : \t %s R%d %d stalled", iex_input.opcode, iex_input.src1, iex_input.literal);
        }
        else if (!(strcmp(iex_input.opcode, "JAL"))){
            if(!(strcmp(imem_input.opcode, "nop"))){
                printf("\n Execution stage : \t %s R%d R%d %d", iex_input.opcode, iex_input.dest, iex_input.src1, iex_input.literal);
                iex_input.result = (register_File[iex_input.src1].value + iex_input.literal - 4000)/4;
                imem_input = iex_input;
                bflag = 1;
                iex_input = nop;
            }
            else
                printf("\n Execution stage : \t %s R%d R%d %d", iex_input.opcode, iex_input.dest, iex_input.src1, iex_input.literal);
        }
        //else if // for other instr
    }
    else
        printf("\n Execution stage : \t idle");
}

void memory(){
    if((strcmp(imem_input.opcode, "nop"))){
        if (!(strcmp(imem_input.opcode, "MOVC"))){
            if (imem_input.id == register_File[imem_input.dest].ins_id)
                register_File[imem_input.dest].status = INVALID;
            if(!(strcmp(iwb_input.opcode, "nop"))){
                printf("\n Memory stage : \t %s R%d %d", imem_input.opcode, imem_input.dest, imem_input.literal);
                //printf("\n reg : %d,  ins : %d, id : %d", register_File[imem_input.dest].ins_id, imem_input.id, id );
                iwb_input = imem_input;
                imem_input = nop;
            }
            else
                printf("\n Memory stage : \t %s R%d %d stalled", imem_input.opcode, imem_input.dest, imem_input.literal);
        }
        else if (!(strcmp(imem_input.opcode, "STORE"))){
            if(!(strcmp(iwb_input.opcode, "nop"))){
                printf("\n Memory stage : \t %s R%d R%d %d", imem_input.opcode, imem_input.src1, imem_input.src2, imem_input.literal);
                data_Memory[imem_input.address/4] = register_File[imem_input.src1].value;
                iwb_input = imem_input;
                //printf("\n\t\t in data memory : %ld", data_Memory[imem_input.address/4]);
                imem_input = nop;
            }
            else
                printf("\n Memory stage : \t %s R%d R%d %d stalled", imem_input.opcode, imem_input.src1, imem_input.src2, imem_input.literal);
        }
        else if (!(strcmp(imem_input.opcode, "LOAD"))){
            if (imem_input.id == register_File[imem_input.dest].ins_id)
                register_File[imem_input.dest].status = INVALID;
            if(!(strcmp(iwb_input.opcode, "nop"))){
                printf("\n Memory stage : \t %s R%d R%d %d", imem_input.opcode, imem_input.dest, imem_input.src1, imem_input.literal);
                iwb_input = imem_input;
                imem_input = nop;
            }
            else
                printf("\n Memory stage : \t %s R%d R%d %d stalled", imem_input.opcode, imem_input.dest, imem_input.src1, imem_input.literal);
        }
        else if (!(strcmp(imem_input.opcode, "ADD"))){
            if (imem_input.id == register_File[imem_input.dest].ins_id)
                    register_File[imem_input.dest].status = INVALID;
            if(!(strcmp(iwb_input.opcode, "nop"))){
                printf("\n Memory stage : \t %s R%d R%d R%d", imem_input.opcode, imem_input.dest, imem_input.src1, imem_input.src2);
                iwb_input = imem_input;
                imem_input = nop;
            }
            else
                printf("\n Memory stage : \t %s R%d R%d R%d stalled", imem_input.opcode, imem_input.dest, imem_input.src1, imem_input.src2);
        }
        else if (!(strcmp(imem_input.opcode, "AND"))){
            if (imem_input.id == register_File[imem_input.dest].ins_id)
                    register_File[imem_input.dest].status = INVALID;
            if(!(strcmp(iwb_input.opcode, "nop"))){
                printf("\n Memory stage : \t %s R%d R%d R%d", imem_input.opcode, imem_input.dest, imem_input.src1, imem_input.src2);
                iwb_input = imem_input;
                imem_input = nop;
            }
            else
                printf("\n Memory stage : \t %s R%d R%d R%d stalled", imem_input.opcode, imem_input.dest, imem_input.src1, imem_input.src2);
        }
        else if (!(strcmp(imem_input.opcode, "SUB"))){
            if (imem_input.id == register_File[imem_input.dest].ins_id)
                    register_File[imem_input.dest].status = INVALID;
            if(!(strcmp(iwb_input.opcode, "nop"))){
                printf("\n Memory stage : \t %s R%d R%d R%d", imem_input.opcode, imem_input.dest, imem_input.src1, imem_input.src2);
                iwb_input = imem_input;
                imem_input = nop;
            }
            else
                printf("\n Memory stage : \t %s R%d R%d R%d stalled", imem_input.opcode, imem_input.dest, imem_input.src1, imem_input.src2);
        }
        else if (!(strcmp(imem_input.opcode, "MUL"))){
            if (imem_input.id == register_File[imem_input.dest].ins_id)
                    register_File[imem_input.dest].status = INVALID;
            if(!(strcmp(iwb_input.opcode, "nop"))){
                printf("\n Memory stage : \t %s R%d R%d R%d", imem_input.opcode, imem_input.dest, imem_input.src1, imem_input.src2);
                iwb_input = imem_input;
                imem_input = nop;
            }
            else
                printf("\n Memory stage : \t %s R%d R%d R%d stalled", imem_input.opcode, imem_input.dest, imem_input.src1, imem_input.src2);
        }
        else if (!(strcmp(imem_input.opcode, "DIV"))){
            if (imem_input.id == register_File[imem_input.dest].ins_id)
                    register_File[imem_input.dest].status = INVALID;
            if(!(strcmp(iwb_input.opcode, "nop"))){
                printf("\n Memory stage : \t %s R%d R%d R%d", imem_input.opcode, imem_input.dest, imem_input.src1, imem_input.src2);
                iwb_input = imem_input;
                imem_input = nop;
            }
            else
                printf("\n Memory stage : \t %s R%d R%d R%d stalled", imem_input.opcode, imem_input.dest, imem_input.src1, imem_input.src2);
        }
        else if (!(strcmp(imem_input.opcode, "OR"))){
            if (imem_input.id == register_File[imem_input.dest].ins_id)
                    register_File[imem_input.dest].status = INVALID;
            if(!(strcmp(iwb_input.opcode, "nop"))){
                printf("\n Memory stage : \t %s R%d R%d R%d", imem_input.opcode, imem_input.dest, imem_input.src1, imem_input.src2);
                iwb_input = imem_input;
                imem_input = nop;
            }
            else
                printf("\n Memory stage : \t %s R%d R%d R%d stalled", imem_input.opcode, imem_input.dest, imem_input.src1, imem_input.src2);
        }
        else if (!(strcmp(imem_input.opcode, "EX-OR"))){
            if (imem_input.id == register_File[imem_input.dest].ins_id)
                    register_File[imem_input.dest].status = INVALID;
            if(!(strcmp(iwb_input.opcode, "nop"))){
                printf("\n Memory stage : \t %s R%d R%d R%d", imem_input.opcode, imem_input.dest, imem_input.src1, imem_input.src2);
                iwb_input = imem_input;
                imem_input = nop;
            }
            else
                printf("\n Memory stage : \t %s R%d R%d R%d stalled", imem_input.opcode, imem_input.dest, imem_input.src1, imem_input.src2);
        }
        else if (!(strcmp(imem_input.opcode, "BNZ"))){
            if(!(strcmp(iwb_input.opcode, "nop"))){
                printf("\n Memory stage : \t %s %d", imem_input.opcode, imem_input.literal);
                iwb_input = imem_input;
                imem_input = nop;
            }
            else
                printf("\n Memory stage : \t %s %d stalled", imem_input.opcode, imem_input.literal);
        }
        else if (!(strcmp(imem_input.opcode, "BZ"))){
            if(!(strcmp(iwb_input.opcode, "nop"))){
                printf("\n Memory stage : \t %s %d", imem_input.opcode, imem_input.literal);
                iwb_input = imem_input;
                imem_input = nop;
            }
            else
                printf("\n Memory stage : \t %s %d stalled", imem_input.opcode, imem_input.literal);
        }
        else if (!(strcmp(imem_input.opcode, "HALT"))){
            printf("\n Memory stage : \t %s ", imem_input.opcode);
            iwb_input = imem_input;
            imem_input = nop;
        }
        else if (!(strcmp(imem_input.opcode, "JUMP"))){
            if(!(strcmp(iwb_input.opcode, "nop"))){
                printf("\n Memory stage : \t %s R%d %d", imem_input.opcode, imem_input.src1, imem_input.literal);
                iwb_input = imem_input;
                bflag = 1;
                imem_input = nop;
            }
            else
                printf("\n Memory stage : \t %s R%d %d", imem_input.opcode, imem_input.src1, imem_input.literal);
        }
        else if (!(strcmp(imem_input.opcode, "JAL"))){
            if(!(strcmp(iwb_input.opcode, "nop"))){
                printf("\n Memory stage : \t %s R%d R%d %d", imem_input.opcode, imem_input.dest, imem_input.src1, imem_input.literal);
                iwb_input = imem_input;
                bflag = 1;
                imem_input = nop;
            }
            else
                printf("\n Memory stage : \t %s R%d R%d %d", imem_input.opcode, imem_input.dest, imem_input.src1, imem_input.literal);
        }
        //else if
    }
    else
        printf("\n memory stage : \t idle");
}

void writeback(){
    //printf("\n\t\t last_arithmetic_ind : %d\n", last_arithmetic_ind);
    if((strcmp(iwb_input.opcode, "nop"))){
        if (!(strcmp(iwb_input.opcode, "MOVC"))){
            printf("\n Writeback stage : \t %s R%d %d", iwb_input.opcode, iwb_input.dest, iwb_input.literal);
            register_File[iwb_input.dest].value = iwb_input.result;
            //printf("\n reg : %d,  ins : %d, id : %d", register_File[iwb_input.dest].ins_id, iwb_input.id, id );
            if (iwb_input.id == register_File[iwb_input.dest].ins_id)
                register_File[iwb_input.dest].status = VALID;
            iwb_input = nop;
        }
        else if (!(strcmp(iwb_input.opcode, "STORE"))){
            printf("\n Writeback stage : \t %s R%d R%d %d", iwb_input.opcode, iwb_input.src1, iwb_input.src2, iwb_input.literal);
            iwb_input = nop;
        }
        else if (!(strcmp(iwb_input.opcode, "LOAD"))){
            printf("\n Writeback stage : \t %s R%d R%d %d", iwb_input.opcode, iwb_input.dest, iwb_input.src1, iwb_input.literal);
            register_File[iwb_input.dest].value = data_Memory[iwb_input.address/4];
            if (iwb_input.id == register_File[iwb_input.dest].ins_id)
                register_File[iwb_input.dest].status = VALID;
            iwb_input = nop;
        }
        else if (!(strcmp(iwb_input.opcode, "ADD"))){
            printf("\n Writeback stage : \t %s R%d R%d R%d", iwb_input.opcode, iwb_input.dest, iwb_input.src1, iwb_input.src2);
            last_arithmetic_ind = iwb_input.index;
            last_arithmetic_result = iwb_input.result;

            register_File[iwb_input.dest].value = iwb_input.result;
            if (iwb_input.id == register_File[iwb_input.dest].ins_id)
                register_File[iwb_input.dest].status = VALID;
            iwb_input = nop;
        }
        else if (!(strcmp(iwb_input.opcode, "SUB"))){
            printf("\n Writeback stage : \t %s R%d R%d R%d", iwb_input.opcode, iwb_input.dest, iwb_input.src1, iwb_input.src2);
            last_arithmetic_ind = iwb_input.index;
            last_arithmetic_result = iwb_input.result;

            register_File[iwb_input.dest].value = iwb_input.result;
            if (iwb_input.id == register_File[iwb_input.dest].ins_id)
                register_File[iwb_input.dest].status = VALID;
            iwb_input = nop;
        }
        else if (!(strcmp(iwb_input.opcode, "MUL"))){
            printf("\n Writeback stage : \t %s R%d R%d R%d", iwb_input.opcode, iwb_input.dest, iwb_input.src1, iwb_input.src2);
            register_File[iwb_input.dest].value = iwb_input.result;
            if (iwb_input.id == register_File[iwb_input.dest].ins_id)
                register_File[iwb_input.dest].status = VALID;
            iwb_input = nop;
        }
        else if (!(strcmp(iwb_input.opcode, "DIV"))){
            printf("\n Writeback stage : \t %s R%d R%d R%d", iwb_input.opcode, iwb_input.dest, iwb_input.src1, iwb_input.src2);
            register_File[iwb_input.dest].value = iwb_input.result;
            if (iwb_input.id == register_File[iwb_input.dest].ins_id)
                register_File[iwb_input.dest].status = VALID;
            iwb_input = nop;
        }
        else if (!(strcmp(iwb_input.opcode, "AND"))){
            printf("\n Writeback stage : \t %s R%d R%d R%d", iwb_input.opcode, iwb_input.dest, iwb_input.src1, iwb_input.src2);
            last_arithmetic_ind = iwb_input.index;
            last_arithmetic_result = iwb_input.result;

            register_File[iwb_input.dest].value = iwb_input.result;
            if (iwb_input.id == register_File[iwb_input.dest].ins_id)
                register_File[iwb_input.dest].status = VALID;
            iwb_input = nop;
        }
        else if (!(strcmp(iwb_input.opcode, "OR"))){
            printf("\n Writeback stage : \t %s R%d R%d R%d", iwb_input.opcode, iwb_input.dest, iwb_input.src1, iwb_input.src2);
            last_arithmetic_ind = iwb_input.index;
            last_arithmetic_result = iwb_input.result;

            register_File[iwb_input.dest].value = iwb_input.result;
            if (iwb_input.id == register_File[iwb_input.dest].ins_id)
                register_File[iwb_input.dest].status = VALID;
            iwb_input = nop;
        }
        else if (!(strcmp(iwb_input.opcode, "EX-OR"))){
            printf("\n Writeback stage : \t %s R%d R%d R%d", iwb_input.opcode, iwb_input.dest, iwb_input.src1, iwb_input.src2);
            last_arithmetic_ind = iwb_input.index;
            last_arithmetic_result = iwb_input.result;

            register_File[iwb_input.dest].value = iwb_input.result;
            if (iwb_input.id == register_File[iwb_input.dest].ins_id)
                register_File[iwb_input.dest].status = VALID;
            iwb_input = nop;
        }
        else if (!(strcmp(iwb_input.opcode, "BNZ"))){
            //if branch was calculated to be taken
            if (iwb_input.branch == 1){
                printf("\n Writeback stage : \t %s %d", iwb_input.opcode, iwb_input.literal);
                if((strcmp(idrf_input.opcode, "nop")))
                    register_File[idrf_input.dest].status = VALID;
                if((strcmp(iex_input.opcode, "nop")))
                    register_File[iex_input.dest].status = VALID;
                if((strcmp(imul2_input.opcode, "nop")))
                    register_File[imul2_input.dest].status = VALID;
                if((strcmp(idiv2_input.opcode, "nop")))
                    register_File[idiv2_input.dest].status = VALID;
                if((strcmp(idiv3_input.opcode, "nop")))
                    register_File[idiv3_input.dest].status = VALID;
                if((strcmp(idiv4_input.opcode, "nop")))
                    register_File[idiv4_input.dest].status = VALID;
                if((strcmp(imem_input.opcode, "nop")))
                    register_File[imem_input.dest].status = VALID;
                if_input = imem_input = iex_input = idrf_input = imul2_input = idiv2_input = idiv4_input = idiv3_input = nop;
                pc = iwb_input.result;
                iwb_input = nop;
                bflag = 1;
            }
            else {
                printf("\n Writeback stage : \t %s %d", iwb_input.opcode, iwb_input.literal);
                iwb_input = nop;
            }
        }
        else if (!(strcmp(iwb_input.opcode, "BZ"))){
            //if branch was calculated to be taken
            if (iwb_input.branch == 1){
                printf("\n Writeback stage : \t %s %d", iwb_input.opcode, iwb_input.literal);
                if((strcmp(idrf_input.opcode, "nop")))
                    register_File[idrf_input.dest].status = VALID;
                if((strcmp(iex_input.opcode, "nop")))
                    register_File[iex_input.dest].status = VALID;
                if((strcmp(imul2_input.opcode, "nop")))
                    register_File[imul2_input.dest].status = VALID;
                if((strcmp(idiv2_input.opcode, "nop")))
                    register_File[idiv2_input.dest].status = VALID;
                if((strcmp(idiv3_input.opcode, "nop")))
                    register_File[idiv3_input.dest].status = VALID;
                if((strcmp(idiv4_input.opcode, "nop")))
                    register_File[idiv4_input.dest].status = VALID;
                if((strcmp(imem_input.opcode, "nop")))
                    register_File[imem_input.dest].status = VALID;
                if_input = imem_input = iex_input = idrf_input = imul2_input = idiv2_input = idiv4_input = idiv3_input = nop;
                pc = iwb_input.result;
                iwb_input = nop;
                bflag = 1;
            }
            else {
                printf("\n Writeback stage : \t %s %d", iwb_input.opcode, iwb_input.literal);
                iwb_input = nop;
            }
        }
        else if (!(strcmp(iwb_input.opcode, "HALT"))){
            printf("\n Writeback stage : \t %s ", iwb_input.opcode);
            iwb_input = nop;
            hflag = 100;
        }
        else if (!(strcmp(iwb_input.opcode, "JUMP"))){
            printf("\n Writeback stage : \t %s R%d %d", iwb_input.opcode, iwb_input.src1, iwb_input.literal);
            if_input = imem_input = iex_input = idrf_input = nop;
            pc = iwb_input.result;
            iwb_input = nop;
            bflag = 1;
        }
        else if (!(strcmp(iwb_input.opcode, "JAL"))){
            printf("\n Writeback stage : \t %s R%d R%d %d", iwb_input.opcode, iwb_input.dest, iwb_input.src1, iwb_input.literal);
            if_input = imem_input = iex_input = idrf_input = nop;
            pc = iwb_input.result;
            register_File[iwb_input.dest].value = iwb_input.index;
            register_File[iwb_input.dest].status = VALID; 
            iwb_input = nop;
            bflag = 1;
        }
        //else if
    }
    else
        printf("\n Writeback stage : \t idle");
}

void mul2(){
    imul2_input.result = register_File[imul2_input.src1].value * register_File[imul2_input.src2].value;
    if (imul2_input.id == register_File[imul2_input.dest].ins_id)
        register_File[imul2_input.dest].status = VALID;
    register_File[imul2_input.dest].value = imul2_input.result;
    last_arithmetic_ind = iex_input.index;
            last_arithmetic_result = iex_input.result;

    if(!(strcmp(imem_input.opcode, "nop"))){
        printf("\n Mul2 stage : \t\t %s R%d R%d R%d", imul2_input.opcode, imul2_input.dest, imul2_input.src1, imul2_input.src2);
        imem_input = imul2_input;
        imul2_input = nop;
        mflag = 0;
    }
    else
        printf("\n Mul2 stage : \t\t %s R%d R%d R%d stalled", iex_input.opcode, iex_input.dest, iex_input.src1, iex_input.src2);
}

void div2(){
    if (!(strcmp(idiv2_input.opcode, "DIV"))){
        if(!(strcmp(idiv3_input.opcode, "nop"))){
            printf("\n DU2 stage : \t\t %s R%d R%d R%d", idiv2_input.opcode, idiv2_input.dest, idiv2_input.src1, idiv2_input.src2);
            idiv3_input = idiv2_input;
            idiv2_input = nop;
            d1flag = 1;
        }
        else
            printf("\n DU2 stage : \t\t %s R%d R%d R%d stalled", idiv2_input.opcode, idiv2_input.dest, idiv2_input.src1, idiv2_input.src2);
    }
    else if (!(strcmp(idiv2_input.opcode, "HALT"))){
        if(!(strcmp(idiv3_input.opcode, "nop"))){
            printf("\n DU2 stage : \t\t %s ", idiv2_input.opcode);
            idiv3_input = idiv2_input;
            idiv2_input = nop;
            d1flag = 1;
        }
        else
            printf("\n DU2 stage : \t\t %s stalled", idiv2_input.opcode);
    }
}

void div3(){
    if (!(strcmp(idiv3_input.opcode, "DIV"))){
        if(!(strcmp(idiv4_input.opcode, "nop"))){
            printf("\n DU3 stage : \t\t %s R%d R%d R%d", idiv3_input.opcode, idiv3_input.dest, idiv3_input.src1, idiv3_input.src2);
            idiv4_input = idiv3_input;
            idiv3_input = nop;
            d2flag = 1;
            d1flag = 0;
        }
        else
            printf("\n DU3 stage : \t\t %s R%d R%d R%d stalled", idiv3_input.opcode, idiv3_input.dest, idiv3_input.src1, idiv3_input.src2);
    }
    else if (!(strcmp(idiv3_input.opcode, "HALT"))){
        if(!(strcmp(idiv4_input.opcode, "nop"))){
            printf("\n DU3 stage : \t\t %s ", idiv3_input.opcode);
            idiv4_input = idiv3_input;
            idiv3_input = nop;
            d2flag = 1;
            d1flag = 0;
        }
        else
            printf("\n DU3 stage : \t\t %s stalled", idiv3_input.opcode);
    }
}

//MAKE HFLAG = 0
void div4(){
    if (!(strcmp(idiv4_input.opcode, "DIV"))){
        idiv4_input.result = (register_File[idiv4_input.src1].value) / (register_File[idiv4_input.src2].value);
        register_File[idiv4_input.dest].value = idiv4_input.result;
        if (idiv4_input.id == register_File[idiv4_input.dest].ins_id)
            register_File[idiv4_input.dest].status = VALID;
        last_arithmetic_ind = iex_input.index;
        last_arithmetic_result = iex_input.result;

        if(!(strcmp(imem_input.opcode, "nop"))){
            printf("\n DU4 stage : \t\t %s R%d R%d R%d", idiv4_input.opcode, idiv4_input.dest, idiv4_input.src1, idiv4_input.src2);
            imem_input = idiv4_input;
            idiv4_input = nop;
            if (!(strcmp(idiv3_input.opcode, "nop")) && !(strcmp(idiv2_input.opcode, "nop"))){
                dflag = 0;
            }
            d2flag = 0;
        }
        else
            printf("\n DU4 stage : \t\t %s R%d R%d R%d stalled", idiv4_input.opcode, idiv4_input.dest, idiv4_input.src1, idiv4_input.src2);
    }
    else if (!(strcmp(idiv4_input.opcode, "HALT"))){
        if(!(strcmp(imem_input.opcode, "nop"))){
            printf("\n DU4 stage : \t\t %s", idiv4_input.opcode);
            imem_input = idiv4_input;
            idiv4_input = nop;
            if (!(strcmp(idiv3_input.opcode, "nop")) && !(strcmp(idiv2_input.opcode, "nop"))){
                dflag = 0;
            }
            d2flag = 0;
        }
        else
            printf("\n DU4 stage : \t\t %s stalled", idiv4_input.opcode);
    }
}

void intialize(){
    for (int i = 0; i < 16; i++){
        register_File[i].status = VALID;
        register_File[i].value = 0.0;
        register_File[i].ins_id = 0;
    }
}

void display(){
    printf("\n---------Register File-----------\n");
    for(int i =0; i<16; i++){
        printf("R%d = %ld \n", i, register_File[i].value);
    }
    printf("\n---------Data Memory-------------\n");
    for(int i =0; i < 100; i++){
        printf("data_mem[%d] = %ld \n", i, data_Memory[i]);
    }
}
