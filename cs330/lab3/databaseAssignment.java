import java.util.Properties;
import java.io.FileInputStream;
import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.util.*;


/** This program compares the values of stock with each other to get
 * returns on the potential investments. It reads the information from
 * one database and writes it to another database
 * 
 * @author 	Blake Oliveira
 * @version	1.0 March 13, 2016
 */

class databaseAssignment {

	static Connection reader = null;
	static Connection writer = null;
	static int INTERVAL = 60;

	/** Gets the parameters for the readers and writers and
	 * forwards it to connect
	 * @param args		optional two parameters. 
	 * 					readers and writers respectively.
	 * 					defaults if they're not supplied
	 * @throws Exception
	 * @return void
	 */
	public static void main(String[] args) throws Exception {

		String readersFile = "readersparams.txt";
		String writersFile = "writersparams.txt";
		try {
		if (args.length >= 1) 
            readersFile = args[0];
        if (args.length >= 2) 
			writersFile = args[1];
		} catch (Exception ex){
        	System.exit(0);
        }
		
        try {
        	connect(readersFile, writersFile);
        } catch (Exception ex){
        	System.exit(0);
        }
	}

	static class Output {
		public String Industry;
		public String Ticker;
		public String StartDate;
		public String EndDate;
		public Double TickerReturn;
		public Double IndustryReturn;
		public void add(String Industry, String Ticker, String StartDate, String EndDate, Double TickerReturn, Double IndustryReturn) {
			this.Industry = Industry;
			this.Ticker = Ticker;
			this.StartDate = StartDate;
			this.EndDate = EndDate;
			this.TickerReturn = TickerReturn;
			this.IndustryReturn = IndustryReturn;
		}
	}
	
	/** Performs the main operations of finding the stock values
	 * This includes handling the logic of reading from one and inserting
	 * into the other.
	 * @param readersFile	The parameters for the readers database
	 * @param writersFile	The parameters for the writers database
	 * @throws Exception
	 */
	static void connect(String readersFile, String writersFile) throws Exception {
		Properties readersprops = new Properties();
		Properties writersprops = new Properties();

		try {
			try {
				readersprops.load(new FileInputStream(readersFile));
				writersprops.load(new FileInputStream(writersFile));
			}
			catch (Exception ex) {
				System.out.printf("Couldn't find readersparams.txt or writersparams.txt%n");
				System.exit(0);
			}
            // Get connection
            Class.forName("com.mysql.jdbc.Driver");
            String dburl = readersprops.getProperty("dburl");
            String username = readersprops.getProperty("user");
            reader = DriverManager.getConnection(dburl, readersprops);
            System.out.printf("Database connection %s %s established.%n", dburl, username);
            
            dburl = writersprops.getProperty("dburl");
            username = writersprops.getProperty("user");
            writer = DriverManager.getConnection(dburl, readersprops);
            System.out.printf("Database connection %s %s established.%n", dburl, username);
            

            createTable();
            Intervals();

			System.out.println("Database connection closed");
			reader.close();
            writer.close();

        } catch (SQLException ex) {
            System.out.printf("SQLException: %s%nSQLState: %s%nVendorError: %s%n",
                                    ex.getMessage(), ex.getSQLState(), ex.getErrorCode());
        } 
	}

	
	
	/** Drops the Performance table and recreates it inside the writers
	 * The purpose for this is to have fresh data every time
	 * the table is parsed.
	 * @throws Exception
	 */
	static void createTable() throws Exception {

		PreparedStatement pstmt = writer.prepareStatement(
                "DROP TABLE IF EXISTS Performance");
		pstmt.executeUpdate();

		pstmt = writer.prepareStatement(
				"CREATE TABLE Performance (" +
                "Industry VARCHAR(50), " +
                "Ticker CHAR(12), " +
				"StartDate CHAR(10), " +
				"EndDate CHAR(10), " +
				"TickerReturn NUMERIC(12, 7), " +
				"IndustryReturn NUMERIC(12, 7))");
		
		pstmt.executeUpdate();
		
		System.out.println("Created Table");

	}
	
	
	
