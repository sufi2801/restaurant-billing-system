🧾 Restaurant Order & Billing System (C Project)

📌 Overview
This is a complete restaurant management system written in C, designed to handle:
	•	Menu display by category
	•	Order creation (Dine-In or Takeaway)
	•	Order modification (Add / Remove / Update items)
	•	Automated bill generation with taxes, service charges, and discounts
	•	Receipt file generation for each order (KOT → Bill)
	•	Table management for up to 50 tables

Ideal for students or developers learning file handling, arrays, and structured programming in C.

⸻

⚙️ Key Features

✅ Digital Menu System
	•	Categorized menu (Starters, Main Course, Beverages, Desserts)
	•	Each item has a unique code, name, price, and availability toggle (admin option)

✅ Order Management
	•	Create, edit, and close multiple orders simultaneously
	•	Dine-In and Takeaway supported
	•	Each order gets a unique Kitchen Order Token (KOT)

✅ Billing & Calculation
	•	GST (5% on food items only)
	•	Service charge (10% for dine-in)
	•	Discounts:
	•	10% off if total > ₹1000
	•	15% off if total > ₹2000
	•	Shows the discount percentage both on-screen and in the saved receipt file

✅ Table Management
	•	Manage up to 50 tables
	•	Shows which tables are free or occupied

✅ Receipts
	•	Each finalized order automatically saves a receipt_<KOT>.txt file
	•	Itemized bill with date/time, subtotal, tax, and discounts

⸻

🧑‍💻 How to Run

On macOS / Linux:
gcc restaurant_system_full_with_discount_percent.c -o restaurant_system
./restaurant_system


On windows: 
gcc restaurant_system_full_with_discount_percent.c -o restaurant_system.exe
restaurant_system.exe

You’ll see the main menu:
====== Restaurant Management System ======
1. View Full Menu
2. Create New Order (Dine-in / Takeaway)
3. Modify Existing Order (Add / Remove / Update qty)
4. Generate Bill & Close Order (KOT -> Receipt)
5. List Active Orders
6. Table Status (50 tables)
7. Toggle Item Availability (Admin)
8. Exit

💡 Example Flow
	1.	View the menu
	2.	Create a Dine-In order for table 5
	3.	Add items (like M03 for Hyderabadi Chicken Biryani)
	4.	Finish adding → Order saved
	5.	Later, generate the bill → receipt is printed and saved as receipt_<KOT>.txt

Example of generated receipt file:
========================================
               BILL / RECEIPT
KOT: 9001
Type: Dine-In
Table: 5
Date/Time: Sat Nov 2 14:42:31 2025
----------------------------------------
Code   Item                      Qty    Amount
----------------------------------------
M03    Hyderabadi Chicken Biryani 2     560.00
B02    Cold Coffee                1     120.00
----------------------------------------
Subtotal:         680.00
GST (5% on food): 28.00
Service:          68.00
Discount (10%):   77.60
TOTAL:            698.40
========================================