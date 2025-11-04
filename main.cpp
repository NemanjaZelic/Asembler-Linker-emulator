#include <iostream> 
#include <cstdio>   
#include <cstdlib>  
extern FILE *yyin;
extern FILE *yyout;
extern int yyparse();

using namespace std;

int main(int argc, char* argv[]) {

    yyin = fopen(argv[1], "r");
    if (!yyin) {
        std::cerr << "ERROR" << argv[1] <<endl;
        return 1;
    }

   
    yyout = fopen(argv[2], "w");
    if (!yyout) {
        std::cerr << "ERROR  " << argv[2] << endl;
        fclose(yyin);
        return 1;
    }


    yyparse();


    fclose(yyin);
    fclose(yyout);

    return 0;
}