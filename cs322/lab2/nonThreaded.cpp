#include <iostream>

using namespace std;

void task (string msg, int itr) {

        float f1 = 1.0123456789f;
	float f2 = 0.5678902134f;

        for(int i = 0; i < itr; i++) {
                f1 = f1/f2;
        }

        cout << "The task says " << msg << endl;
}

int main () {

        task("thread_1", 1000000);
	task("thread_2", 1000000);
	task("thread_3", 1000000);
	task("thread_4", 1000000);
	task("thread_5", 1000000);
}

