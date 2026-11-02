#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

/*
    HOTEL MANAGEMENT & ROOM RESERVATION SYSTEM
    Language: C++17
    Concepts: OOP, STL vector, file handling, searching, sorting,
              date-range conflict checking, billing, reports.

    Demo project only:
    - Payments are simulated; no real payment gateway is connected.
    - Dates must be entered as YYYY-MM-DD.
*/

const string ROOM_FILE = "rooms.txt";
const string BOOKING_FILE = "bookings.txt";

enum class BookingStatus { Confirmed, Cancelled };

struct DateRange {
    string checkIn;
    string checkOut;
};

class Room {
private:
    int roomNumber{};
    string category;
    int capacity{};
    double pricePerNight{};
    bool active{true};

    friend class HotelManagementSystem;

public:
    Room() = default;

    Room(int number, string type, int guests, double price, bool isActive = true)
        : roomNumber(number), category(std::move(type)), capacity(guests),
          pricePerNight(price), active(isActive) {}

    int getRoomNumber() const { return roomNumber; }
    const string& getCategory() const { return category; }
    int getCapacity() const { return capacity; }
    double getPricePerNight() const { return pricePerNight; }
    bool isActive() const { return active; }

    void setActive(bool value) { active = value; }
    void setPrice(double price) { pricePerNight = price; }

    void display() const {
        cout << left << setw(9) << roomNumber
             << setw(14) << category
             << setw(10) << capacity
             << "Rs. " << fixed << setprecision(2) << setw(12)
             << pricePerNight
             << (active ? "Active" : "Maintenance") << '\n';
    }
};

class Booking {
private:
    int bookingId{};
    int roomNumber{};
    string guestName;
    string phone;
    string checkIn;
    string checkOut;
    int guests{};
    double totalAmount{};
    BookingStatus status{BookingStatus::Confirmed};

public:
    Booking() = default;

    Booking(int id, int room, string name, string contact,
            string in, string out, int guestCount, double amount,
            BookingStatus bookingStatus = BookingStatus::Confirmed)
        : bookingId(id), roomNumber(room), guestName(std::move(name)),
          phone(std::move(contact)), checkIn(std::move(in)),
          checkOut(std::move(out)), guests(guestCount),
          totalAmount(amount), status(bookingStatus) {}

    int getBookingId() const { return bookingId; }
    int getRoomNumber() const { return roomNumber; }
    const string& getGuestName() const { return guestName; }
    const string& getPhone() const { return phone; }
    const string& getCheckIn() const { return checkIn; }
    const string& getCheckOut() const { return checkOut; }
    int getGuests() const { return guests; }
    double getTotalAmount() const { return totalAmount; }
    BookingStatus getStatus() const { return status; }
    bool isConfirmed() const { return status == BookingStatus::Confirmed; }

    void cancel() { status = BookingStatus::Cancelled; }

    void display() const {
        cout << "\nBooking ID : " << bookingId
             << "\nGuest      : " << guestName
             << "\nPhone      : " << phone
             << "\nRoom       : " << roomNumber
             << "\nCheck-in   : " << checkIn
             << "\nCheck-out  : " << checkOut
             << "\nGuests     : " << guests
             << "\nTotal      : Rs. " << fixed << setprecision(2) << totalAmount
             << "\nStatus     : "
             << (isConfirmed() ? "CONFIRMED" : "CANCELLED") << '\n';
    }

    friend class HotelManagementSystem;
};

class HotelManagementSystem {
private:
    vector<Room> rooms;
    vector<Booking> bookings;
    int nextBookingId{1001};

    static string trim(const string& value) {
        size_t first = value.find_first_not_of(" \t\r\n");
        if (first == string::npos) return "";
        size_t last = value.find_last_not_of(" \t\r\n");
        return value.substr(first, last - first + 1);
    }

    static string readLine(const string& prompt) {
        cout << prompt;
        string value;
        getline(cin, value);
        return trim(value);
    }

