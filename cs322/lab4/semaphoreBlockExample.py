from pythonThreading import *

aSemaphore = Semaphore(0)

aSemaphore.signal()
aSemaphore.wait()

print "Hello"
