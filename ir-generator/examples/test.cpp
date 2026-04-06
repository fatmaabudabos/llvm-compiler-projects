
int testBinaryOperator(int x) {
    return x%2 + 7 - 8*x/3 + (x << 6) - (x >> 2) + (x & 8) - (x | 7);
}

int testAssignments(int x) {
    int y;
    y = 1;
    x = y = 2;
    y += x;
    (y *= x) = 1;
    return y;
}

int testUnaryOperator(int x) {
    int y = -x;
    x = +1;
    y = ~x;
    ++x;
    (++y) = 2;
    int* p = &x;
    *p = 8;
    return x--;
}

int testIfElse(int x, int y) {
    if(x > 1 && x < 8) {
        x = 25;
    } else {
        x = 36;
    }
    return y;
}

float testIfWithoutElse(float x) {
    if(x > 1.1f || x < 0.1f) {
        x = 2.5f;
    }
    return 3.0f;
}

void testWhileLoop(int x) {
    while(x < 10) {
        x = x + 1;
    }
    return;
}

void testDoWhileLoop() {
    int x = 0;
    do {
        x = (x - 1);
    } while (x > 5);
    return;
}

void testForLoop() {
    int x;
    for(x = 0; x < 10; x = x + 1) {
        x = x + 2;
    }
    return;
}

void testBreakAndContinue(int x, int y) {
    while(x < 9) {
        x = 2;
        if(x < 4) {
            break;
        } else {
            continue;
        }
        x = 3;
    }
    return;
}

char testCharLiteral(char x) {
    x = 'a';
    return 'b';
}

int testTernaryOperator(int x, int y) {
    return (x > y)?(x - y):(y - x);
}

int testCall(int x, int* y) {
    return x + *y;
}

int main() {
    int x = 1;
    int y = testCall(1, &x);
    return 0;
}

