// File : UpToNoGood.java
// Author : Filip Jagodzinski

// imports
import java.util.*;
import java.util.concurrent.*;

// class declaration
public class UpToNoGood {

    // main method declaration
    public static void main(String[] args) {
		
		int numThreads = 8;

		// DO NOT MODIFY -- START
	
		// number of accounts. MUST be 100 million
		int numAccounts = 100000000;
	
		// "simulate" an array of doubles ... accountBalances
		final double[] accountBalances = new double[numAccounts];

		//Moved to a function so I can randomize the non-threaded version as well
		randomize(accountBalances, numAccounts);

		// DO NOT MODIFY -- END	

		String output = "Number of threads";
		System.out.format("%18s %d\n", output, numThreads);
		threaded(accountBalances, numThreads, numAccounts);

		randomize(accountBalances, numAccounts);
		nonThreaded(accountBalances, numAccounts);

    } // end main

	//Do this for each instantiation of accountBalance modifications
	public static void randomize(double[] accountBalances, int numAccounts) {
		for (int i=0; i < numAccounts; i++){
			Random rand = new Random();
			accountBalances[i] = (double) rand.nextInt(10000);
		}
	}

	public static void threaded(double[] accountBalances, int numThreads, int numAccounts) {
		
		int chunkSize = numAccounts/numThreads;

		// get system nanosecond time - start
		double ThreadStart = 0;
		double ThreadEnd = 0;

		try{
 
			ExecutorService ex = Executors.newFixedThreadPool(numThreads);

			// get clock time
			ThreadStart = System.nanoTime();
	    
			// create some number of threads
			for (int i = 0; i < numThreads; i++) {

				final int indexStart = i * chunkSize;
				final int indexEnd = (i + 1) * chunkSize;
		

				ex.execute(new Runnable() {
					@Override
					public void run() {
						// decrease each "account balance" by a tiny amount; x^{0.999}
						for(int i = indexStart; i < indexEnd; i++)
							accountBalances[i] = Math.pow(accountBalances[i], 0.999);
					} // end run
				} );
			} //end for

			// shut down ExecutorServices. If you issue this command, any and all
			// threads that are managed by THIS executor are killed
			ex.shutdown();
			ex.awaitTermination(1, TimeUnit.MINUTES);

			// get clock time
			ThreadEnd = System.nanoTime();
	    
		} catch (InterruptedException e){

			System.out.println("Something went wrong with the threading.");
			System.out.println("Sorry, quitting");
			System.out.println("Inspect the stack to see what went wrong");
		}

		// output the time needed to perform calculations
		String output = "Threaded time";
		System.out.format("%18s %f\n", output, (ThreadEnd - ThreadStart) / 1000000000.0 );
	}

	public static void nonThreaded(double[] accountBalances, int numAccounts) {
		
		double ThreadStart = 0;
		double ThreadEnd = 0;

		ThreadStart = System.nanoTime();

		for(int i = 0; i < numAccounts; i++)
			accountBalances[i] = Math.pow(accountBalances[i], 0.999);

		ThreadEnd = System.nanoTime();

		String output = "Non-threaded time";
		System.out.format("%18s %f\n", output, (ThreadEnd - ThreadStart) / 1000000000.0); 
	}
}