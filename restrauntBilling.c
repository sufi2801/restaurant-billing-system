/*
  restaurant_system_full_with_discount_percent.c

  Full-featured interactive Restaurant Order & Billing System in C
  - Digital menu with categories (Starters, Main Course, Beverages, Desserts)
  - Menu items: unique code, name, price, availability
  - Multi-item order processing with quantity selection
  - Order modification: add items, remove items, update quantity
  - Bill calc: subtotal, GST (5% on food items), service (10% dine-in),
               discounts (10% if >1000, 15% if >2000)
  - Table management for up to 50 tables
  - Kitchen Order Token (KOT) generation
  - Detailed itemized bill receipt and file output
  - Shows discount percentage applied on both printed bill and saved receipt
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define MAX_MENU 80
#define MAX_ORDERS 500
#define MAX_ITEMS_PER_ORDER 60
#define MAX_TABLES 50

#define CODE_LEN 6   /* includes terminating NUL */
#define NAME_LEN 64
#define RECEIPT_FILENAME_LEN 64

/* Rates and thresholds */
#define GST_RATE_FOOD 0.05f
#define SERVICE_RATE 0.10f
#define DISCOUNT_TIER1_THRESHOLD 1000.0f
#define DISCOUNT_TIER1_RATE 0.10f
#define DISCOUNT_TIER2_THRESHOLD 2000.0f
#define DISCOUNT_TIER2_RATE 0.15f

typedef enum { STARTER=1, MAIN_COURSE=2, BEVERAGE=3, DESSERT=4 } Category;

typedef struct {
    char code[CODE_LEN];
    char name[NAME_LEN];
    Category category;
    float price;
    int available;   /* 1 = available, 0 = not available */
} MenuItem;

typedef struct {
    char code[CODE_LEN];
    int qty;
} OrderItem;

typedef struct {
    int orderId;                 /* KOT */
    int dineIn;                  /* 1 dine-in, 0 takeaway */
    int tableNumber;             /* 1..MAX_TABLES if dine-in else 0 */
    OrderItem items[MAX_ITEMS_PER_ORDER];
    int itemCount;
    time_t timestamp;
    int active;                  /* 1 = active, 0 = billed/closed */
} Order;

typedef struct {
    float subtotal;
    float gst;
    float serviceCharge;
    float discount;
    float total;
} Bill;

/* Global storage */
static MenuItem menuList[MAX_MENU];
static int menuCount = 0;
static Order orders[MAX_ORDERS];
static int orderCount = 0;
static int nextOrderId = 9001;
static int tableOrderIndex[MAX_TABLES]; /* -1 if free else orders[index] */

/* Utility prototypes */
void initMenu(void);
void printMenuAll(void);
void printMenuByCategory(Category c);
int findMenuIndexByCode(const char* code);
int createOrder(int dineIn, int tableNumber);
int addItemToOrder(int orderIdx, const char* code, int qty);
int removeItemFromOrder(int orderIdx, const char* code);
int updateItemQtyInOrder(int orderIdx, const char* code, int newQty);
Bill calculateBill(int orderIdx);
void printBill(int orderIdx);
void saveReceiptToFile(int orderIdx, Bill b);
void listActiveOrders(void);
int findOrderIndexById(int orderId);
void showTableStatus(void);
void clearInputBuffer(void);

/* Implementation */

void clearInputBuffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) { }
}

void addMenuItem(const char* code, const char* name, Category cat, float price, int avail) {
    if (menuCount >= MAX_MENU) return;
    strncpy(menuList[menuCount].code, code, CODE_LEN-1);
    menuList[menuCount].code[CODE_LEN-1] = '\0';
    strncpy(menuList[menuCount].name, name, NAME_LEN-1);
    menuList[menuCount].name[NAME_LEN-1] = '\0';
    menuList[menuCount].category = cat;
    menuList[menuCount].price = price;
    menuList[menuCount].available = avail ? 1 : 0;
    menuCount++;
}

