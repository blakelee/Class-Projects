import java.util.Properties;
import java.util.Scanner;
import java.io.FileInputStream;
import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;
import java.util.LinkedList;
import java.util.*;
import java.text.*;

class databaseAssignment {

	static Connection conn = null;

	public static void main(String[] args) throws Exception {

		String paramsFile = "connectparams.txt";
		if (args.length >= 1) {
            paramsFile = args[0];
        }

		connect(paramsFile);
	}

	public static void connect(String paramsFile) throws Exception {
		Properties connectprops = new Properties();
        connectprops.load(new FileInputStream(paramsFile));

        try {
            // Get connection
            Class.forName("com.mysql.jdbc.Driver");
            String dburl = connectprops.getProperty("dburl");
            String username = connectprops.getProperty("user");
            conn = DriverManager.getConnection(dburl, connectprops);
            System.out.printf("Database connection %s %s established.%n", dburl, username);

            // Enter Ticker and TransDate, Fetch data for that ticker and date
            Scanner in = new Scanner(System.in);

			//Section 2
            while (true) {

				//Section 2.1
                System.out.print("Enter ticker symbol [start/end dates]: ");
                String[] data = in.nextLine().trim().split("\\s+");
                
				//Exits if empty string or spaces
				if (data[0].equals(""))
                    break;

				//Check for valid symbol
				if (requestSym(data[0])) {
					
					//Section 2.2
					if (data.length == 3)
						getSplits(data[0], data[1], data[2]);
					else
						getSplits(data[0], "", "");
				}
				else {
					System.out.format("Invalid symbol %s\n", data[0]);
				}
            }

            conn.close();
			System.out.println("Database connection closed");

        } catch (SQLException ex) {
            System.out.printf("SQLException: %s%nSQLState: %s%nVendorError: %s%n",
                                    ex.getMessage(), ex.getSQLState(), ex.getErrorCode());
        }
	}

	//Section 2.2
	static boolean requestSym (String sym) throws SQLException{
		PreparedStatement pstmt = conn.prepareStatement(
                "select Name" +
                "  from Company " +
                "  where Ticker = ?");

		pstmt.setString(1, sym);
		ResultSet rs = pstmt.executeQuery();
		
		if(rs.next()) {
			System.out.format("%s\n", rs.getString("Name"));
			return true;
		}
		
		return false;
	}

	static void getSplits(String sym, String date1, String date2) throws SQLException {
		PreparedStatement pstmt = null;
		int numSplits = 0;
		double split = 1.0;

		//Section 2.3

		//Date modifier
		if (date1.equals("") && date2.equals("")){
			pstmt = conn.prepareStatement(
                "select P.TransDate, P.OpenPrice, P.ClosePrice " +
                "from PriceVolume as P " +
                "where P.Ticker = ? " +
				"order by P.TransDate DESC");
		}

		else {
			pstmt = conn.prepareStatement(
                "select P.TransDate, P.OpenPrice, P.ClosePrice " +
                "from PriceVolume as P " +
                "where P.Ticker = ? AND P.TransDate BETWEEN ? AND ?" +
				"order by P.TransDate DESC");

			pstmt.setString(2, date1);
			pstmt.setString(3, date2);
		}

		pstmt.setString(1, sym);

		ResultSet rs = pstmt.executeQuery();

		//Will be used for average 50-day and splits
		LinkedList<PriceVolume> pv = new LinkedList<PriceVolume>();
		LinkedList<Dividend> dvd = new LinkedList<Dividend>();

		//Parse the results // Section 2.4
		while (rs.next()) {
			//Closing and opening split comparison // Section 2.4
			if(pv.size() > 1) {
				double temp = compare(rs.getString("TransDate"), rs.getDouble("ClosePrice"), pv.get(pv.size()-1).OpenPrice * split);
				split *= temp;

				if (temp > 1.0)
					numSplits++;

			}
			pv.add(new PriceVolume(rs.getString("TransDate"), rs.getDouble("OpenPrice") / split, rs.getDouble("ClosePrice") / split));
		}
		System.out.format("%d splits in %d trading days\n", numSplits, pv.size());

		Collections.reverse(pv);
		trackTrades(pv, date1, date2, sym);
	}

