#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <fstream>
using namespace std;
extern int yylineno;
unordered_map<string, int> operations={{"halt",1},{"int",2},{"iret",3},{"jump",4},{"call",5},{"ret",6},{"beq",7},{"bne",8},{"bgt",9},{"xchg",10},{"add",11},{"sub",12}
,{"mul",13},{"div",14},{"not",15},{"and",16},{"or",17},{"xor",18},{"shl",19},{"shr",20},{"csrrd",21},{"csrrw",22},{"push",23},{"pop",24}};
unordered_map<string, int> ldst={{"lslitdir",1},{"lslitindir",2},{"ldreg",3},{"ldregind",4},{"ldregindpom",5}
,{"stlitdir",6},{"stlitindir",7},{"streg",8},{"stregind",9},{"stregindpom",10}};
string hexa="0123456789ABCDEF";
struct symbols{
string name;
string section;
int offset;
string domen;
};

struct realoc{
    string name;
    string section;
    int offset;
    
};

struct section{
    string name;
    vector<string> instukcije;
    vector<string> literali;
    vector<int> literaloff;
    int offset;
};

bool greska=false;
vector<symbols> symbol_table;
vector<section*> sections;
vector<realoc> realoc_table;
vector<int> realoc_literals;
vector<string> literals;
vector<realoc> nessesery_realoc_table;
section* curent_sec;
int curent_offset;
int inserts=0;
void globalsec(string symb)
{
    cout<<"global ";
        cout<<symb<<" "<<endl;
        symbols priv;
        priv.name=symb;
        priv.offset=0;
        priv.section="UND";
        priv.domen="global";
        symbol_table.push_back(priv);
    
}
void externsec(string symb)
{
        for(int i=0;i<symbol_table.size();i++)
        if(symbol_table[i].name==symb) 
        {
            cout<<"ISTO NAZVANE PROMENLJIVE";
            greska=true;
            return;
        }
        symbols priv;
        priv.name=symb;
        priv.offset=0;
        priv.section="UND";
        priv.domen="extern";
        symbol_table.push_back(priv);
    
}

void newsection(string symb)
{
    for(int i=0;i<symbol_table.size();i++)
        if(symbol_table[i].name==symb) 
        if(symbol_table[i].domen=="global")
        {
            symbol_table[i].section=curent_sec->name;
            symbol_table[i].offset=curent_sec->offset;
            return;
        }
        else 
        {
            cout<<"ISTO NAZVANE PROMENLJIVE";
            greska=true;
            return;
        }
        symbols priv;
        priv.name=symb;
        priv.offset=curent_sec->offset;
        priv.section=curent_sec->name;
        priv.domen="local";
        symbol_table.push_back(priv);
}

void newswction(string name)
{
    curent_sec = new section;
    curent_sec->name=name;
    curent_sec->offset=0;
    sections.push_back(curent_sec);
}

void yyerror(const char *s) {

    fprintf(stderr, "Error on line %d: %s\n", yylineno, s);
}
string dectohex(int num)
{
    char buffer[20];   
    sprintf(buffer, "%08X", num); 
    string hexStr = buffer;        
    return hexStr;
}
string regtohex(int num)
{
    char buffer[20];   
    sprintf(buffer, "%01X", num); 
    string hexStr = buffer;        
    return hexStr;
}
int yylex(void);
extern FILE *yyin;
extern FILE *yyout;
extern int yyparse();


void funtest()
{
    cout<<"HELO";
}

void writecur(string bin)
{
    curent_sec->instukcije.push_back(bin);
    curent_sec->offset+=4;
}


void inisilaze(int num)
{
    char buffer[20];   
    sprintf(buffer, "%08X", num); 
    string hexStr = buffer;        
     writecur(string()+hexStr[0]+hexStr[1]+" "+hexStr[2]+hexStr[3]+" "+hexStr[4]+hexStr[5]+" "+hexStr[6]+hexStr[7]);
}

void asci(string label)
{
    string priv = "00 00 00 00";
    int pos = 0;
    
    for(int i = 0; i < label.size(); i++)
    {
        if(i % 4 == 0 && i != 0) 
        {
            writecur(priv);
            priv = "00 00 00 00";
            pos = 0;
        }
        
        unsigned char value = (unsigned char)label[i];
        char hex_high = hexa[(value >> 4) & 0xF];
        char hex_low = hexa[value & 0xF];
        
      
        priv[pos * 3] = hex_high;
        priv[pos * 3 + 1] = hex_low;
        pos++;
    }
    
  
    writecur(priv);
}