    static int readInt(const string& prompt, int minValue, int maxValue) {
        while (true) {
            cout << prompt;
            int value;
            if (cin >> value && value >= minValue && value <= maxValue) {
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                return value;
            }
            cout << "Invalid input. Enter a number from "
                 << minValue << " to " << maxValue << ".\n";
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }
    }

    static double readDouble(const string& prompt, double minValue, double maxValue) {
        while (true) {
            cout << prompt;
            double value;
            if (cin >> value && value >= minValue && value <= maxValue) {
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                return value;
            }
            cout << "Invalid amount. Please try again.\n";
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }
    }

    static bool isValidDateFormat(const string& date) {
        if (date.size() != 10 || date[4] != '-' || date[7] != '-') return false;

        for (size_t i = 0; i < date.size(); ++i) {
            if (i == 4 || i == 7) continue;
            if (!isdigit(static_cast<unsigned char>(date[i]))) return false;
        }

        int month = stoi(date.substr(5, 2));
        int day = stoi(date.substr(8, 2));
        int year = stoi(date.substr(0, 4));
        if (year < 2020 || month < 1 || month > 12 || day < 1) return false;

        const int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        int maxDay = daysInMonth[month - 1];
        bool leap = (year % 400 == 0) || (year % 4 == 0 && year % 100 != 0);
        if (month == 2 && leap) maxDay = 29;
        return day <= maxDay;
    }

    static string readDate(const string& prompt) {
        while (true) {
            string date = readLine(prompt);
            if (isValidDateFormat(date)) return date;
            cout << "Use a valid date in YYYY-MM-DD format, e.g. 2026-11-05.\n";
        }
    }

    static int nightsBetween(const string& checkIn, const string& checkOut) {
        // Date format is ISO YYYY-MM-DD, so lexical comparison is chronological.
        // Convert dates to serial day numbers to calculate the duration.
        auto serial = [](int y, unsigned m, unsigned d) -> long long {
            y -= m <= 2;
            const int era = (y >= 0 ? y : y - 399) / 400;
            const unsigned yoe = static_cast<unsigned>(y - era * 400);
            const unsigned mp = static_cast<unsigned>(static_cast<int>(m) + (m > 2 ? -3 : 9));
            const unsigned doy = (153 * mp + 2) / 5 + d - 1;
            const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
            return static_cast<long long>(era) * 146097 + static_cast<long long>(doe);
        };

        int y1 = stoi(checkIn.substr(0, 4));
        unsigned m1 = static_cast<unsigned>(stoi(checkIn.substr(5, 2)));
        unsigned d1 = static_cast<unsigned>(stoi(checkIn.substr(8, 2)));
        int y2 = stoi(checkOut.substr(0, 4));
        unsigned m2 = static_cast<unsigned>(stoi(checkOut.substr(5, 2)));
        unsigned d2 = static_cast<unsigned>(stoi(checkOut.substr(8, 2)));

        return static_cast<int>(serial(y2, m2, d2) - serial(y1, m1, d1));
    }

    Room* findRoom(int number) {
        for (auto& room : rooms)
            if (room.getRoomNumber() == number) return &room;
        return nullptr;
    }

    Booking* findBooking(int id) {
        for (auto& booking : bookings)
            if (booking.getBookingId() == id) return &booking;
        return nullptr;
    }

    bool roomAvailable(int roomNumber, const string& in, const string& out) const {
        for (const auto& booking : bookings) {
            if (!booking.isConfirmed() || booking.getRoomNumber() != roomNumber)
                continue;

            // Overlap exists if new check-in is before old check-out
            // AND new check-out is after old check-in.
            if (in < booking.getCheckOut() && out > booking.getCheckIn())
                return false;
        }
        return true;
    }

    void seedRooms() {
        rooms = {
            Room(101, "Standard", 1, 1200),
            Room(102, "Standard", 2, 1500),
            Room(201, "Deluxe",   2, 2400),
            Room(202, "Deluxe",   3, 2800),
            Room(301, "Suite",    4, 4500),
            Room(302, "Suite",    5, 5200),
            Room(401, "Family",   5, 3800),
            Room(402, "Family",   6, 4200)
        };
    }