void initMenu(void) {
    /* Ensure we have 25+ items across categories */
    addMenuItem("S01","Garlic Bread", STARTER, 120.00f, 1);
    addMenuItem("S02","Veg Spring Roll", STARTER, 140.00f, 1);
    addMenuItem("S03","Chicken Tikka", STARTER, 260.00f, 1);
    addMenuItem("S04","Paneer Tikka", STARTER, 220.00f, 1);
    addMenuItem("S05","French Fries", STARTER, 130.00f, 1);
    addMenuItem("S06","Chicken Wings", STARTER, 290.00f, 1);
    addMenuItem("S07","Masala Papad", STARTER, 60.00f, 1);

    addMenuItem("M01","Butter Chicken", MAIN_COURSE, 320.00f, 1);
    addMenuItem("M02","Paneer Butter Masala", MAIN_COURSE, 300.00f, 1);
    addMenuItem("M03","Hyderabadi Chicken Biryani", MAIN_COURSE, 280.00f, 1);
    addMenuItem("M04","Veg Biryani", MAIN_COURSE, 240.00f, 1);
    addMenuItem("M05","Margherita Pizza", MAIN_COURSE, 350.00f, 1);
    addMenuItem("M06","Farmhouse Pizza", MAIN_COURSE, 420.00f, 1);
    addMenuItem("M07","Grilled Fish", MAIN_COURSE, 380.00f, 1);
    addMenuItem("M08","Chicken Fried Rice", MAIN_COURSE, 220.00f, 1);
    addMenuItem("M09","Mixed Veg Curry + Roti", MAIN_COURSE, 180.00f, 1);

    addMenuItem("B01","Masala Chai", BEVERAGE, 40.00f, 1);
    addMenuItem("B02","Cold Coffee", BEVERAGE, 120.00f, 1);
    addMenuItem("B03","Mango Lassi", BEVERAGE, 110.00f, 1);
    addMenuItem("B04","Soft Drink (500ml)", BEVERAGE, 80.00f, 1);
    addMenuItem("B05","Lemonade", BEVERAGE, 85.00f, 1);
    addMenuItem("B06","Mineral Water (1L)", BEVERAGE, 50.00f, 1);

    addMenuItem("D01","Gulab Jamun (2 pcs)", DESSERT, 90.00f, 1);
    addMenuItem("D02","Brownie with Ice Cream", DESSERT, 210.00f, 1);
    addMenuItem("D03","Rasmalai (2 pcs)", DESSERT, 130.00f, 1);
    addMenuItem("D04","Fruit Salad", DESSERT, 150.00f, 1);
    addMenuItem("D05","Kulfi", DESSERT, 110.00f, 1);
    addMenuItem("D06","Ice Cream Scoop", DESSERT, 70.00f, 1);
    addMenuItem("D07","Jalebi (2 pcs)", DESSERT, 95.00f, 1);

    /* initialize table mapping as free */
    for (int i=0;i<MAX_TABLES;i++) tableOrderIndex[i] = -1;
}

/* Show entire menu grouped by category */
void printMenuAll(void) {
    printf("\n========== MENU ==========\n");
    printf("Starters:\n");
    printMenuByCategory(STARTER);
    printf("\nMain Course:\n");
    printMenuByCategory(MAIN_COURSE);
    printf("\nBeverages:\n");
    printMenuByCategory(BEVERAGE);
    printf("\nDesserts:\n");
    printMenuByCategory(DESSERT);
    printf("==========================\n");
}

void printMenuByCategory(Category c) {
    printf("Code  | %-20s | Price  | Avail\n", "Name");
    printf("-----------------------------------------------\n");
    for (int i=0;i<menuCount;i++) {
        if (menuList[i].category == c) {
            printf("%-5s | %-20s | %6.2f | %s\n",
                   menuList[i].code,
                   menuList[i].name,
                   menuList[i].price,
                   menuList[i].available ? "Yes" : "No");
        }
    }
}

