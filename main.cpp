#include <iostream>
#include <string>
#include <curl/curl.h>
#include <chrono>
#include <thread>
#include <map>
#include <mutex>
#include <condition_variable>
#include <json.hpp>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <nlohmann/json.hpp>
using namespace std;
using json = nlohmann::json;
using websocketpp::connection_hdl;

mutex mtx;  // Mutex for thread synchronization
map<string, json> orderBookData;  // Map to store order book data for real-time streaming

// Function to handle the response from the cURL request
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// General function to send a cURL request with optional access token
string sendRequest(const string& url, const json& payload, const string& accessToken = "") {
    string readBuffer;
    CURL* curl;
    CURLcode res;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);  // Set the HTTP method to POST

        // Set the request payload
        string jsonStr = payload.dump();
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonStr.c_str());

        // Set headers, including Authorization if accessToken is provided
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        if (!accessToken.empty()) {
            headers = curl_slist_append(headers, ("Authorization: Bearer " + accessToken).c_str());
        }
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // Set up the write callback to capture the response
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        // Perform the request
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            cerr << "Request failed: " << curl_easy_strerror(res) << endl;
        }

        // Free Resources
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
    return readBuffer;
}

// Function to get the access token
string getAccessToken(const string& clientId, const string& clientSecret) {
    json payload = {
        {"jsonrpc", "2.0"},
        {"id", 1},
        {"method", "public/auth"},
        {"params", {
            {"grant_type", "client_credentials"},
            {"client_id", clientId},
            {"client_secret", clientSecret},
            {"scope", "trade:read_write"}
        }}
    };

    string response = sendRequest("https://test.deribit.com/api/v2/public/auth", payload);
    cout << "Auth Response: " << response << endl;

    try {
        auto responseJson = json::parse(response);
        if (responseJson.contains("result") && responseJson["result"].contains("access_token")) {
            return responseJson["result"]["access_token"];
        } else {
            cerr << "Error in auth response: " << response << endl;
        }
    } catch (const json::parse_error& e) {
        cerr << "JSON Parse Error: " << e.what() << endl;
    }
    return "";
}
int getContractSize(const string& instrument) {
    json payload = {
        {"jsonrpc", "2.0"},
        {"method", "public/get_instruments"},
        {"params", {}},
        {"id", 1}
    };

    string response = sendRequest("https://test.deribit.com/api/v2/public/get_instruments", payload);
    auto responseJson = json::parse(response);

    for (const auto& instrumentInfo : responseJson["result"]) {
        if (instrumentInfo["instrument_name"] == instrument) {
            return instrumentInfo["contract_size"];
        }
    }

    cerr << "Error: Could not find contract size for " << instrument << endl;
    return 0; // Return 0 if not found
}
// Function to place an order
string orderId; // Declare globally to reuse it

// Place an order and capture the order ID
void placeOrder(const string& price, const string& accessToken, const string& amount, const string& instrument) {
    json payload = {
        {"jsonrpc", "2.0"},
        {"method", "private/buy"},
        {"params", {
            {"instrument_name", instrument},
            {"type", "limit"},
            {"price", price},
            {"amount", amount}
        }},
        {"id", 1}
    };

    string response = sendRequest("https://test.deribit.com/api/v2/private/buy", payload, accessToken);
    cout << "Place Order Response: " << response << endl;

    try {
        auto responseJson = json::parse(response);
        if (responseJson.contains("result") && responseJson["result"].contains("order")) {
            orderId = responseJson["result"]["order"]["order_id"];
            cout << "Captured Order ID: " << orderId << endl;
        } else {
            cerr << "Failed to capture order ID." << endl;
        }
    } catch (const json::parse_error& e) {
        cerr << "JSON Parse Error: " << e.what() << endl;
    }
}

// Always check open orders before modifying/canceling
void getOpenOrdersAndValidate(const string& accessToken) {
    json payload = {
        {"jsonrpc", "2.0"},
        {"method", "private/get_open_orders"},
        {"params", {}},
        {"id", 25}
    };

    string response = sendRequest("https://test.deribit.com/api/v2/private/get_open_orders", payload, accessToken);
    cout << "Open Orders Response: " << response << endl;

    auto responseJson = json::parse(response);
    if (responseJson.contains("result") && !responseJson["result"].empty()) {
        for (const auto& order : responseJson["result"]) {
            cout << "Order ID: " << order["order_id"] << endl;
        }
    } else {
        cerr << "No open orders available." << endl;
    }
}

// Function to cancel an order
void cancelOrder(const string& accessToken, const string& orderID) {
    json payload = {
        {"jsonrpc", "2.0"},
        {"method", "private/cancel"},
        {"params", {{"order_id", orderID}}},
        {"id", 6}
    };

    string response = sendRequest("https://test.deribit.com/api/v2/private/cancel", payload,accessToken);
    cout << "Cancel Order Response: " << response << endl;
}

// Function to modify an order
void modifyOrder(const string& accessToken, const string& orderID, int amount, double price) {
    json payload = {
        {"jsonrpc", "2.0"},
        {"method", "private/edit"},
        {"params", {
            {"order_id", orderID},
            {"amount", amount},
            {"price", price}
        }},
        {"id", 11}
    };

    string response = sendRequest("https://test.deribit.com/api/v2/private/edit", payload,accessToken);
    cout << "Modify Order Response: " << response << endl;
}

