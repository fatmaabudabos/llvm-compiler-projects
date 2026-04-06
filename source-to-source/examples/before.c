
void foo(int x) {
    for(x = 0; x < 10; ++x) {
        x += 2;
    }

}

void bar(int x, int y) {

    if(x > 1) {
        x = 25;
    } else {
        x = 36;
        foo(1);
    }

    while(x < 9) {
        x = 2;
        if(x < 4) {
            break;
        } else {
            continue;
        }
        x = 3;
    }

}

int main(int argc, char** argv) {

    int x = 0;

    if(x < 1) {
        x = 25;

    }

    while(x < 10) {
        x = x + 1;
        bar(1, 2);
    }

    do {
        x = x - 1;
    } while (x > 5);

    return 0;

}

