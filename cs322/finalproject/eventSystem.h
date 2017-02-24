#include <vector>
#include <future>

using namespace std;

typedef class events {
private:
	int cont;
	int arrive;
	bool bg;

	vector<future<void>> functions;
	future<void> coordinator;
	void driver();
	void worker(void(*foo)(void), int num);

public:
	void addFunction(void(*foo)(void));
	void execute(); //Creates the coordinator and waits
	void callFunction(int num);
	void checkFunction();
	events();

} eventSystem;