// Function to get the order book
void getOrderBook(const string& accessToken, const string& instrument) {
    json payload = {
        {"jsonrpc", "2.0"},
        {"method", "public/get_order_book"},
        {"params", {{"instrument_name", instrument}}},
        {"id", 15}
    };

    string response = sendRequest("https://test.deribit.com/api/v2/public/get_order_book", payload, accessToken);
    auto responseJson = json::parse(response);

    cout << "Order Book for " << instrument << ":\n\n";
    cout << "Best Bid Price: " << responseJson["result"]["best_bid_price"] << ", Amount: " << responseJson["result"]["best_bid_amount"] << '\n';
    cout << "Best Ask Price: " << responseJson["result"]["best_ask_price"] << ", Amount: " << responseJson["result"]["best_ask_amount"] << '\n';
}

// function to get open orders details
void getOpenOrders(const string& accessToken) {
    json payload = {
        {"jsonrpc", "2.0"},
        {"method", "private/get_open_orders"},
        {"params", {{"kind", "future"}, {"type", "limit"}}},
        {"id", 25}
    };

    string response = sendRequest("https://test.deribit.com/api/v2/private/get_open_orders", payload, accessToken);
    auto responseJson = json::parse(response);

    // Check if the response contains the "result" array
    if (responseJson.contains("result")) {
        cout << "Open Orders:\n\n";
        for (const auto& order : responseJson["result"]) {
            string instrument = order["instrument_name"];
            string orderId = order["order_id"];
            double price = order["price"];
            double amount = order["amount"];

            cout << "Instrument: " << instrument << ", Order ID: " << orderId
                      << ", Price: " << price << ", Amount: " << amount << '\n';
            
            // Example of calling modifyOrder function with the correct order ID
            modifyOrder(accessToken, orderId, amount, price);  // Modify the order with the correct details
        }
    } else {
        cerr << "Error: Could not retrieve open orders." << endl;
    }
}

// Function to get current positions
void getPosition(const string& accessToken, const string& instrument) {
    json payload = {
        {"jsonrpc", "2.0"},
        {"method", "private/get_position"},
        {"params", {{"instrument_name", instrument}}},
        {"id", 20}
    };

    string response = sendRequest("https://test.deribit.com/api/v2/private/get_position", payload, accessToken);
    auto responseJson = json::parse(response);

    if (responseJson.contains("result")) {
        cout << "Position Details for " << instrument << ":\n\n";
        auto result = responseJson["result"];
        cout << "Estimated Liquidation Price: " << result["estimated_liquidation_price"] << '\n';
        cout << "Size Currency: " << result["size_currency"] << '\n';
        cout << "Direction: " << result["direction"] << '\n';
        cout << "Open Orders: " << result["open_orders"] << '\n';
    } else {
        cout << "No position found for " << instrument << ".\n";
    }
}

// WebSocket functions for real-time data streaming
class WebSocketServer {
public:
    void start() {
        server.set_access_channels(websocketpp::log::alevel::all);
        server.set_error_channels(websocketpp::log::elevel::all);

        server.init_asio();
        server.set_open_handler(bind(&WebSocketServer::on_open, this, placeholders::_1));
        server.set_close_handler(bind(&WebSocketServer::on_close, this, placeholders::_1));
        server.set_message_handler(bind(&WebSocketServer::on_message, this, placeholders::_1, placeholders::_2));

        server.listen(9002);  // Start server on port 9002
        server.start_accept();

        cout << "WebSocket Server started on ws://localhost:9002" << endl;
        server.run();
    }

private:
    websocketpp::server<websocketpp::config::asio> server;

    void on_open(connection_hdl hdl) {
        lock_guard<mutex> lock(mtx);
        cout << "Client connected" << endl;
    }

    void on_close(connection_hdl hdl) {
        lock_guard<mutex> lock(mtx);
        cout << "Client disconnected" << endl;
    }

    void on_message(connection_hdl hdl, websocketpp::server<websocketpp::config::asio>::message_ptr msg) {
        string message = msg->get_payload();
        json jmsg = json::parse(message);

        // Subscribe to order book updates for specific symbols
        if (jmsg.contains("subscribe")) {
            string symbol = jmsg["subscribe"];
            cout << "Subscribed to: " << symbol << endl;

            // Stream order book data to the client
            json orderBookUpdate = getOrderBookData(symbol);
            server.send(hdl, orderBookUpdate.dump(), websocketpp::frame::opcode::text);
        }
    }

    json getOrderBookData(const string& symbol) {
        // Fetch order book data for the subscribed symbol
        lock_guard<mutex> lock(mtx);
        if (orderBookData.find(symbol) != orderBookData.end()) {
            return orderBookData[symbol];
        }

        // Placeholder: Return empty object if no data
        return json{};
    }
};












// Main function to test everything
int main() {
    string clientId = "whg5zI7E";
    string clientSecret = "ccG_kYvpnbSTs50q2tTTk8K267vCsuva4MI9XHMPP1E";
    string accessToken = getAccessToken(clientId, clientSecret);

    if (!accessToken.empty()) {
        // WebSocket Server in separate thread
        thread wsThread([]() {
            WebSocketServer server;
            server.start();
        });

        //Example usage: Place an order
      // cout << "Placing an order..." << endl;
       //placeOrder("45000", accessToken, "100", "BTC-PERPETUAL");

        // Simulate modifying an order
        //cout << "Modifying the order..." << endl;
       // modifyOrder(accessToken, "30815193671", 150, 40000.0);

        // Simulate canceling an order
        cout << "Canceling the order..." << endl;
        cancelOrder(accessToken, "30815193671");


        // Get order book for an instrument
        //cout << "Fetching Order Book..." << endl;
        //getOrderBook(accessToken, "BTC-PERPETUAL");

        // Get position for an instrument
        //cout << "Fetching Position..." << endl;
       // getPosition(accessToken, "BTC-PERPETUAL");

       // Get open orders after placing
        cout << "Fetching Open Orders..." << endl;
        getOpenOrders(accessToken);

        // Wait for WebSocket server to finish
        wsThread.join();
    }

    return 0;
}

