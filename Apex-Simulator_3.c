#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef VALID
#define VALID 1
#endif
#ifndef INVALID
#define INVALID 0
#endif
#ifndef max_instructions
#define max_instructions 100 //Maximum number of instructions that can be simulated
#endif

long data_Memory[1000];
int pc = 0;
int line_Number = 0;

//All functions declarations
void fetch();
void decode();
void execute();
void memory();  //Only for Load and Store ins. Forward results to the lsq for LOAD bypassing
void mul1();
void mul2();
void div1();
void div2();
void div3();
void div4();
void intialize();
void simulate(char file_name[]);
void display();
int prf_available();
int find_new_prf();
void old_instance_prf(int ); //for freeing the prf if new instance of AR is generated and last one's value has been received set its newInstance bit to 1.
int find_existing_prf(int, int); //old_instance_prf and find_existing_prf work in the similar way.
void iq();
void LSQ();
void ROB();

//ARF structure
typedef struct {
    long value;
    int status;
    int ins_id;
    int prf_id;
}registers;

registers arf_register_File[16] = {};

//PRF structure
typedef struct {
    long value;
    int status;
    int ins_id;
    int arf_id;
    int busy; // to see if the register is free or busy
    int old_instance; //for freeing if set
}prf;

prf prf_register_File[32];

//Instructions structure
typedef struct {

    int address;  //address offset only for load and store

    char opcode[25];
    int src1;
    int src2;
    int dest;
    int literal;
    long result;
    int index;    // to carry pc value along the BZ, BNZ, JAL instructions for updating pc value in the wb stage.
    int branch;  // 1 if decoded to be taken 0 otherwise
    int id;     //for now, no. of each instruction in the file      // for avoiding the conflict which ins is updating status of the dest reg
                // comparing with last_arithmetic_ind to check if the last arithmatic ins has reached wb
    int status; // use in issu or rob or lsq for their particular operations
                // eg. if status == 1 "commit", == 0 "wait and get the values required "
                // set to

} Instructions;

Instructions instruction[max_instructions];
Instructions * ptr_instruction = instruction;

Instructions iqueue[16]; //Instruction Queue
Instructions rob[32];   //ReOrder Buffer
Instructions lsq[32];   //Load Store Queue

const Instructions nop = {0, "nop", 0, 0, -1, 0, 0, 0, 0, 0, 0};
Instructions if_input = {0, "nop", 0, 0, -1, 0, 0, 0, 0, 0, 0};
Instructions idrf_input = {0, "nop", 0, 0, -1, 0, 0, 0, 0, 0, 0};
Instructions iex_input = {0, "nop", 0, 0, -1, 0, 0, 0, 0, 0, 0};
Instructions imem_input = {0, "nop", 0, 0, -1, 0, 0, 0, 0, 0, 0};
Instructions imul1_input = {0, "nop", 0, 0, -1, 0, 0, 0, 0, 0, 0};
Instructions imul2_input = {0, "nop", 0, 0, -1, 0, 0, 0, 0, 0, 0};
Instructions idiv1_input = {0, "nop", 0, 0, -1, 0, 0, 0, 0, 0, 0};
Instructions idiv2_input = {0, "nop", 0, 0, -1, 0, 0, 0, 0, 0, 0};
Instructions idiv3_input = {0, "nop", 0, 0, -1, 0, 0, 0, 0, 0, 0};
Instructions idiv4_input = {0, "nop", 0, 0, -1, 0, 0, 0, 0, 0, 0};

int last_arithmetic_ind = -1;
int last_arithmetic_instr = -1;
long last_arithmetic_result = -10;
int ind = 0;
int bflag = 0;    // adds one more cycle for branch instructions
int hflag = 0;    // halt flag when set stop fetching instructions
int mflag = 0;    // IS set in IQ
int dflag = 0;
int id = 1;         //for now, no. of each instruction in the file               //for conflicting register status validation
int ch = 0;     // choice of user
int bzflag = 0; //stop fetching instr if branching is calculated to be true at the head of IQ
int jflag = 0;
int jalflag = 0;

int temp1 = 0;  //Use for swapping in the renaming process of dest and sources registers
int temp2 = 0;
int temp3 = 0;

int iq_add = 0;     //index of the IQ where the next instruction should be added if space avalable.
int iq_rem = 0;     //index of the IQ from where the next instruction should be issued if valid.
int iq_full = 0;

int rob_add = 0;    //index for adding 
int rob_com = 0;    // index for comming from
int rob_full = 0;   

int lsq_add = 0;     //index of the LSQ where the next instruction should be added if space avalable.
int lsq_rem = 0;     //index of the LSQ from where the next instruction should be issued if valid.
int lsq_full = 0;

int main(){

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
            ptr_instruction->id = id;
            id++;
            printf("\t%s R%d %d", ptr_instruction->opcode, ptr_instruction->dest, ptr_instruction->literal);
        }
        else if (!(strcmp(ptr_instruction->opcode, "STORE"))){
            sscanf(line, "%[^,],R%d,R%d,#%d", ptr_instruction->opcode, &ptr_instruction->src1, &ptr_instruction->src2, &ptr_instruction->literal);
            printf("\t%s R%d R%d %d", ptr_instruction->opcode, ptr_instruction->src1, ptr_instruction->src2, ptr_instruction->literal);
            ptr_instruction->id = id;
            id++;
            ptr_instruction->dest = -1;
        }
        else if (!(strcmp(ptr_instruction->opcode, "LOAD"))){
            sscanf(line, "%[^,],R%d,R%d,#%d", ptr_instruction->opcode, &ptr_instruction->dest, &ptr_instruction->src1, &ptr_instruction->literal);
            printf("\t%s R%d R%d %d", ptr_instruction->opcode, ptr_instruction->dest, ptr_instruction->src1, ptr_instruction->literal);
            ptr_instruction->id = id;
            id++;
        }
        else if ((!(strcmp(ptr_instruction->opcode, "MUL")) || !(strcmp(ptr_instruction->opcode, "ADD")) || !(strcmp(ptr_instruction->opcode, "SUB")) || !(strcmp(ptr_instruction->opcode, "OR")) || !(strcmp(ptr_instruction->opcode, "AND")) || !(strcmp(ptr_instruction->opcode, "EX-OR")))){
            sscanf(line, "%[^,],R%d,R%d,R%d", ptr_instruction->opcode, &ptr_instruction->dest, &ptr_instruction->src1, &ptr_instruction->src2);
            printf("\t%s R%d R%d R%d", ptr_instruction->opcode, ptr_instruction->dest, ptr_instruction->src1, ptr_instruction->src2);
            ptr_instruction->id = id;
            id++;
        }
        else if ((!(strcmp(ptr_instruction->opcode, "BZ")) || !(strcmp(ptr_instruction->opcode, "BNZ")))){
            sscanf(line, "%[^,],#%d", ptr_instruction->opcode, &ptr_instruction->literal);
            printf("\t%s %d", ptr_instruction->opcode, ptr_instruction->literal);
            ptr_instruction->id = id;
            id++;
            ptr_instruction->dest = -1;
        }
        else if (!(strcmp(ptr_instruction->opcode, "JUMP"))){
            sscanf(line, "%[^,],R%d,#%d", ptr_instruction->opcode, &ptr_instruction->src1, &ptr_instruction->literal);
            printf("\t%s R%d %d", ptr_instruction->opcode, ptr_instruction->src1, ptr_instruction->literal);
            ptr_instruction->id = id;
            id++;
            ptr_instruction->dest = -1;
        }
        else if (!(strcmp(ptr_instruction->opcode, "HALT"))){
            sscanf(line, "%[^,]", ptr_instruction->opcode);
            printf("\t%s", ptr_instruction->opcode);
            ptr_instruction->id = id;
            id++;
            ptr_instruction->dest = -1;
        }
        else if (!(strcmp(ptr_instruction->opcode, "DIV"))) {
            sscanf(line, "%[^,],R%d,R%d,R%d", ptr_instruction->opcode, &ptr_instruction->dest, &ptr_instruction->src1, &ptr_instruction->src2);
            printf("\t%s R%d R%d R%d", ptr_instruction->opcode, ptr_instruction->dest, ptr_instruction->src1, ptr_instruction->src2);
            ptr_instruction->id = id;
            id++;
        }
        else if (!(strcmp(ptr_instruction->opcode, "JAL"))){
            sscanf(line, "%[^,],R%d,R%d,#%d", ptr_instruction->opcode, &ptr_instruction->dest, &ptr_instruction->src1, &ptr_instruction->literal);
            printf("\t%s R%d R%d %d", ptr_instruction->opcode, ptr_instruction->dest, ptr_instruction->src1, ptr_instruction->literal);
            ptr_instruction->id = id;
            id++;
        }
        else{
             printf("\t%s", "Not a valid opcode");
        }
        ptr_instruction++;

    }
    fclose(ptr_File);
    // call lsq before mem stage
    for (int i = 1; i <= cycles; i++){
        printf("\n--------Cycle No. = %d---------", i);
        ROB();
        ROB();
        if (dflag == 1 || hflag == 1){
            div4();
            div3();
            div2();
            div1();
        }
        if (mflag == 1){
            mul2();
            mul1();
        }
        execute();
        iq();
        iq();               // Call before execution stage ***twice***
        memory();
        LSQ();
        decode();
        fetch();
        if (hflag == 100)
            break;
    }
}