/* find index in menuList by code, -1 if not found */
int findMenuIndexByCode(const char* code) {
    for (int i=0;i<menuCount;i++) {
        if (strcmp(menuList[i].code, code) == 0) return i;
    }
    return -1;
}

/* create a new order; returns order index in orders[] or -1 on fail */
int createOrder(int dineIn, int tableNumber) {
    if (orderCount >= MAX_ORDERS) return -1;
    if (dineIn) {
        if (tableNumber < 1 || tableNumber > MAX_TABLES) return -1;
        if (tableOrderIndex[tableNumber-1] != -1) return -1; /* table occupied */
    }
    int idx = orderCount++;
    orders[idx].orderId = nextOrderId++;
    orders[idx].dineIn = dineIn ? 1 : 0;
    orders[idx].tableNumber = dineIn ? tableNumber : 0;
    orders[idx].itemCount = 0;
    orders[idx].timestamp = time(NULL);
    orders[idx].active = 1;
    if (dineIn) tableOrderIndex[tableNumber-1] = idx;
    return idx;
}

/* Add item to order (append or increase qty if exists).
   returns 0 on success, -1 invalid, -2 if full order items */
int addItemToOrder(int orderIdx, const char* code, int qty) {
    if (qty <= 0) return -1;
    if (orderIdx < 0 || orderIdx >= orderCount) return -1;
    if (!orders[orderIdx].active) return -1;
    int midx = findMenuIndexByCode(code);
    if (midx == -1) return -1;
    if (!menuList[midx].available) return -1;
    /* if exists, increment qty */
    for (int i=0;i<orders[orderIdx].itemCount;i++) {
        if (strcmp(orders[orderIdx].items[i].code, code) == 0) {
            orders[orderIdx].items[i].qty += qty;
            return 0;
        }
    }
    if (orders[orderIdx].itemCount >= MAX_ITEMS_PER_ORDER) return -2;
    strncpy(orders[orderIdx].items[orders[orderIdx].itemCount].code, code, CODE_LEN-1);
    orders[orderIdx].items[orders[orderIdx].itemCount].code[CODE_LEN-1] = '\0';
    orders[orderIdx].items[orders[orderIdx].itemCount].qty = qty;
    orders[orderIdx].itemCount++;
    return 0;
}

/* Remove item from order by code. Returns 0 success, -1 not found */
int removeItemFromOrder(int orderIdx, const char* code) {
    if (orderIdx < 0 || orderIdx >= orderCount) return -1;
    if (!orders[orderIdx].active) return -1;
    for (int i=0;i<orders[orderIdx].itemCount;i++) {
        if (strcmp(orders[orderIdx].items[i].code, code) == 0) {
            /* shift left */
            for (int j=i;j<orders[orderIdx].itemCount-1;j++) {
                orders[orderIdx].items[j] = orders[orderIdx].items[j+1];
            }
            orders[orderIdx].itemCount--;
            return 0;
        }
    }
    return -1;
}

/* Update quantity of an item in order. newQty==0 removes item.
   Returns 0 success, -1 not found */
int updateItemQtyInOrder(int orderIdx, const char* code, int newQty) {
    if (orderIdx < 0 || orderIdx >= orderCount) return -1;
    if (!orders[orderIdx].active) return -1;
    for (int i=0;i<orders[orderIdx].itemCount;i++) {
        if (strcmp(orders[orderIdx].items[i].code, code) == 0) {
            if (newQty <= 0) {
                return removeItemFromOrder(orderIdx, code);
            } else {
                orders[orderIdx].items[i].qty = newQty;
                return 0;
            }
        }
    }
    return -1;
}