void addrealoc(string label, int offset)
{
    realoc priv;
    priv.name=label;
    priv.section=curent_sec->name;
    priv.offset=curent_sec->literali.size()*4;
    realoc_table.push_back(priv);
}

void addnesseryrealoc(string label, int offset)
{
    realoc priv;
    priv.name=label;
    priv.section=curent_sec->name;
    priv.offset=curent_sec->offset+offset;
    nessesery_realoc_table.push_back(priv);
}
void push(char r)
{
    writecur(string()+"81 E0 "+r+"F FC");
}

void pop(char r)
{
    writecur(string()+"93 "+r+"E 00 04");
}

void iret()
{
    pop('F');
    writecur("97 0E 00 04");
}

void halt()
{
    cout<<"HALTINGGG";
    writecur("00 00 00 00");
}

void jump(string literal,string mod="8",string r2="0",string r1="0",string r0="0")
{
    curent_sec->literali.push_back(literal);
    curent_sec->literaloff.push_back(curent_sec->offset);
    writecur(string()+"3"+mod+" "+"F"+r1+" "+r2+"0 "+"00");
    
}

void call(string literal)
{
    curent_sec->literali.push_back(literal);
    curent_sec->literaloff.push_back(curent_sec->offset);
    writecur(string()+"2"+"1"+" "+"F"+"0"+" "+"0"+"0 "+"00"); 
}

void beq(int r1,int r2,string literal)
{
    jump(literal,"9",regtohex(r2),regtohex(r1));
}

void bne(int r1,int r2,string literal)
{
    jump(literal,"A",regtohex(r2),regtohex(r1));
}

void bgt(int r1,int r2,string literal)
{
    jump(literal,"B",regtohex(r2),regtohex(r1));
}

void ret()
{
    pop('F');
}

void INT_is()
{
    writecur("10 00 00 00");
}

void xchg(int r1,int r2)
{
    writecur(string()+"40 0"+regtohex(r1)+" "+regtohex(r2)+"0 00");
}

void add(int r1,int r2)
{
    writecur(string()+"50 "+regtohex(r2)+regtohex(r2)+" "+regtohex(r1)+"0 00");
}

void sub(int r1,int r2)
{

    writecur(string()+"51 "+regtohex(r2)+regtohex(r2)+" "+regtohex(r1)+"0 00");
}

void mul(int r1,int r2)
{

    writecur(string()+"52 "+regtohex(r2)+regtohex(r2)+" "+regtohex(r1)+"0 00");
}
void div_operatin(int r1,int r2)
{
    writecur(string()+"53 "+regtohex(r2)+regtohex(r2)+" "+regtohex(r1)+"0 00");
}

void not_operation(int r1)
{
    writecur(string()+"60 "+regtohex(r1)+regtohex(r1)+" "+"00 00");
}

void and_operation(int r1,int r2)
{
    writecur(string()+"61 "+regtohex(r2)+regtohex(r2)+" "+regtohex(r1)+"0 00");
}

void or_operation(int r1,int r2)
{
    writecur(string()+"62 "+regtohex(r2)+regtohex(r2)+" "+regtohex(r1)+"0 00");
}

void xor_operation(int r1,int r2)
{
    writecur(string()+"63 "+regtohex(r2)+regtohex(r2)+" "+regtohex(r1)+"0 00");
}

void shl(int r1,int r2)
{
    writecur(string()+"70 "+regtohex(r2)+regtohex(r2)+" "+regtohex(r1)+"0 00");
}

void shr(int r1,int r2)
{
    writecur(string()+"71 "+regtohex(r2)+regtohex(r2)+" "+regtohex(r1)+"0 00");
}

void csrrd(int r1,int r2)
{
    writecur(string()+"90 "+regtohex(r2)+regtohex(r1)+" "+"00 00");
}

void csrrw(int r1,int r2)
{
    writecur(string()+"94 "+regtohex(r2)+regtohex(r1)+" "+"00 00");
}

void ldlitdir(int r1,string literal)
{
    curent_sec->literali.push_back(literal);
    curent_sec->literaloff.push_back(curent_sec->offset);
    writecur(string()+"9"+"2"+" "+regtohex(r1)+"F"+" "+"0"+"0 "+"00");
 }