void fetch(){
    if((pc <= line_Number) && hflag == 0 && bzflag == 0 && jflag == 0 && jalflag == 0) {
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
                    if_input.index = pc;
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
                    if_input.index = pc;
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
//printf("\n****** dest: %d", if_input.dest);
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
        else if (!(strcmp(if_input.opcode, "LOAD"))){
            if(!(strcmp(idrf_input.opcode, "nop"))){
                if (bflag == 0){
                    printf("\n Fetch stage : \t\t %s R%d R%d %d", if_input.opcode, if_input.dest, if_input.src1, if_input.literal);
                    pc++;
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
        else if (!(strcmp(if_input.opcode, "STORE"))){
            if(!(strcmp(idrf_input.opcode, "nop"))){
                if (bflag == 0){
                    printf("\n Fetch stage : \t\t %s R%d R%d %d", if_input.opcode, if_input.src1, if_input.src2, if_input.literal);
                    pc++;
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

//while renaming always asign new physical reg for the dest register and look for the recent prf for the sources in the rename table
//DRF stage
void decode(){
    int dest = -1;

    if((strcmp(idrf_input.opcode, "nop"))){
        if (!(strcmp(idrf_input.opcode, "MOVC"))){
                if(prf_available() == 1 && iq_full == 0 && rob_full == 0){
                    printf("\n Decode stage : \t %s R%d %d", idrf_input.opcode, idrf_input.dest, idrf_input.literal);

                    rob[rob_add] = idrf_input;
                    rob_add++;
                    
                    temp1 = idrf_input.dest;
                    old_instance_prf(idrf_input.dest);
                    dest = find_new_prf();
                    idrf_input.dest = dest;
                    prf_register_File[dest].status = INVALID;
                    prf_register_File[dest].ins_id = idrf_input.id;         // for later use in rob, iq
                    prf_register_File[dest].busy = 1;                   // make 0 in rob when the inst id of prf mathces the index of instruction being commited
                    prf_register_File[dest].arf_id = temp1;

                    printf("\n Decode stage : \t %s P%d %d", idrf_input.opcode, idrf_input.dest, idrf_input.literal);
                    //printf("\n stat %d ins_id %d busy %d arf_id %d\n", prf_register_File[dest].status, prf_register_File[dest].ins_id, prf_register_File[dest].busy, prf_register_File[dest].arf_id );

                    iqueue[iq_add] = idrf_input;
                    idrf_input = nop;
                    iq_add++;                               // can instead use in IQUEUE function
                }
                else
                    printf("\n Decode stage : \t %s R%d %d stalled", idrf_input.opcode, idrf_input.dest, idrf_input.literal);
        }
        else if (!(strcmp(idrf_input.opcode, "ADD"))){
                if(prf_available() == 1 && iq_full == 0 && rob_full == 0){
                    printf("\n Decode stage : \t %s R%d R%d R%d", idrf_input.opcode, idrf_input.dest, idrf_input.src1, idrf_input.src2);

                    idrf_input.src1 = find_existing_prf(idrf_input.src1, idrf_input.id);
                    idrf_input.src2 = find_existing_prf(idrf_input.src2, idrf_input.id);

                    temp1 = idrf_input.dest;
                    old_instance_prf(idrf_input.dest);
                    dest = find_new_prf();
                    idrf_input.dest = dest;
                    prf_register_File[dest].status = INVALID;
                    prf_register_File[dest].ins_id = idrf_input.id;         // for later use in rob, iq
                    prf_register_File[dest].busy = 1;                   // make 0 in rob when the inst id of prf mathces the index of instruction being commited
                    prf_register_File[dest].arf_id = temp1;

                    printf("\n Decode stage : \t %s P%d P%d P%d", idrf_input.opcode, idrf_input.dest, idrf_input.src1, idrf_input.src2);

                    rob[rob_add] = idrf_input;
                    rob[rob_add].dest = temp1;
                    rob_add++;
                  
                    iqueue[iq_add] = idrf_input;
                    idrf_input = nop;
                    iq_add++;                               // can instead use in IQUEUE function
                }
                else
                    printf("\n Decode stage : \t %s R%d R%d R%d stalled", idrf_input.opcode, idrf_input.dest, idrf_input.src1, idrf_input.src2);
        }
        else if (!(strcmp(idrf_input.opcode, "SUB"))){
                if(prf_available() == 1 && iq_full == 0 && rob_full == 0){
                    printf("\n Decode stage : \t %s R%d R%d R%d", idrf_input.opcode, idrf_input.dest, idrf_input.src1, idrf_input.src2);

                    idrf_input.src1 = find_existing_prf(idrf_input.src1, idrf_input.id);
                    idrf_input.src2 = find_existing_prf(idrf_input.src2, idrf_input.id);
                    
                    temp1 = idrf_input.dest;
                    old_instance_prf(idrf_input.dest);
                    dest = find_new_prf();
                    idrf_input.dest = dest;
                    prf_register_File[dest].status = INVALID;
                    prf_register_File[dest].ins_id = idrf_input.id;         // for later use in rob, iq
                    prf_register_File[dest].busy = 1;                   // make 0 in rob when the inst id of prf mathces the index of instruction being commited
                    prf_register_File[dest].arf_id = temp1;

                    printf("\n Decode stage : \t %s P%d P%d P%d", idrf_input.opcode, idrf_input.dest, idrf_input.src1, idrf_input.src2);

                    rob[rob_add] = idrf_input;
                    rob[rob_add].dest = temp1;
                    rob_add++;
                
                    iqueue[iq_add] = idrf_input;
                    idrf_input = nop;
                    iq_add++;                               // can instead use in IQUEUE function
                }
                else
                    printf("\n Decode stage : \t %s R%d R%d R%d", idrf_input.opcode, idrf_input.dest, idrf_input.src1, idrf_input.src2);
        }
        else if (!(strcmp(idrf_input.opcode, "AND"))){
                if(prf_available() == 1 && iq_full == 0 && rob_full == 0){
                    printf("\n Decode stage : \t %s R%d R%d R%d", idrf_input.opcode, idrf_input.dest, idrf_input.src1, idrf_input.src2);
                    
                    idrf_input.src1 = find_existing_prf(idrf_input.src1, idrf_input.id);
                    idrf_input.src2 = find_existing_prf(idrf_input.src2, idrf_input.id);
                    
                    temp1 = idrf_input.dest;
                    old_instance_prf(idrf_input.dest);
                    dest = find_new_prf();
                    idrf_input.dest = dest;
                    prf_register_File[dest].status = INVALID;
                    prf_register_File[dest].ins_id = idrf_input.id;         // for later use in rob, iq
                    prf_register_File[dest].busy = 1;                   // make 0 in rob when the inst id of prf mathces the index of instruction being commited
                    prf_register_File[dest].arf_id = temp1;

                    printf("\n Decode stage : \t %s P%d P%d P%d", idrf_input.opcode, idrf_input.dest, idrf_input.src1, idrf_input.src2);

                    rob[rob_add] = idrf_input;
                    rob[rob_add].dest = temp1;
                    rob_add++;
                
                    iqueue[iq_add] = idrf_input;
                    idrf_input = nop;
                    iq_add++;                               // can instead use in IQUEUE function
                }
                else
                    printf("\n Decode stage : \t %s R%d R%d R%d stalled", idrf_input.opcode, idrf_input.dest, idrf_input.src1, idrf_input.src2);
        }
        else if (!(strcmp(idrf_input.opcode, "OR"))){
                if(prf_available() == 1 && iq_full == 0 && rob_full == 0){
                    printf("\n Decode stage : \t %s R%d R%d R%d", idrf_input.opcode, idrf_input.dest, idrf_input.src1, idrf_input.src2);
                    
                    idrf_input.src1 = find_existing_prf(idrf_input.src1, idrf_input.id);
                    idrf_input.src2 = find_existing_prf(idrf_input.src2, idrf_input.id);
                    
                    temp1 = idrf_input.dest;
                    old_instance_prf(idrf_input.dest);
                    dest = find_new_prf();
                    idrf_input.dest = dest;
                    prf_register_File[dest].status = INVALID;
                    prf_register_File[dest].ins_id = idrf_input.id;         // for later use in rob, iq
                    prf_register_File[dest].busy = 1;                   // make 0 in rob when the inst id of prf mathces the index of instruction being commited
                    prf_register_File[dest].arf_id = temp1;

                    printf("\n Decode stage : \t %s P%d P%d P%d", idrf_input.opcode, idrf_input.dest, idrf_input.src1, idrf_input.src2);

                    rob[rob_add] = idrf_input;
                    rob[rob_add].dest = temp1;
                    rob_add++;
                
                    iqueue[iq_add] = idrf_input;
                    idrf_input = nop;
                    iq_add++;                               // can instead use in IQUEUE function
                }
                else
                    printf("\n Decode stage : \t %s R%d R%d R%d stalled", idrf_input.opcode, idrf_input.dest, idrf_input.src1, idrf_input.src2);
        }
        else if (!(strcmp(idrf_input.opcode, "EX-OR"))){
                if(prf_available() == 1 && iq_full == 0 && rob_full == 0){
                    printf("\n Decode stage : \t %s R%d R%d R%d", idrf_input.opcode, idrf_input.dest, idrf_input.src1, idrf_input.src2);
                    
                    idrf_input.src1 = find_existing_prf(idrf_input.src1, idrf_input.id);
                    idrf_input.src2 = find_existing_prf(idrf_input.src2, idrf_input.id);
                    
                    temp1 = idrf_input.dest;
                    old_instance_prf(idrf_input.dest);
                    dest = find_new_prf();
                    idrf_input.dest = dest;
                    prf_register_File[dest].status = INVALID;
                    prf_register_File[dest].ins_id = idrf_input.id;         // for later use in rob, iq
                    prf_register_File[dest].busy = 1;                   // make 0 in rob when the inst id of prf mathces the index of instruction being commited
                    prf_register_File[dest].arf_id = temp1;

                    printf("\n Decode stage : \t %s P%d P%d P%d", idrf_input.opcode, idrf_input.dest, idrf_input.src1, idrf_input.src2);

                    rob[rob_add] = idrf_input;
                    rob[rob_add].dest = temp1;
                    rob_add++;
                
                    iqueue[iq_add] = idrf_input;
                    idrf_input = nop;
                    iq_add++;                               // can instead use in IQUEUE function
                }
                else
                    printf("\n Decode stage : \t %s R%d R%d R%d stalled", idrf_input.opcode, idrf_input.dest, idrf_input.src1, idrf_input.src2);
        }
        else if (!(strcmp(idrf_input.opcode, "MUL"))){
                if(prf_available() == 1 && iq_full == 0 && rob_full == 0){
                    printf("\n Decode stage : \t %s R%d R%d R%d", idrf_input.opcode, idrf_input.dest, idrf_input.src1, idrf_input.src2);

                    idrf_input.src1 = find_existing_prf(idrf_input.src1, idrf_input.id);
                    idrf_input.src2 = find_existing_prf(idrf_input.src2, idrf_input.id);

                    temp1 = idrf_input.dest;
                    old_instance_prf(idrf_input.dest);
                    dest = find_new_prf();
                    idrf_input.dest = dest;
                    prf_register_File[dest].status = INVALID;
                    prf_register_File[dest].ins_id = idrf_input.id;         // for later use in rob, iq
                    prf_register_File[dest].busy = 1;                   // make 0 in rob when the inst id of prf mathces the index of instruction being commited
                    prf_register_File[dest].arf_id = temp1;

                    printf("\n Decode stage : \t %s P%d P%d P%d", idrf_input.opcode, idrf_input.dest, idrf_input.src1, idrf_input.src2);

                    rob[rob_add] = idrf_input;
                    rob[rob_add].dest = temp1;
                    rob_add++;
                  
                    iqueue[iq_add] = idrf_input;
                    idrf_input = nop;
                    iq_add++;                               // can instead use in IQUEUE function
                }
                else
                    printf("\n Decode stage : \t %s R%d R%d R%d stalled", idrf_input.opcode, idrf_input.dest, idrf_input.src1, idrf_input.src2);
        }
        else if (!(strcmp(idrf_input.opcode, "DIV"))){
                if(prf_available() == 1 && iq_full == 0 && rob_full == 0){
                    printf("\n Decode stage : \t %s R%d R%d R%d", idrf_input.opcode, idrf_input.dest, idrf_input.src1, idrf_input.src2);

                    idrf_input.src1 = find_existing_prf(idrf_input.src1, idrf_input.id);
                    idrf_input.src2 = find_existing_prf(idrf_input.src2, idrf_input.id);

                    temp1 = idrf_input.dest;
                    old_instance_prf(idrf_input.dest);
                    dest = find_new_prf();
                    idrf_input.dest = dest;
                    prf_register_File[dest].status = INVALID;
                    prf_register_File[dest].ins_id = idrf_input.id;         // for later use in rob, iq
                    prf_register_File[dest].busy = 1;                   // make 0 in rob when the inst id of prf mathces the index of instruction being commited
                    prf_register_File[dest].arf_id = temp1;

                    printf("\n Decode stage : \t %s P%d P%d P%d", idrf_input.opcode, idrf_input.dest, idrf_input.src1, idrf_input.src2);

                    rob[rob_add] = idrf_input;
                    rob[rob_add].dest = temp1;
                    rob_add++;
                  
                    iqueue[iq_add] = idrf_input;
                    idrf_input = nop;
                    iq_add++;                               // can instead use in IQUEUE function
                }
                else
                    printf("\n Decode stage : \t %s R%d R%d R%d stalled", idrf_input.opcode, idrf_input.dest, idrf_input.src1, idrf_input.src2);
        }
        else if (!(strcmp(idrf_input.opcode, "HALT"))){
            if(prf_available() == 1 && iq_full == 0 && rob_full == 0){
                printf("\n Decode stage : \t %s ", idrf_input.opcode);
                
                rob[rob_add] = idrf_input;
                rob_add++;
                iqueue[iq_add] = idrf_input;
                iq_add++;
                
                if_input = nop;
                hflag = 1;   // HALT flag
                idrf_input = nop;
            }
            else
                printf("\n Decode stage : \t %s stalled", idrf_input.opcode);
        }
        else if (!(strcmp(idrf_input.opcode, "JUMP"))){
            if(iq_full == 0 && rob_full == 0){
                printf("\n Decode stage : \t %s R%d %d ", idrf_input.opcode, idrf_input.src1, idrf_input.literal);
                
                idrf_input.src1 = find_existing_prf(idrf_input.src1, idrf_input.id);
                printf("\n Decode stage : \t %s P%d %d ", idrf_input.opcode, idrf_input.src1, idrf_input.literal);

                rob[rob_add] = idrf_input;
                rob_add++;
                iqueue[iq_add] = idrf_input;
                iq_add++;

                if_input = nop;
                jflag = 1;          //set bflag = 1 in rob when commiting JUMP ins.
                idrf_input = nop;
            }
            else
                printf("\n Decode stage : \t %s R%d %d stalled", idrf_input.opcode, idrf_input.src1, idrf_input.literal);
        }
        else if (!(strcmp(idrf_input.opcode, "JAL"))){
                if(prf_available() == 1 && iq_full == 0 && rob_full == 0){
                    printf("\n Decode stage : \t %s R%d R%d %d", idrf_input.opcode, idrf_input.dest, idrf_input.src1, idrf_input.literal);

                    idrf_input.src1 = find_existing_prf(idrf_input.src1, idrf_input.id);
                    
                    temp1 = idrf_input.dest;
                    old_instance_prf(idrf_input.dest);
                    dest = find_new_prf();
                    idrf_input.dest = dest;
                    prf_register_File[dest].status = INVALID;
                    prf_register_File[dest].ins_id = idrf_input.id;         // for later use in rob, iq
                    prf_register_File[dest].busy = 1;                   // make 0 in rob when the inst id of prf mathces the index of instruction being commited
                    prf_register_File[dest].arf_id = temp1;

                    printf("\n Decode stage : \t %s P%d P%d %d", idrf_input.opcode, idrf_input.dest, idrf_input.src1, idrf_input.literal);

                    rob[rob_add] = idrf_input;
                    rob[rob_add].dest = temp1;
                    rob_add++;
                  
                    iqueue[iq_add] = idrf_input;
                    idrf_input = nop;
                    iq_add++;                               // can instead use in IQUEUE function
                    
                    if_input = nop;
                    jalflag = 1;          //make 0 in rob and set bflag = 1 in rob when commiting JAL ins.
                }
                else
                    printf("\n Decode stage : \t %s R%d R%d %d stalled", idrf_input.opcode, idrf_input.dest, idrf_input.src1, idrf_input.literal);
        }
        else if (!(strcmp(idrf_input.opcode, "BNZ"))){
            if(iq_full == 0 && rob_full == 0){
                printf("\n Decode stage : \t %s %d ", idrf_input.opcode, idrf_input.literal);

                rob[rob_add] = idrf_input;
                rob_add++;
                iqueue[iq_add] = idrf_input;
                iq_add++;

                idrf_input = nop;
            }
            else
                printf("\n Decode stage : \t %s %d stalled", idrf_input.opcode, idrf_input.literal);
        }
        else if (!(strcmp(idrf_input.opcode, "BZ"))){
            if(iq_full == 0 && rob_full == 0){
                printf("\n Decode stage : \t %s %d ", idrf_input.opcode, idrf_input.literal);

                rob[rob_add] = idrf_input;
                rob_add++;
                iqueue[iq_add] = idrf_input;
                iq_add++;

                idrf_input = nop;
            }
            else
                printf("\n Decode stage : \t %s %d stalled", idrf_input.opcode, idrf_input.literal);
        }
        else if (!(strcmp(idrf_input.opcode, "LOAD"))){
                if(prf_available() == 1 && iq_full == 0 && rob_full == 0 && lsq_full == 0){
                    printf("\n Decode stage : \t %s R%d R%d %d", idrf_input.opcode, idrf_input.dest, idrf_input.src1, idrf_input.literal);

                    idrf_input.src1 = find_existing_prf(idrf_input.src1, idrf_input.id);

                    temp1 = idrf_input.dest;
                    old_instance_prf(idrf_input.dest);
                    dest = find_new_prf();
                    idrf_input.dest = dest;
                    prf_register_File[dest].status = INVALID;
                    prf_register_File[dest].ins_id = idrf_input.id;         // for later use in rob, iq
                    prf_register_File[dest].busy = 1;                   // make 0 in rob when the inst id of prf mathces the index of instruction being commited
                    prf_register_File[dest].arf_id = temp1;

                    printf("\n Decode stage : \t %s P%d P%d %d", idrf_input.opcode, idrf_input.dest, idrf_input.src1, idrf_input.literal);

                    rob[rob_add] = idrf_input;
                    rob[rob_add].dest = temp1;
                    rob_add++;

                    lsq[lsq_add] = idrf_input;
                    lsq_add++;
                  
                    iqueue[iq_add] = idrf_input;
                    idrf_input = nop;
                    iq_add++;                               // can instead use in IQUEUE function
                }
                else
                    printf("\n Decode stage : \t %s R%d R%d %d stalled", idrf_input.opcode, idrf_input.dest, idrf_input.src1, idrf_input.literal);
        }
        else if (!(strcmp(idrf_input.opcode, "STORE"))){
                if(iq_full == 0 && rob_full == 0 && lsq_full == 0){
                    printf("\n Decode stage : \t %s R%d R%d %d", idrf_input.opcode, idrf_input.src1, idrf_input.src2, idrf_input.literal);

                    idrf_input.src1 = find_existing_prf(idrf_input.src1, idrf_input.id);
                    idrf_input.src2 = find_existing_prf(idrf_input.src2, idrf_input.id);

                    printf("\n Decode stage : \t %s P%d P%d %d", idrf_input.opcode, idrf_input.src1, idrf_input.src2, idrf_input.literal);

                    rob[rob_add] = idrf_input;
                    rob_add++;

                    lsq[lsq_add] = idrf_input;
                    lsq_add++;
                  
                    iqueue[iq_add] = idrf_input;
                    idrf_input = nop;
                    iq_add++;                               // can instead use in IQUEUE function
                }
                else
                    printf("\n Decode stage : \t %s R%d R%d %d stalled", idrf_input.opcode, idrf_input.src1, idrf_input.src2, idrf_input.literal);
        }
        //else if for other ins
    }
    else 
        printf("\n Decode stage : \t idle");
    //Add some mechanism to detect if the ROB or the IQ is full.
    if(abs(iq_add - iq_rem) >= 15)
        iq_full = 1;
    else
        iq_full = 0;
    if(abs(rob_add - rob_com) >= 31)
        rob_full = 1;
    else 
        rob_full = 0;
    if(abs(lsq_add - lsq_rem) >= 31)
        lsq_full = 1;
    else
        lsq_full = 0;
    //setting *_add to proper place
    if(iq_add > 15){
        for(int i = 0; i < 16; i++){
            if(!(strcmp(iqueue[i].opcode, "nop")))
               iq_add = i;
        }
    }
    if(lsq_add > 31){
        for(int i = 0; i < 32; i++){
            if(!(strcmp(lsq[i].opcode, "nop")))
               lsq_add = i;
        }
    }
    if(rob_add > 31){
        for(int i = 0; i < 32; i++){
            if(!(strcmp(rob[i].opcode, "nop")))
               rob_add = i;
        }
    }
}

//INT Functional Unit
void execute(){
    if((strcmp(iex_input.opcode, "nop"))){
        if (!(strcmp(iex_input.opcode, "MOVC"))){
            printf("\n Execute stage : \t %s P%d %d", iex_input.opcode, iex_input.dest, iex_input.literal);
            iex_input.result = iex_input.literal;
            prf_register_File[iex_input.dest].status = VALID;
            prf_register_File[iex_input.dest].value = iex_input.result;
            //Forward the result to rob entry using instruction id
            for (int i = 0; i != 32; i++){
                if(rob[i].id == iex_input.id){
                    rob[i].result = iex_input.result;
                    rob[i].status = VALID;
                }
            }
            iex_input = nop;
        }
        else if (!(strcmp(iex_input.opcode, "ADD"))){
            printf("\n Execute stage : \t %s P%d P%d P%d", iex_input.opcode, iex_input.dest, iex_input.src1, iex_input.src2);
            iex_input.result = prf_register_File[iex_input.src1].value + prf_register_File[iex_input.src2].value;

            prf_register_File[iex_input.dest].status = VALID;
            prf_register_File[iex_input.dest].value = iex_input.result;
            last_arithmetic_instr = iex_input.id;
            last_arithmetic_result = iex_input.result;
            //Forward the result to rob entry using instruction id
            for (int i = 0; i != 32; i++){
                if(rob[i].id == iex_input.id){
                    rob[i].result = iex_input.result;
                    rob[i].status = VALID;
                }
            }
            iex_input = nop;
        }
        else if (!(strcmp(iex_input.opcode, "SUB"))){
            printf("\n Execute stage : \t %s P%d P%d P%d", iex_input.opcode, iex_input.dest, iex_input.src1, iex_input.src2);
            iex_input.result = prf_register_File[iex_input.src1].value - prf_register_File[iex_input.src2].value;

            prf_register_File[iex_input.dest].status = VALID;
            prf_register_File[iex_input.dest].value = iex_input.result;
            last_arithmetic_instr = iex_input.id;
            last_arithmetic_result = iex_input.result;
            //Forward the result to rob entry using instruction id
            for (int i = 0; i != 32; i++){
                if(rob[i].id == iex_input.id){
                    rob[i].result = iex_input.result;
                    rob[i].status = VALID;
                }
            }
            iex_input = nop;
        }
        else if (!(strcmp(iex_input.opcode, "AND"))){
            printf("\n Execute stage : \t %s P%d P%d P%d", iex_input.opcode, iex_input.dest, iex_input.src1, iex_input.src2);
            iex_input.result = prf_register_File[iex_input.src1].value & prf_register_File[iex_input.src2].value;

            prf_register_File[iex_input.dest].status = VALID;
            prf_register_File[iex_input.dest].value = iex_input.result;
            last_arithmetic_instr = iex_input.id;
            last_arithmetic_result = iex_input.result;
            //Forward the result to rob entry using instruction id
            for (int i = 0; i != 32; i++){
                if(rob[i].id == iex_input.id){
                    rob[i].result = iex_input.result;
                    rob[i].status = VALID;
                }
            }
            iex_input = nop;
        }
        else if (!(strcmp(iex_input.opcode, "OR"))){
            printf("\n Execute stage : \t %s P%d P%d P%d", iex_input.opcode, iex_input.dest, iex_input.src1, iex_input.src2);
            iex_input.result = prf_register_File[iex_input.src1].value | prf_register_File[iex_input.src2].value;

            prf_register_File[iex_input.dest].status = VALID;
            prf_register_File[iex_input.dest].value = iex_input.result;
            last_arithmetic_instr = iex_input.id;
            last_arithmetic_result = iex_input.result;
            //Forward the result to rob entry using instruction id
            for (int i = 0; i != 32; i++){
                if(rob[i].id == iex_input.id){
                    rob[i].result = iex_input.result;
                    rob[i].status = VALID;
                }
            }
            iex_input = nop;
        }
        else if (!(strcmp(iex_input.opcode, "EX-OR"))){
            printf("\n Execute stage : \t %s P%d P%d P%d", iex_input.opcode, iex_input.dest, iex_input.src1, iex_input.src2);
            iex_input.result = prf_register_File[iex_input.src1].value ^ prf_register_File[iex_input.src2].value;

            prf_register_File[iex_input.dest].status = VALID;
            prf_register_File[iex_input.dest].value = iex_input.result;
            last_arithmetic_instr = iex_input.id;
            last_arithmetic_result = iex_input.result;
            //Forward the result to rob entry using instruction id
            for (int i = 0; i != 32; i++){
                if(rob[i].id == iex_input.id){
                    rob[i].result = iex_input.result;
                    rob[i].status = VALID;
                }
            }
            iex_input = nop;
        }
        else if (!(strcmp(iex_input.opcode, "JUMP"))){
            printf("\n Execute stage : \t %s P%d %d", iex_input.opcode, iex_input.src1, iex_input.literal);
            iex_input.result = (prf_register_File[iex_input.src1].value + iex_input.literal - 4000)/4;

            //Forward the result to rob entry using instruction id
            for (int i = 0; i != 32; i++){
                if(rob[i].id == iex_input.id){
                    rob[i].result = iex_input.result;
                    rob[i].status = VALID;
                }
            }
            bflag = 1;
            iex_input = nop;
        }
        else if (!(strcmp(iex_input.opcode, "JAL"))){
            printf("\n Execute stage : \t %s P%d P%d %d", iex_input.opcode, iex_input.dest, iex_input.src1, iex_input.literal);
            iex_input.result = (prf_register_File[iex_input.src1].value + iex_input.literal - 4000)/4;

            //Forward the result to rob entry using instruction id
            for (int i = 0; i != 32; i++){
                if(rob[i].id == iex_input.id){
                    rob[i].result = iex_input.result;
                    rob[i].status = VALID;
                }
            }
            bflag = 1;
            iex_input = nop;
        }
        else if (!(strcmp(iex_input.opcode, "BNZ"))){
            printf("\n Execute stage : \t %s %d", iex_input.opcode, iex_input.literal);
            iex_input.result = (iex_input.index + (iex_input.literal/4));

            //Forward the result to rob entry using instruction id
            for (int i = 0; i != 32; i++){
                if(rob[i].id == iex_input.id){
                    rob[i].result = iex_input.result;
                    rob[i].branch = iex_input.branch;
                    rob[i].status = VALID;
                }
            }
            iex_input = nop;
        }
        else if (!(strcmp(iex_input.opcode, "BZ"))){
            printf("\n Execute stage : \t %s %d", iex_input.opcode, iex_input.literal);
            iex_input.result = (iex_input.index + (iex_input.literal/4));

            //Forward the result to rob entry using instruction id
            for (int i = 0; i != 32; i++){
                if(rob[i].id == iex_input.id){
                    rob[i].result = iex_input.result;
                    rob[i].branch = iex_input.branch;
                    rob[i].status = VALID;
                }
            }
            iex_input = nop;
        }
        else if (!(strcmp(iex_input.opcode, "LOAD"))){
            printf("\n Execute stage : \t %s P%d P%d %d", iex_input.opcode, iex_input.dest, iex_input.src1, iex_input.literal);
            iex_input.address = (prf_register_File[iex_input.src1].value + iex_input.literal)/4;

            //Forward the result to rob entry for LOADs using instruction id from memory stage only
            // Forward the address calculated here to the LSQ entry using ins id and loop, and make its status valid.
            for (int i = 0; i < 32; i++){
                if (lsq[i].id == iex_input.id){
                    lsq[i].address = iex_input.address;
                    lsq[i].status = VALID;
                }
            }
            iex_input = nop;
        }
        else if (!(strcmp(iex_input.opcode, "STORE"))){
            printf("\n Execute stage : \t %s P%d P%d %d", iex_input.opcode, iex_input.src1, iex_input.src2, iex_input.literal);
            iex_input.address = (prf_register_File[iex_input.src2].value + iex_input.literal)/4;

            //Forward the result to rob entry for LOADs using instruction id from memory stage only
            // Forward the address calculated here to the LSQ entry using ins id and loop, and make its status valid.
            for (int i = 0; i < 32; i++){
                if (lsq[i].id == iex_input.id){
                    lsq[i].address = iex_input.address;
                    lsq[i].status = VALID;
                }
            }
            iex_input = nop;
        }
        //else if for remaining instructions  
    
        //done//forward results from execution to the ROB using loops and the ins id. if id matches copy the result into rob[i].result and mark ins status valid
        //done//do the same for iq but forward values of the prf_register file in the iqueue function only
        //done//use for loop for finding the ins id of each ins // use rob[i].status for commiting
    }
    else
        printf("\n Execute stage    : \t idle");
}

//MEM stage
void memory(){

    if((strcmp(imem_input.opcode, "nop"))){
        if(!(strcmp(imem_input.opcode, "LOAD"))){
            printf("\n Memory stage : \t %s P%d P%d %d", imem_input.opcode, imem_input.dest, imem_input.src1, imem_input.literal);
            imem_input.result = data_Memory[imem_input.address];
            prf_register_File[imem_input.dest].value = imem_input.result;
            prf_register_File[imem_input.dest].status = VALID;

            for (int i = 0; i < 32; i++){
                if(rob[i].id == imem_input.id){
                    rob[i].result = imem_input.result;
                    rob[i].status = VALID;
                }
            }
            imem_input = nop;
        }
        else if(!(strcmp(imem_input.opcode, "STORE"))){
            printf("\n Memory stage : \t %s P%d P%d %d", imem_input.opcode, imem_input.src1, imem_input.src2, imem_input.literal);
            data_Memory[imem_input.address] = prf_register_File[imem_input.src1].value;

            for (int i = 0; i < 32; i++){
                if(rob[i].id == imem_input.id){
                    rob[i].status = VALID;
                }
            }
            imem_input = nop;
        }
    }
    printf("\n Memory stage     : \t idle");
}

//Mul1 function Unit
void mul1(){
    if((strcmp(imul1_input.opcode, "nop"))){
        if (!(strcmp(imul2_input.opcode, "nop"))){
            printf("\n Mul1 stage : \t\t %s P%d P%d P%d", imul1_input.opcode, imul1_input.dest, imul1_input.src1, imul1_input.src2);
            imul2_input = imul1_input;
            imul1_input = nop;
        }
        else
            printf("\n Mul1 stage : \t\t %s P%d P%d P%d stalled", imul1_input.opcode, imul1_input.dest, imul1_input.src1, imul1_input.src2);
    }
}

//Mul2 Function Unit
void mul2(){
    if((strcmp(imul2_input.opcode, "nop"))){
        printf("\n Mul2 stage : \t\t %s P%d P%d P%d", imul2_input.opcode, imul2_input.dest, imul2_input.src1, imul2_input.src2);
        imul2_input.result = prf_register_File[imul2_input.src1].value * prf_register_File[imul2_input.src2].value;

        prf_register_File[imul2_input.dest].status = VALID;
        prf_register_File[imul2_input.dest].value = imul2_input.result;
        last_arithmetic_instr = imul2_input.id;
        last_arithmetic_result = imul2_input.result;
        //Forward the result to rob entry using instruction id
        for (int i = 0; i != 32; i++){
            if(rob[i].id == imul2_input.id){
                rob[i].result = imul2_input.result;
                rob[i].status = VALID;
            }
        }
//printf("\n******** LAR %ld and LAI %d teting:  id: %d ", last_arithmetic_result, last_arithmetic_instr, imul2_input.id);
        imul2_input = nop;
        if (!(strcmp(imul1_input.opcode, "nop")))
            mflag = 0;
    }
}

//DU1 Function Unit
void div1(){
    if((strcmp(idiv1_input.opcode, "nop"))){
        if (!(strcmp(idiv1_input.opcode, "DIV"))){
            if (!(strcmp(idiv2_input.opcode, "nop"))){
                printf("\n DU1 stage : \t\t %s P%d P%d P%d", idiv1_input.opcode, idiv1_input.dest, idiv1_input.src1, idiv1_input.src2);
                idiv2_input = idiv1_input;
                idiv1_input = nop;
            }
            else
                printf("\n DU1 stage : \t\t %s P%d P%d P%d stalled", idiv1_input.opcode, idiv1_input.dest, idiv1_input.src1, idiv1_input.src2); 
        }
        else if (!(strcmp(idiv1_input.opcode, "HALT"))){
            if (!(strcmp(idiv2_input.opcode, "nop"))){
                printf("\n DU1 stage : \t\t %s ", idiv1_input.opcode);
                idiv2_input = idiv1_input;
                idiv1_input = nop;
            }
            else
                printf("\n DU1 stage : \t\t %s stalled", idiv1_input.opcode);
        }
    }
}

//DU2 Function Unit
void div2(){
    if((strcmp(idiv2_input.opcode, "nop"))){
        if (!(strcmp(idiv2_input.opcode, "DIV"))){
            if (!(strcmp(idiv3_input.opcode, "nop"))){
                printf("\n DU2 stage : \t\t %s P%d P%d P%d", idiv2_input.opcode, idiv2_input.dest, idiv2_input.src1, idiv2_input.src2);
                idiv3_input = idiv2_input;
                idiv2_input = nop;
            }
            else
                printf("\n DU2 stage : \t\t %s P%d P%d P%d stalled", idiv2_input.opcode, idiv2_input.dest, idiv2_input.src1, idiv2_input.src2);
        }
        else if (!(strcmp(idiv2_input.opcode, "HALT"))){
            if (!(strcmp(idiv3_input.opcode, "nop"))){
                printf("\n DU2 stage : \t\t %s", idiv2_input.opcode);
                idiv3_input = idiv2_input;
                idiv2_input = nop;
            }
            else
                printf("\n DU2 stage : \t\t %s stalled", idiv2_input.opcode);
        }
    }
}

//DU3 Function Unit
void div3(){
    if((strcmp(idiv3_input.opcode, "nop"))){
        if (!(strcmp(idiv3_input.opcode, "DIV"))){
            if (!(strcmp(idiv4_input.opcode, "nop"))){
                printf("\n DU3 stage    : \t %s P%d P%d P%d", idiv3_input.opcode, idiv3_input.dest, idiv3_input.src1, idiv3_input.src2);
                idiv4_input = idiv3_input;
                idiv3_input = nop;
            }
            else
                printf("\n DU3 stage    : \t %s P%d P%d P%d stalled", idiv3_input.opcode, idiv3_input.dest, idiv3_input.src1, idiv3_input.src2);
        }
        else if (!(strcmp(idiv3_input.opcode, "HALT"))){
            if (!(strcmp(idiv4_input.opcode, "nop"))){
                printf("\n DU4 stage : \t\t %s", idiv3_input.opcode);
                idiv4_input = idiv3_input;
                idiv3_input = nop;
            }
            else
                printf("\n DU3 stage : \t\t %s stalled", idiv3_input.opcode);
        }
    }
}

//DU4 Function Unit
void div4(){
    if((strcmp(idiv4_input.opcode, "nop"))){
        if (!(strcmp(idiv4_input.opcode, "DIV"))){
            printf("\n DU4 stage : \t\t %s P%d P%d P%d", idiv4_input.opcode, idiv4_input.dest, idiv4_input.src1, idiv4_input.src2);
            idiv4_input.result = prf_register_File[idiv4_input.src1].value / prf_register_File[idiv4_input.src2].value;

            prf_register_File[idiv4_input.dest].status = VALID;
            prf_register_File[idiv4_input.dest].value = idiv4_input.result;
            last_arithmetic_instr = idiv4_input.id;
            last_arithmetic_result = idiv4_input.result;
            //Forward the result to rob entry using instruction id
            for (int i = 0; i != 32; i++){
                if(rob[i].id == idiv4_input.id){
                    rob[i].result = idiv4_input.result;
                    rob[i].status = VALID;
                }
            }
            idiv4_input = nop;
            if (!(strcmp(idiv1_input.opcode, "nop")) && !(strcmp(idiv2_input.opcode, "nop")) && !(strcmp(idiv3_input.opcode, "nop")))
                dflag = 0;
        }
        else if (!(strcmp(idiv4_input.opcode, "HALT"))){
            printf("\n DU4 stage : \t\t %s ", idiv4_input.opcode);

            //Forward the result to rob entry using instruction id
            for (int i = 0; i != 32; i++){
                if(rob[i].id == idiv4_input.id){
                    rob[i].status = VALID;
                }
            }
            idiv4_input = nop;
            if (!(strcmp(idiv1_input.opcode, "nop")) && !(strcmp(idiv2_input.opcode, "nop")) && !(strcmp(idiv3_input.opcode, "nop")))
                dflag = 0;
        }
    }
}

//Initialize the ROB, PRF, ARF and Issue Queue
void intialize(){
    for (int i = 0; i < 16; i++){
        arf_register_File[i].status = VALID;
        arf_register_File[i].value = 0;
        arf_register_File[i].ins_id = 0;
    }
    for (int i = 0; i < 31; i++){
        prf_register_File[i].value = 0;
        prf_register_File[i].ins_id = -1;
        prf_register_File[i].status = 0;
        prf_register_File[i].busy = 0;
        prf_register_File[i].old_instance = 0;
    }
    for (int i = 0; i < 16; i++){
        iqueue[i] = nop;
    }
    for (int i = 0; i < 32; i++){
        rob[i] = nop;
    }
    for (int i = 0; i < 32; i++){
        lsq[i] = nop;
    }
}

//Display the contents of the ARF, PRF and Data memory
void display(){
    printf("\n---------Architecture Register File-----------\n");
    for(int i =0; i<=16; i++){
        printf("R%d = %ld \n", i, arf_register_File[i].value);
    }
    printf("\n---------Physical Register File-----------\n");
    for(int i =0; i<=16; i++){
        printf("P%d = %ld \n", i, prf_register_File[i].value);
    }
    printf("\n---------Data Memory-------------\n");
    for(int i =0; i < 100; i++){
        printf("data_mem[%d] = %ld \n", i*4, data_Memory[i]);
    }
}

//Check for free Physical Register
int prf_available(){
    for (int i = 0; i < 32;){
        if (prf_register_File[i].busy == 0)
            return 1;
        else
            i++;
    }
    return 0;
}

//for renaming the destination register
int find_new_prf(){
    for (int i = 0; i < 32;){
        if (prf_register_File[i].busy == 0){
            return i;
        }
        else {
            i++;
        }
    }
    return -1;
}

//for renaming the source registers corresponding to the pth arf
int find_existing_prf(int p, int q){
    for (int i = 0; i < 32; ){
        if ((prf_register_File[i].arf_id == p) && (prf_register_File[i].busy == 1) && (prf_register_File[i].old_instance == 0)){
            prf_register_File[i].ins_id = q;
            return i;
        }
        else
            i++;
    }
    return -1;
}

//If new instance of a Architecture register is generated, mark the old_instance = 1 for earlier physical reg
void old_instance_prf(int p){
    for (int i = 0; i < 32; ){
        if ((prf_register_File[i].arf_id == p) && (prf_register_File[i].busy == 1)){
            prf_register_File[i].old_instance = 1;
            break;  
        }
        else
            i++;
    }
}

    //done //in iq check for the src reg and when valid issue.
    //no need //apply one loop to get the values of the corresponding physical registers. and then set the ins status to valid
    //issue only when the status of ins is valid 

 
//ISSUE QUEUE function    
void iq(){
/*for(int i = 0; i < 16; i++){
    printf("\n**** i: %d opcode : %s ******", i, iqueue[i].opcode);
}*/

    if((strcmp(iqueue[iq_rem].opcode, "nop"))){
        if(!(strcmp(iex_input.opcode, "nop"))){
            if(!(strcmp(iqueue[iq_rem].opcode, "MOVC"))){

                printf("\n IQ      : \t\t %s P%d %d", iqueue[iq_rem].opcode, iqueue[iq_rem].dest, iqueue[iq_rem].literal);
                iex_input = iqueue[iq_rem];
                iqueue[iq_rem] = nop;
                iq_rem++;
            }
            else if(!(strcmp(iqueue[iq_rem].opcode, "ADD"))){
//printf("\n******* ex opcode : %s ******", iex_input.opcode);
                printf("\n IQ      : \t\t %s P%d P%d P%d", iqueue[iq_rem].opcode, iqueue[iq_rem].dest, iqueue[iq_rem].src1, iqueue[iq_rem].src2);
                if (prf_register_File[iqueue[iq_rem].src1].status == VALID && prf_register_File[iqueue[iq_rem].src2].status == VALID){
                    last_arithmetic_ind = iqueue[iq_rem].id; //compare last_arithmetic_ind with last_aritmetic_ins(updated in ex), for BRANCH.
                    iex_input = iqueue[iq_rem];
                    iqueue[iq_rem] = nop;
                    iq_rem++;
                }
            }
            else if(!(strcmp(iqueue[iq_rem].opcode, "SUB"))){

                printf("\n IQ      : \t\t %s P%d P%d P%d", iqueue[iq_rem].opcode, iqueue[iq_rem].dest, iqueue[iq_rem].src1, iqueue[iq_rem].src2);
                if (prf_register_File[iqueue[iq_rem].src1].status == VALID && prf_register_File[iqueue[iq_rem].src2].status == VALID){
                    last_arithmetic_ind = iqueue[iq_rem].id;
                    iex_input = iqueue[iq_rem];
                    iqueue[iq_rem] = nop;
                    iq_rem++;
                }
            }
            else if(!(strcmp(iqueue[iq_rem].opcode, "AND"))){

                printf("\n IQ      : \t\t %s P%d P%d P%d", iqueue[iq_rem].opcode, iqueue[iq_rem].dest, iqueue[iq_rem].src1, iqueue[iq_rem].src2);
                if (prf_register_File[iqueue[iq_rem].src1].status == VALID && prf_register_File[iqueue[iq_rem].src2].status == VALID){
                    last_arithmetic_ind = iqueue[iq_rem].id;
                    iex_input = iqueue[iq_rem];
                    iqueue[iq_rem] = nop;
                    iq_rem++;
                }
            }
            else if(!(strcmp(iqueue[iq_rem].opcode, "OR"))){

                printf("\n IQ      : \t\t %s P%d P%d P%d", iqueue[iq_rem].opcode, iqueue[iq_rem].dest, iqueue[iq_rem].src1, iqueue[iq_rem].src2);
                if (prf_register_File[iqueue[iq_rem].src1].status == VALID && prf_register_File[iqueue[iq_rem].src2].status == VALID){
                    last_arithmetic_ind = iqueue[iq_rem].id;
                    iex_input = iqueue[iq_rem];
                    iqueue[iq_rem] = nop;
                    iq_rem++;
                }
            }
            else if(!(strcmp(iqueue[iq_rem].opcode, "EX-OR"))){

                printf("\n IQ      : \t\t %s P%d P%d P%d", iqueue[iq_rem].opcode, iqueue[iq_rem].dest, iqueue[iq_rem].src1, iqueue[iq_rem].src2);
                if (prf_register_File[iqueue[iq_rem].src1].status == VALID && prf_register_File[iqueue[iq_rem].src2].status == VALID){
                    last_arithmetic_ind = iqueue[iq_rem].id;
                    iex_input = iqueue[iq_rem];
                    iqueue[iq_rem] = nop;
                    iq_rem++;
                }
            }
            else if(!(strcmp(iqueue[iq_rem].opcode, "JUMP"))){

                printf("\n IQ      : \t\t %s P%d %d", iqueue[iq_rem].opcode, iqueue[iq_rem].src1, iqueue[iq_rem].literal);
                if (prf_register_File[iqueue[iq_rem].src1].status == VALID){
                    iex_input = iqueue[iq_rem];
                    iqueue[iq_rem] = nop;
                    iq_rem++;
                    bflag = 1;
                }
            }
            else if(!(strcmp(iqueue[iq_rem].opcode, "JAL"))){

                printf("\n IQ      : \t\t %s P%d P%d %d", iqueue[iq_rem].opcode, iqueue[iq_rem].dest, iqueue[iq_rem].src1, iqueue[iq_rem].literal);
                if (prf_register_File[iqueue[iq_rem].src1].status == VALID){
                    iex_input = iqueue[iq_rem];
                    iqueue[iq_rem] = nop;
                    iq_rem++;
                    bflag = 1;
                }
            }
            //problem will occur with rolling back since the dest prf will be invalidated 
            //Add mechanism to flush all the ins with id > bnz.id and revert the changes made by these instructions
            else if(!(strcmp(iqueue[iq_rem].opcode, "BNZ"))){
//printf("\n********LAI : %d & LAINS: %d ", last_arithmetic_ind, last_arithmetic_instr);
                if (last_arithmetic_result != 0 && (last_arithmetic_ind == (last_arithmetic_instr + 1))){
                    printf("\n IQ      : \t\t %s %d", iqueue[iq_rem].opcode, iqueue[iq_rem].literal);
                    iqueue[iq_rem].branch = 1;

                    for (int i = 0; i < 16; i++){
                        if(iqueue[i].id > iqueue[iq_rem].id){
                            if(!(strcmp(iqueue[i].opcode, "JUMP")))
                                jflag = 0;
                            else if(!(strcmp(iqueue[i].opcode, "JAL")))
                                jalflag = 0;
                            else if(!(strcmp(iqueue[i].opcode, "HALT")))
                                hflag = 0;
                            for (int j = 0; j < 32; j++){
                                if (j == iqueue[i].dest){
                                    prf_register_File[j].status = VALID;
                                    if (prf_register_File[j].old_instance == 0){
                                        prf_register_File[j].busy = 1;    
                                    }
                                    else{ 
                                        prf_register_File[j].busy = 0;
                                        prf_register_File[j].old_instance = 0;
                                    }    
                                }
                            }
                            idrf_input = if_input = nop;
                            bzflag = 1;
                            iqueue[i] = nop;
                        }
                    }
                    for (int i = 0; i < 32; i++)
                    iex_input = iqueue[iq_rem];
                    iqueue[iq_rem] = nop;
                    iq_rem = iq_add;
                }
                else if (last_arithmetic_result == 0 && (last_arithmetic_ind == (last_arithmetic_instr + 1))){
                    printf("\n IQ      : \t\t %s %d", iqueue[iq_rem].opcode, iqueue[iq_rem].literal);
                    iqueue[iq_rem].branch = 0;
                    iex_input = iqueue[iq_rem];
                    iqueue[iq_rem] = nop;
                    iq_rem++;
                }
                //else
            }
            else if(!(strcmp(iqueue[iq_rem].opcode, "BZ"))){

                if (last_arithmetic_result == 0 && (last_arithmetic_ind == (last_arithmetic_instr + 1))) {
                    printf("\n IQ      : \t\t %s %d", iqueue[iq_rem].opcode, iqueue[iq_rem].literal);
                    iqueue[iq_rem].branch = 1;
                    for (int i = 0; i < 16; i++){
                        if(iqueue[i].id > iqueue[iq_rem].id){
                            if(!(strcmp(iqueue[i].opcode, "JUMP")))
                                jflag = 0;
                            else if(!(strcmp(iqueue[i].opcode, "JAL")))
                                jalflag = 0;
                            else if(!(strcmp(iqueue[i].opcode, "HALT")))
                                hflag = 0;
                            for (int j = 0; j < 32; j++){
                                if (j == iqueue[i].dest && iqueue[i].dest != -1){
                                    prf_register_File[j].status = VALID;
                                    prf_register_File[j].busy = 0;
                                    prf_register_File[j].old_instance = 0;
                                }
                                if (prf_register_File[j].old_instance == 1 && prf_register_File[j].ins_id == iqueue[i].id){
                                    prf_register_File[j].busy = 1;
                                    prf_register_File[j].old_instance = 0;    
                                }
                            }
                            idrf_input = if_input = nop;
                            bzflag = 1;
                            iqueue[i] = nop;
                        }
                    }
                    iex_input = iqueue[iq_rem];
                    iqueue[iq_rem] = nop;
                    iq_rem = iq_add;
                }
                else if (last_arithmetic_result != 0 && (last_arithmetic_ind == (last_arithmetic_instr + 1))){
                    iqueue[iq_rem].branch = 0;
                    iex_input = iqueue[iq_rem];
                    iqueue[iq_rem] = nop;
                    iq_rem++;
                }
                //else
            }
            else if(!(strcmp(iqueue[iq_rem].opcode, "LOAD"))){

                if (prf_register_File[iqueue[iq_rem].src1].status == VALID){
                    printf("\n IQ      : \t\t %s P%d P%d %d", iqueue[iq_rem].opcode, iqueue[iq_rem].dest, iqueue[iq_rem].src1, iqueue[iq_rem].literal);
                    iex_input = iqueue[iq_rem];
                    iqueue[iq_rem] = nop;
                    iq_rem++;
                }
            }
            else if(!(strcmp(iqueue[iq_rem].opcode, "STORE"))){

                if (prf_register_File[iqueue[iq_rem].src2].status == VALID){
                    printf("\n IQ      : \t\t %s P%d P%d %d", iqueue[iq_rem].opcode, iqueue[iq_rem].src1, iqueue[iq_rem].src2, iqueue[iq_rem].literal);
                    iex_input = iqueue[iq_rem];
                    iqueue[iq_rem] = nop;
                    iq_rem++;
                }
            }
            //else if for other ins //done but need to change //if (prf_register_File[iqueue[iq_rem].src1].status == VALID) for ins with sources 
        }
        if(!(strcmp(imul1_input.opcode, "nop"))){
            if(!(strcmp(iqueue[iq_rem].opcode, "MUL"))){

                printf("\n IQ      : \t\t %s P%d P%d P%d", iqueue[iq_rem].opcode, iqueue[iq_rem].dest, iqueue[iq_rem].src1, iqueue[iq_rem].src2);
                if (prf_register_File[iqueue[iq_rem].src1].status == VALID && prf_register_File[iqueue[iq_rem].src2].status == VALID){
                    last_arithmetic_ind = iqueue[iq_rem].id;
                    imul1_input = iqueue[iq_rem];
                    iqueue[iq_rem] = nop;
                    iq_rem++;
                    mflag = 1;
                }
            }
        }
        if(!(strcmp(idiv1_input.opcode, "nop"))){
            if(!(strcmp(iqueue[iq_rem].opcode, "DIV"))){

                if (prf_register_File[iqueue[iq_rem].src1].status == VALID && prf_register_File[iqueue[iq_rem].src2].status == VALID){
                    printf("\n IQ      : \t\t %s P%d P%d P%d", iqueue[iq_rem].opcode, iqueue[iq_rem].dest, iqueue[iq_rem].src1, iqueue[iq_rem].src2);
                    last_arithmetic_ind = iqueue[iq_rem].id;
                    idiv1_input = iqueue[iq_rem];
                    iqueue[iq_rem] = nop;
                    iq_rem++;
                    dflag = 1;
                }
                else 
                    printf("\n IQ      : \t\t %s P%d P%d P%d stalled", iqueue[iq_rem].opcode, iqueue[iq_rem].dest, iqueue[iq_rem].src1, iqueue[iq_rem].src2);
            }
            else if(!(strcmp(iqueue[iq_rem].opcode, "HALT"))){

                printf("\n IQ      : \t\t %s ", iqueue[iq_rem].opcode);
                idiv1_input = iqueue[iq_rem];
                iqueue[iq_rem] = nop;
                iq_rem++;
                hflag = 1;
            }
        } 
    }
    if(iq_rem > 15){
        for(int i = 0; i < 16; i++){
            if((strcmp(rob[i].opcode, "nop")))
              iq_rem = i;
        }
    }
    if (iq_add == iq_rem){
        if((strcmp(iqueue[iq_rem].opcode, "nop")))
            iq_full = 1;
    }
}

//LOAD STORE QUEUE WITH LOADS BYPASSING EARLIER STORES WITH SAME ADDRESS
void LSQ(){
    // LOAD Bypassing
    for (int i = 0; i < 32; i++){
        if(!(strcmp(lsq[i].opcode, "LOAD")) && lsq[i].status == VALID){
            for (int j = 0; j < 32; j++){
                if(!(strcmp(lsq[j].opcode, "STORE")) && (lsq[j].status == VALID) && (lsq[j].id < lsq[i].id)){
                    if (lsq[j].address == lsq[i].address){
                        prf_register_File[lsq[i].dest].value = prf_register_File[lsq[j].src1].value;
                        prf_register_File[lsq[i].dest].status = VALID;

                        for (int k = 0; k < 32; k++){
                            if (rob[k].id == lsq[i].id){
                                rob[k].result = prf_register_File[lsq[i].dest].value;
                                rob[k].status = VALID;
                            }
                        }
                        lsq[i] = nop;

                        for (int p = i; ((p+1) != 32) && (strcmp(lsq[j].opcode, "nop")); p++){
                            lsq[p] = lsq[p+1];
                            lsq_add--;
                        }

                        /*for (int l = i; (l+1) != lsq_rem; l++){
                            if (l+1 == 32 && ((strcmp(lsq[0].opcode, "nop")))){
                                l = 0;
                            }
                            if(!(strcmp(lsq[].opcode, "LOAD")))
                        }*/
                        //lsq[i] = lsq[i+1];
                        //After removing this entry from LSQ, try to copy the next instructions one step back and
                        //do lsq_add-- add checking conditions if this goes negative or while copying keep track since its not a
                        //circular queue
                    }
                }
            }
        }
    }

    if((strcmp(lsq[lsq_rem].opcode, "nop"))){
        if(!(strcmp(imem_input.opcode, "nop"))){
            if(!(strcmp(lsq[lsq_rem].opcode, "LOAD"))){
                if(lsq[lsq_rem].status == VALID){
                    printf("\n LSQ     : \t\t %s P%d P%d %d", lsq[lsq_rem].opcode, lsq[lsq_rem].dest, lsq[lsq_rem].src1, lsq[lsq_rem].literal);
                    imem_input = lsq[lsq_rem];
                    lsq[lsq_rem] = nop;
                    lsq_rem++;
                }
                else
                    printf("\n LSQ     : \t\t %s P%d P%d %d stalled", lsq[lsq_rem].opcode, lsq[lsq_rem].dest, lsq[lsq_rem].src1, lsq[lsq_rem].literal);
            }
            if(!(strcmp(lsq[lsq_rem].opcode, "STORE"))){
                if(prf_register_File[lsq[lsq_rem].src1].status == VALID && lsq[lsq_rem].status == VALID){
                    printf("\n LSQ     : \t\t %s P%d P%d %d", lsq[lsq_rem].opcode, lsq[lsq_rem].src1, lsq[lsq_rem].src2, lsq[lsq_rem].literal);
                    imem_input = lsq[lsq_rem];
                    lsq[lsq_rem] = nop;
                    lsq_rem++;
                }
                else
                    printf("\n LSQ     : \t\t %s P%d P%d %d stalled", lsq[lsq_rem].opcode, lsq[lsq_rem].src1, lsq[lsq_rem].src2, lsq[lsq_rem].literal);
            }
        }
    }
    if (lsq_rem > 31)
        lsq_rem = 0;
    if (lsq_add == lsq_rem){
        if((strcmp(lsq[lsq_add].opcode, "nop")))
            lsq_full = 1;
    }
}

//in rob just do the commiting of results to the arf as the result will be copied from execution state
// if ins.status is valid then only commit

//Re-Order Buffer
void ROB(){
    int i = rob_com;
    
    if((strcmp(rob[i].opcode, "nop"))){
        if(!(strcmp(rob[i].opcode, "MOVC"))){
            printf("\n ROB      : \t\t %s R%d %d", rob[i].opcode, rob[i].dest, rob[i].literal);
            if (rob[i].status == VALID){
                arf_register_File[rob[i].dest].value = rob[i].result;
                rob_com++;
                rob[i] = nop;
            }
        }
        else if(!(strcmp(rob[i].opcode, "ADD"))){
            printf("\n ROB      : \t\t %s R%d P%d P%d", rob[i].opcode, rob[i].dest, rob[i].src1, rob[i].src2);
            if (rob[i].status == VALID){
                arf_register_File[rob[i].dest].value = rob[i].result;
                if (prf_register_File[rob[i].src1].ins_id == rob[i].id && prf_register_File[rob[i].src1].old_instance == 1){
                    prf_register_File[rob[i].src1].busy = 0;
                    prf_register_File[rob[i].src1].old_instance = 0;
                }
                if (prf_register_File[rob[i].src2].ins_id == rob[i].id && prf_register_File[rob[i].src2].old_instance == 1){
                    prf_register_File[rob[i].src2].busy = 0;
                    prf_register_File[rob[i].src2].old_instance = 0;
                }
                rob_com++;
                rob[i] = nop;
            }
        }
        else if(!(strcmp(rob[i].opcode, "SUB"))){
            printf("\n ROB      : \t\t %s R%d P%d P%d", rob[i].opcode, rob[i].dest, rob[i].src1, rob[i].src2);
            if (rob[i].status == VALID){
                arf_register_File[rob[i].dest].value = rob[i].result;
                if (prf_register_File[rob[i].src1].ins_id == rob[i].id && prf_register_File[rob[i].src1].old_instance == 1){
                    prf_register_File[rob[i].src1].busy = 0;
                    prf_register_File[rob[i].src1].old_instance = 0;
                }
                if (prf_register_File[rob[i].src2].ins_id == rob[i].id && prf_register_File[rob[i].src2].old_instance == 1){
                    prf_register_File[rob[i].src2].busy = 0;
                    prf_register_File[rob[i].src2].old_instance = 0;
                }
                rob_com++;
                rob[i] = nop;
            }
        }
        else if(!(strcmp(rob[i].opcode, "AND"))){
            printf("\n ROB      : \t\t %s R%d P%d P%d", rob[i].opcode, rob[i].dest, rob[i].src1, rob[i].src2);
            if (rob[i].status == VALID){
                arf_register_File[rob[i].dest].value = rob[i].result;
                if (prf_register_File[rob[i].src1].ins_id == rob[i].id && prf_register_File[rob[i].src1].old_instance == 1){
                    prf_register_File[rob[i].src1].busy = 0;
                    prf_register_File[rob[i].src1].old_instance = 0;
                }
                if (prf_register_File[rob[i].src2].ins_id == rob[i].id && prf_register_File[rob[i].src2].old_instance == 1){
                    prf_register_File[rob[i].src2].busy = 0;
                    prf_register_File[rob[i].src2].old_instance = 0;
                }
                rob_com++;
                rob[i] = nop;
            }
        }
        else if(!(strcmp(rob[i].opcode, "OR"))){
            printf("\n ROB      : \t\t %s R%d P%d P%d", rob[i].opcode, rob[i].dest, rob[i].src1, rob[i].src2);
            if (rob[i].status == VALID){
                arf_register_File[rob[i].dest].value = rob[i].result;
                if (prf_register_File[rob[i].src1].ins_id == rob[i].id && prf_register_File[rob[i].src1].old_instance == 1){
                    prf_register_File[rob[i].src1].busy = 0;
                    prf_register_File[rob[i].src1].old_instance = 0;
                }
                if (prf_register_File[rob[i].src2].ins_id == rob[i].id && prf_register_File[rob[i].src2].old_instance == 1){
                    prf_register_File[rob[i].src2].busy = 0;
                    prf_register_File[rob[i].src2].old_instance = 0;
                }
                rob_com++;
                rob[i] = nop;
            }
        }
        else if(!(strcmp(rob[i].opcode, "EX-OR"))){
            printf("\n ROB      : \t\t %s R%d P%d P%d", rob[i].opcode, rob[i].dest, rob[i].src1, rob[i].src2);
            if (rob[i].status == VALID){
                arf_register_File[rob[i].dest].value = rob[i].result;
                if (prf_register_File[rob[i].src1].ins_id == rob[i].id && prf_register_File[rob[i].src1].old_instance == 1){
                    prf_register_File[rob[i].src1].busy = 0;
                    prf_register_File[rob[i].src1].old_instance = 0;
                }
                if (prf_register_File[rob[i].src2].ins_id == rob[i].id && prf_register_File[rob[i].src2].old_instance == 1){
                    prf_register_File[rob[i].src2].busy = 0;
                    prf_register_File[rob[i].src2].old_instance = 0;
                }
                rob_com++;
                rob[i] = nop;
            }
        }
        else if(!(strcmp(rob[i].opcode, "MUL"))){
            printf("\n ROB     : \t\t %s R%d P%d P%d", rob[i].opcode, rob[i].dest, rob[i].src1, rob[i].src2);
            if (rob[i].status == VALID){
                arf_register_File[rob[i].dest].value = rob[i].result;
                if (prf_register_File[rob[i].src1].ins_id == rob[i].id && prf_register_File[rob[i].src1].old_instance == 1){
                    prf_register_File[rob[i].src1].busy = 0;
                    prf_register_File[rob[i].src1].old_instance = 0;
                }
                if (prf_register_File[rob[i].src2].ins_id == rob[i].id && prf_register_File[rob[i].src2].old_instance == 1){
                    prf_register_File[rob[i].src2].busy = 0;
                    prf_register_File[rob[i].src2].old_instance = 0;
                }
                rob_com++;
                rob[i] = nop;
            }
        }
        else if(!(strcmp(rob[i].opcode, "DIV"))){
            printf("\n ROB     : \t\t %s R%d P%d P%d", rob[i].opcode, rob[i].dest, rob[i].src1, rob[i].src2);
            if (rob[i].status == VALID){
                arf_register_File[rob[i].dest].value = rob[i].result;
                if (prf_register_File[rob[i].src1].ins_id == rob[i].id && prf_register_File[rob[i].src1].old_instance == 1){
                    prf_register_File[rob[i].src1].busy = 0;
                    prf_register_File[rob[i].src1].old_instance = 0;
                }
                if (prf_register_File[rob[i].src2].ins_id == rob[i].id && prf_register_File[rob[i].src2].old_instance == 1){
                    prf_register_File[rob[i].src2].busy = 0;
                    prf_register_File[rob[i].src2].old_instance = 0;
                }
                rob_com++;
                rob[i] = nop;
            }
        }
        else if(!(strcmp(rob[i].opcode, "HALT"))){
            printf("\n ROB     : \t\t %s ", rob[i].opcode);
            if (rob[i].status == VALID){
                rob_com++;
                rob[i] = nop;
                hflag = 100;
            }
        }
        else if(!(strcmp(rob[i].opcode, "JUMP"))){
            printf("\n ROB     : \t\t %s P%d %d ", rob[i].opcode, rob[i].src1, rob[i].literal);
            if (rob[i].status == VALID){
                rob_com++;
                bflag = 1;
                pc = rob[i].result;
                jflag = 0;
//printf("\n**** PC: %d ****", pc);
                rob[i] = nop;
            }
        }
        else if(!(strcmp(rob[i].opcode, "JAL"))){
            printf("\n ROB     : \t\t %s R%d P%d %d", rob[i].opcode, rob[i].dest, rob[i].src1, rob[i].literal);
            if (rob[i].status == VALID){
                arf_register_File[rob[i].dest].value = rob[i].index;
                pc = rob[i].result;
                if (prf_register_File[rob[i].src1].ins_id == rob[i].id && prf_register_File[rob[i].src1].old_instance == 1){
                    prf_register_File[rob[i].src1].busy = 0;
                    prf_register_File[rob[i].src1].old_instance = 0;
                }
                bflag = 1;
                jalflag = 0;
                rob_com++;
                rob[i] = nop;
            }
        }
        //problem will occur when branch == 1, flushing will require to reset the dest prf to valid 
        //one sol is to loop through the prf with ins.id > bnz.id and validate them. <OLD-INSTANCE = 0>
        else if(!(strcmp(rob[i].opcode, "BNZ"))){
            bzflag = 0;
            if (rob[i].status == VALID){
                if(rob[i].branch == 1){
                    printf("\n ROB     : \t\t %s %d ", rob[i].opcode, rob[i].literal);
                    bflag = 1;
                    //hflag = 0;
                    pc = rob[i].result;
//printf("\n*****PC: %d hflag : %d bflag : %d*********", pc, hflag, bflag);                    
                    for (int j = 0; j < 32; j++){
                        if (rob[j].id > rob[i].id){
//printf("\n***** j: %d & i: %d*****", rob[j].id, rob[i].id);
                            rob[j] = nop;
//printf("\n**** opcodes %d : %s", j, rob[j].opcode);
                        }
                    }
                    rob_com++;
                    rob_add = rob_com;
                    rob[i] = nop;
                }
                else {
                    printf("\n ROB     : \t\t %s %d ", rob[i].opcode, rob[i].literal);
                    rob[i] = nop;
                    rob_com++;
                }
            }
        }
        else if(!(strcmp(rob[i].opcode, "BZ"))){
            bzflag = 0;
            if (rob[i].status == VALID){
                if(rob[i].branch == 1){
                    printf("\n ROB     : \t\t %s %d ", rob[i].opcode, rob[i].literal);
                    bflag = 1;
                    //hflag = 0;
                    pc = rob[i].result;
                    for (int j = 0; j < 32; j++){
                        if (rob[j].id > rob[i].id)
                            rob[j] = nop;
                    }
                    rob_com++;
                    rob_add = rob_com;
                    rob[i] = nop;
                }
                else {
                    printf("\n ROB     : \t\t %s %d ", rob[i].opcode, rob[i].literal);
                    rob[i] = nop;
                    rob_com++;
                }
            }
        }
        else if(!(strcmp(rob[i].opcode, "LOAD"))){
            if (rob[i].status == VALID){
                printf("\n ROB     : \t\t %s R%d P%d %d", rob[i].opcode, rob[i].dest, rob[i].src1, rob[i].literal);
                arf_register_File[rob[i].dest].value = rob[i].result;
                if (prf_register_File[rob[i].src1].ins_id == rob[i].id && prf_register_File[rob[i].src1].old_instance == 1){
                    prf_register_File[rob[i].src1].busy = 0;
                    prf_register_File[rob[i].src1].old_instance = 0;
                }
                rob_com++;
                rob[i] = nop;
            }
        }
        else if(!(strcmp(rob[i].opcode, "STORE"))){
            printf("\n ROB     : \t\t %s R%d P%d %d", rob[i].opcode, rob[i].src1, rob[i].src2, rob[i].literal);
            if (rob[i].status == VALID){
                if (prf_register_File[rob[i].src1].ins_id == rob[i].id && prf_register_File[rob[i].src1].old_instance == 1){
                    prf_register_File[rob[i].src1].busy = 0;
                    prf_register_File[rob[i].src1].old_instance = 0;
                }
                if (prf_register_File[rob[i].src2].ins_id == rob[i].id && prf_register_File[rob[i].src2].old_instance == 1){
                    prf_register_File[rob[i].src2].busy = 0;
                    prf_register_File[rob[i].src2].old_instance = 0;
                }
                rob_com++;
                rob[i] = nop;
            }
        }
        //else if
    }
    if(rob_com > 31){
        for(int i = 0; i < 32; i++){
            if((strcmp(rob[i].opcode, "nop")))
              rob_com = i;
        }
    }
    if (rob_add == rob_com){
        if((strcmp(rob[rob_add].opcode, "nop")))
            rob_full = 1;
        else
            rob_full = 0;
    }
}
