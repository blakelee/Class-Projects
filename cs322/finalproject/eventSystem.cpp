#include "eventSystem.h"

//This is the actual coordinator function
void eventSystem::driver() {
	while(true) {
		while(arrive == 0);
		int temp = arrive;
		for(int i = 0; i < functions.size(); i++) {
			if((temp >> i) & 1 == 1) {
				//Run in background                
				cont |= 1 << i; //set
				arrive &= ~(1 << i); //clear
			}
		}
	}
}

void eventSystem::worker(void(*foo)(void), int num) {
	while(true) {
		//Wait for this continue == 1
		while((cont >> num) & 1 == 0);

		//Calls our function pointer
		foo();

		/*For testing purposes we return since we don't want it to be an infinite loop
		 We want this to be an infinite loop in real world usage since it will be run
		 when a button is pressed or the user specifies an input that toggles it
		*/
		return;
		//Sets this continue to 0
		cont &= ~(1 << num);
	}
}

void eventSystem::checkFunction() {
	for(auto&& function : functions)
		function.get();
}

void eventSystem::addFunction(void(*foo)(void)) {
	functions.emplace_back(async(launch::async, &eventSystem::worker, this, foo, functions.size()));
}

//Starts the coordinator and workers
void eventSystem::execute() {
	coordinator = async(launch::async, &eventSystem::driver, this);
}

void eventSystem::callFunction(int num) {
	arrive |= 1 << num; //set
}

eventSystem::events() {
	cont = 0;
	arrive = 0;
}