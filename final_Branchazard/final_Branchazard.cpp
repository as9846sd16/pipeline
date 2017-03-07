#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include<fstream>
#include <iomanip>
#define MEMSIZE 20
#define REGSIZE 9
using namespace std;
ofstream fout("branchResult.txt", ios::out);
struct {
    int rs, rt, rd;
    string ins;
    void reset(){
        rs = rt = rd = 0;
        ins = "00000000000000000000000000000000";
    }
} IF_ID = {0, 0, 0, "00000000000000000000000000000000"};
struct {
    int ReadData1, ReadData2, sign_ext, rs, rt, rd;
    bool isRd;
    string cs, func;
    void reset(){
        ReadData1 = ReadData2 = sign_ext = rs = rt = rd = 0;
        isRd = 0;
        cs = "000000000";
        func = "000000";
    }
} ID_EX = {0,0,0,0,0,0,0,"000000000", "000000"};// ID/EX register
struct {
    int ALUout, WriteData, rt;
    bool isRd;
    string cs;
    void reset(){
        ALUout = WriteData = rt = 0;
        isRd = 0;
        cs = "00000";
    }
} EX_MEM = {0,0,0,0,"00000"};// EX/MEM register
struct {
    int ReadData, ALUout, rt;
    string cs;
    void reset(){
        ReadData = ALUout = rt = 0;
        cs = "00";
    }
} MEM_WB = {0,0,0,"00"};//
int Reg[REGSIZE] = {0,5,8,6,7,5,1,2,4};
int Mem[MEMSIZE/4] = {9,6,4,2,5};
int insNum=0, PC=0, cycle = 0, maxCycle = 0, dhMEM_WB_ALUout = 0, lhNum = 0, bNum = 0, bcycle = 0;
bool lwhazard= 0, bMade = 0;
string fs = "00", ft = "00";
string ins[100];
string control;
/*-----------------
    0 Mem2Reg;
    1 RegWrite;
    2 MemWrite;
    3 MemRead;
    4 Branch;
    5 ALUSrc;
    6 ALUop0;
    7 ALUop1;
    8 RegDst;
-----------------*/
void read();
void pStatus();
bool equ(bool* ins1, bool* ins2, int b1, int b2,int l);
int decode(string ins, int b, int l);
void IFstage();
void IDstage();
void EXstage();
string ALUctr();
void MEMstage();
void WBstage();
int main(){
    for(int i = 0; i<100; i++){
        ins[i] = "00000000000000000000000000000000";
    }
    read();
    maxCycle = 5 + (insNum-1);
    cycle = 1;
    lhNum = 0;
    while(cycle <= maxCycle + lhNum){
        fs ="00", ft = "00";
        dhMEM_WB_ALUout = 0;
        lwhazard = 0;
        bMade = 0;
        bNum = 0;
        if(cycle >= 5 && cycle <= insNum + 4 + lhNum){
            WBstage();
        }
//-------------------MEM/WB
        if(cycle >= 4 && cycle <= insNum + 3 + lhNum){
            MEMstage();
        }
        else{
            MEM_WB.reset();
        }
//-------------------EX/MEM
        if(cycle >= 3 && cycle <= insNum + 2 + lhNum){
            EXstage();
        }
        else{
            EX_MEM.reset();
        }
//-------------------ID/EX
        if(cycle >= 2 && cycle <= insNum + 1 + lhNum){
            IDstage();
        }
        else{
            ID_EX.reset();
        }
//-------------------IF/ID
        if(cycle >= 1 && cycle <= insNum + lhNum){
            IFstage();
        }
        else{
            IF_ID.reset();
        }
//-------------------print
        PC+=4;
        if(lwhazard){
            PC -= 4;
        }
        if(bMade){
            PC =PC + bNum*4 - 4;
            maxCycle = maxCycle - bNum + 1;
            insNum = insNum -bNum + 1;
            IF_ID.reset();
        }
        pStatus();
        cycle++;
    }
    //pStatus();
    fout.close();
    return 0;
}
void IFstage(){
    //process
    string curins;
    curins = ins[PC/4];
    //setting
    if(!lwhazard){
        IF_ID.rs = decode(curins, 21, 5);
        IF_ID.rt = decode(curins, 16, 5);
        if(curins.substr(0,6) == "000000"){//r-type
            IF_ID.rd = decode(curins, 11, 5);
        }
        else{//i-type
            IF_ID.rd = 0;
        }
        IF_ID.ins = curins;
    }
    return;
}
void IDstage(){
    //-----control
    string op = IF_ID.ins.substr(0,6);
    string func = IF_ID.ins.substr(26,6);
    bool isRd = false;
    if(ID_EX.cs[3] == '1' && (ID_EX.rt == IF_ID.rs || ID_EX.rt == IF_ID.rt)){
        lwhazard = true;
    }
    if(IF_ID.ins == "00000000000000000000000000000000"){
        control = "000000000";
    }
    else if(op == "000000"){//R-type
        control = "010000011";
        if(!lwhazard){
            isRd = true;
        }
    }
    else if(op == "100011"){//lw
        control = "110101000";
    }
    else if(op == "101011"){//sw
        control = "001001000";//"X0100100X"
    }
    else if(op == "000100"){//beq
        control = "000010100";//"X0001010X"
    }
    //process
    if(ID_EX.cs[3] == '1' && (ID_EX.rt == IF_ID.rs || ID_EX.rt == IF_ID.rt)){
        lwhazard = true;
        cout << "Cycle: " << cycle << "---" << "Lw Hazard" << endl;
    }
    //branch
    else if(control[4] == '1' && !(Reg[IF_ID.rs] ^ Reg[IF_ID.rt])){
        bNum = decode(IF_ID.ins,0,16);
        bMade = 1;
        cout << "Cycle: " << cycle << "---" << "Branch Hazard" << endl;
    }
    //----set reg
    ID_EX.func = func;
    if(lwhazard){
        ID_EX.cs = "000000000";
        lhNum++;
    }
    else{
        ID_EX.cs = control;
    }
    ID_EX.ReadData1 = Reg[IF_ID.rs];
    ID_EX.ReadData2 = Reg[IF_ID.rt];
    ID_EX.rs = IF_ID.rs;
    ID_EX.rt = IF_ID.rt;
    ID_EX.rd = IF_ID.rd;
    if(!lwhazard && !isRd){
        ID_EX.sign_ext = decode(IF_ID.ins,0,16);
    }
    else{
        ID_EX.sign_ext = 0;
    }
    ID_EX.isRd = isRd;
    return;
}
void EXstage(){
    //process
    if(EX_MEM.cs[1] == '1' && EX_MEM.rt!=0 && EX_MEM.rt==ID_EX.rs){
        fs = "10";
    }
    if(EX_MEM.cs[1] == '1' && EX_MEM.rt!=0 && EX_MEM.rt==ID_EX.rt){
        ft = "10";
    }
    string opt = ALUctr();
    int op1, op2;
    op1 = ID_EX.ReadData1;
    if(ID_EX.cs[5] == '1'){//lw, sw
        op2 = ID_EX.sign_ext;
    }
    else if(ID_EX.cs[5] == '0'){//r-type, beq
        op2 = ID_EX.ReadData2;
    }
    if(fs == "10"){
        op1 = EX_MEM.ALUout;
        cout << "Cycle: " << cycle << "---" << "EX-Data Hazard: Rs" << endl;
    }
    else if(fs == "01"){
        op1 = dhMEM_WB_ALUout;
        cout << "Cycle: " << cycle << "---" << "MEM-Data Hazard: Rs" << endl;
    }
    if(ft == "10"){
        op2 = EX_MEM.ALUout;
        cout << "Cycle: " << cycle << "---" << "EX-Data Hazard: Rt" << endl;
    }
    else if(ft == "01"){
        op2 = dhMEM_WB_ALUout;
        cout << "Cycle: " << cycle << "---" << "MEM-Data Hazard: Rt" << endl;
    }
    //ALU
    int ans;
    bool zero;
    if(opt == "000"){//and
        ans = op1 & op2;
    }
    else if(opt == "001"){//or
        ans = op1 | op2;
    }
    else if(opt == "010"){//add
        ans = op1 + op2;
    }
    else if(opt == "110"){//sub
        ans = op1 - op2;
    }
    else if(opt == "111"){//slt
        ans = (int)(op1 < op2);
    }
    else{//don't care
        ans = -1;
    }

    zero = (ans == 0);
    //setting
    EX_MEM.WriteData = ID_EX.ReadData2;
    if(ft == "10"){
        EX_MEM.WriteData = EX_MEM.ALUout;
    }
    else if(ft == "01"){
        EX_MEM.WriteData = dhMEM_WB_ALUout;
    }
    EX_MEM.ALUout = ans;
    EX_MEM.cs = ID_EX.cs.substr(0,5);
    EX_MEM.isRd = ID_EX.isRd;
    EX_MEM.rt = (ID_EX.isRd ? ID_EX.rd : ID_EX.rt);
    return;
}
string ALUctr(){
    string opt ="";
    //----ALUcontrol
    if(ID_EX.cs.substr(6,2) == "00"){
        opt = "010";
    }
    else if(ID_EX.cs[6] == '1') {
        opt = "110";
    }
    else if(ID_EX.cs[7] == '1'){
        if(ID_EX.func.substr(2, 4) == "0000"){
            opt = "010";
        }
        else if(ID_EX.func.substr(2, 4) == "0010"){
            opt = "110";
        }
        else if(ID_EX.func.substr(2, 4) == "0100"){
            opt = "000";
        }
        else if(ID_EX.func.substr(2, 4) == "0101"){
            opt = "001";
        }
        else if(ID_EX.func.substr(2, 4) == "1010"){
            opt = "111";
        }
    }
    return opt;
}
void MEMstage(){
    int data = 0;
    //process
    if(EX_MEM.cs[2] == '1') {// MEMwrite == 1
        if(EX_MEM.ALUout/4 <= MEMSIZE && EX_MEM.ALUout/4 >= 0){
            Mem[EX_MEM.ALUout/4] = EX_MEM.WriteData;
        }
        else{
            cout << "MEM[EX_MEM.ALUout/4 ("<< EX_MEM.ALUout/4 <<")] = EX_MEM.rt out of range" << endl;
        }
    }
    if(EX_MEM.cs[3] == '1') {// MEMread == 1
        if(EX_MEM.ALUout<= MEMSIZE && EX_MEM.ALUout >=0){
            data = Mem[EX_MEM.ALUout/4];
        }
        else{
            cout << "data = MEM[EX_MEM.ALUout/4(" << EX_MEM.ALUout/4 <<")] out of range" << endl;
        }
    }
    if(MEM_WB.cs[1] == '1' && MEM_WB.rt!=0 && EX_MEM.rt!=ID_EX.rs && MEM_WB.rt == ID_EX.rs){
        fs = "01";
        if(MEM_WB.cs[0] == '1'){//lw
            dhMEM_WB_ALUout = MEM_WB.ReadData;
        }
        else if(MEM_WB.cs[0] == '0'){//r-type
            dhMEM_WB_ALUout = MEM_WB.ALUout;
        }
    }
    if(MEM_WB.cs[1] == '1' && MEM_WB.rt!=0 && EX_MEM.rt!=ID_EX.rt && MEM_WB.rt == ID_EX.rt){
        ft = "01";
        dhMEM_WB_ALUout = MEM_WB.ALUout;
    }
    //setting
    MEM_WB.ALUout = EX_MEM.ALUout;
    MEM_WB.cs = EX_MEM.cs.substr(0,2);
    MEM_WB.ReadData = data;
    MEM_WB.rt = EX_MEM.rt;
    return;
}
void WBstage(){
    //process
        int data;
        if(MEM_WB.cs[0] == '1'){//lw
            data = MEM_WB.ReadData;
        }
        else if(MEM_WB.cs[0] == '0'){// r-type
            data = MEM_WB.ALUout;
        }
        if(MEM_WB.cs[1] == '1'){// lw, R-type
            Reg[MEM_WB.rt] = data;
        }
    return;
}
int decode(string ins, int b, int l){
    int reg = 0, digit = 1, tmp = 0;
    for(int i = b; i<b+l; i++,digit = digit * 2){
        tmp = ins[31-i] - '0';
        reg = reg + tmp * digit;
    }
    return reg;
}
bool equ(bool* ins1, bool* ins2, int b1, int b2,int l){
    bool flag = 1;
    for(int i = 0; i<l; i++, b1++, b2++){
        if(ins1[b1] != ins2[b2]){
            flag = false;
        }
    }
    return flag;
}
void read(){
    ifstream fin("Branchazard.txt", ios::in);
    insNum = 0;
    while(fin >> ins[insNum]){
        insNum++;
    }
    fin.close();
}
void pStatus(){
    fout << "CC " << cycle << ":" << endl << endl;
    //Registers
    fout << "Registers:" << endl;
    for(int i = 0; i<REGSIZE; i++){
        fout << "$" << i << ": " << Reg[i] << "\t";
        if(i == 2 || i == 5 || i == 8)
            fout << endl;
    }
    //Data memory
    fout << endl;
    fout << "Data memory:" << endl;
    for(int i = 0; i<MEMSIZE; i+=4){
        fout << setw(2) << setfill('0') << i << ":";
        fout << "\t" << Mem[i/4] << endl;
    }
    //IF/ID
    fout << endl;
    fout << "IF/ID :" << endl;
    fout << "PC\t\t" << PC << endl;
    fout << "Instruction\t";
    fout << IF_ID.ins << endl;
    //ID/EX
    fout << endl << endl;
    fout << "ID/EX :" << endl;
    fout << "ReadData1\t" << ID_EX.ReadData1 << endl;
    fout << "ReadData2\t" << ID_EX.ReadData2 << endl;
    fout << "sign_ext\t" << ID_EX.sign_ext << endl;
    fout << "Rs\t\t" << ID_EX.rs << endl;
    fout << "Rt\t\t" << ID_EX.rt << endl;
    fout << "Rd\t\t" << ID_EX.rd << endl;
    fout << "Control signals\t";
    for(int i = 8; i>=0; i--){
        fout << ID_EX.cs[i];
    }
    //EX/MEM
    fout << endl<<endl;
    fout << "EX/MEM :" << endl;
    fout << "ALUout\t\t" << EX_MEM.ALUout << endl;
    fout << "WriteData\t" << EX_MEM.WriteData << endl;
    fout << (EX_MEM.isRd ? "Rd" : "Rt") << "\t\t" << EX_MEM.rt << endl;
    fout << "Control signals\t";
    for(int i = 4; i>=0; i--){
        fout << EX_MEM.cs[i];
    }
    //MEM/WB
    fout << endl << endl;
    fout << "MEM/WB :" << endl;
    fout << "ReadData\t" << MEM_WB.ReadData << endl;
    fout << "ALUout\t\t" << MEM_WB.ALUout << endl;
    fout << "Control signals\t";
    for(int i = 1; i>=0; i--){
        fout << MEM_WB.cs[i];
    }
    fout << endl;
    fout << "=================================================================" << endl;
}