/* Calculate bill. GST applied only on non-beverage items. */
Bill calculateBill(int orderIdx) {
    Bill b = {0.0f,0.0f,0.0f,0.0f,0.0f};
    if (orderIdx < 0 || orderIdx >= orderCount) return b;
    Order *o = &orders[orderIdx];
    float foodSubtotal = 0.0f;
    for (int i=0;i<o->itemCount;i++) {
        int m = findMenuIndexByCode(o->items[i].code);
        if (m == -1) continue;
        float line = menuList[m].price * (float)o->items[i].qty; /* int->float conversion */
        b.subtotal += line;
        if (menuList[m].category != BEVERAGE) foodSubtotal += line;
    }
    b.gst = foodSubtotal * GST_RATE_FOOD;
    b.serviceCharge = o->dineIn ? (b.subtotal * SERVICE_RATE) : 0.0f;
    float temp = b.subtotal + b.gst + b.serviceCharge;
    if (temp > DISCOUNT_TIER2_THRESHOLD) b.discount = temp * DISCOUNT_TIER2_RATE;
    else if (temp > DISCOUNT_TIER1_THRESHOLD) b.discount = temp * DISCOUNT_TIER1_RATE;
    else b.discount = 0.0f;
    b.total = temp - b.discount;
    return b;
}

/* Print a nicely formatted bill to stdout and save to file */
void printBill(int orderIdx) {
    if (orderIdx < 0 || orderIdx >= orderCount) {
        printf("Invalid order index.\n");
        return;
    }
    Order *o = &orders[orderIdx];
    if (!o->active) {
        printf("Order already billed/closed.\n");
        return;
    }
    Bill b = calculateBill(orderIdx);

    /* Determine discount percentage for display */
    float discountPercent = 0.0f;
    float preDiscountTotal = b.subtotal + b.gst + b.serviceCharge;
    if (preDiscountTotal > DISCOUNT_TIER2_THRESHOLD)
        discountPercent = DISCOUNT_TIER2_RATE * 100.0f;
    else if (preDiscountTotal > DISCOUNT_TIER1_THRESHOLD)
        discountPercent = DISCOUNT_TIER1_RATE * 100.0f;
    else
        discountPercent = 0.0f;

    printf("\n========================================\n");
    printf("               BILL / RECEIPT           \n");
    printf("KOT: %d\n", o->orderId);
    printf("Type: %s\n", o->dineIn ? "Dine-In" : "Takeaway");
    if (o->dineIn) printf("Table: %d\n", o->tableNumber);
    printf("Date/Time: %s", ctime(&o->timestamp));
    printf("----------------------------------------\n");
    printf("%-6s %-25s %-6s %-8s\n", "Code", "Item", "Qty", "Amount");
    printf("----------------------------------------\n");
    for (int i=0;i<o->itemCount;i++) {
        int m = findMenuIndexByCode(o->items[i].code);
        if (m == -1) continue;
        float line = menuList[m].price * (float)o->items[i].qty;
        printf("%-6s %-25s %-6d %-8.2f\n",
               menuList[m].code, menuList[m].name, o->items[i].qty, line);
    }
    printf("----------------------------------------\n");
    printf("Subtotal:        %8.2f\n", b.subtotal);
    printf("GST (5%% on food):%8.2f\n", b.gst);
    printf("Service:         %8.2f\n", b.serviceCharge);

    if (discountPercent > 0.0f) {
        printf("Discount (%.0f%%):  %8.2f\n", discountPercent, b.discount);
    } else {
        printf("Discount:        %8.2f\n", b.discount);
    }

    printf("TOTAL:           %8.2f\n", b.total);
    printf("========================================\n");

    saveReceiptToFile(orderIdx, b);

    /* mark order closed and free table if dine-in */
    o->active = 0;
    if (o->dineIn && o->tableNumber >=1 && o->tableNumber <= MAX_TABLES) {
        tableOrderIndex[o->tableNumber-1] = -1;
    }
}

