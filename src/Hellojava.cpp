#include <iostream>
#include <string>
#include <stdexcept>
#include <memory>
#include <iomanip> // For formatting output (like currency)
#include <limits>  // Required for numeric_limits

// MySQL Connector/C++ specific headers
#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/exception.h>

class ElectricityBillingSystem {
private:
    sql::Driver* driver;
    std::unique_ptr<sql::Connection> conn;

    // Helper function to calculate the bill amount based on tiered rates
    double calculateBill(int units) {
        double amount = 0;
        if (units <= 100) {
            amount = units * 5.0;
        } else if (units <= 300) {
            amount = 100 * 5.0 + (units - 100) * 7.0;
        } else {
            amount = 100 * 5.0 + 200 * 7.0 + (units - 300) * 10.0;
        }
        return amount;
    }

public:
    // Constructor: Establishes the database connection and sets up tables
    ElectricityBillingSystem() {
        try {
            driver = get_mysql_driver_instance();
            
            // First, connect to the server without a specific database to create it
            std::unique_ptr<sql::Connection> tempConn(driver->connect("tcp://127.0.0.1:3306", "root", "root"));
            std::unique_ptr<sql::Statement> stmt(tempConn->createStatement());
            stmt->execute("CREATE DATABASE IF NOT EXISTS electricity");

            // Now, connect to the 'electricity' database
            conn.reset(driver->connect("tcp://127.0.0.1:3306", "root", "root"));
            conn->setSchema("electricity");

            // Create tables if they don't exist
            std::unique_ptr<sql::Statement> tableStmt(conn->createStatement());
            tableStmt->execute(
                "CREATE TABLE IF NOT EXISTS customers ("
                "meter_no VARCHAR(20) PRIMARY KEY, "
                "name VARCHAR(100), "
                "address VARCHAR(200), "
                "email VARCHAR(100))"
            );
            tableStmt->execute(
                "CREATE TABLE IF NOT EXISTS `usage` ("
                "meter_no VARCHAR(20), "
                "month VARCHAR(20), "
                "units_consumed INT, "
                "PRIMARY KEY (meter_no, month), "
                "FOREIGN KEY (meter_no) REFERENCES customers(meter_no))"
            );
            
            std::cout << "Database connection successful." << std::endl;

        } catch (sql::SQLException &e) {
            // Handle connection errors gracefully
            std::cerr << "ERROR: Database connection failed. " << e.what() << std::endl;
            exit(1); // Exit if we can't connect to the DB
        }
    }

