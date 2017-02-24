#include <iostream>
using namespace std;

void task (string msg) {
	cout << "The task says " << msg << endl;
}

int main () {

	task ("thread_1");
	task ("thread_2");
	task ("thread_3");
}