    void saveRooms() const {
        ofstream file(ROOM_FILE);
        for (const auto& room : rooms) {
            file << room.roomNumber << ' '
                 << quoted(room.category) << ' '
                 << room.capacity << ' '
                 << room.pricePerNight << ' '
                 << room.active << '\n';
        }
    }

    void saveBookings() const {
        ofstream file(BOOKING_FILE);
        for (const auto& b : bookings) {
            file << b.bookingId << ' ' << b.roomNumber << ' '
                 << quoted(b.guestName) << ' ' << quoted(b.phone) << ' '
                 << b.checkIn << ' ' << b.checkOut << ' '
                 << b.guests << ' ' << b.totalAmount << ' '
                 << static_cast<int>(b.status) << '\n';
        }
    }

    void loadData() {
        ifstream roomFile(ROOM_FILE);
        if (roomFile) {
            rooms.clear();
            int number, capacity;
            string category;
            double price;
            bool active;
            while (roomFile >> number >> quoted(category) >> capacity >> price >> active)
                rooms.emplace_back(number, category, capacity, price, active);
        }
        if (rooms.empty()) {
            seedRooms();
            saveRooms();
        }

        ifstream bookingFile(BOOKING_FILE);
        if (bookingFile) {
            Booking b;
            int status;
            while (bookingFile >> b.bookingId >> b.roomNumber
                   >> quoted(b.guestName) >> quoted(b.phone)
                   >> b.checkIn >> b.checkOut >> b.guests
                   >> b.totalAmount >> status) {
                b.status = (status == 1) ? BookingStatus::Cancelled
                                         : BookingStatus::Confirmed;
                nextBookingId = max(nextBookingId, b.bookingId + 1);
                bookings.push_back(b);
            }
        }
    }

    void printRoomHeader() const {
        cout << left << setw(9) << "Room No."
             << setw(14) << "Category"
             << setw(10) << "Capacity"
             << setw(17) << "Price/Night" << "Status\n";
        cout << string(62, '-') << '\n';
    }

    void showAllRooms() const {
        cout << "\n================ ROOM INVENTORY ================\n";
        printRoomHeader();
        for (const auto& room : rooms) room.display();
    }

    void searchAvailableRooms() const {
        string in = readDate("\nCheck-in date (YYYY-MM-DD): ");
        string out = readDate("Check-out date (YYYY-MM-DD): ");

        if (out <= in) {
            cout << "Check-out must be later than check-in.\n";
            return;
        }

        int guests = readInt("Number of guests (1-10): ", 1, 10);
        double maxBudget = readDouble("Maximum budget per night (Rs.): ", 1, 1000000);

        cout << "\nAvailable rooms for " << in << " to " << out << ":\n";
        printRoomHeader();
        bool found = false;

        for (const auto& room : rooms) {
            if (room.isActive() && room.getCapacity() >= guests &&
                room.getPricePerNight() <= maxBudget &&
                roomAvailable(room.getRoomNumber(), in, out)) {
                room.display();
                found = true;
            }
        }

        if (!found) cout << "No rooms match those requirements.\n";
    }

