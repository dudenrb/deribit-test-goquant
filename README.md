# GoQuant Project

## Overview
GoQuant is a high-performance trading application designed for algorithmic trading. It includes features such as WebSocket-based market data streaming, order placement, and advanced latency benchmarking. The application is written in C++ and leverages `libcurl` for HTTP communication, WebSocket++ for WebSocket server functionalities, and JSON parsing libraries for data handling.

---
## Project Demo Video

[Click here to watch the demo video](https://drive.google.com/file/d/1xAQGZBiiRBJi4ocfxNWVLuEfbpQgDxwy/view?usp=sharing)


## Features

### Core Functionality
- **Order Placement**: Submit buy/sell orders to the exchange.
- **Market Data Streaming**: Stream real-time market data via WebSocket.
- **Order Book Management**: Fetch and process order book data for specific instruments.
- **Position Management**: Retrieve current positions and P&L.

### Advanced Features
- **Latency Benchmarking**:
  - Measure order placement latency.
  - Monitor market data processing time.
  - Analyze WebSocket message propagation delay.
  - Benchmark end-to-end trading loop latency.

- **Optimizations**:
  - Efficient memory management.
  - Optimized network communication and threading.
  - Use of lightweight data structures for high-speed operations.
  - CPU and concurrency optimizations for low-latency execution.

---

## Requirements

### Dependencies
- **Compiler**: `g++` (GCC) with C++11 support or later.
- **Libraries**:
  - [libcurl](https://curl.se/libcurl/): For HTTP requests.
  - [WebSocket++](https://github.com/zaphoyd/websocketpp): For WebSocket communication.
  - [nlohmann/json](https://github.com/nlohmann/json): For JSON parsing.

### Installation

On Ubuntu or Debian-based systems:
```bash
sudo apt update
sudo apt install g++ libcurl4-openssl-dev
```

Download and include WebSocket++ and nlohmann/json headers in your project directory.

---

## Setup and Build

### Clone Repository
```bash
git clone <repository-url>
cd GoQuant
```

### Compile the Project
```bash
g++ main.cpp -o main -I"/path/to/include/directory" -lcurl
```
- Replace `/path/to/include/directory` with the path to your header files (e.g., WebSocket++, JSON headers).

### Run the Executable
```bash
./main
```

## File Structure
```
GoQuant/
├── main.cpp              # Entry point of the application
├── include/              # Header files (WebSocket++, JSON, custom headers)
├── lib/                  # External libraries (e.g., libcurl)
├── README.md             # Documentation
└── examples/             # Example usage scripts
```

## License
This project is licensed under the MIT License. See the `LICENSE` file for details.

---

## Contact For any questions or feedback, please reach out to 
## [Nikhil Raj / roxstar811307@gmail.com / https://www.linkedin.com/in/nikhil811307/].
