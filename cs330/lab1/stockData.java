import java.util.*;
import java.io.*;

public class stockData {

	public static void main (String[] args) {
		String sym, date;
		Double open, close;
		int count = 0;

		open = close = null;
		sym = date = "";
		Scanner s = null;
		try {
			s = new Scanner(new File("stockdata.txt"));
		} catch (FileNotFoundException e1) {
			System.err.println("FILE NOT FOUND: stockdata.txt");
			System.exit(2);
		}

		while(s.hasNext()) {
			String tsym = s.next();
			String tdate = s.next();
			Double topen = s.nextDouble();
			s.next(); //Skip high price
			s.next(); //Skip low price
			Double tclose = s.nextDouble();
			s.next(); //Skip volume
			s.next(); //Skip adjusted closing price

			//Process new symbol
			if (!tsym.equals(sym)) {
				if (!sym.equals(""))
					System.out.println("splits: " + count + "\n");

				System.out.println("Processing " + tsym + "...");
				count = 0;
			}
			else if (compare(date, close, topen))
					count++;

			sym = tsym;
			date = tdate;
			open = topen;
			close = tclose;
		}
	}

	private static boolean compare(String date, Double closing, Double opening) {
		if(Math.abs((closing/opening) - 1.5) < 0.075) {
			System.out.println("3:2 split on " + date + " " + opening + " --> " + closing);
			return true;
		}
		if(Math.abs((closing/opening) - 2) < 0.10) {
			System.out.println("2:1 split on " + date + " " + opening + " --> " + closing);
			return true;
		}
		if (Math.abs((closing/opening) - 3 ) < 0.15) {
			System.out.println("3:1 split on " + date + " " + opening + " --> " + closing);
			return true;
		}
		
		return false;
	}
}