    void bookRoom() {
        cout << "\n================ CREATE RESERVATION ================\n";
        string in = readDate("Check-in date (YYYY-MM-DD): ");
        string out = readDate("Check-out date (YYYY-MM-DD): ");

        if (out <= in) {
            cout << "Check-out must be later than check-in.\n";
            return;
        }

        int guests = readInt("Number of guests (1-10): ", 1, 10);
        string name = readLine("Guest full name: ");
        if (name.empty()) {
            cout << "Guest name cannot be empty.\n";
            return;
        }

        string phone = readLine("Contact number: ");
        if (phone.empty()) {
            cout << "Contact number cannot be empty.\n";
            return;
        }

        searchAvailableRoomsForBooking(in, out, guests);

        int roomNumber = readInt("Enter room number to reserve (0 to cancel): ", 0, 999999);
        if (roomNumber == 0) {
            cout << "Reservation process cancelled.\n";
            return;
        }

        Room* room = findRoom(roomNumber);
        if (!room || !room->isActive()) {
            cout << "Room not found or currently under maintenance.\n";
            return;
        }
        if (room->getCapacity() < guests) {
            cout << "This room does not have enough capacity.\n";
            return;
        }
        if (!roomAvailable(roomNumber, in, out)) {
            cout << "Room is already reserved for part of those dates.\n";
            return;
        }

        int nights = nightsBetween(in, out);
        double subtotal = room->getPricePerNight() * nights;
        double tax = subtotal * 0.12;
        double total = subtotal + tax;

        cout << "\n---------------- BILL SUMMARY ----------------\n";
        cout << "Room number       : " << roomNumber << '\n';
        cout << "Category          : " << room->getCategory() << '\n';
        cout << "Nights            : " << nights << '\n';
        cout << "Room charges      : Rs. " << fixed << setprecision(2) << subtotal << '\n';
        cout << "Estimated tax (12%): Rs. " << tax << '\n';
        cout << "Total amount      : Rs. " << total << '\n';
        cout << "Payment mode      : Demo / simulated payment only\n";

        int confirm = readInt("Confirm reservation? (1 = Yes, 0 = No): ", 0, 1);
        if (confirm == 0) {
            cout << "Reservation not created.\n";
            return;
        }

        bookings.emplace_back(nextBookingId++, roomNumber, name, phone,
                              in, out, guests, total);
        saveBookings();

        cout << "\nReservation confirmed successfully!\n";
        cout << "Your booking ID is: " << bookings.back().getBookingId() << '\n';
        cout << "Keep this ID for viewing or cancelling your booking.\n";
    }

    void searchAvailableRoomsForBooking(const string& in, const string& out,
                                        int guests) const {
        cout << "\nRooms available for the selected dates:\n";
        printRoomHeader();
        bool found = false;
        for (const auto& room : rooms) {
            if (room.isActive() && room.getCapacity() >= guests &&
                roomAvailable(room.getRoomNumber(), in, out)) {
                room.display();
                found = true;
            }
        }
        if (!found) cout << "No rooms are currently available for these dates.\n";
    }

    void viewBooking() const {
        int id = readInt("\nEnter booking ID: ", 1, 1000000000);
        for (const auto& booking : bookings) {
            if (booking.getBookingId() == id) {
                booking.display();
                return;
            }
        }
        cout << "No booking found with that ID.\n";
    }

    void cancelBooking() {
        int id = readInt("\nEnter booking ID to cancel: ", 1, 1000000000);
        Booking* booking = findBooking(id);

        if (!booking) {
            cout << "Booking ID not found.\n";
            return;
        }
        if (!booking->isConfirmed()) {
            cout << "This booking has already been cancelled.\n";
            return;
        }

        booking->cancel();
        saveBookings();
        cout << "Booking " << id << " has been cancelled.\n";
        cout << "Refunds are not processed in this demo project.\n";
    }

    void showBookingHistory() const {
        cout << "\n================ BOOKING HISTORY ================\n";
        if (bookings.empty()) {
            cout << "No bookings recorded yet.\n";
            return;
        }

        vector<Booking> sorted = bookings;
        sort(sorted.begin(), sorted.end(),
             [](const Booking& a, const Booking& b) {
                 return a.getBookingId() < b.getBookingId();
             });

        for (const auto& booking : sorted) {
            cout << "\nID: " << booking.getBookingId()
                 << " | Guest: " << booking.getGuestName()
                 << " | Room: " << booking.getRoomNumber()
                 << " | " << booking.getCheckIn() << " to " << booking.getCheckOut()
                 << " | Rs. " << fixed << setprecision(2) << booking.getTotalAmount()
                 << " | " << (booking.isConfirmed() ? "CONFIRMED" : "CANCELLED")
                 << '\n';
        }
    }