	/** Compares each stock in an industry group with the group as a whole.
	 * Each stock will be compared to a "basket" of stock which consist of 
	 * other stocks in the industry group.
	 * @param void
	 * @return void
	 * @throws Exception
	 */
	static void compareIndustry() throws Exception {
		try {
		PreparedStatement pstmt;
		pstmt = reader.prepareStatement(
				"select Ticker, min(TransDate), max(TransDate), count(distinct TransDate) " +
				"from Company left outer join PriceVolume using(Ticker) " +
				"where Industry = 'Telecommunications Services' " +
				"group by Ticker " +
				"order by Ticker");
		
		pstmt.executeQuery();
		}
		catch (Exception ex) {
			System.exit(0);
		}
	}
	
	
	
	/** Calculates the trading intervals of 60 consecutive trading days
	 * @param	void
	 * @return	void
	 * @throws 	Exception
	 */
	static void Intervals() throws Exception {

		PreparedStatement pstmt;
		//This is a list of industries with a list of ticker with their list of outputs
		List<List<List<Output>>> output = new ArrayList<List<List<Output>>>();
		
		//Gets the list of industries
		ResultSet industryRS;
		pstmt = reader.prepareStatement(
			"SELECT DISTINCT Industry as Industry " +
			"FROM Company ");
		
		industryRS = pstmt.executeQuery();	
		
		//For each industry get the trading and industry results
		while(industryRS.next()) {
			String Industry = industryRS.getString("Industry");
			
			//Date ranges for the tickers per industry
			String minDate = minmaxDate(Industry, false);
			String maxDate = minmaxDate(Industry, true);
			
			//Gets only the tickers that have at least 150 trading days
			ResultSet rs = getCountAll(Industry, minDate, maxDate);
		
			//Gets the list of tickers
			List<String> tickers = new ArrayList<String>();
			int count = 0;
			while(rs.next()){
				tickers.add(rs.getString("Ticker"));
				count = rs.getInt("TradingDays");
			}
			
			List<List<Output>> tickerOut = new ArrayList<List<Output>>();
			//For each ticker get the trading interval
			for(String ticker : tickers)
				tickerOut.add(TickerReturns(Industry, ticker, count));

			output.add(tickerOut);
		}

		InsertIntoTable(output);
	}
	
	/** Goes through our list of industries, through the list of tickers in the industries,
	 * through the data of each ticker and inserts in into the performance table.
	 * @param output
	 * @throws SQLException
	 */
	static void InsertIntoTable(List<List<List<Output>>> output) throws SQLException {
		PreparedStatement pstmt = writer.prepareStatement(
				"INSERT INTO Performance " +
				"(Industry, Ticker, StartDate, EndDate, TickerReturn, IndustryReturn) " +
				"VALUES " +
				"(?, ?, ?, ?, ?, ?); ");
		
		for(List<List<Output>> industry : output) {
			for(List<Output> ticker : industry) {
				for(Output data : ticker) {
					pstmt.setString(1, data.Industry);
					pstmt.setString(2, data.Ticker);
					pstmt.setString(3, data.StartDate);
					pstmt.setString(4, data.EndDate);
					pstmt.setDouble(5, data.TickerReturn);
					pstmt.setDouble(6, data.IndustryReturn);
					pstmt.executeUpdate();
				}
			}
		}
	}
	
