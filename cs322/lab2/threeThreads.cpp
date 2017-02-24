#include <iostream>
#include <thread>

using namespace std;

void task (string msg) {
	cout << "The task says " << msg << endl;
}


int main () {

	thread t1(task, "thread_1");
	thread t2(task, "thread_2");
	thread t3(task, "thread_3");

	t1.join();
	t2.join();
	t3.join();
}
