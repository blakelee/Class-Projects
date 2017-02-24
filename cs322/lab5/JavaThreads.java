// File : ThreadExample.java
// Author : Filip Jagodzinski

// import statements
import java.util.concurrent.*;
import java.util.*;

// class declaration
public class JavaThreads {

    // main method
    public static void main(String[] args) {

	// Variables. The chunkSize variable is the number of
	// dim divided equally by the number of threads
	int numDims = 10000;
	int numThreads = 1;
	int chunkSize = numDims/numThreads;
	double timeStart = 0;
	double timeEnd = 0;
	
	// create a 2D array of ints; all unique nums
	final double[][] anArray = new double[numDims][numDims];
	for (int i=0; i<anArray.length; i++){
	    for (int j=0; j<anArray[0].length; j++){
		anArray[i][j] = i + j;
	    }
	}
	
	// Threaded; must be in a try catch statement because the
	// ExecutorService is an interface, so all of its abstract
	// methods must be overriden, and its run() method MUST
	// throw an exception of type InterruptedException
	try{

	    // ExecutorService is a subclass (child) of Executors,
	    // a class that provides methods to manage threads. The
	    // newFixedThreadPool method sets the number of threads
	    // that are desired.	    
	    ExecutorService ex = Executors.newFixedThreadPool(numThreads);

	    // get clock time
	    timeStart = System.nanoTime();
	    
	    // create some number of threads
	    for (int i = 0; i < numThreads; i++) {
		
		// for each iteration of the body's for loop,
		// calculate the starting and ending indexes
		// of the ith chunk of the y dimension of
		// the anArray array
		final int indexStart = i * chunkSize;
		final int indexEnd = (i + 1) * chunkSize;
		
		// each "execute" method of the Executor class
		// creates a new object of type Runnable ...
		// be careful with the parentheses here ... the
		// entire next code block is being passes
		// as a parameter to the execute method
		ex.execute(new Runnable() {
			
			// The run() method is declared abstract in the Runnable
			// class, so it MUST be overriden. The body of the
			// run method is the "code" that each thread executes
			@Override
			public void run() {
			    
			    for (int j=0; j<anArray.length; j++){
				for (int k = indexStart; k < indexEnd; k++)
				    anArray[j][k] = anArray[j][k] * anArray[j][k] / 3.45 * Math.sqrt(anArray[j][k]);
			    } // end for

			} // end run
		    }
		    );
	    } //end for

	    // shut down ExecutorServices. If you issue this command, any and all
	    // threads that are managed by THIS executor are killed
	    ex.shutdown();
	    ex.awaitTermination(1, TimeUnit.MINUTES);

	    // get clock time
	    timeEnd = System.nanoTime();
	    
	    
	}catch (InterruptedException e){

	    // print out custom error messages
	    System.out.println("Something went wrong with the threading.");
	    System.out.println("Sorry, quitting");
	    System.out.println("Inspect the stack to see what went wrong");
	    
	} // end try-catch

	// Print timing results
	System.out.println("\nNum of threads      " + numThreads);
	System.out.println("Time to completion  " + ((timeEnd - timeStart)/1000000000.0) + "\n");
	
    } // end main
} // end class