/* Save receipt to file receipt_<orderId>.txt */
void saveReceiptToFile(int orderIdx, Bill b) {
    char fname[RECEIPT_FILENAME_LEN];
    Order *o = &orders[orderIdx];

    /* Determine discount percentage for file display */
    float discountPercent = 0.0f;
    float preDiscountTotal = b.subtotal + b.gst + b.serviceCharge;
    if (preDiscountTotal > DISCOUNT_TIER2_THRESHOLD)
        discountPercent = DISCOUNT_TIER2_RATE * 100.0f;
    else if (preDiscountTotal > DISCOUNT_TIER1_THRESHOLD)
        discountPercent = DISCOUNT_TIER1_RATE * 100.0f;
    else
        discountPercent = 0.0f;

    snprintf(fname, sizeof(fname), "receipt_%d.txt", o->orderId);
    FILE *f = fopen(fname, "w");
    if (!f) {
        printf("Failed to write receipt to file.\n");
        return;
    }
    fprintf(f, "========================================\n");
    fprintf(f, "               BILL / RECEIPT           \n");
    fprintf(f, "KOT: %d\n", o->orderId);
    fprintf(f, "Type: %s\n", o->dineIn ? "Dine-In" : "Takeaway");
    if (o->dineIn) fprintf(f, "Table: %d\n", o->tableNumber);
    fprintf(f, "Date/Time: %s", ctime(&o->timestamp));
    fprintf(f, "----------------------------------------\n");
    fprintf(f, "%-6s %-25s %-6s %-8s\n", "Code", "Item", "Qty", "Amount");
    fprintf(f, "----------------------------------------\n");
    for (int i=0;i<o->itemCount;i++) {
        int m = findMenuIndexByCode(o->items[i].code);
        if (m == -1) continue;
        float line = menuList[m].price * (float)o->items[i].qty;
        fprintf(f, "%-6s %-25s %-6d %-8.2f\n",
               menuList[m].code, menuList[m].name, o->items[i].qty, line);
    }
    fprintf(f, "----------------------------------------\n");
    fprintf(f, "Subtotal:        %8.2f\n", b.subtotal);
    fprintf(f, "GST (5%% on food):%8.2f\n", b.gst);
    fprintf(f, "Service:         %8.2f\n", b.serviceCharge);

    if (discountPercent > 0.0f) {
        fprintf(f, "Discount (%.0f%%):  %8.2f\n", discountPercent, b.discount);
    } else {
        fprintf(f, "Discount:        %8.2f\n", b.discount);
    }

    fprintf(f, "TOTAL:           %8.2f\n", b.total);
    fprintf(f, "========================================\n");
    fclose(f);
    printf("Receipt saved to: %s\n", fname);
}

/* List active orders summary */
void listActiveOrders(void) {
    printf("\nActive Orders:\n");
    printf("KOT   | Type     | Table | Items | Time\n");
    printf("----------------------------------------------\n");
    for (int i=0;i<orderCount;i++) {
        if (!orders[i].active) continue;
        printf("%-5d | %-8s | %-5d | %-5d | %s",
               orders[i].orderId,
               orders[i].dineIn ? "Dine-In" : "Takeaway",
               orders[i].tableNumber,
               orders[i].itemCount,
               ctime(&orders[i].timestamp));
    }
}

/* Find order index by KOT (orderId) */
int findOrderIndexById(int orderId) {
    for (int i=0;i<orderCount;i++) {
        if (orders[i].orderId == orderId) return i;
    }
    return -1;
}

/* Show table occupancy */
void showTableStatus(void) {
    printf("\nTable Status (1..%d):\n", MAX_TABLES);
    for (int i=0;i<MAX_TABLES;i++) {
        if (tableOrderIndex[i] == -1) {
            printf("Table %2d: Free\n", i+1);
        } else {
            int oi = tableOrderIndex[i];
            printf("Table %2d: Occupied (KOT %d, items %d)\n", i+1, orders[oi].orderId, orders[oi].itemCount);
        }
    }
}