	static List<Output> TickerReturns (String Industry, String Ticker, int count) throws SQLException {

		PreparedStatement pstmt;
		List<Output> tickerOut = new ArrayList<Output>();
		ResultSet tickerRS;
		
		String interval = numInterval(count);
		pstmt = reader.prepareStatement("SET @rank:=0");
		pstmt.executeQuery();
		
		pstmt = reader.prepareStatement(
		"SELECT * " +
		"FROM ( " +
			"SELECT " +
			"Ticker, TransDate, OpenPrice, ClosePrice, @rank:=@rank+1 AS rank " +
			"FROM PriceVolume " +
			"WHERE Ticker = ? " +
			"ORDER BY TransDate ASC, Ticker " +
			") AS TMP " +
		"WHERE " + interval);
		pstmt.setString(1, Ticker);
		
		tickerRS = pstmt.executeQuery();
		
		
		//Parse the resultSet
		while(tickerRS.next()) {
			String tickTemp = tickerRS.getString("Ticker");
			String startDate = tickerRS.getString("TransDate");
			Double openPrice = tickerRS.getDouble("OpenPrice");
			tickerRS.next();
			String endDate = tickerRS.getString("TransDate");
			Double closePrice = tickerRS.getDouble("ClosePrice");
			Output op = new Output();
			op.add(Industry, tickTemp, startDate, endDate, (closePrice/openPrice) - 1, 0.0);
			tickerOut.add(op);
		}
		
		return tickerOut;
	}
	
	
	static String numInterval(int count) {
		
		//Gets the floor of the interval
		int interval = count / INTERVAL;
		String selection = "";
		
		for(int i = 0; i < interval; i++){
			//This is the opening price and closing price
			if (i == interval - 1)
				selection += " rank = " + (i * INTERVAL + 1) + " OR rank = " + (i * INTERVAL + INTERVAL);
			else
				selection += " rank = " + (i * INTERVAL + 1) + " OR rank = " + (i * INTERVAL + INTERVAL) + " OR";
		}
		return selection;
	}
	
	
	/** Gets the ResultSet of all tickers in an industry following the criteria
	 * @param Industry
	 * @return the ResultSet
	 * @throws SQLException 
	 */
	static ResultSet getCountAll(String Industry, String minDate, String maxDate) throws SQLException {
			

			PreparedStatement pstmt;
			pstmt = reader.prepareStatement(
				"select DISTINCT Ticker, COUNT(DISTINCT TransDate) as TradingDays " +
				"from Company join PriceVolume using(Ticker) " +
				"where Industry = ? " +
				"and TransDate between ? and ? " +
				"GROUP BY Ticker " +
				"HAVING TradingDays >= 150 " + 
				"order by Ticker ");
			
			
			pstmt.setString(1, Industry);
			pstmt.setString(2, minDate);
			pstmt.setString(3, maxDate);
			return pstmt.executeQuery();
	}
	
	
	/** Generates the returns for a given period of time
	 * 
	 */
	static void IndustryReturns(String Industry, String Ticker) throws SQLException {

		PreparedStatement pstmt;
		pstmt = reader.prepareStatement(
				""
				);
		pstmt.executeQuery();
	}
	
	
	
	
	/** Gets the min of max or max of min date of all the dates for a given Industry
	 * 
	 * @param Industry
	 * @param type		if true then when gets the max else min
	 * @return
	 * @throws Exception
	 */
	static String minmaxDate (String Industry, boolean type) throws SQLException {

		String s1 = "MAX";
		String s2 = "MIN";
		if (type) {
			s1 = "MIN";
			s2 = "MAX";
		}

		PreparedStatement pstmt;
		pstmt = reader.prepareStatement(
		"SELECT " + s1 + "(Date) AS newDate " +		
		"FROM (SELECT " + s2 + "(TransDate) AS Date " +
			"FROM Company LEFT OUTER JOIN PriceVolume USING (Ticker) " +
			"WHERE Industry = ? " +
			"GROUP BY Ticker " +
			"HAVING COUNT(DISTINCT TransDate) >= 150 " +
			"ORDER BY Ticker) AS t ");
		pstmt.setString(1, Industry);
		
		ResultSet rs = pstmt.executeQuery();
		rs.next(); 	 
		String date = rs.getString("newDate");
		return date;
		
	}
}