	/* Dividends/Buying/Selling
	 * Keeps track of all trades and investments
	 */
	static void trackTrades(LinkedList<PriceVolume> pv, String date1, String date2, String sym) {
		int numTransactions = 0, curShares = 0, numDividends;
		double average = 0.0, curCash = 0.0;

		LinkedList<Dividend> dvd = null;
		
		try {
			dvd = getDividends(sym, date1, date2);
			numDividends = dvd.size();
			System.out.format("%d Dividends\n\n", numDividends);
		} catch (SQLException ex) {
			System.out.printf("SQLException: %s%nSQLState: %s%nVendorError: %s%n",
            ex.getMessage(), ex.getSQLState(), ex.getErrorCode());
		}

		if(pv.size() > 51) {
			System.out.println("Executing investment strategy");

			//Remove all dividends before the first index since we don't start trading until after day 51
			try {
				while(dvd.size() > 0 && compareDates(pv.get(51).TransDate, dvd.getFirst().DivDate))
					dvd.poll();
			} catch (ParseException ex) {
				System.out.printf("ParseException\n");
			}

			for(int index = 50; index < pv.size() - 2; index++) {
				//Section 2.7, 2.8
				average = calcAverage(pv, index);

				// Get all dividends before transdate and do operations // 2.9.2
				try {
					while(dvd.size() > 0 && compareDates(pv.get(index - 1).TransDate, dvd.getFirst().DivDate)) {
						double amount = dvd.poll().Amount;
						curCash += amount * curShares;
					}
				} catch (ParseException ex) {
					System.out.printf("ParseException\n");
				}

				// 2.9.3
				if (buyShare(pv.get(index).ClosePrice, pv.get(index).OpenPrice, average)) {
					curShares += 100;
					curCash -= 100 * pv.get(index + 1).OpenPrice;
					curCash -= 8.00; //Fee :(
					numTransactions++;
				}
				// 2.9.4
				else if (sellShare(curShares, pv.get(index).OpenPrice, average, pv.get(index - 1).ClosePrice)) {
					curShares -= 100;
					curCash += 100 * ((pv.get(index).OpenPrice + pv.get(index).ClosePrice) / 2);
					curCash -= 8.00; //Fee :(
					numTransactions++;
				}

				// Do nothing // 2.9.5
			}
		}
		else
			return;

		//2.10
		if (curShares > 0) {
			curCash += curShares * pv.get(pv.size() - 1).OpenPrice;
			curShares = 0;
		}

		System.out.format("Transactions executed: %d\n", numTransactions);
		System.out.format("Net cash: %.2f\n", curCash);
	}

	/*Gets the average by iterating through the previous 50 days not including current day
	EXTREMELY INEFFICIENT, BUT IT WORKS! */
	static double calcAverage(LinkedList<PriceVolume> pv, int index) {
		double average = 0.0;

		for(int i = index - 50; i < index; i++)
			average += pv.get(i).ClosePrice;

		average /= 50.0;
		return average;
	}

	/* Returns the list of dividends to compare from
	 */
	static LinkedList<Dividend> getDividends (String sym, String date1, String date2) throws SQLException{
		
		LinkedList<Dividend> dvd = new LinkedList<Dividend>();
		PreparedStatement pstmt = null;
	
		//Date modifier
		if (date1.equals("") && date2.equals("")){
			pstmt = conn.prepareStatement(
            "select D.DivDate, D.Amount " +
            "from Dividend as D " +
			"where D.Ticker = ? " +
			"order by D.DivDate ASC");
		}

		else {
			pstmt = conn.prepareStatement(
            "select D.DivDate, D.Amount " +
            "from Dividend as D " +
            "where D.Ticker = ? AND D.DivDate BETWEEN ? AND ? " +
			"order by D.DivDate ASC");

			pstmt.setString(2, date1);
			pstmt.setString(3, date2);
		}

		pstmt.setString(1, sym);

		ResultSet rs = pstmt.executeQuery();

		while (rs.next())
			dvd.add(new Dividend(rs.getString("DivDate"), rs.getDouble("Amount")));

		return dvd;
	}

	// 2.9.3
	static boolean buyShare (double close, double open, double average) {
		if(close < average && (close/open) < 0.97000001)
			return true;

		return false;
	}

	// current shares, open(d), 50 day average, close(d-1) // 2.9.4
	static boolean sellShare (int curShares, double open, double average, double close) {
		if (curShares >= 100 && open > average && (open / close) > 1.00999999)
			return true;

		return false;
	}

	/* We want to check when to add dividends by making sure dividend date is at least <=
	 * the transdate.
	 */
	static boolean compareDates (String TransDate, String DivDate) throws ParseException{
		SimpleDateFormat format = new SimpleDateFormat("yyyy-MM-dd");
		Date transDate = format.parse(TransDate);
		Date divDate = format.parse(DivDate);

		if(divDate.compareTo(transDate) <= 0)
			return true;

		return false;
	}

	static double compare(String date, Double closing, Double opening) {
		if(Math.abs((closing/opening) - 1.5) < 0.15) {
			System.out.println("3:2 split on " + date + " " + closing + " --> " + opening);
			return 1.5;
		}
		if(Math.abs((closing/opening) - 2) < 0.20) {
			System.out.println("2:1 split on " + date + " " + closing + " --> " + opening);
			return 2.0;
		}
		if (Math.abs((closing/opening) - 3 ) < 0.30) {
			System.out.println("3:1 split on " + date + " " + closing + " --> " + opening);
			return 3.0;
		}
		
		return 1.0;
	}

	public static class PriceVolume {
		public String TransDate;
		public double OpenPrice;
		public double ClosePrice;

		public PriceVolume(String TransDate, double OpenPrice, double ClosePrice) {
			this.TransDate = TransDate;
			this.OpenPrice = OpenPrice;
			this.ClosePrice = ClosePrice;
		}
	}

	public static class Dividend {
		public String DivDate;
		public double Amount;

		public Dividend(String DivDate, double Amount) {
			this.DivDate = DivDate;
			this.Amount = Amount;
		}
	}
}