    void revenueReport() const {
        double confirmedRevenue = 0.0;
        double cancelledValue = 0.0;
        int confirmedCount = 0;
        int cancelledCount = 0;

        for (const auto& booking : bookings) {
            if (booking.isConfirmed()) {
                confirmedRevenue += booking.getTotalAmount();
                ++confirmedCount;
            } else {
                cancelledValue += booking.getTotalAmount();
                ++cancelledCount;
            }
        }

        cout << "\n================ HOTEL REPORT ================\n";
        cout << "Total rooms              : " << rooms.size() << '\n';
        cout << "Total bookings           : " << bookings.size() << '\n';
        cout << "Confirmed bookings       : " << confirmedCount << '\n';
        cout << "Cancelled bookings       : " << cancelledCount << '\n';
        cout << "Confirmed booking value  : Rs. " << fixed << setprecision(2)
             << confirmedRevenue << '\n';
        cout << "Cancelled booking value  : Rs. " << cancelledValue << '\n';
        cout << "\nNote: This is booking value, not verified cash received.\n";
    }

    void updateRoomPrice() {
        int number = readInt("\nEnter room number: ", 1, 999999);
        Room* room = findRoom(number);

        if (!room) {
            cout << "Room not found.\n";
            return;
        }

        cout << "Current price per night: Rs. " << room->getPricePerNight() << '\n';
        double price = readDouble("Enter new price (Rs. 1 to 1000000): ", 1, 1000000);
        room->setPrice(price);
        saveRooms();
        cout << "Room price updated successfully.\n";
    }

    void toggleMaintenance() {
        int number = readInt("\nEnter room number: ", 1, 999999);
        Room* room = findRoom(number);

        if (!room) {
            cout << "Room not found.\n";
            return;
        }

        if (room->isActive()) {
            bool hasFutureBooking = false;
            for (const auto& b : bookings) {
                if (b.isConfirmed() && b.getRoomNumber() == number) {
                    hasFutureBooking = true;
                    break;
                }
            }
            if (hasFutureBooking) {
                cout << "This room has a confirmed booking. Review reservations first.\n";
                return;
            }
        }

        room->setActive(!room->isActive());
        saveRooms();
        cout << "Room status changed to "
             << (room->isActive() ? "Active" : "Maintenance") << ".\n";
    }

public:
    HotelManagementSystem() {
        loadData();
    }

    void run() {
        int choice;
        do {
            cout << "\n\n====================================================\n";
            cout << "       HOTEL MANAGEMENT & RESERVATION SYSTEM\n";
            cout << "====================================================\n";
            cout << " 1. View complete room inventory\n";
            cout << " 2. Search rooms by dates, guests and budget\n";
            cout << " 3. Create a room reservation\n";
            cout << " 4. View booking using booking ID\n";
            cout << " 5. Cancel a reservation\n";
            cout << " 6. View complete booking history\n";
            cout << " 7. Revenue and booking report\n";
            cout << " 8. Update room price (admin demo)\n";
            cout << " 9. Mark room active / maintenance (admin demo)\n";
            cout << "10. Save and exit\n";
            cout << "----------------------------------------------------\n";

            choice = readInt("Select an option: ", 1, 10);

            switch (choice) {
                case 1: showAllRooms(); break;
                case 2: searchAvailableRooms(); break;
                case 3: bookRoom(); break;
                case 4: viewBooking(); break;
                case 5: cancelBooking(); break;
                case 6: showBookingHistory(); break;
                case 7: revenueReport(); break;
                case 8: updateRoomPrice(); break;
                case 9: toggleMaintenance(); break;
                case 10:
                    saveRooms();
                    saveBookings();
                    cout << "Data saved. Thank you for using the system!\n";
                    break;
            }
        } while (choice != 10);
    }
};

int main() {
    HotelManagementSystem hotel;
    hotel.run();
    return 0;
}