void ldlitindir(int r1,string literal)
{
    curent_sec->literali.push_back(literal);
    curent_sec->literaloff.push_back(curent_sec->offset);
    writecur(string()+"9"+"2"+" "+regtohex(r1)+"F"+" "+"0"+"0 "+"00");
    writecur(string()+"9"+"2"+" "+regtohex(r1)+regtohex(r1)+" "+"0"+"0 "+"00");
}

void ldreg(int r1,int r2,string literal)
{
    writecur(string()+"9"+"1"+" "+regtohex(r1)+regtohex(r2)+" "+literal[5]+literal[6]+" "+literal[7]);
 }

void ldregind(int r1,int r2,string literal)
{
    curent_sec->literali.push_back(literal);
    curent_sec->literaloff.push_back(curent_sec->offset);
    writecur(string()+"9"+"2"+" "+regtohex(r1)+regtohex(r2)+" "+"0"+"0 "+"00");
 }

void ldreginddisp(int r1, int r2, string displacement)
{
    writecur(string()+"9"+"2"+" "+regtohex(r1)+regtohex(r2)+" "+"0"+displacement[5]+" "+displacement[6]+displacement[7]);
}



void stlitdir(int r1,string literal)
{

    writecur(string()+"8"+"0"+" "+"0"+"0"+" "+regtohex(r1)+literal[5]+" "+literal[6]+literal[7]);
}

void stlitindir(int r1,string literal)
{
    curent_sec->literali.push_back(literal);
    curent_sec->literaloff.push_back(curent_sec->offset);
    writecur(string()+"8"+"2"+" "+"F"+"0"+" "+regtohex(r1)+"0 "+"00");
}

void streg(int r1,int r2,string literal)
{
    writecur(string()+"8"+"0"+" "+regtohex(r2)+"0"+" "+regtohex(r1)+literal[5]+" "+literal[6]+literal[7]);
}

