from pythonThreading import *

def function_1(val_1, val_2, val_3):
        value = val_3
        for x in range(1, val_1):
                for y in range (1, val_2):
                        value = value + x + y
        print "function 1 says :", value
	aSemaphore.signal()

def function_2(val_1, val_2, val_3):
        value = val_3
        for x in range(1, val_1):
                for y in range( 1, val_2):
                        value = value + x + y

	aSemaphore.wait()
        print "function 2 says :", value

aSemaphore = Semaphore(0)

Thread(function_1, 1000, 1000, 3.6)

Thread(function_2, 100, 100, 3.6)

