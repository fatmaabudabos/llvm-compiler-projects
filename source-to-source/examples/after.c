
#include <string.h>

struct __Controller {
    unsigned int skipThen;
    unsigned int skipElse;
    unsigned int skipWhileLoops;
    unsigned int skipDoWhileLoops;
    unsigned int skipForLoops;
    unsigned int skipBreaks;
    unsigned int skipContinues;
    char* skipFunctionName;
};

void foo(int x, struct __Controller __c) {
    for(x = 0; (x < 10) && !__c.skipForLoops; ++x) {
        x += 2;
    }

}

void bar(int x, int y, struct __Controller __c) {

    if((x > 1) && !__c.skipThen) {
        x = 25;
    } else if(!__c.skipElse) {
        x = 36;
        if(strcmp(__c.skipFunctionName, "foo") != 0) foo(1, __c);
    }

    while((x < 9) && !__c.skipWhileLoops) {
        x = 2;
        if((x < 4) && !__c.skipThen) {
            if(!__c.skipBreaks) break;
        } else if(!__c.skipElse) {
            if(!__c.skipContinues) continue;
        }
        x = 3;
    }

}

int main(int argc, char** argv) {

struct __Controller __c;
__c.skipThen = 0;
__c.skipElse = 0;
__c.skipWhileLoops = 0;
__c.skipDoWhileLoops = 0;
__c.skipForLoops = 0;
__c.skipBreaks = 0;
__c.skipContinues = 0;
__c.skipFunctionName = "";

    int x = 0;

    if((x < 1) && !__c.skipThen) {
        x = 25;

    }

    while((x < 10) && !__c.skipWhileLoops) {
        x = x + 1;
        if(strcmp(__c.skipFunctionName, "bar") != 0) bar(1, 2, __c);
    }

    if(!__c.skipDoWhileLoops)
do {
        x = x - 1;
    } while (x > 5);

    return 0;

}