/* --- Interactive menu-driven flow --- */
int main(void) {
    initMenu();

    while (1) {
        printf("\n====== Restaurant Management System ======\n");
        printf("1. View Full Menu\n");
        printf("2. Create New Order (Dine-in / Takeaway)\n");
        printf("3. Modify Existing Order (Add / Remove / Update qty)\n");
        printf("4. Generate Bill & Close Order (KOT -> Receipt)\n");
        printf("5. List Active Orders\n");
        printf("6. Table Status (50 tables)\n");
        printf("7. Toggle Item Availability (Admin)\n");
        printf("8. Exit\n");
        printf("Choose option: ");
        int opt = 0;
        if (scanf("%d", &opt) != 1) {
            clearInputBuffer();
            printf("Invalid input.\n");
            continue;
        }
        clearInputBuffer();

        if (opt == 1) {
            printMenuAll();
        }
        else if (opt == 2) {
            printf("Dine-In (1) or Takeaway (0)? ");
            int dineIn = -1;
            if (scanf("%d", &dineIn) != 1) { clearInputBuffer(); printf("Invalid.\n"); continue; }
            clearInputBuffer();
            int tableNo = 0;
            if (dineIn == 1) {
                printf("Enter table number (1..%d): ", MAX_TABLES);
                if (scanf("%d", &tableNo) != 1) { clearInputBuffer(); printf("Invalid.\n"); continue; }
                clearInputBuffer();
                if (tableNo < 1 || tableNo > MAX_TABLES) { printf("Invalid table.\n"); continue; }
                if (tableOrderIndex[tableNo-1] != -1) { printf("Table occupied.\n"); continue; }
            } else {
                dineIn = 0;
            }
            int idx = createOrder(dineIn, tableNo);
            if (idx == -1) { printf("Failed to create order.\n"); continue; }
            printf("Created Order KOT: %d\n", orders[idx].orderId);

            /* add items loop */
            while (1) {
                printMenuAll();
                char code[CODE_LEN];
                printf("Enter item code to add (or 0 to finish): ");
                if (fgets(code, sizeof(code), stdin) == NULL) break;
                code[strcspn(code, "\n")] = '\0';
                if (strcmp(code, "0") == 0) break;
                int midx = findMenuIndexByCode(code);
                if (midx == -1) {
                    printf("Invalid item code.\n");
                    continue;
                }
                int qty = 0;
                printf("Enter quantity: ");
                if (scanf("%d", &qty) != 1) { clearInputBuffer(); printf("Invalid.\n"); continue; }
                clearInputBuffer();
                int ret = addItemToOrder(idx, code, qty);
                if (ret == 0) printf("Added.\n");
                else if (ret == -2) { printf("Order items full.\n"); break; }
                else printf("Failed to add item.\n");
            }

            printf("Order saved. KOT: %d\n", orders[idx].orderId);
        }
        else if (opt == 3) {
            printf("Enter KOT (order id) to modify: ");
            int kot;
            if (scanf("%d", &kot) != 1) { clearInputBuffer(); printf("Invalid.\n"); continue; }
            clearInputBuffer();
            int oidx = findOrderIndexById(kot);
            if (oidx == -1) { printf("Order not found.\n"); continue; }
            if (!orders[oidx].active) { printf("Order already closed.\n"); continue; }

            while (1) {
                printf("\nModify Order KOT %d\n", kot);
                printf("1. Add Item\n2. Remove Item\n3. Update Item Quantity\n4. Show Order Details\n5. Back\n");
                printf("Choice: ");
                int mopt;
                if (scanf("%d", &mopt) != 1) { clearInputBuffer(); printf("Invalid.\n"); continue; }
                clearInputBuffer();
                if (mopt == 1) {
                    printMenuAll();
                    char code[CODE_LEN];
                    printf("Item code to add: ");
                    if (fgets(code, sizeof(code), stdin) == NULL) continue;
                    code[strcspn(code, "\n")] = '\0';
                    int qty;
                    printf("Quantity: ");
                    if (scanf("%d", &qty) != 1) { clearInputBuffer(); printf("Invalid.\n"); continue; }
                    clearInputBuffer();
                    int res = addItemToOrder(oidx, code, qty);
                    if (res == 0) printf("Added.\n");
                    else printf("Failed to add item.\n");
                } else if (mopt == 2) {
                    printf("Enter item code to remove: ");
                    char code[CODE_LEN];
                    if (fgets(code, sizeof(code), stdin) == NULL) continue;
                    code[strcspn(code, "\n")] = '\0';
                    if (removeItemFromOrder(oidx, code) == 0) printf("Removed.\n");
                    else printf("Item not found.\n");
                } else if (mopt == 3) {
                    printf("Enter item code to update: ");
                    char code[CODE_LEN];
                    if (fgets(code, sizeof(code), stdin) == NULL) continue;
                    code[strcspn(code, "\n")] = '\0';
                    printf("Enter new quantity (0 to remove): ");
                    int nq;
                    if (scanf("%d", &nq) != 1) { clearInputBuffer(); printf("Invalid.\n"); continue; }
                    clearInputBuffer();
                    if (updateItemQtyInOrder(oidx, code, nq) == 0) printf("Updated.\n");
                    else printf("Item not found.\n");
                } else if (mopt == 4) {
                    Order *o = &orders[oidx];
                    printf("\nOrder KOT: %d | Type: %s | Table: %d | Items: %d\n",
                           o->orderId, o->dineIn ? "Dine-In" : "Takeaway", o->tableNumber, o->itemCount);
                    if (o->itemCount == 0) printf("No items.\n");
                    else {
                        printf("%-6s %-25s %-6s %-8s\n","Code","Item","Qty","Amount");
                        for (int i=0;i<o->itemCount;i++) {
                            int m = findMenuIndexByCode(o->items[i].code);
                            if (m==-1) continue;
                            printf("%-6s %-25s %-6d %-8.2f\n", menuList[m].code, menuList[m].name, o->items[i].qty, menuList[m].price * o->items[i].qty);
                        }
                        Bill b = calculateBill(oidx);
                        printf("Subtotal: %.2f | GST: %.2f | Service: %.2f | Discount: %.2f | Total: %.2f\n",
                               b.subtotal, b.gst, b.serviceCharge, b.discount, b.total);
                    }
                } else if (mopt == 5) break;
                else printf("Invalid choice.\n");
            }
        }
        else if (opt == 4) {
            printf("Enter KOT (order id) to bill: ");
            int kot;
            if (scanf("%d", &kot) != 1) { clearInputBuffer(); printf("Invalid.\n"); continue; }
            clearInputBuffer();
            int idx = findOrderIndexById(kot);
            if (idx == -1) { printf("Order not found.\n"); continue; }
            if (orders[idx].itemCount == 0) { printf("Order has no items.\n"); continue; }
            printBill(idx); /* will close order and free table */
        }
        else if (opt == 5) {
            listActiveOrders();
        }
        else if (opt == 6) {
            showTableStatus();
        }
        else if (opt == 7) {
            /* Admin toggle availability */
            printMenuAll();
            printf("Enter item code to toggle availability: ");
            char code[CODE_LEN];
            if (fgets(code, sizeof(code), stdin) == NULL) continue;
            code[strcspn(code, "\n")] = '\0';
            int m = findMenuIndexByCode(code);
            if (m == -1) { printf("Invalid code.\n"); continue; }
            menuList[m].available = !menuList[m].available;
            printf("%s now %s\n", menuList[m].name, menuList[m].available ? "Available" : "Unavailable");
        }
        else if (opt == 8) {
            printf("Exiting...\n");
            break;
        }
        else {
            printf("Invalid option.\n");
        }
    }

    return 0;
}