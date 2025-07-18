import java.sql.*;
import java.time.LocalDate;
import java.time.format.DateTimeFormatter;
import java.util.Scanner;

public class Hellojava {
    static Connection conn;
    public static void connectDB() throws Exception {
        Class.forName("com.mysql.cj.jdbc.Driver");
        // Connect to MySQL server (no DB selected)
        conn = DriverManager.getConnection(
            "jdbc:mysql://localhost:3306/?user=root&password=root", "root", "root"
        );
        Statement stmt = conn.createStatement();
        stmt.executeUpdate("CREATE DATABASE IF NOT EXISTS electricity");
        conn.close();

        // Now connect to the electricity database
        conn = DriverManager.getConnection(
            "jdbc:mysql://localhost:3306/electricity", "root", "root"
        );

        // Create tables in the electricity database
        Statement tableStmt = conn.createStatement();
        tableStmt.executeUpdate(
            "CREATE TABLE IF NOT EXISTS customers (" +
            "meter_no VARCHAR(20) PRIMARY KEY, " +
            "name VARCHAR(100), " +
            "address VARCHAR(200), " +
            "email VARCHAR(100))"
        );
        tableStmt.executeUpdate(
            "CREATE TABLE IF NOT EXISTS `usage` (" +
            "meter_no VARCHAR(20), " +
            "month VARCHAR(20), " +
            "units_consumed INT, " +
            "PRIMARY KEY (meter_no, month), " +
            "FOREIGN KEY (meter_no) REFERENCES customers(meter_no))"
        );
    }

    public static void addCustomer(String meterNo, String name, String address, String email) throws SQLException {
        String sql = "INSERT INTO customers VALUES (?, ?, ?, ?)";
        PreparedStatement stmt = conn.prepareStatement(sql);
        stmt.setString(1, meterNo);
        stmt.setString(2, name);
        stmt.setString(3, address);
        stmt.setString(4, email);
        stmt.executeUpdate();
        System.out.println("✅ Customer added successfully.");
    }

    public static void recordUsage(String meterNo, String month, int units) throws SQLException {
        String sql = "INSERT INTO `usage` VALUES (?, ?, ?)";
        PreparedStatement stmt = conn.prepareStatement(sql);
        stmt.setString(1, meterNo);
        stmt.setString(2, month);
        stmt.setInt(3, units);
        stmt.executeUpdate();
        System.out.println("✅ Usage recorded.");
    }

    public static void generateBill(String meterNo, String month) throws SQLException {
        String usageQuery = "SELECT units_consumed FROM `usage` WHERE meter_no=? AND month=?";
        PreparedStatement stmt = conn.prepareStatement(usageQuery);
        stmt.setString(1, meterNo);
        stmt.setString(2, month);
        ResultSet rs = stmt.executeQuery();

        if (rs.next()) {
            int units = rs.getInt("units_consumed");
            double amount = calculateBill(units);

            // Fetch customer details
            String customerQuery = "SELECT name, address, email FROM customers WHERE meter_no=?";
            PreparedStatement custStmt = conn.prepareStatement(customerQuery);
            custStmt.setString(1, meterNo);
            ResultSet custRs = custStmt.executeQuery();

            String name = "", address = "", email = "";
            if (custRs.next()) {
                name = custRs.getString("name");
                address = custRs.getString("address");
                email = custRs.getString("email");
            }

            // Get current date
            String date = LocalDate.now().format(DateTimeFormatter.ofPattern("dd-MM-yyyy"));

            // Additional options
            double fixedCharge = 50.0;
            double tax = amount * 0.05;
            double total = amount + fixedCharge + tax;

            System.out.println("\n📄 Electricity Bill");
            System.out.println("Date         : " + date);
            System.out.println("Meter No     : " + meterNo);
            System.out.println("Name         : " + name);
            System.out.println("Address      : " + address);
            System.out.println("Email        : " + email);
            System.out.println("Month        : " + month);
            System.out.println("Units        : " + units);
            System.out.println("Base Amount  : ₹" + amount);
            System.out.println("Fixed Charge : ₹" + fixedCharge);
            System.out.println("Tax (5%)     : ₹" + String.format("%.2f", tax));
            System.out.println("Total Amount : ₹" + String.format("%.2f", total));
            System.out.println("--------------------------------------");
            System.out.println("Thank you for using our service!");
        } else {
            System.out.println("⚠️ No usage record found for given month.");
        }
    }

    public static double calculateBill(int units) {
        double amount = 0;
        if (units <= 100) {
            amount = units * 5;
        } else if (units <= 300) {
            amount = 100 * 5 + (units - 100) * 7;
        } else {
            amount = 100 * 5 + 200 * 7 + (units - 300) * 10;
        }
        return amount;
    }

    public static void main(String[] args) {
        try {
            connectDB();
            Scanner sc = new Scanner(System.in);

            while (true) {
                System.out.println("\n===== Electricity Bill Generator =====");
                System.out.println("1. Add Customer");
                System.out.println("2. Record Usage");
                System.out.println("3. Generate Bill");
                System.out.println("4. Exit");
                System.out.print("Choose option: ");
                int choice = sc.nextInt();
                sc.nextLine();

                if (choice == 1) {
                    System.out.print("Meter No : ");
                    String meterNo = sc.nextLine();
                    System.out.print("Name     : ");
                    String name = sc.nextLine();
                    System.out.print("Address  : ");
                    String address = sc.nextLine();
                    System.out.print("Email    : ");
                    String email = sc.nextLine();
                    addCustomer(meterNo, name, address, email);

                } else if (choice == 2) {
                    System.out.print("Meter No : ");
                    String meterNo = sc.nextLine();
                    System.out.print("Month    : ");
                    String month = sc.nextLine();
                    System.out.print("Units    : ");
                    int units = sc.nextInt();
                    recordUsage(meterNo, month, units);

                } else if (choice == 3) {
                    System.out.print("Meter No : ");
                    String meterNo = sc.nextLine();
                    System.out.print("Month    : ");
                    String month = sc.nextLine();
                    generateBill(meterNo, month);

                } else if (choice == 4) {
                    System.out.println("👋 Exiting...");
                    break;
                } else {
                    System.out.println("⚠️ Invalid option.");
                }
            }
            sc.close();
            conn.close();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}