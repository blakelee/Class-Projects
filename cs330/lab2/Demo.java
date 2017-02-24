import java.util.Properties;
import java.util.Scanner;
import java.io.FileInputStream;
import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;
import java.sql.*;

class Demo {

    static Connection conn = null;

    public static void main(String[] args) throws Exception {
        // Get connection properties
        String paramsFile = "connectparams.txt";
        if (args.length >= 1) {
            paramsFile = args[0];
        }
        Properties connectprops = new Properties();
        connectprops.load(new FileInputStream(paramsFile));

        try {
            // Get connection
            Class.forName("com.mysql.jdbc.Driver");
            String dburl = connectprops.getProperty("dburl");
            String username = connectprops.getProperty("user");
            conn = DriverManager.getConnection(dburl, connectprops);
            System.out.printf("Database connection %s %s established.%n", dburl, username);

            //showCompanies();

            // Enter Ticker and TransDate, Fetch data for that ticker and date
            Scanner in = new Scanner(System.in);
            //while (true) {
					runQuery("SELECT * FROM ( SELECT Ticker, TransDate, ROW_NUMBER() OVER(ORDER BY ID) AS ROW FROM PriceVolume WHERE Industry = 'Telecommunications Services' ) AS TMP WHERE ROW = 0 OR ROW = 59");
				//runQuery(data);
				/*
                String[] data = in.nextLine().trim().split("\\s+");
                if (data.length < 2)
                    break;
                showTickerDay(data[0], data[1]);*/
            //}

            //conn.close();
        } catch (SQLException ex) {
            System.out.printf("SQLException: %s%nSQLState: %s%nVendorError: %s%n",
                                    ex.getMessage(), ex.getSQLState(), ex.getErrorCode());
        }
    }

	static void runQuery(String statement) throws SQLException {
		PreparedStatement pstmt = conn.prepareStatement(statement);
		ResultSet rs = pstmt.executeQuery();
		ResultSetMetaData rsmd = rs.getMetaData();
		 int columnsNumber = rsmd.getColumnCount();
		while (rs.next()) {
			for (int i = 1; i <= columnsNumber; i++) {
				if (i > 1) System.out.print(",  ");
				String columnValue = rs.getString(i);
				System.out.print(columnValue + " " + rsmd.getColumnName(i));
			}
        System.out.println("");
		}
	}

	static void runUpdate (String statement) throws SQLException {
		PreparedStatement pstmt = conn.prepareStatement(statement);
		pstmt.executeUpdate();
	}

    static void showCompanies() throws SQLException {
        // Create and execute a query
        Statement stmt = conn.createStatement();
        ResultSet results = stmt.executeQuery("select Ticker, Name from Company");

        // Show results
        while (results.next()) {
            System.out.printf("%5s %s%n", results.getString("Ticker"), results.getString("Name"));
        }
        stmt.close();
    }

    static void showTickerDay(String ticker, String date) throws SQLException {
        // Prepare query
        PreparedStatement pstmt = conn.prepareStatement(
                "select OpenPrice, HighPrice, LowPrice, ClosePrice " +
                "  from PriceVolume " +
                "  where Ticker = ? and TransDate = ?");

        // Fill in the blanks
        pstmt.setString(1, ticker);
        pstmt.setString(2, date);
        ResultSet rs = pstmt.executeQuery();

        // Did we get anything? If so, output data.
        if (rs.next()) {
            System.out.printf("Open: %.2f, High: %.2f, Low: %.2f, Close: %.2f%n",
                    rs.getDouble(1), rs.getDouble(2), rs.getDouble(3), rs.getDouble(4));
        } else {
            System.out.printf("Ticker %s, Date %s not found.%n", ticker, date);
        }
        pstmt.close();
    }
}