void stregind(int r1,int r2,string literal)
{
    writecur(string()+"8"+"2"+" "+regtohex(r2)+"0"+" "+regtohex(r1)+literal[5]+" "+literal[6]+literal[7]);
}
void asem(string operation,int r1=0 ,int r2=0, string literal="",int num=0)
{
    int opcode=operations[operation];
    switch(opcode)
    {
        case 1:
            halt();
            break;
        case 2:
            INT_is();
            break;
        case 3:
            iret();
            break;
        case 4:
            jump(dectohex(num));
            break;
        case 5:
            call(dectohex(num));    
            break;
        case 6:
            iret();
            break;
        case 7:
            beq(r1,r2,dectohex(num));
            break;
        case 8:
            bne(r1,r2,dectohex(num));
            break;
        case 9:
            bgt(r1,r2,dectohex(num));
            break;
        case 10:
            xchg(r1,r2);
            break;
        case 11:
            add(r1,r2);
            break;
        case 12:
            sub(r1,r2);
            break;
        case 13:
            mul(r1,r2);
            break;
        case 14:
            div_operatin(r1,r2);
            break;
        case 15:
            not_operation(r1);
            break;
        case 16:
            and_operation(r1,r2);
            break;
        case 17:
            or_operation(r1,r2);
            break;
        case 18:
            xor_operation(r1,r2);
            break;
        case 19:
            shl(r1,r2);
            break;
        case 20:
            shr(r1,r2);
            break;
        case 21:
            csrrd(r1,r2);
            break;
        case 22:
            csrrw(r1,r2);
            break;
        case 23:
            push(hexa[r1]);
            break;
        case 24:
            pop(hexa[r1]);
            break;
    }
}
void loadstore(string op, int r1,int r2, string literal="",int num=0)
{
    int opcode=ldst[op];
    switch(opcode)
    {
        case 1:
            ldlitdir(r1,dectohex(num));
            break;
        case 2:
            ldlitindir(r1,dectohex(num));
            break;
        case 3:
            ldreg( r1, r2, dectohex(0));
            break;
        case 4:
            ldregind( r1, r2, dectohex(0));
            break;
        case 5:
            if(num>4095)greska=true;
            ldreginddisp( r1, r2, dectohex(num));
            break;
        case 6:
            stlitdir(r1,dectohex(num));
            break;
        case 7:
            stlitindir(r1,dectohex(num));
            break;
        case 8:
            streg(r1,r2,dectohex(0));
            break;
        case 9:
            stregind(r1,r2,dectohex(0));
            break;
        case 10:
            if(num>4095)greska=true;
            stregind(r1,r2,dectohex(num));
            break;
    }
}
string razmak(string literal)
{
    return string()+literal[0]+literal[1]+" "+literal[2]+literal[3]+" "+literal[4]+literal[5]+" "+literal[6]+literal[7];
}
int main(int argc, char* argv[]) {
    string inputFile;
    string outputFileName = "a.o";

    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "-o" && i + 1 < argc) {
            outputFileName = argv[++i];
        } else {
            inputFile = arg;
        }
    }

    yyin = fopen(inputFile.c_str(), "r");
    if (!yyin) {
        cerr << "ERROR opening input: " << inputFile << endl;
        return 1;
    }


    yyparse();
    
    for(int i=0;i<nessesery_realoc_table.size();i++)
    {
         for(int o=0;o<sections.size();o++)
        {
            if(nessesery_realoc_table[i].section==sections[o]->name)
            {
                nessesery_realoc_table[i].offset+=sections[o]->offset;
            }
        }
    }
    std::ofstream outputFile(outputFileName);

    if (!outputFile.is_open()) {
        std::cerr << "ERROR opening output file " << outputFileName << std::endl;
        return 1;
    }
    

    for(int i=0;i<realoc_table.size();i++)
    {
         for(int o=0;o<sections.size();o++)
        {
            if(realoc_table[i].section==sections[o]->name)
            {
                realoc_table[i].offset+=sections[o]->offset;
            }
        }
    }


    for(int i=0;i<nessesery_realoc_table.size();i++)
    {
          for(int j=0;j<symbol_table.size();j++)
        {
            if(nessesery_realoc_table[i].name==symbol_table[j].name)
            {
                if(symbol_table[j].section=="vrednost")
                {
                    if(symbol_table[j].offset>4095)
                    {
                    greska=true;
                    break;
                    }
                    string literal=dectohex(symbol_table[j].offset);
                    
                    for(int o=0;o<sections.size();o++)
                    {
                        if(nessesery_realoc_table[i].section==sections[o]->name)
                        {
                            int n=nessesery_realoc_table[i].offset /4;
                            for(int k=0;k<11;k++)
                            {
                                sections[o]->instukcije[n+1-k/8][8-k%8]=literal[11-k];
                            }
                        }
                    }
                }
                else
                {
                    greska=true;
                    break;
                }
            }
        }   
        if(greska==true)break;
    }   

    cout<<endl<<endl<<endl<<endl<<endl<<endl<<endl<<endl<<endl;
    for(int i=0;i<sections.size();i++)
    {
         for(int j=0;j<sections[i]->literali.size();j++)
        {
            sections[i]->instukcije.push_back(razmak(sections[i]->literali[j]));
            int dis=sections[i]->offset - (sections[i]->literaloff[j] + 4);
            sections[i]->offset+=4;
            string pom=dectohex(dis);
            sections[i]->instukcije[sections[i]->literaloff[j]/4][7]=pom[5];
            sections[i]->instukcije[sections[i]->literaloff[j]/4][9]=pom[6];
            sections[i]->instukcije[sections[i]->literaloff[j]/4][10]=pom[7];
        }

            outputFile<<"SECTION: "<<sections[i]->name<<endl;
            int test=0;
        for(int j=0;j<sections[i]->instukcije.size();j++)
        {
            outputFile<<test<<": "<<sections[i]->instukcije[j];
            outputFile<<endl;
            test+=4;
        }
        outputFile<<endl<<endl;
        free(sections[i]);
    }

    outputFile<<"SYMBOLS: "<<endl;

    for(int j=0;j<symbol_table.size();j++)
    {
        outputFile<<symbol_table[j].name<<" "<<symbol_table[j].section<<" "<<symbol_table[j].offset<<" "<<symbol_table[j].domen;
        outputFile<<endl;
    }   

    outputFile<<"REALOCS: "<<endl;

    for(int j=0;j<realoc_table.size();j++)
    {
        outputFile<<realoc_table[j].name<<" "<<realoc_table[j].section<<" "<<realoc_table[j].offset;
        outputFile<<endl;
    }   
    
    outputFile.close();

    
    if(argc > 1)
    {
        fclose(yyin);
    }
    return 0;
}