    void addCustomer() {
        std::string meterNo, name, address, email;
        std::cout << "Enter Meter No : ";
        std::getline(std::cin, meterNo);
        std::cout << "Enter Name     : ";
        std::getline(std::cin, name);
        std::cout << "Enter Address  : ";
        std::getline(std::cin, address);
        std::cout << "Enter Email    : ";
        std::getline(std::cin, email);

        try {
            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement("INSERT INTO customers VALUES (?, ?, ?, ?)"));
            pstmt->setString(1, meterNo);
            pstmt->setString(2, name);
            pstmt->setString(3, address);
            pstmt->setString(4, email);
            pstmt->executeUpdate();
            std::cout << "✅ Customer added successfully." << std::endl;
        } catch (sql::SQLException &e) {
            std::cerr << "ERROR: Could not add customer. " << e.what() << std::endl;
        }
    }

    void recordUsage() {
        std::string meterNo, month;
        int units;
        std::cout << "Enter Meter No : ";
        std::getline(std::cin, meterNo);
        std::cout << "Enter Month    : ";
        std::getline(std::cin, month);
        std::cout << "Enter Units    : ";
        std::cin >> units;
        // Clear the input buffer after reading a number
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        try {
            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement("INSERT INTO `usage` VALUES (?, ?, ?)"));
            pstmt->setString(1, meterNo);
            pstmt->setString(2, month);
            pstmt->setInt(3, units);
            pstmt->executeUpdate();
            std::cout << "✅ Usage recorded." << std::endl;
        } catch (sql::SQLException &e) {
            std::cerr << "ERROR: Could not record usage. " << e.what() << std::endl;
        }
    }
    
    void generateBill() {
        std::string meterNo, month;
        std::cout << "Enter Meter No : ";
        std::getline(std::cin, meterNo);
        std::cout << "Enter Month    : ";
        std::getline(std::cin, month);

        try {
            // Fetch usage details first
            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement("SELECT units_consumed FROM `usage` WHERE meter_no=? AND month=?"));
            pstmt->setString(1, meterNo);
            pstmt->setString(2, month);
            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

            if (res->next()) {
                int units = res->getInt("units_consumed");
                double amount = calculateBill(units);

                // Now fetch customer details
                std::unique_ptr<sql::PreparedStatement> custPstmt(conn->prepareStatement("SELECT name, address, email FROM customers WHERE meter_no=?"));
                custPstmt->setString(1, meterNo);
                std::unique_ptr<sql::ResultSet> custRes(custPstmt->executeQuery());

                std::string name = "N/A", address = "N/A", email = "N/A";
                if (custRes->next()) {
                    name = custRes->getString("name");
                    address = custRes->getString("address");
                    email = custRes->getString("email");
                }
                
                // Additional fixed charges and taxes
                double fixedCharge = 50.0;
                double tax = amount * 0.05;
                double total = amount + fixedCharge + tax;

                // Print the formatted bill
                std::cout << "\n📄  Electricity Bill" << std::endl;
                std::cout << "--------------------------------------" << std::endl;
                std::cout << std::left << std::setw(15) << "Meter No" << ": " << meterNo << std::endl;
                std::cout << std::left << std::setw(15) << "Name" << ": " << name << std::endl;
                std::cout << std::left << std::setw(15) << "Address" << ": " << address << std::endl;
                std::cout << std::left << std::setw(15) << "Month" << ": " << month << std::endl;
                std::cout << std::left << std::setw(15) << "Units Consumed" << ": " << units << std::endl;
                std::cout << "--------------------------------------" << std::endl;
                // Use fixed and setprecision for clean currency formatting
                std::cout << std::fixed << std::setprecision(2);
                std::cout << std::left << std::setw(15) << "Base Amount" << ": Rs. " << amount << std::endl;
                std::cout << std::left << std::setw(15) << "Fixed Charge" << ": Rs. " << fixedCharge << std::endl;
                std::cout << std::left << std::setw(15) << "Tax (5%)" << ": Rs. " << tax << std::endl;
                std::cout << "--------------------------------------" << std::endl;
                std::cout << std::left << std::setw(15) << "Total Amount" << ": Rs. " << total << std::endl;
                std::cout << "--------------------------------------" << std::endl;

            } else {
                std::cout << "⚠️  No usage record found for the given meter and month." << std::endl;
            }

        } catch (sql::SQLException &e) {
            std::cerr << "ERROR: Could not generate bill. " << e.what() << std::endl;
        }
    }

    // The main loop for the command-line interface
    void run() {
        int choice = 0;
        while (choice != 4) {
            std::cout << "\n===== Electricity Bill Generator =====" << std::endl;
            std::cout << "1. Add Customer" << std::endl;
            std::cout << "2. Record Usage" << std::endl;
            std::cout << "3. Generate Bill" << std::endl;
            std::cout << "4. Exit" << std::endl;
            std::cout << "Choose option: ";
            std::cin >> choice;

            // Important: After reading a number with cin, we need to clear the newline
            // character from the input buffer before we can use getline successfully.
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

            switch (choice) {
                case 1:
                    addCustomer();
                    break;
                case 2:
                    recordUsage();
                    break;
                case 3:
                    generateBill();
                    break;
                case 4:
                    std::cout << "👋 Exiting..." << std::endl;
                    break;
                default:
                    std::cout << "⚠️  Invalid option. Please try again." << std::endl;
                    break;
            }
        }
    }
};

int main() {
    try {
        ElectricityBillingSystem billingSystem;
        billingSystem.run();
    } catch (const std::exception& e) {
        std::cerr << "An unexpected error occurred: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
